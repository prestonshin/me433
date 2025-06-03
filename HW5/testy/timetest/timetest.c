#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include <math.h>

#define RAM_CS_PIN 17
#define DAC_CS_PIN 14
#define SINE_FREQ 1
#define NUM_FLOATS 1000

union FloatInt {
    float f;
    uint32_t i;
};

static void spi_setup(uint dac_cs_pin, uint ram_cs_pin){
    spi_init(spi_default, 1000 * 1000); // the baud, or bits per second

    gpio_set_function(PICO_DEFAULT_SPI_RX_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICO_DEFAULT_SPI_SCK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PICO_DEFAULT_SPI_TX_PIN, GPIO_FUNC_SPI);

    gpio_init(dac_cs_pin);
    gpio_set_dir(dac_cs_pin, GPIO_OUT);
    gpio_init(ram_cs_pin);
    gpio_set_dir(ram_cs_pin, GPIO_OUT);
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

static void write(uint8_t* buf, uint len, uint cs_pin){
    cs_select(cs_pin);
    spi_write_blocking(spi_default, buf, len); // where data is a uint8_t array with length len
    cs_deselect(cs_pin);
}

static void read_write(uint8_t* send_buf, uint8_t* read_buf, uint len, uint cs_pin){
    cs_select(cs_pin);
    spi_write_read_blocking(spi_default, send_buf, read_buf, len); // where data is a uint8_t array with length len
    cs_deselect(cs_pin);  
}

static void dac_generate_buffer(uint8_t* buf, uint8_t channel, uint16_t data){
    buf[0] = 0b00110000 | (data >> 6) | channel << 7;
    buf[1] = data << 2;
}

static void spi_ram_setup(){
    uint8_t buf[2];
    buf[0] = 0b00000001; // mode
    buf[1] = 0b01000000; // sequential
    write(buf, 2, RAM_CS_PIN);
}

static void write_floats(union FloatInt* floats, uint num_floats){

    const uint setup_buffer_len = 3 + 4*num_floats;

    uint8_t buf[setup_buffer_len];

    buf[0] = 0b00000010; // write
    buf[1] = 0b00000000; // memory address 1
    buf[2] = 0b00000000; // memory address 2

    int index = 3;

    for(int i = 0; i < num_floats && index < setup_buffer_len-3; i++, index+=4){
        buf[index] = floats[i].i;
        buf[index + 1] = (floats[i].i >> 8);
        buf[index + 2] = (floats[i].i >> 16);
        buf[index + 3] = (floats[i].i >> 24);
    }

    write(buf, setup_buffer_len, RAM_CS_PIN);

}

static union FloatInt read_float(uint16_t addr){
    uint8_t send_buf[7];
    send_buf[0] = 0b00000011; // read
    send_buf[1] = addr >> 8; // memory address 1
    send_buf[2] = addr; // memory address 2 
    uint8_t read_buf[7];
    read_write(send_buf, read_buf, 7, RAM_CS_PIN);
    
    union FloatInt res;
    res.i = read_buf[3] | read_buf[4] << 8 | read_buf[5] << 16 | read_buf[6] << 24;

    return res;
}

int main()
{
    stdio_init_all();
    while (!stdio_usb_connected()) {
        sleep_ms(100);
    }
    spi_setup(DAC_CS_PIN,RAM_CS_PIN);
    spi_ram_setup();

    union FloatInt sine_vals[NUM_FLOATS];

    for(int i = 0; i < NUM_FLOATS; i++){
        sine_vals[i].f = (((sin((2.0*M_PI*SINE_FREQ/NUM_FLOATS)*i)+1)/2)*3.3);
        printf(" %.3f", sine_vals[i].f);
    }

    write_floats(sine_vals, NUM_FLOATS);


    while (true){
        union FloatInt res;
        uint8_t buf[2];
        int val;
        for(int i = 0; i < 4*NUM_FLOATS; i+=4){
            res = read_float((uint16_t)i);
            val = (int)(1024*res.f/3.3);
            printf("%d ", val);
            dac_generate_buffer(buf, 0, val);
            write(buf, 2, DAC_CS_PIN);
            sleep_ms(1);
        }
        printf("\n\n\n");
    

    }


}