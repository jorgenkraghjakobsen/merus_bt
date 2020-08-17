#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "driver/i2s.h"
#include "freertos/ringbuf.h"
#include "dsps_biquad_gen.h"
#include "dsps_biquad.h"

#include "dsp_processor.h"

static xTaskHandle s_dsp_i2s_task_handle = NULL;
static RingbufHandle_t s_ringbuf_i2s = NULL;
extern xQueueHandle i2s_queue;
float samplerate = 44100.0;  
extern enum dspFlows dspFlow;

uint8_t muteCH[4] = {0};
ptype_t bq[6];

void setup_dsp_i2s(uint32_t sample_rate)
{
  i2s_config_t i2s_config0 = {
    .mode = I2S_MODE_MASTER | I2S_MODE_TX,                                    // Only TX
    .sample_rate = 44100,
    .bits_per_sample = 32,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,                             //2-channels
    .communication_format = I2S_COMM_FORMAT_STAND_I2S, 
    .dma_buf_count = 8, 
    .dma_buf_len = 512,
    .use_apll = true,
    .fixed_mclk = 0,
    .tx_desc_auto_clear = true                                                //Auto clear tx descriptor on underflow
  };
  
  i2s_driver_install(0, &i2s_config0, 4, &i2s_queue);
  i2s_zero_dma_buffer(0);

  i2s_pin_config_t pin_config0 = {
    .bck_io_num = CONFIG_I2S_BCK_PIN,
    .ws_io_num =  CONFIG_I2S_LRCK_PIN,
    .data_out_num = CONFIG_I2S_DATA_PIN,
    .data_in_num = -1                                                       //Not used
  };
  printf("%d %d %d\n",CONFIG_I2S_BCK_PIN,CONFIG_I2S_LRCK_PIN,CONFIG_I2S_DATA_PIN);

  i2s_set_pin(0, &pin_config0);
  gpio_set_drive_capability(CONFIG_I2S_BCK_PIN,0);
  gpio_set_drive_capability(CONFIG_I2S_LRCK_PIN,0);
  gpio_set_drive_capability(CONFIG_I2S_DATA_PIN,0);
}


static void dsp_i2s_task_handler(void *arg)
{ uint32_t cnt = 0;
  uint8_t *audio = NULL;
  float sbuffer0[1024];
  float sbuffer1[1024];
  float sbuffer2[1024];
  float sbufout0[1024];
  float sbufout1[1024];
  float sbufout2[1024];
  float sbuftmp0[1024];

  uint8_t dsp_audio[4*1024];
  uint8_t dsp_audio1[4*1024];

  size_t chunk_size = 0;
  size_t bytes_written = 0;

  for (;;) {
    cnt++;
    audio = (uint8_t *)xRingbufferReceiveUpTo(s_ringbuf_i2s, &chunk_size, (portTickType)portMAX_DELAY,960);
    if (chunk_size !=0 ){
        int16_t len = chunk_size/4;
        if (cnt%200 == 0)
        { ESP_LOGI("I2S", "Chunk :%d",chunk_size);
        }
        uint8_t *data_ptr = audio;

        for (uint16_t i=0;i<len;i++)
        {
          sbuffer0[i] = ((float) ((int16_t) (audio[i*4+1]<<8) + audio[i*4+0]))/32768;
          sbuffer1[i] = ((float) ((int16_t) (audio[i*4+3]<<8) + audio[i*4+2]))/32768;
          sbuffer2[i] = ((sbuffer0[i]/2) +  (sbuffer1[i]/2));
        }

        switch (dspFlow) {
          case dspfStereo :
            { for (uint16_t i=0; i<len; i++)
              { audio[i*4+0] = (muteCH[0] == 1)? 0 : audio[i*4+0];
                audio[i*4+1] = (muteCH[0] == 1)? 0 : audio[i*4+1];
                audio[i*4+2] = (muteCH[1] == 1)? 0 : audio[i*4+2];
                audio[i*4+3] = (muteCH[1] == 1)? 0 : audio[i*4+3];
              }
              i2s_write_expand(0, (char*)audio, chunk_size,16,32, &bytes_written, portMAX_DELAY);
            }
            break;
          case dspfBiamp :
            { // Process audio ch0 LOW PASS FILTER
              dsps_biquad_f32_ae32(sbuffer0, sbuftmp0, len, bq[0].coeffs, bq[0].w);
              dsps_biquad_f32_ae32(sbuftmp0, sbufout0, len, bq[1].coeffs, bq[1].w);

              // Process audio ch1 HIGH PASS FILTER
              dsps_biquad_f32_ae32(sbuffer0, sbuftmp0, len, bq[2].coeffs, bq[2].w);
              dsps_biquad_f32_ae32(sbuftmp0, sbufout1, len, bq[3].coeffs, bq[3].w);

              int16_t valint[2];
              for (uint16_t i=0; i<len; i++)
              { valint[0] = (muteCH[0] == 1) ? (int16_t) 0 : (int16_t) (sbufout0[i]*32768);
                valint[1] = (muteCH[1] == 1) ? (int16_t) 0 : (int16_t) (sbufout1[i]*32768);
                dsp_audio[i*4+0] = (valint[0] & 0xff);
                dsp_audio[i*4+1] = ((valint[0] & 0xff00)>>8);
                dsp_audio[i*4+2] = (valint[1] & 0xff);
                dsp_audio[i*4+3] = ((valint[1] & 0xff00)>>8);
              }
              i2s_write_expand(0, (char*)dsp_audio, chunk_size,16,32, &bytes_written, portMAX_DELAY);
            }
            break;

          default :
            break;
        }
        vRingbufferReturnItem(s_ringbuf_i2s,(void *)audio);
    }
  }
}

