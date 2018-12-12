/*
 * peak_detect.h
 *
 *  Created on: Dec 12, 2018
 *      Author: Dominic Doty
 */

#ifndef PEAK_DETECT_H_
#define PEAK_DETECT_H_

/* INCLUDES */
#include "MKL25Z4.h"
#include "stddef.h"
#include "stdlib.h"
#include "fsl_common.h"
#include <stdio.h>

/* DEFINES & TYPEDEFS */


/* FUNCTION DECLARATIONS */

// Find the Peak in a buffer, Find the decay of the last sample, return the larger
uint16_t peak_output(volatile int16_t* buffer, uint8_t buffer_size, uint8_t decay_shift);

// Take a ADC Reading and Convert to 16 bit scale dBFS
int16_t dbfs_output(uint16_t input);

// Print a graphic line proportional to the input
void pretty_print(uint16_t sample, uint8_t scale_shift);


#endif /* PEAK_DETECT_H_ */
