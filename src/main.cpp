#include <math.h>
#include <WiFi.h>
#include "driver/i2s.h"
#include "driver/adc.h"

// ==========================
// CONFIGURAÇÕES WIFI
// ==========================
const char* WIFI_SSID      = "Carrinho dos Guri";
const char* WIFI_PASSWORD  = "a1b23e75z123";
const char* SERVER_IP      = "10.69.69.71";
const uint16_t SERVER_PORT = 5005;

WiFiClient client;

// ==========================
// AMOSTRAGEM
// ==========================
#define SAMPLE_RATE 80000
#define N_MIC1      100
#define N_MIC2      300

// ==========================
// ADC (GPIO34 e 35)
// ==========================
#define ADC_CH_MIC1 ADC1_CHANNEL_6
#define ADC_CH_MIC2 ADC1_CHANNEL_7

// ==========================
// I2S
// ==========================
#define I2S_PORT I2S_NUM_0
#define I2S_BUF_LEN 512

// ==========================
// BUFFERS
// ==========================
uint16_t raw1[N_MIC1];
uint16_t raw2[N_MIC2];

uint8_t mic1[N_MIC1];
uint8_t mic2[N_MIC2];

uint8_t vetor1_norm[N_MIC1];
uint8_t vetor2_norm[N_MIC2];

uint16_t correlacao_resultado[201];

uint8_t  maior_mic1   = 0;
uint8_t  maior_mic2   = 0;
uint16_t max_val      = 0;
int      max_index    = 0;
int      angulo_theta = 0;

// atraso estimado (1 troca de canal)
float atraso_us = 0.0;

// ==========================
// I2S INIT
// ==========================
void i2s_init() {

  i2s_config_t config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_RIGHT,
    .communication_format = I2S_COMM_FORMAT_STAND_MSB,
    .intr_alloc_flags = 0,
    .dma_buf_count = 8,
    .dma_buf_len = I2S_BUF_LEN,
    .use_apll = true
  };

  i2s_driver_install(I2S_PORT, &config, 0, NULL);

  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(ADC_CH_MIC1, ADC_ATTEN_DB_11);
  adc1_config_channel_atten(ADC_CH_MIC2, ADC_ATTEN_DB_11);
}

// ==========================
// LEITURA I2S + DMA (DUAL)
// ==========================
void i2s_read_dual(uint16_t *out1, uint16_t *out2, int n1, int n2) {
  
  // out1[] → MIC1
  // out2[] → MIC2

  // Esse buffer recebe dados direto do I2S (DMA)

   // cada amostra = 16 bits
  // ADC real = 12 bits (depois você mascara)

  uint16_t buffer[I2S_BUF_LEN]; 
  size_t bytes_read;

  // i1 → posição atual em out1
  // i2 → posição atual em out2
  int i1 = 0, i2 = 0;
  bool toggle = false;   // controla qual canal será lido

  // você manda o hardware mudar o canal do ADC interno

  while (i1 < n1 || i2 < n2) {

    if (toggle) {
      i2s_set_adc_mode(ADC_UNIT_1, ADC_CH_MIC1);
    } else {
      i2s_set_adc_mode(ADC_UNIT_1, ADC_CH_MIC2);
    }

    toggle = !toggle;

    //  delay para garantir a estabilidade
    ets_delay_us(2);

    // o I2S já está rodando continuamente DMA enche o buffer automaticamente CPU só copia

    // Você recebe um bloco de amostras já prontas

    i2s_read(I2S_PORT, buffer, sizeof(buffer), &bytes_read, portMAX_DELAY);

    // quantidade de amostras
    int samples = bytes_read / 2;

    for (int i = 0; i < samples; i++) {

      // remove lixo dos 4 bits superiores
      // ADC = 12 bits
      // I2S entrega 16 bits
      uint16_t val = buffer[i] & 0x0FFF;

      if (toggle && i1 < n1) {
        out1[i1++] = val;
      } else if (!toggle && i2 < n2) {
        out2[i2++] = val;
      }

      if (i1 >= n1 && i2 >= n2) break;
    }
  }

  // atraso fixo = 1 período de amostragem
  atraso_us = 1000000.0 / SAMPLE_RATE;

  // Isso assume: cada troca de canal = 1 período de amostragem

  // Exemplo: 80 kHz → 12.5 µs Esse é o Δt entre os sinais
}

// ==========================
// CORREÇÃO DE ATRASO
// ==========================
void corrigir_atraso(uint8_t *sig, int n, float atraso_us) {



  float Ts = 1000000.0 / SAMPLE_RATE;
  float shift = atraso_us / Ts;

  for (int i = 0; i < n - 1; i++) {

    float idx = i + shift;
    int i0 = (int)idx;
    float frac = idx - i0;

    if (i0 >= 0 && i0 < n - 1) {
      sig[i] = (uint8_t)((1.0 - frac) * sig[i0] + frac * sig[i0 + 1]);
    }
  }
}

