#!/usr/bin/env python2
#
# Copyright (c) 2016, Conor Patrick
# Copyright (c) 2018, Nitrokey UG
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this
#    list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

#
#
#
# Client application for U2F Zero that can be used
# for configuring new builds or accessing custom functionality.
#
#
from __future__ import print_function
import time, os, sys, array, binascii, signal, random, hashlib

try:
    import hid
except:
    print('python hidapi module is required')
    print('try running: ')
    print('     apt-get install libusb-1.0-0-dev libudev-dev')
    print('     pip install hidapi')
    sys.exit(1)

try:
    import ecdsa
except:
    print('python ecdsa module is required')
    print('try running: ')
    print('     pip install ecdsa')
    sys.exit(1)


cid_broadcast = [0xFF, 0xFF, 0xFF, 0xFF]
cmd_prefix    = [0] + cid_broadcast

class ATECC:
    CMD_COUNTER = 0x24
    # P1
    COUNTER_READ = 0
    COUNTER_INC = 1
    # P2
    COUNTER0 = 0
    COUNTER1 = 1

    CMD_RNG = 0x1B
    RNG_P1 = 0
    RNG_P2 = 0

    CMD_SHA = 0x47
    # P1
    SHA_START = 0x0
    SHA_UPDATE = 0x1
    SHA_END = 0x2
    SHA_HMACSTART = 0x4
    SHA_HMACEND = 0x5
    # P2 is keyslot
    # ID = for hmac and message length otherwise

    CMD_READ = 0x02
    # P1
    RW_CONFIG = 0x00
    RW_OTP = 0x01
    RW_DATA = 0x02
    RW_EXT = 0x80
    # P2
    # read = addr

    CMD_WRITE = 0x12
    # P1
    # same = as for read
    # P2
    # write = addr

    # EEPROM_SLOT(x) 		(0x5 + ((x)>>1))
    # EEPROM_SLOT_OFFSET(x) ( (x) & 1 ? 2 : 0 )
    # EEPROM_SLOT_SIZE = 0x2

    # EEPROM_KEY(x) 		(24 + ((x)>>1))
    # EEPROM_KEY_OFFSET(x) 	( (x) & 1 ? 2 : 0 )
    # EEPROM_KEY_SIZE = 0x2

    # EEPROM_DATA_SLOT(x) 		(x<<3)

    # EEPROM_B2A(b) 		((b)>>2)
    # EEPROM_B2O(b) 		((b)&0x3)

    CMD_LOCK = 0x17
    # P1
    # flags =
    LOCK_CONFIG = 0x00
    LOCK_DATA_OTP = 0x01
    LOCK_SLOT = 0x02
    # LOCK_SLOTNUM(x) 		(((x)&0xf)<<2)
    LOCK_IGNORE_SUMMARY = 0x08
    # P2 is CRC or 0

    CMD_GENKEY = 0x40
    # P1
    GENKEY_PRIVATE = 0x04
    GENKEY_PUBLIC = 0x00
    GENKEY_PUBDIGEST = 0x08
    GENKEY_PUBDIGEST2 = 0x10
    # P2 is keyid

    CMD_NONCE = 0x16
    # P1
    NONCE_RNG_UPDATE = 0x0
    NONCE_TEMP_UPDATE = 0x3
    # P2 is 0

    CMD_SIGN = 0x41
    # P1
    SIGN_INTERNAL = 0x00
    SIGN_EXTERNAL = 0x80
    # P2 is keyid

    CMD_GENDIG = 0x15
    # P1
    # same = as for read
    # P2 is keyid

    CMD_INFO = 0x30
    # P1
    # same = as for read
    INFO_REVISION = 0x00
    INFO_KEYVALID = 0x01
    INFO_STATE = 0x02
    INFO_GPIO = 0x03
    # P2 is keyid

    CMD_PRIVWRITE = 0x46
    # P1
    PRIVWRITE_ENC = 0x40
    # P2 is keyid

