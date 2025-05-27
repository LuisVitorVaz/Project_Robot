#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "driver/ledc.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Definições dos pinos para ESP32
// Motor Esquerdo (controlado pelos pinos 0-3)
#define PIN_ESQUERDA_FRENTE_IN1    25  // IN1 para o motor esquerdo frente
#define PIN_ESQUERDA_FRENTE_IN2    26  // IN2 para o motor esquerdo frente
#define PIN_ESQUERDA_TRAS_IN3      27  // IN3 para o motor esquerdo traseiro
#define PIN_ESQUERDA_TRAS_IN4      14  // IN4 para o motor esquerdo traseiro

// Motor Direito (controlado pelos pinos 4-7)
#define PIN_DIREITA_FRENTE_IN1     12  // IN1 para o motor direito frente
#define PIN_DIREITA_FRENTE_IN2     13  // IN2 para o motor direito frente
#define PIN_DIREITA_TRAS_IN3       4   // IN3 para o motor direito traseiro
#define PIN_DIREITA_TRAS_IN4       15  // IN4 para o motor direito traseiro

// Configurações de PWM
#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_DUTY_RES           LEDC_TIMER_8_BIT    // Resolução de 8 bits (0-255)
#define LEDC_FREQUENCY          300                 // Frequência inicial (300Hz)
#define LEDC_DUTY_MAX           255                 // Valor máximo (100%)

// Definição dos canais do LEDC para cada motor
#define CHANNEL_ESQUERDA_FRENTE_IN1  0
#define CHANNEL_ESQUERDA_FRENTE_IN2  1
#define CHANNEL_ESQUERDA_TRAS_IN3    2
#define CHANNEL_ESQUERDA_TRAS_IN4    3
#define CHANNEL_DIREITA_FRENTE_IN1   4
#define CHANNEL_DIREITA_FRENTE_IN2   5
#define CHANNEL_DIREITA_TRAS_IN3     6
#define CHANNEL_DIREITA_TRAS_IN4     7

// Estados do carrinho
typedef enum {
    PARADO,
    FRENTE,
    TRAS,
    ESQUERDA,
    DIREITA
} EstadoCarrinho;
// Estrutura para configuração do PWM
typedef struct {
    uint8_t pin;           // Número do pino GPIO
    uint8_t canal;         // Canal LEDC
    bool estado;           // Estado atual (ligado/desligado)
    uint16_t frequencia;   // Frequência em Hz
    uint8_t ciclo_trabalho; // Ciclo de trabalho (0-100%)
} ConfigPWM;

// Variáveis globais
EstadoCarrinho estado_atual = PARADO;
uint8_t velocidade_padrao_frente = 80;      // 80% da velocidade máxima para frente (após aceleração)
uint8_t velocidade_padrao_tras = 80;        // 80% da velocidade máxima para trás (após aceleração)
uint8_t velocidade_curva_lado_externo = 80; // Motor externo na curva
uint8_t velocidade_curva_lado_interno = 40; // Motor interno na curva
uint8_t velocidade_aceleracao = 100;        // Velocidade inicial para aceleração (100%)
uint16_t tempo_aceleracao_ms = 500;         // Tempo de aceleração em ms


// Configuração inicial dos pinos PWM
ConfigPWM pinos_pwm[8] = {
    {PIN_ESQUERDA_FRENTE_IN1, CHANNEL_ESQUERDA_FRENTE_IN1, false, LEDC_FREQUENCY, 0},
    {PIN_ESQUERDA_FRENTE_IN2, CHANNEL_ESQUERDA_FRENTE_IN2, false, LEDC_FREQUENCY, 0},
    {PIN_ESQUERDA_TRAS_IN3,   CHANNEL_ESQUERDA_TRAS_IN3,   false, LEDC_FREQUENCY, 0},
    {PIN_ESQUERDA_TRAS_IN4,   CHANNEL_ESQUERDA_TRAS_IN4,   false, LEDC_FREQUENCY, 0},
    {PIN_DIREITA_FRENTE_IN1,  CHANNEL_DIREITA_FRENTE_IN1,  false, LEDC_FREQUENCY, 0},
    {PIN_DIREITA_FRENTE_IN2,  CHANNEL_DIREITA_FRENTE_IN2,  false, LEDC_FREQUENCY, 0},
    {PIN_DIREITA_TRAS_IN3,    CHANNEL_DIREITA_TRAS_IN3,    false, LEDC_FREQUENCY, 0},
    {PIN_DIREITA_TRAS_IN4,    CHANNEL_DIREITA_TRAS_IN4,    false, LEDC_FREQUENCY, 0}
};

