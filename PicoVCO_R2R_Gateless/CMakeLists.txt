add_executable(PicoVCO_R2R_Gateless)

pico_generate_pio_header(PicoVCO_R2R_Gateless ${CMAKE_CURRENT_LIST_DIR}/eightbit_r2r_dac_simple.pio)

target_sources(PicoVCO_R2R_Gateless PRIVATE PicoVCO_R2R.c waves.h waves.c)

target_link_libraries(PicoVCO_R2R_Gateless PRIVATE
        pico_stdlib
        hardware_pio
	pico_multicore
	hardware_adc
        )

pico_enable_stdio_usb(PicoVCO_R2R_Gateless 1)
pico_enable_stdio_uart(PicoVCO_R2R_Gateless 0)

pico_add_extra_outputs(PicoVCO_R2R_Gateless)
