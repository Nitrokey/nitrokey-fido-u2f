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
 * atecc508.c
 * 		Implementation for ATECC508 peripheral.
 *
 */
#include <endian.h>
#include <stdint.h>
#include "app.h"
#include "atecc508a.h"
#include "i2c.h"
#include "eeprom.h"
#include "gpio.h"

#include "bsp.h"

struct SHA_context sha_ctx;

struct atecc_response res_digest;

#ifdef ATECC_SETUP_DEVICE
// 1 page - 64 bytes
struct DevConf device_configuration;


int8_t read_masks(){
	u2f_prints("reading masks -----\r\n");
	memset(&device_configuration, 42, sizeof(device_configuration));
	eeprom_read(EEPROM_DATA_RMASK, device_configuration.RMASK, sizeof(device_configuration.RMASK));
	eeprom_read(EEPROM_DATA_WMASK, device_configuration.WMASK, sizeof(device_configuration.WMASK));
	u2f_prints("current write key: "); dump_hex(device_configuration.WMASK,36);
	u2f_prints("current read key: "); dump_hex(device_configuration.RMASK,36);
	return 0;
}

int8_t write_masks(){
	eeprom_erase(EEPROM_DATA_RMASK);
	eeprom_erase(EEPROM_DATA_WMASK);
	eeprom_write(EEPROM_DATA_RMASK, device_configuration.RMASK, sizeof(device_configuration.RMASK));
	eeprom_write(EEPROM_DATA_WMASK, device_configuration.WMASK, sizeof(device_configuration.WMASK));
	return 0;
}
#endif

uint8_t atecc_used = 0;

int8_t atecc_send(uint8_t cmd, uint8_t p1, uint16_t p2,
					uint8_t * buf, uint8_t len)
{
	static data uint8_t params[6];
	params[0] = 0x3;
	params[1] = 7+len;
	params[2] = cmd;
	params[3] = p1;
	params[4] = ((uint8_t*)&p2)[1];
	params[5] = ((uint8_t* )&p2)[0];

	smb_set_ext_write(buf, len);
	smb_write( ATECC508A_ADDR, params, sizeof(params));
	if (SMB_WAS_NACKED())
	{
		return -1;
	}
	return 0;
}

void atecc_idle()
{
	smb_write( ATECC508A_ADDR, "\x02", 1);
}

void atecc_sleep()
{
	smb_write( ATECC508A_ADDR, "\x01", 1);
}

void atecc_wake()
{
	smb_write( ATECC508A_ADDR, "\0\0", 2);
}

#define PKT_CRC(buf, pkt_len) (htole16(*((uint16_t*)(buf+pkt_len-2))))

int8_t atecc_recv(uint8_t * buf, uint8_t buflen, struct atecc_response* res)
{
	uint8_t pkt_len;
	pkt_len = smb_read( ATECC508A_ADDR,buf,buflen);
	if (SMB_WAS_NACKED())
	{
		return -1;
	}

	if (SMB_FLAGS & SMB_READ_TRUNC)
	{
		set_app_error(ERROR_READ_TRUNCATED);
		return -1;
	}

	if (pkt_len <= buflen && pkt_len >= 4)
	{
		if (PKT_CRC(buf,pkt_len) != SMB_crc)
		{
			set_app_error(ERROR_I2C_CRC);
			return -1;
		}
	}
	else
	{
		set_app_error(ERROR_I2C_BAD_LEN);
		return -1;
	}

	if (pkt_len == 4 && buf[1] != 0)
	{
		set_app_error(buf[1]);
		return -1;
	}

	if (res != NULL)
	{
		res->len = pkt_len - 3;
		res->buf = buf+1;
	}
	return pkt_len;
}

static void delay_cmd(uint8_t cmd)
{
	uint8_t d = 0;
	switch(cmd)
	{
	case ATECC_CMD_COUNTER:
		d = 20; break;
	case ATECC_CMD_GENDIG:
		d = 11; break;
	case ATECC_CMD_INFO:
		d = 1; break;
	case ATECC_CMD_LOCK:
		d = 32; break;
	case ATECC_CMD_NONCE:
		d = 7; break;
	case ATECC_CMD_PRIVWRITE:
		d = 48; break;
	case ATECC_CMD_READ:
		d = 1; break;
	case ATECC_CMD_RNG:
		d = 23; break;
	case ATECC_CMD_SHA:
		d = 9; break;
	case ATECC_CMD_WRITE:
		d = 26; break;
	case ATECC_CMD_SIGN:
		d = 50; break;
	case ATECC_CMD_GENKEY:
		d = 115; break;
	default:
		d = 58; break;
	}
	u2f_delay(d/4+1);
}

