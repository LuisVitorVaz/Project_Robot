#include <Wire.h>
#include <Arduino.h>

// Assembly function
extern "C" uint8_t rotary_encoder();

// Variables from Assembly
extern "C" volatile uint8_t enc_dir;
extern "C" volatile uint8_t enc_passo;
extern "C" volatile uint8_t enc_volta;

const int MPU = 0x68;

int16_t AcX, AcY, AcZ, Tmp, GyX, GyY, GyZ;

//--------------------------------------------------
// Encoders
//--------------------------------------------------
void readEncoders() {
  uint8_t encoder = rotary_encoder();

  if (encoder >= 1 && encoder <= 4) {
    Serial.print("E"); Serial.print(encoder);
    Serial.print(enc_dir ? " R" : " L");
    Serial.print(" passo="); Serial.print(enc_passo);
    Serial.print(" volta="); Serial.println(enc_volta);
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

    Serial.print("AcX:"); Serial.print(AcX / 16384.0 * 9.80665, 3);
    Serial.print(" AcY:"); Serial.print(AcY / 16384.0 * 9.80665, 3);
    Serial.print(" AcZ:"); Serial.println(AcZ / 16384.0 * 9.80665, 3);

    Serial.print("GyX:"); Serial.print(GyX / 131.0 * 0.0174533, 4);
    Serial.print(" GyY:"); Serial.print(GyY / 131.0 * 0.0174533, 4);
    Serial.print(" GyZ:"); Serial.println(GyZ / 131.0 * 0.0174533, 4);

    Serial.print("Tmp:"); Serial.println(Tmp / 340.0 + 36.53, 2);
    Serial.println("---");
  }
}

//--------------------------------------------------
void setup() {
  Serial.begin(9600);

  Wire.begin();
  Wire.beginTransmission(MPU);
  Wire.write(0x6B); // wake up MPU6050
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