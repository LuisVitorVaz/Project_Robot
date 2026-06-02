/*
 * ============================================================
 *  Localização de som por TDOA — ESP32
 *  VERSÃO AJUSTADA
 *  ALTERAÇÃO:
 *      - correlação usando TODO vetor
 *      - sem mudar lógica original
 * ============================================================
 */

#include <Arduino.h>
#include <math.h>
#include "driver/adc.h"

// ============================================================
// CONFIGURAÇÃO
// ============================================================

#define N_AMOSTRAS        1000

#define ADC_CH_MIC1       ADC1_CHANNEL_4   // GPIO32
#define ADC_CH_MIC2       ADC1_CHANNEL_5   // GPIO33

#define FS_CANAL          100000

#define DIST_MICS_M       0.10f

#define VELOC_SOM         343.0f

#define TAU_MAX_AMOSTRAS  58

#define CORR_TAMANHO      ((2 * N_AMOSTRAS) - 1)

#define OFFSET_CALIBRACAO 0

// ============================================================
// BUFFERS
// ============================================================

uint16_t raw1[N_AMOSTRAS];
uint16_t raw2[N_AMOSTRAS];

int16_t mic1[N_AMOSTRAS];
int16_t mic2[N_AMOSTRAS];

uint8_t vetor1_norm[N_AMOSTRAS];
uint8_t vetor2_norm[N_AMOSTRAS];

uint32_t correlacao_resultado[CORR_TAMANHO];

int16_t maior_mic1 = 0;
int16_t maior_mic2 = 0;

uint32_t max_val = 0;

int max_index = 0;
int max_index_corr = 0;

float tau_segundos = 0.0f;

int angulo_theta = 0;

// ============================================================
// ADC INIT
// ============================================================

void adc_init() {

    adc1_config_width(ADC_WIDTH_BIT_12);

    adc1_config_channel_atten(ADC_CH_MIC1,ADC_ATTEN_DB_11);

    adc1_config_channel_atten(ADC_CH_MIC2,ADC_ATTEN_DB_11);

    Serial.println("================================");
    Serial.println("ADC inicializado");
    Serial.println("Leitura alternada MIC1/MIC2");
    Serial.println("================================");
}

// ============================================================
// COLETA
// ============================================================

void IRAM_ATTR coleta_amostras() {

    for (int i = 0; i < N_AMOSTRAS; i++) {

        raw1[i] = adc1_get_raw(ADC_CH_MIC1);

        raw2[i] = adc1_get_raw(ADC_CH_MIC2);
    }

    // =========================
    // REMOVE OFFSET DC
    // =========================

    uint32_t soma1 = 0;
    uint32_t soma2 = 0;

    for (int i = 0; i < N_AMOSTRAS; i++) {

        soma1 += raw1[i];
        soma2 += raw2[i];
    }

    int16_t media1 =
        soma1 / N_AMOSTRAS;

    int16_t media2 =
        soma2 / N_AMOSTRAS;

    for (int i = 0; i < N_AMOSTRAS; i++) {

        mic1[i] =
            raw1[i] - media1;

        mic2[i] =
            raw2[i] - media2;
    }
}

// ============================================================
// FUNÇÕES ORIGINAIS
// ============================================================

void encontrar_maior(const int16_t *v,int n,int16_t *dest) {

    *dest = abs(v[0]);

    for (int i = 1; i < n; i++) {

        int16_t val = abs(v[i]);

        if (val > *dest)
            *dest = val;
    }
}

// ============================================================

void normalizar_vetor(const int16_t *v,uint8_t *out,int n,int16_t mv) {

    if (mv == 0)
        mv = 1;

    for (int i = 0; i < n; i++) {

        int32_t val =
            (((int32_t)v[i] * 127) / mv) + 128;

        if (val < 0)
            val = 0;

        if (val > 255)
            val = 255;

        out[i] = (uint8_t)val;
    }
}

// ============================================================
// CORRELAÇÃO AJUSTADA
// APENAS USANDO TODO O VETOR
// ============================================================

void correlacao_cruzada(const uint8_t *v1,const uint8_t *v2,uint32_t *res) {

    for (int i = 0; i < CORR_TAMANHO; i++) {

        uint64_t soma = 0;

        int k = N_AMOSTRAS - 1;
        int j = i;

        if (j > (N_AMOSTRAS - 1)) {

            k -= (j - (N_AMOSTRAS - 1));

            j = N_AMOSTRAS - 1;
        }

        while (j >= 0 && k >= 0) {

            soma += v2[k] * v1[j];

            j--;
            k--;
        }

        res[i] = soma / N_AMOSTRAS;
    }
}

// ============================================================

void detectar_angulo(uint32_t *corr) {

    max_val = corr[0];

    max_index = 0;

    for (int i = 1; i < CORR_TAMANHO; i++) {

        if (corr[i] > max_val) {

            max_val = corr[i];

            max_index = i;
        }
    }

    max_index_corr = max_index;

    int lag_amostras = max_index_corr - (N_AMOSTRAS - 1);

    tau_segundos = (float)lag_amostras /(float)FS_CANAL;
}

// ============================================================

void equacao_final() {

    angulo_theta = (int)(
        (-0.174 * pow(max_index, 2.0)) +
        (10.6 * max_index) +
        0.122
    );
}

// ============================================================
// SETUP
// ============================================================

void setup() {

    Serial.begin(115200);

    adc_init();
}

// ============================================================
// LOOP
// ============================================================

void loop() {

    for (int i = 0; i < CORR_TAMANHO; i++)
        correlacao_resultado[i] = 0;

    noInterrupts();

    coleta_amostras();

    interrupts();

    encontrar_maior(mic1,N_AMOSTRAS,&maior_mic1);

    encontrar_maior(mic2,N_AMOSTRAS,&maior_mic2);

    if (maior_mic1 == 0)
        maior_mic1 = 1;

    if (maior_mic2 == 0)
        maior_mic2 = 1;

    normalizar_vetor(mic1,vetor1_norm,N_AMOSTRAS,maior_mic1);

    normalizar_vetor(mic2,vetor2_norm,N_AMOSTRAS,maior_mic2);

    correlacao_cruzada(vetor1_norm,vetor2_norm,correlacao_resultado);

    detectar_angulo(correlacao_resultado);

    equacao_final();

    int lag = max_index_corr - (N_AMOSTRAS - 1);

    // ========================================================
    // DEBUG
    // ========================================================

    Serial.printf(
        "angulo=%d | lag=%d | tau=%.1f us\n",
        angulo_theta,
        lag,
        tau_segundos * 1e6f
    );

    // ========================================================
    // ENVIO PYTHON
    // ========================================================

    Serial.println("MIC1:");

    for (int i = 0; i < N_AMOSTRAS; i++) {

        Serial.println(raw1[i]);
    }

    Serial.println("MIC2:");

    for (int i = 0; i < N_AMOSTRAS; i++) {

        Serial.println(raw2[i]);
    }

    Serial.println("END");

    delay(1);
}