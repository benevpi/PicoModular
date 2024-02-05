// -------------------------------------------------- //
// This file is autogenerated by pioasm; do not edit! //
// -------------------------------------------------- //

#pragma once

#if !PICO_NO_HARDWARE
#include "hardware/pio.h"
#endif

// ----------------------- //
// eightbit_r2r_dac_simple //
// ----------------------- //

#define eightbit_r2r_dac_simple_wrap_target 0
#define eightbit_r2r_dac_simple_wrap 2

static const uint16_t eightbit_r2r_dac_simple_program_instructions[] = {
            //     .wrap_target
    0x6008, //  0: out    pins, 8                    
    0x80a0, //  1: pull   block                      
    0x0000, //  2: jmp    0                          
            //     .wrap
};

#if !PICO_NO_HARDWARE
static const struct pio_program eightbit_r2r_dac_simple_program = {
    .instructions = eightbit_r2r_dac_simple_program_instructions,
    .length = 3,
    .origin = -1,
};

static inline pio_sm_config eightbit_r2r_dac_simple_program_get_default_config(uint offset) {
    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_wrap(&c, offset + eightbit_r2r_dac_simple_wrap_target, offset + eightbit_r2r_dac_simple_wrap);
    return c;
}

#include "hardware/gpio.h"
static inline void eightbit_r2r_dac_simple_init(PIO pio, uint sm, uint prog_offs, uint pin, float init_clkdiv) {
	pio_sm_config c = eightbit_r2r_dac_simple_program_get_default_config(prog_offs);
    sm_config_set_out_shift(&c, true, false, 24);
	for(int i=0; i<8; i++) {
		pio_gpio_init(pio, pin+i);
	}
    // Set the pin direction to output at the PIO
    pio_sm_set_out_pins(pio, sm, pin, 8);
	pio_sm_set_consecutive_pindirs(pio, sm, pin, 8, true);
	sm_config_set_out_pins(&c, pin, 8);
	sm_config_set_clkdiv(&c, init_clkdiv);
	pio_sm_init(pio, sm, prog_offs, &c);
    pio_sm_set_enabled(pio, sm, true);
	}

#endif
