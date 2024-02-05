ADD_LIBRARY(pio-i2c-hs pio_i2c.c)

pico_generate_pio_header(pio-i2c-hs ${CMAKE_CURRENT_LIST_DIR}/i2c.pio)

target_sources(pio-i2c-hs PRIVATE
        pio_i2c.c
        pio_i2c.h
        )

target_link_libraries(pio-i2c-hs PRIVATE pico_stdlib hardware_pio hardware_dma)