void dsp_i2s_task_init(uint32_t sample_rate)
{ setup_dsp_i2s(sample_rate);
  s_ringbuf_i2s = xRingbufferCreate(8*1024,RINGBUF_TYPE_BYTEBUF);  
  if (s_ringbuf_i2s == NULL) { return; }
  xTaskCreate(dsp_i2s_task_handler, "DSP_I2S", 48*1024, NULL, 7, &s_dsp_i2s_task_handle);
}

void dsp_i2s_task_deninit(void)
{ if (s_dsp_i2s_task_handle) {
    vTaskDelete(s_dsp_i2s_task_handle);
    s_dsp_i2s_task_handle = NULL;
  }
  if (s_ringbuf_i2s) {
      vRingbufferDelete(s_ringbuf_i2s);
      s_ringbuf_i2s = NULL;
  }
}

size_t write_ringbuf(const uint8_t *data, size_t size)
{
   BaseType_t done = xRingbufferSend(s_ringbuf_i2s, (void *)data, size, (portTickType)portMAX_DELAY);
   return (done)?size:0;
}


// DSP processor
//======================================================
// Each time a buffer of audio is passed to the DSP - samples are
// processoed according to a list audio node processings.

// Each audio processor node consist of a data struct holding the
// required weights and states for processing a automomous filter
// function. The high level parameters is maintained in the structre
// as well.

// Release - Prove off concept
// ----------------------------------------
// Fixed 2x2 biquad flow high low seperation for each channel
// Interface for cross over and level
// Additional dynamic bass boost
//

void dsp_setup_flow(double freq) {
  float f = freq/48000/2;

  bq[0] = (ptype_t) { LPF, f, 0, 0.707, NULL, NULL, {0,0,0,0,0}, {0, 0} } ;
  bq[1] = (ptype_t) { LPF, f, 0, 0.707, NULL, NULL, {0,0,0,0,0}, {0, 0} } ;
  bq[2] = (ptype_t) { HPF, f, 0, 0.707, NULL, NULL, {0,0,0,0,0}, {0, 0} } ;
  bq[3] = (ptype_t) { HPF, f, 0, 0.707, NULL, NULL, {0,0,0,0,0}, {0, 0} } ;
  bq[4] = (ptype_t) { HPF, f, 0, 0.707, NULL, NULL, {0,0,0,0,0}, {0, 0} } ;
  bq[5] = (ptype_t) { HPF, f, 0, 0.707, NULL, NULL, {0,0,0,0,0}, {0, 0} } ;

  pnode_t * aflow = NULL;
  aflow = malloc(sizeof(pnode_t));
  if (aflow == NULL)
  { printf("Could not create node");
  }

  for (uint8_t n=0; n<=5; n++)
  { switch (bq[n].filtertype) {
      case LPF: dsps_biquad_gen_lpf_f32( bq[n].coeffs, bq[n].freq, bq[n].q );
              break;
      case HPF: dsps_biquad_gen_hpf_f32( bq[n].coeffs, bq[n].freq, bq[n].q );
              break;
      default : break;
    }
    for (uint8_t i = 0;i <=3 ;i++ )
    {  printf("%.6f ",bq[n].coeffs[i]);
    }
    printf("\n");
 }
}
void dsp_set_xoverfreq(uint8_t freqh, uint8_t freql) {
  float freq =  (freqh*256 + freql)/4;
  ESP_LOGI("I2C","Freq %.0f",freq);
  float f = freq/48000.0/2.0;
  for ( int8_t n=0; n<=5; n++)
  { bq[n].freq = f ;
    switch (bq[n].filtertype) {
      case LPF:
        dsps_biquad_gen_lpf_f32( bq[n].coeffs, bq[n].freq, bq[n].q );
        break;
      case HPF:
        dsps_biquad_gen_hpf_f32( bq[n].coeffs, bq[n].freq, bq[n].q );
        break;
      default : break;
    }
  }
}
