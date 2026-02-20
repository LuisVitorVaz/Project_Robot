#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

// const char* ssid = "ESP32-Car";
// const char* password = "12345678";

const char* ssid = "Gloomy-Beholder";
const char* password = "a1b23e75z123";

WebServer server(80);

// Volatile variables for interrupt service routines (ISRs)
volatile long encoder1Count = 0;
volatile long encoder2Count = 0;
volatile long encoder3Count = 0;
volatile long encoder4Count = 0;

uint8_t oldState1 = 0;
uint8_t oldState2 = 0;
uint8_t oldState3 = 0;
uint8_t oldState4 = 0;

// Lookup table for encoder direction
const int vetor[16] = {
  0, -1, 1, 2,
  1, 0, 2, -1,
  -1, 2, 0, 1,
  2, 1, -1, 0
};


// Global state variable
String movimentoAtual = "parado";

// Function prototypes
void parar();
void moverParaFrente();
void moverParaTras();
void virarEsquerda();
void virarDireita();


// === Interrupt service routines (ISRs) for each encoder ===
void IRAM_ATTR encoder1ISR() {
  uint8_t s = (digitalRead(ENC1_A) << 1) | digitalRead(ENC1_B);
  encoder1Count += vetor[(oldState1 << 2) | s];
  oldState1 = s;
}

void IRAM_ATTR encoder2ISR() {
  uint8_t s = (digitalRead(ENC2_A) << 1) | digitalRead(ENC2_B);
  encoder2Count += vetor[(oldState2 << 2) | s];
  oldState2 = s;
}

void IRAM_ATTR encoder3ISR() {
  uint8_t s = (digitalRead(ENC3_A) << 1) | digitalRead(ENC3_B);
  encoder3Count += vetor[(oldState3 << 2) | s];
  oldState3 = s;
}

void IRAM_ATTR encoder4ISR() {
  uint8_t s = (digitalRead(ENC4_A) << 1) | digitalRead(ENC4_B);
  encoder4Count += vetor[(oldState4 << 2) | s];
  oldState4 = s;
}

// Function to convert encoder counts to distance
float getDistCM(long count) {
  return (float)(count / PULSOS_POR_ROTACAO) * CIRCUNFERENCIA_CM;
}

// === Motor control functions ===
void parar() {
  // Set all motor PWM channels to 0
  ledcWrite(canalPWM1, 0);
  ledcWrite(canalPWM2, 0);
  ledcWrite(canalPWM3, 0);
  ledcWrite(canalPWM4, 0);
  ledcWrite(canalPWM5, 0);
  ledcWrite(canalPWM6, 0);
  ledcWrite(canalPWM7, 0);
  ledcWrite(canalPWM8, 0);
  Serial.println("Parando");
}

void moverParaFrente() {
  // Motores Esquerda (Motor 1 e Motor 3) - Para frente
  ledcWrite(canalPWM1, 0);
  ledcWrite(canalPWM2, 255);

  ledcWrite(canalPWM5, 0);
  ledcWrite(canalPWM6, 255);

  // Motores Direita (Motor 2 e Motor 4) - Para frente
  ledcWrite(canalPWM3, 0);
  ledcWrite(canalPWM4, 255);

  ledcWrite(canalPWM7, 0);
  ledcWrite(canalPWM8, 255);

  Serial.println("Movendo para frente");
}

void moverParaTras() {
  // Motores Esquerda (Motor 1 e Motor 3) - Para trás
  ledcWrite(canalPWM1, 255);
  ledcWrite(canalPWM2, 0);

  ledcWrite(canalPWM5, 255);
  ledcWrite(canalPWM6, 0);

  // Motores Direita (Motor 2 e Motor 4) - Para trás
  ledcWrite(canalPWM3, 255);
  ledcWrite(canalPWM4, 0);

  ledcWrite(canalPWM7, 255);
  ledcWrite(canalPWM8, 0);

  Serial.println("Movendo para trás");
}
void virarEsquerda() {
  // Right motors forward, Left motors backward
  ledcWrite(canalPWM1, 200); // Back Left
  ledcWrite(canalPWM2, 0);
  ledcWrite(canalPWM3, 0);  // Back Right
  ledcWrite(canalPWM4, 200);
  ledcWrite(canalPWM5, 200); // Front Left
  ledcWrite(canalPWM6, 0);
  ledcWrite(canalPWM7, 0); // Front Right
  ledcWrite(canalPWM8, 200);
  Serial.println("Virando para a esquerda");
}

