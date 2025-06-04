/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "bsp/board_api.h"
#include "tusb.h"
#include "hardware/gpio.h"

#include "usb_descriptors.h"

//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF PROTYPES
//--------------------------------------------------------------------+

/* Blink pattern
 * - 250 ms  : device not mounted
 * - 1000 ms : device mounted
 * - 2500 ms : device is suspended
 */
enum  {
  BLINK_NOT_MOUNTED = 250,
  BLINK_MOUNTED = 1000,
  BLINK_SUSPENDED = 2500,
};

// GPIO pin definitions
#define PIN_UP 2
#define PIN_DOWN 3
#define PIN_LEFT 4
#define PIN_RIGHT 5
#define PIN_MODE 6
#define PIN_LED 25
#define SPEED_1 1
#define SPEED_2 3
#define SPEED_3 5
#define SPEED_4 7


#define SPEED_THRESHOLD_1 500
#define SPEED_THRESHOLD_2 1000
#define SPEED_THRESHOLD_3 2000


#define CIRCLE_RADIUS 200
#define CIRCLE_SPEED 0.05

static uint32_t blink_interval_ms = BLINK_NOT_MOUNTED;
static bool remote_work_mode = false;
static float circle_angle = 0.0f;
static uint32_t button_press_time[4] = {0}; // up down left right
static bool button_was_pressed[4] = {false};

void led_blinking_task(void);
void hid_task(void);
void init_buttons(void);
bool read_button(uint8_t pin);
void handle_mode_button(void);
int8_t calculate_speed(uint32_t press_duration);

/*------------- MAIN -------------*/
int main(void)
{
  board_init();
  init_buttons();

  // init device stack on configured roothub port
  tud_init(BOARD_TUD_RHPORT);

  if (board_init_after_tusb) {
    board_init_after_tusb();
  }

  while (1)
  {
    tud_task(); // tinyusb device task
    led_blinking_task();
    handle_mode_button();
    hid_task();
  }
}

//--------------------------------------------------------------------+
// Button and GPIO Functions
//--------------------------------------------------------------------+

void init_buttons(void)
{
  gpio_init(PIN_UP);
  gpio_set_dir(PIN_UP, GPIO_IN);
  gpio_pull_up(PIN_UP);
  
  gpio_init(PIN_DOWN);
  gpio_set_dir(PIN_DOWN, GPIO_IN);
  gpio_pull_up(PIN_DOWN);
  
  gpio_init(PIN_LEFT);
  gpio_set_dir(PIN_LEFT, GPIO_IN);
  gpio_pull_up(PIN_LEFT);
  
  gpio_init(PIN_RIGHT);
  gpio_set_dir(PIN_RIGHT, GPIO_IN);
  gpio_pull_up(PIN_RIGHT);
  
  gpio_init(PIN_MODE);
  gpio_set_dir(PIN_MODE, GPIO_IN);
  gpio_pull_up(PIN_MODE);
  
  // led
  gpio_init(PIN_LED);
  gpio_set_dir(PIN_LED, GPIO_OUT);
  gpio_put(PIN_LED, 0);
}

bool read_button(uint8_t pin)
{
  return !gpio_get(pin);
}

void handle_mode_button(void)
{
  static bool mode_button_was_pressed = false;
  bool mode_button_pressed = read_button(PIN_MODE);
  if (mode_button_pressed && !mode_button_was_pressed) {
    remote_work_mode = !remote_work_mode;
    gpio_put(PIN_LED, remote_work_mode);
  }
  
  mode_button_was_pressed = mode_button_pressed;
}

int8_t calculate_speed(uint32_t press_duration)
{
  if (press_duration < SPEED_THRESHOLD_1) {
    return SPEED_1;
  } else if (press_duration < SPEED_THRESHOLD_2) {
    return SPEED_2;
  } else if (press_duration < SPEED_THRESHOLD_3) {
    return SPEED_3;
  } else {
    return SPEED_4;
  }
}

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void)
{
  blink_interval_ms = BLINK_MOUNTED;
}

// Invoked when device is unmounted
void tud_umount_cb(void)
{
  blink_interval_ms = BLINK_NOT_MOUNTED;
}

// Invoked when usb bus is suspended
void tud_suspend_cb(bool remote_wakeup_en)
{
  (void) remote_wakeup_en;
  blink_interval_ms = BLINK_SUSPENDED;
}

// Invoked when usb bus is resumed
void tud_resume_cb(void)
{
  blink_interval_ms = tud_mounted() ? BLINK_MOUNTED : BLINK_NOT_MOUNTED;
}

