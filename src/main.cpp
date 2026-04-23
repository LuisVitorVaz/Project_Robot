#include <arduinoFFT.h>
#include <math.h>

const int MIC1_PIN = A0;
const int MIC2_PIN = A1;

uint8_t mic1[100];
uint8_t mic2[300];

uint8_t vetor1_norm[100];
uint8_t vetor2_norm[300];

uint16_t correlacao_resultado[350];
uint8_t vetor_final[300];

uint16_t vet_valores_finais[10];
uint16_t vet_valores_finais2[10];

uint8_t maior_mic1 = 0;
uint8_t maior_mic2 = 0;

uint16_t max_val;
int cont = 0;
int angulo_theta = 0;
int indice_mais_proximo = 0;
int max_index = 0;

const int sampleRate = 9000;
unsigned long lastMicros = 0;

// ==========================

void coleta_amostras() {

  const unsigned long intervalo = 1000000UL / sampleRate;
  lastMicros = micros();

  for (int i = 0; i < 300; i++) {

    while (micros() - lastMicros < intervalo);
    lastMicros += intervalo;

    if (i < 100) {
      mic1[i] = analogRead(MIC1_PIN) >> 2;
    }

    mic2[i] = analogRead(MIC2_PIN) >> 2;
  }
}

void print_amostras(){
    delay(2000);

    Serial.println("START");

    Serial.println("Amostras mic1:");
    for (int i = 0; i < 100; i++) {
      Serial.print("mic1[");
      Serial.print(i);
      Serial.print("] = ");
      Serial.println(mic1[i]);
    }

    Serial.println("Amostras mic2:");
    for (int i = 0; i < 300; i++) {
      Serial.print("mic2[");
      Serial.print(i);
      Serial.print("] = ");
      Serial.println(mic2[i]);
    }
}

// ==========================

void encontrar_maior(const uint8_t *vetor, int tamanho, uint8_t *maior_destino) {
  *maior_destino = vetor[0];

  for (int i = 1; i < tamanho; i++) {
    if (vetor[i] > *maior_destino) {
      *maior_destino = vetor[i];
    }
  }
}

void normalizar_vetor(const uint8_t *vetor, uint8_t *saida, int tamanho, uint8_t max_val) {
  for (int i = 0; i < tamanho; i++) {
    saida[i] = max_val != 0 ? ((uint16_t)vetor[i]) / max_val : 0;
  }
}

// mic1 = 100
// mic2 = 300 
void correlacao_cruzada(const uint8_t *mic1, const uint8_t *mic2, uint16_t *resultado) {

  for (int i = 0; i < 201; i++) {

    int soma = 0;
    int k = 99;
    int j = i;

    if (j > 99) {
      k = k - (j - 99);
      j = 99;
    }

    while (j >= 0 && k >= 0) {
      soma = soma + mic2[k] * mic1[j];
      j = j - 1;
      k = k - 1;
    }

    resultado[i] = soma / 100;
  }

  Serial.println("Resultado da correlacao cruzada:");

  for (int i = 0; i < 201; i++) {
    Serial.print("correlacao[");
    Serial.print(i);
    Serial.print("] = ");
    Serial.println(resultado[i]);
  }

  Serial.println("END");
}

// ==========================

void detectar_angulo(uint16_t *correlacao) {
  max_val = correlacao[0];
  max_index = 0;

  for (int i = 1; i < 201; i++) {
    if (correlacao[i] > max_val) {
      max_val = correlacao[i];
      max_index = i;
    }
  }
}

void equacao_final() {
  angulo_theta = (-0.174 * pow(max_index, 2.0)) + (10.6 * max_index) + 0.122;
}

// ==========================

void setup() {
  Serial.begin(115200);
}

// ==========================

void loop() {

  if (cont < 1) {

    for (int i = 0; i < 350; i++) correlacao_resultado[i] = 0;

    coleta_amostras();
    print_amostras();

    encontrar_maior(mic1, 100, &maior_mic1);
    encontrar_maior(mic2, 300, &maior_mic2);

    normalizar_vetor(mic1, vetor1_norm, 100, maior_mic1);
    normalizar_vetor(mic2, vetor2_norm, 300, maior_mic2);

    correlacao_cruzada(vetor1_norm, vetor2_norm, correlacao_resultado);

    detectar_angulo(correlacao_resultado);

    vet_valores_finais[cont] = max_index;
    vet_valores_finais2[cont] = max_val;

    cont++;

    equacao_final();
  }

  while(1);
}