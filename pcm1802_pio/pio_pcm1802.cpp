
#include "pio_pcm1802.h"

#include <stdio.h>

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "pio_pcm1802.pio.h"
#include "pico/multicore.h"


static int32_t pcm1802_current_buffer_index;
static int32_t pcm1802_buffer_ready_index = 0;
static int pcm1802_dma_data_channel;
static uint pcm1802_pio_sm_num;
semaphore_t sem_irq_dma_PCM1802;
int32_t pcm1802_capture_buffers[2][CAPTURE_BUFFER_WORD_COUNT]; // double buffering

void __not_in_flash_func(audio_dma_irq0_hdlr)()
{
  if (dma_channel_get_irq0_status(pcm1802_dma_data_channel))
  {
    // acknowledge IRQ
    dma_channel_acknowledge_irq0(pcm1802_dma_data_channel);

    // store index of buffer filled with audio samples
    pcm1802_buffer_ready_index = pcm1802_current_buffer_index;

    // use next buffer
    pcm1802_current_buffer_index = (pcm1802_current_buffer_index + 1) % 2;

    // start DMA to write to next buffer
    dma_channel_set_write_addr(pcm1802_dma_data_channel, &pcm1802_capture_buffers[pcm1802_current_buffer_index][0], true);

    // notify buffer ready
    sem_release(&sem_irq_dma_PCM1802);
  }

}

// init stuff (semaphore, DMA channel, IRQ handler)
void init_PCM1802()
{
  sem_init(&sem_irq_dma_PCM1802, 0, 1);
  pcm1802_dma_data_channel = dma_claim_unused_channel(true);
  irq_set_exclusive_handler(DMA_IRQ_0, audio_dma_irq0_hdlr);
}

// release stuff
void release_PCM1802()
{
  dma_channel_cleanup(pcm1802_dma_data_channel);
  dma_channel_unclaim(pcm1802_dma_data_channel);
  irq_remove_handler(DMA_IRQ_0, audio_dma_irq0_hdlr);
}

static void configureDMA()
{
  // use first buffer
  pcm1802_current_buffer_index = 0;

  dma_channel_config c = dma_channel_get_default_config(pcm1802_dma_data_channel);
  channel_config_set_read_increment(&c, false);
  channel_config_set_write_increment(&c, true);
  channel_config_set_transfer_data_size(&c, DMA_SIZE_32);
  channel_config_set_dreq(&c, pio_get_dreq(PCM1802_PIO_INST, pcm1802_pio_sm_num, false));

  // use DMA IRQ0 to trigger processing after capture
  dma_channel_acknowledge_irq0(pcm1802_dma_data_channel);
  dma_channel_set_irq0_enabled(pcm1802_dma_data_channel, true);
  irq_set_enabled(DMA_IRQ_0, true);

  dma_channel_configure(pcm1802_dma_data_channel, &c,
                        &pcm1802_capture_buffers[pcm1802_current_buffer_index][0], // Destination pointer
                        &PCM1802_PIO_INST->rxf[pcm1802_pio_sm_num], // Source pointer
                        CAPTURE_BUFFER_WORD_COUNT, // Number of transfers
                        false // delay start
                        );
}

static void start_PCM1802_dma()
{
  dma_channel_set_write_addr(pcm1802_dma_data_channel, &pcm1802_capture_buffers[pcm1802_current_buffer_index][0], true);
}


void start_PCM1802()
{
  //configure DMA
  configureDMA();

  // clock on GPIO21
  // PCM1802 configured as 384*fs
  // 384*44100=16934400
  // best approximation
  // div = 7.38142395
  // real sample rate = (125MHz / div) / 384 = 44100.005
  // adjust for other sample rates (48kHz, 96kHz, etc.)
  gpio_init(PIN_PCM1802_SYSCLK);
  gpio_set_dir(PIN_DOUT, GPIO_OUT);
  clock_gpio_init(PIN_PCM1802_SYSCLK, CLOCKS_CLK_GPOUT0_CTRL_AUXSRC_VALUE_CLK_SYS, 7.38142395);

  gpio_init(PIN_DOUT);
  gpio_init(PIN_BCLK);
  gpio_init(PIN_LRCK);

  gpio_set_dir(PIN_DOUT, GPIO_IN);
  gpio_set_dir(PIN_BCLK, GPIO_IN);
  gpio_set_dir(PIN_LRCK, GPIO_IN);


  // see PIO program : left channel is ignored (tunable)
  pcm1802_pio_sm_num = pio_claim_unused_sm(PCM1802_PIO_INST, true);
  uint offset = pio_add_program(PCM1802_PIO_INST, &pcm1802_in_master_program);
  pcm1802_in_master_program_init(PCM1802_PIO_INST, pcm1802_pio_sm_num, offset, 32, PIN_DOUT);
  pio_sm_set_enabled(PCM1802_PIO_INST, pcm1802_pio_sm_num, true);

  //start DMA
  start_PCM1802_dma();
}

// try to get address to an available buffer. Return nullptr if none.
int32_t* get_buffer()
{
  if (sem_try_acquire(&sem_irq_dma_PCM1802) != true)
  {
    return nullptr;
  }
  else
  {
    return &pcm1802_capture_buffers[pcm1802_buffer_ready_index][0];
  }
}