int8_t atecc_send_recv(uint8_t cmd, uint8_t p1, uint16_t p2,
							uint8_t* tx, uint8_t txlen, uint8_t * rx,
							uint8_t rxlen, struct atecc_response* res)
{
	uint8_t errors = 0;
	uint16_t errarr[20]; //store error codes for debugging
	memset(errarr, 0, sizeof(errarr));
	atecc_used = 1;
	atecc_wake();
	u2f_delay(5);

	resend:
	set_app_error(ERROR_NOTHING);
	while(atecc_send(cmd, p1, p2, tx, txlen) == -1)
	{
		errarr[errors] = 0x1000+get_app_error();
		errors++;
		if (errors > 8)
		{
			return -1;
		}
	}
	while(atecc_recv(rx,rxlen, res) == -1)
	{
		errarr[errors] = 0x2000+get_app_error();
		errors++;
		if (errors > 16)
		{
			return -2;
		}
		switch(get_app_error())
		{
			case ERROR_NOTHING:
				delay_cmd(cmd);
				break;
			case ERROR_ATECC_WATCHDOG:
				atecc_idle();
				u2f_delay(5);
				atecc_wake();
				u2f_delay(5);
				goto resend;
				break;
			case ERROR_ATECC_WAKE:
				u2f_delay(1);
				goto resend;
				break;
			default:
				u2f_delay(10);
				goto resend;
				break;
		}

	}
	atecc_idle();
	return 0;
}

/**
 * Initializes sha256 computation on ATECC chip. Uses global sha_ctx variable.
 */
void u2f_sha256_start(uint8_t hmac_key, uint8_t sha_flags)
{
	sha_ctx.SHA_FLAGS = sha_flags;
	sha_ctx.SHA_HMAC_KEY = hmac_key;
	sha_ctx.shaoffset = 0;
	atecc_send_recv(ATECC_CMD_SHA,
			sha_ctx.SHA_FLAGS, sha_ctx.SHA_HMAC_KEY,NULL,0,
			sha_ctx.shabuf, sizeof(sha_ctx.shabuf), NULL);
}

void u2f_sha256_start_default()
{
	u2f_sha256_start(0, ATECC_SHA_START);
}

/**
 * Sends data for sha256 computation on ATECC chip. Buffers input data in global buffer
 * `sha_ctx.shabuf` with 64 bytes size and sends only, if input exceeds its capacity.
 */
void u2f_sha256_update(uint8_t * buf, uint8_t len)
{
	uint8_t i = 0;
	watchdog();
	while(len--)
	{
		sha_ctx.shabuf[sha_ctx.shaoffset++] = *buf++;
		if (sha_ctx.shaoffset == 64)
		{
			atecc_send_recv(ATECC_CMD_SHA,
					ATECC_SHA_UPDATE, 64, sha_ctx.shabuf, 64,
					sha_ctx.shabuf, sizeof(sha_ctx.shabuf), NULL);
			sha_ctx.shaoffset = 0;
		}
	}
}

/**
 * Sends remaining data for sha256 computation.
 * Out: internal ATECC's TempKey buffer, copied back to the MCU into `res_digest` variable
 */
struct atecc_response* u2f_sha256_finish()
{
	if (sha_ctx.SHA_FLAGS == ATECC_SHA_START) sha_ctx.SHA_FLAGS = ATECC_SHA_END;
	else if (sha_ctx.SHA_FLAGS == ATECC_SHA_HMACSTART) sha_ctx.SHA_FLAGS = ATECC_SHA_HMACEND;
	atecc_send_recv(ATECC_CMD_SHA,
			sha_ctx.SHA_FLAGS, sha_ctx.shaoffset,sha_ctx.shabuf,sha_ctx.shaoffset,
			sha_ctx.shabuf, sizeof(sha_ctx.shabuf), &res_digest);
	return &res_digest;
}

/**
 * Makes hash of PRIVWRITE(slot) command's payload, key and mask
 * Out: internal ATECC's TempKey buffer, copied back to the MCU into `res_digest` variable
 */
