# FIDO U2F Security Architecture

This description is reduced to the relevant aspects so that length fields and other fields are not mentioned (indicated with “...”). The complete overview of request and response fields is covered in the official FIDO specifications. 
In this document "`||`" symbol means data concatenation.

Global use counter is set to zero initially. It is defined per-device and incremented for each registration and authentication.

During both registration and authentication procedures, private keys are generated. These have to be transferred temporary between MCU and ATECC chip to perform further cryptographic operations. To maintain security, they are XORed with both `wkey` and `rkey` to keep the I2C bus communication encrypted:
- `rkey` is present only on the MCU. This is a XOR-key for I2C bus encryption on read (modifying data read from ATECC chip). 
- `wkey` is created on the host from the SHA-256 hash of the `device_key` number. Similarly to `wkey`, it is present only on the MCU. This is a XOR-key for I2C bus encryption on write (to the ATECC chip).

# Phases

## Certificate and attestation key generation

Certificate and attestation key are generated on the vendor's computer in this phase and stored on hard drive.

## Configuration / SETUP phase

1. RNG is run on the host to produce three 32-byte random numbers for `device key`, `_wkey` and `_rkey`. The former is the device-specific secret for U2F functionality which - after setup - won't leave the ATECC. The latter two will be used during the device's work for encrypting communication with the ATECC chip on the I2C bus.
2. `device key` is loaded to the ATECC chip unmodified.
3. `_rkey` and `_wkey` are SHA256-hashed with a known constant. The outcomes are target `rkey` and `wkey` respectively, and written to the FINAL firmware (to the `cert.c` file).
4. `wkey` is loaded to the device and kept in the RAM.
5. Attestation key is sent to the device and saved to RAM. 
6. A key hash is calculated to authenticate its encrypted write to the ATECC chip.
7. Attestation key is XORed with `wkey` and written to the ATECC chip using previously calculated key hash MAC.


![Keys handling diagram - setup][keys-setup]



## Firmware compilation
During the compilation the attestation certificate and `wkey`/`rkey` constants are included directly into the firmware files as static binary blobs.


## Registration
 
### Registration Request:
- challenge parameter: SHA-256 hash 
- application parameter: SHA-256 hash of application identity. This parameter could be also wrongly phrased as AppID, while both are actually different (but related) values.

### Registration calculation:
1. Generate random nonce from RNG
2. `private key := HMAC-SHA256(device key, application parameter || nonce)` - this private key is application-specific
3. `user public key := derive from private key`
4. `MAC := HMAC-SHA256(device key, private key || application parameter)`
5. `Key handle := nonce || MAC`
6. `signature := ecdsa(private key, application parameter || challenge parameter || key handle || user public key || ...)`

### Registration Response:
- A user public key [65 bytes]. This is the (uncompressed) x,y-representation of a curve point on the P-256 NIST elliptic curve.
- key handle: This a handle that allows the U2F token to identify the generated key pair. U2F tokens may wrap the generated private key and the  application id it was generated for, and output that as the key handle. 
- signature: This is a ECDSA signature (on P-256) over the following byte string:
    - The application parameter [32 bytes] from the registration request message.
    - The challenge parameter[32 bytes] from the registration request message.
    - The above key handle [variable length]. (Note that the key handle length is not included in the signature base string. This doesn't cause confusion in the signature base string, since all other parameters in the signature base string are fixed-length.)
    - The above user public key [65 bytes].
    - ...

![Registration diagram][register]

### Transport Encryption During Registration
1. `priv_user_key_rkey := priv_user_key ^ rkey` - a generated private key is XORed with `rkey`.
2. `key_write_auth_mac := sha256(wkey || priv_user_key_rkey || privwrite_const)` - the modified key is used to compute write authentication hash for ATECC chip.
3. `priv_user_key_rkey_wkey := priv_user_key_rkey ^ wkey` - the modified key is once again XORed with `wkey`
4. `privwrite(priv_user_key_rkey_wkey, device_key || key_write_auth_mac)`  - private key is saved to the device using previously calculated write hash.
5. `priv_user_key_rkey:= priv_user_key_rkey_wkey ^ wkey` - on ATECC the received data is decrypted with `wkey`, leaving `rkey` mask applied to the sent key. 

![Keys handling diagram - registration][keys-registration]


## Authentication
### Authentication Request:
- The challenge parameter [32 bytes]. The challenge parameter is the SHA-256 hash of the Client Data, a stringified JSON data structure that the FIDO Client prepares. Among other things, the Client Data contains the challenge from the relying party (hence the name of the parameter). See below for a detailed explanation of Client Data.
- The application parameter [32 bytes]. The application parameter is the SHA-256 hash of the UTF-8 encoding of the application identity of the application requesting the authentication as provided by the relying party.
- A key handle [length specified in previous field]. The key handle. This is provided by the relying party, and was obtained by the relying party during registration. `Key handle = nonce || MAC`
- …

### Authentication calculation:
1. `nonce := subtract from key handle`
2. `MAC1 := subtract from key handle`
3. `private key := HMAC-SHA256(device key, application parameter || nonce)` - this is the same calculation as during registration
4. `MAC2 := HMAC-SHA256(device key, private key || application parameter)`
5. `verify: MAC1 == MAC2`  - during authentication, the MAC helps to ensure that a key handle is only valid for the particular combination of device and AppID that it was created for during registration
6. `signature := ecdsa(private key, application parameter || challenge parameter || global use counter || ...)`
7. increment global use counter

### Authentication Response:
- A signature. This is a ECDSA signature (on P-256) over the following byte string:
- The application parameter [32 bytes] from the authentication request message.
- The challenge parameter [32 bytes] from the authentication request message.
- ...

![Authentication diagram][auth]

### Transport Encryption During Authentication
Authentication differs from Registration usage by not computing the write MAC for the ATECC chip, and instead reusing the `key_handle`, which contains the computed on registration value. 

![Keys handling diagram - authentication][keys-authentication]




[register]: images/u2f-registration.png "U2F Registeration"
[auth]: images/u2f-authentication.png "U2F Authorization"
[keys-setup]: images/keys_usage-configuration.png "Keys handling diagram - setup"
[keys-registration]: images/keys_usage-u2f_registration.png "Keys handling diagram - registration"
[keys-authentication]: images/keys_usage-u2f_authentication.png "Keys handling diagram - authentication"