class commands:
    U2F_CONFIG_GET_SERIAL_NUM = 0x80
    U2F_CONFIG_IS_BUILD = 0x81
    U2F_CONFIG_IS_CONFIGURED = 0x82
    U2F_CONFIG_LOCK = 0x83
    U2F_CONFIG_GENKEY = 0x84
    U2F_CONFIG_LOAD_TRANS_KEY = 0x85
    U2F_CONFIG_LOAD_WRITE_KEY = 0x86
    U2F_CONFIG_LOAD_ATTEST_KEY = 0x87
    U2F_CONFIG_BOOTLOADER = 0x88
    U2F_CONFIG_BOOTLOADER_DESTROY = 0x89
    U2F_CONFIG_ATECC_PASSTHROUGH = 0x8a
    U2F_CONFIG_LOAD_READ_KEY = 0x8b
    U2F_CONFIG_GEN_DEVICE_KEY = 0x8c
    U2F_CONFIG_GET_SLOTS_FINGERPRINTS = 0x8d
    U2F_CONFIG_TEST_CONFIG = 0x8e

    U2F_CUSTOM_RNG = 0x21
    U2F_CUSTOM_SEED = 0x22
    U2F_CUSTOM_WIPE = 0x23
    U2F_CUSTOM_WINK = 0x24
    
    U2F_HID_INIT = 0x86
    U2F_HID_PING = 0x81


if len(sys.argv) not in [2,3,4,5,6]:
    print('usage: %s <action> [<arguments>] [-s serial-number]' % sys.argv[0])
    print('actions: ')
    print("""   configure <ecc-private-key>: setup the device configuration.
    Specify ECC P-256 private key for token attestation.  Specify temporary output file for generated
    keys.""")
    print('     rng: Continuously dump random numbers from the devices hardware RNG.')
    print('     seed: update the hardware RNG seed with input from stdin')
    print('     list: list all connected U2F Zero tokens.')
    print('     wink: blink the LED')
    print('     ping <bytes count>: test ping capabilities of the device')
    print('     bootloader-destroy: permanently disable the bootloader')
    print('     fingerprints: print data slots fingerprints (debug firmware only)')
    sys.exit(1)

def open_u2f(SN=None):
    h = hid.device()
    try:
        h.open(0x20a0,0x4287,SN if SN is None else unicode(SN))
        if SN is None:
            SN = h.get_serial_number_string()
        print('opened ', SN)
    except IOError as ex:
        print( ex)
        if SN is None: print( 'U2F Zero not found')
        else: print ('U2F Zero %s not found' % SN)
        sys.exit(1)
    return h


def do_list():
    for d in hid.enumerate(0x20a0, 0x4287):
        if not d['serial_number']:
            print('\n!!! Serial number is empty. Try to run the tool with administrator privileges (e.g. with `sudo`).\n')
        keys = list(d.keys())
        keys.sort()
        for key in keys:
            print("%s : %s" % (key, d[key]))
    print('')


def die(msg):
    print( msg)
    sys.exit(1)


def feed_crc(crc, b):
    crc ^= b
    crc = (crc >> 1) ^ 0xa001 if crc & 1 else crc >> 1
    crc = (crc >> 1) ^ 0xa001 if crc & 1 else crc >> 1
    crc = (crc >> 1) ^ 0xa001 if crc & 1 else crc >> 1
    crc = (crc >> 1) ^ 0xa001 if crc & 1 else crc >> 1
    crc = (crc >> 1) ^ 0xa001 if crc & 1 else crc >> 1
    crc = (crc >> 1) ^ 0xa001 if crc & 1 else crc >> 1
    crc = (crc >> 1) ^ 0xa001 if crc & 1 else crc >> 1
    crc = (crc >> 1) ^ 0xa001 if crc & 1 else crc >> 1
    return crc

def reverse_bits(crc):
    crc = (((crc & 0xaaaa) >> 1) | ((crc & 0x5555) << 1))
    crc = (((crc & 0xcccc) >> 2) | ((crc & 0x3333) << 2))
    crc = (((crc & 0xf0f0) >> 4) | ((crc & 0x0f0f) << 4))
    return (((crc & 0xff00) >> 8) | ((crc & 0x00ff) << 8))

def get_crc(data):
    crc = 0
    for i in data:
        crc = feed_crc(crc,ord(i))
    crc = reverse_bits(crc)
    crc2 = crc & 0xff
    crc1 = (crc>>8) & 0xff
    return [crc1,crc2]

def read_n_tries(dev,tries,num,wait):
    data = None
    for i in range(0,tries-1):
        try:
            return dev.read(num,wait)
        except:
            time.sleep(.1)
            pass
    return dev.read(num,wait)

def get_write_mask(key):
    m = hashlib.new('sha256')
    m.update(key + '\x15\x02\x01\x00\xee\x01\x23' + ('\x00'*57))
    h1 = m.hexdigest()
    m = hashlib.new('sha256')
    m.update(binascii.unhexlify(h1))
    h2 = m.hexdigest()

    return h1 + h2[:8]


