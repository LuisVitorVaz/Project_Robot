#include <Arduino.h>
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

// 12,13,14,27 mover para frente
// 25,26,32,33 mover para traz

// 27,14 lado direito frente
// 26,25 lado direito traz
#define motor1Pin1 12       // Pino de saída PWM
#define motor1Pin2 13       // Pino de saída PWM
#define motor2Pin1 14       // Pino de saída PWM
#define motor2Pin2 27       // Pino de saída PWM
#define motor3Pin1 32       // Pino de saída PWM
#define motor3Pin2 33       // Pino de saída PWM
#define motor4Pin1 25       // Pino de saída PWM
#define motor4Pin2 26       // Pino de saída PWM

#define canalPWM1 0           // Canal PWM de 0 a 15
#define canalPWM2 1  
#define canalPWM3 2  
#define canalPWM4 3  
#define canalPWM5 4  
#define canalPWM6 5  
#define canalPWM7 6  
#define canalPWM8 7  
#define frequenciaM1 300      // Frequência em Hz
#define frequenciaM2 300
#define frequenciaM3 300
#define frequenciaM4 300
#define resolucao 8          // Resolução de 8 bits (0 a 255)

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
void piscarLED() {
  digitalWrite(ledPin, HIGH);
  delay(100);
  digitalWrite(ledPin, LOW);
}
void parar() {
  // Desligar todos os motores
  ledcWrite(0, 0);  // 0% duty no motor1
  ledcWrite(1, 0);  // 0% duty no motor1
  ledcWrite(2, 0);  // 0% duty no motor1
  ledcWrite(3, 0);  // 0% duty no motor1
  ledcWrite(4, 0);  // 100% duty no motor1
  ledcWrite(5, 0);  // 100% duty no motor1
  ledcWrite(6, 0);  // 100% duty no motor1
  ledcWrite(7, 0);  // 100% duty no motor1
  
  Serial.println("Parando");
}

// Controle dos motores
void moverParaFrente() {
    piscarLED();
  unsigned long tempoInicial = millis();
  while (millis() - tempoInicial < 10)
  {
  ledcWrite(0, 0);  // 0% duty no motor1
  ledcWrite(1, 0);  // 0% duty no motor1
  ledcWrite(2, 0);  // 0% duty no motor1
  ledcWrite(3, 0);  // 0% duty no motor1
  ledcWrite(4, 256);  // 100% duty no motor1
  ledcWrite(5, 256);  // 100% duty no motor1
  ledcWrite(6, 256);  // 100% duty no motor1
  ledcWrite(7, 256);  // 100% duty no motor1
  }
  ledcWrite(0, 0);  // 0% duty no motor1
  ledcWrite(1, 0);  // 0% duty no motor1
  ledcWrite(2, 0);  // 0% duty no motor1
  ledcWrite(3, 0);  // 0% duty no motor1
  ledcWrite(4, 200);  // 100% duty no motor1
  ledcWrite(5, 200);  // 100% duty no motor1
  ledcWrite(6, 200);  // 100% duty no motor1
  ledcWrite(7, 200);  // 100% duty no motor1
  Serial.println("Movendo para frente");
}

void moverParaTras() {
    piscarLED();
  unsigned long tempoInicial = millis();
  while (millis() - tempoInicial < 10) 
  {
      ledcWrite(0, 256);  // 0% duty no motor1
      ledcWrite(1, 256);  // 0% duty no motor1
      ledcWrite(2, 256);  // 0% duty no motor1
      ledcWrite(3, 256);  // 0% duty no motor1
      ledcWrite(4, 0);  // 100% duty no motor1
      ledcWrite(5, 0);  // 100% duty no motor1
      ledcWrite(6, 0);  // 100% duty no motor1
      ledcWrite(7, 0);  // 100% duty no motor1
  }
      ledcWrite(0, 200);  // 0% duty no motor1
      ledcWrite(1, 200);  // 0% duty no motor1
      ledcWrite(2, 200);  // 0% duty no motor1
      ledcWrite(3, 200);  // 0% duty no motor1
      ledcWrite(4, 0);  // 100% duty no motor1
      ledcWrite(5, 0);  // 100% duty no motor1
      ledcWrite(6, 0);  // 100% duty no motor1
      ledcWrite(7, 0);  // 100% duty no motor1

  Serial.println("Movendo para trás");
}

void virarEsquerda() {
  // parar();
  piscarLED();
  Serial.println("Virando para a esquerda");

  // unsigned long tempoInicial = millis();
  // while (millis() - tempoInicial < 10000) {
    // Motores da esquerda para frente
    ledcWrite(0, 0);
    ledcWrite(1, 0);
    ledcWrite(4, 200);
    ledcWrite(5, 200);
    
    // Motores da direita para trás
    ledcWrite(2, 200);
    ledcWrite(3, 200);
    ledcWrite(6, 0);
    ledcWrite(7, 0);
  }
// }

void virarDireita() {
  // parar();
  piscarLED();
  Serial.println("Virando para a direita");

  // unsigned long tempoInicial = millis();
  // while (millis() - tempoInicial < 10000) {
  //   // Motores da direita para frente
    ledcWrite(2, 0);
    ledcWrite(3, 0);
    ledcWrite(6, 200);
    ledcWrite(7, 200);

    // Motores da esquerda para trás
    ledcWrite(0, 200);
    ledcWrite(1, 200);
    ledcWrite(4, 0);
    ledcWrite(5, 0);
  // }
  // parar();
  
  Serial.println("Virando à direita");
}

// Função chamada ao acessar "/cmd"
void handleCommand() {
  String dir = server.arg("dir");

  Serial.print("Comando recebido: ");
  Serial.println(dir);

  // piscarLED();
  // Controlar os motores com base no comando
 if (dir == "forward" || dir == "backward" || dir == "left" || dir == "right") {
    movimentoAtual = dir;
  } else if (dir == "stop") {
    movimentoAtual = "parado";
    parar();
  }
  server.send(200, "text/plain", "OK");
}
void setup() {
  Serial.begin(115200);
  
  // Configurar pinos dos motores como saída
  ledcSetup(canalPWM1, frequenciaM1, resolucao);
  ledcAttachPin(motor1Pin1, canalPWM1);

   ledcSetup(canalPWM2, frequenciaM1, resolucao);
  ledcAttachPin(motor1Pin2, canalPWM2);

   ledcSetup(canalPWM3, frequenciaM2, resolucao);
  ledcAttachPin(motor2Pin1, canalPWM3);

   ledcSetup(canalPWM4, frequenciaM2, resolucao);
  ledcAttachPin(motor2Pin2, canalPWM4);

   ledcSetup(canalPWM5, frequenciaM3, resolucao);
  ledcAttachPin(motor3Pin1, canalPWM5);

   ledcSetup(canalPWM6, frequenciaM3, resolucao);
  ledcAttachPin(motor3Pin2, canalPWM6);

   ledcSetup(canalPWM7, frequenciaM4, resolucao);
  ledcAttachPin(motor4Pin1, canalPWM7);

   ledcSetup(canalPWM8, frequenciaM4, resolucao);
  ledcAttachPin(motor4Pin2, canalPWM8);
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