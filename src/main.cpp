#include <arduinoFFT.h>
#include <math.h>

// LEDs
#define LED_FRENTE 10
#define LED_45_DIR_FRENTE 2
#define LED_DIREITA 6
#define LED_45_DIR_TRAS 3
#define LED_ATRAS 5
#define LED_45_ESQ_TRAS 4
#define LED_ESQUERDA 9
#define LED_45_ESQ_FRENTE 8

#define LED_PAUSA 7
#define LED_COLETA 13

const int MIC1_PIN = A0;
const int MIC2_PIN = A1;

// ===== NOVOS TAMANHOS =====
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
int maior_indice_do_vetor;

const int sampleRate = 9000;
unsigned long lastMicros = 0;

// ==========================

// void apaga_leds() {
//   digitalWrite(LED_ESQUERDA, LOW);
//   digitalWrite(LED_DIREITA, LOW);
//   digitalWrite(LED_FRENTE, LOW);
//   digitalWrite(LED_ATRAS, LOW);
//   digitalWrite(LED_45_ESQ_FRENTE, LOW);
//   digitalWrite(LED_45_DIR_FRENTE, LOW);
//   digitalWrite(LED_45_ESQ_TRAS, LOW);
//   digitalWrite(LED_45_DIR_TRAS, LOW);
//   digitalWrite(LED_PAUSA, LOW);
// }

// ==========================

void ajustar_tdoa_por_janela(uint16_t *correlacao, int *max_idx_filtrado) {
  const int centro = 100;
  const int janela = 20;

  uint16_t max_val_filtro = correlacao[centro];
  int melhor_idx = centro;

  for (int i = centro - janela; i <= centro + janela; i++) {
    if (i >= 0 && i < 201) {
      if (correlacao[i] > max_val_filtro) {
        max_val_filtro = correlacao[i];
        melhor_idx = i;
      }
    }
  }

  *max_idx_filtrado = melhor_idx;
}

// ==========================

void coleta_amostras() {
  // digitalWrite(LED_COLETA, HIGH);

  for (int i = 0; i < 300; i++) {

    // while (micros() - lastMicros < (1000000 / sampleRate));
    // lastMicros = micros();

    if (i < 100) {
      mic1[i] = analogRead(MIC1_PIN) >> 2;
    }

    mic2[i] = analogRead(MIC2_PIN) >> 2;
  }

  // digitalWrite(LED_COLETA, LOW);

  // ===== INÍCIO DO BLOCO =====
}
void print_amostras(){
    Serial.println("START");

    // ===== PRINT mic1 =====
    Serial.println("Amostras mic1:");
    for (int i = 0; i < 100; i++) {
      Serial.print("mic1[");
      Serial.print(i);
      Serial.print("] = ");
      Serial.println(mic1[i]);
    }

    // ===== PRINT mic2 =====
    Serial.println("Amostras mic2:");
    for (int i = 0; i < 300; i++) {
      Serial.print("mic2[");
      Serial.print(i);
      Serial.print("] = ");
      Serial.println(mic2[i]);
    }
}

// void coleta_amostras_grande(int bloco) {
//   for (int i = 0; i < 300; i++) {
//     vetor_final[i] = mic2[i];
//   }

//   digitalWrite(LED_PAUSA, HIGH);
//   delay(5000);
//   digitalWrite(LED_PAUSA, LOW);
// }

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
    saida[i] = max_val != 0 ? (vetor[i] * 255) / max_val : 0;
  }
}

void correlacao_cruzada(const uint8_t *mic1, const uint8_t *mic2, uint16_t *resultado) {
  for (int k = 0; k < 201; k++) {
    uint32_t soma = 0;

    for (int n = 0; n < 100; n++) {
      soma += mic1[n] * mic2[n + k];
    }

    resultado[k] = soma / 100;
  }

    Serial.println("Resultado da correlacao cruzada:");

    for (int i = 0; i < 201; i++) {
      Serial.print("correlacao[");
      Serial.print(i);
      Serial.print("] = ");
      Serial.println(correlacao_resultado[i]);
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

void acender_led_mais_proximo() {
  float angulos[] = {0, 45, 90, 135, 180};

  if (angulo_theta <= (angulos[0] + angulos[1]) / 2) {
    indice_mais_proximo = 0;
  } else if (angulo_theta >= (angulos[4] + angulos[3]) / 2) {
    indice_mais_proximo = 4;
  } else {
    for (int i = 0; i < 4; i++) {
      float li = (angulos[i] + angulos[i + 1]) / 2;
      float ls = (angulos[i + 1] + angulos[i + 2]) / 2;

      if (angulo_theta >= li && angulo_theta < ls) {
        indice_mais_proximo = i + 1;
        break;
      }
    }
  }
}

void acender_led() {
  // apaga_leds();

  // switch (indice_mais_proximo) {
  //   case 0: digitalWrite(LED_FRENTE, HIGH); break;
  //   case 1: digitalWrite(LED_45_DIR_FRENTE, HIGH); break;
  //   case 2: digitalWrite(LED_DIREITA, HIGH); break;
  //   case 3: digitalWrite(LED_45_DIR_TRAS, HIGH); break;
  //   case 4: digitalWrite(LED_ATRAS, HIGH); break;
  // }
}

// ==========================

void setup() {
  Serial.begin(115200);

  pinMode(LED_ESQUERDA, OUTPUT);
  pinMode(LED_DIREITA, OUTPUT);
  pinMode(LED_FRENTE, OUTPUT);
  pinMode(LED_ATRAS, OUTPUT);
  pinMode(LED_45_ESQ_FRENTE, OUTPUT);
  pinMode(LED_45_DIR_FRENTE, OUTPUT);
  pinMode(LED_45_ESQ_TRAS, OUTPUT);
  pinMode(LED_45_DIR_TRAS, OUTPUT);
  pinMode(LED_PAUSA, OUTPUT);
  pinMode(LED_COLETA, OUTPUT);
}

// ==========================

void loop() {

  if (cont < 10) {

    for (int i = 0; i < 350; i++) correlacao_resultado[i] = 0;

    coleta_amostras();
    print_amostras();

    encontrar_maior(mic1, 100, &maior_mic1);
    encontrar_maior(mic2, 300, &maior_mic2);

    normalizar_vetor(mic1, vetor1_norm, 100, maior_mic1);
    normalizar_vetor(mic2, vetor2_norm, 300, maior_mic2);

    Serial.println("NORMALIZADO mic1:");
      for (int i = 0; i < 100; i++) {
        Serial.println(vetor1_norm[i]);
      }

    correlacao_cruzada(vetor1_norm, vetor2_norm, correlacao_resultado);

    ajustar_tdoa_por_janela(correlacao_resultado, &max_index);
    detectar_angulo(correlacao_resultado);

    vet_valores_finais[cont] = max_index;
    vet_valores_finais2[cont] = max_val;

    cont++;

    equacao_final();
    acender_led_mais_proximo();
    acender_led();
  }
  else {
    for (int i = 0; i < 10; i++) {
      Serial.print("resultado[");
      Serial.print(i);
      Serial.print("] = ");
      Serial.print(vet_valores_finais[i]);
      Serial.print(" | ");
      Serial.println(vet_valores_finais2[i]);
      Serial.println(max_index);
    }
  }
}