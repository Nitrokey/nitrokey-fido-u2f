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

 */

#ifndef APP_H_
#define APP_H_

#include <SI_EFM8UB3_Register_Enums.h>
#include <stdarg.h>
#include "u2f_hid.h"


// hw settings
#define BUTTON_MIN_PRESS_T_MS    750

#define LED_BLINK_T_ON           100                                 // ms
#define LED_BLINK_T_OFF          (led_blink_period_t - LED_BLINK_T_ON)  // ms
#define LED_BLINK_NUM_INF        255

// application settings
#define U2F_ATTESTATION_KEY_SLOT	15
#define U2F_MASTER_KEY_SLOT			1
#define U2F_TEMP_KEY_SLOT			2
#define U2F_WKEY_KEY_SLOT			1
#define U2F_DEVICE_KEY_SLOT			5

// Comment these out to fit firmware with a bootloader.
#define U2F_SUPPORT_WINK
#define U2F_SUPPORT_HID_LOCK
#define U2F_SUPPORT_RNG_CUSTOM
#define U2F_SUPPORT_SEED_CUSTOM

#define SHOW_TOUCH_REGISTERED

//#define DISABLE_WATCHDOG
//#define FAKE_TOUCH

// Uncomment this to make configuration firmware (stage 1 firmware)
#define ATECC_SETUP_DEVICE

// Uncomment to make a production firmware release, with selected flags
//#define _PRODUCTION_RELEASE

// Touch button test function
//#define __BUTTON_TEST__                             // Button drives directly the LED. Minimal required press time is determined by BUTTON_MIN_PRESS_T_MS

//#define U2F_PRINT
//#define U2F_BLINK_ERRORS

#ifdef _PRODUCTION_RELEASE
	#undef FAKE_TOUCH
	#undef SHOW_TOUCH_REGISTERED
	#undef DISABLE_WATCHDOG
	#undef U2F_PRINT
	#undef U2F_BLINK_ERRORS
	#undef __BUTTON_TEST__
	#undef U2F_USING_BOOTLOADER
	#ifndef ATECC_SETUP_DEVICE
		#define _SECURE_EEPROM
	#endif
#endif


typedef enum
{
	APP_NOTHING = 0,
	APP_HID_MSG,
	APP_WINK,
	_APP_WINK,
}
APP_STATE;


typedef enum
{
	ERROR_NOTHING = 0,
	ERROR_I2C_ERRORS_EXCEEDED = 2,
	ERROR_READ_TRUNCATED = 6,
	ERROR_ATECC_VERIFY = 0x01,
	ERROR_ATECC_PARSE = 0x03,
	ERROR_ATECC_FAULT = 0x05,
	ERROR_ATECC_EXECUTION = 0x0f,
	ERROR_ATECC_WAKE = 0x11,
	ERROR_ATECC_WATCHDOG = 0xee,
	ERROR_ATECC_CRC = 0xff,
	ERROR_I2C_CRC = 0x15,
	ERROR_SEQ_EXCEEDED = 0x12,
	ERROR_BAD_KEY_STORE = 0x13,
	ERROR_USB_WRITE = 0x14,
	ERROR_I2C_BAD_LEN = 0x16,
	ERROR_HID_BUFFER_FULL = 0x17,
	ERROR_HID_INVALID_CMD = 0x18,
	ERROR_DAMN_WATCHDOG = 0x19,
	ERROR_OUT_OF_CIDS = 0x20,
	ERROR_I2C_RESTART = 0x21,
}
APP_ERROR_CODE;

struct APP_DATA
{
	// must be at least 70 bytes
	uint8_t tmp[70];
};

#define U2F_CONFIG_GET_SERIAL_NUM		0x80
#define	U2F_CONFIG_IS_BUILD				0x81
#define U2F_CONFIG_IS_CONFIGURED		0x82
#define U2F_CONFIG_LOCK					0x83
//#define U2F_CONFIG_GENKEY				0x84
#define U2F_CONFIG_LOAD_TRANS_KEY		0x85
#define U2F_CONFIG_LOAD_WRITE_KEY		0x86
#define U2F_CONFIG_LOAD_ATTEST_KEY		0x87
#define U2F_CONFIG_BOOTLOADER			0x88
#define U2F_CONFIG_BOOTLOADER_DESTROY	0x89
#define U2F_CONFIG_ATECC_PASSTHROUGH	0x8a
#define U2F_CONFIG_LOAD_RMASK_KEY		0x8b
#define U2F_CONFIG_GEN_DEVICE_KEY		0x8c
#define U2F_CONFIG_GET_SLOTS_FINGERPRINTS	0x8d
#define U2F_CONFIG_TEST_CONFIG			0x8e

#ifdef ATECC_SETUP_DEVICE
// 1 page - 64 bytes
extern struct DevConf{
	uint8_t RMASK[36];
	uint8_t WMASK[36];
} device_configuration;
#endif

struct config_msg
{
	uint8_t cmd;
	uint8_t buf[HID_PACKET_SIZE-1];
};

extern uint8_t hidmsgbuf[64];
extern data struct APP_DATA appdata;

void set_app_u2f_hid_msg(struct u2f_hid_msg * msg );

void set_app_error(APP_ERROR_CODE ec);

uint8_t get_app_error();

uint8_t get_app_state();

void set_app_state(APP_STATE s);



#ifdef ATECC_SETUP_DEVICE

#include "atecc508a.h"

void atecc_setup_device(struct config_msg * msg);
void atecc_setup_init(uint8_t * buf);


void u2f_config_request();

#define U2F_HID_DISABLE
#define U2F_DISABLE
#define u2f_hid_init(x)
#define u2f_hid_request(x)	atecc_setup_device((struct config_msg*)x)
#define u2f_hid_set_len(x)
#define u2f_hid_flush(x)
#define u2f_hid_writeback(x)
#define u2f_hid_check_timeouts(x)

#else

#define atecc_setup_device(x)
#define atecc_setup_init(x)
#endif


#endif /* APP_H_ */
