#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/pio.h"

// SPI Defines
// We are going to use SPI 0, and allocate it to the following GPIO pins
// Pins can be changed, see the GPIO function select table in the datasheet for information on GPIO assignments
#define SPI_PORT spi0
#define PIN_MISO 16
#define PIN_CS   17
#define PIN_SCK  18
#define PIN_MOSI 19

// struct repeating_timer hbt_timer;
// bool hbt = true;

static inline void cs_select(uint cs_pin) {
    asm volatile("nop \n nop \n nop"); // FIXME
    gpio_put(cs_pin, 0);
    asm volatile("nop \n nop \n nop"); // FIXME
}

static inline void cs_deselect(uint cs_pin) {
    asm volatile("nop \n nop \n nop"); // FIXME
    gpio_put(cs_pin, 1);
    asm volatile("nop \n nop \n nop"); // FIXME
}

uint8_t first;
uint8_t second;
void write_dac(int channel, uint16_t val){
    // Ensure val is within 10-bit rangeSS
    
    if(channel == 1){
        first = 0b00110000;
    }else{
        first = 0b10110000;
    }

    first = first | (val >> 6);
    second = (val << 2);

    uint8_t data[2] = {first, second};
    // printf("Channel %d, Val: %d, data: 0x%02X, 0x%02X\r\n", channel, val, first, second);
    cs_select(PIN_CS);
    spi_write_blocking(SPI_PORT, data, 2);
    cs_deselect(PIN_CS);
}

// Generate sine wave values
uint16_t generate_sine(float time_seconds, float frequency) {
    float sine_val = sinf(2.0* M_PI * frequency * time_seconds);
    uint16_t dac_val = (uint16_t)(1*((sine_val) * 420)+420);
    printf("%d\r\n",(uint16_t)(1*((sine_val) * 512)+512));
    return dac_val;
}


uint16_t generate_triangle(float time_seconds, float frequency) {
    float period = 1.0/frequency;
    float phase = fmodf(time_seconds, period) / period; 
    float triangle_val;
    if (phase < 0.5) {
        triangle_val = 4.0 * phase - 1.0;
    } else {
        triangle_val = 3.0 - 4.0 * phase;
    }
    uint16_t dac_val = (uint16_t)((triangle_val * 420)+420);
    return dac_val;
}

void generate_waveforms() {

    
    absolute_time_t start_time = get_absolute_time();
    
    while (true) {
        absolute_time_t current_time = get_absolute_time();
        float elapsed_seconds = absolute_time_diff_us(start_time, current_time) / 1000000.0;
        
        // Generate waveform values
        uint16_t sine_val = generate_sine(elapsed_seconds, 2);
        uint16_t triangle_val = generate_triangle(elapsed_seconds, 1);
        write_dac(0, sine_val);
        write_dac(1, triangle_val);
        
        sleep_us(1000);
    }
}

int main()
{
    stdio_init_all();
    gpio_init(PIN_CS);
    // gpio_init(PIN_SCK);
    // gpio_init(PIN_MISO);
    // gpio_init(PIN_MOSI);

    // SPI initialisation. This example will use SPI at 1MHz.
    spi_init(spi0, 12 * 1000); // the baud, or bits per second
    gpio_set_function(PICO_DEFAULT_SPI_RX_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICO_DEFAULT_SPI_SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICO_DEFAULT_SPI_TX_PIN, GPIO_FUNC_SPI); 
     

    
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);
    // For more examples of SPI use see https://github.com/raspberrypi/pico-examples/tree/master/spi


    /*
    while (true) {
        printf("Hello, world!\n");
        write_dac(1, 0b1010101010);
        sleep_ms(1000);
    }
    */
    
    // Generate waveforms
    generate_waveforms();
    
    return 0;
}