// Protótipos das funções
void inicializar_pinos(void);
void definir_ciclo_trabalho(uint8_t canal, uint8_t ciclo_porcentagem);
void configurar_frequencia(uint16_t frequencia);
void parar(void);
void mover_frente(void);
void mover_tras(void);
void virar_direita(void);
void virar_esquerda(void);
void ajustar_parametros(uint8_t vel_frente, uint8_t vel_tras, uint8_t vel_externa, uint8_t vel_interna, uint16_t tempo_acel);
void exibir_status(void);

// Função para inicializar os pinos PWM na ESP32
void inicializar_pinos(void) {
    printf("Inicializando pinos PWM para ESP32...\n");
    
    // Configuração do timer LEDC
    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_DUTY_RES,
        .freq_hz = LEDC_FREQUENCY,
        .speed_mode = LEDC_MODE,
        .timer_num = LEDC_TIMER
    };
    ledc_timer_config(&ledc_timer);
    
    // Configuração dos canais LEDC
    for (int i = 0; i < 8; i++) {
        ledc_channel_config_t ledc_channel = {
            .channel    = pinos_pwm[i].canal,
            .duty       = 0,
            .gpio_num   = pinos_pwm[i].pin,
            .speed_mode = LEDC_MODE,
            .timer_sel  = LEDC_TIMER
        };
        ledc_channel_config(&ledc_channel);
        
        printf("Configurando pino %d como saída PWM no canal %d\n", 
               pinos_pwm[i].pin, pinos_pwm[i].canal);
    }
    
    printf("Inicialização concluída.\n");
}

// Função para definir o ciclo de trabalho (duty cycle) de um pino específico
void definir_ciclo_trabalho(uint8_t canal, uint8_t ciclo_porcentagem) {
    // Converter o valor percentual (0-100%) para a escala de duty (0-255)
    uint32_t duty = (ciclo_porcentagem * LEDC_DUTY_MAX) / 100;
    
    // Aplica o ciclo de trabalho ao canal
    ledc_set_duty(LEDC_MODE, canal, duty);
    ledc_update_duty(LEDC_MODE, canal);
    
    // Atualiza o valor na estrutura de configuração
    for (int i = 0; i < 8; i++) {
        if (pinos_pwm[i].canal == canal) {
            pinos_pwm[i].ciclo_trabalho = ciclo_porcentagem;
            break;
        }
    }
}

// Função para configurar a frequência do PWM para todos os canais
void configurar_frequencia(uint16_t frequencia) {
    ledc_set_freq(LEDC_MODE, LEDC_TIMER, frequencia);
    
    // Atualiza o valor na estrutura de configuração
    for (int i = 0; i < 8; i++) {
        pinos_pwm[i].frequencia = frequencia;
    }
    
    printf("Frequência configurada para %d Hz\n", frequencia);
}

// Função para parar o carrinho
void parar(void) {
    printf("Comando: PARAR\n");
    
    // Desativa todos os sinais PWM
    for (int i = 0; i < 8; i++) {
        definir_ciclo_trabalho(pinos_pwm[i].canal, 0);
    }
    
    estado_atual = PARADO;
    printf("Carrinho parado.\n");
}

