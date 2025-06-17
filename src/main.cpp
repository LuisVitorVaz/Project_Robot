#include <Arduino.h>
#include <math.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --------------------- OLED CONFIG ---------------------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// --------------------- MIC CONFIG ---------------------
#define MIC1_PIN A0
#define MIC2_PIN A1
const int numSamples = 100;
int mic1[numSamples];
int mic2[numSamples];

const float micDistance = 0.2;      // 20 cm
const float soundSpeed = 343.0;     // m/s
const int sampleRate = 5000;        // 5kHz
const float dt = 1.0 / sampleRate;

const float gainMic1 = 1.0;   // ajuste de diferenca de sensibilidade
const float gainMic2 = 0.787;
   // ajuste de sensibilidade

// --------------------- LED CONFIG ---------------------
#define LED_45_ESQ_FRENTE 5
#define LED_45_DIR_FRENTE 2
#define LED_45_ESQ_TRAS 4
#define LED_45_DIR_TRAS 3

#define LED_ESQUERDA 8
#define LED_DIREITA 9
#define LED_FRENTE 10
#define LED_ATRAS 11

// Desliga todos os LEDs
void apagarTodosLEDs() {
  digitalWrite(LED_45_ESQ_FRENTE, LOW);
  digitalWrite(LED_45_DIR_FRENTE, LOW);
  digitalWrite(LED_45_ESQ_TRAS, LOW);
  digitalWrite(LED_45_DIR_TRAS, LOW);
  digitalWrite(LED_ESQUERDA, LOW);
  digitalWrite(LED_DIREITA, LOW);
  digitalWrite(LED_FRENTE, LOW);
  digitalWrite(LED_ATRAS, LOW);
}

// Acende o LED de acordo com o ângulo
void acendeLEDPorAngulo(float theta) {
  apagarTodosLEDs();

  if (theta < 0) theta += 360;

  if (theta < 22.5 || theta >= 337.5) {
    digitalWrite(LED_FRENTE, HIGH);
  } else if (theta < 67.5) {
    digitalWrite(LED_45_ESQ_FRENTE, HIGH);
  } else if (theta < 112.5) {
    digitalWrite(LED_ESQUERDA, HIGH);
  } else if (theta < 157.5) {
    digitalWrite(LED_45_ESQ_TRAS, HIGH);
  } else if (theta < 202.5) {
    digitalWrite(LED_ATRAS, HIGH);
  } else if (theta < 247.5) {
    digitalWrite(LED_45_DIR_TRAS, HIGH);
  } else if (theta < 292.5) {
    digitalWrite(LED_DIREITA, HIGH);
  } else {
    digitalWrite(LED_45_DIR_FRENTE, HIGH);
  }
}

// Mostra a direção no OLED
void mostrarDirecaoOLED(float angle) {
  String direcao;
  if (angle < 0) angle += 360;

  if (angle < 22.5 || angle >= 337.5) {
    direcao = "FRENTE";
  } else if (angle < 67.5) {
    direcao = "45 ESQ FRENTE";
  } else if (angle < 112.5) {
    direcao = "ESQUERDA";
  } else if (angle < 157.5) {
    direcao = "45 ESQ TRAS";
  } else if (angle < 202.5) {
    direcao = "TRAS";
  } else if (angle < 247.5) {
    direcao = "45 DIR TRAS";
  } else if (angle < 292.5) {
    direcao = "DIREITA";
  } else {
    direcao = "45 DIR FRENTE";
  }

  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.println("Direcao do Som:");
  display.setCursor(0, 20);
  display.setTextSize(2);
  display.println(direcao);
  display.display();
}

void setup() {
  // Inicializa LEDs
  pinMode(LED_45_ESQ_FRENTE, OUTPUT);
  pinMode(LED_45_DIR_FRENTE, OUTPUT);
  pinMode(LED_45_ESQ_TRAS, OUTPUT);
  pinMode(LED_45_DIR_TRAS, OUTPUT);
  pinMode(LED_ESQUERDA, OUTPUT);
  pinMode(LED_DIREITA, OUTPUT);
  pinMode(LED_FRENTE, OUTPUT);
  pinMode(LED_ATRAS, OUTPUT);
  apagarTodosLEDs();

  Serial.begin(115200);

  // Inicializa display OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("OLED não encontrado"));
    while (true); // Travar aqui se falhar
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Sistema Iniciado...");
  display.display();
  delay(1000);
}

void loop() {
  // 1. Captura dos sinais
  for (int i = 0; i < numSamples; i++) {
    mic1[i] = analogRead(MIC1_PIN);
    mic2[i] = analogRead(MIC2_PIN);
    delayMicroseconds(200); // 5kHz
  }

  // 2. Remoção de offset
  long sum1 = 0, sum2 = 0;
  for (int i = 0; i < numSamples; i++) {
    sum1 += mic1[i];
    sum2 += mic2[i];
  }
  int mean1 = sum1 / numSamples;
  int mean2 = sum2 / numSamples;

  for (int i = 0; i < numSamples; i++) {
    mic1[i] = (mic1[i] - mean1) * gainMic1;
    mic2[i] = (mic2[i] - mean2) * gainMic2;
  }

  // 3. Filtro de amplitude
  long totalAmp = 0;
  for (int i = 0; i < numSamples; i++) {
    totalAmp += abs(mic1[i]) + abs(mic2[i]);
  }
  int avgAmp = totalAmp / (2 * numSamples);
  if (avgAmp < 10) return; // Som muito fraco

  // 4. Correlação cruzada
 int maxLag = 20;        // máximo deslocamento para tentar (em amostras)
int bestLag = 0;        // melhor deslocamento encontrado
long bestCorr = 0;      // maior correlação encontrada

// Para cada possível deslocamento (lag) entre -maxLag e +maxLag
for (int lag = -maxLag; lag <= maxLag; lag++) {
    long corr = 0;      // acumula a correlação para este lag

    // Percorre as amostras, alinhando mic1 e mic2 com deslocamento lag
    for (int i = 0; i < numSamples - abs(lag); i++) {
        int j = i + lag;    // índice deslocado no mic2

        if (j < 0 || j >= numSamples) 
            continue;       // evita índices inválidos

        // soma o produto das amostras alinhadas
        corr += (long)mic1[i] * (long)mic2[j];
    }

    // Verifica se a correlação atual é maior que a melhor encontrada
    if (abs(corr) > abs(bestCorr)) {
        bestCorr = corr;
        bestLag = lag;     // salva o lag que gerou a melhor correlação
    }
}
  // 5. Cálculo do ângulo
  float deltaT = bestLag * dt;
  float ratio = (soundSpeed * deltaT) / micDistance;
  ratio = constrain(ratio, -1.0, 1.0);
  float angle = asin(ratio) * 180.0 / PI;

  if (abs(angle) < 10.0) angle = 0.0;

  // 6. Atualiza LED e Display
  acendeLEDPorAngulo(angle);
  mostrarDirecaoOLED(angle);

  // 7. Debug serial
  Serial.print("Ângulo estimado: ");
  Serial.print(angle);
  Serial.println(" graus");

  delay(100);
}
