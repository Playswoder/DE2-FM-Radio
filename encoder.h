#ifndef ENCODER_H
#define ENCODER_H

#include <stdint.h>

// Initialize encoder hardware
void encoder_init(void);

// Poll encoder and return direction: +1 = CW, -1 = CCW, 0 = no change
int8_t encoder_poll(void);

#endif
