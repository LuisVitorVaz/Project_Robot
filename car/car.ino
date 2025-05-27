#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "ESP32-Car";
const char* password = "12345678";

WebServer server(80);

// Definição dos pinos de controle dos motores
// Motor 1 (Frente Esquerdo)
// Velocidade

String movimentoAtual = "parado";
const int ledPin = 2; 
// motor 1 (frente esquerdo)
const int motor1Pin1 = 26;
const int motor1Pin2 = 27;

// Motor 2 (Frente Direito)
const int motor2Pin1 = 14;
const int motor2Pin2 = 12;

// Motor 3 (Traseiro Esquerdo)
const int motor3Pin1 = 25;
const int motor3Pin2 = 33;

// Motor 4 (Traseiro Direito)
const int motor4Pin1 = 32;
const int motor4Pin2 = 35;

// Página HTML com botões de controle usando imagens Base64
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
      // Encontra o botão clicado com base no comando
      let buttons = document.querySelectorAll('.button');
      
      // Remove a classe 'clicked' de todos os botões
      buttons.forEach(btn => btn.classList.remove('clicked'));
      
      // Adiciona a classe 'clicked' ao botão correspondente
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
      
      // Após 300ms, remove a classe 'clicked' para retornar ao visual normal
      setTimeout(() => {
        buttons.forEach(btn => btn.classList.remove('clicked'));
      }, 300);
      
      // Envia o comando para o ESP32
      fetch("/cmd?dir=" + cmd)
        .then(response => console.log('Comando enviado: ' + cmd))
        .catch(error => console.error('Erro ao enviar comando:', error));
    }
  </script>
</body>
</html>
)rawliteral";

// Controle dos motores
void moverParaFrente() {
  // Motor 1 - Frente
  digitalWrite(motor1Pin1, HIGH);
  digitalWrite(motor1Pin2, LOW);
  
  // Motor 2 - Frente
  digitalWrite(motor2Pin1, HIGH);
  digitalWrite(motor2Pin2, LOW);
  
  // Motor 3 - Frente
  digitalWrite(motor3Pin1, HIGH);
  digitalWrite(motor3Pin2, LOW);
  
  // Motor 4 - Frente
  digitalWrite(motor4Pin1, HIGH);
  digitalWrite(motor4Pin2, LOW);
  
  Serial.println("Movendo para frente");
}

void moverParaTras() {
  // Motor 1 - Trás
  analogWrite(motor1Pin1, 0);
  analogWrite(motor1Pin2, dutyCycle);

  analogWrite(motor2Pin1, 0);
  analogWrite(motor2Pin2, dutyCycle);

  analogWrite(motor3Pin1, 0);
  analogWrite(motor3Pin2, dutyCycle);

  analogWrite(motor4Pin1, 0);
  analogWrite(motor4Pin2, dutyCycle);

  Serial.println("Movendo para trás");
}

void virarEsquerda() {
  // Motores do lado direito - Frente
  digitalWrite(motor2Pin1, HIGH);
  digitalWrite(motor2Pin2, LOW);
  digitalWrite(motor4Pin1, HIGH);
  digitalWrite(motor4Pin2, LOW);
  
  // Motores do lado esquerdo - Trás
  digitalWrite(motor1Pin1, LOW);
  digitalWrite(motor1Pin2, HIGH);
  digitalWrite(motor3Pin1, LOW);
  digitalWrite(motor3Pin2, HIGH);
  
  Serial.println("Virando à esquerda");
}

void virarDireita() {
  // Motores do lado esquerdo - Frente
  digitalWrite(motor1Pin1, HIGH);
  digitalWrite(motor1Pin2, LOW);
  digitalWrite(motor3Pin1, HIGH);
  digitalWrite(motor3Pin2, LOW);
  
  // Motores do lado direito - Trás
  digitalWrite(motor2Pin1, LOW);
  digitalWrite(motor2Pin2, HIGH);
  digitalWrite(motor4Pin1, LOW);
  digitalWrite(motor4Pin2, HIGH);
  
  Serial.println("Virando à direita");
}
//  pinMode(motor1Pin1, OUTPUT);
void parar() {
  // Desligar todos os motores
  dutyCycle=255;
  analogWrite(motor1Pin1, dutyCycle);
  // digitalWrite(motor1Pin1, LOW);
  digitalWrite(motor1Pin2, LOW);
  digitalWrite(motor2Pin1, LOW);
  digitalWrite(motor2Pin2, LOW);
  digitalWrite(motor3Pin1, LOW);
  digitalWrite(motor3Pin2, LOW);
  digitalWrite(motor4Pin1, LOW);
  digitalWrite(motor4Pin2, LOW);
  
  Serial.println("Parando");
}

// Função chamada ao acessar "/cmd"
void handleCommand() {
  String dir = server.arg("dir");

  Serial.print("Comando recebido: ");
  Serial.println(dir);

  piscarLED();
  // Controlar os motores com base no comando
 if (dir == "forward" || dir == "backward" || dir == "left" || dir == "right") {
    movimentoAtual = dir;
  } else if (dir == "stop") {
    movimentoAtual = "parado";
    parar();
  }
  server.send(200, "text/plain", "OK");
}
void piscarLED() {
  digitalWrite(ledPin, HIGH);
  delay(100);
  digitalWrite(ledPin, LOW);
}
void setup() {
  Serial.begin(115200);
  
  // Configurar pinos dos motores como saída
  pinMode(motor1Pin1, OUTPUT);
  pinMode(motor1Pin2, OUTPUT);
  pinMode(motor2Pin1, OUTPUT);
  pinMode(motor2Pin2, OUTPUT);
  pinMode(motor3Pin1, OUTPUT);
  pinMode(motor3Pin2, OUTPUT);
  pinMode(motor4Pin1, OUTPUT);
  pinMode(motor4Pin2, OUTPUT);
  pinMode(ledPin, OUTPUT);
  
  // Inicialmente, todos os motores desligados
  parar();

  // Iniciar Access Point
  WiFi.softAP(ssid, password);
  Serial.println("Access Point iniciado");
  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());

  // Roteamento
  server.on("/", []() {
    server.send_P(200, "text/html", htmlPage);
  });
  server.on("/cmd", handleCommand);
  server.begin();
  Serial.println("Servidor iniciado");
}

void loop() {
  server.handleClient();
   if (movimentoAtual == "forward") {
    moverParaFrente();
  } else if (movimentoAtual == "backward") {
    moverParaTras();
  } else if (movimentoAtual == "left") {
    virarEsquerda();
  } else if (movimentoAtual == "right") {
    virarDireita();
  }
  delay(50); // Pequeno delay para não sobrecarregar o processador
}