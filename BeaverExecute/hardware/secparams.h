#ifndef HARDWARE_SECPARAMS_H_
#define HARDWARE_SECPARAMS_H_

#include "stdint.h"

#include "chacha-portable/chacha-portable.h"

__attribute__ ((section(".secp"))) struct {
    uint8_t SymmetricKey[CHACHA20_KEY_SIZE];
}SecParams;

uint8_t ChaChaNonce[CHACHA20_NONCE_SIZE] = {0};

#endif /* HARDWARE_SECPARAMS_H_ */
