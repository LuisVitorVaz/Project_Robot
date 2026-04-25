#include <math.h>
#include <WiFi.h>

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
const int SAMPLE_RATE = 20000;
const int N_MIC1      = 100;
const int N_MIC2      = 300;

// ==========================
// PINOS — apenas ADC1 (WiFi usa ADC2)
// ==========================
const int MIC1_PIN = 34;
const int MIC2_PIN = 35;

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

// ==========================
// SEMÁFOROS DUAL-CORE
// ==========================
SemaphoreHandle_t sem_iniciar;
SemaphoreHandle_t sem_mic1_ok;
SemaphoreHandle_t sem_mic2_ok;

// ==========================
// TASK NÚCLEO 0 — MIC1
// ==========================
void task_mic1(void* param) {
  const unsigned long intervalo = 1000000UL / SAMPLE_RATE;
  while (true) {
    xSemaphoreTake(sem_iniciar, portMAX_DELAY);
    unsigned long t = micros();
    for (int i = 0; i < N_MIC1; i++) {
      while (micros() - t < intervalo);
      t += intervalo;
      raw1[i] = analogRead(MIC1_PIN);
    }
    xSemaphoreGive(sem_mic1_ok);
  }
}

// ==========================
// TASK NÚCLEO 1 — MIC2
// ==========================
void task_mic2(void* param) {
  const unsigned long intervalo = 1000000UL / SAMPLE_RATE;
  while (true) {
    xSemaphoreTake(sem_iniciar, portMAX_DELAY);
    unsigned long t = micros();
    for (int i = 0; i < N_MIC2; i++) {
      while (micros() - t < intervalo);
      t += intervalo;
      raw2[i] = analogRead(MIC2_PIN);
    }
    xSemaphoreGive(sem_mic2_ok);
  }
}

// ==========================
// COLETA — dispara os dois núcleos juntos
// ==========================
void coleta_amostras() {
  xSemaphoreGive(sem_iniciar);   // libera task_mic1
  xSemaphoreGive(sem_iniciar);   // libera task_mic2
  xSemaphoreTake(sem_mic1_ok, portMAX_DELAY);
  xSemaphoreTake(sem_mic2_ok, portMAX_DELAY);

  // remoção DC
  uint32_t soma1 = 0, soma2 = 0;
  for (int i = 0; i < N_MIC1; i++) soma1 += raw1[i];
  for (int i = 0; i < N_MIC2; i++) soma2 += raw2[i];
  int16_t media1 = soma1 / N_MIC1;
  int16_t media2 = soma2 / N_MIC2;

  int16_t sinal1[N_MIC1];
  int16_t sinal2[N_MIC2];
  for (int i = 0; i < N_MIC1; i++) sinal1[i] = (int16_t)raw1[i] - media1;
  for (int i = 0; i < N_MIC2; i++) sinal2[i] = (int16_t)raw2[i] - media2;

  // máximo absoluto + limiar
  uint16_t max1 = 0, max2 = 0;
  for (int i = 0; i < N_MIC1; i++) { uint16_t v = abs(sinal1[i]); if (v > max1) max1 = v; }
  for (int i = 0; i < N_MIC2; i++) { uint16_t v = abs(sinal2[i]); if (v > max2) max2 = v; }
  if (max1 < 20) max1 = 20;
  if (max2 < 20) max2 = 20;

  // auto scale 8 bits
  for (int i = 0; i < N_MIC1; i++) mic1[i] = (abs(sinal1[i]) * 255UL) / max1;
  for (int i = 0; i < N_MIC2; i++) mic2[i] = (abs(sinal2[i]) * 255UL) / max2;
}

// ==========================
// NORMALIZAR / MAIOR
// ==========================
void encontrar_maior(const uint8_t *v, int n, uint8_t *dest) {
  *dest = v[0];
  for (int i = 1; i < n; i++) if (v[i] > *dest) *dest = v[i];
}

void normalizar_vetor(const uint8_t *v, uint8_t *out, int n, uint8_t mv) {
  for (int i = 0; i < n; i++) out[i] = mv != 0 ? ((uint16_t)v[i]) / mv : 0;
}

// ==========================
// CORRELAÇÃO CRUZADA
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
// DETECTAR ÂNGULO
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
// ENVIO VIA SOCKET TCP
// Bloqueia e tenta reconectar até o receiver estar no ar
// ==========================
void enviar_dados_socket() {
  int tentativas = 0;
  while (!client.connect(SERVER_IP, SERVER_PORT)) {
    tentativas++;
    Serial.printf("[socket] receiver indisponivel, tentativa %d...\n", tentativas);
    delay(1000);
  }

  client.println("START");

  client.println("Amostras mic1:");
  for (int i = 0; i < N_MIC1; i++) {
    client.print("mic1["); client.print(i); client.print("] = "); client.println(mic1[i]);
  }

  client.println("Amostras mic2:");
  for (int i = 0; i < N_MIC2; i++) {
    client.print("mic2["); client.print(i); client.print("] = "); client.println(mic2[i]);
  }

  client.println("Resultado da correlacao cruzada:");
  for (int i = 0; i < 201; i++) {
    client.print("correlacao["); client.print(i); client.print("] = "); client.println(correlacao_resultado[i]);
  }

  client.print("max_index = ");    client.println(max_index);
  client.print("max_val = ");      client.println(max_val);
  client.print("angulo_theta = "); client.println(angulo_theta);

  client.println("END");
  client.stop();
}

// ==========================
// SETUP
// ==========================
void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  sem_iniciar = xSemaphoreCreateCounting(2, 0);
  sem_mic1_ok = xSemaphoreCreateBinary();
  sem_mic2_ok = xSemaphoreCreateBinary();

  xTaskCreatePinnedToCore(task_mic1, "mic1", 2048, NULL, 2, NULL, 0);
  xTaskCreatePinnedToCore(task_mic2, "mic2", 2048, NULL, 2, NULL, 1);

  Serial.print("Conectando WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nWiFi OK — IP: " + WiFi.localIP().toString());
  Serial.printf("Sample rate: %d Hz | mic1:%d mic2:%d amostras\n", SAMPLE_RATE, N_MIC1, N_MIC2);
}

// ==========================
// LOOP — captura e envia continuamente
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

  Serial.printf("angulo=%d  max_index=%d  max_val=%d\n", angulo_theta, max_index, max_val);

  enviar_dados_socket();
}