void compute_key_hash(uint8_t * key, uint16_t mask, int slot)
{
	eeprom_read(mask, appdata.tmp, 32);

	u2f_sha256_start_default();
	u2f_sha256_update(appdata.tmp, 32);

	// key must start with 4 zeros
	memset(appdata.tmp,0,28);
	memmove(appdata.tmp + 28, key, 36);

	appdata.tmp[0] = ATECC_CMD_PRIVWRITE;
	appdata.tmp[1] = ATECC_PRIVWRITE_ENC;
	appdata.tmp[2] = slot;
	appdata.tmp[3] = 0;
	appdata.tmp[4] = 0xee;
	appdata.tmp[5] = 0x01;
	appdata.tmp[6] = 0x23;

	u2f_sha256_update(appdata.tmp,28 + 36);
	u2f_sha256_finish();
}

#define CWH_ZEROES_COUNT	(25)
#define CWH_HEADER_LEN		(7)
#define CWH_WMASK_LEN		(32)
#define CWH_DATA_LEN		(32)

void compute_write_hash(uint8_t * key, uint16_t mask, int slot)
{
	// Compute hash from encrypted WRITE. See chapter 9.21 from complete data sheet.
	// SHA-256(TempKey, Opcode, Param1, Param2, SN<8>, SN<0:1>, <25 bytes of zeros>, PlainTextData)
	eeprom_read(mask, appdata.tmp, CWH_WMASK_LEN);

	u2f_sha256_start_default();
	u2f_sha256_update(appdata.tmp, CWH_WMASK_LEN);

	memset(appdata.tmp,0,CWH_HEADER_LEN+CWH_ZEROES_COUNT);
	memmove(appdata.tmp +CWH_HEADER_LEN+CWH_ZEROES_COUNT, key, CWH_DATA_LEN);

//	ATECC_RW_DATA|ATECC_RW_EXT, ATECC_EEPROM_DATA_SLOT(U2F_DEVICE_KEY_SLOT)
	appdata.tmp[0] = ATECC_CMD_WRITE;
	appdata.tmp[1] = ATECC_RW_DATA|ATECC_RW_EXT;
	appdata.tmp[2] = slot;
	appdata.tmp[3] = 0;
	appdata.tmp[4] = 0xee; //FIXME SN values for ATECC508A, might not work on other model
	appdata.tmp[5] = 0x01;
	appdata.tmp[6] = 0x23;

	u2f_sha256_update(appdata.tmp,CWH_HEADER_LEN+CWH_ZEROES_COUNT +CWH_DATA_LEN);
	u2f_sha256_finish();
}

int atecc_prep_encryption()
{
	struct atecc_response res;
	memset(appdata.tmp,0,32);
	if( atecc_send_recv(ATECC_CMD_NONCE,ATECC_NONCE_TEMP_UPDATE,0,
								appdata.tmp, 32,
								appdata.tmp, 40, &res) != 0 )
	{
		u2f_prints("pass through to tempkey failed\r\n");
		return -1;
	}
	if( atecc_send_recv(ATECC_CMD_GENDIG,
			ATECC_RW_DATA, U2F_WKEY_KEY_SLOT, NULL, 0,
			appdata.tmp, 40, &res) != 0)
	{
		u2f_prints("GENDIG failed\r\n");
		return -1;
	}

	return 0;
}

int atecc_privwrite(uint16_t keyslot, uint8_t * key, uint16_t mask, uint8_t * digest)
{
	struct atecc_response res;
	uint8_t i;

	atecc_prep_encryption();

	memmove(appdata.tmp, key, 36);
	eeprom_xor(mask, appdata.tmp, 36);

	memmove(appdata.tmp+36, digest, 32);

	if( atecc_send_recv(ATECC_CMD_PRIVWRITE,
			ATECC_PRIVWRITE_ENC, keyslot, appdata.tmp, 68,
			appdata.tmp, 40, &res) != 0)
	{
		u2f_prints("PRIVWRITE failed\r\n");
		return -1;
	}
	return 0;
}


#ifdef ATECC_SETUP_DEVICE

