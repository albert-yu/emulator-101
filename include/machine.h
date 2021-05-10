#ifndef MACHINE_H
#define MACHINE_H

#include <inttypes.h>

typedef struct machine_t {
    uint16_t shift_register;
} Machine;

uint8_t machine_in(uint8_t port);

#endif
