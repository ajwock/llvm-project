#include <stdint.h>

int8_t func2(int8_t input) {
    if (input > 0x80) {
        return -1;
    } else {
        return 1;
    }
}
