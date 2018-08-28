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

#include "atecc508a.h"
#include <string.h>

uint8_t get_readable_config(){

	uint8_t * slotconfig = 		 "\x83\x71"
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

	uint8_t * keyconfig = 		"\x13\x00"
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


	struct atecc_slot_config *c;
	struct atecc_key_config *d;

	struct atecc_slot_config slot_arr[16];
	struct atecc_key_config key_arr[16];

	uint8_t result;

	memset(slot_arr, 0, sizeof(slot_arr));
	memset(key_arr, 0, sizeof(key_arr));

	c = slot_arr;
	d = key_arr;

#define READABLE_CONFIG
#ifdef READABLE_CONFIG

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

#endif

	result = memcmp(slot_arr, slotconfig, sizeof(slot_arr));
	if (result != 0) return 1;

	result = memcmp(key_arr, keyconfig, sizeof(key_arr));
	if (result != 0) return 2;

	return 0;
}
