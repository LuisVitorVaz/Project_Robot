#include "motor.h"

static const char *TAG = "motor";

const bdc_motor_mcpwm_config_t mcpwm_config = {
        .group_id = 0,
        .resolution_hz = resolucaoTimer,
    };

const bdc_motor_config_t front_left_motor_config = {
        .pwm_freq_hz = frequenciaMotor,
        .pwma_gpio_num = motor1Pin1,
        .pwmb_gpio_num = motor1Pin2,
    };

const bdc_motor_config_t front_right_motor_config = {
        .pwm_freq_hz = frequenciaMotor,
        .pwma_gpio_num = motor2Pin1,
        .pwmb_gpio_num = motor2Pin2,
    };

const bdc_motor_config_t rear_left_motor_config = {
        .pwm_freq_hz = frequenciaMotor,
        .pwma_gpio_num = motor3Pin1,
        .pwmb_gpio_num = motor3Pin2,
    };

const bdc_motor_config_t rear_right_motor_config= {
        .pwm_freq_hz = frequenciaMotor,
        .pwma_gpio_num = motor4Pin1,
        .pwmb_gpio_num = motor4Pin2,
    };

bdc_motor_handle_t front_left_motor;
bdc_motor_handle_t front_right_motor;
bdc_motor_handle_t rear_left_motor;
bdc_motor_handle_t rear_right_motor;

void setup_motors(void){
    ESP_ERROR_CHECK(bdc_motor_new_mcpwm_device(&front_left_motor_config, &mcpwm_config, &front_left_motor));
    ESP_ERROR_CHECK(bdc_motor_new_mcpwm_device(&front_right_motor_config, &mcpwm_config, &front_right_motor));
    ESP_ERROR_CHECK(bdc_motor_new_mcpwm_device(&rear_left_motor_config, &mcpwm_config, &rear_left_motor));
    ESP_ERROR_CHECK(bdc_motor_new_mcpwm_device(&rear_right_motor_config, &mcpwm_config, &rear_right_motor));
    ESP_LOGI(TAG, "Configuração dos motores finalizada.");

    ESP_ERROR_CHECK(bdc_motor_enable(front_left_motor));
    ESP_ERROR_CHECK(bdc_motor_enable(front_right_motor));
    ESP_ERROR_CHECK(bdc_motor_enable(rear_left_motor));
    ESP_ERROR_CHECK(bdc_motor_enable(rear_right_motor));
    ESP_LOGI(TAG, "Motores ligados.");
}

void brake(void){
    ESP_LOGI(TAG, "Parando veículo.");
    ESP_ERROR_CHECK(bdc_motor_brake(front_left_motor));
    ESP_ERROR_CHECK(bdc_motor_brake(front_right_motor));
    ESP_ERROR_CHECK(bdc_motor_brake(rear_left_motor));
    ESP_ERROR_CHECK(bdc_motor_brake(rear_right_motor));
}

void moveForward(void){
    ESP_LOGI(TAG, "Movendo veículo para frente.");
    ESP_ERROR_CHECK(bdc_motor_forward(front_left_motor));
    ESP_ERROR_CHECK(bdc_motor_forward(front_right_motor));
    ESP_ERROR_CHECK(bdc_motor_forward(rear_left_motor));
    ESP_ERROR_CHECK(bdc_motor_forward(rear_right_motor));
}

void moveBackward(void){
    ESP_LOGI(TAG, "Movendo veículo para trás.");
    ESP_ERROR_CHECK(bdc_motor_reverse(front_left_motor));
    ESP_ERROR_CHECK(bdc_motor_reverse(front_right_motor));
    ESP_ERROR_CHECK(bdc_motor_reverse(rear_left_motor));
    ESP_ERROR_CHECK(bdc_motor_reverse(rear_right_motor));
}

void rotateLeft(void){
    ESP_LOGI(TAG, "Girando veículo para esquerda.");
    ESP_ERROR_CHECK(bdc_motor_reverse(front_left_motor));
    ESP_ERROR_CHECK(bdc_motor_forward(front_right_motor));
    ESP_ERROR_CHECK(bdc_motor_reverse(rear_left_motor));
    ESP_ERROR_CHECK(bdc_motor_forward(rear_right_motor));
}

void rotateRight(void){
    ESP_LOGI(TAG, "Girando veículo para direita.");
    ESP_ERROR_CHECK(bdc_motor_forward(front_left_motor));
    ESP_ERROR_CHECK(bdc_motor_reverse(front_right_motor));
    ESP_ERROR_CHECK(bdc_motor_forward(rear_left_motor));
    ESP_ERROR_CHECK(bdc_motor_reverse(rear_right_motor));
}