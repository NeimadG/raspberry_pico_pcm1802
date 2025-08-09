

#ifndef PICO_PCM_1802
#define PICO_PCM_1802

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"


extern void init_PCM1802();
extern void start_PCM1802();
extern void release_PCM1802();
extern int32_t* get_buffer();

#define PIN_DOUT 2
#define PIN_LRCK 4
#define PIN_BCLK 6
#define PIN_PCM1802_SYSCLK 21
#define PCM1802_PIO_INST pio0

// for example, 0.1s of capture
#define CAPTURE_CHANNEL_COUNT 2
#define CAPTURE_FRAME_NUMBER 4410
#define CAPTURE_BUFFER_WORD_COUNT (CAPTURE_FRAME_NUMBER*CAPTURE_CHANNEL_COUNT)

#endif
