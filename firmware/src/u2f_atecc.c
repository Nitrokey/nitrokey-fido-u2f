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
 *
 * u2f_atecc.c
 * 		platform specific functionality for implementing U2F
 *
 */

#include "app.h"

#undef U2F_DISABLE
#ifndef U2F_DISABLE
#include "bsp.h"
#include "gpio.h"
#include "u2f.h"
#include "u2f_hid.h"
#include "eeprom.h"
#include "atecc508a.h"


static void gen_u2f_zero_tag(uint8_t * dst, uint8_t * appid, uint8_t * handle);

static struct u2f_hid_msg res;
static uint8_t* resbuf = (uint8_t*)&res;
static uint8_t resseq = 0;
static uint8_t serious = 0;


void u2f_response_writeback(uint8_t * buf, uint16_t len)
{
	u2f_hid_writeback(buf, len);
}

void u2f_response_flush()
{
	watchdog();
	u2f_hid_flush();
}

void u2f_response_start()
{
	watchdog();
}

void clear_button_press(){
	led_on();
	BUTTON_RESET_ON();
	do {
		watchdog();
		u2f_delay(6); 				//6ms activation time + 105ms maximum sleep in NORMAL power mode
	} while (IS_BUTTON_PRESSED()); // Wait to release button
	BUTTON_RESET_OFF();
	led_off();
}

int8_t u2f_get_user_feedback()
{
	uint32_t t;
	uint8_t user_presence = 0;

	clear_button_press();

	led_blink(LED_BLINK_NUM_INF, 375);
	watchdog();

	t = get_ms();
	while(button_get_press() == 0)                         // Wait to push button
	{
		led_blink_manager();                               // Run led driver to ensure blinking
        button_manager();                                 // Run button driver
		if (get_ms() - t > U2F_MS_USER_INPUT_WAIT)    // 3 secs elapsed without button press
			break;                                    // Timeout
		u2f_delay(10);
		watchdog();
#ifdef FAKE_TOUCH
		if (get_ms() - t > 1010) break; //1212
#endif
		}

#ifndef FAKE_TOUCH
	if (button_get_press() == 1)
#else //FAKE_TOUCH
	if (true)
#endif
	{
		// Button has been pushed in time
		user_presence = 1;
		led_off();
#ifdef SHOW_TOUCH_REGISTERED
		t = get_ms();
		while(get_ms() - t < 110){
			led_on();
			u2f_delay(12);
			led_off();
			u2f_delay(25);
		}
		led_off();
#endif
	} else {                                          // Button hasnt been pushed within the timeout
		led_off();
		user_presence = 0;                                     // Return error code
	}

	clear_button_press();

	return user_presence? 0 : 1;
}


int8_t u2f_ecdsa_sign(uint8_t * dest, uint8_t * handle, uint8_t * appid)
{
	struct atecc_response res;
	uint16_t slot = U2F_TEMP_KEY_SLOT;
	if (handle == U2F_ATTESTATION_HANDLE)
	{
		slot = U2F_ATTESTATION_KEY_SLOT;
	}

	if( atecc_send_recv(ATECC_CMD_SIGN,
			ATECC_SIGN_EXTERNAL, slot, NULL, 0,
			appdata.tmp, 70, &res) != 0)
	{
		return -1;
	}
	memmove(dest, res.buf, 64);
	return 0;
}


