add_executable(PicoVCO_LFO_R2R)

pico_generate_pio_header(PicoVCO_LFO_R2R ${CMAKE_CURRENT_LIST_DIR}/eightbit_r2r_dac_lfo_simple.pio)

target_sources(PicoVCO_LFO_R2R PRIVATE PicoVCO_LFO_R2R.c waves.h waves.c)

target_link_libraries(PicoVCO_LFO_R2R PRIVATE
        pico_stdlib
        hardware_pio
	pico_multicore
	hardware_adc
        )

pico_add_extra_outputs(PicoVCO_LFO_R2R)
