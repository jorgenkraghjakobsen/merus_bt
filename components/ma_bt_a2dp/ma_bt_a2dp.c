#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_system.h"
#include "esp_log.h"

#include "esp_bt.h"
#include "bt_app_core.h"
#include "bt_app_av.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"
#include "esp_a2dp_api.h"
#include "esp_avrc_api.h"
#include "esp_spp_api.h"

#include "dsp_processor.h"
#include "MerusAudio.h"
#include "ma120x0.h"

static void esp_spp_cb(esp_spp_cb_event_t event, esp_spp_cb_param_t *param);
uint32_t spp_handle = 0; 
static const esp_spp_mode_t esp_spp_mode = ESP_SPP_MODE_CB;

extern enum dspFlows dspFlow;
extern uint8_t muteCH[4];
uint32_t freqBT = 1000.0; 
bool callMemReleaseOnBoot = true; /*!< Variable to control if mem.release must be called. */
static char TAG[] = "ma_bt_a2dp"; /*!< Tag used with ESP_LOGx function. */

/** event for handler "bt_av_hdl_stack_up */
enum
{
    BT_APP_EVT_STACK_UP = 0,
};

/** Handler for bluetooth stack enabled events */
static void bt_av_hdl_stack_evt(uint16_t event, void *p_param);

/** @brief Stop Bluetooth 
 *  This function stops a running BT application.
*/
void ma_bt_stop(void)
{
    esp_err_t err;

    /* Shut down BT task.  */
    bt_app_task_shut_down();

    /* Deinitialize Audio/Video Remote Control Protocol controller (AVRCP). */
    if ((err = esp_avrc_ct_deinit()) == ESP_OK){
        ESP_LOGI(TAG, "%s: avrc_ct deinitialized: %s.", __func__, esp_err_to_name(err));
    }else{
        ESP_LOGW(TAG, "%s: Failed to deinitialize avrc_ct: %s.", __func__, esp_err_to_name(err));
    }

    /* Deinitialize AVRCP target.) */
    if (esp_avrc_tg_deinit() == ESP_OK){
        ESP_LOGI(TAG, "avrc_tg deinitialized.");
    }else{
        ESP_LOGW(TAG, "Failed to deinitialize avrc_tg.");
    }

    /* Deinitialize Advanced Audio Distribution Profile (A2DP) sink. */
    if ((err = esp_a2d_sink_deinit()) == ESP_OK){
        ESP_LOGI(TAG, "%s: a2dp sink deinitialized: %s", __func__, esp_err_to_name(err));
    }else{
        ESP_LOGW(TAG, "%s: a2dp sink failed to deinitialize: %s", __func__, esp_err_to_name(err));
    }

    /* Disable Bluedroid. */
    if ((err = esp_bluedroid_disable()) == ESP_OK){
        ESP_LOGI(TAG, "%s: Bluedroid disabled: %s", __func__, esp_err_to_name(err));
    }else{
        ESP_LOGW(TAG, "%s: Failed to disable Bluedroid: %s", __func__, esp_err_to_name(err));
    }

    /* Deinitialize Bluedroid. */
    if ((err = esp_bluedroid_deinit()) == ESP_OK){
        ESP_LOGI(TAG, "%s: Bluedroid deinitialized: %s", __func__, esp_err_to_name(err));
    }else{
        ESP_LOGW(TAG, "%s: Failed to deinitialize Bluedroid: %s", __func__, esp_err_to_name(err));
    }

    /* Disable BT controller. */
    if ((err = esp_bt_controller_disable()) == ESP_OK){
        ESP_LOGI(TAG, "%s: BT controller disabled: %s", __func__, esp_err_to_name(err));
    }else{
        ESP_LOGW(TAG, "%s: Failed to disable BT controller: %s.", __func__, esp_err_to_name(err));
    }

    /* Deinitialize BT controller. */
    if ((err = esp_bt_controller_deinit()) == ESP_OK){
        ESP_LOGI(TAG, "%s: BT controller deinitialized: %s.", __func__, esp_err_to_name(err));
    }else{
        ESP_LOGW(TAG, "%s: Failed to deinitialize BT controller: %s", __func__, esp_err_to_name(err));
    }
}

/**
 * @brief Setup and start Bluetooth.
 * This function must be called to setup Bluetooth A2DP.
 * */
