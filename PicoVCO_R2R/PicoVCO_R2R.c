/**
 * Copyright (c) 2023 Ben Everard
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
TODO
Add gate
Since core 2 is updating the output, we a take our time readig the input
Read in ADC, calculate new values, chuck them in in one go.
Is it worth double-buffering? -- probably
**/

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
#define GATE_THRESHOLD 20
#define MAX_ADC 4095

int original_wave[WAVESIZE];
//nt buffer1[WAVESIZE];
//int buffer2[WAVESIZE];
//int *buffers[2] = {buffer1, buffer2};
int buffers[2][WAVESIZE];
int playing_buffer = 0;
int last_gate_val = 0;
PIO pio;
uint sm;
bool flip = false;





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
		if (flip) {
			playing_buffer = (playing_buffer + 1) % 2;
			flip = false;
		}
		for(int i=0; i<WAVESIZE;i++){
			pio_sm_put_blocking(pio, sm, buffers[playing_buffer][i]);
		}
	}
}

//note -- need pointers to original wave, current wave and processing_wave
//can switch between them as needed
void scale_wave(int scale_adc) {
	printf("sw1");
	int back_buffer = (playing_buffer + 1) % 2;
	int scale_factor = scale_adc / MAX_ADC;
	printf("scale_factor: %d, \n", scale_factor);
	printf("sw1");
	for(int i = 0; i< WAVESIZE; i++) {

		buffers[back_buffer][i] = original_wave[i]*scale_factor;
	}
}

void main () {
	stdio_init_all();
	sleep_ms(10000);
	
	adc_init();
	adc_gpio_init(26);
	adc_gpio_init(27);
	
	const float conversion_factor = 3.3f / (1 << 12);
	
	generate_triange(original_wave, WAVESIZE, PEAK);
	scale_wave(MAX_ADC);
	playing_buffer = 1;
	
	pio = pio0;
	uint offset = pio_add_program(pio, &eightbit_r2r_dac_simple_program);
	sm = pio_claim_unused_sm(pio, true);
	
	float init_clkdiv = calc_clkdiv(440, WAVESIZE);
	
	eightbit_r2r_dac_simple_init(pio, sm, offset, START_PIN, init_clkdiv);
	
	multicore_launch_core1(core1_loop);
	
	float divider;
	float freq;
	
	printf("starting loop\n");
	
	while(true) {
		adc_select_input(0);
		printf("1");
		uint16_t result = adc_read();
		printf("2");
		freq = LOWEST_FREQ * pow(2, result*conversion_factor);
		printf("3");
		divider = calc_clkdiv(freq, WAVESIZE);
		printf("4");
		pio_sm_set_clkdiv(pio, sm, divider);
		printf("5");
		
		
		adc_select_input(1);
		printf("6");
		uint16_t gate_adc = adc_read();
		printf(".");
		
		if (gate_adc > (last_gate_val + GATE_THRESHOLD) || gate_adc < (last_gate_val - GATE_THRESHOLD)) {
			printf("voltage change\n");
			scale_wave(gate_adc);
			printf("v1");
			flip = true;
			//playing_buffer = (playing_buffer) + 1 % 2;
			printf("v2");
			last_gate_val = gate_adc;
			printf("v3");
		}
		sleep_ms(500);
		printf("end");
		
	}

}