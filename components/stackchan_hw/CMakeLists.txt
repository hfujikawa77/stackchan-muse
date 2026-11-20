idf_component_register(
    SRCS "stackchan_hw.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES esp_driver_i2c esp_driver_uart esp_driver_gpio esp_timer nvs_flash)
