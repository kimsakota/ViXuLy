#ifndef SYSTEM_CONFIG_H_
#define SYSTEM_CONFIG_H_

#define F_CPU				8000000UL

#define UART_USE_SOFT		1
#define UART_BAUDRATE		9600

#define FRAME_HEADER		0xAA
#define FRAME_MAX_PAYLOAD	8
#define FRAME_MAX_DATA       8

#define ROM_SIZE             0x4000
#define RAM_SIZE             0x4000

#define MEM_ROM_START        0x0000
#define MEM_ROM_END          0x3FFF

#define MEM_RAM_START        0x4000
#define MEM_RAM_END          0x7FFF

#define MEM_8255_START       0x8000
#define MEM_8255_END         0x8003

#define  DEVICE_COUNT		8

#endif /* SYSTEM_CONFIG_H_ */