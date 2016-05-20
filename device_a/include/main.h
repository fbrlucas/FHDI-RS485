#ifndef __MAIN_H__
#define __MAIN_H__


//#ifndef NULL
//#define NULL 0
//#endif

#define CMD_MAX_SIZE 256
#define CMD_HDR_SIZE 4
#define CMD_TRL_SIZE 2

#define FRAME_FLAG 0x7E
#define FRAME_ESC  0x7D

#define CMD_DEV_ADDR 10

#define CMD_ITF_VER          0x00
#define CMD_IDENT            0x01
#define CMD_POINT_DESC_BASE  0x10
#define CMD_POINT_READ_BASE  0x30
#define CMD_POINT_WRITE_BASE 0x50

//Sensor Version
#define CMD_VERSION 1

//Sensor Identification
#define MODEL  "STARKIL1"
#define MANUF  "EMPIRE"
//#define ID     3237998090
#define ID     1666111666
#define REV    2
#define POINT  15

//Sensor Point Description




typedef struct cmd_s
{
    uint8_t dst;
    uint8_t src;
    uint8_t reg;
    uint8_t size;
    uint8_t payload[CMD_MAX_SIZE];
    uint16_t crc;
} cmd_t;

typedef union frame_u
{
    cmd_t cmd;
    uint8_t buffer[CMD_HDR_SIZE+CMD_MAX_SIZE+CMD_TRL_SIZE];  
} frame_t;

#endif /* __MAIN_H__ */

