/*
 * device_info.h
 *
 *  Created on: Jun 15, 2016
 *      Author: DiLuka
 */

#ifndef DEVICE_A_INCLUDE_DEVICE_INFO_H_
#define DEVICE_A_INCLUDE_DEVICE_INFO_H_

#ifdef __cplusplus
extern "C" {
#endif


//Sensor Version
#define CMD_VERSION 1

//Sensor Identification
#define MODEL    "STARKIL1"
#define MANUF    "EMPIRE"
#define ID       1666111666
#define REV      1
#define N_POINT  2 //Contar quantos points tinha


typedef struct point_s
{
	uint8_t address;
	char name[8];
    uint8_t type;
    uint8_t unit;
    uint8_t rights;
    uint8_t value;
} point_t;


#ifdef __cplusplus
}
#endif
#endif /* DEVICE_A_INCLUDE_DEVICE_INFO_H_ */
