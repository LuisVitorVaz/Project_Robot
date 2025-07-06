#include <arduinoFFT.h>
#include <math.h>

// ------- MIC CONFIG ---------------------
// o que eu fiz fiz a correlacao cruzada entre os dois vetor apos 
// foi guardado o vetor correlacao cruzada nesse vetor se procurou o maior indice quando 
// localizado foi pegado sua posicao no vetor e imprimida na tela
const int MIC1_PIN = A0;
const int MIC2_PIN = A1;
int soma=0;
float mic1[20];
float mic2[100];
float maior_mic1 = 0.0;
float maior_mic2 = 0.0;
float vetor1_norm[20];
float vetor2_norm[100];
float correlacao_resultado[81];
float vetor_final[1000];
float vet_valores_finais[10];
float menor_diferenca=0;
int angulos[] = {0, 45, 90, 135, 180};
int cont=0;
int angulo_theta =0;
int indice_mais_proximo = 0;
 int i, max_index = 0;
 int maior_indice_do_vetor;

float max_val;
// LEDs
#define LED_FRENTE 10
#define LED_45_DIR_FRENTE 2
#define LED_DIREITA 9
#define LED_45_DIR_TRAS 3
#define LED_ATRAS 5
#define LED_45_ESQ_TRAS 4
#define LED_ESQUERDA 9
#define LED_45_ESQ_FRENTE 8

#define LED_PAUSA 7  // Novo LED para indicar pausa/coleta
#define LED_COLETA 13  // Novo LED para indicar coleta

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
  digitalWrite(LED_PAUSA, LOW);  // apaga o LED de pausa
}
void imprimir_vetor(){
    for (int i = 0; i < 1000; i++) {
      Serial.print("valores vetor final[");
      Serial.print(i);
      Serial.print("] = ");
      Serial.println(vetor_final[i], 3);
}
}
void coleta_amostras_grande(int bloco) {

    for (int i = 0; i < 100; i++) {
      // mic2[i] = (float)analogRead(MIC2_PIN);
      vetor_final[bloco * 100 + i] = mic2[i];
    }

    // Acende LED no pino 7
    digitalWrite(LED_PAUSA, HIGH);
    Serial.println("Aguardando 1 minuto...");
    delay(20000); // espera de 1 minuto (60000 ms)

    // Apaga LED
    digitalWrite(LED_PAUSA, LOW);
    delay(100);
  }

