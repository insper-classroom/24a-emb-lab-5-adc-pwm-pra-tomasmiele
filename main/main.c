/*
 * LED blink with FreeRTOS
 */
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

#include "pico/stdlib.h"
#include <stdio.h>
#include "hardware/adc.h"

#include <math.h>
#include <stdlib.h>

#define ADC_PIN_X 26 // Exemplo de pino ADC para o eixo X
#define ADC_PIN_Y 27 // Exemplo de pino ADC para o eixo Y

QueueHandle_t xQueueAdc;

typedef struct adc {
    int axis;
    int val;
} adc_t;

// Função para inicialização do ADC
void adc_init_pin(uint pin) {
    adc_gpio_init(pin);
}

// Função para leitura e envio dos valores de ADC
void adc_task(void *params) {
    int axis = (int)params;
    uint pin = axis == 0 ? ADC_PIN_X : ADC_PIN_Y;
    int val = 0;
    adc_t data;

    while (1) {
        adc_select_input(axis);
        val = adc_read();
        // Aplicar mapeamento e filtragem aqui
        val = map(val, 0, 4095, -255, 255);
        if (abs(val) < 30) val = 0;
        
        data.axis = axis;
        data.val = val;
        xQueueSend(xQueueAdc, &data, portMAX_DELAY);
        vTaskDelay(pdMS_TO_TICKS(100)); // Taxa de amostragem
    }
}

void uart_task(void *p) {
    adc_t data;
    uint8_t datagram[4];

    while (1) {
        if (xQueueReceive(xQueueAdc, &data, portMAX_DELAY)) {
            datagram[0] = data.axis;
            datagram[1] = (data.val >> 8) & 0xFF;
            datagram[2] = data.val & 0xFF;
            datagram[3] = 0xFF; // EOP
            uart_write_blocking(UART_ID, datagram, sizeof(datagram));
        }
    }
}

int main() {
    stdio_init_all();
    adc_init();
    
    adc_init_pin(ADC_PIN_X);
    adc_init_pin(ADC_PIN_Y);

    xQueueAdc = xQueueCreate(10, sizeof(adc_t));

    xTaskCreate(adc_task, "X Axis Task", 256, (void *)0, 1, NULL);
    xTaskCreate(adc_task, "Y Axis Task", 256, (void *)1, 1, NULL);
    xTaskCreate(uart_task, "UART Task", 256, NULL, 1, NULL);
    vTaskStartScheduler();

    while (true);
}
