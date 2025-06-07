#include <Arduino.h>
#include <math.h> // Necessário para usar sqrt() e acos()

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

float theta = 0;
double media_valores1[10] = {0};
double media_valores2[10] = {0};
int valor1 = 0;
int valor2 = 0;
int cont = 0;
double max1 = 0;
double max2 = 0;

void funcao_media() {
  if (cont == 10) {
    // Reset dos máximos
    max1 = media_valores1[0];
    max2 = media_valores2[0];

    for (int i = 1; i < 10; i++) {
      if (media_valores1[i] > max1) {
        max1 = media_valores1[i];
      }
      if (media_valores2[i] > max2) {
        max2 = media_valores2[i];
      }

      Serial.print("Vetor1[");
      Serial.print(i);
      Serial.print("] = ");
      Serial.println(media_valores1[i]);
    }

    Serial.print("Maior valor vetor1: ");
    Serial.println(max1);
    Serial.print("Maior valor vetor2: ");
    Serial.println(max2);

    cont = 0; // reinicia contador
  }
}

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

void angulo_theta() {
  apaga_leds();

  if (max1 > 0 && max2 > 0) {
    double hipotenusa = sqrt(pow(max1, 2) + pow(max2, 2));
    theta = acos(max1 / hipotenusa); // theta em radianos
    Serial.print("Theta (radianos): ");
    Serial.println(theta);

    // Mapeamento dos ângulos para os LEDs
    if (theta < 0.4) {
      digitalWrite(LED_45_ESQ_TRAS, HIGH);
    } else if (theta < 0.6) {
      digitalWrite(LED_ESQUERDA, HIGH);
    } else if (theta < 0.8) {
      digitalWrite(LED_45_ESQ_FRENTE, HIGH);
    } else if (theta < 1.2) {
      digitalWrite(LED_FRENTE, HIGH);
    } else if (theta < 1.4) {
      digitalWrite(LED_45_DIR_FRENTE, HIGH);
    } else if (theta < 1.6) {
      digitalWrite(LED_DIREITA, HIGH);
    } else if (theta < 1.8) {
      digitalWrite(LED_45_DIR_TRAS, HIGH);
    } else {
      digitalWrite(LED_ATRAS, HIGH);
    }

  } else {
    Serial.println("Erro: valores inválidos para cálculo de theta");
  }

  delay(500); // Aguarda meio segundo
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

  Serial.print("Sensor 1: ");
  Serial.println(valor1);
  Serial.print("Sensor 2: ");
  Serial.println(valor2);

  media_valores1[cont] = valor1;
  media_valores2[cont] = valor2;
  cont++;

  Serial.print("Contador: ");
  Serial.println(cont);

  funcao_media();
  angulo_theta();
}