// Função para mover o carrinho para frente
void mover_frente(void) {
    printf("Comando: FRENTE\n");
    
    // Primeiro para todos os motores para evitar conflitos
    parar();
    
    // Configura os pinos para direção "para frente"
    // Motor esquerdo frente
    definir_ciclo_trabalho(CHANNEL_ESQUERDA_FRENTE_IN1, velocidade_aceleracao);
    definir_ciclo_trabalho(CHANNEL_ESQUERDA_FRENTE_IN2, 0);
    // Motor esquerdo traseiro
    definir_ciclo_trabalho(CHANNEL_ESQUERDA_TRAS_IN3, velocidade_aceleracao);
    definir_ciclo_trabalho(CHANNEL_ESQUERDA_TRAS_IN4, 0);
    // Motor direito frente
    definir_ciclo_trabalho(CHANNEL_DIREITA_FRENTE_IN1, velocidade_aceleracao);
    definir_ciclo_trabalho(CHANNEL_DIREITA_FRENTE_IN2, 0);
    // Motor direito traseiro
    definir_ciclo_trabalho(CHANNEL_DIREITA_TRAS_IN3, velocidade_aceleracao);
    definir_ciclo_trabalho(CHANNEL_DIREITA_TRAS_IN4, 0);
    
    printf("Acelerando a 100%% por %d ms...\n", tempo_aceleracao_ms);
    vTaskDelay(tempo_aceleracao_ms / portTICK_PERIOD_MS);
    
    // Reduz para a velocidade padrão após a aceleração inicial
    definir_ciclo_trabalho(CHANNEL_ESQUERDA_FRENTE_IN1, velocidade_padrao_frente);
    definir_ciclo_trabalho(CHANNEL_ESQUERDA_TRAS_IN3, velocidade_padrao_frente);
    definir_ciclo_trabalho(CHANNEL_DIREITA_FRENTE_IN1, velocidade_padrao_frente);
    definir_ciclo_trabalho(CHANNEL_DIREITA_TRAS_IN3, velocidade_padrao_frente);
    
    printf("Velocidade reduzida para %d%%\n", velocidade_padrao_frente);
    estado_atual = FRENTE;
}

// Função para mover o carrinho para trás
void mover_tras(void) {
    printf("Comando: TRAS\n");
    
    // Primeiro para todos os motores para evitar conflitos
    parar();
    
    // Configura os pinos para direção "para trás"
    // Motor esquerdo frente
    definir_ciclo_trabalho(CHANNEL_ESQUERDA_FRENTE_IN1, 0);
    definir_ciclo_trabalho(CHANNEL_ESQUERDA_FRENTE_IN2, velocidade_aceleracao);
    // Motor esquerdo traseiro
    definir_ciclo_trabalho(CHANNEL_ESQUERDA_TRAS_IN3, 0);
    definir_ciclo_trabalho(CHANNEL_ESQUERDA_TRAS_IN4, velocidade_aceleracao);
    // Motor direito frente
    definir_ciclo_trabalho(CHANNEL_DIREITA_FRENTE_IN1, 0);
    definir_ciclo_trabalho(CHANNEL_DIREITA_FRENTE_IN2, velocidade_aceleracao);
    // Motor direito traseiro
    definir_ciclo_trabalho(CHANNEL_DIREITA_TRAS_IN3, 0);
    definir_ciclo_trabalho(CHANNEL_DIREITA_TRAS_IN4, velocidade_aceleracao);
    
    printf("Acelerando a 100%% por %d ms...\n", tempo_aceleracao_ms);
    vTaskDelay(tempo_aceleracao_ms / portTICK_PERIOD_MS);
    
    // Reduz para a velocidade padrão após a aceleração inicial
    definir_ciclo_trabalho(CHANNEL_ESQUERDA_FRENTE_IN2, velocidade_padrao_tras);
    definir_ciclo_trabalho(CHANNEL_ESQUERDA_TRAS_IN4, velocidade_padrao_tras);
    definir_ciclo_trabalho(CHANNEL_DIREITA_FRENTE_IN2, velocidade_padrao_tras);
    definir_ciclo_trabalho(CHANNEL_DIREITA_TRAS_IN4, velocidade_padrao_tras);
    
    printf("Velocidade reduzida para %d%%\n", velocidade_padrao_tras);
    estado_atual = TRAS;
}

