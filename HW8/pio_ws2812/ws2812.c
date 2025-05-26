/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"
#include "ws2812.pio.h"

/**
 * NOTE:
 *  Take into consideration if your WS2812 is a RGB or RGBW variant.
 *
 *  If it is RGBW, you need to set IS_RGBW to true and provide 4 bytes per 
 *  pixel (Red, Green, Blue, White) and use urgbw_u32().
 *
 *  If it is RGB, set IS_RGBW to false and provide 3 bytes per pixel (Red,
 *  Green, Blue) and use urgb_u32().
 *
 *  When RGBW is used with urgb_u32(), the White channel will be ignored (off).
 *
 */
#define IS_RGBW false
#define NUM_PIXELS 4
#define SERVOPIN 16
#define SERVO_MIN_PULSE 0.5 // 0.5ms pulse for 0 degrees
#define SERVO_MAX_PULSE 2.5 // 2.5ms pulse for 180 degrees

#ifdef PICO_DEFAULT_WS2812_PIN
#define WS2812_PIN PICO_DEFAULT_WS2812_PIN
#else
// default to pin 2 if the board doesn't have a default WS2812 pin defined
#define WS2812_PIN 2
#endif

// Check the pin is compatible with the platform
#if WS2812_PIN >= NUM_BANK0_GPIOS
#error Attempting to use a pin>=32 on a platform that does not support it
#endif

// link three 8bit colors together
typedef struct {
    unsigned char r;
    unsigned char g;
    unsigned char b;
} wsColor; 


// adapted from https://forum.arduino.cc/index.php?topic=8498.0
// hue is a number from 0 to 360 that describes a color on the color wheel
// sat is the saturation level, from 0 to 1, where 1 is full color and 0 is gray
// brightness sets the maximum brightness, from 0 to 1

wsColor HSBtoRGB(float hue, float sat, float brightness) {
    float red = 0.0;
    float green = 0.0;
    float blue = 0.0;

    if (sat == 0.0) {
        red = brightness;
        green = brightness;
        blue = brightness;
    } else {
        if (hue == 360.0) {
            hue = 0;
        }

        int slice = hue / 60.0;
        float hue_frac = (hue / 60.0) - slice;

        float aa = brightness * (1.0 - sat);
        float bb = brightness * (1.0 - sat * hue_frac);
        float cc = brightness * (1.0 - sat * (1.0 - hue_frac));

        switch (slice) {
            case 0:
                red = brightness;
                green = cc;
                blue = aa;
                break;
            case 1:
                red = bb;
                green = brightness;
                blue = aa;
                break;
            case 2:
                red = aa;
                green = brightness;
                blue = cc;
                break;
            case 3:
                red = aa;
                green = bb;
                blue = brightness;
                break;
            case 4:
                red = cc;
                green = aa;
                blue = brightness;
                break;
            case 5:
                red = brightness;
                green = aa;
                blue = bb;
                break;
            default:
                red = 0.0;
                green = 0.0;
                blue = 0.0;
                break;
        }
    }

    unsigned char ired = red * 255.0;
    unsigned char igreen = green * 255.0;
    unsigned char iblue = blue * 255.0;

    wsColor c;
    c.r = ired;
    c.g = igreen;
    c.b = iblue;
    return c;
}


static inline void put_pixel(PIO pio, uint sm, uint32_t pixel_grb) {
    pio_sm_put_blocking(pio, sm, pixel_grb << 8u);
}

static inline uint32_t urgb_u32(uint8_t g, uint8_t r, uint8_t b) {
    return
            ((uint32_t) (g) << 8) |
            ((uint32_t) (r) << 16) |
            (uint32_t) (b);
}

// Initialize PWM for servo control (50Hz)
void initServo(uint gpio_pin) {
    gpio_set_function(gpio_pin, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(gpio_pin);
    
    // PWM frequency = 50Hz (20ms period)
    // System clock = 125MHz (default)
    float div = 150.0;
    uint16_t wrap = 20000; // 1MHz / 20000 = 50Hz
    
    pwm_set_clkdiv(slice_num, div);
    pwm_set_wrap(slice_num, wrap);
    pwm_set_enabled(slice_num, true);
}

// Set servo angle (0-180 degrees)
void setServoAngle(uint gpio_pin, float angle) {
    // Constrain angle between 0 and 180 degrees
    if (angle < 0) angle = 0;
    if (angle > 180) angle = 180;
    
    uint slice_num = pwm_gpio_to_slice_num(gpio_pin);
    
    // Calculate pulse width (in PWM counts)
    // 0 degrees = 0.5ms pulse = 2.5% duty cycle
    // 180 degrees = 2.5ms pulse = 12.5% duty cycle
    float pulse_width = SERVO_MIN_PULSE + (angle / 180.0) * (SERVO_MAX_PULSE - SERVO_MIN_PULSE);
    uint16_t level = (uint16_t)((pulse_width / 20.0)*20000);
    
    pwm_set_gpio_level(gpio_pin, level);
}


int main() {
    //set_sys_clock_48();
    stdio_init_all();
    initServo(SERVOPIN);

    // todo get free sm
    PIO pio;
    uint sm;
    uint offset;

    // This will find a free pio and state machine for our program and load it for us
    // We use pio_claim_free_sm_and_add_program_for_gpio_range (for_gpio_range variant)
    // so we will get a PIO instance suitable for addressing gpios >= 32 if needed and supported by the hardware
    bool success = pio_claim_free_sm_and_add_program_for_gpio_range(&ws2812_program, &pio, &sm, &offset, WS2812_PIN, 1, true);
    hard_assert(success);

    ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, IS_RGBW);

    int t = 0;
    wsColor colors[4];
    // colors[0]
    unsigned int counter = 0;
    // setServoAngle(SERVOPIN, 0);
    // sleep_ms(4000);

    while (1) {
        colors[0] = HSBtoRGB(counter%361, 1, 0.5);
        colors[1] = HSBtoRGB((counter + 90)%361, 1, 0.5);
        colors[2] = HSBtoRGB((counter + 180)%361, 1, 0.5);
        colors[3] = HSBtoRGB((counter + 270)%361, 1, 0.5);

        int i;
        for(i=0;i<NUM_PIXELS;i++){
            put_pixel(pio, sm, urgb_u32(colors[i].r, colors[i].g, colors[i].b)); // assuming you've made arrays of colors to send
        }
        counter = counter+1;

        

        setServoAngle(SERVOPIN, 180 - abs(180-counter));
        if(counter >= 361){
            counter = 0;
        }
        printf("%d\r\n", 180 - abs(180-counter));
        sleep_ms(13); // wait at least the reset time
        
    }

    // This will free resources and unload our program
    pio_remove_program_and_unclaim_sm(&ws2812_program, pio, sm, offset);
}
