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
#define PIN_CS_DAC   17  // Chip select for DAC
#define PIN_CS_SRAM  20  // Chip select for SRAM (different pin)
#define PIN_SCK  18
#define PIN_MOSI 19

// SRAM Commands
#define SRAM_WRITE 0b00000010
#define SRAM_READ  0b00000011
#define SRAM_RDSR  0b00000101  // Read status register
#define SRAM_WRSR  0b00000001  // Write status register

// SRAM Modes
#define SRAM_BYTE_MODE       0b00000000
#define SRAM_PAGE_MODE       0b10000000
#define SRAM_SEQUENTIAL_MODE 0b01000000

// Union for float/byte conversion
union FloatInt {
    float f;
    uint32_t i;
};

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
    // Ensure val is within 10-bit range
    
    if(channel == 1){
        first = 0b00110000;
    }else{
        first = 0b10110000;
    }

    first = first | (val >> 6);
    second = (val << 2);

    uint8_t data[2] = {first, second};
    // printf("Channel %d, Val: %d, data: 0x%02X, 0x%02X\r\n", channel, val, first, second);
    cs_select(PIN_CS_DAC);  // Use DAC chip select
    spi_write_blocking(SPI_PORT, data, 2);
    cs_deselect(PIN_CS_DAC);
}

// Initialize the external SRAM chip
void spi_ram_init() {
    // Set SRAM to sequential mode
    cs_select(PIN_CS_SRAM);  // Use SRAM chip select
    uint8_t mode_cmd[] = {SRAM_WRSR, SRAM_SEQUENTIAL_MODE};
    spi_write_blocking(SPI_PORT, mode_cmd, 2);
    cs_deselect(PIN_CS_SRAM);
    sleep_ms(1); // Small delay to ensure command is processed
}

// Write a float to external SRAM at specified address
void sram_write_float(uint16_t address, float value) {
    union FloatInt converter;
    converter.f = value;
    
    cs_select(PIN_CS_SRAM);  // Use SRAM chip select
    
    // Send write command and address
    uint8_t cmd_addr[] = {
        SRAM_WRITE,
        (address >> 8) & 0xFF,  // High byte of address
        address & 0xFF          // Low byte of address
    };
    spi_write_blocking(SPI_PORT, cmd_addr, 3);
    
    // Send the 4 bytes of the float (MSB first)
    uint8_t float_bytes[] = {
        (converter.i >> 24) & 0xFF,
        (converter.i >> 16) & 0xFF,
        (converter.i >> 8) & 0xFF,
        converter.i & 0xFF
    };
    spi_write_blocking(SPI_PORT, float_bytes, 4);
    
    cs_deselect(PIN_CS_SRAM);
}

// Read a float from external SRAM at specified address
float sram_read_float(uint16_t address) {
    union FloatInt converter;
    
    cs_select(PIN_CS_SRAM);  // Use SRAM chip select
    
    // Send read command and address
    uint8_t cmd_addr[] = {
        SRAM_READ,
        (address >> 8) & 0xFF,  // High byte of address
        address & 0xFF          // Low byte of address
    };
    spi_write_blocking(SPI_PORT, cmd_addr, 3);
    
    // Read the 4 bytes of the float
    uint8_t float_bytes[4];
    spi_read_blocking(SPI_PORT, 0, float_bytes, 4);
    
    cs_deselect(PIN_CS_SRAM);
    
    // Reconstruct the float (MSB first)
    converter.i = ((uint32_t)float_bytes[0] << 24) |
                  ((uint32_t)float_bytes[1] << 16) |
                  ((uint32_t)float_bytes[2] << 8) |
                  (uint32_t)float_bytes[3];
    
    return converter.f;
}

// Initialize SRAM with 1000 sine wave values
void initialize_sine_wave() {
    printf("Initializing SRAM with sine wave data...\n");
    
    for (int i = 0; i < 1000; i++) {
        // Generate sine wave value from 0V to 3.3V
        float angle = 2.0 * M_PI * i / 1000.0;  // Full cycle over 1000 samples
        float sine_value = sinf(angle);
        float voltage = (sine_value + 1.0) * 3.3 / 2.0;  // Scale to 0V-3.3V
        
        // Convert voltage to DAC value (assuming 10-bit DAC, 0-1023 range)
        uint16_t dac_value = (uint16_t)(voltage * 1023.0 / 3.3);
        
        // Store as float for this assignment
        float dac_float = (float)dac_value;
        
        // Write to SRAM (each float takes 4 bytes, so address = i * 4)
        sram_write_float(i * 4, dac_float);
        
        if (i % 100 == 0) {
            printf("Stored sample %d: voltage=%.3fV, dac_value=%d\n", i, voltage, dac_value);
        }
    }
    
    printf("SRAM initialization complete!\n");
}

// Generate sine wave from SRAM data
void generate_sine_from_sram() {
    printf("Starting sine wave generation from SRAM...\n");
    
    int sample_index = 0;
    
    while (true) {
        // Read float from SRAM
        float dac_float = sram_read_float(sample_index * 4);
        uint16_t dac_value = (uint16_t)dac_float;
        
        // Write to DAC
        write_dac(1, dac_value);
        
        // Move to next sample
        sample_index++;
        if (sample_index >= 1000) {
            sample_index = 0;  // Wrap around to create continuous sine wave
        }
        
        // Delay 1ms for 1Hz sine wave (1000 samples * 1ms = 1 second per cycle)
        sleep_ms(1);
    }
}

int main()
{
    stdio_init_all();
    printf("Starting External SRAM Sine Wave Generator\n");
    
    // Initialize GPIO pins for both chip selects
    gpio_init(PIN_CS_DAC);
    gpio_set_dir(PIN_CS_DAC, GPIO_OUT);
    gpio_put(PIN_CS_DAC, 1);  // CS high (deselected)
    
    gpio_init(PIN_CS_SRAM);
    gpio_set_dir(PIN_CS_SRAM, GPIO_OUT);
    gpio_put(PIN_CS_SRAM, 1);  // CS high (deselected)

    // SPI initialisation. This example will use SPI at 1MHz.
    spi_init(SPI_PORT, 1000 * 1000); // 1MHz
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    
    // Initialize external SRAM
    spi_ram_init();
    
    // Load sine wave data into SRAM during initialization
    initialize_sine_wave();
    
    // Generate sine wave from SRAM data
    generate_sine_from_sram();
    
    return 0;
}