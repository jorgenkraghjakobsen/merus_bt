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

#include "driver/rmt.h"
#include "led_strip.h"

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

led_strip_t *strip;

extern enum dspFlows dspFlow;
unsigned samplerate = 44100;
extern uint32_t spp_handle; 

void app_main(void)
{   esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    
    printf("Setup pixel ring\n");
    rmt_config_t config = RMT_DEFAULT_CONFIG_TX(18,RMT_CHANNEL_0);
    // set counter clock to 40MHz
    config.clk_div = 2;
    ESP_ERROR_CHECK(rmt_config(&config));
    ESP_ERROR_CHECK(rmt_driver_install(config.channel, 0, 0));
    printf("Done\n");
    led_strip_config_t strip_config = LED_STRIP_DEFAULT_CONFIG(12, (led_strip_dev_t)config.channel);
    strip = led_strip_new_rmt_ws2812(&strip_config);
    if (!strip) {
        ESP_LOGE(TAG, "install WS2812 driver failed");
    }
    ESP_ERROR_CHECK(strip->clear(strip, 100));
    ESP_ERROR_CHECK(strip->set_pixel(strip, 0, 128 , 0, 0));
    ESP_ERROR_CHECK(strip->refresh(strip, 100));
    ma_bt_start();
    
    setup_ma120x0(); 
    
    dsp_i2s_task_init(samplerate);

    dspFlow = dspfDynBass; //dspfBiamp;
    dsp_setup_flow(200.0);
    dsp_setup_dynbass(300.0, 6, 0.707);

    prot_queue = xQueueCreate(10, sizeof(uint8_t *) );
    xTaskCreatePinnedToCore(protocolHandlerTask, "prot_handler_task", 2*1024, NULL, 5, NULL,0);
    uint32_t n = 0; 
    while(1)
    { uint8_t amp_state[8];
      for (;;)   // read back 96 to 102 (7 bytes)
      { 
        ma_read(0x20,1, MA_dcu_mon0__PM_mon__a, amp_state, 7); 
        if (spp_handle != 0)
        {   //printf("BT tx\n");
            esp_spp_write(spp_handle, 7, amp_state);
        }
      
       vTaskDelay(pdMS_TO_TICKS(200));
       n++; 
      } 
   }
}