int8_t atecc_write_eeprom(uint8_t base, uint8_t offset, uint8_t* srcbuf, uint8_t len)
{
	uint8_t buf[7];
	struct atecc_response res;

	uint8_t * dstbuf = srcbuf;
	if (offset + len > 4)
		return -1;
	if (len < 4)
	{
		atecc_send_recv(ATECC_CMD_READ,
				ATECC_RW_CONFIG, base, NULL, 0,
				buf, sizeof(buf), &res);

		dstbuf = res.buf;
		memmove(res.buf + offset, srcbuf, len);
	}

	atecc_send_recv(ATECC_CMD_WRITE,
			ATECC_RW_CONFIG, base, dstbuf, 4,
			buf, sizeof(buf), &res);

	if (res.buf[0])
	{
		set_app_error(-res.buf[0]);
		return -1;
	}
	return 0;
}


/**
 * Reads one ATECC508A's configuration byte. Read is done by 4 bytes.
 */
static uint8_t read_config_byte(uint8_t pos)
{
	uint8_t buf[ATECC_RW_LENGTH + ATECC_RW_SUFFIX_LENGTH];
	struct atecc_response res;
	atecc_send_recv(ATECC_CMD_READ,
					ATECC_RW_CONFIG, pos/ATECC_RW_LENGTH, NULL, 0,
					buf, sizeof(buf), &res);
	return res.buf[pos % ATECC_RW_LENGTH];
}

/**
 * See 2.2 EEPROM Configuration Zone of ATECC508A's Complete Data Sheet
 */
static int is_config_locked(uint8_t * buf)
{
	if (read_config_byte(ATECC_CONFIG_LOCK_CONFIG_POS) == 0)
		return 1;
	else
		return 0;
}

/**
 * See 2.2 EEPROM Configuration Zone of ATECC508A's Complete Data Sheet
 */
static int is_data_locked(uint8_t * buf)
{
	if (read_config_byte(ATECC_CONFIG_LOCK_VALUE_POS) == 0)
		return 1;
	else
		return 0;
}

/**
 * Prints ATECC508A's configuration over serial line
 */
static void dump_config(uint8_t* buf)
{
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

#ifndef U2F_PRINT
	return;
#endif

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

}

static void atecc_setup_config(uint8_t* buf)
{
	uint8_t i;

//	struct atecc_slot_config c;


	uint8_t * slot_configs = "\x83\x71"
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

	uint8_t * key_configs = "\x13\x00"
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

	// write configuration
	for (i = 0; i < 16; i++)
	{
		if ( atecc_write_eeprom(ATECC_EEPROM_SLOT(i), ATECC_EEPROM_SLOT_OFFSET(i), slot_configs+i*2, ATECC_EEPROM_SLOT_SIZE) != 0)
		{
			u2f_printb("1 atecc_write_eeprom failed ",1, i);
		}

		if ( atecc_write_eeprom(ATECC_EEPROM_KEY(i), ATECC_EEPROM_KEY_OFFSET(i), key_configs+i*2, ATECC_EEPROM_KEY_SIZE) != 0)
		{
			u2f_printb("2 atecc_write_eeprom failed " ,1,i);
		}

	}


	dump_config(buf);
}

static uint8_t trans_key[36];
static uint8_t write_key[36];

#ifdef ENABLE_TESTS
void atecc_test_enc_read(uint8_t * buf)
{
	struct atecc_response res;
	memset(trans_key,0x5a,sizeof(trans_key));
	u2f_prints("plaintext: "); dump_hex(trans_key, 32);

	if( atecc_send_recv(ATECC_CMD_WRITE,
			ATECC_RW_DATA|ATECC_RW_EXT, ATECC_EEPROM_DATA_SLOT(3), trans_key, 32,
			buf, 40, &res) != 0)
	{
		u2f_prints("writing test key failed\r\n");
		return;
	}

	atecc_prep_encryption();

	if (atecc_send_recv(ATECC_CMD_READ,
			ATECC_RW_DATA | ATECC_RW_EXT, ATECC_EEPROM_DATA_SLOT(3), NULL, 0,
			buf, 40, &res) != 0)
	{
		u2f_prints("READ slot 3 failed\r\n");
		return;
	}
	else
	{
		u2f_prints("ciphertext: "); dump_hex(res.buf, 32);
	}
}

