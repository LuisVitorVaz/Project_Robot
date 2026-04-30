#include <Arduino.h>
#include "driver/adc.h"

// ==========================
#define ADC_CH_MIC1 ADC1_CHANNEL_6 // GPIO34
#define ADC_CH_MIC2 ADC1_CHANNEL_7 // GPIO35

// ==========================
float adc_to_voltage(int val) {
  return (val / 4095.0) * 3.3;
}

// ==========================
void setup() {
  Serial.begin(115200);

  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(ADC_CH_MIC1, ADC_ATTEN_DB_11);
  adc1_config_channel_atten(ADC_CH_MIC2, ADC_ATTEN_DB_11);

  Serial.println("Leitura direta ADC (SEM I2S)");
}

// ==========================
void loop() {

  Serial.println("==== MIC1 (V) ====");
  for (int i = 0; i < 20; i++) {
    int val = adc1_get_raw(ADC_CH_MIC1);
    Serial.println(adc_to_voltage(val), 4);
    delayMicroseconds(200);
  }

  Serial.println("==== MIC2 (V) ====");
  for (int i = 0; i < 20; i++) {
    int val = adc1_get_raw(ADC_CH_MIC2);
    Serial.println(adc_to_voltage(val), 4);
    delayMicroseconds(200);
  }

  Serial.println("------------------------");
  delay(500);
}