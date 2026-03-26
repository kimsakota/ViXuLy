#ifndef FRAME_H_
#define FRAME_H_

#include <stdint.h>
#include <stdbool.h>

// FRAME_HEADER represents the first byte of a valid data frame in the protocol. 
// Uses 0xAA value so the system knows when a new frame begins.
#define FRAME_HEADER 0xAA
// FRAME_MAX_DATA is the max number of data bytes in a single frame.
#define FRAME_MAX_DATA 8

// Data structure storing information of a parsed frame: cmd (1 byte), 
// len (1 byte - number of actual data bytes), and data array.
typedef struct {
	uint8_t cmd;
	uint8_t len;
	uint8_t data[FRAME_MAX_DATA];
} frame_t;

// Initializes data structure or state required for frame processing.
void frame_init();

// Parses incoming bytes and attempts to construct a complete data frame.
// Returns true when a valid frame with matching signature and checksum is found.
bool frame_parse_byte(uint8_t byte, frame_t* out);

// Calculates checksum for a data frame based on its components: cmd, len, and data.
// Value will verify if data was not corrupted during transmission.
uint8_t frame_calculate_checksum(uint8_t cmd, uint8_t len, uint8_t* data);

// Builds and transmits a complete frame over UART.
// Format: [HEADER][cmd][len][data[0]..data[len-1]][checksum]
void frame_send(uint8_t cmd, uint8_t len, uint8_t* data);

#endif /* FRAME_H_ */