void atecc_test_signature(int keyslot, uint8_t * buf)
{
	struct atecc_response res;
	if ( atecc_send_recv(ATECC_CMD_GENKEY,
			ATECC_GENKEY_PUBLIC, keyslot, NULL, 0,
			appdata.tmp, 70, &res) != 0)
	{
		u2f_prints("GENKEY public failed\r\n");
		return;
	}

	u2f_prints("pubkey: "); dump_hex(res.buf, 64);

	u2f_prints("signing input: "); dump_hex(res_digest.buf, 32);

	if( atecc_send_recv(ATECC_CMD_NONCE,ATECC_NONCE_TEMP_UPDATE,0,
								res_digest.buf, 32,
								buf, 40, &res) != 0 )
	{
		u2f_prints("signing pass through to tempkey failed\r\n");
		return;
	}

	if( atecc_send_recv(ATECC_CMD_SIGN,
			ATECC_SIGN_EXTERNAL, keyslot, NULL, 0,
			appdata.tmp, 70, &res) != 0)
	{
		u2f_prints("signing failed\r\n");
		return;
	}

	dump_signature_der(res.buf);
}
#endif


void atecc_setup_init(uint8_t * buf)
{
	// 13s watchdog
	WDTCN = 7;
	dump_config(buf);
	if (!is_config_locked(buf))
	{
		u2f_prints("setting up config...\r\n");
		atecc_setup_config(buf);
	}
	else
	{
		u2f_prints("already locked\r\n");
	}
}

uint8_t generate_random_data(uint8_t *out_buf){
	struct atecc_response res;
	if (atecc_send_recv(ATECC_CMD_RNG,ATECC_RNG_P1,ATECC_RNG_P2,
					NULL, 0,
					appdata.tmp, sizeof(appdata.tmp),
					&res) == 0 )
		{
			memmove(out_buf, res.buf, 32);
			return 0;
		}
	return 1;
}

void generate_mask(uint8_t *output, uint8_t wkey){
	u2f_prints("generating mask ... ");	dump_hex(&wkey,1);

	if (generate_random_data(output+32) != 0){
		u2f_prints("failed\r\n");
		output[0] = 0;
		return;
	}
	u2f_prints("generated random output+32: "); dump_hex(output+32,32);

	if (wkey == 1){
		memmove(trans_key, output+32, 32);
		u2f_prints("generated trans_key: "); dump_hex(trans_key,32);

		if(atecc_send_recv(ATECC_CMD_WRITE,
				ATECC_RW_DATA|ATECC_RW_EXT, ATECC_EEPROM_DATA_SLOT(U2F_MASTER_KEY_SLOT),
				trans_key, 32,
				output, 32, NULL) != 0)
		{
			u2f_prints("writing master key/wkey failed\r\n");
			return;
		}
	}

	u2f_sha256_start_default();
	u2f_sha256_update(output+32, 32);

	memset(output, 0, 64);
	output[0] = 0x15;
	output[1] = 0x02;
	output[2] = 0x01;
	output[3] = 0;
	output[4] = 0xee;
	output[5] = 0x01;
	output[6] = 0x23;
	u2f_sha256_update(output, 64);

	u2f_sha256_finish();

	memmove(output, res_digest.buf, 32);
	u2f_prints("generated key mask output: "); dump_hex(output,32);

	//stage 2
	u2f_sha256_start_default();
	u2f_sha256_update(output, 32);
	u2f_sha256_finish();

	memmove(output+32, res_digest.buf, 8);
	u2f_prints("generated key mask2 output: "); dump_hex(output+32,8);

	u2f_prints("generated key mask: "); dump_hex(output,32+8);
}