void coleta_amostras() {
  digitalWrite(LED_COLETA,HIGH);
  for (int i = 0; i < 100; i++) {
    if (i < 20) {
      mic1[i] = (float)analogRead(MIC1_PIN);
    }
    mic2[i] = (float)analogRead(MIC2_PIN);
    
    // coleta_amostras_grande();

  }
  Serial.println("Fim da coleta de amostras.");
  digitalWrite(LED_COLETA,LOW);
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

// void encontrar_maior(const float *vetor, int tamanho, float *maior_destino) {
//   *maior_destino = vetor[0];
//   for (int i = 1; i < tamanho; i++) {
//     if (vetor[i] >= *maior_destino) {
//       *maior_destino = vetor[i];
//       maior_indice_do_vetor=i;
//     }
//   }
// }

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
void correlacao_cruzada(const float *mic1, const float *mic2, float *resultado) {
  for (int k = 0; k <= 80; k++) { // 100 - 20 = 80 deslocamentos possíveis
    float soma = 0.0;
    for (int n = 0; n < 20; n++) {
      // soma += mic1[n] * mic2[n + k];
      correlacao_resultado[k] += mic1[n] * mic2[n + k];
    }
    resultado[k] = soma/20; // normaliza pela quantidade de amostras

    //  deve retornar um vetor  ou a posicao onde teve o maior valor da correlacao
    // encontrar_maior();
  }
}
void equacao_final(){
  // equation of function
  angulo_theta =(0.00476*(pow(max_index,2.0))+(1.05*(pow(max_index,1.0)))-61.5);
  // angulo_theta= 45*(max_index);
  Serial.print("valor da funcao e \n");
  Serial.print(max_index);
}
void acender_led_mais_proximo() {
  menor_diferenca = abs(angulo_theta - angulos[0]);

  // Encontrar o índice do ângulo mais próximo
  for (int i = 0; i < 5; i++) {
    float diferenca = abs(angulo_theta - angulos[i]);
    if (diferenca < menor_diferenca) {
      menor_diferenca = diferenca;
      indice_mais_proximo = i;
    }
  }
  // Serial.println("a menor diferenca e");
  // Serial.print(indice_mais_proximo);
}
void acender_led()
{
      // Resetar todos os LEDs antes (boa prática)
    digitalWrite(LED_FRENTE, LOW);
    digitalWrite(LED_45_DIR_FRENTE, LOW);
    digitalWrite(LED_DIREITA, LOW);
    digitalWrite(LED_45_DIR_TRAS, LOW);
    digitalWrite(LED_ATRAS, LOW);
    digitalWrite(LED_45_ESQ_TRAS, LOW);
    digitalWrite(LED_ESQUERDA, LOW);
    digitalWrite(LED_45_ESQ_FRENTE, LOW);

  // Acionar LED baseado no índice do pico (direção do som) com margem de erro ±2
  if (indice_mais_proximo >= 22 && indice_mais_proximo <= 26)
  {
      digitalWrite(LED_ESQUERDA, HIGH);       // Ângulo -180°

  } 
  else if (indice_mais_proximo >= 9 && indice_mais_proximo <= 13) {
      digitalWrite(LED_ATRAS, HIGH);          // Ângulo -90°
  } 
  else if (indice_mais_proximo >= 6 && indice_mais_proximo <= 10) {
      digitalWrite(LED_45_ESQ_TRAS, HIGH);    // Ângulo -135°
  }
  else if (indice_mais_proximo >= 14 && indice_mais_proximo <= 18) {
      digitalWrite(LED_45_DIR_FRENTE, HIGH);  // Ângulo -45°
  } 
  else if (indice_mais_proximo >= 63 && indice_mais_proximo <= 67) {
      digitalWrite(LED_45_ESQ_FRENTE, HIGH);  // Ângulo 0°
  } 
}
void detectar_angulo(float correlacao[81]) {
  
    max_val = correlacao[0];

    // Encontrar índice do pico da correlação
    for (i = 1; i < 81; i++) {
        if (correlacao[i] >= max_val) {
            max_val = correlacao[i];
            max_index = i;
        }
    } 
    // delay(10000);
    Serial.print("aguardando intervalo de tempo \n");
    Serial.print(max_val);
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
  pinMode(LED_PAUSA, OUTPUT);
  pinMode(LED_COLETA, OUTPUT);


}

void loop() {
  if(cont<10)
  {  
    coleta_amostras();
    imprimir_amostras();

    normalizar_vetor(mic1, vetor1_norm, 20, maior_mic1);
    normalizar_vetor(mic2, vetor2_norm, 100, maior_mic2);

    imprimir_vetor_normalizado(vetor1_norm, 20, "mic1");
    imprimir_vetor_normalizado(vetor2_norm, 100, "mic2");

    correlacao_cruzada(vetor1_norm,vetor2_norm, correlacao_resultado);
    detectar_angulo(correlacao_resultado);
    // encontrar_maior(correlacao_resultado,100, &maior_mic1);
    // encontrar_maior(mic2, 100, &maior_mic2);

    Serial.print("Maior valor do vetor correlacao: ");
    Serial.println(maior_mic1);
    Serial.println(maior_indice_do_vetor);
    vet_valores_finais[cont]=maior_indice_do_vetor;
    // Serial.print("Maior valor de mic2: ");
    // Serial.println(maior_mic2);
    // Serial.println(maior_indice_do_vetor);

    // Serial.println("Resultado da correlação cruzada:");
  //   for (int i = 0; i < 81; i++) {
  //     Serial.print("correlacao[");
  //     Serial.print(i);
  //     Serial.print("] = ");
  //     Serial.println(correlacao_resultado[i], 3);
  // }
      coleta_amostras_grande(cont);
      cont++;
      // na funcao detectar angulo e preciso fazer a seguinte coisa 
      // de acordo com a posicao encontrada no vetor mostrar qual e a direcao que aquele valore representa
      equacao_final();
      acender_led_mais_proximo();
        
  }
  // Serial.print("valores no vetor final");
  // for(int i=0;i<10;i++)
  // {
  //   Serial.print(vet_valores_finais[i]);
  // }
  //  delay(30000);
 
  }