#include <Wire.h>
#include <Arduino.h>
#include <SoftwareSerial.h>

// Assembly function
extern "C" uint8_t rotary_encoder();

// Variables from Assembly
extern "C" volatile uint8_t enc_dir;
extern "C" volatile uint8_t enc_passo;
extern "C" volatile uint8_t enc_volta;

const int MPU = 0x68;

int16_t AcX, AcY, AcZ, Tmp, GyX, GyY, GyZ;

//--------------------------------------------------
// SoftwareSerial para comunicação com ESP32
// Pino 10 = RX (não usado mas obrigatório declarar)
// Pino 11 = TX → conectar ao RX2 (GPIO16) da ESP32
// LEMBRE: GND em comum entre Arduino e ESP32
//         e divisor de tensão no TX (5V → 3.3V)
//--------------------------------------------------
SoftwareSerial espSerial(10, 11); // RX, TX

//--------------------------------------------------
// Envio UART para ESP32
//--------------------------------------------------
void sendToESP32(const String& prefix, const String& payload) {
  espSerial.print(prefix);
  espSerial.print(":");
  espSerial.print(payload);
  espSerial.print("\n");
}

//--------------------------------------------------
// Encoders
//--------------------------------------------------
void readEncoders() {
  uint8_t encoder = rotary_encoder();

  if (encoder >= 1 && encoder <= 4) {
    // Debug local (monitor serial USB)
    Serial.print("E"); Serial.print(encoder);
    Serial.print(enc_dir ? " R" : " L");
    Serial.print(" passo="); Serial.print(enc_passo);
    Serial.print(" volta="); Serial.println(enc_volta);

    // Envio para ESP32
    String payload = "E";
    payload += encoder;
    payload += enc_dir ? ",R" : ",L";
    payload += ",passo=";
    payload += enc_passo;
    payload += ",volta=";
    payload += enc_volta;
    sendToESP32("ENC", payload);
  }
}

//--------------------------------------------------
// MPU6050
//--------------------------------------------------
void readMPU6050() {
  Wire.beginTransmission(MPU);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU, 14, true);

  if (Wire.available() == 14) {
    AcX = Wire.read() << 8 | Wire.read();
    AcY = Wire.read() << 8 | Wire.read();
    AcZ = Wire.read() << 8 | Wire.read();
    Tmp = Wire.read() << 8 | Wire.read();
    GyX = Wire.read() << 8 | Wire.read();
    GyY = Wire.read() << 8 | Wire.read();
    GyZ = Wire.read() << 8 | Wire.read();

    // Debug local (monitor serial USB) — MANTIDO INTACTO
    Serial.print("AcX:"); Serial.print(AcX / 16384.0 * 9.80665, 3);
    Serial.print(" AcY:"); Serial.print(AcY / 16384.0 * 9.80665, 3);
    Serial.print(" AcZ:"); Serial.println(AcZ / 16384.0 * 9.80665, 3);

    Serial.print("GyX:"); Serial.print(GyX / 131.0 * 0.0174533, 4);
    Serial.print(" GyY:"); Serial.print(GyY / 131.0 * 0.0174533, 4);
    Serial.print(" GyZ:"); Serial.println(GyZ / 131.0 * 0.0174533, 4);

    Serial.print("Tmp:"); Serial.println(Tmp / 340.0 + 36.53, 2);
    Serial.println("---");

    // Envio para ESP32 — acelerômetro
    String accPayload = String(AcX / 16384.0 * 9.80665, 3) + ",";
    accPayload       += String(AcY / 16384.0 * 9.80665, 3) + ",";
    accPayload       += String(AcZ / 16384.0 * 9.80665, 3);
    sendToESP32("ACC", accPayload);

    // Envio para ESP32 — giroscópio
    String gyrPayload = String(GyX / 131.0 * 0.0174533, 4) + ",";
    gyrPayload        += String(GyY / 131.0 * 0.0174533, 4) + ",";
    gyrPayload        += String(GyZ / 131.0 * 0.0174533, 4);
    sendToESP32("GYR", gyrPayload);

    // Envio para ESP32 — temperatura
    sendToESP32("TMP", String(Tmp / 340.0 + 36.53, 2));
  }
}

//--------------------------------------------------
void setup() {
  Serial.begin(9600);       // Debug USB — MANTIDO
  espSerial.begin(9600);    // SoftwareSerial para ESP32

  Wire.begin();
  Wire.beginTransmission(MPU);
  Wire.write(0x6B);         // wake up MPU6050
  Wire.write(0);
  Wire.endTransmission(true);

  Serial.println("Iniciado.");
  Serial.println("---");
}

//--------------------------------------------------
void loop() {
  readEncoders();
  readMPU6050();
  delay(100);
}