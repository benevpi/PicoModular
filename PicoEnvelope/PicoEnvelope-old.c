#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/spi.h"
#include "hardware/adc.h"
#include "hardware/dma.h"
#include "hardware/irq.h"
#include "pio_i2c.h"

#define PIN_MISO 12
#define PIN_CS   13
#define PIN_SCK  10
#define PIN_MOSI 11

//from datasheet recommended speed for 3v
#define SPI_SPEED 2340000

#define PIN_SCL 1
#define PIN_SDA 0
//not used but needs to be defined
#define PIN_HS_ENABLE 2

#define ATTACK_CHANNEL 0
#define DECAY_CHANNEL 1
#define SUSTAIN_CHANNEL 2
#define RELEASE_CHANNEL 3

#define SPI spi1

//should ths be onboard or offboard -- might be more straight forward to have it off.
//Should be onboard so we can decouple it from the read cycle on the other ADC.
#define INPUT_CHANNEL 1

#define CAPTURE_DEPTH 500 //probably too deep

uint16_t capture_buf[2][CAPTURE_DEPTH];
uint16_t output_buf[2][CAPTURE_DEPTH];

float multiplier = 1.0;

float current_volume = 0; // could double-buffer this if needed

uint dma_chan;
dma_channel_config cfg;

int buff = 0;

int read_analogue(uint8_t channel) {
	 gpio_put(PIN_CS, 0);
	 uint8_t write_buff[3];
	 uint8_t read_buff[4];
	 write_buff[0]= 1<<7 | channel<<4;
	 spi_write_read_blocking(SPI, write_buff, read_buff, 3);
	 gpio_put(PIN_CS, 1);
	 // not completely sure about this, but it's broadly woreking.
	 int output = ((read_buff[0] &1) << 9) | (read_buff[1] << 1) | read_buff[2] >> 7;
	 printf("read buff 0: %d \n", read_buff[0]);
	 printf("read buff 1: %d \n", read_buff[1]);
	 printf("read buff 2: %d \n", read_buff[2]);
	 return output;
	 
}

void write_analogue(int value) {
}

void core_two_loop() {
	//read and debounce trigger
	// read al four inputs
	// calculate and set current multiplier
}

void launch_adc_dma() {
	buff = (buff+1)%2;
	dma_hw->ints0 = 1u << dma_chan;
	dma_channel_configure(dma_chan, &cfg,
		capture_buf[buff],    // dst
		&adc_hw->fifo,  // src
		CAPTURE_DEPTH,  // transfer count
		true            // start immediately
	);
	
	printf("\nStarting capture %d\n", buff);
	//if DAC dma still running at this point, add one to the adc clock divider if not, take one off it.
	//launch DAC DMA on on procssed buffer
	
}

void adc_dma_handler() {
	
	//hw do we know which buffer to process?
	//grab t right now
	int proccess_buffer = buff;
	//just crack on and try to grab new values as fast as possible
	launch_adc_dma();
	
	//process buffer in here

		
}
	

bool reserved_addr(uint8_t addr) {
    return (addr & 0x78) == 0 || (addr & 0x78) == 0x78;
}

void main() {
	stdio_init_all();

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
	
	/*
    adc_fifo_setup(
        true,    // Write each completed conversion to the sample FIFO
        true,    // Enable DMA data request (DREQ)
        1,       // DREQ (and IRQ) asserted when at least 1 sample present
        false,   // err bit is probably going to be confusing.
        false     // capture the full 12 bits
    );

    // Divisor of 0 -> full speed. Free-running capture with the divider is
    // equivalent to pressing the ADC_CS_START_ONCE button once per `div + 1`
    // cycles (div not necessarily an integer). Each conversion takes 96
    // cycles, so in general you want a divider of 0 (hold down the button
    // continuously) or > 95 (take samples less frequently than 96 cycle
    // intervals). This is all timed by the 48 MHz ADC clock.
	
	//don't want it at this speed. need to work out how to time it.
	//details on speed at https://www.hackster.io/AlexWulff/adc-sampling-and-fft-on-raspberry-pi-pico-f883dd#toc-2--adc-sampling-code-1
	//let's go super slow for a test
    adc_set_clkdiv(96000);
	
	// Set up the DMA to start transferring data as soon as it appears in FIFO
    dma_chan = dma_claim_unused_channel(true);
    cfg = dma_channel_get_default_config(dma_chan);
	
	channel_config_set_transfer_data_size(&cfg, DMA_SIZE_16);
    channel_config_set_read_increment(&cfg, false); 
    channel_config_set_write_increment(&cfg, true);
	
	// Pace transfers based on availability of ADC samples
    channel_config_set_dreq(&cfg, DREQ_ADC);
	
	//set up DMA for the ADC
	dma_channel_set_irq0_enabled(dma_chan, true);

    // Configure the processor to run dma_handler() when DMA IRQ 0 is asserted
    irq_set_exclusive_handler(DMA_IRQ_0, adc_dma_handler);
    //irq_set_enabled(DMA_IRQ_0, true);
	
	
	//launch_adc_dma();
	//adc_run(true);
	
	/**
	while(true) {
		sleep_ms(1000);
		
	}
	*/
	
	//set to Hs mode think this is already done
	//pio_i2c_write_blocking(PIO pio, uint sm, uint pin_hs, uint8_t addr, uint8_t *txbuf, uint len);

	uint8_t start = 0x40; //0b01000000;
	uint8_t end = 0;
	
	uint8_t high[3];
	high[0] = start;
	high[1] = 200;
	high[2] = end;
	
	uint8_t low[3];
	low[0] = start;
	low[1] = 0;
	low[2] = end;
	
	
	uint8_t test_sig [60000];
	
	for(int i=0; i++; i<30000) {
		if (i<20000) {test_sig[i*2] = 200;}
		else { test_sig[i*3+1] = 0;}
		test_sig[i*2+1] = 0;
	}

	float multiplier = 1.0;
	int err;
	uint16_t output;
	uint16_t adc_raw;
	//pio_i2c_write_blocking(pio, sm, PIN_HS_ENABLE, 0x62, comm, 3);
	while(true) {
		adc_raw = adc_read();
		output = (uint16_t)(adc_raw*multiplier);
		high[1] = output >> 4;
		high[2] = output << 4;
		err = pio_i2c_write_blocking(pio, sm, PIN_HS_ENABLE, 0x62, high, 3);
	}
	
	//
	while(true) {
		printf("analogue val: %d \n", read_analogue(0));
		sleep_ms(1000);
		//get input -- is this from onboard or external ADC?
		//write_analogue(input*multiplier);
	}
	
	
}