idf_component_register(
    SRCS "stackchan_tts.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES esp_http_client esp-tls mbedtls nvs_flash esp_timer)
