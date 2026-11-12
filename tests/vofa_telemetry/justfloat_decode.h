#ifndef TEST_JUSTFLOAT_DECODE_H
#define TEST_JUSTFLOAT_DECODE_H

#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

/* Decode the actual wire bytes, including the delimiter search used by VOFA.
 * Integer-semantic channels are rounded before checking metadata/bit masks.
 * int64_t also represents INT32_MAX rounded to 2147483648 by float32. */
static void ReadJustFloatFields(const uint8_t *data, uint16_t length,
                                int64_t *fields, unsigned count)
{
    const uint8_t tail[] = {0x00, 0x00, 0x80, 0x7f};
    assert(length == count * 4U + sizeof tail);
    for (unsigned offset = 0; offset + sizeof tail <= length; ++offset) {
        if (memcmp(data + offset, tail, sizeof tail) == 0) {
            assert(offset == count * 4U);
            break;
        }
    }
    assert(memcmp(data + count * 4U, tail, sizeof tail) == 0);
    for (unsigned i = 0; i < count; ++i) {
        float value;
        memcpy(&value, data + i * 4U, sizeof value);
        assert(isfinite(value));
        assert(value >= -2147483904.0 && value <= 2147483904.0);
        fields[i] = llround((double)value);
    }
}

#endif
