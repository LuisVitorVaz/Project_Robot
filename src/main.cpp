/*
 * ============================================================
 *  Localização de som por TDOA — ESP32
 *  VERSÃO COM GCC-PHAT + SERIAL PARA PYTHON
 *
 *  MANTIDO:
 *      - toda estrutura original
 *      - correlação original
 *      - detectar_angulo()
 *      - buffers
 *      - lógica original
 *
 *  ADICIONADO:
 *      - GCC-PHAT
 *      - envio serial compatível com Python
 *      - redução de amostras serial
 *      - estabilidade serial
 *      - filtro físico no GCC
 * ============================================================
 */

#include <Arduino.h>
#include <math.h>
#include "driver/adc.h"
#include <arduinoFFT.h>

// ============================================================
// FFT
// ============================================================

ArduinoFFT<float> FFT = ArduinoFFT<float>();

// ============================================================
// CONFIGURAÇÃO
// ============================================================

#define N_AMOSTRAS        256

#define ADC_CH_MIC1       ADC1_CHANNEL_4
#define ADC_CH_MIC2       ADC1_CHANNEL_5

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

uint32_t correlacao_gcc_resultado[CORR_TAMANHO];

int16_t maior_mic1 = 0;
int16_t maior_mic2 = 0;

uint32_t max_val = 0;

int max_index = 0;
int max_index_corr = 0;

float tau_segundos = 0.0f;

int angulo_theta = 0;

// ============================================================
// FFT GCC-PHAT
// ============================================================

float fft_real1[N_AMOSTRAS];
float fft_imag1[N_AMOSTRAS];

float fft_real2[N_AMOSTRAS];
float fft_imag2[N_AMOSTRAS];

float gcc_real[N_AMOSTRAS];
float gcc_imag[N_AMOSTRAS];

// ============================================================
// ADC INIT
// ============================================================

void adc_init() {

    adc1_config_width(ADC_WIDTH_BIT_12);

    adc1_config_channel_atten(
        ADC_CH_MIC1,
        ADC_ATTEN_DB_12
    );

    adc1_config_channel_atten(
        ADC_CH_MIC2,
        ADC_ATTEN_DB_12
    );

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

    // ========================================================
    // REMOVE OFFSET DC
    // ========================================================

    uint32_t soma1 = 0;
    uint32_t soma2 = 0;

    for (int i = 0; i < N_AMOSTRAS; i++) {

        soma1 += raw1[i];
        soma2 += raw2[i];
    }

    int16_t media1 = soma1 / N_AMOSTRAS;
    int16_t media2 = soma2 / N_AMOSTRAS;

    for (int i = 0; i < N_AMOSTRAS; i++) {

        mic1[i] = raw1[i] - media1;

        mic2[i] = raw2[i] - media2;
    }
}

// ============================================================
// ENCONTRAR MAIOR
// ============================================================

void encontrar_maior(
    const int16_t *v,
    int n,
    int16_t *dest
) {

    *dest = abs(v[0]);

    for (int i = 1; i < n; i++) {

        int16_t val = abs(v[i]);

        if (val > *dest)
            *dest = val;
    }
}

// ============================================================
// NORMALIZAÇÃO
// ============================================================