// Função para virar o carrinho para a direita
void virar_direita(void) {
    printf("Comando: DIREITA\n");
    
    // Primeiro para todos os motores para evitar conflitos
    parar();
    
    // Lado esquerdo (externo da curva) - velocidade maior
    // Motor esquerdo frente
    definir_ciclo_trabalho(CHANNEL_ESQUERDA_FRENTE_IN1, velocidade_curva_lado_externo);
    definir_ciclo_trabalho(CHANNEL_ESQUERDA_FRENTE_IN2, 0);
    // Motor esquerdo traseiro
    definir_ciclo_trabalho(CHANNEL_ESQUERDA_TRAS_IN3, velocidade_curva_lado_externo);
    definir_ciclo_trabalho(CHANNEL_ESQUERDA_TRAS_IN4, 0);
    
    // Lado direito (interno da curva) - velocidade menor ou inversa para curva mais acentuada
    // Motor direito frente - potência menor
    definir_ciclo_trabalho(CHANNEL_DIREITA_FRENTE_IN1, velocidade_curva_lado_interno);
    definir_ciclo_trabalho(CHANNEL_DIREITA_FRENTE_IN2, 0);
    // Motor direito traseiro - potência menor
    definir_ciclo_trabalho(CHANNEL_DIREITA_TRAS_IN3, velocidade_curva_lado_interno);
    definir_ciclo_trabalho(CHANNEL_DIREITA_TRAS_IN4, 0);
    
    printf("Virando à direita - Motor esquerdo: %d%%, Motor direito: %d%%\n", 
           velocidade_curva_lado_externo, velocidade_curva_lado_interno);
    
    estado_atual = DIREITA;
}

// Função para virar o carrinho para a esquerda
void virar_esquerda(void) {
    printf("Comando: ESQUERDA\n");
    
    // Primeiro para todos os motores para evitar conflitos
    parar();
    
    // Lado direito (externo da curva) - velocidade maior
    // Motor direito frente
    definir_ciclo_trabalho(CHANNEL_DIREITA_FRENTE_IN1, velocidade_curva_lado_externo);
    definir_ciclo_trabalho(CHANNEL_DIREITA_FRENTE_IN2, 0);
    // Motor direito traseiro
    definir_ciclo_trabalho(CHANNEL_DIREITA_TRAS_IN3, velocidade_curva_lado_externo);
    definir_ciclo_trabalho(CHANNEL_DIREITA_TRAS_IN4, 0);
    
    // Lado esquerdo (interno da curva) - velocidade menor ou inversa para curva mais acentuada
    // Motor esquerdo frente - potência menor
    definir_ciclo_trabalho(CHANNEL_ESQUERDA_FRENTE_IN1, velocidade_curva_lado_interno);
    definir_ciclo_trabalho(CHANNEL_ESQUERDA_FRENTE_IN2, 0);
    // Motor esquerdo traseiro - potência menor
    definir_ciclo_trabalho(CHANNEL_ESQUERDA_TRAS_IN3, velocidade_curva_lado_interno);
    definir_ciclo_trabalho(CHANNEL_ESQUERDA_TRAS_IN4, 0);
    
    printf("Virando à esquerda - Motor direito: %d%%, Motor esquerdo: %d%%\n", 
           velocidade_curva_lado_externo, velocidade_curva_lado_interno);
    
    estado_atual = ESQUERDA;
}

// Função para ajustar parâmetros de movimento
void ajustar_parametros(uint8_t vel_frente, uint8_t vel_tras, uint8_t vel_externa, uint8_t vel_interna, uint16_t tempo_acel) {
    velocidade_padrao_frente = vel_frente;
    velocidade_padrao_tras = vel_tras;
    velocidade_curva_lado_externo = vel_externa;
    velocidade_curva_lado_interno = vel_interna;
    tempo_aceleracao_ms = tempo_acel;
    
    printf("Parâmetros ajustados:\n");
    printf("- Velocidade para frente: %d%%\n", velocidade_padrao_frente);
    printf("- Velocidade para trás: %d%%\n", velocidade_padrao_tras);
    printf("- Velocidade lado externo da curva: %d%%\n", velocidade_curva_lado_externo);
    printf("- Velocidade lado interno da curva: %d%%\n", velocidade_curva_lado_interno);
    printf("- Tempo de aceleração: %d ms\n", tempo_aceleracao_ms);
    
    // Se o carrinho estiver em movimento, aplica as novas configurações
    if (estado_atual != PARADO) {
        switch (estado_atual) {
            case FRENTE: mover_frente(); break;
            case TRAS: mover_tras(); break;
            case DIREITA: virar_direita(); break;
            case ESQUERDA: virar_esquerda(); break;
            default: break;
        }
    }
}