void generate_device_key(uint8_t *output, uint8_t *buf, uint8_t buflen){
	u2f_prints("generating device key ... ");

	if (generate_random_data(trans_key) == 0){
		u2f_prints("succeed\r\n");
		output[0] = 1;
	} else {
		u2f_prints("failed\r\n");
		output[0] = 0;
		return;
	}

#ifndef _PRODUCTION_RELEASE
	u2f_prints("device key: "); dump_hex(trans_key,32);
	memmove(output+1, trans_key, 16);
#endif

	compute_write_hash(trans_key,  EEPROM_DATA_WMASK, ATECC_EEPROM_DATA_SLOT(U2F_DEVICE_KEY_SLOT));

	atecc_prep_encryption();

	memmove(appdata.tmp, trans_key, 32);
	memmove(appdata.tmp+32, res_digest.buf, 32);

	eeprom_xor(EEPROM_DATA_WMASK, appdata.tmp, 32);

	if(atecc_send_recv(ATECC_CMD_WRITE,
		ATECC_RW_DATA|ATECC_RW_EXT, ATECC_EEPROM_DATA_SLOT(U2F_DEVICE_KEY_SLOT),
		appdata.tmp, 32+32,
		buf, buflen, NULL) != 0)
	{
		output[0] = 2; //failed, stage 2, key writing
		u2f_prints("writing device key failed\r\n");
		return;
	}
	u2f_prints("writing device key succeed\r\n");

	// generate u2f_zero_const
	generate_random_data(buf);
	eeprom_erase(EEPROM_DATA_U2F_CONST);
	eeprom_write(EEPROM_DATA_U2F_CONST, buf, U2F_CONST_LENGTH);
#ifndef _PRODUCTION_RELEASE
	u2f_prints("u2f_zero_const: "); dump_hex(buf,U2F_CONST_LENGTH);
	memmove(output+1+32, buf, 16);

	u2f_sha256_start(U2F_DEVICE_KEY_SLOT, ATECC_SHA_HMACSTART);
	u2f_sha256_update("successful write test");
	u2f_sha256_finish();
	memmove(output+1+16, res_digest.buf, 16);
#endif
}

typedef struct atecc_command{
	uint8_t opcode;
	uint8_t P1;
	uint8_t P2;
	uint8_t data_length;
	uint8_t buf[64-4];
} atecc_cmd;