void ma_bt_start(void)
{
    esp_err_t err;

    if (callMemReleaseOnBoot) /* Only run on boot */
    {
        ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));
        callMemReleaseOnBoot = false;
    }

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();

    /* Initialize BT controller. */
    if ((err = esp_bt_controller_init(&bt_cfg)) != ESP_OK){
        ESP_LOGE(BT_AV_TAG, "%s initialize controller failed: %s\n", __func__, esp_err_to_name(err));
        return;
    }

    /* Enable controller. */
    if ((err = esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT)) != ESP_OK){
        ESP_LOGE(BT_AV_TAG, "%s enable controller failed: %s\n", __func__, esp_err_to_name(err));
        return;
    }

    /* Initialize Bluedroid. */
    if ((err = esp_bluedroid_init()) != ESP_OK){
        ESP_LOGE(BT_AV_TAG, "%s initialize bluedroid failed: %s\n", __func__, esp_err_to_name(err));
        return;
    }

    /* Enable Bluedroid. */
    if ((err = esp_bluedroid_enable()) != ESP_OK){
        ESP_LOGE(BT_AV_TAG, "%s enable bluedroid failed: %s\n", __func__, esp_err_to_name(err));
        return;
    }
    // Enable SPP
    if ((err = esp_spp_register_callback(esp_spp_cb)) != ESP_OK) {
        ESP_LOGE("SPP_TAG", "%s spp register failed: %s\n", __func__, esp_err_to_name(err));
        return;
    }
    if ((err = esp_spp_init(esp_spp_mode)) != ESP_OK) {
        ESP_LOGE("SPP_TAG", "%s spp init failed: %s\n", __func__, esp_err_to_name(err));
        return;
    }

  
    /* Create application task */
    bt_app_task_start_up();

    /* Bluetooth device name, connection mode and profile set up */
    bt_app_work_dispatch(bt_av_hdl_stack_evt, BT_APP_EVT_STACK_UP, NULL, 0, NULL);

    /* Set default parameters for Secure Simple Pairing */
    esp_bt_sp_param_t param_type = ESP_BT_SP_IOCAP_MODE;
    esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_IO;
    esp_bt_gap_set_security_param(param_type, &iocap, sizeof(uint8_t));
}

void bt_app_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
    switch (event){
    case ESP_BT_GAP_AUTH_CMPL_EVT:{
        if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS){
            ESP_LOGI(BT_AV_TAG, "authentication success: %s", param->auth_cmpl.device_name);
            esp_log_buffer_hex(BT_AV_TAG, param->auth_cmpl.bda, ESP_BD_ADDR_LEN);
        }else{
            ESP_LOGE(BT_AV_TAG, "authentication failed, status:%d", param->auth_cmpl.stat);
        }
        break;
    }

#if (CONFIG_BT_SSP_ENABLED == true)
    case ESP_BT_GAP_CFM_REQ_EVT:
        ESP_LOGI(BT_AV_TAG, "ESP_BT_GAP_CFM_REQ_EVT Please compare the numeric value: %d", param->cfm_req.num_val);
        esp_bt_gap_ssp_confirm_reply(param->cfm_req.bda, true);
        break;
    case ESP_BT_GAP_KEY_NOTIF_EVT:
        ESP_LOGI(BT_AV_TAG, "ESP_BT_GAP_KEY_NOTIF_EVT passkey:%d", param->key_notif.passkey);
        break;
    case ESP_BT_GAP_KEY_REQ_EVT:
        ESP_LOGI(BT_AV_TAG, "ESP_BT_GAP_KEY_REQ_EVT Please enter passkey!");
        break;
#endif

    default:{
        ESP_LOGI(BT_AV_TAG, "event: %d", event);
        break;
    }
    }
    return;
}

