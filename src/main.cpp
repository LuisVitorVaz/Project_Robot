#include <Arduino.h>
#include <arduinoFFT.h>
#include <math.h>

// Constantes físicas
const double SOUND_SPEED = 343.0; // m/s
const double MIC_DISTANCE = 0.2;  // metros (20 cm)

// Parâmetros FFT
#define FFT_SIZE 128
const double SAMPLING_FREQ = 5000.0;  // Hz

// Pinos dos sensores
#define SENSOR1 A0
#define SENSOR2 A1

// LEDs
#define LED_FRENTE 10      // 0°
#define LED_45_DIR_FRENTE 2  // 45°
#define LED_DIREITA 9       // 90°
#define LED_45_DIR_TRAS 3   // 135°
#define LED_ATRAS 11        // 180°
#define LED_45_ESQ_TRAS 4   // 225°
#define LED_ESQUERDA 8      // 270°
#define LED_45_ESQ_FRENTE 5 // 315°

arduinoFFT FFT1 = arduinoFFT();
arduinoFFT FFT2 = arduinoFFT();

double vReal1[FFT_SIZE];
double vImag1[FFT_SIZE];

double vReal2[FFT_SIZE];
double vImag2[FFT_SIZE];

bool buffer_pronto = false;

void apaga_leds() {
  digitalWrite(LED_ESQUERDA, LOW);
  digitalWrite(LED_DIREITA, LOW);
  digitalWrite(LED_FRENTE, LOW);
  digitalWrite(LED_ATRAS, LOW);
  digitalWrite(LED_45_ESQ_FRENTE, LOW);
  digitalWrite(LED_45_DIR_FRENTE, LOW);
  digitalWrite(LED_45_ESQ_TRAS, LOW);
  digitalWrite(LED_45_DIR_TRAS, LOW);
}

void acende_led_theta(float theta) {
  if (theta < 0) theta += 360;

  Serial.print("Direção estimada: ");
  Serial.print(theta);
  Serial.println(" graus");

  apaga_leds();

  if (theta >= 337.5 || theta < 22.5)
    digitalWrite(LED_FRENTE, HIGH);
  else if (theta >= 22.5 && theta < 67.5)
    digitalWrite(LED_45_DIR_FRENTE, HIGH);
  else if (theta >= 67.5 && theta < 112.5)
    digitalWrite(LED_DIREITA, HIGH);
  else if (theta >= 112.5 && theta < 157.5)
    digitalWrite(LED_45_DIR_TRAS, HIGH);
  else if (theta >= 157.5 && theta < 202.5)
    digitalWrite(LED_ATRAS, HIGH);
  else if (theta >= 202.5 && theta < 247.5)
    digitalWrite(LED_45_ESQ_TRAS, HIGH);
  else if (theta >= 247.5 && theta < 292.5)
    digitalWrite(LED_ESQUERDA, HIGH);
  else if (theta >= 292.5 && theta < 337.5)
    digitalWrite(LED_45_ESQ_FRENTE, HIGH);
}

void coleta_amostras() {
  for (int i = 0; i < FFT_SIZE; i++) {
    vReal1[i] = analogRead(SENSOR1);
    vImag1[i] = 0;

    vReal2[i] = analogRead(SENSOR2);
    vImag2[i] = 0;

    delayMicroseconds((1.0 / SAMPLING_FREQ) * 1e6);
  }
  buffer_pronto = true;
}

void calcula_direcao() {
  // FFT dos dois sinais
  FFT1.Windowing(vReal1, FFT_SIZE, FFT_WIN_TYP_HAMMING, FFT_FORWARD);

  FFT1.Compute(vReal1, vImag1, FFT_SIZE, FFT_FORWARD);

  FFT2.Windowing(vReal2, FFT_SIZE, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT2.Compute(vReal2, vImag2, FFT_SIZE, FFT_FORWARD);

  // Encontrar frequência dominante (maior pico no sinal 1)
  int peakIndex = 1;
  double maxMag = 0;
  for (int i = 1; i < FFT_SIZE / 2; i++) {
    double mag = sqrt(vReal1[i] * vReal1[i] + vImag1[i] * vImag1[i]);
    if (mag > maxMag) {
      maxMag = mag;
      peakIndex = i;
    }
  }

  double freqDominante = (peakIndex * SAMPLING_FREQ) / FFT_SIZE;
  double lambda = SOUND_SPEED / freqDominante;

  Serial.print("Frequência dominante: ");
  Serial.print(freqDominante);
  Serial.println(" Hz");

  // Fases
  double fase1 = atan2(vImag1[peakIndex], vReal1[peakIndex]);
  double fase2 = atan2(vImag2[peakIndex], vReal2[peakIndex]);
  double deltaPhase = fase2 - fase1;

  if (deltaPhase > PI) deltaPhase -= 2 * PI;
  if (deltaPhase < -PI) deltaPhase += 2 * PI;

  // Cálculo angular mais preciso
  double numerador = deltaPhase * lambda;
  double denominador = 2.0 * PI * MIC_DISTANCE;
  double seno_theta = numerador / denominador;

  // Limita seno para faixa válida
  seno_theta = constrain(seno_theta, -1.0, 1.0);

  float theta_rad = asin(seno_theta);
  float theta_deg = theta_rad * 180.0 / PI;

  // Mapeamento para frente (0°) até 360°
  if (theta_deg < 0)
    theta_deg = 360.0 + theta_deg;

  acende_led_theta(theta_deg);
  buffer_pronto = false;
}

void setup() {
  Serial.begin(9600);

  pinMode(LED_ESQUERDA, OUTPUT);
  pinMode(LED_DIREITA, OUTPUT);
  pinMode(LED_FRENTE, OUTPUT);
  pinMode(LED_ATRAS, OUTPUT);
  pinMode(LED_45_ESQ_FRENTE, OUTPUT);
  pinMode(LED_45_DIR_FRENTE, OUTPUT);
  pinMode(LED_45_ESQ_TRAS, OUTPUT);
  pinMode(LED_45_DIR_TRAS, OUTPUT);

  apaga_leds();
}

void loop() {
  if (!buffer_pronto) {
    coleta_amostras();  // Preenche os vetores
  } else {
    calcula_direcao();  // Usa os dados e acende o LED
    delay(400);         // Tempo para visualizar o LED antes de nova coleta
  }
}
