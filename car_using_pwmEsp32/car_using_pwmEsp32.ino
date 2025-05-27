#include <WiFi.h>
#include <WebServer.h>
#include <Arduino.h>

// WiFi
const char* ssid = "ESP32-Car";
const char* password = "12345678";

WebServer server(80);

// Pino LED indicador
const int ledPin = 2;

// Controle dos motores
const int motor1Pin1 = 26, motor1Pin2 = 27; // Motor frente esquerda
const int motor2Pin1 = 14, motor2Pin2 = 12; // Motor frente direita
const int motor3Pin1 = 25, motor3Pin2 = 33; // Motor trás esquerda
const int motor4Pin1 = 32, motor4Pin2 = 13; // Motor trás direita (alterado de 35 para 13)


// Configurações PWM
const int pwmFreq = 1000;
const int pwmResolution = 8;
const int maxDutyCycle = 255;
const int normalSpeed = 200;    // Velocidade normal (78%)
const int frontCurveSpeed = 153; // 60% para frente na curva
const int rearCurveSpeed = 102;  // 40% para trás na curva

// Canais PWM para cada motor
const int pwmChannel1 = 0; // Motor 1
const int pwmChannel2 = 1; // Motor 2
const int pwmChannel3 = 2; // Motor 3
const int pwmChannel4 = 3; // Motor 4

String movimentoAtual = "parado";

// Página HTML com botões de controle
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
      background-color: #f0f0f0;
      margin: 0;
      padding: 20px;
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
      min-height: 100vh;
    }
    h2 {
      color: #333;
      margin-bottom: 20px;
    }
    .control-panel {
      display: inline-block;
      background-color: #fff;
      padding: 20px;
      border-radius: 10px;
      box-shadow: 0 0 10px rgba(0,0,0,0.1);
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
      background-color: #3498db;
      color: white;
      font-size: 24px;
      cursor: pointer;
      display: flex;
      align-items: center;
      justify-content: center;
      transition: all 0.2s;
    }
    .button:active {
      background-color: #2980b9;
      transform: scale(0.95);
      box-shadow: inset 0 0 10px rgba(0,0,0,0.3);
    }
    .button.clicked {
      background-color: #2980b9;
      transform: scale(0.95);
      box-shadow: inset 0 0 10px rgba(0,0,0,0.3);
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

  <div class="control-panel">
    <!-- Botão para frente -->
    <div class="button-row">
      <button onclick="sendCommand('forward')" class="button">&#9650;</button>
    </div>
    
    <!-- Botões esquerda, parar e direita -->
    <div class="button-row">
      <button onclick="sendCommand('left')" class="button">&#9664;</button>
      <button onclick="sendCommand('stop')" class="button stop">&#9632;</button>
      <button onclick="sendCommand('right')" class="button">&#9654;</button>
    </div>
    
    <!-- Botão para trás -->
    <div class="button-row">
      <button onclick="sendCommand('backward')" class="button">&#9660;</button>
    </div>
  </div>

  <script>
    function sendCommand(cmd) {
      let buttons = document.querySelectorAll('.button');
      
      buttons.forEach(btn => btn.classList.remove('clicked'));
      
      if (cmd === 'forward') {
        document.querySelector('.button-row:nth-child(1) .button').classList.add('clicked');
      } else if (cmd === 'left') {
        document.querySelector('.button-row:nth-child(2) .button:nth-child(1)').classList.add('clicked');
      } else if (cmd === 'stop') {
        document.querySelector('.button-row:nth-child(2) .button:nth-child(2)').classList.add('clicked');
      } else if (cmd === 'right') {
        document.querySelector('.button-row:nth-child(2) .button:nth-child(3)').classList.add('clicked');
      } else if (cmd === 'backward') {
        document.querySelector('.button-row:nth-child(3) .button').classList.add('clicked');
      }
      
      setTimeout(() => {
        buttons.forEach(btn => btn.classList.remove('clicked'));
      }, 300);
      
      fetch("/cmd?dir=" + cmd)
        .then(response => console.log('Comando enviado: ' + cmd))
        .catch(error => console.error('Erro ao enviar comando:', error));
    }
  </script>
</body>
</html>
)rawliteral";

// =================== Configuração PWM ===================
void configurarPWM() {
  // Configura canais PWM usando a nova API do ESP32 3.x
  ledcAttach(motor1Pin2, pwmFreq, pwmResolution); // Canal 0
  ledcAttach(motor2Pin2, pwmFreq, pwmResolution); // Canal 1
  ledcAttach(motor3Pin2, pwmFreq, pwmResolution); // Canal 2
  ledcAttach(motor4Pin2, pwmFreq, pwmResolution); // Canal 3
  
  Serial.println("PWM configurado com sucesso");
}

// =================== Funções de Movimento ===================
void moverParaFrente() {
  // Todos os motores para frente com velocidade normal
  digitalWrite(motor1Pin1, HIGH); 
  ledcWrite(motor1Pin2, maxDutyCycle - normalSpeed);
  
  digitalWrite(motor2Pin1, HIGH); 
  ledcWrite(motor2Pin2, maxDutyCycle - normalSpeed);
  
  digitalWrite(motor3Pin1, HIGH); 
  ledcWrite(motor3Pin2, maxDutyCycle - normalSpeed);
  
  digitalWrite(motor4Pin1, HIGH); 
  ledcWrite(motor4Pin2, maxDutyCycle - normalSpeed);
  
  Serial.println("Movendo para frente");
}

