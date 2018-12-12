/*
 * Copyright 2016-2018 NXP Semiconductor, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * o Redistributions of source code must retain the above copyright notice, this list
 *   of conditions and the following disclaimer.
 *
 * o Redistributions in binary form must reproduce the above copyright notice, this
 *   list of conditions and the following disclaimer in the documentation and/or
 *   other materials provided with the distribution.
 *
 * o Neither the name of NXP Semiconductor, Inc. nor the names of its
 *   contributors may be used to endorse or promote products derived from this
 *   software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
 
/**
 * @file    DMA_Project.c
 * @brief   Application entry point.
 */
#include <stdio.h>
#include "board.h"
#include "peripherals.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "MKL25Z4.h"
#include "fsl_debug_console.h"

/* APPLICATION INCLUDES */
#include "adc_driver.h"
#include "dma_driver.h"


/* DEFINES AND TYPEDEFS */
#define BUFF_DOUBLE_SIZE	128
#define BUFF_ITEM_BYTES		4
#define BUFF_HALF_SIZE		(BUFF_DOUBLE_SIZE/2)
#define BUFF_HALF_BYTES		(BUFF_HALF_SIZE*BUFF_ITEM_BYTES)

/* GLOBALS */
volatile bool dma_in_buffer_0 = true;
volatile uint32_t buffer[BUFF_DOUBLE_SIZE];


/*
 * @brief   Application entry point.
 */
int main(void)
{
  	/* Init board hardware. */
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitBootPeripherals();
  	/* Init FSL debug console. */
    BOARD_InitDebugConsole();

    PRINTF("START\n");

    // SETUP DMA
    dma_init_config dma_fig_chan0 = DMA_INIT_CONFIG_DEFAULT;
    dma_fig_chan0.dma = DMA0;
    dma_fig_chan0.src_addr = &(ADC0->R[ADC_MUX_A]);
    dma_fig_chan0.dest_addr = &buffer[0];
    dma_fig_chan0.byte_count = BUFF_HALF_BYTES;
    dma_fig_chan0.interrupt = true;
    dma_fig_chan0.peripheral_en = true;
    dma_fig_chan0.steal_cycles = true;
    dma_fig_chan0.dest_inc = true;
    dma_fig_chan0.auto_disable_req = true;

    dma_init_config dma_fig_chan1 = dma_fig_chan0;
    dma_fig_chan1.channel = DMA_CHANNEL_1;
    dma_fig_chan1.dest_addr = &buffer[BUFF_HALF_SIZE];
    dma_fig_chan1.peripheral_en = true;

    dma_error dma_0_err = dma_init(&dma_fig_chan0);
    dma_error dma_1_err = dma_init(&dma_fig_chan1);

    // SETUP DMAMUX
    dma_mux_config dma_mux_fig_chan0 = DMA_MUX_CONFIG_DEFAULT;
    dma_mux_fig_chan0.channel_enable = true;

    dma_mux_config dma_mux_fig_chan1 = dma_mux_fig_chan0;
    dma_mux_fig_chan1.channel = DMA_CHANNEL_1;

    dma_error dma_mux_0_err = dma_mux_init(&dma_mux_fig_chan0);
    dma_error dma_mux_1_err = dma_mux_init(&dma_mux_fig_chan1);

    // SETUP ADC
    adc_init_config adc_fig = ADC_INIT_CONFIG_DEFAULT;
    adc_fig.channel = ADC_CHAN_DAD0;
    adc_fig.bits = ADC_BITS_16BIT_DIFF;
    adc_fig.continuous = ADC_CONTINUOUS_CONTINUOUS;
    adc_fig.avg_samps = ADC_SAMP_AVG_16;
    adc_fig.sample_cycle_add = ADC_SMP_CYCLE_ADD_6;
    adc_fig.port = PORTE;
    adc_fig.pin_1 = 20;
    adc_fig.pin_2 = 21;
    adc_fig.dma_mode = ADC_DMA_ENABLED;

    adc_error adc_err = adc_init(&adc_fig);

    if(	(dma_0_err != DMA_ERROR_SUCCESS)	|
    	(dma_1_err != DMA_ERROR_SUCCESS)	|
		(adc_err != ADC_ERROR_SUCCESS)		|
		(dma_mux_0_err != DMA_ERROR_SUCCESS)|
		(dma_mux_1_err != DMA_ERROR_SUCCESS))
    {
    	__asm__("BKPT");
    }

    while(1);

    return 0 ;
}

void DMA0_DriverIRQHandler()
{
	//__asm__("BKPT");
	// Disable Interrupts
	// Turn on Pin
	// Enable Other DMA
	dma_in_buffer_0 = !dma_in_buffer_0;	// Switch DMA Buffer Flag
	// Turn off Pin
	// Enable Interrupts
}
