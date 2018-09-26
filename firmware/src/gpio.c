/*
 * Copyright (c) 2018, Nitrokey UG
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 *
 * gpio.c
 * 		This file contains the GPIO access functions.
 * 		It provides abstractions for access to the LED as well as the user button.
 *
 */
#include <SI_EFM8UB3_Register_Enums.h>

#include "bsp.h"
#include "app.h"
#include "gpio.h"

#define ms_since(ms,num) (((uint16_t)get_ms() - (ms)) >= num ? ((ms=(uint16_t)get_ms())):0)

data  uint32_t        button_press_t;                   // Timer for TaskButton() timings
data  BUTTON_STATE_T  button_state = BST_INITIALIZING;    // Holds the actual registered logical state of the button

static data uint32_t  led_blink_tim;                    // Timer for TaskLedBlink() timings
static data uint16_t  led_blink_period_t;                // Period time register
static data uint8_t   led_blink_num;                    // Blink number counter, also an indicator if blinking is on

static data uint32_t  button_manager_start_t = 0;

void button_manager (void) {                          // Requires at least a 750ms long button press to register a valid user button press

	if (button_state == BST_INITIALIZING){
		if (button_manager_start_t == 0){
			button_manager_start_t = get_ms();
			return;
		}
		if (get_ms() - button_manager_start_t <= U2F_MS_INIT_BUTTON_PERIOD){
			return;
		}
		button_state = BST_INITIALIZING_READY_TO_CLEAR;
	}
	if (button_state == BST_INITIALIZING_READY_TO_CLEAR){
		return;
	}

	if (IS_BUTTON_PRESSED()) {                        // Button's physical state: pressed
		switch (button_state) {                        // Handle press phase
		    case BST_UNPRESSED: {                     // It happened at this moment
				button_state  = BST_PRESSED_RECENTLY;  // Update button state
				button_press_t = get_ms();              // Start measure press time
		    }break;
		    case BST_PRESSED_RECENTLY: {              // Button is already pressed, press time measurement is ongoing
				if (get_ms() - button_press_t >= BUTTON_MIN_PRESS_T_MS) { // Press time reached the critical value to register a valid user touch
				    button_state = BST_PRESSED_REGISTERED; // Update button state
				}
		    }break;
		    case BST_PRESSED_CONSUMED:
		    	break;
		    case BST_PRESSED_REGISTERED:
				if (get_ms() - button_press_t >= BUTTON_MAX_PRESS_T_MS) {
					button_state = BST_PRESSED_REGISTERED_TRANSIENT;
				}
				break;
		    case BST_PRESSED_REGISTERED_TRANSIENT:
		    	if (get_ms() - button_press_t >= BUTTON_MIN_PRESS_T_MS_EXT) {
					button_state = BST_PRESSED_REGISTERED_EXT;
				}
		    	break;
		    default:
		    	break;
		}
	} else {                                          // Button is unprssed
		button_state = BST_UNPRESSED;                  // Update button state
	}
}

uint8_t button_get_press (void) {
	return ((button_state == BST_PRESSED_REGISTERED)? 1 : 0);
}

BUTTON_STATE_T button_get_press_state (void) {
	return button_state;
}

uint8_t button_get_press_extended (void) {
	return ((button_state == BST_PRESSED_REGISTERED_EXT)? 1 : 0);
}

uint8_t button_press_in_progress(void){
	return ((button_state == BST_PRESSED_RECENTLY ||
			button_state == BST_PRESSED_REGISTERED)? 1 : 0);
}

void button_press_set_consumed(void){
	button_state = BST_PRESSED_CONSUMED;
}

uint8_t button_press_is_consumed(void){
	return ((button_state == BST_PRESSED_CONSUMED)? 1 : 0);
}


void led_on (void) {
	led_blink_num = 0;                                  // Stop ongoing blinking
	LED_ON();                                         // LED physical state -> ON
}

void led_off (void) {
	led_blink_num = 0;                                  // Stop ongoing blinking
	LED_OFF();                                        // LED physical state -> OFF
}

bool led_is_blinking(void){
	return led_blink_num != 0;
}

void led_blink (uint8_t blink_num, uint16_t period_t) {
	led_blink_num     	= blink_num;
	led_blink_period_t 	= period_t;
	led_blink_tim     	= get_ms();
    LED_ON();
}

void led_blink_manager (void) {
	if (led_blink_num) {                                     // LED blinking is on
		if (IS_LED_ON()) {                                 // ON state
			if (get_ms() - led_blink_tim >= LED_BLINK_T_ON) { // ON time expired
				LED_OFF();                                 // LED physical state -> OFF
				if (led_blink_num) {                         // It isnt the last blink round: initialize OFF state:
					led_blink_tim   = get_ms();		       // Init OFF timer
					if (led_blink_num != LED_BLINK_NUM_INF) {              // Not endless blinking:
						led_blink_num--;                     // Update the remaining blink num
					}
				}
			}
		} else {                                           // OFF state
			if (get_ms() - led_blink_tim >= LED_BLINK_T_OFF) { // OFF time expired
				LED_ON();                                  // LED physical state -> ON
				led_blink_tim   = get_ms();		           // Init ON timer
			}
		}
	}
}

static void set_button_cleared(){
	button_state = BST_UNPRESSED;
}


static uint32_t last_button_cleared_time = 0;

void clear_button_press(){
	// do not clear if enough time has not passed, unless button is ready to be cleared
	if (button_get_press_state() != BST_INITIALIZING_READY_TO_CLEAR
			&& (get_ms() - last_button_cleared_time < U2F_MS_CLEAR_BUTTON_PERIOD) )
		return;
	// do not clear, when:
	if (button_get_press_state() == BST_INITIALIZING			// button is not ready for clear yet
			|| button_get_press_state() == BST_PRESSED_RECENTLY	// button is pressed by the user
			|| button_get_press_state() == BST_PRESSED_CONSUMED	// button is pressed by the user
			){
		return;
	}
	last_button_cleared_time = get_ms();

#ifndef _PRODUCTION_RELEASE
	led_on();
#endif
	BUTTON_RESET_ON();
	do {
		u2f_delay(6); 				//6ms activation time + 105ms maximum sleep in NORMAL power mode
	} while (IS_BUTTON_PRESSED()); // Wait to release button
	BUTTON_RESET_OFF();
#ifndef _PRODUCTION_RELEASE
	led_off();
#endif

	if (button_get_press_state() == BST_INITIALIZING_READY_TO_CLEAR){
		set_button_cleared();
	}

}
