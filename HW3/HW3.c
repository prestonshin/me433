#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/adc.h"

#define LED_PIN 14
#define BUTTON_PIN 12

int button_pressed = 0;


// Initialize the GPIO for the LED
void pico_led_init(void) {
    #ifdef LED_PIN
        gpio_init(LED_PIN);
        gpio_set_dir(LED_PIN, GPIO_OUT);
    #endif
    }

// Initialize the GPIO for the button
void pico_button_init(void) {
    #ifdef BUTTON_PIN
        gpio_init(LED_PIN);
        gpio_set_dir(LED_PIN, GPIO_IN);
        gpio_set_irq_enabled_with_callback(BUTTON_PIN, GPIO_IRQ_EDGE_FALL, true, &do_stuff);
        gpio_put(PICO_DEFAULT_LED_PIN, true);
    #endif
    }

void do_stuff(uint gpio, uint32_t events) {
    button_pressed = 1;
    gpio_put(LED_PIN, false);
}

int main() {
    stdio_init_all();
    pico_led_init();
    pico_button_init();


    while (!stdio_usb_connected()) {
        sleep_ms(100);
    }
    printf("Start!\n");
    gpio_put(LED_PIN, true);
 
    while (1) {
        if(button_pressed == 1){
            char message[100];
            scanf("%d", message);
            printf("message: %d\r\n",message);
            sleep_ms(50);
        }
    }
}
