#include <WiFi.h>
#include <WebServer.h>

// Rede Wi-Fi criada pelo ESP32
const char* ssid = "ESP32-Car";
const char* password = "12345678";

WebServer server(80);

// Controle de estado e pinos
String movimentoAtual = "parado";
const int ledPin = 2;
const int motor1Pin1 = 26;
const int motor1Pin2 = 27;
const int motor2Pin1 = 14;
const int motor2Pin2 = 12;
const int motor3Pin1 = 25;
const int motor3Pin2 = 33;
const int motor4Pin1 = 32;
const int motor4Pin2 = 35;

// PWM
const int freq = 300;
const int resolution = 8;
int dutyCycle = 255;

// Função para configurar um canal PWM
void setupPWMChannel(int pin, int channel) {
  ledcSetup(channel, freq, resolution);
  ledcAttachPin(pin, channel);
}

// HTML permanece igual...
const char htmlPage[] PROGMEM = R"rawliteral(
<!-- TODO: HTML já fornecido acima permanece exatamente igual -->
)rawliteral";

// Comandos de movimento
void moverParaFrente() {
  digitalWrite(motor1Pin1, HIGH); digitalWrite(motor1Pin2, LOW);
  digitalWrite(motor2Pin1, HIGH); digitalWrite(motor2Pin2, LOW);
  digitalWrite(motor3Pin1, HIGH); digitalWrite(motor3Pin2, LOW);
  digitalWrite(motor4Pin1, HIGH); digitalWrite(motor4Pin2, LOW);
  Serial.println("Movendo para frente");
}

void moverParaTras() {
  ledcWrite(0, 0);  ledcWrite(1, dutyCycle);
  ledcWrite(2, 0);  ledcWrite(3, dutyCycle);
  ledcWrite(4, 0);  ledcWrite(5, dutyCycle);
  ledcWrite(6, 0);  ledcWrite(7, dutyCycle);
  Serial.println("Movendo para trás");
}

void virarEsquerda() {
  digitalWrite(motor2Pin1, HIGH); digitalWrite(motor2Pin2, LOW);
  digitalWrite(motor4Pin1, HIGH); digitalWrite(motor4Pin2, LOW);
  digitalWrite(motor1Pin1, LOW);  digitalWrite(motor1Pin2, HIGH);
  digitalWrite(motor3Pin1, LOW);  digitalWrite(motor3Pin2, HIGH);
  Serial.println("Virando à esquerda");
}

void virarDireita() {
  digitalWrite(motor1Pin1, HIGH); digitalWrite(motor1Pin2, LOW);
  digitalWrite(motor3Pin1, HIGH); digitalWrite(motor3Pin2, LOW);
  digitalWrite(motor2Pin1, LOW);  digitalWrite(motor2Pin2, HIGH);
  digitalWrite(motor4Pin1, LOW);  digitalWrite(motor4Pin2, HIGH);
  Serial.println("Virando à direita");
}

void parar() {
  dutyCycle = 255;
  ledcWrite(0, dutyCycle);
  digitalWrite(motor1Pin2, LOW);
  digitalWrite(motor2Pin1, LOW); digitalWrite(motor2Pin2, LOW);
  digitalWrite(motor3Pin1, LOW); digitalWrite(motor3Pin2, LOW);
  digitalWrite(motor4Pin1, LOW); digitalWrite(motor4Pin2, LOW);
  Serial.println("Parando");
}

void piscarLED() {
  digitalWrite(ledPin, HIGH);
  delay(100);
  digitalWrite(ledPin, LOW);
}

void handleCommand() {
  String dir = server.arg("dir");
  Serial.print("Comando recebido: "); Serial.println(dir);
  piscarLED();
  if (dir == "forward") {
    movimentoAtual = dir;
    moverParaFrente();
  } else if (dir == "backward") {
    movimentoAtual = dir;
    moverParaTras();
  } else if (dir == "left") {
    movimentoAtual = dir;
    virarEsquerda();
  } else if (dir == "right") {
    movimentoAtual = dir;
    virarDireita();
  } else if (dir == "stop") {
    movimentoAtual = "parado";
    parar();
  }
  server.send(200, "text/plain", "OK");
}

void setup() {
  Serial.begin(115200);

  // Inicialização de pinos
  pinMode(ledPin, OUTPUT);
  pinMode(motor1Pin1, OUTPUT); pinMode(motor1Pin2, OUTPUT);
  pinMode(motor2Pin1, OUTPUT); pinMode(motor2Pin2, OUTPUT);
  pinMode(motor3Pin1, OUTPUT); pinMode(motor3Pin2, OUTPUT);
  pinMode(motor4Pin1, OUTPUT); pinMode(motor4Pin2, OUTPUT);

  // Configuração dos canais PWM para motor1Pin1/Pin2, motor2Pin1/Pin2 etc.
  setupPWMChannel(motor1Pin1, 0);
  setupPWMChannel(motor1Pin2, 1);
  setupPWMChannel(motor2Pin1, 2);
  setupPWMChannel(motor2Pin2, 3);
  setupPWMChannel(motor3Pin1, 4);
  setupPWMChannel(motor3Pin2, 5);
  setupPWMChannel(motor4Pin1, 6);
  setupPWMChannel(motor4Pin2, 7);

  // Inicializa Wi-Fi como Access Point
  WiFi.softAP(ssid, password);
  Serial.println("WiFi AP iniciado. Endereço IP: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", HTTP_GET, []() {
    server.send_P(200, "text/html", htmlPage);
  });
  server.on("/cmd", HTTP_GET, handleCommand);
  server.begin();
  Serial.println("Servidor iniciado.");
}

void loop() {
  server.handleClient();
}
