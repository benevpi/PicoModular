#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/spi.h"
#include "hardware/adc.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "pio_i2c.h"
#include "pico/multicore.h"

#define PIN_MISO 12
#define PIN_CS   13
#define PIN_SCK  10
#define PIN_MOSI 11

#define PIN_TRIGGER 3

//from datasheet recommended speed for 3v
#define SPI_SPEED 2340000

#define PIN_SCL 1
#define PIN_SDA 0
//not used but needs to be defined
#define PIN_HS_ENABLE 2

#define ATTACK_CHANNEL 0
#define DECAY_CHANNEL 2
#define SUSTAIN_CHANNEL 1
#define RELEASE_CHANNEL 3

//to spimplify everything, just have the numbers as microseconds.
#define MAX_ATTACK 1024
#define MAX_DECAY 1024
#define MAX_SUSTAIN 1
#define MAX_RELEASE 1024

#define SPI spi1

//should ths be onboard or offboard -- might be more straight forward to have it off.
//Should be onboard so we can decouple it from the read cycle on the other ADC.
#define INPUT_CHANNEL 1

float multiplier = 0;

float current_volume = 0; // could double-buffer this if needed

int trigger_state = 0;


//why am I only reading three bytes here?
int read_analogue(uint8_t channel) {
	 gpio_put(PIN_CS, 0);
	 uint8_t write_buff[4];
	 uint8_t read_buff[4];
	 //write_buff[0]= 1<<7 | channel<<4;
	 write_buff[0] = 0x01;
	 write_buff[1] = 1<<7 | channel << 4;
	 
	 spi_write_read_blocking(SPI, write_buff, read_buff, 4);
	 gpio_put(PIN_CS, 1);
	 // not completely sure about this, but it's broadly woreking.
	 //int output = ((read_buff[0] &1) << 9) | (read_buff[1] << 1) | read_buff[2] >> 7;
	 int output = (((uint16_t)(read_buff[1] & 0x07)) << 8) | read_buff[2]; // stolen from adafruit
	 printf("read buff 0: %d \n", read_buff[0]);
	 printf("read buff 1: %d \n", read_buff[1]);
	 printf("read buff 2: %d \n", read_buff[2]);
	 return output;
	 
}


void core_one_loop() {
	int attack;
	int decay;
	int sustain;
	int release;
	while(true) {
		//read trigger
		if(gpio_get(PIN_TRIGGER) ==1) {
			//really simple debounce
			sleep_ms(50);
			if (gpio_get(PIN_TRIGGER) ==1) {
				trigger_state = 1;
				//do attack and sustain here
				//let's get the state of all the pins
				attack = read_analogue(ATTACK_CHANNEL);
				decay = read_analogue(DECAY_CHANNEL);
				sustain = read_analogue(SUSTAIN_CHANNEL);
				release = read_analogue(RELEASE_CHANNEL);
			
			
				printf("attack: %d \n", attack);
				printf("decay: %d \n", decay);
				printf("sustain: %d \n", sustain);
				
				printf("release: %d \n", release);

				
				multiplier = 0; // it should already be
				
				printf("multiplier: %f \n", multiplier);
				
				//attack stage
				for(int i=1;i<100;i++) {
					multiplier += 0.01;
					sleep_ms(attack/100);
				}
				
				multiplier = 1.0;
				
				//decay stage
				for(int i=1;i<100;i++) {
					multiplier -= ((float)1024-(float)sustain)/(float)(102400);
					sleep_ms(decay/100);
					printf("multiplier decay: %f\n", multiplier);
				}
				
				
				//sustain stage
				while(true) {
					if (gpio_get(PIN_TRIGGER) == 0) {
						sleep_ms(50);
						if (gpio_get(PIN_TRIGGER) ==0) {
							break;
						}
					}
					printf("multiplier sustain: %f\n", multiplier);
					sleep_ms(50);
					
				}
				
				//release stage
				for(int i=1;i<100;i++) {
					multiplier =  ( ((float)sustain/ (float)1024) / (float)100) * (float)(100-i);
					sleep_ms(release/100);
					printf("multiplier: %f\n", multiplier);
				}
				
				multiplier = 0; 
			}
					
		
		}		
			
	}
	//read and debounce trigger
	// read al four inputs
	// calculate and set current multiplier
}



void main() {
	stdio_init_all();
	
	//set up trigger
	gpio_init(PIN_TRIGGER);
	gpio_set_dir(PIN_TRIGGER, GPIO_IN);
	gpio_set_pulls(PIN_TRIGGER, false, true);

//set up spi for adc
    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);
	
	gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
	gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
	
	spi_init(SPI, SPI_SPEED);
	
//set up i2cfor DAC
	set_sys_clock_khz(136000, true);
	PIO pio = pio0;
    uint sm = 0;
    uint offset = pio_add_program(pio, &i2c_program);
    i2c_program_init(pio, sm, offset, PIN_SDA, PIN_SCL, PIN_HS_ENABLE);
	pio_i2c_start(pio, sm, PIN_HS_ENABLE);
	
	//setup adc
	adc_gpio_init(26 + INPUT_CHANNEL);
	
	adc_init();
	
    adc_select_input(INPUT_CHANNEL);
	
	int err;
	uint16_t output;
	uint16_t adc_raw;
	
	//launch core 1
	multicore_launch_core1(core_one_loop);
	
	uint8_t i2c_buff[3];
	i2c_buff[0] = 0x40;
	i2c_buff[1] = 0;
	i2c_buff[2] = 0;
	while(true) {
		adc_raw = adc_read();
		output = (uint16_t)(adc_raw*multiplier);
		i2c_buff[1] = output >> 4;
		i2c_buff[2] = output << 4;
		err = pio_i2c_write_blocking(pio, sm, PIN_HS_ENABLE, 0x62, i2c_buff, 3);
	}
	
	//
	while(true) {
		printf("analogue val: %d \n", read_analogue(0));
		sleep_ms(1000);
		//get input -- is this from onboard or external ADC?
		//write_analogue(input*multiplier);
	}
	
	
}