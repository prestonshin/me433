#include <stdio.h>
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


struct repeating_timer hbt_timer;
bool hbt = true;

bool hbt_callback(__unused struct repeating_timer *t){
    gpio_put(PICO_DEFAULT_LED_PIN, hbt);
    hbt = !hbt;
    return true;
}

void hbt_init(){
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    add_repeating_timer_ms(500, hbt_callback, NULL, &hbt_timer);
}

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
void write_dac(int channel,uint16_t val){
    if(channel == 1){
        first = 0b01110000;
    }else{
        first = 0b11110000;
    }

    first = first | (val >> 4);
    second = val << 2;

    first = 0b01111000;
    second = 0b00000000;

    uint8_t data[2] = {first, second};
    printf("data: \t%d, \t%d\r\n", first, second);
    cs_select(PIN_CS);
    spi_write_blocking(SPI_PORT, data, 2); // where data is a uint8_t array with length len
    cs_deselect(PIN_CS);

}

int main()
{
    stdio_init_all();
    hbt_init();
    gpio_init(PIN_CS);
    // gpio_init(PIN_SCK);
    // gpio_init(PIN_MISO);
    // gpio_init(PIN_MOSI);

    // SPI initialisation. This example will use SPI at 1MHz.
    spi_init(spi_default, 12 * 1000); // the baud, or bits per second
    gpio_set_function(PICO_DEFAULT_SPI_RX_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICO_DEFAULT_SPI_SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICO_DEFAULT_SPI_TX_PIN, GPIO_FUNC_SPI); 
    
    // Chip select is active-low, so we'll initialise it to a driven-high state
    
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);
    // For more examples of SPI use see https://github.com/raspberrypi/pico-examples/tree/master/spi

    while (true) {
        printf("Hello, world!\n");
        write_dac(1, 0b1000000000);
        sleep_ms(1000);
    }
}
