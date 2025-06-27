#include <arduinoFFT.h>
#include <math.h>

// ------- MIC CONFIG ---------------------
const int MIC1_PIN = A0;
const int MIC2_PIN = A1;
float mic1[20];
float mic2[100];
float maior_mic1 = 0.0;
float maior_mic2 = 0.0;
float vetor1_norm[20];
float vetor2_norm[100];
float correlacao_resultado[81];


// LEDs
#define LED_FRENTE 10
#define LED_45_DIR_FRENTE 2
#define LED_DIREITA 9
#define LED_45_DIR_TRAS 3
#define LED_ATRAS 11
#define LED_45_ESQ_TRAS 4
#define LED_ESQUERDA 8
#define LED_45_ESQ_FRENTE 5

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

void coleta_amostras() {
  for (int i = 0; i < 100; i++) {
    if (i < 20) {
      mic1[i] = (float)analogRead(MIC1_PIN);
    }
    mic2[i] = (float)analogRead(MIC2_PIN);
  }
  Serial.println("Fim da coleta de amostras.");
}

void imprimir_amostras() {
  Serial.println("Amostras coletadas:");
  for (int i = 0; i < 100; i++) {
    if (i < 20) {
      Serial.print("mic1["); Serial.print(i); Serial.print("] = ");
      Serial.print(mic1[i]);
      Serial.print(" | ");
    }
    Serial.print("mic2["); Serial.print(i); Serial.print("] = ");
    Serial.println(mic2[i]);
  }
}

void encontrar_maior(const float *vetor, int tamanho, float *maior_destino) {
  *maior_destino = vetor[0];
  for (int i = 1; i < tamanho; i++) {
    if (vetor[i] > *maior_destino) {
      *maior_destino = vetor[i];
    }
  }
}

void normalizar_vetor(const float *vetor, float *vetor_norm, int tamanho, float normalizar) {
  for (int i = 0; i < tamanho; i++) {
    vetor_norm[i] = vetor[i] / normalizar;
  }
}

void imprimir_vetor_normalizado(const float *vetor, int tamanho, const char* nome) {
  Serial.print("Vetor "); Serial.print(nome); Serial.println(" normalizado:");
  for (int i = 0; i < tamanho; i++) {
    Serial.print(vetor[i], 3);
    Serial.print(" ");
    if ((i + 1) % 10 == 0) Serial.println();
  }
  Serial.println();
}
// void correlacao_cruzada(const float *mic1, const float *mic2, float *resultado) {
//   for (int k = 0; k < 20; k++) {
//     float soma = 0.0;
//     for (int n = 0; n < 20; n++) {
//       soma += vetor1_norm[n] * vetor2_norm[n + k];  // mic2 deslocado em k
//     }
//     resultado[k] = soma/20.0;
//   }
// }
void correlacao_cruzada(const float *mic1, const float *mic2, float *resultado) {
  for (int k = 0; k <= 80; k++) { // 100 - 20 = 80 deslocamentos possíveis
    float soma = 0.0;
    for (int n = 0; n < 20; n++) {
      soma += mic1[n] * mic2[n + k];
    }
    resultado[k] = soma / 20.0; // normaliza pela quantidade de amostras
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

  coleta_amostras();
  imprimir_amostras();

  encontrar_maior(mic1, 20, &maior_mic1);
  encontrar_maior(mic2, 100, &maior_mic2);

  Serial.print("Maior valor de mic1: ");
  Serial.println(maior_mic1);
  Serial.print("Maior valor de mic2: ");
  Serial.println(maior_mic2);

  normalizar_vetor(mic1, vetor1_norm, 20, maior_mic1);
  normalizar_vetor(mic2, vetor2_norm, 100, maior_mic2);

  imprimir_vetor_normalizado(vetor1_norm, 20, "mic1");
  imprimir_vetor_normalizado(vetor2_norm, 100, "mic2");

  correlacao_cruzada(vetor1_norm,vetor2_norm, correlacao_resultado);
  Serial.println("Resultado da correlação cruzada:");
  for (int i = 0; i < 81; i++) {
    Serial.print("correlacao[");
    Serial.print(i);
    Serial.print("] = ");
    Serial.println(correlacao_resultado[i], 3);
}

}

void loop() {
  // Aguarda comandos futuros
}
