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
#include <SI_EFM8UB3_Register_Enums.h>
#include <stdint.h>

#include "eeprom.h"
#include "bsp.h"

//See EFM8UB1 Reference Manual, 4.3.1 Security Options
#define SECURITY_BYTE_POSITION 	(0xFBFF)
#define SECURITY_BYTE_UNSET 	(0xFF)
#define SECURITY_BYTE_PAGE	 	(0xFBC0)

//number of pages has to be 1's complement
#define LOCK_FIRST_N_PAGES(x) 	(0xFF^(x))
//page has 512 bytes
#define LOCK_FIRST_N_KBYTES(x) 	(LOCK_FIRST_N_PAGES( (x)*2 ))

void eeprom_init()
{
	uint8_t secbyte;
	eeprom_read(SECURITY_BYTE_POSITION,&secbyte,1);
	if (secbyte == SECURITY_BYTE_UNSET)
	{
		eeprom_erase(SECURITY_BYTE_PAGE);
		secbyte = LOCK_FIRST_N_KBYTES(40);
		eeprom_write(SECURITY_BYTE_POSITION, &secbyte, 1);
	}
}

void eeprom_xor(uint16_t addr, uint8_t * out_buf, uint8_t len){
	uint8_t code * eepaddr =  (uint8_t code *) addr;
	bit old_int;

	watchdog();
	while(len--)
	{
		old_int = IE_EA;
		IE_EA = 0;
		*out_buf++ ^= *eepaddr++;
		IE_EA = old_int;
	}
	watchdog();
}

void eeprom_read(uint16_t addr, uint8_t * buf, uint8_t len)
{
	uint8_t code * eepaddr =  (uint8_t code *) addr;
	bit old_int;

	watchdog();
	while(len--)
	{
		old_int = IE_EA;
		IE_EA = 0;
		*buf++ = *eepaddr++;
		IE_EA = old_int;
	}
	watchdog();
}

void _eeprom_write(uint16_t addr, uint8_t * buf, uint8_t len, uint8_t flags)
{
	uint8_t xdata * data eepaddr = (uint8_t xdata *) addr;
	bit old_int;

	while(len--)
	{
		old_int = IE_EA;
		IE_EA = 0;
		// Enable VDD monitor
		VDM0CN = 0x80;
		RSTSRC = 0x02;

		// unlock key
		FLKEY  = 0xA5;
		FLKEY  = 0xF1;
		PSCTL |= flags;

		*eepaddr = *buf;
		PSCTL &= ~flags;
		IE_EA = old_int;

		eepaddr++;
		buf++;
	}
}
