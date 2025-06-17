#include <Arduino.h>
#include <math.h> // Para sqrt() e acos()

#define MIC1_PIN A0
#define MIC2_PIN A1

const int ledPins[8] = {2, 3, 4, 5, 6, 7, 8, 9};
const int numSamples = 100;
int mic1[numSamples];
int mic2[numSamples];

const float micDistance = 0.2;
const float soundSpeed = 343.0;
const int sampleRate = 5000;
const float dt = 1.0 / sampleRate;

// Ajuste de sensibilidade
const float gainMic1 = 1.0;
const float gainMic2 = 0.9;

// 👇 Protótipos das funções usadas no loop()
int getLEDIndex(float angle);
void acendeLED(int index);

void setup() {
  for (int i = 0; i < 8; i++) {
    pinMode(ledPins[i], OUTPUT);
    digitalWrite(ledPins[i], LOW);
  }
  Serial.begin(115200);
}
// Acende apenas o LED do setor indicado
void acendeLED(int index) {
  for (int i = 0; i < 8; i++) {
    digitalWrite(ledPins[i], (i == index) ? HIGH : LOW);
  }
}
void loop() {
  // 1. Captura de amostras
  for (int i = 0; i < numSamples; i++) {
    mic1[i] = analogRead(MIC1_PIN);
    mic2[i] = analogRead(MIC2_PIN);
    delayMicroseconds(200); // 5kHz
  }

  // 2. Remoção de offset DC (normalização)
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

  // 3. Verificação de amplitude mínima (filtro de ruído)
  long totalAmp = 0;
  for (int i = 0; i < numSamples; i++) {
    totalAmp += abs(mic1[i]) + abs(mic2[i]);
  }
  int avgAmp = totalAmp / (2 * numSamples);
  if (avgAmp < 10) return; // Ignora se o som for muito fraco

  // 4. Correlação cruzada para achar defasagem
  int maxLag = 10;
  int bestLag = 0;
  long bestCorr = 0;

  for (int lag = -maxLag; lag <= maxLag; lag++) {
    long corr = 0;
    for (int i = 0; i < numSamples - abs(lag); i++) {
      int j = i + lag;
      if (j < 0 || j >= numSamples) continue;
      corr += (long)mic1[i] * (long)mic2[j];
    }

    if (abs(corr) > abs(bestCorr)) {
      bestCorr = corr;
      bestLag = lag;
    }
  }

  // 5. Cálculo do ângulo estimado
  float deltaT = bestLag * dt;
  float ratio = (soundSpeed * deltaT) / micDistance;
  ratio = constrain(ratio, -1.0, 1.0); // Limita para evitar erros no asin()
  float angle = asin(ratio) * 180.0 / PI;

  // 6. Margem de erro de 10 graus (considera "frente")
  if (abs(angle) < 10.0) angle = 0.0;

  // 7. Determina qual LED acender com base no ângulo
  int ledIndex = getLEDIndex(angle);

  // 8. Acende o LED correspondente
  acendeLED(ledIndex);

  // 9. Debug serial
  Serial.print("Ângulo estimado: ");
  Serial.print(angle);
  Serial.print("°, LED: ");
  Serial.println(ledIndex);

  delay(100); // Pequena pausa
}

// Mapeia o ângulo estimado para um dos 8 LEDs (cada 45°)
int getLEDIndex(float angle) {
  if (angle >= -22.5 && angle < 22.5) return 0;     // 0°
  if (angle >= 22.5 && angle < 67.5) return 1;      // 45°
  if (angle >= 67.5 && angle < 112.5) return 2;     // 90°
  if (angle >= 112.5 && angle < 157.5) return 3;    // 135°
  if (angle >= 157.5 || angle < -157.5) return 4;   // 180° ou -180°
  if (angle >= -157.5 && angle < -112.5) return 5;  // -135°
  if (angle >= -112.5 && angle < -67.5) return 6;   // -90°
  if (angle >= -67.5 && angle < -22.5) return 7;    // -45°
  return 0; // Fallback (caso extremo)
}

