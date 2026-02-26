#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "Gloomy-Beholder";
const char* password = "a1b23e75z123";

WebServer server(80);

// 12,13,14,27 mover para frente
// 25,26,32,33 mover para traz

// 27,14 lado direito frente
// 26,25 lado direito traz

// const int motor1Pin1 = 12, 
// const int motor1Pin2 = 13; // Frente esquerda
// const int motor2Pin1 = 14, motor2Pin2 = 27; // Frente direita
// const int motor3Pin1 = 32, motor3Pin2 = 33; // Trás esquerda
// const int motor4Pin1 = 25, motor4Pin2 = 26; // Trás direita

// PWM channels
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

String movimentoAtual = "parado";

// ================= HTML =================
const char htmlPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Controle do Carrinho</title>
<style>
body {
  text-align:center;
  font-family:Arial;
  background:#2c3e50;
  color:white;
  padding-top:40px;
}
.button {
  width:70px;
  height:70px;
  margin:8px;
  font-size:24px;
  border:none;
  border-radius:10px;
  background:#2980b9;
  color:white;
}
.stop { background:#e74c3c; }
</style>
</head>
<body>

<h2>Controle do Carrinho</h2>

<div>
  <button onclick="sendCommand('forward')" class="button">&#9650;</button>
</div>

<div>
  <button onclick="sendCommand('left')" class="button">&#9664;</button>
  <button onclick="sendCommand('stop')" class="button stop">&#9632;</button>
  <button onclick="sendCommand('right')" class="button">&#9654;</button>
</div>

<div>
  <button onclick="sendCommand('backward')" class="button">&#9660;</button>
</div>

<script>
function sendCommand(cmd){
  fetch("/cmd?dir=" + cmd);
}
</script>

</body>
</html>
)rawliteral";

// ================= CONTROLE DOS MOTORES =================
void parar() {
  for(int i=0;i<8;i++) ledcWrite(i,0);
}

void moverParaFrente() {
  // Motores Esquerda (Motor 1 e Motor 3) - Para frente
  ledcWrite(canalPWM1, 0);
  ledcWrite(canalPWM2, 0);

  ledcWrite(canalPWM5, 200);
  ledcWrite(canalPWM6, 200);

  // // Motores Direita (Motor 2 e Motor 4) - Para frente
  ledcWrite(canalPWM3, 0);
  ledcWrite(canalPWM4, 0);

  ledcWrite(canalPWM7, 200);
  ledcWrite(canalPWM8, 200);

  Serial.println("Movendo para frente");
}

void moverParaTras() {
  // Motores Esquerda (Motor 1 e Motor 3) - Para trás
  ledcWrite(canalPWM1, 200);
  ledcWrite(canalPWM2, 200);

  ledcWrite(canalPWM5, 0);
  ledcWrite(canalPWM6, 0);

  // // Motores Direita (Motor 2 e Motor 4) - Para trás
  ledcWrite(canalPWM3, 200);
  ledcWrite(canalPWM4, 200);

  ledcWrite(canalPWM7, 0);
  ledcWrite(canalPWM8, 0);

  Serial.println("Movendo para trás");
}

void virarEsquerda() {

  // Motores Esquerda (Motor 1 e Motor 3)
  ledcWrite(canalPWM1, 0); // Back Left
  ledcWrite(canalPWM2, 0);

  ledcWrite(canalPWM5, 200); // Front Left
  ledcWrite(canalPWM6, 200);

  // // Motores Direita (Motor 2 e Motor 4)
  ledcWrite(canalPWM3, 200);  // Back Right
  ledcWrite(canalPWM4, 200);

  ledcWrite(canalPWM7, 0); // Front Right
  ledcWrite(canalPWM8, 0);
  
  Serial.println("Virando para a esquerda");
}

void virarDireita() {
  
  // Motores Esquerda (Motor 1 e Motor 3)
  ledcWrite(canalPWM1, 200); // Back Left
  ledcWrite(canalPWM2, 200);

  ledcWrite(canalPWM5, 0); // Front Left
  ledcWrite(canalPWM6, 0);

  // // Motores Direita (Motor 2 e Motor 4)
  ledcWrite(canalPWM3, 0); // Back Right
  ledcWrite(canalPWM4, 0);

  ledcWrite(canalPWM7, 200); // Front Right
  ledcWrite(canalPWM8, 200);

  Serial.println("Virando para a direita");
}

// ================= WEB =================
void handleCommand() {
  movimentoAtual = server.arg("dir");
  server.send(200, "text/plain", "OK");
}

void setup() {
  Serial.begin(9600);

  // PWM setup
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

  parar();

  WiFi.softAP(ssid, password);

  server.on("/", []() {
    server.send_P(200, "text/html", htmlPage);
  });

  server.on("/cmd", handleCommand);
  server.begin();
}

void loop() {
  server.handleClient();

  if (movimentoAtual == "forward") moverParaFrente();
  else if (movimentoAtual == "backward") moverParaTras();
  else if (movimentoAtual == "left") virarEsquerda();
  else if (movimentoAtual == "right") virarDireita();
  else parar();

  delay(50);
}