// bad if this gets interrupted
int8_t u2f_new_keypair(uint8_t * handle, uint8_t * appid, uint8_t * pubkey)
{
	struct atecc_response res;
	uint8_t private_key[36];
	int i;

	watchdog();

	if (atecc_send_recv(ATECC_CMD_RNG,ATECC_RNG_P1,ATECC_RNG_P2,
		NULL, 0,
		appdata.tmp,
		sizeof(appdata.tmp), &res) != 0 )
	{
		return -1; //U2F_SW_CUSTOM_RNG_GENERATION
	}

	u2f_sha256_start(U2F_DEVICE_KEY_SLOT, ATECC_SHA_HMACSTART);
	u2f_sha256_update(appid,32);
	u2f_sha256_update(res.buf,4);
	u2f_sha256_finish();

	memmove(handle, res.buf, 4);  // size of key handle must be 36

	memset(private_key,0,4);
	memmove(private_key+4, res_digest.buf, 32);

	eeprom_xor(EEPROM_DATA_RMASK, private_key+4, 32);

	watchdog();
	compute_key_hash(private_key, EEPROM_DATA_WMASK, U2F_TEMP_KEY_SLOT);
	memmove(handle+4, res_digest.buf, 32);  // size of key handle must be 36+28


	if ( atecc_privwrite(U2F_TEMP_KEY_SLOT, private_key, EEPROM_DATA_WMASK, handle+4) != 0)
	{
		return -2; // U2F_SW_CUSTOM_PRIVWRITE
	}

	memset(private_key,0,36);

	if ( atecc_send_recv(ATECC_CMD_GENKEY,
			ATECC_GENKEY_PUBLIC, U2F_TEMP_KEY_SLOT, NULL, 0,
			appdata.tmp, 70, &res) != 0)
	{
		return -3; // U2F_SW_CUSTOM_GENKEY
	}

	memmove(pubkey, res.buf, 64);

	// the + 28/U2F_KEY_HANDLE_ID_SIZE
	gen_u2f_zero_tag(handle + U2F_KEY_HANDLE_KEY_SIZE, appid, handle);

	return 0;
}

int8_t u2f_load_key(uint8_t * handle, uint8_t * appid)
{
	uint8_t private_key[36];

	watchdog();
	u2f_sha256_start(U2F_DEVICE_KEY_SLOT, ATECC_SHA_HMACSTART);
	u2f_sha256_update(appid,32);
	u2f_sha256_update(handle,4);
	u2f_sha256_finish();

	memset(private_key,0,4);
	memmove(private_key+4, res_digest.buf, 32);

	eeprom_xor(EEPROM_DATA_RMASK, private_key+4, 32);

	return atecc_privwrite(U2F_TEMP_KEY_SLOT, private_key, EEPROM_DATA_WMASK, handle+4);
}

static void gen_u2f_zero_tag(uint8_t * dst, uint8_t * appid, uint8_t * handle)
{
	u2f_sha256_start(U2F_DEVICE_KEY_SLOT, ATECC_SHA_HMACSTART);

	u2f_sha256_update(handle,U2F_KEY_HANDLE_KEY_SIZE);

	eeprom_read(EEPROM_DATA_U2F_CONST, appdata.tmp, U2F_CONST_LENGTH);
	u2f_sha256_update(appdata.tmp,U2F_CONST_LENGTH);
	memset(appdata.tmp, 0, U2F_CONST_LENGTH);

	u2f_sha256_update(appid,32);

	u2f_sha256_finish();

	if (dst) memmove(dst, res_digest.buf, U2F_KEY_HANDLE_ID_SIZE);
}

int8_t u2f_appid_eq(uint8_t * handle, uint8_t * appid)
{
	gen_u2f_zero_tag(NULL,appid, handle);
	return memcmp(handle+U2F_KEY_HANDLE_KEY_SIZE, res_digest.buf, U2F_KEY_HANDLE_ID_SIZE);
}

uint32_t u2f_count()
{
	struct atecc_response res;
	atecc_send_recv(ATECC_CMD_COUNTER,
			ATECC_COUNTER_INC, ATECC_COUNTER0,NULL,0,
			appdata.tmp, sizeof(appdata.tmp), &res);
	return le32toh(*(uint32_t*)res.buf);
}

extern uint16_t __attest_size;
extern code char __attest[];

uint8_t * u2f_get_attestation_cert()
{
	return __attest;
}

uint16_t u2f_attestation_cert_size()
{
	return __attest_size;
}

void set_response_length(uint16_t len)
{
	u2f_hid_set_len(len);
}

#endif