def do_generate_device_key(h):
    h.write([0,commands.U2F_CONFIG_LOAD_TRANS_KEY])
    data = read_n_tries(h,5,64,1000)
    if data[1] != 1:
        die('failed generating master key')
    print(repr(data))

def _call_atec_command(h, opcode, param1, param2, data):
    atecc_cmd = [opcode, param1, param2, len(data)] + data
    h.write([0,commands.U2F_CONFIG_ATECC_PASSTHROUGH]+atecc_cmd)
    res = read_n_tries(h, 5, 64, 1000)
    cmd = res[0]
    errcode = res[1]
    ret_data = res[2:]
    return errcode, ret_data

def do_passt(h):
#   use passthrough
    data = [0]*50
    res = _call_atec_command(h, ATECC.CMD_RNG, ATECC.RNG_P1, ATECC.RNG_P2, data)
    print(repr(res))

    res = _call_atec_command(h, ATECC.CMD_RNG, ATECC.RNG_P1, ATECC.RNG_P2, data)
    print(repr(res))


def next_i(iterator, count):
    res = []
    try:
        for i in range(count):
            res.append(next(iterator))
    except:
        pass
    return res

def do_configure(h,pemkey):

    if not os.path.exists(pemkey):
        die('Pemkey path does not exist: '+pemkey)

    config = "\x01\x23\x6d\x10\x00\x00\x50\x00\xd7\x2c\xa5\x71\xee\xc0\x85\x00\xc0\x00\x55\x00\x83\x71\x81\x01\x83\x71\xC1\x01\x83\x71\x83\x71\x83\x71\xC1\x71\x01\x01\x83\x71\x83\x71\xC1\x71\x83\x71\x83\x71\x83\x71\x83\x71\xff\xff\xff\xff\x00\x00\x00\x00\xff\xff\xff\xff\x00\x00\x00\x00\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\x00\x00\x55\x55\xff\xff\x00\x00\x00\x00\x00\x00\x13\x00\x3C\x00\x13\x00\x3C\x00\x13\x00\x3C\x00\x13\x00\x3C\x00\x3c\x00\x3C\x00\x13\x00\x3C\x00\x13\x00\x3C\x00\x13\x00\x33\x00"


    h.write([0,commands.U2F_CONFIG_IS_BUILD])
    data = h.read(64,1000)
    if len(data) >= 2 and data[1] == 1:
        print('Device is configured.')
    else:
        print(repr(data))
        die('Device not configured. Try to power-cycle (e.g. reinsert) the device.')

    time.sleep(0.250)

    h.write([0,commands.U2F_CONFIG_GET_SERIAL_NUM])
    while True:
        data = read_n_tries(h,5,64,1000)
        l = data[1]
        print( 'read %i bytes' % l)
        if data[0] == commands.U2F_CONFIG_GET_SERIAL_NUM:
            break
    print( data)
    config = array.array('B',data[2:2+l]).tostring() + config[l:]
    print( 'conf: ', binascii.hexlify(config))
    time.sleep(0.250)


    crc = get_crc(config)
    print( 'crc is ', [hex(x) for x in crc])
    h.write([0,commands.U2F_CONFIG_LOCK] + crc)
    data = read_n_tries(h,5,64,1000)

    if data[1] == 1:
        print( 'locked eeprom with crc ',crc)
    else:
        die('not locked')

    time.sleep(0.250)


    h.write([0,commands.U2F_CONFIG_LOAD_WRITE_KEY])
    data = read_n_tries(h,5,64,1000)
    if data[1] != 1:
        print('recv wkey: ' + repr(data))
        die('failed loading write key ({})'.format(data[1]))

    h.write([0,commands.U2F_CONFIG_LOAD_READ_KEY])
    data = read_n_tries(h,5,64,1000)
    if data[1] != 1:
        die('failed loading read key')


    attestkey = ecdsa.SigningKey.from_pem(open(pemkey).read())
    if len(attestkey.to_string()) != 32:
        die('Incorrect key type.  Must be prime256v1 ECC private key in PEM format.')


    h.write([0,commands.U2F_CONFIG_LOAD_ATTEST_KEY] + [ord(x) for x in attestkey.to_string()])
    data = read_n_tries(h,5,64,1000)
    if len(data)<2 or data[1] != 1:
        print(data[:2])
        die('failed loading attestation key')


    h.write([0, commands.U2F_CONFIG_GEN_DEVICE_KEY])
    data = read_n_tries(h, 5, 64, 1000)
    if data[1] != 1:
        die('failed generating device key' + repr(data[:2]))



    data_i = iter(data)
    next_i(data_i, 2)
    print('generated device key: ' + repr(next_i(data_i, 16)))
    print('written device key hash: ' + repr(next_i(data_i, 16)))
    print('generated u2f_zero_const: ' + repr(next_i(data_i, 16)))
    print('full response: ' + repr(data))


    print( 'Done.  Erasing bootloader code pages on MCU.')
    h.write([0,commands.U2F_CONFIG_BOOTLOADER_DESTROY])
    data = read_n_tries(h,5,64,1000)
    if len(data)<2 or data[1] != 1:
        print(data)
        die('failed to remove the bootloader.')
    else:
        print('Device bootloader mode removed.')