//--------------------------------------------------------------------+
// USB HID
//--------------------------------------------------------------------+

static void send_mouse_report(int8_t delta_x, int8_t delta_y)
{
  // skip if hid is not ready yet
  if (!tud_hid_ready()) return;
  
  // Send mouse report: no buttons, delta_x, delta_y, no scroll, no pan
  tud_hid_mouse_report(REPORT_ID_MOUSE, 0x00, delta_x, delta_y, 0, 0);
}

void hid_task(void)
{
  // Poll every 10ms
  const uint32_t interval_ms = 10;
  static uint32_t start_ms = 0;

  if (board_millis() - start_ms < interval_ms) return;
  start_ms += interval_ms;

  uint32_t current_time = board_millis();
  int8_t delta_x = 0, delta_y = 0;

  if (remote_work_mode) {
    


    float x_pos = CIRCLE_RADIUS * cos(circle_angle);
    float y_pos = CIRCLE_RADIUS * sin(circle_angle);

    float next_angle = circle_angle + CIRCLE_SPEED;
    float next_x = CIRCLE_RADIUS * cos(next_angle);
    float next_y = CIRCLE_RADIUS * sin(next_angle);
    

    delta_x = (int8_t)(next_x - x_pos);
    delta_y = (int8_t)(next_y - y_pos);
    
    circle_angle = next_angle;
    if (circle_angle > 2.0f * 3.14159f) {
      circle_angle = 0.0f;
    }
  } else {
    // regular mode
    bool buttons[4] = {
      read_button(PIN_UP),
      read_button(PIN_DOWN), 
      read_button(PIN_LEFT),
      read_button(PIN_RIGHT)
    };
    
    for (int i = 0; i < 4; i++) {
      if (buttons[i] && !button_was_pressed[i]) {
        button_press_time[i] = current_time;
        button_was_pressed[i] = true;
      } else if (!buttons[i] && button_was_pressed[i]) {

        button_was_pressed[i] = false;
      }
    }
    if (buttons[0]) {
      uint32_t duration = current_time - button_press_time[0];
      delta_y -= calculate_speed(duration);
    }
    if (buttons[1]) {
      uint32_t duration = current_time - button_press_time[1];
      delta_y += calculate_speed(duration);
    }
    if (buttons[2]) {
      uint32_t duration = current_time - button_press_time[2];
      delta_x -= calculate_speed(duration);
    }
    if (buttons[3]) {
      uint32_t duration = current_time - button_press_time[3];
      delta_x += calculate_speed(duration);
    }
  }

  if (delta_x != 0 || delta_y != 0) {
    send_mouse_report(delta_x, delta_y);
  }


  uint32_t btn = board_button_read();
  if (tud_suspended() && btn) {
    tud_remote_wakeup();
  }
}

// // Invoked when sent REPORT successfully to host
// // Application can use this to send the next report
// // Note: For composite reports, report[0] is report ID
// void tud_hid_report_complete_cb(uint8_t instance, uint8_t const* report, uint16_t len)
// {
//   (void) instance;
//   (void) len;

//   uint8_t next_report_id = report[0] + 1u;

//   if (next_report_id < REPORT_ID_COUNT)
//   {
//     send_hid_report(next_report_id, board_button_read());
//   }
// }

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen)
{
  // TODO not Implemented
  (void) instance;
  (void) report_id;
  (void) report_type;
  (void) buffer;
  (void) reqlen;

  return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize)
{
  (void) instance;

  if (report_type == HID_REPORT_TYPE_OUTPUT)
  {
    // Set keyboard LED e.g Capslock, Numlock etc...
    if (report_id == REPORT_ID_KEYBOARD)
    {
      // bufsize should be (at least) 1
      if ( bufsize < 1 ) return;

      uint8_t const kbd_leds = buffer[0];

      if (kbd_leds & KEYBOARD_LED_CAPSLOCK)
      {
        // Capslock On: disable blink, turn led on
        blink_interval_ms = 0;
        board_led_write(true);
      }else
      {
        // Caplocks Off: back to normal blink
        board_led_write(false);
        blink_interval_ms = BLINK_MOUNTED;
      }
    }
  }
}

//--------------------------------------------------------------------+
// BLINKING TASK
//--------------------------------------------------------------------+
void led_blinking_task(void)
{
  static uint32_t start_ms = 0;
  static bool led_state = false;

  // blink is disabled
  if (!blink_interval_ms) return;

  // Blink every interval ms
  if ( board_millis() - start_ms < blink_interval_ms) return; // not enough time
  start_ms += blink_interval_ms;

  board_led_write(led_state);
  led_state = 1 - led_state; // toggle
}