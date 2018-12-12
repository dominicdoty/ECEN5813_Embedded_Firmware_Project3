/*
 * adc_driver.c
 *
 *  Created on: Dec 10, 2018
 *      Author: Dominic Doty
 */


/* HEADER */
#include "adc_driver.h"


/* STATIC FUNCTION DECLARATIONS */
static bool adc_null_ptrs(adc_init_config* config);
static bool adc_incompatible_mode(adc_channel channel, adc_bits bits);


/* FUNCTION DEFINITIONS */

// Initialize the ADC with set parameters
adc_error adc_init(adc_init_config* config)
{
	// Init Variables
	adc_error ret = ADC_ERROR_SUCCESS;
	ADC_Type* adc = config->adc;

	// Check validity of config struct
	if(adc_null_ptrs(config))
	{
		ret = ADC_ERROR_NULL_PTR;
	}
	// Check valid mode (diff modes with diff channels)
	else if(adc_incompatible_mode(config->channel, config->bits))
	{
		ret = ADC_ERROR_CHANNEL_MODE_INCOMPATIBLE;
	}
	else if(adc != ADC0)
	{
		ret = ADC_ERROR_UNKNOWN_ADC;
	}
	else
	{
		// GPIO Setup
		clock_ip_name_t kclock = ((((uint32_t)(config->port)) >> 12) & 0xFU) + 0x10380000U;
		CLOCK_EnableClock(kclock);
		PORT_SetPinMux(config->port, config->pin_1, kPORT_PinDisabledOrAnalog);
		PORT_SetPinMux(config->port, config->pin_2, kPORT_PinDisabledOrAnalog);

		// Enable Clock To Peripheral
		CLOCK_EnableClock(kCLOCK_Adc0);

		// CFG1
		adc->CFG1 = ADC_CFG1_ADLPC(config->low_power)										|
					ADC_CFG1_ADIV(config->clock_div)										|
					ADC_CFG1_ADLSMP(ADC_SAMP_CYCLE_ADDER_ADLSMP(config->sample_cycle_add))	|
					ADC_CFG1_MODE(ADC_BITS(config->bits))									|
					ADC_CFG1_ADICLK(config->clock);

		// CFG2
		adc->CFG2 = ADC_CFG2_MUXSEL(config->mux)											|
					ADC_CFG2_ADACKEN(config->async_state)									|
					ADC_CFG2_ADHSC(ADC_SAMP_CYCLE_ADDER_ADHSC(config->sample_cycle_add))	|
					ADC_CFG2_ADLSTS(ADC_SAMP_CYCLE_ADDER_ADLSTS(config->sample_cycle_add));

		// SC2 - Note, ADTRG is set after calibration at the end
		adc->SC2 = 	ADC_SC2_COMP(config->compare_mode)	|
					ADC_SC2_DMAEN(config->dma_mode)		|
					ADC_SC2_REFSEL(config->ref_volt)	;

		// SC3
		adc->SC3 =	ADC_SC3_ADCO(config->continuous)	|
					ADC_SC3_AVG(config->avg_samps)	;

		switch(config->compare_mode)
		{
			case ADC_COMPARE_DISABLED:
				break;

			case ADC_COMPARE_LESS:
				adc->CV1 = ADC_CV1_CV(MAX(config->compare_1, config->compare_2));
				break;

			case ADC_COMPARE_RANGE_EXCLUSIVE_INSIDE:
				adc->CV1 = ADC_CV1_CV(MAX(config->compare_1, config->compare_2));
				adc->CV2 = ADC_CV2_CV(MIN(config->compare_1, config->compare_2));
				break;

			case ADC_COMPARE_GREATER:
				adc->CV1 = ADC_CV1_CV(MAX(config->compare_1, config->compare_2));
				break;

			case ADC_COMPARE_RANGE_INCLUSIVE_INSIDE:
				adc->CV1 = ADC_CV1_CV(MIN(config->compare_1, config->compare_2));
				adc->CV2 = ADC_CV2_CV(MAX(config->compare_1, config->compare_2));
				break;

			case ADC_COMPARE_RANGE_EXCLUSIVE_OUTSIDE:
				adc->CV1 = ADC_CV1_CV(MIN(config->compare_1, config->compare_2));
				adc->CV2 = ADC_CV2_CV(MAX(config->compare_1, config->compare_2));
				break;

			case ADC_COMPARE_RANGE_INCLUSIVE_OUTSIDE:
				adc->CV1 = ADC_CV1_CV(MAX(config->compare_1, config->compare_2));
				adc->CV2 = ADC_CV2_CV(MIN(config->compare_1, config->compare_2));
				break;
		}

		// Calibrate
		adc->SC3 |= ADC_SC3_CAL(1);
		while(!(*(adc->SC1) & ADC_SC1_COCO_MASK));	// Wait for cal to complete

		// Check for Failure
		if(adc->SC3 & ADC_SC3_CALF_MASK)
		{
			ret = ADC_ERROR_FAILED_CAL;
		}
		else
		{
			// Set up Interrupt Flag, Trigger
			if(config->interrupt == ADC_INT_ON_COMPLETE)
			{
				adc->SC2 |= ADC_SC2_ADTRG(config->trigger);
				NVIC_EnableIRQ(ADC0_IRQn);
			}

			// SC1 set channel (also starts conversion)
			adc->SC1[0] = 	ADC_SC1_ADCH_DIFF(config->channel) |
							ADC_SC1_AIEN(config->interrupt);
		}
	}

	return ret;
}