// ==========================
// COLETA
// ==========================
void coleta_amostras() {

  i2s_read_dual(raw1, raw2, N_MIC1, N_MIC2);

  uint32_t soma1 = 0, soma2 = 0;
  for (int i = 0; i < N_MIC1; i++) soma1 += raw1[i];
  for (int i = 0; i < N_MIC2; i++) soma2 += raw2[i];

  int16_t media1 = soma1 / N_MIC1;
  int16_t media2 = soma2 / N_MIC2;

  int16_t sinal1[N_MIC1];
  int16_t sinal2[N_MIC2];

  for (int i = 0; i < N_MIC1; i++) sinal1[i] = raw1[i] - media1;
  for (int i = 0; i < N_MIC2; i++) sinal2[i] = raw2[i] - media2;

  uint16_t max1 = 0, max2 = 0;

  for (int i = 0; i < N_MIC1; i++) {
    uint16_t v = abs(sinal1[i]);
    if (v > max1) max1 = v;
  }

  for (int i = 0; i < N_MIC2; i++) {
    uint16_t v = abs(sinal2[i]);
    if (v > max2) max2 = v;
  }

  if (max1 < 20) max1 = 20;
  if (max2 < 20) max2 = 20;

  for (int i = 0; i < N_MIC1; i++)
    mic1[i] = (abs(sinal1[i]) * 255UL) / max1;

  for (int i = 0; i < N_MIC2; i++)
    mic2[i] = (abs(sinal2[i]) * 255UL) / max2;

  // CORREÇÃO DE ATRASO AQUI
  corrigir_atraso(mic2, N_MIC2, atraso_us);
}

// ==========================
// NORMALIZAÇÃO
// ==========================
void encontrar_maior(const uint8_t *v, int n, uint8_t *dest) {
  *dest = v[0];
  for (int i = 1; i < n; i++) if (v[i] > *dest) *dest = v[i];
}

void normalizar_vetor(const uint8_t *v, uint8_t *out, int n, uint8_t mv) {
  for (int i = 0; i < n; i++) out[i] = mv != 0 ? ((uint16_t)v[i]) / mv : 0;
}

// ==========================
// CORRELAÇÃO
// ==========================
void correlacao_cruzada(const uint8_t *v1, const uint8_t *v2, uint16_t *res) {
  for (int i = 0; i < 201; i++) {
    int soma = 0, k = 99, j = i;
    if (j > 99) { k -= (j - 99); j = 99; }
    while (j >= 0 && k >= 0) { soma += v2[k] * v1[j]; j--; k--; }
    res[i] = soma / 100;
  }
}

// ==========================
// ÂNGULO
// ==========================
void detectar_angulo(uint16_t *corr) {
  max_val = corr[0]; max_index = 0;
  for (int i = 1; i < 201; i++) {
    if (corr[i] > max_val) { max_val = corr[i]; max_index = i; }
  }
}

void equacao_final() {
  angulo_theta = (int)((-0.174 * pow(max_index, 2.0)) + (10.6 * max_index) + 0.122);
}

// ==========================
// SOCKET
// ==========================
void enviar_dados_socket() {

  while (!client.connect(SERVER_IP, SERVER_PORT)) {
    delay(1000);
  }

  client.println("START");

  for (int i = 0; i < N_MIC1; i++) client.println(mic1[i]);
  for (int i = 0; i < N_MIC2; i++) client.println(mic2[i]);

  client.print("angulo=");
  client.println(angulo_theta);

  client.println("END");
  client.stop();
}

// ==========================
// SETUP
// ==========================
void setup() {

  Serial.begin(115200);

  i2s_init();

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) delay(500);

  Serial.println("Sistema pronto (I2S + DMA)");
}

// ==========================
// LOOP
// ==========================
void loop() {

  for (int i = 0; i < 201; i++) correlacao_resultado[i] = 0;

  coleta_amostras();

  encontrar_maior(mic1, N_MIC1, &maior_mic1);
  encontrar_maior(mic2, N_MIC2, &maior_mic2);

  normalizar_vetor(mic1, vetor1_norm, N_MIC1, maior_mic1);
  normalizar_vetor(mic2, vetor2_norm, N_MIC2, maior_mic2);

  correlacao_cruzada(vetor1_norm, vetor2_norm, correlacao_resultado);
  detectar_angulo(correlacao_resultado);
  equacao_final();

  Serial.printf("angulo=%d  max_index=%d\n", angulo_theta, max_index);

  enviar_dados_socket();
}