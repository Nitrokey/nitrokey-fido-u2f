/*
 * Copyright (c) 2016, Conor Patrick
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
 * main.c
 * 		This file contains the main loop of the application.
 * 		It listens for messages on USB and upon receiving a message,
 * 		it will pass it up to the U2F HID layer, implemented in u2f_hid.c.
 *
 */
#include "configuration.h"
#include <SI_EFM8UB3_Register_Enums.h>
#include <usb_serial.h>

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
#include "adc_0.h"
#include "cvd_hw.h"
#include "cvd.h"


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

#ifdef __ADC_TEST__

void AdcVref_1p65 (void) {
	uint8_t SFRPAGE_save = SFRPAGE;

	SFRPAGE = 0x00;
	REF0CN &= ~REF0CN_IREFLVL__BMASK;
	SFRPAGE = SFRPAGE_save;
}

void AdcVref_2p4 (void) {
	uint8_t SFRPAGE_save = SFRPAGE;

	SFRPAGE = 0x00;
	REF0CN &= ~REF0CN_IREFLVL__BMASK;
	REF0CN |= REF0CN_IREFLVL__2P4;
	SFRPAGE = SFRPAGE_save;
}

void AdcGain_0p5 (void) {
	uint8_t SFRPAGE_save = SFRPAGE;

	SFRPAGE = 0x00;
	ADC0CF &= ~ADC0CF_ADGN__BMASK;
	SFRPAGE = SFRPAGE_save;
}

void AdcGain_1 (void) {
	uint8_t SFRPAGE_save = SFRPAGE;

	SFRPAGE = 0x00;
	ADC0CF &= ~ADC0CF_ADGN__BMASK;
	ADC0CF |= ADC0CF_ADGN__GAIN_1;
	SFRPAGE = SFRPAGE_save;
}

volatile uint16_t adc1;                               // Uch = 1.8V (LDO), Vref=1.65V, Gain=0.5, should be ~558
volatile uint16_t adc2;                               // Uch = 1.8V (LDO), Vref=1.65V, Gain=1,   should be 1023
volatile uint16_t adc3;                               // Uch = 0V (GND),   Vref=1.65V, Gain=0.5, should be ~0                               // Vref=1.65V, Gain=0.5, should be ~558
volatile uint16_t adc4;                               // Uch = 0V (GND),   Vref=1.65V, Gain=1,   should be ~0
volatile uint16_t adc5;                               // Uch = 1.8V (LDO), Vref=2.4V,  Gain=0.5, should be ~384
volatile uint16_t adc6;                               // Uch = 1.8V (LDO), Vref=2.4V,  Gain=1,   should be ~767
volatile uint16_t adc7;                               // Uch = 0V (GND),   Vref=2.4V,  Gain=0.5, should be ~0
volatile uint16_t adc8;                               // Uch = 0V (GND),   Vref=2.4V,  Gain=1,   should be ~0


uint16_t AdcConv (void) {
	ADC0_startConversion();                           // Start conversion
	while(!ADC0_isConversionComplete());              // Wait for conversion
	return ADC0_getResult();
}


void AdcTest (void) {
   /* formula: adc = ADC_TOP_VALUE * (u_ch * gain) / u_vref */

   /* Vref = 1.65V */
   AdcVref_1p65();                                     // u_vref = 1.65V
   ADC0_setPositiveInput(ADC0_POSITIVE_INPUT_LDO_OUT); // u_ch = u_ldo = 1.8V

   AdcGain_0p5();                                      // gain = 0.5
   adc1 = AdcConv();                                   // adc =~ 558
   AdcGain_1();                                        // gain = 1
   adc2 = AdcConv();                                   // adc = 1023

   ADC0_setPositiveInput(ADC0_POSITIVE_INPUT_GND);     // u_ch = u_gnd = 0V
   AdcGain_0p5();                                      // gain = 0.5
   adc3 = AdcConv();                                   // adc =~ 0
   AdcGain_1();                                        // gain = 1
   adc4 = AdcConv();                                   // adc =~ 0

   /* Vref = 2.4V */
   AdcVref_2p4();                                      // u_vref = 2.4V
   ADC0_setPositiveInput(ADC0_POSITIVE_INPUT_LDO_OUT); // u_ch = u_ldo = 1.8V

   AdcGain_0p5();                                      // gain = 0.5
   adc5 = AdcConv();                                   // adc =~ 384
   AdcGain_1();                                        // gain = 1
   adc6 = AdcConv();                                   // adc =~ 767

   ADC0_setPositiveInput(ADC0_POSITIVE_INPUT_GND);
   AdcGain_0p5();                                      // gain = 0.5
   adc7 = AdcConv();                                   // adc =~ 0
   AdcGain_1();                                        // gain = 1
   adc8 = AdcConv();                                   // adc =~ 0
}

#else
#define AdcTest()       ;
#endif

static uint8_t TriggerCnt = 0;
int16_t main(void) {
	data uint8_t xdata * clear = 0;
	uint16_t i;

	configuration_read();
	// initialize USB
	update_USB_serial();
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

	get_serial_num();

	if (RSTSRC & RSTSRC_WDTRSF__SET)
	{
		u2f_prints("r");
	}
	u2f_prints("U2F ZERO ==================================\r\n");

#ifndef _PRODUCTION_RELEASE
	run_tests();
#endif
	BUTTON_RESET_OFF();
	led_off();

	led_blink(1, 0);                                   // Blink once after successful startup

	led_blink(100, 500);
	while (1) {
		watchdog();

		//AdcTest();

		//if (CvdSample() > CVD_THRESHOLD) { led_on();  }
		//else                             { led_off(); }
		if (CvdSample() > CVD_THRESHOLD) {
			if (TriggerCnt < 255) {
				TriggerCnt++;
			}
		}


		clear_button_press();
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
			if (USBD_Read(EP1OUT, hidmsgbuf, sizeof(hidmsgbuf), true) != USB_STATUS_OK){
				set_app_error(ERROR_USB_WRITE);
			}
		}

		u2f_hid_check_timeouts();

		switch(state) {
			case APP_NOTHING: {}break;                     // Idle state:

			case APP_HID_MSG: {                            // HID msg received, pass to protocols:
#ifndef ATECC_SETUP_DEVICE
				struct CID* cid = NULL;
				cid = get_cid(hid_msg->cid);
				if (cid == NULL || !cid->busy) {                          // There is no ongoing U2FHID transfer
					if (!custom_command(hid_msg)) {
						u2f_hid_request(hid_msg);
					}
				} else {
					u2f_hid_request(hid_msg);
				}
#else //!ATECC_SETUP_DEVICE
				if (!custom_command(hid_msg)) {
					 u2f_hid_request(hid_msg);
				}
#endif //ATECC_SETUP_DEVICE
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

			clear = 0;
			for (i=0; i<2048; i++)                    // wipe ram
			{
				if (clear == &i)
					continue;
				*(clear++) = 0;
			}

#ifdef ON_ERROR_RESET_IMMEDIATELY
			u2f_delay(100);
			RSTSRC = RSTSRC_SWRSF__SET | RSTSRC_PORSF__SET;
#endif

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

		}
	}
}

