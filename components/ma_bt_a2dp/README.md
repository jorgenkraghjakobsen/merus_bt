# Component: Bluetooth Advanced Audio Distribution Protocol (A2DP) for ESP32

This component is to be run on an ESP32. It enables Bluetooth A2DP.

## Adding this component to a project

To add this component to a project (in project's root folder):

```
git submodule add [url] [folder]
```

If the submodules's folder is empty, try:

```
git submodule update --init --recursive
git pull
```

To add changes to this component:

```
cd [path/to/submodule]
git add <stuff>
git commit -m "comment"
git push
```

## Initialize NVS

```
/* Initialize NVS — it is used to store PHY calibration data */
esp_err_t err = nvs_flash_init();
if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
{
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
}
```

## Menuconfig

**_Make sure_** that the following is set in 'Menu Config' - > 'Component Config':

- Bluetooth - 'Bluetooth'
- Bluetooth Controller Mode - 'BR/EDR only'
- Bluetooth Host - 'Bluedroid Dual Mode'
- Bluedroid Options: 'Classic Bluetooth', 'A2DP', 'SPP' for future updates
