#include <Arduino.h>
#include <WiFi.h>
#include <math.h>
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
#define N_MIC1 2500
#define N_MIC2 2600

#define ADC_CH_MIC1 ADC1_CHANNEL_6 // GPIO34
#define ADC_CH_MIC2 ADC1_CHANNEL_7 // GPIO35

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
// ADC
// ==========================
void adc_init() {
  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(ADC_CH_MIC1, ADC_ATTEN_DB_11);
  adc1_config_channel_atten(ADC_CH_MIC2, ADC_ATTEN_DB_11);
}

// ==========================
// COLETA ESTÁVEL (SEM I2S)
// ==========================
void coleta_amostras() {

  // MIC1
  for (int i = 0; i < N_MIC1; i++) {
    raw1[i] = adc1_get_raw(ADC_CH_MIC1);
  }

  // MIC2
  for (int i = 0; i < N_MIC2; i++) {
    raw2[i] = adc1_get_raw(ADC_CH_MIC2);
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
void encontrar_maior(const int16_t *v, int n, int16_t *dest) {
  *dest = abs(v[0]);
  for (int i = 1; i < n; i++) {
    int16_t val = abs(v[i]);
    if (val > *dest) *dest = val;
  }
}

void normalizar_vetor(const int16_t *v, uint8_t *out, int n, int16_t mv) {
  if (mv == 0) mv = 1;
  for (int i = 0; i < n; i++) {
    out[i] = (uint8_t)((v[i] * 127) / mv + 128);
  }
}

// ==========================
void correlacao_cruzada(const uint8_t *v1, const uint8_t *v2, uint16_t *res) {
  for (int i = 0; i < 201; i++) {
    int soma = 0, k = 99, j = i;

    if (j > 99) {
      k -= (j - 99);
      j = 99;
    }

    while (j >= 0 && k >= 0) {
      soma += v2[k] * v1[j];
      j--;
      k--;
    }

    res[i] = soma / 100;
  }
}

void detectar_angulo(uint16_t *corr) {
  max_val = corr[0];
  max_index = 0;

  for (int i = 1; i < 201; i++) {
    if (corr[i] > max_val) {
      max_val = corr[i];
      max_index = i;
    }
  }
}

void equacao_final() {
  angulo_theta = (int)((-0.174 * pow(max_index, 2.0)) +
                       (10.6 * max_index) + 0.122);
}

// ==========================
// ENVIO TCP
// ==========================
void enviar_dados_socket() {

  if (!client.connect(SERVER_IP, SERVER_PORT)) {
    Serial.println("Falha socket");
    return;
  }

  // ===== MIC1 =====
  client.println("MIC1:");
  for (int i = 0; i < N_MIC1; i++) {
    client.println(raw1[i]);  // 🔥 RAW 0–4095
  }

  // ===== MIC2 =====
  client.println("MIC2:");
  for (int i = 0; i < N_MIC2; i++) {
    client.println(raw2[i]);  // 🔥 RAW 0–4095
  }

  client.println("END");

  client.stop();
}

// ==========================
void setup() {
  Serial.begin(115200);
  adc_init();

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.print("Conectando WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi conectado");
}

// ==========================
void loop() {

  for (int i = 0; i < 201; i++)
    correlacao_resultado[i] = 0;

  coleta_amostras();

  encontrar_maior(mic1, N_MIC1, &maior_mic1);
  encontrar_maior(mic2, N_MIC2, &maior_mic2);

  if (maior_mic1 == 0) maior_mic1 = 1;
  if (maior_mic2 == 0) maior_mic2 = 1;

  normalizar_vetor(mic1, vetor1_norm, N_MIC1, maior_mic1);
  normalizar_vetor(mic2, vetor2_norm, N_MIC2, maior_mic2);

  correlacao_cruzada(vetor1_norm, vetor2_norm, correlacao_resultado);
  detectar_angulo(correlacao_resultado);
  equacao_final();

  Serial.printf("angulo=%d\n", angulo_theta);

  enviar_dados_socket();

  delay(200);
}