void moverParaTras() {
  // Todos os motores para trás com velocidade normal
  digitalWrite(motor1Pin1, LOW); 
  ledcWrite(motor1Pin2, normalSpeed);
  
  digitalWrite(motor2Pin1, LOW); 
  ledcWrite(motor2Pin2, normalSpeed);
  
  digitalWrite(motor3Pin1, LOW); 
  ledcWrite(motor3Pin2, normalSpeed);
  
  digitalWrite(motor4Pin1, LOW); 
  ledcWrite(motor4Pin2, normalSpeed);
  
  Serial.println("Movendo para trás");
}

void virarEsquerda() {
  // Lado direito para frente (motor2 e motor4) - velocidade maior na frente
  digitalWrite(motor2Pin1, HIGH); 
  ledcWrite(motor2Pin2, maxDutyCycle - frontCurveSpeed); // 60% na frente
  
  digitalWrite(motor4Pin1, HIGH); 
  ledcWrite(motor4Pin2, maxDutyCycle - rearCurveSpeed);  // 40% atrás
  
  // Lado esquerdo para trás (motor1 e motor3) - velocidade maior na frente
  digitalWrite(motor1Pin1, LOW); 
  ledcWrite(motor1Pin2, frontCurveSpeed); // 60% na frente
  
  digitalWrite(motor3Pin1, LOW); 
  ledcWrite(motor3Pin2, rearCurveSpeed);  // 40% atrás
  
  Serial.println("Virando à esquerda");
}

void virarDireita() {
  // Lado esquerdo para frente (motor1 e motor3) - velocidade maior na frente
  digitalWrite(motor1Pin1, HIGH); 
  ledcWrite(motor1Pin2, maxDutyCycle - frontCurveSpeed); // 60% na frente
  
  digitalWrite(motor3Pin1, HIGH); 
  ledcWrite(motor3Pin2, maxDutyCycle - rearCurveSpeed);  // 40% atrás
  
  // Lado direito para trás (motor2 e motor4) - velocidade maior na frente
  digitalWrite(motor2Pin1, LOW); 
  ledcWrite(motor2Pin2, frontCurveSpeed); // 60% na frente
  
  digitalWrite(motor4Pin1, LOW); 
  ledcWrite(motor4Pin2, rearCurveSpeed);  // 40% atrás
  
  Serial.println("Virando à direita");
}

void parar() {
  // Para todos os motores
  digitalWrite(motor1Pin1, LOW); 
  ledcWrite(motor1Pin2, 0);
  
  digitalWrite(motor2Pin1, LOW); 
  ledcWrite(motor2Pin2, 0);
  
  digitalWrite(motor3Pin1, LOW); 
  ledcWrite(motor3Pin2, 0);
  
  digitalWrite(motor4Pin1, LOW); 
  ledcWrite(motor4Pin2, 0);
  
  Serial.println("Parando");
}

// =================== Web Server ===================
void piscarLED() {
  digitalWrite(ledPin, HIGH);
  delay(100);
  digitalWrite(ledPin, LOW);
}

void handleCommand() {
  String dir = server.arg("dir");
  Serial.print("Comando recebido: ");
  Serial.println(dir);

  piscarLED();

  if (dir == "forward" || dir == "backward" || dir == "left" || dir == "right") {
    movimentoAtual = dir;
  } else if (dir == "stop") {
    movimentoAtual = "parado";
    parar();
  }

  server.send(200, "text/plain", "OK");
}

// =================== Setup ===================
void setup() {
  Serial.begin(115200);
  Serial.println("Iniciando ESP32 Car Controller...");

  // Configura pinos
  pinMode(ledPin, OUTPUT);
  pinMode(motor1Pin1, OUTPUT); pinMode(motor1Pin2, OUTPUT);
  pinMode(motor2Pin1, OUTPUT); pinMode(motor2Pin2, OUTPUT);
  pinMode(motor3Pin1, OUTPUT); pinMode(motor3Pin2, OUTPUT);
  pinMode(motor4Pin1, OUTPUT); pinMode(motor4Pin2, OUTPUT);

  // Configura PWM
  configurarPWM();
  
  // Para todos os motores inicialmente
  parar();

  // Configura WiFi como Access Point
  WiFi.softAP(ssid, password);
  Serial.println("Access Point iniciado");
  Serial.print("SSID: ");
  Serial.println(ssid);
  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());

  // Configura rotas do servidor web
  server.on("/", []() {
    server.send_P(200, "text/html", htmlPage);
  });
  server.on("/cmd", handleCommand);
  
  // Inicia servidor
  server.begin();
  Serial.println("Servidor web iniciado");
  Serial.println("Conecte-se ao WiFi 'ESP32-Car' e acesse: http://192.168.4.1");
}

// =================== Loop ===================
void loop() {
  server.handleClient();

  // Executa movimento atual
  if (movimentoAtual == "forward") {
    moverParaFrente();
  } else if (movimentoAtual == "backward") {
    moverParaTras();
  } else if (movimentoAtual == "left") {
    virarEsquerda();
  } else if (movimentoAtual == "right") {
    virarDireita();
  } else {
    parar();
  }

  delay(50); // Pequeno delay para estabilidade
}