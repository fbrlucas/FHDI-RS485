/*
 * device_info.c
 *
 *  Created on: Jun 15, 2016
 *      Author: DiLuka
 */

#include <stdint.h>
#include "device_info.h"



//Initialize here, the value of the struct points inside the array
//point_t point_1 = {1,"TEMP",8,32,2,0};
//point_t point_2 = {2,"UMID",8,57,0,0};
point_t points[N_POINT] = {3,"TEMP",7,32,2,0, 7,"UMID",8,57,0,0};

//The first variable (address) is a number from 1 to 32 (0x10 to 0x2F)

/*
	uint8_t address;
	char name[8];
    uint8_t type;
    uint8_t unit;
    uint8_t rights;
    uint8_t value;
 */
