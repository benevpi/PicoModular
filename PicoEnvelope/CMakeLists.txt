add_executable(PicoEnvelope)

target_sources(PicoEnvelope PRIVATE PicoEnvelope.c)

target_link_libraries(PicoEnvelope PRIVATE
        pico_stdlib
	pico_multicore
	hardware_adc
	hardware_spi
	hardware_pio
	hardware_adc
	hardware_dma
	hardware_irq
	pio-i2c-hs
        )

pico_enable_stdio_usb(PicoEnvelope 1)
pico_enable_stdio_uart(PicoEnvelope 0)

pico_add_extra_outputs(PicoEnvelope)
