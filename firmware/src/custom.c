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
 * These are commands are "middleware" for HID messages.  So they don't need
 * the HID U2F layer to be called.  Thus they are custom commands not necessary for U2F.
 *
 */

#include <stdint.h>
#include "custom.h"
#include "bsp.h"
#include "gpio.h"
#include "atecc508a.h"
#include "eeprom.h"
#include "u2f.h"
#include "configuration.h"

uint8_t custom_command(struct u2f_hid_msg * msg)
{
	struct atecc_response res;
	uint8_t ec;
	uint8_t *out = msg->pkt.init.payload;

	switch(msg->pkt.init.cmd)
	{
		case U2F_CUSTOM_UPDATE_CONFIG:
			if(u2f_get_user_feedback_extended_wipe()){
				memset(out, 0xEE, sizeof(msg->pkt.init.payload));
				out[0] = 0;
				U2FHID_SET_LEN(msg, sizeof(msg->pkt.init.payload));
				usb_write((uint8_t*)msg, 64);
				break;
			}

			eeprom_erase(EEPROM_DATA_CONFIG);
			eeprom_write(EEPROM_DATA_CONFIG, msg->pkt.init.payload, sizeof(Configuration));
			configuration_read();
			memset(out, 0xEE, sizeof(msg->pkt.init.payload));
#ifndef _PRODUCTION_RELEASE
			eeprom_read(EEPROM_DATA_CONFIG, out+2, sizeof(Configuration));
#endif
			out[0] = 1;
			U2FHID_SET_LEN(msg, sizeof(msg->pkt.init.payload));
			usb_write((uint8_t*)msg, 64);
			u2f_delay(100);
			RSTSRC = RSTSRC_SWRSF__SET | RSTSRC_PORSF__SET;
		break;

#ifdef FEAT_FACTORY_RESET
		case U2F_CUSTOM_FACTORY_RESET:
			memset(out, 0xEE, sizeof(msg->pkt.init.payload));

			if(u2f_get_user_feedback_extended_wipe()){
				U2FHID_SET_LEN(msg, sizeof(msg->pkt.init.payload));
				usb_write((uint8_t*)msg, 64);
				break;
			}

			// clear device key explicitly
			memset(device_configuration.RMASK, 0, sizeof(device_configuration.RMASK));
			memset(device_configuration.WMASK, 0, sizeof(device_configuration.WMASK));
			eeprom_erase(EEPROM_DATA_WMASK);
			eeprom_erase(EEPROM_DATA_RMASK);
			eeprom_erase(EEPROM_DATA_U2F_CONST);

#ifndef _PRODUCTION_RELEASE
			eeprom_read(EEPROM_DATA_WMASK, out+3+8+8+8+8+8, 4);
			eeprom_read(EEPROM_DATA_RMASK, out+3+8+8+8+8+8+4, 4);
#endif //#ifndef _PRODUCTION_RELEASE
			out[0] = generate_WMASK(appdata.tmp, sizeof(appdata.tmp));
			out[1] = generate_RMASK(appdata.tmp, sizeof(appdata.tmp));
			out[2] = generate_device_key(NULL, appdata.tmp, sizeof(appdata.tmp));
#ifndef _PRODUCTION_RELEASE
			memmove(out+3, device_configuration.WMASK, 8);
			memmove(out+3+8, device_configuration.RMASK, 8);
			eeprom_read(EEPROM_DATA_U2F_CONST, out+3+8+8, 8);
			eeprom_read(EEPROM_DATA_WMASK, out+3+8+8+8, 8);
			eeprom_read(EEPROM_DATA_RMASK, out+3+8+8+8+8, 8);
#endif //#ifndef _PRODUCTION_RELEASE

			U2FHID_SET_LEN(msg, sizeof(msg->pkt.init.payload));
			usb_write((uint8_t*)msg, 64);
			break;
#endif // #ifdef FEAT_FACTORY_RESET

#ifdef U2F_SUPPORT_RNG_CUSTOM
		case U2F_CUSTOM_GET_RNG:
			if (atecc_send_recv(ATECC_CMD_RNG,ATECC_RNG_P1,ATECC_RNG_P2,
				NULL, 0,
				appdata.tmp,
				sizeof(appdata.tmp), &res) == 0 )
			{
				memmove(msg->pkt.init.payload, res.buf, 32);
				U2FHID_SET_LEN(msg, 32);
				usb_write((uint8_t*)msg, 64);
			}
			else
			{
				U2FHID_SET_LEN(msg, 0);
				usb_write((uint8_t*)msg, 64);
			}

			break;
#endif //#ifdef U2F_SUPPORT_RNG_CUSTOM
#ifdef U2F_SUPPORT_SEED_CUSTOM
		case U2F_CUSTOM_SEED_RNG:
			ec = atecc_send_recv(ATECC_CMD_NONCE,ATECC_NONCE_RNG_UPDATE,0,
							msg->pkt.init.payload, 20,
							appdata.tmp,
							sizeof(appdata.tmp), &res);
			U2FHID_SET_LEN(msg, 1);
			msg->pkt.init.payload[0] = ec == 0 ? 1 : 0;
			usb_write((uint8_t*)msg, 64);
			break;
#endif //#ifdef U2F_SUPPORT_SEED_CUSTOM
#ifdef U2F_SUPPORT_WINK
		case U2F_CUSTOM_WINK:
			led_blink(5, 300);
			break;
#endif //#ifdef U2F_SUPPORT_WINK
		default:
			return 0;
	}
	return 1;
}
