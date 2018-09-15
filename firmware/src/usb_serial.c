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


	Control USB serial number. Get it from ATECC508A chip, and expose where requested.

 */


#include "descriptors.h"
#include <stdint.h>
#include "eeprom.h"
#include "atecc508a.h"

#define NK_SERIAL_LEN			(13)
#define NK_SERIAL_ASCII_LEN		(NK_SERIAL_LEN*2)

typedef struct {
	uint8_t header[3];
	uint8_t serial_ascii[NK_SERIAL_ASCII_LEN+1];
} USBSerialDescr;

USBSerialDescr serial_descriptor;

/**
 * Convert single hex digit to ascii character.
 */
static uint8_t hex2ascii(uint8_t c){
	if (c > 15){
		return 'x';
	}
	if(c <= 9){
		return c + '0';
	}
	c -= 10;
	return c + 'A';
}

/**
 * Convert uint8_t to string, e.g. 0x21 to "21".
 */
static void convert_bin_to_hex(uint8_t *src, uint8_t src_len, uint8_t *dest, uint8_t dest_len){
	uint8_t i, c;

	if (src_len*2 > dest_len)
		return;

	for (i=0; i<src_len; i++){
	    c = 0xF0 & src[i];
	    c >>= 4;
	    dest[i*2] = hex2ascii(c);

	    c = 0x0F & src[i];
	    dest[i*2+1] = hex2ascii(c);
	  }
}

/**
 * Update USB serial of the device, with a value read from EEPROM.
 */
void update_USB_serial(){
	uint8_t i;
	uint8_t buf[NK_SERIAL_LEN];

	memset(serial_descriptor.serial_ascii, 0, sizeof(serial_descriptor.serial_ascii));
	// load target USB serial number
	eeprom_read(EEPROM_DATA_SERIAL, buf, NK_SERIAL_LEN);
	convert_bin_to_hex(buf, sizeof(buf), serial_descriptor.serial_ascii, sizeof(serial_descriptor.serial_ascii));

	serial_descriptor.header[0] = USB_STRING_DESCRIPTOR_UTF16LE_PACKED;
	serial_descriptor.header[1] = sizeof(serial_descriptor.serial_ascii)*2;
	serial_descriptor.header[2] = USB_STRING_DESCRIPTOR;

	initstruct.stringDescriptors[3] = (uint8_t*) &serial_descriptor;
}


/**
 * Get serial number from the ATECC508A chip and save it on MCU's FLASH memory.
 * Serial is exposed in the first 12 bytes of ATECC508A configuration zone.
 * Abort, if the memory slot is not empty.
 * Requires ATECC508A to be configured and awake.
 * See 2.2 EEPROM Configuration Zone, ATECC508A Datasheet Complete DS20005927A-page 13
 */
void get_serial_num(){
	uint8_t buf[40];
	struct atecc_response res;
	uint8_t i;

	eeprom_read(EEPROM_DATA_SERIAL, buf, NK_SERIAL_LEN);
	for (i=0; i<NK_SERIAL_LEN; i++){
		if (buf[i] != 0xFF) return;				//serial number set already, abort
	}

	// serial number is not set in EEPROM, reading from ATECC
	atecc_send_recv(ATECC_CMD_READ,
		ATECC_RW_CONFIG | ATECC_RW_EXT, 0, NULL, 0,
		buf, sizeof(buf), &res);


	eeprom_erase(EEPROM_DATA_SERIAL);
	eeprom_write(EEPROM_DATA_SERIAL, res.buf, res.len);
}
