file(GLOB_RECURSE CHARGE_SRCS "gun_spi.c" "gun_presskey.c" "charge/*.c")
file(GLOB_RECURSE BUTTON_SRCS "gun_presskey.c" "button/*.c")
file(GLOB_RECURSE LED_SRCS "gun_ws2812.c" "led/*.c")

idf_component_register(SRCS "gun_adc.c"
                            "gun_spi.c"
							"gun_infrared.c"
							${CHARGE_SRCS}
							${BUTTON_SRCS}
							${LED_SRCS}

					   INCLUDE_DIRS "."
					   INCLUDE_DIRS "charge/"
					   INCLUDE_DIRS "button/"
					   INCLUDE_DIRS "led/"

					   PRIV_REQUIRES esp_adc_cal framework) 