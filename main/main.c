/* Merus Bluetooth Audio Streaming

   Written by Jørgen Kragh Jakobsen
   Infineon Merus Digital Audio
   Aug 2020

   This code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/


#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "driver/gpio.h"
#include "sdkconfig.h"


#include "MerusAudio.h"
#include "ma120x0.h"

#include "bt_app_core.h"
#include "bt_app_av.h"
#include "esp_spp_api.h"

#include "ma_bt_a2dp.h"
#include "protocol.h"
#include "dsp_processor.h"

static const char *TAG = "Main";
xQueueHandle prot_queue;
xQueueHandle i2s_queue;

enum dspFlows dspFlow;
extern uint32_t spp_handle; 

void app_main(void)
{   esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
   
    ma_bt_start();
    
    setup_ma120x0(); 
    
    dsp_i2s_task_init(44100);

    dspFlow = dspfStereo; //dspfBiamp;
    dsp_setup_flow(200.0);
    
    prot_queue = xQueueCreate(10, sizeof(uint8_t *) );
    xTaskCreatePinnedToCore(protocolHandlerTask, "prot_handler_task", 2*1024, NULL, 5, NULL,0);
    
    while(1)
    { uint8_t amp_state[8];
      for (;;)   // read back 96 to 102 (7 bytes)
      { ma_read(0x20,1, MA_dcu_mon0__PM_mon__a, amp_state, 7); 
        if (spp_handle != 0)
        {   //printf("BT tx\n");
            esp_spp_write(spp_handle, 7, amp_state);
        }
      
       vTaskDelay(pdMS_TO_TICKS(200));
      }
   }
}
