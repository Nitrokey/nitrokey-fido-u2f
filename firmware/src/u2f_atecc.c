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


static void gen_u2f_zero_tag(uint8_t * out_dst, uint8_t * appid, uint8_t * handle);

static struct u2f_hid_msg res;


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

static uint32_t last_button_cleared_time = 0;

void clear_button_press(){
	if (get_ms() - last_button_cleared_time < U2F_MS_CLEAR_BUTTON_PERIOD)
		return;
	last_button_cleared_time = get_ms();

#ifndef _PRODUCTION_RELEASE
	led_on();
#endif
	BUTTON_RESET_ON();
	do {
		u2f_delay(6); 				//6ms activation time + 105ms maximum sleep in NORMAL power mode
	} while (IS_BUTTON_PRESSED()); // Wait to release button
	BUTTON_RESET_OFF();
	led_off();
}

static int8_t _u2f_get_user_feedback(BUTTON_STATE_T target_button_state, bool blink)
{
	uint32_t t;
	uint8_t user_presence = 0;

	if (button_press_is_consumed())
		return 1;

	if (blink == true && led_is_blinking() == false)
		led_blink(10, 375);
	watchdog();

	t = get_ms();
	while(button_get_press_state() != target_button_state)	// Wait to push button
	{
		led_blink_manager();                               // Run led driver to ensure blinking
        button_manager();                                 // Run button driver
		if (get_ms() - t > U2F_MS_USER_INPUT_WAIT    // 3 secs elapsed without button press
				&& !button_press_in_progress())			// Button press has not been started
			break;                                    // Timeout
		u2f_delay(10);
		watchdog();
		break;
#ifdef FAKE_TOUCH
		if (get_ms() - t > 1010) break; //1212
#endif
		}

#ifndef FAKE_TOUCH
	if (button_get_press_state() == target_button_state)
#else //FAKE_TOUCH
	if (true)
#endif
	{
		// Button has been pushed in time
		user_presence = 1;
		button_press_set_consumed();
		led_off();
#ifdef SHOW_TOUCH_REGISTERED
		//show short confirming animation
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
		user_presence = 0;                                     // Return error code
	}


	return user_presence? 0 : 1;
}

int8_t u2f_get_user_feedback(){
	return _u2f_get_user_feedback(BST_PRESSED_REGISTERED, true);
}

int8_t u2f_get_user_feedback_extended_wipe(){
	return _u2f_get_user_feedback(BST_PRESSED_REGISTERED_EXT, false);
}


// FIXME unused appid
int8_t u2f_ecdsa_sign(uint8_t * out_dest, uint8_t * handle, uint8_t * appid)
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
	memmove(out_dest, res.buf, 64);
	return 0;
}


// bad if this gets interrupted
int8_t u2f_new_keypair(uint8_t * out_handle, uint8_t * appid, uint8_t * out_pubkey)
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

	memmove(out_handle, res.buf, 4);  // size of key handle must be 36

	memset(private_key,0,4);
	memmove(private_key+4, res_digest.buf, 32);

	eeprom_xor(EEPROM_DATA_RMASK, private_key+4, 32);

	watchdog();
	compute_key_hash(private_key, EEPROM_DATA_WMASK, U2F_TEMP_KEY_SLOT);
	memmove(out_handle+4, res_digest.buf, 32);  // size of key handle must be 36+28


	if ( atecc_privwrite(U2F_TEMP_KEY_SLOT, private_key, EEPROM_DATA_WMASK, out_handle+4) != 0)
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

	memmove(out_pubkey, res.buf, 64);

	// the + 28/U2F_KEY_HANDLE_ID_SIZE
	gen_u2f_zero_tag(out_handle + U2F_KEY_HANDLE_KEY_SIZE, appid, out_handle);

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

static void gen_u2f_zero_tag(uint8_t * out_dst, uint8_t * appid, uint8_t * handle)
{
	u2f_sha256_start(U2F_DEVICE_KEY_SLOT, ATECC_SHA_HMACSTART);

	u2f_sha256_update(handle,U2F_KEY_HANDLE_KEY_SIZE);

	eeprom_read(EEPROM_DATA_U2F_CONST, appdata.tmp, U2F_CONST_LENGTH);
	u2f_sha256_update(appdata.tmp,U2F_CONST_LENGTH);
	memset(appdata.tmp, 0, U2F_CONST_LENGTH);

	u2f_sha256_update(appid,32);

	u2f_sha256_finish();

	if (out_dst) memmove(out_dst, res_digest.buf, U2F_KEY_HANDLE_ID_SIZE);
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
