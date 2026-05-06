#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WiFiUdp.h>

const char* ssid = "Gloomy-Beholder";
const char* password = "a1b23e75z123";

WebServer server(80);

//  PWM channels
#define canalPWM1 0
#define canalPWM2 1
#define canalPWM3 2
#define canalPWM4 3
#define canalPWM5 4
#define canalPWM6 5
#define canalPWM7 6
#define canalPWM8 7

// Motor pins
#define motor1Pin1 12
#define motor1Pin2 13
#define motor2Pin1 14
#define motor2Pin2 27
#define motor3Pin1 32
#define motor3Pin2 33
#define motor4Pin1 25
#define motor4Pin2 26

// PWM settings
#define frequenciaMotor 300
#define resolucao 8

// ================= UDP / TCP DISCOVERY =================
#define UDP_PORT  4210
#define TALK_PORT 4211

// ================= THREADS =================
#define STACK_SIZE 4096

TaskHandle_t taskWifiHandle      = NULL;
TaskHandle_t taskSerialHandle    = NULL;
TaskHandle_t taskDiscoveryHandle = NULL;
TaskHandle_t taskMotorHandle     = NULL;

// ================= MUTEX =================
SemaphoreHandle_t mutexMovimento;
SemaphoreHandle_t mutexTcp;
SemaphoreHandle_t mutexSerial;  // protege Serial.print entre tasks

// ================= DADOS COMPARTILHADOS =================
String movimentoAtual = "parado";

WiFiUDP udp;
WiFiClient tcpClient;
bool tcpConectado  = false;
bool wifiPronto    = false;

// Últimos dados recebidos do Arduino (protegidos por mutexSerial)
struct DadosSensores {
  float acX, acY, acZ;
  float gyX, gyY, gyZ;
  float temperatura;
  int   encoderNum;
  char  encoderDir;
  int   encoderPasso;
  int   encoderVolta;
  bool  encoderAtualizado;
} sensores = {};

// ================= CONTROLE DOS MOTORES =================
void parar() {
  for(int i = 0; i < 8; i++) ledcWrite(i, 0);
}

void moverParaFrente() {
  ledcWrite(canalPWM1, 0);
  ledcWrite(canalPWM2, 0);
  ledcWrite(canalPWM5, 200);
  ledcWrite(canalPWM6, 200);
  ledcWrite(canalPWM3, 0);
  ledcWrite(canalPWM4, 0);
  ledcWrite(canalPWM7, 200);
  ledcWrite(canalPWM8, 200);
}

void moverParaTras() {
  ledcWrite(canalPWM1, 200);
  ledcWrite(canalPWM2, 200);
  ledcWrite(canalPWM5, 0);
  ledcWrite(canalPWM6, 0);
  ledcWrite(canalPWM3, 200);
  ledcWrite(canalPWM4, 200);
  ledcWrite(canalPWM7, 0);
  ledcWrite(canalPWM8, 0);
}

void virarEsquerda() {
  ledcWrite(canalPWM1, 0);
  ledcWrite(canalPWM2, 0);
  ledcWrite(canalPWM5, 200);
  ledcWrite(canalPWM6, 200);
  ledcWrite(canalPWM3, 200);
  ledcWrite(canalPWM4, 200);
  ledcWrite(canalPWM7, 0);
  ledcWrite(canalPWM8, 0);
}

void virarDireita() {
  ledcWrite(canalPWM1, 200);
  ledcWrite(canalPWM2, 200);
  ledcWrite(canalPWM5, 0);
  ledcWrite(canalPWM6, 0);
  ledcWrite(canalPWM3, 0);
  ledcWrite(canalPWM4, 0);
  ledcWrite(canalPWM7, 200);
  ledcWrite(canalPWM8, 200);
}

void executarMovimento(const String& mov) {
  if      (mov == "forward")  moverParaFrente();
  else if (mov == "backward") moverParaTras();
  else if (mov == "left")     virarEsquerda();
  else if (mov == "right")    virarDireita();
  else                        parar();
}

