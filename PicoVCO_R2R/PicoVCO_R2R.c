/**
 * Copyright (c) 2023 Ben Everard
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdio.h>
#include <math.h>

#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/adc.h"
#include "eightbit_r2r_dac_simple.pio.h"
#include "waves.h"


//How to store waveforms?
//could have a simple .h file?
// Could calculate them on the fly here
// read them in from disk?
// Most of this is for the future. Let's just calculate them here.
// I'll want these for the future, so let's create a wave library once this is working

// not too sure what the best is here?
#define WAVESIZE 2000
#define PEAK 255
#define START_PIN 0
#define LOWEST_FREQ 100
#define CYCLES_PER_PIO_LOOP 3
#define CPUFREQ 130000000



int triangle_wave[WAVESIZE];
PIO pio;
uint sm;



//The clock divider slows the state machine’s execution by a constant factor, represented as a 16.8 fixed-point fractional number
float calc_clkdiv(float frequency, int length) {
//note - is it worth encoding this in the PIO loop?
//note to self. max 16 bit number is: 65,535
// for super low notes, it might be easier to do longer waves.
// note do I need to include fractional bits? Let's start without and move from there.

	return (CPUFREQ / (frequency*CYCLES_PER_PIO_LOOP*length));

}

void core1_loop() {
	while(true) {
		for(int i=0; i<WAVESIZE;i++){
			pio_sm_put_blocking(pio, sm, triangle_wave[i]);

		}
	}
}

void main () {
	stdio_init_all();
	
	adc_init();
	adc_gpio_init(26);
	adc_select_input(0);
	const float conversion_factor = 3.3f / (1 << 12);
	
	
	generate_triange(triangle_wave, WAVESIZE, PEAK);
	
	pio = pio0;
	uint offset = pio_add_program(pio, &eightbit_r2r_dac_simple_program);
	sm = pio_claim_unused_sm(pio, true);
	
	float init_clkdiv = calc_clkdiv(440, WAVESIZE);
	
	eightbit_r2r_dac_simple_init(pio, sm, offset, START_PIN, init_clkdiv);
	
	multicore_launch_core1(core1_loop);
	
	float divider;
	float freq;
	while(true) {
		uint16_t result = adc_read();
		freq = LOWEST_FREQ * pow(2, result*conversion_factor);
		divider = calc_clkdiv(freq, WAVESIZE);
		pio_sm_set_clkdiv(pio, sm, divider);
		
	}

}