/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "pico/stdlib.h"
#include <stdio.h>
#include "pico/stdlib.h"


#ifndef LED_DELAY_MS
#define LED_DELAY_MS 1000
#endif

#ifndef LED_PIN
#define LED_PIN 14
#endif

#ifndef BUTTON_PIN
#define BUTTON_PIN 12
#endif

volatile unsigned int presses = 0;
bool on = true;

// Initialize the GPIO for the LED
void pico_led_init(void) {
#ifdef LED_PIN
    // A device like Pico that uses a GPIO for the LED will define LED_PIN
    // so we can use normal GPIO functionality to turn the led on and off
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
#endif
}


void do_stuff(uint gpio, uint32_t events) {
    presses++;
    gpio_put(LED_PIN, on);
    on = !on;
    // printf("presses: %d", presses);
}

int main() {
    stdio_init_all();
    pico_led_init();
    

    gpio_init(BUTTON_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_IN);
    
    gpio_set_irq_enabled_with_callback(BUTTON_PIN, GPIO_IRQ_EDGE_FALL, true, &do_stuff);
    gpio_put(PICO_DEFAULT_LED_PIN, true);
    while (true) ;
    //  {
    //     pico_set_led(true);
    //     sleep_ms(LED_DELAY_MS);
    //     pico_set_led(false);
    //     sleep_ms(LED_DELAY_MS);
    // }
}