def bootloader_destroy(h):
    print( 'Erasing bootloader code pages on MCU.')
    h.write([0,commands.U2F_CONFIG_BOOTLOADER_DESTROY])
    data = read_n_tries(h,5,64,1000)
    if len(data)<2 or data[1] != 1:
        print(data)
        die('failed to remove the bootloader.')
    else:
        print('Device bootloader mode removed.')



def do_rng(h):
    cmd = [0,0xff,0xff,0xff,0xff, commands.U2F_CUSTOM_RNG, 0,0]
    # typically runs around 700 bytes/s
    while True:
        h.write(cmd)
        rng = h.read(64,1000)
        if not rng or rng[4] != commands.U2F_CUSTOM_RNG:
            sys.stderr.write('error: device error\n')
        else:
            if rng[6] != 32:
                sys.stderr.write('error: device error\n')
            else:
                data = array.array('B',rng[6+1:6+1+32]).tostring()
                sys.stdout.write(data)
                sys.stdout.flush()

def do_seed(h):
    cmd = cmd_prefix + [ commands.U2F_CUSTOM_SEED, 0,20]
    num = 0
    # typically runs around 414 bytes/s
    def signal_handler(signal, frame):
        print('seeded %i bytes' % num)
        sys.exit(0)
    signal.signal(signal.SIGINT, signal_handler)
    while True:
        # must be 20 bytes or less at a time
        c = sys.stdin.read(20)
        if not c:
            break
        buf = [ord(x) for x in c]
        h.write(cmd + buf)
        res = h.read(64, 1000)
        if not res or res[7] != 1:
            sys.stderr.write('error: device error\n')
        else:
            num += len(c)

    h.close()

def do_wipe(h):
    cmd = cmd_prefix + [ commands.U2F_CUSTOM_WIPE, 0,0]
    h.write(cmd)
    print( 'Press U2F button repeatedly until the LED is no longer red.')
    res = None
    while not res:
        res = h.read(64, 10000)
    if res[7] != 1:
        print( 'Wipe failed')
    else:
        print( 'Wipe succeeded')


    h.close()

def hexcode2bytes(color):
    h = [ord(x) for x in color.replace('#','').decode('hex')]
    return h

def do_wink(h):
    cmd = cmd_prefix + [ commands.U2F_CUSTOM_WINK, 0,0]
    h.write(cmd)

def u2fhid_init(h):
    nonce = [random.randint(0, 0xFF) for i in xrange(0, 8)]
    cmd = cid_broadcast + [commands.U2F_HID_INIT, 0, 8] + nonce
    h.write([0] + cmd)
    ans = h.read(19, 1000)
    return ans[15:19]
	
def get_response_packet_payload(cmd_seq):                # Reads an U2FHID packet, checks it's command/sequence field by the given parameter, returns the payload 
    ans = h.read(64, 200)                                
    if len(ans) == 0:                                    # Read timeout
        return[]
    else:                                                # Read success
        print('pkt recvd(%i):' % ans[4])
        print(" ".join('%03d'%x for x in ans))
        if ans[4] == cmd_seq:                            # Error check: OK
            if cmd_seq >= 128:                           # Initialization packet                                
                return ans[7:]                           # Payload
            else:                                        # Sequence packet
                return ans[5:]                           # Payload 
        die('ERR: cmd/seq field in received packet is %i instead of %i' % (ans[4], cmd_seq)) # Error check: ERR (1st byte of payload is the error code in case of a command packet)
            
 
