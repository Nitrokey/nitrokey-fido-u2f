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

 */

#include "app.h"
#include "atecc508a.h"
#include <string.h> // memset()
#include "bsp.h" // used in dump_config()
#include "i2c.h" // used in dump_config()

static uint8_t * binary_slot_configs =
						"\x83\x71"
						"\x81\x01"
						"\x83\x71"
						"\xC1\x01"
						"\x83\x71"
						"\x83\x71"
						"\x83\x71"
						"\xC1\x71"
						"\x01\x01"
						"\x83\x71"
						"\x83\x71"
						"\xC1\x71"
						"\x83\x71"
						"\x83\x71"
						"\x83\x71"
						"\x83\x71";

static uint8_t * binary_key_configs =
						"\x13\x00"
						"\x3C\x00"
						"\x13\x00"
						"\x3C\x00"
						"\x13\x00"
						"\x3C\x00"
						"\x13\x00"
						"\x3C\x00"
						"\x3C\x00"
						"\x3C\x00"
						"\x13\x00"
						"\x3C\x00"
						"\x13\x00"
						"\x3C\x00"
						"\x13\x00"
						"\x33\x00";

/**
 * Prints ATECC508A's configuration over serial line
 * Requires temporary buffer of size 40.
 */
void dump_config(uint8_t* buf)
{
#ifdef U2F_PRINT
	uint8_t i,j;
	uint16_t crc = 0;
	struct atecc_response res;
	uint8_t config_data[128];
	struct atecc_slot_config *c;
	struct atecc_key_config *d;

	//See 2.2 EEPROM Configuration Zone of ATECC508A Complete Data Sheet
	const int slot_config_offset = 20;
	const int key_config_offset = 96;
	const int slot_config_size = sizeof(struct atecc_slot_config);
	const int key_config_size = sizeof(struct atecc_key_config);


	u2f_prints("config dump:\r\n");
	for (i=0; i < 4; i++)
	{
		if ( atecc_send_recv(ATECC_CMD_READ,
				ATECC_RW_CONFIG | ATECC_RW_EXT, i << 3, NULL, 0,
				buf, 40, &res) != 0)
		{
			u2f_prints("read failed\r\n");
		}
		for(j = 0; j < res.len; j++)
		{
			crc = feed_crc(crc,res.buf[j]);
		}
		dump_hex(res.buf,res.len);
		memmove(config_data+i*32, res.buf, 32);
	}

	u2f_printx("current config crc:", 1,reverse_bits(crc));


#define _PRINT(x) u2f_printb(#x": ", 1, x)

	for (i=0; i<16; i++){
		u2f_printb("Slot config: ", 1, i);
		c = (struct atecc_slot_config*) (config_data + slot_config_offset + i * slot_config_size);
		u2f_prints("hex: "); dump_hex(c,2);
		_PRINT(c->writeconfig);
		_PRINT(c->writekey);
		_PRINT(c->secret);
		_PRINT(c->encread);
		_PRINT(c->limiteduse);
		_PRINT(c->nomac);
		_PRINT(c->readkey);


		u2f_printb("Key config: ", 1, i);
		d = (struct atecc_key_config *) (config_data + key_config_offset + i * key_config_size);
		u2f_prints("hex: "); dump_hex(d,2);

		_PRINT(d->x509id);
		_PRINT(d->rfu);
		_PRINT(d->intrusiondisable);
		_PRINT(d->authkey);
		_PRINT(d->reqauth);
		_PRINT(d->reqrandom);
		_PRINT(d->lockable);
		_PRINT(d->keytype);
		_PRINT(d->pubinfo);
		_PRINT(d->private);
	}

#undef _PRINT

#endif
}


#ifdef ATECC_SETUP_DEVICE

