#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"
#include "ssd1306.h"
#include "font.h"



// // Based on the adafruit and sparkfun libraries
// #define SSD1306_MEMORYMODE          0x20 
// #define SSD1306_COLUMNADDR          0x21 
// #define SSD1306_PAGEADDR            0x22 
// #define SSD1306_SETCONTRAST         0x81 
// #define SSD1306_CHARGEPUMP          0x8D 
// #define SSD1306_SEGREMAP            0xA0 
// #define SSD1306_DISPLAYALLON_RESUME 0xA4 
// #define SSD1306_NORMALDISPLAY       0xA6 
// #define SSD1306_INVERTDISPLAY       0xA7 
// #define SSD1306_SETMULTIPLEX        0xA8 
// #define SSD1306_DISPLAYOFF          0xAE 
// #define SSD1306_DISPLAYON           0xAF 
// #define SSD1306_COMSCANDEC          0xC8 
// #define SSD1306_SETDISPLAYOFFSET    0xD3 
// #define SSD1306_SETDISPLAYCLOCKDIV  0xD5 
// #define SSD1306_SETPRECHARGE        0xD9 
// #define SSD1306_SETCOMPINS          0xDA 
// #define SSD1306_SETVCOMDETECT       0xDB 
// #define SSD1306_SETSTARTLINE        0x40 
// #define SSD1306_DEACTIVATE_SCROLL   0x2E ///< Stop scroll



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

void drawLetterColumn(int x_offset, int y_offset, char c){
    ssd1306_drawPixel(x_offset, y_offset+0, (c >> 0)%2);
    ssd1306_drawPixel(x_offset, y_offset+1, (c >> 1)%2);
    ssd1306_drawPixel(x_offset, y_offset+2, (c >> 2)%2);
    ssd1306_drawPixel(x_offset, y_offset+3, (c >> 3)%2);
    ssd1306_drawPixel(x_offset, y_offset+4, (c >> 4)%2);
    ssd1306_drawPixel(x_offset, y_offset+5, (c >> 5)%2);
    ssd1306_drawPixel(x_offset, y_offset+6, (c >> 6)%2);
    ssd1306_drawPixel(x_offset, y_offset+7, (c >> 7)%2);
       
}

void drawLetter(int x_offset, int y_offset, char c){
    c = c-32;
    // x_offset, y_offset represent the top left of the letter.
    drawLetterColumn(x_offset+0, y_offset, ASCII[c][0]);
    drawLetterColumn(x_offset+1, y_offset, ASCII[c][1]);
    drawLetterColumn(x_offset+2, y_offset, ASCII[c][2]);
    drawLetterColumn(x_offset+3, y_offset, ASCII[c][3]);
    drawLetterColumn(x_offset+4, y_offset, ASCII[c][4]);

}


// this function is dumb bc it assumes you start at 0 0 but I can easily fix later.
void drawMessage(int x_offset, int y_offset, char* message){
    // int x_offset = 0;
    // int y_offset = 0;
    for(int i = 0; i < 10; i++){
        if(message[i] != 0){
            drawLetter(x_offset+(5*i)%125, y_offset, message[i]);
        }else{
            break;
        }
        // if(i >= 24){
        //     y_offset = 8;
        // }
    }
    
}

struct repeating_timer timer;

bool LED_on = true;
bool callback(__unused struct repeating_timer *t){
    // ssd1306_drawPixel(0, 0, LED_on);
    ssd1306_update();
    gpio_put(PICO_DEFAULT_LED_PIN, LED_on);
    LED_on = !LED_on;
    return true;
}

void pico_adc_init(void) {
    adc_init(); // init the adc module
    adc_gpio_init(26); // set ADC0 pin to be adc input instead of GPIO
    adc_select_input(0); // select to read from ADC0
}

int main() {
    stdio_init_all();
    pico_adc_init();
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    
    // hbt
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
    
    ssd1306_setup();

    // 1Hz interrupt
    // add_repeating_timer_ms(500, callback, NULL, &timer);


    char message[10]; 
    sprintf(message, "abcdefghijklmnopqrstuvwxyz"); 
    // drawMessage(message); // draw starting at x=20,y=10 
    float t = 0;
    float prev_t = 0;
    while (true) {  
        t = to_us_since_boot(get_absolute_time());     
        // 
        sprintf(message, "fps: %.3f", 1000000.0/(t-prev_t)); 
        drawMessage(0, 24, message);
        sprintf(message, "V: %.3f", 3.3*adc_read()/4095); 
        drawMessage(0, 0, message);
        // printf("V: %.3f", 3.3*adc_read()/4095);
        ssd1306_update(); 
        
        prev_t = t;  
        
    }
}