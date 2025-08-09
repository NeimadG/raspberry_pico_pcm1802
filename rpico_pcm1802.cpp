

#include <stdio.h>
#include <stdint.h>
#include <cstring>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "pcm1802_pio/pio_pcm1802.h"
#include "hardware/pwm.h"
#include "math.h"

#define PIN_SIM_LRCK 14
#define PIN_SIM_BCK  13
#define PIN_SIM_DOUT 12


#define NB_SAMPLES_CAPTURE 4410

int main()
{
  stdio_init_all();

  // setup blink led
  gpio_init(PICO_DEFAULT_LED_PIN);
  gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);
  while (!stdio_usb_connected())
  { // blink the pico's led until usb connection is established
   gpio_put(PICO_DEFAULT_LED_PIN, 1);
   sleep_ms(250);
   gpio_put(PICO_DEFAULT_LED_PIN, 0);
   sleep_ms(250);
  }

  init_PCM1802();
  start_PCM1802();

  // read samples and update vu-meter displayed on console output as
  // ####### left #######|###### right #######
  while (1)
  {
    int32_t* bufferAudio = get_buffer();
    uint64_t cumul[2] = {0, 0};
    if (bufferAudio != nullptr)
    {

      for(uint32_t i = 0 ; i < CAPTURE_BUFFER_WORD_COUNT ; ++i)
      {
        cumul[ i % 2 ] += uint64_t(bufferAudio[i] >> 16) * uint64_t(bufferAudio[i] >> 16);
      }

      cumul[0] = (logf(sqrt(cumul[0] / CAPTURE_FRAME_NUMBER))*2.5f)-3.4f;
      cumul[1] = (logf(sqrt(cumul[1] / CAPTURE_FRAME_NUMBER))*2.5f)-3.4f;

      for (uint32_t i = 0 ; i < 20 ; i++)
      {
        if (cumul[0] >= (20-i))
        {
          printf("#");
        }
        else
        {
          printf(" ");
        }
      }

      printf("|");

      for (uint32_t i = 0 ; i < 20 ; i++)
      {
        if (cumul[1] > i)
        {
          printf("#");
        }
        else
        {
          printf(" ");
        }
      }

      printf("\r");

    }
  }
}

