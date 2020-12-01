#include <string.h>

#include "datastore.h"
#include "comms/uds.h"

#define PERM_READ   0x01
#define PERM_WRITE  0x02

typedef struct {
    uint32_t curValue;
    uint8_t permissions;
    uint8_t minsec;
} tDataValue;

tDataValue dataStore[DATASTORE_ENTRIES];

void InitDatastore(void)
{
    memset(&dataStore, 0x00, sizeof(dataStore));
    dataStore[ENTRY_INPUTS].permissions = PERM_READ;
    dataStore[ENTRY_OUTPUTS].permissions = PERM_READ;

    dataStore[ENTRY_OUTHOLDS].permissions = PERM_READ | PERM_WRITE;
    dataStore[ENTRY_OUTHOLDVALS].permissions = PERM_READ | PERM_WRITE;
}

int32_t ReadId(uint16_t id, uint32_t *pValue, bool internal)
{
    if (id >= DATASTORE_ENTRIES)
    {
        return DATASTORE_INVALID;
    }

    if ((internal == false) && ((dataStore[id].permissions & PERM_READ) == 0x00))
    {
        return DATASTORE_NOPERM;
    }

    if ((internal == false) && (GetSecurityLevel() < dataStore[id].minsec))
    {
        return DATASTORE_NOSEC;
    }

    *pValue = dataStore[id].curValue;
    return DATASTORE_OK;
}

int32_t MaskedReadId(uint16_t id, uint32_t *pValue, uint32_t bitmask, bool internal)
{
    int32_t retval = ReadId(id, pValue, internal);
    if (retval == DATASTORE_OK)
    {
        *pValue &= bitmask;
        return DATASTORE_OK;
    }

    return retval;
}

int32_t WriteId(uint16_t id, uint32_t value, bool internal)
{
    if (id >= DATASTORE_ENTRIES)
    {
        return DATASTORE_INVALID;
    }

    if ((internal == false) && ((dataStore[id].permissions & PERM_WRITE) == 0x00))
    {
        return DATASTORE_NOPERM;
    }

    if ((internal == false) && (GetSecurityLevel() < dataStore[id].minsec))
    {
        return DATASTORE_NOSEC;
    }

    dataStore[id].curValue = value;
    return DATASTORE_OK;
}

int32_t MaskedWriteId(uint16_t id, uint32_t value, uint32_t bitmask, bool internal)
{
    uint32_t maskedVal;
    int32_t readRet = ReadId(id, &maskedVal, internal);
    if (readRet == DATASTORE_OK)
    {
        maskedVal = (value & bitmask) | (maskedVal & ~bitmask);
    }

    return WriteId(id, maskedVal, internal);
}