// Start an ADC Conversion - Only needed in single shot mode, init automatically starts continuous mode
void adc_start_conversion(ADC_Type* adc, adc_mux_select mux, adc_channel channel)
{
	adc->SC1[mux] = ((adc->SC1[mux]) & ~(ADC_SC1_ADCH_MASK | ADC_SC1_DIFF_MASK)) | channel;
}

// Get result blocking
uint16_t adc_blocking_result(ADC_Type* adc, adc_mux_select mux, adc_bits bits)
{
	uint16_t mask_lut[] = ADC_BITS_RESULT_MASK_LUT;
	while(!(adc->SC1[mux] & ADC_SC1_COCO_MASK));
	return adc->R[mux] & mask_lut[bits];
}

// Calculate the sample rate based on supplied clock parameters
// Not meant for use in normal application, used for debugging
uint32_t adc_sample_rate_calc(adc_init_config* config)
{
	// Initialize
	uint32_t sample_rate = 0;

	// Find Clock rate based on source, (bus, bus/2, alt, async), calc the div freq
	uint32_t clock_rate = 0;
	switch(config->clock)
	{
		case ADC_CLOCK_SEL_BUS:
			clock_rate = CLOCK_GetBusClkFreq();
			break;
		case ADC_CLOCK_SEL_BUSDIV2:
			clock_rate = CLOCK_GetBusClkFreq()/2;
			break;
		case ADC_CLOCK_SEL_ALTCLK:
			clock_rate = 0; // Cannot find info on Alt freq
			break;
		case ADC_CLOCK_SEL_ADACK:;
			// Typical Frequency Values - page 29 of datasheet
			uint32_t ADACK_freqs[] = ADACK_FREQUENCY_LUT;
			uint8_t ADACK_index = ADACK_FREQUENCY_LUT_INDEX((config->low_power), (config->sample_cycle_add >> 3));
			clock_rate = ADACK_freqs[ADACK_index];
			break;
		default:
			break;
	}

	// Calculate Divided Clock Rate
	clock_rate /= (1 << config->clock_div);


	// Calculate Clocks/Sample
	uint8_t average_number[] = ADC_SAMP_AVERAGE_LUT;
	uint8_t mode_base_cycles[] = ADC_BITS_BASE_CYCLE_LUT;
	uint8_t long_mode_cycles[] = ADC_SAMP_CYCLE_ADDER_LUT;

	// Find Clock/Samp based on Average Mode, Convert Mode, Long Sample Time Adder, and High Speed Time Adder
	// Note that this ignores Single/First Continuous Time Add
	uint16_t clocks_per_sample =(average_number[config->avg_samps])			*
								((mode_base_cycles[config->bits]) + (long_mode_cycles[config->sample_cycle_add]));


	// Calculate Sample Rate
	sample_rate = clock_rate/clocks_per_sample;


	// Check that we're within clock limits
	if(	(config->bits == ADC_BITS_16BIT) |
		(config->bits == ADC_BITS_16BIT_DIFF))
	{
		if(clock_rate > 12000000)
		{
			sample_rate = 0; 	// 16 bit can go 12MHz clock source
		}
	}
	else
	{
		if(clock_rate > 18000000)
		{
			sample_rate = 0;	// 13 bit or less can go 18MHz clock source
		}
	}


	return sample_rate;
}


/* STATIC FUNCTION DEFINITIONS */

// Check for null pointers in the config struct
static bool adc_null_ptrs(adc_init_config* config)
{
	// Initialize
	bool ret = false;

	// Null config struct
	if(	(config == NULL)		|	// Null Config struct
		(config->adc == NULL)	|	// Null ADC pointer
		(config->port == NULL)	)	// Null GPIO Port
	{
		ret = true;
	}
	return ret;
}

// Check that mode and selected channel are compatible (diff channel with diff mode)
static bool adc_incompatible_mode(adc_channel channel, adc_bits bits)
{
	// Initialize
	bool ret = false;

	if(channel != ADC_CHAN_DISABLED)	// If channel is disabled, don't bother checking mode
	{
		bool diff_mode = (bits >= 0x4U);	// Diff Modes, Diff Channels
		bool diff_channel =((channel == ADC_CHAN_DAD0)			|
							(channel == ADC_CHAN_DAD1)			|
							(channel == ADC_CHAN_DAD2)			|
							(channel == ADC_CHAN_DAD3)			|
							(channel == ADC_CHAN_TEMP_DIFF)		|
							(channel == ADC_CHAN_BANDGAP_DIFF)	|
							(channel == ADC_CHAN_VREFSH_DIFF));

		ret = !(diff_mode == diff_channel);
	}

	return ret;
}
