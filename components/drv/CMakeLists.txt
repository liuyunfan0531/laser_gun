file(GLOB_RECURSE CHARGE_SRCS "gun_presskey.c" "charge/*.c")
file(GLOB_RECURSE BUTTON_SRCS "gun_presskey.c" "button/*.c")

idf_component_register(SRCS "gun_adc.c"
							${CHARGE_SRCS}
							${BUTTON_SRCS}

					   INCLUDE_DIRS "."
					   INCLUDE_DIRS "charge/"
					   INCLUDE_DIRS "button/"

					   PRIV_REQUIRES esp_adc_cal framework) 