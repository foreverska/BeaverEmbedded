#ifndef DATASTORE_DATASTORE_H_
#define DATASTORE_DATASTORE_H_

#include <stdint.h>
#include <stdbool.h>

#define DATASTORE_OK        (0)
#define DATASTORE_INVALID   (-1)
#define DATASTORE_NOPERM    (-2)
#define DATASTORE_NOSEC     (-3)

#define DATASTORE_ENTRIES   (4)
#define ENTRY_INPUTS        (0)
#define ENTRY_OUTPUTS       (1)
#define ENTRY_OUTHOLDS      (2)
#define ENTRY_OUTHOLDVALS   (3)

#define OUT_LLB             (0x00000001)
#define OUT_RLB             (0x00000002)
#define OUT_LHB             (0x00000004)
#define OUT_RHB             (0x00000008)
#define OUT_HEADLIGHT_MASK  (0x0000000F)
#define OUT_FLTS            (0x00000010)
#define OUT_FRTS            (0x00000020)
#define OUT_RLTS            (0x00000040)
#define OUT_RRTS            (0x00000080)
#define OUT_TURN_MASK       (0x000000F0)

void InitDatastore(void);

int32_t ReadId(uint16_t id, uint32_t *pValue, bool internal);
int32_t MaskedReadId(uint16_t id, uint32_t *pValue, uint32_t bitmask, bool internal);
int32_t WriteId(uint16_t id, uint32_t value, bool internal);
int32_t MaskedWriteId(uint16_t id, uint32_t value, uint32_t bitmask, bool internal);

#endif /* DATASTORE_DATASTORE_H_ */
