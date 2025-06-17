#include <Arduino.h>
#include <math.h> // Para sqrt() e acos()

#define SENSOR1 A0
#define SENSOR2 A1

// LEDs de 45 graus
#define LED_45_ESQ_FRENTE 5
#define LED_45_DIR_FRENTE 2
#define LED_45_ESQ_TRAS 4
#define LED_45_DIR_TRAS 3

// LEDs principais
#define LED_ESQUERDA 8
#define LED_DIREITA 9
#define LED_FRENTE 10
#define LED_ATRAS 11
int valor1 = 0;
int valor2 = 0;
float theta = 0;
double media_valores1[20] = {0};
double media_valores2[20] = {0};
int cont = 0;

double max1 = 0;
double max2 = 0;

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

void detectar_picos() {
  // Detecta pico local baseado em mudança de sinal na derivada
  max1 = media_valores1[0];
  max2 = media_valores2[0];

  for (int i = 1; i < 19; i++) {
    double d1 = media_valores1[i] - media_valores1[i - 1];
    double d2 = media_valores1[i + 1] - media_valores1[i];
    if (d1 > 0 && d2 < 0 && media_valores1[i] > max1) {
      max1 = media_valores1[i];
    }

    d1 = media_valores2[i] - media_valores2[i - 1];
    d2 = media_valores2[i + 1] - media_valores2[i];
    if (d1 > 0 && d2 < 0 && media_valores2[i] > max2) {
      max2 = media_valores2[i];
    }
  }

  Serial.print("Pico detectado Sensor 1: ");
  delay(100);
  Serial.println(max1);
  Serial.print("Pico detectado Sensor 2: ");
  delay(100);
  Serial.println(max2);
}

void calcular_theta_e_acender_led() {
  apaga_leds();

  if (max1 > 0 && max2 > 0) {
    theta = atan2(max2, max1);  // agora theta considera os 2 quadrantes
    if (theta < 0) theta += 2 * PI;

    Serial.print("Theta (radianos): ");
    Serial.println(theta);

    // Mapeamento com base no círculo
    if (theta < PI / 8 || theta > 15 * PI / 8) {
      digitalWrite(LED_FRENTE, HIGH);
    } else if (theta < 3 * PI / 8) {
      digitalWrite(LED_45_DIR_FRENTE, HIGH);
    } else if (theta < 5 * PI / 8) {
      digitalWrite(LED_DIREITA, HIGH);
    } else if (theta < 7 * PI / 8) {
      digitalWrite(LED_45_DIR_TRAS, HIGH);
    } else if (theta < 9 * PI / 8) {
      digitalWrite(LED_ATRAS, HIGH);
    } else if (theta < 11 * PI / 8) {
      digitalWrite(LED_45_ESQ_TRAS, HIGH);
    } else if (theta < 13 * PI / 8) {
      digitalWrite(LED_ESQUERDA, HIGH);
    } else {
      digitalWrite(LED_45_ESQ_FRENTE, HIGH);
    }
  } else {
    Serial.println("Erro: valores inválidos para cálculo de theta");
  }
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
}

void loop() {
   valor1 = analogRead(SENSOR1);
   valor2 = analogRead(SENSOR2);

  media_valores1[cont] = valor1;
  media_valores2[cont] = valor2;
  cont++;
  Serial.println("vslor do cont");
  Serial.println(cont);

  if (cont == 20) {
    detectar_picos();
    calcular_theta_e_acender_led();

    cont = 0;
    max1 = 0;
    max2 = 0;
  }

  delay(2); // Pequeno delay para taxa de aquisição estável
}