uint8_t atecc_setup_config(uint8_t* buf)
{
	uint8_t i;
	uint8_t errors = 0;
	uint8_t slot_arr[32];
	uint8_t key_arr[32];

	i = get_readable_config(slot_arr, sizeof(slot_arr), key_arr, sizeof(key_arr));
	if (i != 0) {
		while (1) ; // refuse to work with invalid configuration data, execute watchdog reset
	}

	// write configuration
	for (i = 0; i < 16; i++)
	{
		if ( atecc_write_eeprom(ATECC_EEPROM_SLOT(i), ATECC_EEPROM_SLOT_OFFSET(i), slot_arr+i*2, ATECC_EEPROM_SLOT_SIZE) != 0)
		{
			u2f_printb("1 atecc_write_eeprom failed ",1, i);
			errors++;
			break;
		}

		if ( atecc_write_eeprom(ATECC_EEPROM_KEY(i), ATECC_EEPROM_KEY_OFFSET(i), key_arr+i*2, ATECC_EEPROM_KEY_SIZE) != 0)
		{
			u2f_printb("2 atecc_write_eeprom failed " ,1,i);
			errors++;
			break;
		}
	}

	dump_config(buf);
	return errors;
}

uint8_t get_readable_config(uint8_t * out_slotconfig, uint8_t slotconfig_len,
							uint8_t * out_keyconfig,  uint8_t keyconfig_len){
	struct atecc_slot_config *c;
	struct atecc_key_config *d;

	struct atecc_slot_config slot_arr[16];
	struct atecc_key_config key_arr[16];

	uint8_t result;

	if (out_slotconfig != NULL && slotconfig_len != sizeof(slot_arr))
		return 3;

	if (out_keyconfig != NULL && keyconfig_len != sizeof(key_arr))
		return 4;


	memset(slot_arr, 0, sizeof(slot_arr));
	memset(key_arr, 0, sizeof(key_arr));

	c = slot_arr;
	d = key_arr;

	// Slot config = 0;
	// hex = 8371;
	c->writeconfig = 7;
	c->writekey = 1;
	c->secret = 1;
	c->encread = 0;
	c->limiteduse = 0;
	c->nomac = 0;
	c->readkey = 3;
	// Key config = 0;
	// hex = 1300;
	d->x509id = 0;
	d->rfu = 0;
	d->intrusiondisable = 0;
	d->authkey = 0;
	d->reqauth = 0;
	d->reqrandom = 0;
	d->lockable = 0;
	d->keytype = 4;
	d->pubinfo = 1;
	d->private = 1;

	c++; d++;
	// Slot config = 1;
	// hex = 8101;
	c->writeconfig = 0;
	c->writekey = 1;
	c->secret = 1;
	c->encread = 0;
	c->limiteduse = 0;
	c->nomac = 0;
	c->readkey = 1;
	// Key config = 1;
	// hex = 3c00;
	d->x509id = 0;
	d->rfu = 0;
	d->intrusiondisable = 0;
	d->authkey = 0;
	d->reqauth = 0;
	d->reqrandom = 0;
	d->lockable = 1;
	d->keytype = 7;
	d->pubinfo = 0;
	d->private = 0;

	c++; d++;
	// Slot config = 2;
	// hex = 8371;
	c->writeconfig = 7;
	c->writekey = 1;
	c->secret = 1;
	c->encread = 0;
	c->limiteduse = 0;
	c->nomac = 0;
	c->readkey = 3;
	// Key config = 2;
	// hex = 1300;
	d->x509id = 0;
	d->rfu = 0;
	d->intrusiondisable = 0;
	d->authkey = 0;
	d->reqauth = 0;
	d->reqrandom = 0;
	d->lockable = 0;
	d->keytype = 4;
	d->pubinfo = 1;
	d->private = 1;

	c++; d++;
	// Slot config = 3;
	// hex = c101;
	c->writeconfig = 0;
	c->writekey = 1;
	c->secret = 1;
	c->encread = 1;
	c->limiteduse = 0;
	c->nomac = 0;
	c->readkey = 1;
	// Key config = 3;
	// hex = 3c00;
	d->x509id = 0;
	d->rfu = 0;
	d->intrusiondisable = 0;
	d->authkey = 0;
	d->reqauth = 0;
	d->reqrandom = 0;
	d->lockable = 1;
	d->keytype = 7;
	d->pubinfo = 0;
	d->private = 0;

	c++; d++;
	// Slot config = 4;
	// hex = 8371;
	c->writeconfig = 7;
	c->writekey = 1;
	c->secret = 1;
	c->encread = 0;
	c->limiteduse = 0;
	c->nomac = 0;
	c->readkey = 3;
	// Key config = 4;
	// hex = 1300;
	d->x509id = 0;
	d->rfu = 0;
	d->intrusiondisable = 0;
	d->authkey = 0;
	d->reqauth = 0;
	d->reqrandom = 0;
	d->lockable = 0;
	d->keytype = 4;
	d->pubinfo = 1;
	d->private = 1;

	c++; d++;
	// Slot config = 5;
	// hex = 8371;
	c->writeconfig = 7;
	c->writekey = 1;
	c->secret = 1;
	c->encread = 0;
	c->limiteduse = 0;
	c->nomac = 0;
	c->readkey = 3;
	// Key config = 5;
	// hex = 3c00;
	d->x509id = 0;
	d->rfu = 0;
	d->intrusiondisable = 0;
	d->authkey = 0;
	d->reqauth = 0;
	d->reqrandom = 0;
	d->lockable = 1;
	d->keytype = 7;
	d->pubinfo = 0;
	d->private = 0;

	c++; d++;
	// Slot config = 6;
	// hex = 8371;
	c->writeconfig = 7;
	c->writekey = 1;
	c->secret = 1;
	c->encread = 0;
	c->limiteduse = 0;
	c->nomac = 0;
	c->readkey = 3;
	// Key config = 6;
	// hex = 1300;
	d->x509id = 0;
	d->rfu = 0;
	d->intrusiondisable = 0;
	d->authkey = 0;
	d->reqauth = 0;
	d->reqrandom = 0;
	d->lockable = 0;
	d->keytype = 4;
	d->pubinfo = 1;
	d->private = 1;

	c++; d++;
	// Slot config = 7;
	// hex = c171;
	c->writeconfig = 7;
	c->writekey = 1;
	c->secret = 1;
	c->encread = 1;
	c->limiteduse = 0;
	c->nomac = 0;
	c->readkey = 1;
	// Key config = 7;
	// hex = 3c00;
	d->x509id = 0;
	d->rfu = 0;
	d->intrusiondisable = 0;
	d->authkey = 0;
	d->reqauth = 0;
	d->reqrandom = 0;
	d->lockable = 1;
	d->keytype = 7;
	d->pubinfo = 0;
	d->private = 0;

	c++; d++;
	// Slot config = 8;
	// hex = 0101;
	c->writeconfig = 0;
	c->writekey = 1;
	c->secret = 0;
	c->encread = 0;
	c->limiteduse = 0;
	c->nomac = 0;
	c->readkey = 1;
	// Key config = 8;
	// hex = 3c00;
	d->x509id = 0;
	d->rfu = 0;
	d->intrusiondisable = 0;
	d->authkey = 0;
	d->reqauth = 0;
	d->reqrandom = 0;
	d->lockable = 1;
	d->keytype = 7;
	d->pubinfo = 0;
	d->private = 0;

	c++; d++;
	// Slot config = 9;
	// hex = 8371;
	c->writeconfig = 7;
	c->writekey = 1;
	c->secret = 1;
	c->encread = 0;
	c->limiteduse = 0;
	c->nomac = 0;
	c->readkey = 3;
	// Key config = 9;
	// hex = 3c00;
	d->x509id = 0;
	d->rfu = 0;
	d->intrusiondisable = 0;
	d->authkey = 0;
	d->reqauth = 0;
	d->reqrandom = 0;
	d->lockable = 1;
	d->keytype = 7;
	d->pubinfo = 0;
	d->private = 0;

	c++; d++;
	// Slot config = a;
	// hex = 8371;
	c->writeconfig = 7;
	c->writekey = 1;
	c->secret = 1;
	c->encread = 0;
	c->limiteduse = 0;
	c->nomac = 0;
	c->readkey = 3;
	// Key config = a;
	// hex = 1300;
	d->x509id = 0;
	d->rfu = 0;
	d->intrusiondisable = 0;
	d->authkey = 0;
	d->reqauth = 0;
	d->reqrandom = 0;
	d->lockable = 0;
	d->keytype = 4;
	d->pubinfo = 1;
	d->private = 1;

	c++; d++;
	// Slot config = b;
	// hex = c171;
	c->writeconfig = 7;
	c->writekey = 1;
	c->secret = 1;
	c->encread = 1;
	c->limiteduse = 0;
	c->nomac = 0;
	c->readkey = 1;
	// Key config = b;
	// hex = 3c00;
	d->x509id = 0;
	d->rfu = 0;
	d->intrusiondisable = 0;
	d->authkey = 0;
	d->reqauth = 0;
	d->reqrandom = 0;
	d->lockable = 1;
	d->keytype = 7;
	d->pubinfo = 0;
	d->private = 0;

	c++; d++;
	// Slot config = c;
	// hex = 8371;
	c->writeconfig = 7;
	c->writekey = 1;
	c->secret = 1;
	c->encread = 0;
	c->limiteduse = 0;
	c->nomac = 0;
	c->readkey = 3;
	// Key config = c;
	// hex = 1300;
	d->x509id = 0;
	d->rfu = 0;
	d->intrusiondisable = 0;
	d->authkey = 0;
	d->reqauth = 0;
	d->reqrandom = 0;
	d->lockable = 0;
	d->keytype = 4;
	d->pubinfo = 1;
	d->private = 1;

	c++; d++;
	// Slot config = d;
	// hex = 8371;
	c->writeconfig = 7;
	c->writekey = 1;
	c->secret = 1;
	c->encread = 0;
	c->limiteduse = 0;
	c->nomac = 0;
	c->readkey = 3;
	// Key config = d;
	// hex = 3c00;
	d->x509id = 0;
	d->rfu = 0;
	d->intrusiondisable = 0;
	d->authkey = 0;
	d->reqauth = 0;
	d->reqrandom = 0;
	d->lockable = 1;
	d->keytype = 7;
	d->pubinfo = 0;
	d->private = 0;

	c++; d++;
	// Slot config = e;
	// hex = 8371;
	c->writeconfig = 7;
	c->writekey = 1;
	c->secret = 1;
	c->encread = 0;
	c->limiteduse = 0;
	c->nomac = 0;
	c->readkey = 3;
	// Key config = e;
	// hex = 1300;
	d->x509id = 0;
	d->rfu = 0;
	d->intrusiondisable = 0;
	d->authkey = 0;
	d->reqauth = 0;
	d->reqrandom = 0;
	d->lockable = 0;
	d->keytype = 4;
	d->pubinfo = 1;
	d->private = 1;

	c++; d++;
	// Slot config = f;
	// hex = 8371;
	c->writeconfig = 7;
	c->writekey = 1;
	c->secret = 1;
	c->encread = 0;
	c->limiteduse = 0;
	c->nomac = 0;
	c->readkey = 3;
	// Key config = f;
	// hex = 3300;
	d->x509id = 0;
	d->rfu = 0;
	d->intrusiondisable = 0;
	d->authkey = 0;
	d->reqauth = 0;
	d->reqrandom = 0;
	d->lockable = 1;
	d->keytype = 4;
	d->pubinfo = 1;
	d->private = 1;

	if (out_keyconfig != NULL)
		memmove(out_keyconfig, key_arr, keyconfig_len);

	if (out_slotconfig != NULL)
		memmove(out_slotconfig, slot_arr, slotconfig_len);

	return 0;
}


#define MIN(a,b)	(((a)<(b))?(a):(b))
uint8_t compare_binary_readable_configs(uint8_t* out_config, uint8_t out_len){
	struct atecc_slot_config slot_arr[16];
	struct atecc_key_config key_arr[16];
	uint8_t result;

	result = get_readable_config(slot_arr, sizeof(slot_arr),
								key_arr, sizeof(key_arr));
	if (result != 0) return 3;

	result = sizeof(slot_arr);
	result = memcmp(slot_arr, binary_slot_configs, sizeof(slot_arr));
	if (result != 0) return 1;

	result = sizeof(key_arr);
	result = memcmp(key_arr, binary_key_configs, sizeof(key_arr));
	if (result != 0) return 2;

	if (out_config != NULL && out_len >= 0){
		memmove(out_config, slot_arr, MIN(sizeof(slot_arr), out_len) );
		if (out_len > sizeof(slot_arr)){
			out_len -= sizeof(slot_arr);
			memmove(out_config + sizeof(slot_arr), key_arr, MIN(sizeof(key_arr), out_len));
		}
	}

	return 0;
}
#undef MIN

#endif //ATECC_SETUP_DEVICE