// Função para exibir o status atual dos motores
void exibir_status(void) {
    const char* estados[] = {"PARADO", "FRENTE", "TRAS", "ESQUERDA", "DIREITA"};
    
    printf("\n--- Status do Carrinho ---\n");
    printf("Estado atual: %s\n", estados[estado_atual]);
    printf("Frequência PWM: %d Hz\n", pinos_pwm[0].frequencia);
    printf("\n");
    
    printf("Canal | Pino GPIO | Ciclo (%%) \n");
    printf("---------------------------\n");
    
    for (int i = 0; i < 8; i++) {
        printf("  %d   |    %2d     |   %3d%%  \n", 
               pinos_pwm[i].canal,
               pinos_pwm[i].pin,
               pinos_pwm[i].ciclo_trabalho);
    }
    printf("---------------------------\n");
    
    printf("Parâmetros configurados:\n");
    printf("- Velocidade para frente: %d%%\n", velocidade_padrao_frente);
    printf("- Velocidade para trás: %d%%\n", velocidade_padrao_tras);
    printf("- Velocidade lado externo da curva: %d%%\n", velocidade_curva_lado_externo);
    printf("- Velocidade lado interno da curva: %d%%\n", velocidade_curva_lado_interno);
    printf("- Tempo de aceleração: %d ms\n", tempo_aceleracao_ms);
}

int app_main(void) {
    printf("\n==============================================\n");
    printf("Controlador de Carrinho com ESP32 - PWM 300Hz\n");
    printf("==============================================\n");
    
    // Inicializa os pinos e configura o PWM
    inicializar_pinos();
    configurar_frequencia(LEDC_FREQUENCY);
    
    // Define os parâmetros iniciais
    ajustar_parametros(
        80,   // velocidade_padrao_frente (80%)
        80,   // velocidade_padrao_tras (80%)
        80,   // velocidade_curva_lado_externo (80%)
        40,   // velocidade_curva_lado_interno (40%)
        500   // tempo_aceleracao_ms (500ms)
    );
    
    // Exibe o status inicial
    exibir_status();
    
    // Exemplo de sequência de comandos:
    printf("\nDemonstrando sequência de comandos:\n");
    
    // Teste 1: Mover para frente
    mover_frente();
    vTaskDelay(2000 / portTICK_PERIOD_MS);  // Aguarda 2 segundos
    
    // Teste 2: Virar à direita
    virar_direita();
    vTaskDelay(1500 / portTICK_PERIOD_MS);  // Aguarda 1.5 segundos
    
    // Teste 3: Mover para frente novamente
    mover_frente();
    vTaskDelay(2000 / portTICK_PERIOD_MS);  // Aguarda 2 segundos
    
    // Teste 4: Virar à esquerda
    virar_esquerda();
    vTaskDelay(1500 / portTICK_PERIOD_MS);  // Aguarda 1.5 segundos
    
    // Teste 5: Mover para trás
    mover_tras();
    vTaskDelay(2000 / portTICK_PERIOD_MS);  // Aguarda 2 segundos
    
    // Teste 6: Parar
    parar();
    
    // Exibe o status final
    exibir_status();
    
    printf("\nTestes concluídos. Sistema pronto para uso.\n");
    printf("Use as funções mover_frente(), mover_tras(), virar_direita(), virar_esquerda() e parar()\n");
    printf("Para ajustar os parâmetros, use ajustar_parametros(vel_frente, vel_tras, vel_externa, vel_interna, tempo_acel)\n");
    
    // Em um sistema real, você poderia implementar um loop infinito para receber comandos
    // Exemplo básico:
    
   
    
    return 0;
});
    
    // Altera o ciclo de trabalho do pino 5 para 75%
    configurar_ciclo_trabalho(PIN_5, 75);
    
    // Liga os pinos 0, 3 e 6
    controlar_pino(PIN_0, true);
    controlar_pino(PIN_3, true);
    controlar_pino(PIN_6, true);
    
    // Exibe o status atualizado
    exibir_status();
    
    // Configuração de todos os pinos em 400Hz e 60% de ciclo de trabalho
    printf("\nConfigurando todos os pinos para 400Hz com ciclo de trabalho de 60%%\n");
    configurar_todos_pinos(400, 60);
    
    // Exibe o status final
    exibir_status();
    
    // Em um sistema real, você teria um loop principal aqui
    printf("\nIniciando geração de PWM...\n");
    atualizar_pwm();
    
    return 0;
}