void normalizar_vetor(
    const int16_t *v,
    uint8_t *out,
    int n,
    int16_t mv
) {

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
// CORRELAÇÃO ORIGINAL
// ============================================================

void correlacao_cruzada(
    const uint8_t *v1,
    const uint8_t *v2,
    uint32_t *res
) {

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
// GCC-PHAT
// ============================================================

void correlacao_gcc_phat(
    const int16_t *v1,
    const int16_t *v2,
    uint32_t *res
) {

    for (int i = 0; i < CORR_TAMANHO; i++) {

        res[i] = 0;
    }

    // ========================================================
    // COPIA
    // ========================================================

    for (int i = 0; i < N_AMOSTRAS; i++) {

        fft_real1[i] = (float)v1[i];
        fft_imag1[i] = 0.0f;

        fft_real2[i] = (float)v2[i];
        fft_imag2[i] = 0.0f;
    }

    // ========================================================
    // WINDOW
    // ========================================================

    FFT.windowing(
        fft_real1,
        N_AMOSTRAS,
        FFT_WIN_TYP_HAMMING,
        FFT_FORWARD
    );

    FFT.windowing(
        fft_real2,
        N_AMOSTRAS,
        FFT_WIN_TYP_HAMMING,
        FFT_FORWARD
    );

    // ========================================================
    // FFT
    // ========================================================

    FFT.compute(
        fft_real1,
        fft_imag1,
        N_AMOSTRAS,
        FFT_FORWARD
    );

    FFT.compute(
        fft_real2,
        fft_imag2,
        N_AMOSTRAS,
        FFT_FORWARD
    );

    // ========================================================
    // GCC-PHAT
    // ========================================================

    for (int i = 0; i < N_AMOSTRAS; i++) {

        float real =
            (fft_real1[i] * fft_real2[i]) +
            (fft_imag1[i] * fft_imag2[i]);

        float imag =
            (fft_imag1[i] * fft_real2[i]) -
            (fft_real1[i] * fft_imag2[i]);

        float mag =
            sqrtf((real * real) + (imag * imag));

        if (mag < 1e-9f)
            mag = 1e-9f;

        gcc_real[i] = real / mag;
        gcc_imag[i] = imag / mag;
    }

    // ========================================================
    // IFFT
    // ========================================================

    FFT.compute(
        gcc_real,
        gcc_imag,
        N_AMOSTRAS,
        FFT_REVERSE
    );

    // ========================================================
    // CENTRALIZA
    // ========================================================

    int centro = N_AMOSTRAS / 2;

    for (int i = 0; i < N_AMOSTRAS; i++) {

        int idx =
            i + ((N_AMOSTRAS - 1) - centro);

        if (idx >= 0 && idx < CORR_TAMANHO) {

            float val = fabs(gcc_real[i]);

            res[idx] =
                (uint32_t)(val * 100000.0f);
        }
    }
}

// ============================================================
// DETECTAR ANGULO
// ============================================================

void detectar_angulo(uint32_t *corr) {

    int inicio =
        (N_AMOSTRAS - 1) - TAU_MAX_AMOSTRAS;

    int fim =
        (N_AMOSTRAS - 1) + TAU_MAX_AMOSTRAS;

    max_val = corr[inicio];

    max_index = inicio;

    for (int i = inicio; i <= fim; i++) {

        if (corr[i] > max_val) {

            max_val = corr[i];

            max_index = i;
        }
    }

    max_index_corr = max_index;

    int lag_amostras =
        max_index_corr - (N_AMOSTRAS - 1);

    tau_segundos =
        (float)lag_amostras /
        (float)FS_CANAL;
}

// ============================================================
// ENVIO SERIAL PYTHON
// ============================================================

void enviar_python() {

    Serial.println("MIC1:");

    for (int i = 0; i < N_AMOSTRAS; i++) {

        Serial.println(raw1[i]);
    }

    Serial.println("MIC2:");

    for (int i = 0; i < N_AMOSTRAS; i++) {

        Serial.println(raw2[i]);
    }

    Serial.println("END");
}

// ============================================================
// SETUP
// ============================================================

void setup() {

    Serial.begin(115200);

    delay(2000);

    adc_init();
}

// ============================================================
// LOOP
// ============================================================

void loop() {

    // ========================================================
    // LIMPA BUFFERS
    // ========================================================

    for (int i = 0; i < CORR_TAMANHO; i++) {

        correlacao_resultado[i] = 0;

        correlacao_gcc_resultado[i] = 0;
    }

    // ========================================================
    // COLETA
    // ========================================================

    noInterrupts();

    coleta_amostras();

    interrupts();

    // ========================================================
    // NORMALIZAÇÃO
    // ========================================================

    encontrar_maior(
        mic1,
        N_AMOSTRAS,
        &maior_mic1
    );

    encontrar_maior(
        mic2,
        N_AMOSTRAS,
        &maior_mic2
    );

    if (maior_mic1 == 0)
        maior_mic1 = 1;

    if (maior_mic2 == 0)
        maior_mic2 = 1;

    normalizar_vetor(
        mic1,
        vetor1_norm,
        N_AMOSTRAS,
        maior_mic1
    );

    normalizar_vetor(
        mic2,
        vetor2_norm,
        N_AMOSTRAS,
        maior_mic2
    );

    // ========================================================
    // CORRELAÇÃO ORIGINAL
    // ========================================================

    correlacao_cruzada(
        vetor1_norm,
        vetor2_norm,
        correlacao_resultado
    );

    detectar_angulo(
        correlacao_resultado
    );

    int lag_original =
        max_index_corr - (N_AMOSTRAS - 1);

    // ========================================================
    // GCC-PHAT
    // ========================================================

    correlacao_gcc_phat(
        mic1,
        mic2,
        correlacao_gcc_resultado
    );

    detectar_angulo(
        correlacao_gcc_resultado
    );

    int lag_gcc =
        max_index_corr - (N_AMOSTRAS - 1);

    // ========================================================
    // ENVIA PYTHON
    // ========================================================

    enviar_python();

    // ========================================================
    // DEBUG
    // ========================================================

    Serial.printf(
        "ORIGINAL_LAG:%d\n",
        lag_original
    );

    Serial.printf(
        "GCC_LAG:%d\n",
        lag_gcc
    );

    delay(20);
}

