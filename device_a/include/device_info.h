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


typedef union point_type_u
{
	uint8_t _ui8; //byte
	int8_t  _i8; //byte
	uint16_t _us16; //short
	int16_t _s16; //short
	uint32_t _ul32; //long
	int32_t _l32; //long
	uint64_t _ull64; //long long
	int64_t _ll64; //long long
	float _f;
	double _d;
	//uint32_t ui32;
} point_type_t;

typedef struct point_s
{
	uint8_t address;
	char name[8];
    uint8_t type;
    uint8_t unit;
    uint8_t rights;
    point_type_t value;
} point_t;
#ifdef __cplusplus
}
#endif
#endif /* DEVICE_A_INCLUDE_DEVICE_INFO_H_ */