// ================= PWM SETUP =================
void setupMotores() {
  ledcSetup(canalPWM1, frequenciaMotor, resolucao); ledcAttachPin(motor1Pin1, canalPWM1);
  ledcSetup(canalPWM2, frequenciaMotor, resolucao); ledcAttachPin(motor1Pin2, canalPWM2);
  ledcSetup(canalPWM3, frequenciaMotor, resolucao); ledcAttachPin(motor2Pin1, canalPWM3);
  ledcSetup(canalPWM4, frequenciaMotor, resolucao); ledcAttachPin(motor2Pin2, canalPWM4);
  ledcSetup(canalPWM5, frequenciaMotor, resolucao); ledcAttachPin(motor3Pin1, canalPWM5);
  ledcSetup(canalPWM6, frequenciaMotor, resolucao); ledcAttachPin(motor3Pin2, canalPWM6);
  ledcSetup(canalPWM7, frequenciaMotor, resolucao); ledcAttachPin(motor4Pin1, canalPWM7);
  ledcSetup(canalPWM8, frequenciaMotor, resolucao); ledcAttachPin(motor4Pin2, canalPWM8);
}

// ================= WEB =================
void handleCommand() {
  String dir = server.arg("dir");
  if (xSemaphoreTake(mutexMovimento, pdMS_TO_TICKS(10)) == pdTRUE) {
    movimentoAtual = dir;
    xSemaphoreGive(mutexMovimento);
  }
  server.send(200, "text/plain", "OK");
}

// ================= PARSE SERIAL =================
// Parseia linha recebida do Arduino e salva em `sensores`
void parsearLinha(const String& linha) {

  // ================= PACOTE UNICO =================
  // DATA:acx,acy,acz,gyx,gyy,gyz,tmp,dir,passo,volta,total x4
  if (linha.startsWith("DATA:")) {

    String d = linha.substring(5);

    float valores[7]; // acX, acY, acZ, gyX, gyY, gyZ, temp
    int index = 0;

    // ----------- PARSE DOS 7 PRIMEIROS FLOATS -----------
    for (int i = 0; i < 7; i++) {
      int virgula = d.indexOf(',');
      if (virgula == -1) return;

      valores[i] = d.substring(0, virgula).toFloat();
      d = d.substring(virgula + 1);
    }

    sensores.acX = valores[0];
    sensores.acY = valores[1];
    sensores.acZ = valores[2];
    sensores.gyX = valores[3];
    sensores.gyY = valores[4];
    sensores.gyZ = valores[5];
    sensores.temperatura = valores[6];

    // ----------- PARSE DOS 4 ENCODERS -----------
    for (int i = 1; i <= 4; i++) {

      // direção
      int virgula = d.indexOf(',');
      if (virgula == -1) return;
      char dir = d.substring(0, virgula)[0];
      d = d.substring(virgula + 1);

      // passo
      virgula = d.indexOf(',');
      if (virgula == -1) return;
      int passo = d.substring(0, virgula).toInt();
      d = d.substring(virgula + 1);

      // volta
      virgula = d.indexOf(',');
      if (virgula == -1) return;
      int volta = d.substring(0, virgula).toInt();
      d = d.substring(virgula + 1);

      // total
      virgula = d.indexOf(',');
      int total;

      if (virgula == -1) {
        total = d.toInt(); // último elemento
        d = "";
      } else {
        total = d.substring(0, virgula).toInt();
        d = d.substring(virgula + 1);
      }

      // salva SOMENTE o último encoder (mantendo sua struct intacta)
      sensores.encoderNum   = i;
      sensores.encoderDir   = dir;
      sensores.encoderPasso = passo;
      sensores.encoderVolta = volta;
      sensores.encoderAtualizado = true;
    }
  }
}

