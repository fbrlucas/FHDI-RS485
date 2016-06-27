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
point_t points[N_POINT] = { 2,"REDLED",0,32,2,0,
							3,"SWITCH",1,57,0,0,
							4,"TEMP",2,32,0,0,
							5,"PWM",3,57,2,0,
							6,"UI32",4,57,2,0,
							7,"I32",5,57,2,0,
							8,"UI64",6,57,2,0,
							9,"I64",7,57,2,0,
							10,"FLOAT",8,57,2,1.0,
						    11,"DOUBLE",9,57,2,0};

//The first variable (address) is a number from 1 to 32 (0x10 to 0x2F)

/*
	uint8_t address;
	char name[8];
    uint8_t type;
    uint8_t unit;
    uint8_t rights;
    uint8_t value;
 */
