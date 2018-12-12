/*
 * peak_detect.c
 *
 *  Created on: Dec 12, 2018
 *      Author: Dominic Doty
 */

/* HEADER */
#include "peak_detect.h"

/* DEFINES AND STATIC DATA */
#define dBFS_LUT_COUNTS	{0,1,3,7,15,31,63,127,255,511,1023,2047,4095,8191,16383,32767}
#define dBFS_LUT_dB	{-127000,-90000,-81000,-73000,-67000,-60000,-54000,-48000,-42000,-36000,-30000,-24000,-18000,-12000,-6000,0}
#define dBFS_LUT_SLOPE {0,1968048048,147456000,65536000,24576000,14336000,6144000,3072000,1536000,768000,384000,192000,96000,48000,24000,12000}
#define dBFS_LUT_ENTRIES 16

static int32_t 	dBFS_dB[] = dBFS_LUT_dB;
static int32_t dBFS_Slope[] = dBFS_LUT_SLOPE;

/* FUNCTION DEFINITIONS */
uint32_t peak_output(volatile int32_t* buffer, uint8_t buffer_size, uint8_t decay_shift)
{
	// Calc Decay Number
	static uint32_t decay_number = 0;
	decay_number >>= decay_shift;

	// Find Max in Buffer
	uint32_t max = 0;
	for(volatile int32_t* ptr = &buffer[0]; ptr < &buffer[buffer_size]; ptr++)
	{
		if(abs(*ptr) > max)
		{
			max = abs(*ptr);
		}
	}

	if(max > decay_number)
	{
		decay_number = max;
	}

	return decay_number;
}

// Take a ADC Reading and Convert to 16 bit scale dBFS
int32_t dbfs_output(uint32_t input)
{
	int32_t output = 0;

	// Find the LUT entry we need to look near
	uint8_t index = 0;
	uint32_t input_temp = input;
	while(input_temp)
	{
		input_temp >>= 1;
		index++;
	}
	if(index == 0)
	{
		output = dBFS_dB[index];
	}
	else
	{
		// Interpolate
		int32_t db_smaller = dBFS_dB[index - 1];
		int32_t slope = dBFS_Slope[index];

		// Line
		output = ((input*slope)/(2<<15)) + db_smaller;
	}

	return output;
}

void pretty_print(int32_t sample, uint8_t scale_shift)
{
	uint32_t limit = ( (abs(sample)) >> scale_shift);
	printf("%0*d>\n", limit, 0);
}
