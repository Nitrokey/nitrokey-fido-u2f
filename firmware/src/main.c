/*
 * Copyright (c) 2016, Conor Patrick
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
 * main.c
 * 		This file contains the main loop of the application.
 * 		It listens for messages on USB and upon receiving a message,
 * 		it will pass it up to the U2F HID layer, implemented in u2f_hid.c.
 *
 */
#include <SI_EFM8UB1_Register_Enums.h>

#include "InitDevice.h"
#include "app.h"
#include "i2c.h"
#include "atecc508a.h"
#include "eeprom.h"
#include "bsp.h"
#include "custom.h"
#include "u2f.h"
#include "tests.h"

#define ms_since(ms,num) (((uint16_t)get_ms() - (ms)) >= num ? ((ms=(uint16_t)get_ms())):0)

typedef enum {
	BST_UNPRESSED,
	BST_PRESSED_RECENTLY,
	BST_PRESSED_REGISTERED,

	BST_MAX_NUM
} BUTTON_STATE_T;


data struct APP_DATA appdata;

uint8_t error;
uint8_t state;

data  uint32_t        ButtonPressT;                   // Timer for TaskButton() timings
data  BUTTON_STATE_T  ButtonState;                    // Holds the actual registered logical state of the button

static data uint32_t  LedBlinkTmr;                    // Timer for TaskLedBlink() timings
static data uint16_t  LedBlinkPeriodT;                // Period time register
static data uint8_t   LedBlinkNum;                    // Blink number counter, also an indicator if blinking is on

struct u2f_hid_msg * hid_msg;


static void init(struct APP_DATA* ap)
{

	u2f_hid_init();
	smb_init();
	atecc_idle();
	eeprom_init();

	state = APP_NOTHING;
	error = ERROR_NOTHING;
}

void set_app_error(APP_ERROR_CODE ec)
{
	error = ec;
}

uint8_t get_app_error()
{
	return error;
}

uint8_t get_app_state()
{
	return state;
}


void set_app_u2f_hid_msg(struct u2f_hid_msg * msg )
{
	state = APP_HID_MSG;
	hid_msg = msg;
}

int16_t main(void) {
	uint16_t ms_heart;
	uint16_t ms_wink;
	data uint8_t xdata * clear = 0;
	uint16_t i;
    #ifdef U2F_BLINK_ERRORS
	uint16_t ii;
    #endif

	enter_DefaultMode_from_RESET();

	// ~200 ms interval watchdog
	WDTCN = 4;

	watchdog();
	init(&appdata);

	// Enable interrupts
	IE_EA = 1;
	watchdog();



	if (RSTSRC & RSTSRC_WDTRSF__SET)
	{
		//error = ERROR_DAMN_WATCHDOG;
		u2f_prints("r");
	}
	u2f_prints("U2F ZERO\r\n");

	run_tests();
	ButtonState = BST_UNPRESSED;
	BUTTON_RESET_OFF();
	LedOff();
	atecc_setup_init(appdata.tmp);

	LedBlink(1, 0);                                   // Blink once after successful startup

	while (1) {
		watchdog();

        TaskButton();
        TaskLedBlink();
        #ifdef __BUTTON_TEST__
        if (!LedBlinkNum) {
            if (IsButtonPressed()) { LedOn();  }
            else                   { LedOff(); }
        }
        #endif

		if (!USBD_EpIsBusy(EP1OUT) && !USBD_EpIsBusy(EP1IN) && state != APP_HID_MSG)
		{
			USBD_Read(EP1OUT, hidmsgbuf, sizeof(hidmsgbuf), true);
		}

		u2f_hid_check_timeouts();

		switch(state) {
			case APP_NOTHING: {}break;                     // Idle state:

			case APP_HID_MSG: {                            // HID msg received, pass to protocols:
				struct CID* cid = NULL;

				cid = get_cid(hid_msg->cid);
				if (!cid->busy) {                          // There is no ongoing U2FHID transfer
					if (!custom_command(hid_msg)) {
						u2f_hid_request(hid_msg);
					}
				} else {
					u2f_hid_request(hid_msg);
				}

				if (state == APP_HID_MSG) {                // The USB msg doesnt ask a special app state
					state = APP_NOTHING;	               // We can go back to idle
				}
			}break;
		}

		if (error)
		{
			u2f_printx("error: ", 1, (uint16_t)error);
#ifdef U2F_BLINK_ERRORS
			for (ms_grad=0; ms_grad < 8; ms_grad++)
			{
				if (error & (1<<ms_grad))
				{
					rgb_hex(U2F_DEFAULT_COLOR_INPUT_SUCCESS);
				}
				else
				{
					rgb_hex(U2F_DEFAULT_COLOR_ERROR);
				}
				u2f_delay(400);
				rgb_hex(0);
				u2f_delay(100);

			}
#else
			//LedBlink(LED_BLINK_NUM_INF, 375);       // Blink wont work because of the following
			for (i=0; i<0x400;i++)                    // wipe ram
			{
				*(clear++) = 0x0;
				watchdog();
			}
#endif

			// wait for watchdog to reset
			while(1)
				;
		}
	}
}


void TaskButton (void) {                              // Requires at least a 750ms long button press to register a valid user button press
	if (IS_BUTTON_PRESSED()) {                        // Button's physical state: pressed
		switch (ButtonState) {                        // Handle press phase
		    case BST_UNPRESSED: {                     // It happened at this moment
				ButtonState  = BST_PRESSED_RECENTLY;  // Update button state
				ButtonPressT = get_ms();              // Start measure press time
		    }break;
		    case BST_PRESSED_RECENTLY: {              // Button is already pressed, press time measurement is ongoing
				if (get_ms() - ButtonPressT >= BUTTON_MIN_PRESS_T_MS) { // Press time reached the critical value to register a valid user touch
				    ButtonState = BST_PRESSED_REGISTERED; // Update button state
				}
		    }break;
		    default: {}break;
		}
	} else {                                          // Button is unprssed
		ButtonState = BST_UNPRESSED;                  // Update button state
	}
}

uint8_t IsButtonPressed (void) {
	return ((ButtonState == BST_PRESSED_REGISTERED)? 1 : 0);
}


void LedOn (void) {
	LedBlinkNum = 0;                                  // Stop ongoing blinking
	LED_ON();                                         // LED physical state -> ON
}

void LedOff (void) {
	LedBlinkNum = 0;                                  // Stop ongoing blinking
	LED_OFF();                                        // LED physical state -> OFF
}

void LedBlink (uint8_t blink_num, uint16_t period_t) {
	LedBlinkNum     = blink_num;
	LedBlinkPeriodT = period_t;
	LedBlinkTmr     = get_ms();
    LED_ON();
}

void TaskLedBlink (void) {
	if (LedBlinkNum) {                                     // LED blinking is on
		if (IS_LED_ON()) {                                 // ON state
			if (get_ms() - LedBlinkTmr >= LED_BLINK_T_ON) { // ON time expired
				LED_OFF();                                 // LED physical state -> OFF
				if (LedBlinkNum) {                         // It isnt the last blink round: initialize OFF state:
					LedBlinkTmr   = get_ms();		       // Init OFF timer
					if (LedBlinkNum != 255) {              // Not endless blinking:
						LedBlinkNum--;                     // Update the remaining blink num
					}
				}
			}
		} else {                                           // OFF state
			if (get_ms() - LedBlinkTmr >= LED_BLINK_T_OFF) { // OFF time expired
				LED_ON();                                  // LED physical state -> ON
				LedBlinkTmr   = get_ms();		           // Init ON timer
			}
		}
	}
}
