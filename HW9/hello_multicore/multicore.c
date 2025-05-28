/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"



#define FLAG_VALUE 123
#define LED_PIN 15
#define ADC_PIN 26
#define CMD_READ_ADC 0
#define CMD_LED_ON 1
#define CMD_LED_OFF 2
volatile float adc_voltage = 0.0f;

void core1_entry() {
    // setup
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);
    adc_init();
    adc_gpio_init(ADC_PIN);
    adc_select_input(0);
    
    multicore_fifo_push_blocking(FLAG_VALUE);
    
    while (1) {
        uint32_t cmd = multicore_fifo_pop_blocking();
        
        switch (cmd) {
            case CMD_READ_ADC:
                uint16_t raw = adc_read();
                adc_voltage = (float)raw * 3.3f / 4095.0f;
                multicore_fifo_push_blocking(FLAG_VALUE);
                break;              
            case CMD_LED_ON:
                gpio_put(LED_PIN, 1);
                multicore_fifo_push_blocking(FLAG_VALUE);
                break;    
            case CMD_LED_OFF:
                gpio_put(LED_PIN, 0);
                multicore_fifo_push_blocking(FLAG_VALUE);
                break;
            default:
                multicore_fifo_push_blocking(0);
                break;
        }
    }
}

int main() {
    stdio_init_all();

    while (!stdio_usb_connected()) {
        sleep_ms(100);
    }

    printf("Hello, multicore!\n");
    printf("Commands:\n");
    printf("  '0': Read voltage on pin A0\n");
    printf("  '1': Turn on LED on GP15\n");
    printf("  '2': Turn off LED on GP15\n");

    /// \tag::setup_multicore[]

    multicore_launch_core1(core1_entry);

    // Wait for core 1 to initialize

    uint32_t g = multicore_fifo_pop_blocking();

    if (g != FLAG_VALUE)
        printf("Hmm, that's not right on core 0!\n");
    else {
        multicore_fifo_push_blocking(FLAG_VALUE);
        printf("It's all gone well on core 0!");
    }

    while(1) {
        printf("\nEnter command (0, 1, or 2): ");
        int ch = getchar();
        
        printf("You entered: %c\n", ch);
        
        uint32_t cmd = 0;
        
        switch (ch) {
            case '0':
                printf("Reading ADC...\n");
                cmd = CMD_READ_ADC;
                break;
            case '1':
                printf("Turning LED on...\n");
                cmd = CMD_LED_ON;
                break;
            case '2':
                printf("Turning LED off...\n");
                cmd = CMD_LED_OFF;
                break;
            default:
                printf("Invalid command. Use '0', '1', or '2'\n");
                continue;
        }
        
        multicore_fifo_push_blocking(cmd);
        uint32_t response = multicore_fifo_pop_blocking();
        
        if (response == FLAG_VALUE) {
            if (cmd == CMD_READ_ADC) {
                printf("Voltage on A0: %.2f V\n", adc_voltage);
            } else if (cmd == CMD_LED_ON) {
                printf("LED turned on\n");
            } else if (cmd == CMD_LED_OFF) {
                printf("LED turned off\n");
            }
        } else {
            printf("Error: Command failed\n");
        }
    }

    /// \end::setup_multicore[]
}