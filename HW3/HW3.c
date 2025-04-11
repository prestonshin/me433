#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/adc.h"

#define LED_PIN 14

// Initialize the GPIO for the LED
void pico_led_init(void) {
    #ifdef LED_PIN
        gpio_init(LED_PIN);
        gpio_set_dir(LED_PIN, GPIO_OUT);
    #endif
    }


int main() {
    stdio_init_all();
    pico_led_init();

    while (!stdio_usb_connected()) {
        sleep_ms(100);
    }
    printf("Start!\n");
    gpio_put(LED_PIN, true);
 
    while (1) {
        char message[100];
        scanf("%s", message);
        printf("message: %s\r\n",message);
        sleep_ms(50);
    }
}
