# Secure element's (ATECC508A) configuration
In this document a current configuration (same with `nk/v0.1` tag), locked on ATECC chip, is described. Once the configuration is locked, it cannot be further changed in the lifetime of the chip.


## General configuration
As of `nk/v0.1` tag, ATECC508A chip is configured as follows:

```
config dump:
0123bcae00005000455b669beec04500c0005500837181018371c10183718371
8371c171010183718371c1718371837183718371000000ff00020002ffffffff
00000000ffffffffffffffffffffffffffffffff00000000ffff000000000000
13003c0013003c0013003c0013003c003c003c0013003c0013003c0013003300
current config crc:ce2d
```

Configuration is written to the chip at the very first start up with [atecc508a.c#L348](https://github.com/Nitrokey/nitrokey-fido-u2f-firmware/blob/nk/v0.1/firmware/src/atecc508a.c#L348), and locked by the `client.py` tool using `U2F_CONFIG_LOCK` command ([atecc508a.c#L589](https://github.com/Nitrokey/nitrokey-fido-u2f-firmware/blob/nk/v0.1/firmware/src/atecc508a.c#L589)).

## Slots' configuration in detail
There are 16 data slots, each having separate slot and key configurations.
Meaning of the fields in detail describe chapters 2.2.1 and 2.2.5 of ATECC508A Complete Data Sheet.

### Brief description of used values
#### Slot config
- `writekey`: slot number to use for write authorization; all keys' writes currently are authorized by slot `1`;
- `writeconfig`: what commands could be used with given slot:
    - `7` - encrypted write, genkey, derivekey; 
    - `0` - clear text write only;
- `encread`: 
    - `1` - read will be encrypted, as configured in readkey bits; 
    - `0` - read is not encrypted;
- `readkey`: 
    - if (`private_key`==`true`): 
        `slot_no` to use for encryption for read command;
    - if (`private_key`==`false`): 
        - `1` - could be used to sign; 
        - `3` - internal signatures of Gendig or GenKey are enabled

#### Key config

- `lockable`: determines, when slot can be locked with the `LOCK` command. See `slot locked` field in the Configuration zone to determine current lock state.
    - `1` - slot can be locked;
    - `0` - cannot be locked; 
- `keytype`: 
    - `7` - key in the slot is not ECC; 
    - `4` - this is an ECC key;
- `(keytype, pubinfo,private)`: 
    - `(4,1,1)`: contains ECC private key, for which public key can be generated;
    - `(7,0,0)`: contains other data or SHA key.

### Slots configuration data
Slots' details were interpreted using EFM8UB10 chip and written to the serial line. After confrontation with the ATECC508A's Data Sheet, they appear to be valid. Slots are numbered by hex digit, and separated with `---`. Data structures are available in the firmware and defined at [atecc508a.h#L134](https://github.com/Nitrokey/nitrokey-fido-u2f-firmware/blob/nk/v0.1/firmware/inc/atecc508a.h#L134). 

```
Slot config: 0
hex: 8371
writeconfig: 7
writekey: 1
secret: 1
encread: 0
limiteduse: 0
nomac: 0
readkey: 3

Key config: 0
hex: 1300
x509id: 0
rfu: 0
intrusiondisable: 0
authkey: 0
reqauth: 0
reqrandom: 0
lockable: 0
keytype: 4
pubinfo: 1
private: 1

---

Slot config: 1
hex: 8101
writeconfig: 0
writekey: 1
secret: 1
encread: 0
limiteduse: 0
nomac: 0
readkey: 1

Key config: 1
hex: 3c00
x509id: 0
rfu: 0
intrusiondisable: 0
authkey: 0
reqauth: 0
reqrandom: 0
lockable: 1
keytype: 7
pubinfo: 0
private: 0

---

Slot config: 2
hex: 8371
writeconfig: 7
writekey: 1
secret: 1
encread: 0
limiteduse: 0
nomac: 0
readkey: 3

Key config: 2
hex: 1300
x509id: 0
rfu: 0
intrusiondisable: 0
authkey: 0
reqauth: 0
reqrandom: 0
lockable: 0
keytype: 4
pubinfo: 1
private: 1

---

Slot config: 3
hex: c101
writeconfig: 0
writekey: 1
secret: 1
encread: 1
limiteduse: 0
nomac: 0
readkey: 1

Key config: 3
hex: 3c00
x509id: 0
rfu: 0
intrusiondisable: 0
authkey: 0
reqauth: 0
reqrandom: 0
lockable: 1
keytype: 7
pubinfo: 0
private: 0

---

Slot config: 4
hex: 8371
writeconfig: 7
writekey: 1
secret: 1
encread: 0
limiteduse: 0
nomac: 0
readkey: 3

Key config: 4
hex: 1300
x509id: 0
rfu: 0
intrusiondisable: 0
authkey: 0
reqauth: 0
reqrandom: 0
lockable: 0
keytype: 4
pubinfo: 1
private: 1

---

Slot config: 5
hex: 8371
writeconfig: 7
writekey: 1
secret: 1
encread: 0
limiteduse: 0
nomac: 0
readkey: 3

Key config: 5
hex: 3c00
x509id: 0
rfu: 0
intrusiondisable: 0
authkey: 0
reqauth: 0
reqrandom: 0
lockable: 1
keytype: 7
pubinfo: 0
private: 0

---

Slot config: 6
hex: 8371
writeconfig: 7
writekey: 1
secret: 1
encread: 0
limiteduse: 0
nomac: 0
readkey: 3

Key config: 6
hex: 1300
x509id: 0
rfu: 0
intrusiondisable: 0
authkey: 0
reqauth: 0
reqrandom: 0
lockable: 0
keytype: 4
pubinfo: 1
private: 1

---

Slot config: 7
hex: c171
writeconfig: 7
writekey: 1
secret: 1
encread: 1
limiteduse: 0
nomac: 0
readkey: 1

Key config: 7
hex: 3c00
x509id: 0
rfu: 0
intrusiondisable: 0
authkey: 0
reqauth: 0
reqrandom: 0
lockable: 1
keytype: 7
pubinfo: 0
private: 0

---

Slot config: 8
hex: 0101
writeconfig: 0
writekey: 1
secret: 0
encread: 0
limiteduse: 0
nomac: 0
readkey: 1

Key config: 8
hex: 3c00
x509id: 0
rfu: 0
intrusiondisable: 0
authkey: 0
reqauth: 0
reqrandom: 0
lockable: 1
keytype: 7
pubinfo: 0
private: 0

---

Slot config: 9
hex: 8371
writeconfig: 7
writekey: 1
secret: 1
encread: 0
limiteduse: 0
nomac: 0
readkey: 3

Key config: 9
hex: 3c00
x509id: 0
rfu: 0
intrusiondisable: 0
authkey: 0
reqauth: 0
reqrandom: 0
lockable: 1
keytype: 7
pubinfo: 0
private: 0

---

Slot config: a
hex: 8371
writeconfig: 7
writekey: 1
secret: 1
encread: 0
limiteduse: 0
nomac: 0
readkey: 3

Key config: a
hex: 1300
x509id: 0
rfu: 0
intrusiondisable: 0
authkey: 0
reqauth: 0
reqrandom: 0
lockable: 0
keytype: 4
pubinfo: 1
private: 1

---

Slot config: b
hex: c171
writeconfig: 7
writekey: 1
secret: 1
encread: 1
limiteduse: 0
nomac: 0
readkey: 1

Key config: b
hex: 3c00
x509id: 0
rfu: 0
intrusiondisable: 0
authkey: 0
reqauth: 0
reqrandom: 0
lockable: 1
keytype: 7
pubinfo: 0
private: 0

---

Slot config: c
hex: 8371
writeconfig: 7
writekey: 1
secret: 1
encread: 0
limiteduse: 0
nomac: 0
readkey: 3

Key config: c
hex: 1300
x509id: 0
rfu: 0
intrusiondisable: 0
authkey: 0
reqauth: 0
reqrandom: 0
lockable: 0
keytype: 4
pubinfo: 1
private: 1

---

Slot config: d
hex: 8371
writeconfig: 7
writekey: 1
secret: 1
encread: 0
limiteduse: 0
nomac: 0
readkey: 3

Key config: d
hex: 3c00
x509id: 0
rfu: 0
intrusiondisable: 0
authkey: 0
reqauth: 0
reqrandom: 0
lockable: 1
keytype: 7
pubinfo: 0
private: 0

---

Slot config: e
hex: 8371
writeconfig: 7
writekey: 1
secret: 1
encread: 0
limiteduse: 0
nomac: 0
readkey: 3

Key config: e
hex: 1300
x509id: 0
rfu: 0
intrusiondisable: 0
authkey: 0
reqauth: 0
reqrandom: 0
lockable: 0
keytype: 4
pubinfo: 1
private: 1

---

Slot config: f
hex: 8371
writeconfig: 7
writekey: 1
secret: 1
encread: 0
limiteduse: 0
nomac: 0
readkey: 3

Key config: f
hex: 3300
x509id: 0
rfu: 0
intrusiondisable: 0
authkey: 0
reqauth: 0
reqrandom: 0
lockable: 1
keytype: 4
pubinfo: 1
private: 1
```