// buf should be at least 40 bytes
void atecc_setup_device(struct config_msg * msg)
{
	struct atecc_response res;
	struct config_msg usbres;

	static uint16_t crc = 0;
	int i;
	uint8_t buf[40];

	memset(&usbres, 0, sizeof(struct config_msg));
	usbres.cmd = msg->cmd;
	u2f_prints("incoming msg: "); dump_hex(msg,64);

	switch(msg->cmd)
	{
#ifndef _PRODUCTION_RELEASE
		case U2F_CONFIG_ATECC_PASSTHROUGH:
		{
			atecc_cmd* cmd = (atecc_cmd*)msg->buf;
			uint8_t err = atecc_send_recv(cmd->opcode,
					cmd->P1, cmd->P2, cmd->buf, cmd->data_length,
					buf, sizeof(buf), &res);
			memset(usbres.buf, 0, sizeof(usbres.buf));
			memmove(usbres.buf+1, res.buf, res.len);
			usbres.buf[0] = err;
		} break;
#endif
		case U2F_CONFIG_GET_SERIAL_NUM:

			u2f_prints("U2F_CONFIG_GET_SERIAL_NUM\r\n");
			atecc_send_recv(ATECC_CMD_READ,
					ATECC_RW_CONFIG | ATECC_RW_EXT, 0, NULL, 0,
					buf, 40, &res);
			memmove(usbres.buf+1, res.buf, 15);
			usbres.buf[0] = 15;
			break;

		case U2F_CONFIG_LOAD_TRANS_KEY:
			u2f_prints("U2F_CONFIG_LOAD_TRANS_KEY\r\n");
			u2f_prints("Use U2F_CONFIG_LOAD_WRITE_KEY\r\n");
			break;

		case U2F_CONFIG_IS_BUILD:
			u2f_prints("U2F_CONFIG_IS_BUILD\r\n");
			usbres.buf[0] = 1;
			break;
		case U2F_CONFIG_IS_CONFIGURED:
			u2f_prints("U2F_CONFIG_IS_CONFIGURED\r\n");
			usbres.buf[0] = 1;
			break;
		case U2F_CONFIG_LOCK:
			crc = *(uint16_t*)msg->buf;
			usbres.buf[0] = 1;
			u2f_printx("got crc: ",1,crc);

			if (!is_config_locked(buf))
			{
				if (atecc_send_recv(ATECC_CMD_LOCK,
						ATECC_LOCK_CONFIG, crc, NULL, 0,
						buf, sizeof(buf), NULL))
				{
					u2f_prints("ATECC_CMD_LOCK config failed\r\n");
					return;
				}
			}
			else
			{
				u2f_prints("config already locked\r\n");
			}

			if (!is_data_locked(buf))
			{
				crc = 0;
				if (atecc_send_recv(ATECC_CMD_LOCK,
						ATECC_LOCK_DATA_OTP | ATECC_LOCK_IGNORE_SUMMARY, crc, NULL, 0,
						buf, sizeof(buf), NULL))
				{
					u2f_prints("ATECC_CMD_LOCK data failed\r\n");
					return;
				}
			}
			else
			{
				u2f_prints("data already locked\r\n");
			}
			break;

		case U2F_CONFIG_LOAD_RMASK_KEY:
			u2f_prints("U2F_CONFIG_LOAD_RMASK_KEY\r\n");
			u2f_prints("current read key: "); dump_hex(device_configuration.RMASK,36);

			generate_mask(appdata.tmp, 0);
			memmove(device_configuration.RMASK,appdata.tmp,36);

			write_masks();
			read_masks();
			u2f_prints("new set read key: "); dump_hex(device_configuration.RMASK,36);
			usbres.buf[0] = 1;
			memmove(usbres.buf+1,device_configuration.RMASK,36);
			break;

		case U2F_CONFIG_LOAD_WRITE_KEY:
			u2f_prints("U2F_CONFIG_LOAD_WRITE_KEY\r\n");
			u2f_prints("current write key: "); dump_hex(device_configuration.WMASK,36);

			generate_mask(appdata.tmp, 1);
			memmove(write_key,appdata.tmp,36);
			memmove(device_configuration.WMASK,appdata.tmp,36);

			write_masks();
			read_masks();
			u2f_prints("new set write key: "); dump_hex(device_configuration.WMASK,36);
			usbres.buf[0] = 1;
			memmove(usbres.buf + 1 , device_configuration.WMASK, 36);
			break;

		case U2F_CONFIG_GEN_DEVICE_KEY:
			u2f_prints("U2F_CONFIG_GEN_DEVICE_KEY\r\n");
			generate_device_key(usbres.buf, appdata.tmp, sizeof(appdata.tmp));
			break;

#ifndef _PRODUCTION_RELEASE
		case U2F_CONFIG_GET_SLOTS_FINGERPRINTS:
			usbres.buf[0] = 0;

			for (i=0; i<16; i++){
				u2f_sha256_start(i, ATECC_SHA_HMACSTART);
				u2f_sha256_update("successful write test");
				u2f_sha256_finish();
				if (get_app_error() == ERROR_NOTHING)
						memmove(usbres.buf+i*3+1, res_digest.buf, 3);
			}

			usbres.buf[0] = 1;
			set_app_error(ERROR_NOTHING);
			break;
#endif

		case U2F_CONFIG_LOAD_ATTEST_KEY:
			u2f_prints("U2F_CONFIG_LOAD_ATTEST_KEY\r\n");

			//reusing trans_key buffer for the attestation key upload
			memset(trans_key,0,36);
			memmove(trans_key+4,msg->buf,32);
			usbres.buf[0] = 1;
			compute_key_hash(trans_key,  EEPROM_DATA_WMASK, U2F_ATTESTATION_KEY_SLOT);

			u2f_prints("write key: "); dump_hex(write_key,36);

			if (atecc_privwrite(U2F_ATTESTATION_KEY_SLOT, trans_key, EEPROM_DATA_WMASK, res_digest.buf) != 0)
			{
//				The slot indicated by this command must be configured via KeyConfig.Private to contain an ECC private
//				key, and SlotConfig.IsSecret must be set to one, or else this command will return an error. If the slot is
//				individually locked using SlotLocked, then this command will also return an error.
				u2f_prints("load attest key failed\r\n");
				usbres.buf[0] = 0;
			}

			break;
		case U2F_CONFIG_BOOTLOADER_DESTROY:
			eeprom_erase(EEPROM_PAGE_START(EEPROM_LAST_PAGE_NUM-0));
			eeprom_erase(EEPROM_PAGE_START(EEPROM_LAST_PAGE_NUM-1));
			eeprom_erase(EEPROM_PAGE_START(EEPROM_LAST_PAGE_NUM-2));
			usbres.buf[0] = 1;
			led_blink(1, 100);
			break;
		default:
			u2f_printb("invalid command: ",1,msg->cmd);
			usbres.buf[0] = 0;
	}

	usb_write((uint8_t*)&usbres, HID_PACKET_SIZE);
	memset(msg, 0, 64);
}
#endif
