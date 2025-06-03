#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"
#include "ssd1306.h"
#include "font.h"

// MPU6050 I2C address
#define MPU6050_ADDRESS 0x68

// MPU6050 config registers
#define CONFIG 0x1A
#define GYRO_CONFIG 0x1B
#define ACCEL_CONFIG 0x1C
#define PWR_MGMT_1 0x6B
#define PWR_MGMT_2 0x6C

// MPU6050 sensor data registers
#define ACCEL_XOUT_H 0x3B
#define ACCEL_XOUT_L 0x3C
#define ACCEL_YOUT_H 0x3D
#define ACCEL_YOUT_L 0x3E
#define ACCEL_ZOUT_H 0x3F
#define ACCEL_ZOUT_L 0x40
#define TEMP_OUT_H   0x41
#define TEMP_OUT_L   0x42
#define GYRO_XOUT_H  0x43
#define GYRO_XOUT_L  0x44
#define GYRO_YOUT_H  0x45
#define GYRO_YOUT_L  0x46
#define GYRO_ZOUT_H  0x47
#define GYRO_ZOUT_L  0x48
#define WHO_AM_I     0x75
#define DISPLAY_WIDTH 128
#define DISPLAY_HEIGHT 32
#define CENTER_X (DISPLAY_WIDTH / 2)
#define CENTER_Y (DISPLAY_HEIGHT / 2)

typedef struct {
    int16_t accel_x, accel_y, accel_z;
    int16_t temp;
    int16_t gyro_x, gyro_y, gyro_z;
} imu_data_t;

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

bool mpu6050_init() {
    uint8_t who_am_i = readReg(MPU6050_ADDRESS, WHO_AM_I);
    if (who_am_i != 0x68 && who_am_i != 0x98) {
        return false;
    }
    writeReg(MPU6050_ADDRESS, PWR_MGMT_1, 0x00);
    writeReg(MPU6050_ADDRESS, ACCEL_CONFIG, 0x00);
    writeReg(MPU6050_ADDRESS, GYRO_CONFIG, 0x18);
    
    return true;
}
void mpu6050_read_data(imu_data_t *data) {
    uint8_t buffer[14];
    uint8_t reg = ACCEL_XOUT_H;
    
    i2c_write_blocking(I2C_PORT, MPU6050_ADDRESS, &reg, 1, true);
    i2c_read_blocking(I2C_PORT, MPU6050_ADDRESS, buffer, 14, false);
    data->accel_x = (int16_t)((buffer[0] << 8) | buffer[1]);
    data->accel_y = (int16_t)((buffer[2] << 8) | buffer[3]);
    data->accel_z = (int16_t)((buffer[4] << 8) | buffer[5]);
    data->temp = (int16_t)((buffer[6] << 8) | buffer[7]);
    data->gyro_x = (int16_t)((buffer[8] << 8) | buffer[9]);
    data->gyro_y = (int16_t)((buffer[10] << 8) | buffer[11]);
    data->gyro_z = (int16_t)((buffer[12] << 8) | buffer[13]);
}

float accel_to_g(int16_t raw_accel) {
    return raw_accel * 0.000061;
}

void draw_line_vertical(int x0, int y0, int y1) {
    
    while(y0 < y1){
        ssd1306_drawPixel(x0, y0, 1);
        y0 += 1;
    }
    while(y0 > y1){
        ssd1306_drawPixel(x0, y0, 1);
        y0 -= 1;
    }
}

void draw_line_horizontal(int x0, int y0, int x1) {
    
    while(x0 < x1){
        ssd1306_drawPixel(x0, y0, 1);
        x0 += 1;
    }
    while(x0 > x1){
        ssd1306_drawPixel(x0, y0, 1);
        x0 -= 1;
    }
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
    drawLetterColumn(x_offset+0, y_offset, ASCII[c][0]);
    drawLetterColumn(x_offset+1, y_offset, ASCII[c][1]);
    drawLetterColumn(x_offset+2, y_offset, ASCII[c][2]);
    drawLetterColumn(x_offset+3, y_offset, ASCII[c][3]);
    drawLetterColumn(x_offset+4, y_offset, ASCII[c][4]);
}

void drawMessage(int x_offset, int y_offset, char* message){
    for(int i = 0; i < 20 && message[i] != 0; i++){
        int x_pos = x_offset + (5 * i) % 125;
        int y_pos = y_offset + ((5 * i) / 125) * 8;
        if (y_pos < DISPLAY_HEIGHT - 8) {
            drawLetter(x_pos, y_pos, message[i]);
        }
    }
}

void pico_adc_init(void) {
    adc_init();
    adc_gpio_init(26);
    adc_select_input(0);
}

int main() {
    stdio_init_all();
    pico_adc_init();
    
    // Initialize I2C
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    
    // Initialize OLED
    ssd1306_setup();
    mpu6050_init()
    
    imu_data_t imu_data;
    char message[25];
    
    
    while (true) {
        mpu6050_read_data(&imu_data);
        float accel_x_g = accel_to_g(imu_data.accel_x);
        float accel_y_g = accel_to_g(imu_data.accel_y);
        float accel_z_g = accel_to_g(imu_data.accel_z);

        ssd1306_clear();
    
        int line_x = CENTER_X - (int)(accel_x_g * 20);
        int line_y = CENTER_Y + (int)(accel_y_g * 20);
        if (line_x < 0) line_x = 0;
        if (line_x >= DISPLAY_WIDTH) line_x = DISPLAY_WIDTH - 1;
        if (line_y < 0) line_y = 0;
        if (line_y >= DISPLAY_HEIGHT) line_y = DISPLAY_HEIGHT - 1;
        

        draw_line_vertical(CENTER_X, CENTER_Y, line_y);
        draw_line_horizontal(CENTER_X, CENTER_Y, line_x);
        ssd1306_drawPixel(CENTER_X, CENTER_Y, 1);
        ssd1306_update();
        
        // Print to console for debugging
        printf("Accel: X=%.3f Y=%.3f Z=%.3f g\n", accel_x_g, accel_y_g, accel_z_g);
        
        // Run at approximately 100Hz
        sleep_ms(10);
    }
}