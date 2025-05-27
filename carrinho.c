#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "ESP32-Car";
const char* password = "12345678";

WebServer server(80);

// Página HTML com botões de controle usando imagens Base64
const char htmlPage[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Controle do Carrinho</title>
  <style>
    body { text-align: center; font-family: Arial; }
    .button {
      width: 64px;
      height: 64px;
      background-size: cover;
      margin: 10px;
      display: inline-block;
      cursor: pointer;
    }
  </style>
</head>
<body>
  <h2>Controle do Carrinho</h2>
  <div onclick="sendCommand('forward')" class="button" style="background-image: url('data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAEAAAAA...');"></div><br>
  <div onclick="sendCommand('left')" class="button" style="background-image: url('data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAEAAAAA...');"></div>
  <div onclick="sendCommand('stop')" class="button" style="background-color: #888; color: white; line-height: 64px;">⏹</div>
  <div onclick="sendCommand('right')" class="button" style="background-image: url('data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAEAAAAA...');"></div><br>
  <div onclick="sendCommand('backward')" class="button" style="background-image: url('data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAEAAAAA...');"></div>

  <script>
    function sendCommand(cmd) {
      fetch("/cmd?dir=" + cmd);
    }
  </script>
</body>
</html>
)rawliteral";

// Função chamada ao acessar "/cmd"
void handleCommand() {
  String dir = server.arg("dir");

  Serial.print("Comando recebido: ");
  Serial.println(dir);

  // Aqui você controla os motores com base no comando
  if (dir == "forward") {
    // GPIOs para frente
  } else if (dir == "backward") {
    // GPIOs para trás
  } else if (dir == "left") {
    // virar à esquerda
  } else if (dir == "right") {
    // virar à direita
  } else if (dir == "stop") {
    // parar
  }

  server.send(200, "text/plain", "OK");
}

void setup() {
  Serial.begin(115200);

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
}
