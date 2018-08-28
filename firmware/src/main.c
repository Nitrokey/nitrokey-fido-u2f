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
#include <SI_EFM8UB3_Register_Enums.h>

#include "InitDevice.h"
#include "app.h"
#include "i2c.h"
#include "gpio.h"
#include "atecc508a.h"
#include "eeprom.h"
#include "bsp.h"
#include "custom.h"
#include "u2f.h"
#include "tests.h"

#define ms_since(ms,num) (((uint16_t)get_ms() - (ms)) >= num ? ((ms=(uint16_t)get_ms())):0)


data struct APP_DATA appdata;

uint8_t error;
uint8_t state;


struct u2f_hid_msg * hid_msg;


static void init(struct APP_DATA* ap)
{

	u2f_hid_init();
	smb_init();
	atecc_idle();
#ifdef _SECURE_EEPROM
	eeprom_init();
#endif

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

	// ~800 ms interval watchdog
	WDTCN = 5;

	watchdog();
	init(&appdata);

	atecc_sleep();

#ifdef DISABLE_WATCHDOG
	IE_EA = 0;
	WDTCN = 0xDE;
	WDTCN = 0xAD;
#endif
	// Enable interrupts
	IE_EA = 1;
	watchdog();


	if (RSTSRC & RSTSRC_WDTRSF__SET)
	{
		//error = ERROR_DAMN_WATCHDOG;
		u2f_prints("r");
	}
	u2f_prints("U2F ZERO ==================================\r\n");

	run_tests();
	BUTTON_RESET_OFF();
	led_off();
	atecc_setup_init(appdata.tmp);

	led_blink(1, 0);                                   // Blink once after successful startup

	while (1) {
		watchdog();

        button_manager();
        led_blink_manager();
        #ifdef __BUTTON_TEST__
//        if (!LedBlinkNum) {
            if (button_get_press()) { led_on();  }
            else                    { led_off(); }
//        }
        #endif

		if (!USBD_EpIsBusy(EP1OUT) && !USBD_EpIsBusy(EP1IN) && state != APP_HID_MSG)
		{
			USBD_Read(EP1OUT, hidmsgbuf, sizeof(hidmsgbuf), true);
		}

		u2f_hid_check_timeouts();

		switch(state) {
			case APP_NOTHING: {}break;                     // Idle state:

			case APP_HID_MSG: {                            // HID msg received, pass to protocols:
#ifndef ATECC_SETUP_DEVICE
				struct CID* cid = NULL;
				cid = get_cid(hid_msg->cid);
				if (!cid->busy) {                          // There is no ongoing U2FHID transfer
					if (!custom_command(hid_msg)) {
						u2f_hid_request(hid_msg);
					}
				} else {
					u2f_hid_request(hid_msg);
				}
#else
				if (!custom_command(hid_msg)) {
					 u2f_hid_request(hid_msg);
				}
#endif
				if (state == APP_HID_MSG) {                // The USB msg doesnt ask a special app state
					state = APP_NOTHING;	               // We can go back to idle
				}
			}break;
		}

		watchdog();
		if(atecc_used){
			atecc_sleep();
			atecc_used = 0;
		}

		if (error)
		{
			u2f_printx("error: ", 1, (uint16_t)error);

#ifndef ON_ERROR_RESET_IMMEDIATELY

//			for (i=0; i<0x400;i++)                    // wipe ram
//			{
//				*(clear++) = 0x0;
//			}
			u2f_hid_set_len(2);
			i = 0x1300 + error;
			u2f_response_writeback(&i,2);
			watchdog();
#endif

#ifdef ON_ERROR_RESET_IMMEDIATELY
			u2f_delay(100);
			RSTSRC = RSTSRC_SWRSF__SET | RSTSRC_PORSF__SET;
#else

#ifdef U2F_BLINK_ERRORS
			LedBlink(LED_BLINK_NUM_INF, 50);
			// wait for watchdog to reset
			while(1)
			{
				led_blink_manager();
			}

#else //!U2F_BLINK_ERRORS
			// wait for watchdog to reset
			while(1)
				;
#endif //U2F_BLINK_ERRORS

#endif
		}
	}
}

