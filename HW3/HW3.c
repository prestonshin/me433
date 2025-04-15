#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/adc.h"

#define LED_PIN 14
#define BUTTON_PIN 12

volatile int button_pressed = 0;
int num_readings = 0;
float result = 0;
struct repeating_timer timer;
volatile int print_count = 0;
volatile int max_prints = 0;


// Initialize the GPIO for the LED
void pico_led_init(void) {
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    }

bool repeating_timer_callback(__unused struct repeating_timer *t){
    if (print_count >= max_prints) {
        return false;  // Stop the timer
    }
    result = 3.3*adc_read()/4095.0;
    printf("%f V\r\n", result);
    print_count++;
    return true;
}

void read_print_adc(int samples){
    print_count = 0;
    max_prints = samples;
    add_repeating_timer_ms(10, repeating_timer_callback, NULL, &timer);
}

void do_stuff(uint gpio, uint32_t events) {
    gpio_put(LED_PIN, false);
    button_pressed = 1;
    sleep_ms(50);
}

// Initialize the GPIO for the button
void pico_button_init(void) {
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_set_irq_enabled_with_callback(BUTTON_PIN, GPIO_IRQ_EDGE_FALL, true, &do_stuff);
    gpio_put(PICO_DEFAULT_LED_PIN, true);
    }



void pico_adc_init(void) {
    adc_init(); // init the adc module
    adc_gpio_init(26); // set ADC0 pin to be adc input instead of GPIO
    adc_select_input(0); // select to read from ADC0
    }

int main() {
    stdio_init_all();
    pico_adc_init();
    pico_led_init();
    pico_button_init();
    // gpio_put(PICO_DEFAULT_LED_PIN, true);

    


    while (!stdio_usb_connected()) {
        sleep_ms(100);
    }
    printf("Start!\n");
    gpio_put(LED_PIN, true);
 
    while (1) {
        // printf("%d", button_pressed);
        if(button_pressed == 1){
            printf("how many readings do you want: ");
            char message[20];
            scanf("%s", message);
            printf("message: %s\r\n",message);
            int num;
            sscanf(message, "%d", &num);
            read_print_adc(num);
            button_pressed = 0;
        }
    }
}
