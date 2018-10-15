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

 */

#ifndef EEPROM_H_
#define EEPROM_H_

#include "app.h"

void eeprom_init();

void eeprom_read(uint16_t addr, uint8_t * buf, uint8_t len);
void eeprom_xor(uint16_t addr, uint8_t * out_buf, uint8_t len);

void _eeprom_write(uint16_t addr, uint8_t * buf, uint8_t len, uint8_t flags);


#define eeprom_write(a,b,l) 		_eeprom_write((a),(b),(l),0x1)
#define eeprom_erase(a) 			_eeprom_write((a),appdata.tmp,1,0x3)

#define EEPROM_PAGE_START(p)		(0x200*(p))
#define EEPROM_KB_START(p)			(EEPROM_PAGE_START(2*(p)))
#define EEPROM_PAGE_COUNT			(79)
#define EEPROM_LAST_PAGE_NUM		(EEPROM_PAGE_COUNT-1)
// 0x8000 -> 20kB -> EEPROM page 40
#define EEPROM_DATA_START 			(EEPROM_PAGE_START(40))
// pages are 512-bytes each, required to be cleared separately
// FIXME allocate all constants on one page (36 + 36 + 16), when required
#define EEPROM_DATA_RMASK 			EEPROM_PAGE_START(40)
#define EEPROM_DATA_WMASK 			EEPROM_PAGE_START(41)
#define EEPROM_DATA_U2F_CONST  		EEPROM_PAGE_START(42)
#define EEPROM_DATA_SERIAL			EEPROM_PAGE_START(43)
#define EEPROM_DATA_CONFIG			EEPROM_PAGE_START(44)

#define U2F_CONST_LENGTH			(32)

#endif /* EEPROM_H_ */