void virarDireita() {
  // Left motors forward, Right motors backward
  ledcWrite(canalPWM1, 0); // Back Left
  ledcWrite(canalPWM2, 200);
  ledcWrite(canalPWM3, 200); // Back Right
  ledcWrite(canalPWM4, 0);
  ledcWrite(canalPWM5, 0); // Front Left
  ledcWrite(canalPWM6, 200);
  ledcWrite(canalPWM7, 200); // Front Right
  ledcWrite(canalPWM8, 0);
  Serial.println("Virando para a direita");
}

// === Web server handlers ===
void handleCommand() {
  String dir = server.arg("dir");

  Serial.print("Comando recebido: ");
  Serial.println(dir);

  // Update the global state variable
  movimentoAtual = dir;
  server.send(200, "text/plain", "OK");
}

void handleEncoders() {
  String json = "{\"e1\":" + String(encoder1Count) + ",\"e2\":" + String(encoder2Count) + ",\"e3\":" + String(encoder3Count) + ",\"e4\":" + String(encoder4Count) + "}";
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(9600);

  // Configure encoder pins
  pinMode(ENC1_A, INPUT); pinMode(ENC1_B, INPUT);
  pinMode(ENC2_A, INPUT); pinMode(ENC2_B, INPUT);
  pinMode(ENC3_A, INPUT); pinMode(ENC3_B, INPUT);
  pinMode(ENC4_A, INPUT); pinMode(ENC4_B, INPUT);

  // Read initial states
  oldState1 = (digitalRead(ENC1_A) << 1) | digitalRead(ENC1_B);
  oldState2 = (digitalRead(ENC2_A) << 1) | digitalRead(ENC2_B);
  oldState3 = (digitalRead(ENC3_A) << 1) | digitalRead(ENC3_B);
  oldState4 = (digitalRead(ENC4_A) << 1) | digitalRead(ENC4_B);

  // Attach interrupts for encoders
  attachInterrupt(digitalPinToInterrupt(ENC1_A), encoder1ISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC1_B), encoder1ISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC2_A), encoder2ISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC2_B), encoder2ISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC3_A), encoder3ISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC3_B), encoder3ISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC4_A), encoder4ISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENC4_B), encoder4ISR, CHANGE);
  
  // Configure motor PWM channels
  ledcSetup(canalPWM1, frequenciaMotor, resolucao);
  ledcAttachPin(motor1Pin1, canalPWM1);
  ledcSetup(canalPWM2, frequenciaMotor, resolucao);
  ledcAttachPin(motor1Pin2, canalPWM2);
  ledcSetup(canalPWM3, frequenciaMotor, resolucao);
  ledcAttachPin(motor2Pin1, canalPWM3);
  ledcSetup(canalPWM4, frequenciaMotor, resolucao);
  ledcAttachPin(motor2Pin2, canalPWM4);
  ledcSetup(canalPWM5, frequenciaMotor, resolucao);
  ledcAttachPin(motor3Pin1, canalPWM5);
  ledcSetup(canalPWM6, frequenciaMotor, resolucao);
  ledcAttachPin(motor3Pin2, canalPWM6);
  ledcSetup(canalPWM7, frequenciaMotor, resolucao);
  ledcAttachPin(motor4Pin1, canalPWM7);
  ledcSetup(canalPWM8, frequenciaMotor, resolucao);
  ledcAttachPin(motor4Pin2, canalPWM8);

  // Initially, all motors are off
  parar();

  // Set up web server routes
  server.on("/", []() {
    server.send_P(200, "text/html", htmlPage);
  });
  server.on("/cmd", handleCommand);
  server.on("/encoder", handleEncoders); // Added the encoder endpoint
  server.begin();
  Serial.println("Servidor iniciado");
}

void loop() {
  server.handleClient();

  // Control the motors based on the current state
  if (movimentoAtual == "forward") {
    moverParaFrente();
  } else if (movimentoAtual == "backward") {
    moverParaTras();
  } else if (movimentoAtual == "left") {
    virarEsquerda();
  } else if (movimentoAtual == "right") {
    virarDireita();
  } else {
    // If the command is "stop" or an unknown state, stop the motors
    parar();
  }
  
  delay(50); // Small delay to prevent processor overload
}