def do_ping(h, num):
    # Init (set U2F HID channel address)
    cid = u2fhid_init(h)
    
    # Prepare ping data
    dlen = int(num)
    data = [random.randint(1, 0xFF) for i in xrange(0, dlen)]     # Ping data: random bytes, except 0
    data_req = data
    
    # Send initialization packet of request message
    cmd = cid + [ commands.U2F_HID_PING, int((dlen & 0xFF00) >> 8), int(dlen & 0x00FF)] + data[0:57] # Send ping command
    h.write([0] + cmd)
    print('init. pkt sent(%i):' % cmd[4])
    print(" ".join('%03d'%x for x in cmd))

    # Request/Response transfer in case of one packet composed message
    data_resp = []
    if dlen < 58:                                                 # Message fits into one packet, no continuation packets will be sent
        data_resp = get_response_packet_payload(commands.U2F_HID_PING)
    
    # Request/Response transfer in case of a multiple packet composed message
    else:                                                         # Message doesnt fit into one packet
        seq_tx = 0
        seq_rx = commands.U2F_HID_PING
        data = data[57:]
        resp_dlen = 0
        while len(data) > 0:                                      # Send remaining data of request message in continuation packets                           
            cmd = cid + [seq_tx] + data[:64 - 5]
            h.write([0] + cmd)
            data = data[64 - 5:]
            print('cont. pkt sent(%i):' % seq_tx)
            print(" ".join('%03d'%x for x in cmd))
            seq_tx += 1
            
            ans = get_response_packet_payload(seq_rx)             # Collect response message packets in the meantime to avoid input buffer ovf
            if len(ans):
                if seq_rx == commands.U2F_HID_PING:
                    seq_rx = 0
                else:
                    seq_rx += 1
                data_resp += ans
        print('Collecting remaining  data...')
        while True:                                               # Collect remaining response message packets after the total request message has been sent                               
            ans = get_response_packet_payload(seq_rx)
            if len(ans):
                if seq_rx == commands.U2F_HID_PING:
                    seq_rx = 0
                else:
                    seq_rx += 1
                data_resp += ans
            else:
                break;
    resp_dlen = len(data_resp)
    
    # Adjust ping data: remove padding zeros from the end
    for i in reversed(range(resp_dlen)):
        if data_resp[i] == 0:
            data_resp.pop(i)
        else:
            break
    
    # Check ping data (compare sent/received message payloads)
    # cut response to compare only sent data
    data_resp = data_resp[:len(data_req)]
    if data_req == data_resp:
        print('Ping OK')
    else:
        print('Ping ERR')
        print('{} {}'.format(len(data_req), len(data_resp)))


def do_config_test(h):
    h.write([0, commands.U2F_CONFIG_TEST_CONFIG])
    data = read_n_tries(h, 5, 64, 1000)
    print (len(data), repr(data))

def do_fingerprints(h):
    print('Get data slots fingerprints')
    h.write([0, commands.U2F_CONFIG_GET_SLOTS_FINGERPRINTS])
    data = read_n_tries(h, 5, 64, 1000)
    print (len(data), repr(data))
    if len(data) < 2:
        return
    data_i = iter(data)
    print('status', repr(next_i(data_i, 2)))
    for i in range(16):
        print('{}: {}'.format(i, repr(next_i(data_i, 3))))
    print()


if __name__ == '__main__':
    action = sys.argv[1].lower()
    h = None
    SN = None
    if '-s' in sys.argv:
        if sys.argv.index('-s') + 1 > len(sys.argv):
            print('need serial number')
            sys.exit(1)
        SN = sys.argv[sys.argv.index('-s') + 1]

    if action == 'configure':
        if len(sys.argv) < 3:
            print('error: need ecc private key')
            sys.exit(1)
        h = open_u2f(SN)
        do_configure(h, sys.argv[2])
    elif action == 'generate_device_key':
        h = open_u2f(SN)
        do_generate_device_key(h)
    elif action == 'rng':
        h = open_u2f(SN)
        do_rng(h)
    elif action == 'seed':
        h = open_u2f(SN)
        do_seed(h)
    elif action == 'wipe':
        h = open_u2f(SN)
        do_wipe(h)
    elif action == 'passt':
        h = open_u2f(SN)
        do_passt(h)
    elif action == 'list':
        do_list()
    elif action == 'config-test':
        h = open_u2f(SN)
        do_config_test(h)
    elif action == 'wink':
        h = open_u2f(SN)
        do_wink(h)
    elif action == 'fingerprints':
        h = open_u2f(SN)
        do_fingerprints(h)
    elif action == 'bootloader-destroy':
        h = open_u2f(SN)
        bootloader_destroy(h)
    elif action == 'ping':
        if len(sys.argv) < 3:
            print('error: need bytes count to send')
            sys.exit(1)
        h = open_u2f(SN)
        do_ping(h, sys.argv[2])
    else:
        print( 'error: invalid action: ', action)
        sys.exit(1)

    if h is not None: h.close()