#define SPP_TAG "SPP_TAG"
static void esp_spp_cb(esp_spp_cb_event_t event, esp_spp_cb_param_t *param)
{ esp_err_t err;
  switch (event) {
    case ESP_SPP_INIT_EVT:
           ESP_LOGI(SPP_TAG, "ESP_SPP_INIT_EVT");
           esp_bt_dev_set_device_name("EXCAMPLE_DEVICE_NAME");
           esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
           esp_spp_start_srv(ESP_SPP_SEC_AUTHENTICATE,ESP_SPP_ROLE_SLAVE, 0, "SPP_SERVER");
           break;
    case ESP_SPP_DISCOVERY_COMP_EVT:
           ESP_LOGI(SPP_TAG, "ESP_SPP_DISCOVERY_COMP_EVT");
           break;
    case ESP_SPP_OPEN_EVT:
           ESP_LOGI(SPP_TAG, "ESP_SPP_OPEN_EVT");
           break;
    case ESP_SPP_CLOSE_EVT:
           ESP_LOGI(SPP_TAG, "ESP_SPP_CLOSE_EVT");
           break;
    case ESP_SPP_START_EVT:
           ESP_LOGI(SPP_TAG, "ESP_SPP_START_EVT");
           break;
    case ESP_SPP_CL_INIT_EVT:
           ESP_LOGI(SPP_TAG, "ESP_SPP_CL_INIT_EVT");
           break;
    case ESP_SPP_DATA_IND_EVT:
           ESP_LOGI(SPP_TAG, "ESP_SPP_DATA_IND_EVT len=%d handle=%d",param->data_ind.len, param->data_ind.handle);
           if (param->data_ind.data[0] == 1) {
              if (param->data_ind.data[1] == 1) {
                ma_write_byte(0x20,1,(uint16_t) param->data_ind.data[2],param->data_ind.data[3]);  
              }
           }
           if (param->data_ind.data[0] == 5) {
              if (param->data_ind.data[1] == 1) {
                 freqBT = param->data_ind.data[3]*256+param->data_ind.data[4]; 
                 printf("%d\n",freqBT/4);
                 dsp_set_xoverfreq((uint8_t) param->data_ind.data[3], (uint8_t) param->data_ind.data[4]);  
              }  
           } 
           if (param->data_ind.data[1] == 2) {  // Mute CH0 or CH1  
                muteCH[param->data_ind.data[2]] = param->data_ind.data[3];  
           }     
           if (param->data_ind.data[1] == 3) {  // Set DSPmode   
                dspFlow = param->data_ind.data[2];  
                printf("DSP mode : %d \n",dspFlow);    
           }     
           if (param->data_ind.data[1] == 4) {  // Sync UI elements   
              uint8_t UI_sync_vector[15];
              uint8_t UI_sync_vol[4];   
              ma_read(0x20,1, (uint16_t) MA_vol_db_master__a, UI_sync_vol, 4 );
              UI_sync_vector[0] = 5;
              UI_sync_vector[1] = 4;
              UI_sync_vector[2] = 0;
              UI_sync_vector[3] = 9;
              UI_sync_vector[4] = UI_sync_vol[0];
              UI_sync_vector[5] = UI_sync_vol[2];
              UI_sync_vector[6] = UI_sync_vol[3];
              UI_sync_vector[7] = muteCH[0];
              UI_sync_vector[8] = muteCH[1];
              UI_sync_vector[9] = 0;
              UI_sync_vector[10] = dspFlow;
              UI_sync_vector[11] = (uint16_t)freqBT / 256; 
              UI_sync_vector[12] = (uint16_t)freqBT % 256;  

              if (spp_handle != 0)
              {   printf("BT tx\n");
                  esp_spp_write(spp_handle, 13, UI_sync_vector);
              }
            }
            esp_log_buffer_hex("",param->data_ind.data,param->data_ind.len);
            break;
    case ESP_SPP_CONG_EVT:
            ESP_LOGI(SPP_TAG, "ESP_SPP_CONG_EVT");
            break;
    case ESP_SPP_WRITE_EVT:
            //ESP_LOGI(SPP_TAG, "ESP_SPP_WRITE_EVT");
            break;
    case ESP_SPP_SRV_OPEN_EVT:
            spp_handle = param->open.handle;
            printf("%d spp handle \n",spp_handle); 
            ESP_LOGI(SPP_TAG, "ESP_SPP_SRV_OPEN_EVT");
            break;
    default:
            break;
    }
}

static void bt_av_hdl_stack_evt(uint16_t event, void *p_param)
{
    ESP_LOGD(BT_AV_TAG, "%s evt %d", __func__, event);
    switch (event){
        case BT_APP_EVT_STACK_UP:{
            /* set up device name */
            char *dev_name = "MerusAmp";
            esp_bt_dev_set_device_name(dev_name);

            esp_bt_gap_register_callback(bt_app_gap_cb);

            /* initialize AVRCP controller */
            esp_avrc_ct_init();
            esp_avrc_ct_register_callback(bt_app_rc_ct_cb);

            /* initialize AVRCP target */
            assert(esp_avrc_tg_init() == ESP_OK);
            esp_avrc_tg_register_callback(bt_app_rc_tg_cb);

            esp_avrc_rn_evt_cap_mask_t evt_set = {0};
            esp_avrc_rn_evt_bit_mask_operation(ESP_AVRC_BIT_MASK_OP_SET, &evt_set, ESP_AVRC_RN_VOLUME_CHANGE);
            assert(esp_avrc_tg_set_rn_evt_cap(&evt_set) == ESP_OK);

            /* initialize A2DP sink */
            esp_a2d_register_callback(&bt_app_a2d_cb);
            esp_a2d_sink_register_data_callback(bt_app_a2d_data_cb);
            esp_a2d_sink_init();

            /* set discoverable and connectable mode, wait to be connected */
            esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
            break;
        }
        default:
            ESP_LOGE(BT_AV_TAG, "%s unhandled evt %d", __func__, event);
            break;
    }
}