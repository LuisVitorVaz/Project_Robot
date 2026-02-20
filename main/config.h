// Each PWM channel corresponds to a motor input pin (LEDC channel)
#define canalPWM1 0   // Motor1 Pin1 -> 12
#define canalPWM2 1   // Motor1 Pin2 -> 13
#define canalPWM3 2   // Motor2 Pin1 -> 14
#define canalPWM4 3   // Motor2 Pin2 -> 27
#define canalPWM5 4   // Motor3 Pin1 -> 32
#define canalPWM6 5   // Motor3 Pin2 -> 33
#define canalPWM7 6   // Motor4 Pin1 -> 25
#define canalPWM8 7   // Motor4 Pin2 -> 26

// Pin definitions
// Motor 1 (Front Left)
#define motor1Pin1 12
#define motor1Pin2 13

// Motor 2 (Front Right)
#define motor2Pin1 14
#define motor2Pin2 27

// Motor 3 (Rear Left)
#define motor3Pin1 32
#define motor3Pin2 33

// Motor 4 (Rear Right)
#define motor4Pin1 25
#define motor4Pin2 26

// === Encoder pin definitions ===
#define ENC1_A 34
#define ENC1_B 15

#define ENC2_A 22   // Changed from 2 → 35 to avoid boot pin conflict
#define ENC2_B 4

#define ENC3_A 36
#define ENC3_B 18

#define ENC4_A 39
#define ENC4_B 21

// Encoder constants
#define PULSOS_POR_ROTACAO 333.33
#define DIAMETRO_CM 6
#define CIRCUNFERENCIA_CM (PI * DIAMETRO_CM)

// PWM settings
#define frequenciaMotor 300
#define resolucao 8 // 0 to 255