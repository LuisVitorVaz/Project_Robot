#include <math.h>
#include <WiFi.h>
#include "driver/i2s.h"
#include "driver/adc.h"

// ==========================
// WIFI
// ==========================
const char* WIFI_SSID      = "Carrinho dos Guri";
const char* WIFI_PASSWORD  = "a1b23e75z123";
const char* SERVER_IP      = "10.241.8.71";
const uint16_t SERVER_PORT = 5005;

WiFiClient client;

// ==========================
// AMOSTRAGEM
// ==========================
#define SAMPLE_RATE 80000
#define N_MIC1 500
#define N_MIC2 600

// ==========================
// ADC
// ==========================
#define ADC_CH_MIC1 ADC1_CHANNEL_6 // GPIO34
#define ADC_CH_MIC2 ADC1_CHANNEL_7 // GPIO35

#define I2S_PORT I2S_NUM_0

// ==========================
// BUFFERS
// ==========================
uint16_t raw1[N_MIC1];
uint16_t raw2[N_MIC2];

int16_t mic1[N_MIC1];
int16_t mic2[N_MIC2];

uint8_t vetor1_norm[N_MIC1];
uint8_t vetor2_norm[N_MIC2];

uint16_t correlacao_resultado[201];

int16_t maior_mic1 = 0;
int16_t maior_mic2 = 0;

uint16_t max_val = 0;
int max_index = 0;
int angulo_theta = 0;

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
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 256,
    .use_apll = false // ✔ estabilidade
  };

  i2s_driver_install(I2S_PORT, &config, 0, NULL);

  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(ADC_CH_MIC1, ADC_ATTEN_DB_11);
  adc1_config_channel_atten(ADC_CH_MIC2, ADC_ATTEN_DB_11);
}

// ==========================
// NORMALIZAÇÃO
// ==========================
void encontrar_maior(const int16_t *v, int n, int16_t *dest) {
  *dest = abs(v[0]);
  for (int i = 1; i < n; i++) {
    int16_t val = abs(v[i]);
    if (val > *dest) *dest = val;
  }
}

void normalizar_vetor(const int16_t *v, uint8_t *out, int n, int16_t mv) {
  if (mv == 0) mv = 1; // ✔ proteção

  for (int i = 0; i < n; i++) {
    int16_t val = v[i];
    out[i] = (uint8_t)((val * 127) / mv + 128);
  }
}

// ==========================
// COLETA
// ==========================
void coleta_amostras() {
  size_t bytes_read;

  // MIC1
  i2s_set_adc_mode(ADC_UNIT_1, ADC_CH_MIC1);
  delay(2);

  for (int i = 0; i < N_MIC1; i++) {
    uint16_t sample;
    i2s_read(I2S_PORT, &sample, sizeof(sample),
             &bytes_read, pdMS_TO_TICKS(10));

    raw1[i] = sample & 0x0FFF;

    if (i % 50 == 0) delay(0); // ✔ mantém WiFi vivo
  }

  // MIC2
  i2s_set_adc_mode(ADC_UNIT_1, ADC_CH_MIC2);
  delay(2);

  for (int i = 0; i < N_MIC2; i++) {
    uint16_t sample;
    i2s_read(I2S_PORT, &sample, sizeof(sample),
             &bytes_read, pdMS_TO_TICKS(10));

    raw2[i] = sample & 0x0FFF;

    if (i % 50 == 0) delay(0);
  }

  // DC removal
  uint32_t soma1 = 0, soma2 = 0;
  for (int i = 0; i < N_MIC1; i++) soma1 += raw1[i];
  for (int i = 0; i < N_MIC2; i++) soma2 += raw2[i];

  int16_t media1 = soma1 / N_MIC1;
  int16_t media2 = soma2 / N_MIC2;

  for (int i = 0; i < N_MIC1; i++) mic1[i] = raw1[i] - media1;
  for (int i = 0; i < N_MIC2; i++) mic2[i] = raw2[i] - media2;
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
  if (!client.connect(SERVER_IP, SERVER_PORT)) {
    Serial.println("Falha socket");
    return;
  }

  client.println("START");

  client.println("MIC1:");
  for (int i = 0; i < N_MIC1; i++) client.println(mic1[i]);

  client.println("MIC2:");
  for (int i = 0; i < N_MIC2; i++) client.println(mic2[i]);

  client.println("CORR:");
  for (int i = 0; i < 201; i++) client.println(correlacao_resultado[i]);

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

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi conectado");
}

// ==========================
// LOOP
// ==========================
void loop() {
  for (int i = 0; i < 201; i++) correlacao_resultado[i] = 0;

  coleta_amostras();

  encontrar_maior(mic1, N_MIC1, &maior_mic1);
  encontrar_maior(mic2, N_MIC2, &maior_mic2);

  // ✔ proteção extra
  if (maior_mic1 == 0) maior_mic1 = 1;
  if (maior_mic2 == 0) maior_mic2 = 1;

  normalizar_vetor(mic1, vetor1_norm, N_MIC1, maior_mic1);
  normalizar_vetor(mic2, vetor2_norm, N_MIC2, maior_mic2);

  correlacao_cruzada(vetor1_norm, vetor2_norm, correlacao_resultado);
  detectar_angulo(correlacao_resultado);
  equacao_final();

  Serial.printf("angulo=%d  max_index=%d\n", angulo_theta, max_index);

  enviar_dados_socket();
}