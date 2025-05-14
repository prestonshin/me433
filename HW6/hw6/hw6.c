#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

// I2C defines
#define I2C_PORT i2c0
#define I2C_SDA 8
#define I2C_SCL 9

// chip registers
#define MCP23008_ADDR 0x20
#define REG_IODIR 0x00
#define REG_GPINTEN 0x02
#define REG_DEFVAL 0x03
#define REG_INTCON 0x04
#define REG_IOCON 0x05
#define REG_GPPU 0x06
#define REG_INTF 0x07
#define REG_INTCAP 0x08
#define REG_GPIO 0x09
#define REG_OLAT 0x0A

// Pin definitions
#define MCP_LED_PIN 7           // GP7 on MCP23008
#define MCP_BUTTON_PIN 0        // GP0 on MCP23008


void writeReg(uint8_t address, uint8_t reg, uint8_t value) {
    uint8_t buf[2] = {reg, value};
    i2c_write_blocking(I2C_PORT, address, buf, 2, false);
}

uint8_t readReg(uint8_t address, uint8_t reg) {
    uint8_t value = 0;
    i2c_write_blocking(I2C_PORT, address, &reg, 1, true);  // true to keep master control of bus
    i2c_read_blocking(I2C_PORT, address, &value, 1, false); // false - finished with bus
    return value;
}

void setPin(uint8_t address, uint8_t pin, bool state) {
    uint8_t olat = readReg(address, REG_OLAT);
    
    if (state) {
        olat |= (1 << pin);  // Set the bit
    } else {
        olat &= ~(1 << pin); // Clear the bit
    }
    
    writeReg(address, REG_OLAT, olat);
}

bool readPin(uint8_t address, uint8_t pin) {
    uint8_t gpio = readReg(address, REG_GPIO);
    return (gpio & (1 << pin)) != 0;
}

int main() {
    stdio_init_all();
    i2c_init(I2C_PORT, 400 * 1000);
    
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    
    // hbt
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    
    // initialize MCP23008
    writeReg(MCP23008_ADDR, REG_IODIR, 0x7F);
    writeReg(MCP23008_ADDR, REG_GPPU, (1 << MCP_BUTTON_PIN));
    

    bool button_pressed = false;
    bool led_state = false;
    uint32_t last_heartbeat = 0;
    int hbt_counter = 0;
    
    while (true) { 
        // hbt
        if (hbt_counter >= 10) {
            led_state = !led_state;
            gpio_put(PICO_DEFAULT_LED_PIN, led_state);
            hbt_counter = 0;
        }
        

        button_pressed = !readPin(MCP23008_ADDR, MCP_BUTTON_PIN);

        setPin(MCP23008_ADDR, MCP_LED_PIN, button_pressed);
        
        // Small delay to avoid flooding the console and I2C bus
        sleep_ms(50);
        hbt_counter += 1;
    }
}