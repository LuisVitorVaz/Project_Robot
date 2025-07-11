#include <arduinoFFT.h>
#include <math.h>

// LEDs
#define LED_FRENTE 10
#define LED_45_DIR_FRENTE 2
#define LED_DIREITA 6         // Corrigido: pino duplicado antes
#define LED_45_DIR_TRAS 3
#define LED_ATRAS 5
#define LED_45_ESQ_TRAS 4
#define LED_ESQUERDA 9        // Corrigido: agora diferente de LED_DIREITA
#define LED_45_ESQ_FRENTE 8

#define LED_PAUSA 7
#define LED_COLETA 13

const int MIC1_PIN = A0;
const int MIC2_PIN = A1;

float mic1[20];
float mic2[100];

float vetor1_norm[20];
float vetor2_norm[100];
float correlacao_resultado[81];
float vetor_final[1000];
float vet_valores_finais[10];
float vet_valores_finais2[10];

float maior_mic1 = 0.0;
float maior_mic2 = 0.0;
float max_val;
int cont = 0;
int angulo_theta = 0;
int indice_mais_proximo = 0;
int  max_index = 0;
int maior_indice_do_vetor;


void apaga_leds() {
  digitalWrite(LED_ESQUERDA, LOW);
  digitalWrite(LED_DIREITA, LOW);
  digitalWrite(LED_FRENTE, LOW);
  digitalWrite(LED_ATRAS, LOW);
  digitalWrite(LED_45_ESQ_FRENTE, LOW);
  digitalWrite(LED_45_DIR_FRENTE, LOW);
  digitalWrite(LED_45_ESQ_TRAS, LOW);
  digitalWrite(LED_45_DIR_TRAS, LOW);
  digitalWrite(LED_PAUSA, LOW);
}

void imprimir_vetor() {
  for (int i = 0; i < 1000; i++) {
    Serial.print("valores vetor final[");
    Serial.print(i);
    Serial.print("] = ");
    Serial.println(vetor_final[i], 3);
  }
}

void coleta_amostras_grande(int bloco) {
  for (int i = 0; i < 100; i++) {
    vetor_final[bloco * 100 + i] = mic2[i];
  }

  digitalWrite(LED_PAUSA, HIGH);
  Serial.println("Aguardando 5 segundos...");
  delay(5000);
  digitalWrite(LED_PAUSA, LOW);
  delay(100);
}

void coleta_amostras() {
  digitalWrite(LED_COLETA, HIGH);
  for (int i = 0; i < 100; i++) {
    if (i < 20) mic1[i] = (float)analogRead(MIC1_PIN);
    mic2[i] = (float)analogRead(MIC2_PIN);
  }
  Serial.println("Fim da coleta de amostras.");
  digitalWrite(LED_COLETA, LOW);
}

void imprimir_amostras() {
  Serial.println("Amostras coletadas:");
  for (int i = 0; i < 100; i++) {
    if (i < 20) {
      Serial.print("mic1["); Serial.print(i); Serial.print("] = ");
      Serial.print(mic1[i]); Serial.print(" | ");
    }
    Serial.print("mic2["); Serial.print(i); Serial.print("] = ");
    Serial.println(mic2[i]);
  }
}

void encontrar_maior(const float *vetor, int tamanho, float *maior_destino) {
  *maior_destino = vetor[0];
  maior_indice_do_vetor = 0;
  for (int i = 1; i < tamanho; i++) {
    if (vetor[i] > *maior_destino) {
      *maior_destino = vetor[i];
      maior_indice_do_vetor = i;
    }
  }
}

void normalizar_vetor(const float *vetor, float *vetor_norm, int tamanho, float normalizar) {
  for (int i = 0; i < tamanho; i++) {
    vetor_norm[i] = normalizar != 0 ? vetor[i] / normalizar : 0;
  }
}

void imprimir_vetor_normalizado(const float *vetor, int tamanho, const char* nome) {
  Serial.print("Vetor "); Serial.print(nome); Serial.println(" normalizado:");
  for (int i = 0; i < tamanho; i++) {
    Serial.print(vetor[i], 3);
    Serial.print(" ");
    if ((i + 1) % 10 == 0) Serial.println();
  }
  Serial.println();
}

void correlacao_cruzada(const float *mic1, const float *mic2, float *resultado) {
  for (int k = 0; k <= 80; k++) {
    float soma = 0.0;
    for (int n = 0; n < 20; n++) {
      soma += mic1[n] * mic2[n + k];
    }
    resultado[k] = soma/20;
  }
}

