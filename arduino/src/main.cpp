#include <Wire.h>
#include <Arduino.h>

//--------------------------------------------------
// NÃO usa SoftwareSerial (evita conflito de ISR)
//--------------------------------------------------
#define espSerial Serial

//--------------------------------------------------
// MPU6050
//--------------------------------------------------
const int MPU = 0x68;
int16_t AcX, AcY, AcZ, Tmp, GyX, GyY, GyZ;

//--------------------------------------------------
// Encoders
//--------------------------------------------------
const uint8_t ENC_CLK[5] = {0, A0, A2, 3, 5};
const uint8_t ENC_DT [5] = {0, A1, A3, 4, 6};

volatile uint8_t  cnt      [5] = {0};
volatile uint8_t  vol      [5] = {0};
volatile uint8_t  last_dir [5] = {0};
volatile int32_t  enc_total[5] = {0};

uint8_t  snap_dir  [5] = {0};
uint8_t  snap_passo[5] = {0};
uint8_t  snap_volta[5] = {0};
int32_t  snap_total[5] = {0};

//--------------------------------------------------
void handleEncoder(uint8_t id) {
  uint8_t clk = digitalRead(ENC_CLK[id]);
  uint8_t dt  = digitalRead(ENC_DT [id]);

  if (clk != dt) {
    cnt[id]++;
    last_dir[id] = 1;
    if (cnt[id] == 0x00) vol[id]++;
    enc_total[id] += cnt[id];
  } else {
    cnt[id]--;
    last_dir[id] = 0;
    if (cnt[id] == 0xFF) vol[id]--;
    enc_total[id] -= cnt[id];
  }
}

//--------------------------------------------------
ISR(PCINT1_vect) {
  static uint8_t prev_portc = 0xFF;
  uint8_t curr = PINC;
  uint8_t changed = curr ^ prev_portc;
  prev_portc = curr;

  if (changed & (1 << 0)) handleEncoder(1);
  if (changed & (1 << 2)) handleEncoder(2);
}

ISR(PCINT2_vect) {
  static uint8_t prev_portd = 0xFF;
  uint8_t curr = PIND;
  uint8_t changed = curr ^ prev_portd;
  prev_portd = curr;

  if (changed & (1 << 3)) handleEncoder(3);
  if (changed & (1 << 5)) handleEncoder(4);
}

//--------------------------------------------------
void snapshotEncoders() {
  noInterrupts();
  for (uint8_t i = 1; i <= 4; i++) {
    snap_dir[i]   = last_dir[i];
    snap_passo[i] = cnt[i];
    snap_volta[i] = vol[i];
    snap_total[i] = enc_total[i];
  }
  interrupts();
}

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

    snapshotEncoders();

    for (uint8_t i = 1; i <= 4; i++) {
      if (snap_passo[i] != 0 || snap_total[i] != 0) {
        Serial.print("E"); Serial.print(i);
        Serial.print(snap_dir[i] ? " R" : " L");
        Serial.print(" passo="); Serial.print(snap_passo[i]);
        Serial.print(" volta="); Serial.print(snap_volta[i]);
        Serial.print(" total="); Serial.println(snap_total[i]);
      }
    }

    String packet = "DATA:";
    packet += String(AcX / 16384.0 * 9.80665, 3); packet += ",";
    packet += String(AcY / 16384.0 * 9.80665, 3); packet += ",";
    packet += String(AcZ / 16384.0 * 9.80665, 3); packet += ",";
    packet += String(GyX / 131.0 * 0.0174533, 4); packet += ",";
    packet += String(GyY / 131.0 * 0.0174533, 4); packet += ",";
    packet += String(GyZ / 131.0 * 0.0174533, 4); packet += ",";
    packet += String(Tmp / 340.0 + 36.53, 2);

    for (uint8_t i = 1; i <= 4; i++) {
      packet += ",";
      packet += (snap_dir[i] ? "R" : "L");
      packet += ",";
      packet += snap_passo[i];
      packet += ",";
      packet += snap_volta[i];
      packet += ",";
      packet += snap_total[i];
    }

    packet += "\n";
    espSerial.print(packet);
  }
}

//--------------------------------------------------
void setup() {
  Serial.begin(9600);

  for (uint8_t i = 1; i <= 4; i++) {
    pinMode(ENC_CLK[i], INPUT_PULLUP);
    pinMode(ENC_DT[i], INPUT_PULLUP);
  }

  PCICR  |= (1 << PCIE1) | (1 << PCIE2);

  PCMSK1 |= (1 << PCINT8) | (1 << PCINT10);
  PCMSK2 |= (1 << PCINT19) | (1 << PCINT21);

  Wire.begin();
  Wire.beginTransmission(MPU);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);

  Serial.println("Iniciado.");
  Serial.println("---");
}

//--------------------------------------------------
void loop() {
  readMPU6050();
  delay(100);
}