#include "main.h"
#include <math.h>
#include <stdio.h>

volatile uint32_t ic_val;  // Captured timer value (ticks)

// Step 1: Discharge the capacitor by pulling the pin low
void discharge(void) {
    HAL_GPIO_WritePin(C_DISCHARGE_GPIO_Port, C_DISCHARGE_Pin, GPIO_PIN_RESET);
    HAL_Delay_us(100);  // Wait ≥5τ to ensure full discharge
}

// Step 2: Start timing by enabling input capture on a high-Z pin
void sample(void) {
    __HAL_TIM_SET_COUNTER(&htim2, 0);  // Reset timer
    HAL_GPIO_DeInit(C_DISCHARGE_GPIO_Port, C_DISCHARGE_Pin);  // Set pin to high-impedance (floating)
    HAL_TIM_IC_Start_IT(&htim2, TIM_CHANNEL_1);  // Start input capture with interrupt
}

// Step 3: Interrupt callback when capacitor charges to threshold (~63% of V_s)
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM2 && htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1) {
        ic_val = HAL_TIM_ReadCapturedValue(&htim2, TIM_CHANNEL_1);  // Read timer count
        HAL_TIM_IC_Stop_IT(&htim2, TIM_CHANNEL_1);  // Stop capture

        // Convert captured ticks to time in seconds (timer clock = 72 MHz)
        float t = ic_val / 72e6;

        // RC time constant: C = t / (0.693 * R), where R = 100kΩ
        float Cx = t / (0.693 * 100e3);  // Capacitance in Farads

        log_result(Cx);  // Display or record the result
        reinit_discharge_pin();  // Reconfigure pin for next discharge cycle
    }
}

// Convert capacitance to water height and print the result
void log_result(float Cx) {
    float CpF = Cx * 1e12;  // Convert from Farads to pF
    float height_cm = (CpF - 51.0f) / (0.0157f * 10.0f);  // Transfer function from report

    printf("C = %.1f pF, Height = %.2f cm\r\n", CpF, height_cm);
}

// Set the discharge pin back to output-low (pull-down)
void reinit_discharge_pin(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = C_DISCHARGE_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(C_DISCHARGE_GPIO_Port, &GPIO_InitStruct);

    HAL_GPIO_WritePin(C_DISCHARGE_GPIO_Port, C_DISCHARGE_Pin, GPIO_PIN_RESET);
}