void equacao_final() {
  angulo_theta = (-0.174 * pow(max_index, 2.0)) + (10.6 * max_index) + 0.122;
  Serial.print("valor da funcao é: ");
  Serial.println(angulo_theta);
}
// TESTAR VALIDADE DESSA FUNCAO
void acender_led_mais_proximo() {
  float angulos[] = {0, 45, 90, 135, 180};  // Lista de ângulos de referência
  // int indice_mais_proximo = 0;

  // Verifica limites inferiores e superiores
  if (angulo_theta <= (angulos[0] + angulos[1]) / 2) {
    indice_mais_proximo = 0;
  } else if (angulo_theta >= (angulos[4] + angulos[3]) / 2) {
    indice_mais_proximo = 4;
  } else {
    // Verifica qual intervalo o angulo_theta está mais próximo
    for (int i = 0; i < 4; i++) {
      float limite_inferior = (angulos[i] + angulos[i + 1]) / 2;
      float limite_superior = (angulos[i + 1] + angulos[i + 2]) / 2;

      if (angulo_theta >= limite_inferior && angulo_theta < limite_superior) {
        indice_mais_proximo = i + 1;
        break;
      }
    }
  }

  // Aqui você já tem o índice do ângulo mais próximo:
  // Use `indice_mais_proximo` para acender o LED correspondente
}

// VERIFICAR ESTA FUNCAO TESTAR CODIGO
void acender_led() {
  apaga_leds();  // Garante que todos os LEDs sejam apagados primeiro

  // Acende o LED correspondente ao ângulo mais próximo
  Serial.print("valor mais proximos \n");
  Serial.print(indice_mais_proximo);
  
  switch (indice_mais_proximo) {
    case 0:
      digitalWrite(LED_FRENTE, HIGH);           // Ângulo 0°
      break;
    case 1:
      digitalWrite(LED_45_DIR_FRENTE, HIGH);    // Ângulo 45°
      break;
    case 2:
      digitalWrite(LED_DIREITA, HIGH);          // Ângulo 90°
      break;
    case 3:
      digitalWrite(LED_45_DIR_TRAS, HIGH);      // Ângulo 135°
      break;
    case 4:
      digitalWrite(LED_ATRAS, HIGH);            // Ângulo 180°
      break;
  }
}

void detectar_angulo(float correlacao[81]) {
  max_val = correlacao[0];
  max_index = 0;

  for (int i = 1; i < 81; i++) {
    if (correlacao[i] > max_val) {
      max_val = correlacao[i];
      max_index = i;
    }
  }

  Serial.println("Valor máximo da correlação:");
  Serial.print("Índice: "); Serial.print(max_index);
  Serial.print(" Valor: "); Serial.println(max_val);
}

void setup() {
  Serial.begin(9600);

  pinMode(LED_ESQUERDA, OUTPUT);
  pinMode(LED_DIREITA, OUTPUT);
  pinMode(LED_FRENTE, OUTPUT);
  pinMode(LED_ATRAS, OUTPUT);
  pinMode(LED_45_ESQ_FRENTE, OUTPUT);
  pinMode(LED_45_DIR_FRENTE, OUTPUT);
  pinMode(LED_45_ESQ_TRAS, OUTPUT);
  pinMode(LED_45_DIR_TRAS, OUTPUT);
  pinMode(LED_PAUSA, OUTPUT);
  pinMode(LED_COLETA, OUTPUT);
}

void loop() {
  if (cont < 10) {
    // Resetar vetores
    for (int i = 0; i < 81; i++) correlacao_resultado[i] = 0.0;

    coleta_amostras();
    imprimir_amostras();

    encontrar_maior(mic1, 20, &maior_mic1);
    encontrar_maior(mic2, 100, &maior_mic2);

    normalizar_vetor(mic1, vetor1_norm, 20, maior_mic1);
    normalizar_vetor(mic2, vetor2_norm, 100, maior_mic2);

    imprimir_vetor_normalizado(vetor1_norm, 20, "mic1");
    imprimir_vetor_normalizado(vetor2_norm, 100, "mic2");

    correlacao_cruzada(vetor1_norm, vetor2_norm, correlacao_resultado);

    detectar_angulo(correlacao_resultado);
    vet_valores_finais[cont] = max_index;
    vet_valores_finais2[cont] = max_val;


    Serial.println("Resultado da correlação cruzada:");
    for (int i = 0; i < 81; i++) {
      Serial.print("correlacao[");
      Serial.print(i);
      Serial.print("] = ");
      Serial.println(correlacao_resultado[i], 3);
    }

    coleta_amostras_grande(cont);
    cont++;

    equacao_final();
    acender_led_mais_proximo();
    acender_led();
  }
  else
  {
    for (int i = 0; i < 10; i++) {
      Serial.print("resultado da correlacao[");
      Serial.print(i);
      Serial.print("] = ");
      Serial.println(vet_valores_finais[i], 3); // tem o index do vetor
      Serial.println(vet_valores_finais2[i], 3);
    }
  }
}
