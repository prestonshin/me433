#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"

// Define pins
#define LED_PIN 15
#define ADC_PIN 26  // GP26 is ADC0

// Command flags
#define FLAG_VALUE 123
#define CMD_READ_ADC 0
#define CMD_LED_ON 1
#define CMD_LED_OFF 2

// Shared variable for ADC reading
volatile float adc_voltage = 0.0f;

void core1_entry() {
    // Setup pins on core 1
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
    gpio_put(LED_PIN, 0);  // LED off initially
    
    // Initialize ADC
    adc_init();
    adc_gpio_init(ADC_PIN);
    adc_select_input(0);  // ADC0 corresponds to GP26
    
    // Signal to core 0 that initialization is complete
    multicore_fifo_push_blocking(FLAG_VALUE);
    
    // Main loop for core 1
    while (1) {
        // Wait for a command from core 0
        uint32_t cmd = multicore_fifo_pop_blocking();
        
        switch (cmd) {
            case CMD_READ_ADC:
                // Read ADC and convert to voltage (0-3.3V)
                uint16_t raw = adc_read();
                adc_voltage = (float)raw * 3.3f / 4095.0f;
                // Signal that the ADC reading is done
                multicore_fifo_push_blocking(FLAG_VALUE);
                break;
                
            case CMD_LED_ON:
                gpio_put(LED_PIN, 1);  // Turn LED on
                multicore_fifo_push_blocking(FLAG_VALUE);
                break;
                
            case CMD_LED_OFF:
                gpio_put(LED_PIN, 0);  // Turn LED off
                multicore_fifo_push_blocking(FLAG_VALUE);
                break;
                
            default:
                // Unknown command
                multicore_fifo_push_blocking(0);
                break;
        }
    }
    
    // This should never be reached
    while (1)
        tight_loop_contents();
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
        printf("Core 1 initialization failed!\n");
    else {
        printf("Core 1 initialized successfully\n");
    }

    // Main communication loop
    while(1) {
        // ask user to enter 0 1 2
        printf("\nEnter command (0, 1, or 2): ");
        
        // read from user
        int ch = getchar();
        
        // print back what user entered
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
                continue;  // Skip sending command for invalid input
        }
        
        // push command to core 1
        multicore_fifo_push_blocking(cmd);
        
        // wait for response from core 1
        uint32_t response = multicore_fifo_pop_blocking();
        
        // print result based on command
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
    
    return 0;
}