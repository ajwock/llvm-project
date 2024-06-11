#include <stdint.h>

int8_t func(int8_t input) {
    if (input > 0x80) {
        return input;
    } else {
        return -input;
    }
}
