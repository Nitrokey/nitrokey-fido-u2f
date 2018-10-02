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

#include <string.h>
#include "eeprom.h"
#include <stdbool.h>
#include "sanity-check.h"

#ifdef _SECURE_EEPROM
	#define _secure_eeprom 1
#else
	#define _secure_eeprom 0
#endif

#ifdef FAKE_TOUCH
	#define _fake_touch 1
#else
	#define _fake_touch 0
#endif

#ifdef DISABLE_WATCHDOG
	#define _disable_watchdog 1
#else
	#define _disable_watchdog 0
#endif

#ifdef ATECC_SETUP_DEVICE
	#define _setup 1
#else
	#define _setup 0
#endif

bool sanity_check_passed = false;

bool test_if_memory_empty(uint16_t addr, uint8_t len){
	uint8_t buf[36];
	uint8_t i, zeroes = 0;
	bool res = true;
	if (len > sizeof(buf)) return false;

	eeprom_read(addr, buf, len);

	// make it constant time, do not return early
	// reject all 0xFF's
	for (i=0; i<len; i++){
		zeroes += (buf[i] == 0xFF);
	}
	res &= (zeroes != len);
	zeroes = 0;

	// reject all 0x00's
	for (i=0; i<len; i++){
		zeroes += (buf[i] == 0x00);
	}
	res &= (zeroes != len);
	zeroes = 0;

	memset(buf, 0, len);
	return res;
}

check_info sanity_check_builder(){
	check_info c;
	c.constants_filled = true;
	// make it constant time, do not return early
	c.constants_filled &= test_if_memory_empty(EEPROM_DATA_RMASK, EEPROM_DATA_RWMASK_LENGTH);
	c.constants_filled &= test_if_memory_empty(EEPROM_DATA_WMASK, EEPROM_DATA_RWMASK_LENGTH);
	c.constants_filled &= test_if_memory_empty(EEPROM_DATA_U2F_CONST, U2F_CONST_LENGTH);
	c.eeprom_protection = _secure_eeprom;
	c.fake_touch = _fake_touch;
	c.disable_watchdog = _disable_watchdog;
	c.setup_firmware = _setup;

	return c;
}

bool sanity_check(check_info *out_c){
	check_info c;
	c = sanity_check_builder();
	if (out_c != NULL)
		*out_c = c;
	sanity_check_passed = c.constants_filled && c.eeprom_protection && !c.fake_touch
			&& !c.disable_watchdog && !c.setup_firmware;
	return sanity_check_passed;
}
