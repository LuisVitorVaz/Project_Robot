#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

// const char* ssid = "ESP32-Car";
// const char* password = "12345678";

const char* ssid = "Gloomy-Beholder";
const char* password = "a1b23e75z123";

WebServer server(80);

// Each PWM channel corresponds to a motor input pin (LEDC channel)
#define canalPWM1 0   // Motor1 Pin1 -> 12
#define canalPWM2 1   // Motor1 Pin2 -> 13
#define canalPWM3 2   // Motor2 Pin1 -> 14
#define canalPWM4 3   // Motor2 Pin2 -> 27
#define canalPWM5 4   // Motor3 Pin1 -> 32
#define canalPWM6 5   // Motor3 Pin2 -> 33
#define canalPWM7 6   // Motor4 Pin1 -> 25
#define canalPWM8 7   // Motor4 Pin2 -> 26

// Pin definitions
// Motor 1 (Front Left)
#define motor1Pin1 12
#define motor1Pin2 13

// Motor 2 (Front Right)
#define motor2Pin1 14
#define motor2Pin2 27

// Motor 3 (Rear Left)
#define motor3Pin1 32
#define motor3Pin2 33

// Motor 4 (Rear Right)
#define motor4Pin1 25
#define motor4Pin2 26

// === Encoder pin definitions ===
#define ENC1_A 34
#define ENC1_B 15

#define ENC2_A 22   // Changed from 2 → 35 to avoid boot pin conflict
#define ENC2_B 4

#define ENC3_A 36
#define ENC3_B 18

#define ENC4_A 39
#define ENC4_B 21

// Encoder constants
#define PULSOS_POR_ROTACAO 333.33
#define DIAMETRO_CM 6
#define CIRCUNFERENCIA_CM (PI * DIAMETRO_CM)

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

// PWM settings
#define frequenciaMotor 300
#define resolucao 8 // 0 to 255

// Global state variable
String movimentoAtual = "parado";

// Function prototypes
void parar();
void moverParaFrente();
void moverParaTras();
void virarEsquerda();
void virarDireita();

// === HTML Page with control buttons ===
const char htmlPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Controle do Carrinho</title>
  <style>
    body {
      text-align: center;
      font-family: Arial, sans-serif;
      background-color: #2c3e50; /* dark bluish-gray */
      margin: 0;
      padding: 20px;
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: flex-start;
      min-height: 100vh;
      color: #ecf0f1; /* light text */
    }
    h2 {
      color: #ecf0f1;
      margin-bottom: 20px;
    }

    /* Encoder status panel */
    .encoder-panel {
      display: grid;
      grid-template-columns: repeat(4, 1fr);
      gap: 15px;
      margin-bottom: 30px;
      width: 100%;
      max-width: 600px;
    }
    .encoder-box {
      background-color: #34495e; /* softer dark card */
      padding: 15px;
      border-radius: 10px;
      box-shadow: 0 4px 12px rgba(0,0,0,0.4);
      text-align: center;
    }
    .encoder-box h3 {
      margin: 0 0 8px 0;
      font-size: 16px;
      color: #bdc3c7;
    }
    .encoder-value {
      font-size: 22px;
      font-weight: bold;
      color: #1abc9c; /* teal highlight */
    }

    /* Control panel */
    .control-panel {
      display: inline-block;
      background-color: #34495e;
      padding: 20px;
      border-radius: 10px;
      box-shadow: 0 4px 12px rgba(0,0,0,0.4);
      margin: 0 auto;
    }
    .button-row {
      display: flex;
      justify-content: center;
    }
    .button {
      width: 70px;
      height: 70px;
      margin: 8px;
      border-radius: 10px;
      border: none;
      background-color: #2980b9;
      color: white;
      font-size: 24px;
      cursor: pointer;
      display: flex;
      align-items: center;
      justify-content: center;
      transition: all 0.2s;
      box-shadow: 0 4px 8px rgba(0,0,0,0.3);
    }
    .button:active, .button.clicked {
      background-color: #1f6391;
      transform: scale(0.95);
      box-shadow: inset 0 0 8px rgba(0,0,0,0.5);
    }
    .stop {
      background-color: #e74c3c;
    }
    .stop:active, .stop.clicked {
      background-color: #c0392b;
    }
  </style>
</head>
<body>
  <h2>Controle do Carrinho</h2>

  <div class="encoder-panel">
    <div class="encoder-box">
      <h3>Encoder 1</h3>
      <div id="enc1" class="encoder-value">0</div>
    </div>
    <div class="encoder-box">
      <h3>Encoder 2</h3>
      <div id="enc2" class="encoder-value">0</div>
    </div>
    <div class="encoder-box">
      <h3>Encoder 3</h3>
      <div id="enc3" class="encoder-value">0</div>
    </div>
    <div class="encoder-box">
      <h3>Encoder 4</h3>
      <div id="enc4" class="encoder-value">0</div>
    </div>
  </div>

  <div class="control-panel">
    <div class="button-row">
      <button onclick="sendCommand('forward')" class="button">&#9650;</button>
    </div>
    <div class="button-row">
      <button onclick="sendCommand('left')" class="button">&#9664;</button>
      <button onclick="sendCommand('stop')" class="button stop">&#9632;</button>
      <button onclick="sendCommand('right')" class="button">&#9654;</button>
    </div>
    <div class="button-row">
      <button onclick="sendCommand('backward')" class="button">&#9660;</button>
    </div>
  </div>

  <script>
    // Send command to ESP32 and highlight button
    function sendCommand(cmd) {
      let buttons = document.querySelectorAll('.button');
      buttons.forEach(btn => btn.classList.remove('clicked'));
      
      if (cmd === 'forward') document.querySelector('.button-row:nth-child(1) .button').classList.add('clicked');
      else if (cmd === 'left') document.querySelector('.button-row:nth-child(2) .button:nth-child(1)').classList.add('clicked');
      else if (cmd === 'stop') document.querySelector('.button-row:nth-child(2) .button:nth-child(2)').classList.add('clicked');
      else if (cmd === 'right') document.querySelector('.button-row:nth-child(2) .button:nth-child(3)').classList.add('clicked');
      else if (cmd === 'backward') document.querySelector('.button-row:nth-child(3) .button').classList.add('clicked');

      setTimeout(() => buttons.forEach(btn => btn.classList.remove('clicked')), 300);

      fetch("/cmd?dir=" + cmd)
        .then(response => console.log('Comando enviado: ' + cmd))
        .catch(error => console.error('Erro ao enviar comando:', error));
    }

    // Update encoder values from ESP32 variables via /encoder
    function updateEncoders() {
      fetch("/encoder")
        .then(res => res.json())
        .then(data => {
          document.getElementById("enc1").textContent = data.e1;
          document.getElementById("enc2").textContent = data.e2;
          document.getElementById("enc3").textContent = data.e3;
          document.getElementById("enc4").textContent = data.e4;
        })
        .catch(err => console.error("Erro ao buscar encoders:", err));
    }

    // Update encoders every 200ms for real-time display
    setInterval(updateEncoders, 200);
  </script>
</body>
</html>
)rawliteral";

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

  // Start Access Point
  WiFi.softAP(ssid, password);
  Serial.println("Access Point iniciado");
  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());

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