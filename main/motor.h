#ifndef MOTOR_H
#define MOTOR_H

#include <stdio.h>
#include "config.h"
#include "bdc_motor.h"
#include "esp_log.h"

#define frequenciaMotor 300
#define resolucaoTimer 10000000 // 10MHz

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

void setup_motors(void);
void brake(void);
void moveForward(void);
void moveBackward(void);
void rotateLeft(void);
void rotateRight(void);

#endif // MOTOR_H