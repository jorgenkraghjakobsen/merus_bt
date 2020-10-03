// Side channel audio generater 
// 
// Isolated task that generate synth audio for audioable qeues 
// from high pitch clik to drones 
// 

#include "stdint.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/i2s.h"
#include "math.h"
//#include "signalgenerator.h"
//#include "websocket_if.h"
#include "dsp_processor.h"

extern xQueueHandle i2s_queue;
TaskHandle_t xTaskSignal;

uint8_t mode ;     // Cont, Sweep, Burst
uint8_t oscType ;  // Sine, Tri, Square, Noise
double freq = 2000.0 ;
uint32_t sweepCnt;
uint gate;
double angleSpeed ;
double sampleRate = 48000.0;
static bool signalState = 0;

void sideCH_task(void *arg);

void sideCH_start(){ 
  if (signalState == 0 )
  { 
    xTaskCreate(sideCH_task, "sideCH", 8*1024, NULL, 8, &xTaskSignal);
    signalState = 1;
  }  
}
void sideCH_task(void *arg)
{   uint32_t bufferRate  = 100;
    uint32_t lengthSig   = sampleRate/bufferRate;
    uint32_t byteWritten = 0;
    uint32_t timer_cnt   = 0;
    uint8_t sample_buf[480*4];
    uint8_t monibuf[480*2];
    double sigbuf[480];
    double amp   = 0.5;
    double phase = 0;
    double sweepSpeed = 1.0125;
    double sweepStartFreq  = 50.0;
    double sweepStopFreq   = 20000;
    double sweepPeriod     = 4;        // in sec
    uint32_t i = 0;
    i2s_event_t i2s_evt;
    sweepCnt = 0;
    freq = 50.0;
    angleSpeed  = 2*M_PI*freq/sampleRate;

    while (1) {
        xQueueReceive(i2s_queue, &i2s_evt, portMAX_DELAY);
        if (i2s_evt.type == I2S_EVENT_TX_DONE)
        { timer_cnt++;
          switch (mode) {
            case 0 : gate = 1;              // Continues
                     break;
            case 1 :                  // Sweep
                     if (sweepCnt == 0 ) {
                       gate = 1;
                       freq = sweepStartFreq;
                     } else
                     { freq = freq * sweepSpeed;
                       angleSpeed = 2*M_PI*freq/sampleRate;
                     }
                     if ( freq > sweepStopFreq )
                     { gate = 0;
                       //printf("sweepCnt : %d",sweepCnt);
                       sweepCnt = ( sweepCnt > sweepPeriod*lengthSig) ? 0 : sweepCnt + 1;
                     } else
                     { sweepCnt++;
                     }
                     break;
            case 2 : gate = 1;             // Burst
                     break;
          }
          for ( i = 0; i < lengthSig; i++)
          {  if (oscType == 0) {
                sigbuf[i] = amp*sinf((double) (i*angleSpeed) + phase );
             } else if (oscType == 1) {
                sigbuf[i] = (sinf((double) (i*angleSpeed) + phase )>=0) ? amp:-amp ;
             } else if (oscType == 2) {
                sigbuf[i] = amp*sinf((double) (i*angleSpeed) + phase );
             } else if (oscType == 3) {
                sigbuf[i] = amp*((float)(esp_random()-2147483648.0)/2147483648.0);
             }
          }

          // How many period round the circle did se do and where on the circle did we end
          double phase_in  = (double) (i*angleSpeed) + phase;
          // Save the end location on the circle as new starting point
          phase            = phase_in - (double) 2.0*M_PI * (int) (phase_in/(2*M_PI));

          //if (timer_cnt < 10)
          //{  printf("out: %d %f %f   \n",i, phase_in, phase); }

          for (int i = 0; i < lengthSig; i++)
          {  int16_t sample_int = gate*sigbuf[i]*0x7FFF;

             sample_buf[i*4 + 0 ] = sample_int & 0x00FF;
             sample_buf[i*4 + 1 ] = (sample_int & 0xFF00)>>8;
             sample_buf[i*4 + 2 ] = sample_int & 0x00FF;
             sample_buf[i*4 + 3 ] = (sample_int & 0xFF00)>>8;
             monibuf[i*2 ]         = sample_int & 0x00FF;
             monibuf[i*2 + 1]     = (sample_int & 0xFF00)>>8;

          }
          write_ringbuf(sample_buf,lengthSig*4);
          //if (timer_cnt%500==0)
          //{ int res = ws_server_send_bin_client(0,(char*)monibuf, lengthSig/2);
            //printf("%d %d \n",byteWritten, i2s_evt.size );
          //}
        }
        else
        {
           printf("Other interupt on i2s queue \n");
        }
    }
}