// ================= TASK: WiFi =================
// Core 1 — sobe o AP e o WebServer, fica servindo clientes
void taskWifi(void* param) {
  WiFi.softAP(ssid, password);

  if (xSemaphoreTake(mutexSerial, portMAX_DELAY) == pdTRUE) {
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());
    xSemaphoreGive(mutexSerial);
  }

  server.on("/cmd", handleCommand);
  server.begin();
  wifiPronto = true;

  for (;;) {
    server.handleClient();
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

// ================= TASK: Serial2 (Arduino) =================
// Core 0 — leitura contínua sem perder pacotes de encoder
void taskSerial(void* param) {
  for (;;) {
    if (Serial2.available()) {
      String linha = Serial2.readStringUntil('\n');
      linha.trim();
      if (linha.length() == 0) {
        vTaskDelay(pdMS_TO_TICKS(1));
        continue;
      }

      if (xSemaphoreTake(mutexSerial, pdMS_TO_TICKS(5)) == pdTRUE) {
        parsearLinha(linha);

        // Log no monitor
        if (linha.startsWith("ENC:")) {
          Serial.println("[ENC] " + linha.substring(4));
        } else {
          Serial.println("[SER] " + linha);
        }
        xSemaphoreGive(mutexSerial);
      }

      // Encaminha via TCP se conectado
      if (xSemaphoreTake(mutexTcp, pdMS_TO_TICKS(5)) == pdTRUE) {
        if (tcpConectado && tcpClient.connected()) {
          tcpClient.println(linha);
        }
        xSemaphoreGive(mutexTcp);
      }
    }
    // Sem delay fixo — cede apenas se não há dados
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

// ================= TASK: UDP Discovery + TCP =================
// Core 0 — escuta broadcast "hello", conecta TCP
void taskDiscovery(void* param) {
  // Aguarda WiFi subir
  while (!wifiPronto) vTaskDelay(pdMS_TO_TICKS(100));

  udp.begin(UDP_PORT);

  if (xSemaphoreTake(mutexSerial, portMAX_DELAY) == pdTRUE) {
    Serial.println("UDP discovery aguardando na porta " + String(UDP_PORT));
    xSemaphoreGive(mutexSerial);
  }

  for (;;) {
    int tamanho = udp.parsePacket();
    if (tamanho > 0) {
      char buffer[64];
      int len = udp.read(buffer, sizeof(buffer) - 1);
      if (len > 0) {
        buffer[len] = '\0';
        if (String(buffer) == "hello") {
          IPAddress ip = udp.remoteIP();

          if (xSemaphoreTake(mutexSerial, pdMS_TO_TICKS(10)) == pdTRUE) {
            Serial.println("Broadcast 'hello' de: " + ip.toString());
            xSemaphoreGive(mutexSerial);
          }

          if (xSemaphoreTake(mutexTcp, pdMS_TO_TICKS(10)) == pdTRUE) {
            if (!tcpConectado) {
              if (tcpClient.connect(ip, TALK_PORT)) {
                tcpConectado = true;
                if (xSemaphoreTake(mutexSerial, pdMS_TO_TICKS(10)) == pdTRUE) {
                  Serial.println("TCP conectado: " + ip.toString() + ":" + String(TALK_PORT));
                  xSemaphoreGive(mutexSerial);
                }
              }
            }
            xSemaphoreGive(mutexTcp);
          }
        }
      }
    }
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}

// ================= TASK: Motor =================
// Core 1 — executa movimento lendo movimentoAtual com mutex
void taskMotor(void* param) {
  for (;;) {
    String mov;
    if (xSemaphoreTake(mutexMovimento, pdMS_TO_TICKS(10)) == pdTRUE) {
      mov = movimentoAtual;
      xSemaphoreGive(mutexMovimento);
    }
    executarMovimento(mov);
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

// ================= SETUP =================
void setup() {
  Serial.begin(9600);
  Serial2.begin(9600, SERIAL_8N1, 16, 17);

  setupMotores();
  parar();

  mutexMovimento = xSemaphoreCreateMutex();
  mutexTcp       = xSemaphoreCreateMutex();
  mutexSerial    = xSemaphoreCreateMutex();

  // Core 0: serial contínua (encoder) + discovery
  xTaskCreatePinnedToCore(taskSerial,    "Serial",    STACK_SIZE, NULL, 2, &taskSerialHandle,    0);
  xTaskCreatePinnedToCore(taskDiscovery, "Discovery", STACK_SIZE, NULL, 1, &taskDiscoveryHandle, 0);

  // Core 1: WiFi/WebServer + motor
  xTaskCreatePinnedToCore(taskWifi,  "WiFi",  STACK_SIZE, NULL, 1, &taskWifiHandle,  1);
  xTaskCreatePinnedToCore(taskMotor, "Motor", STACK_SIZE, NULL, 1, &taskMotorHandle, 1);
}

// loop() fica livre — FreeRTOS assume o controle
void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}