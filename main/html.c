
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
