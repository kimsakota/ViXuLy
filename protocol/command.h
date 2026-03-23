#ifndef COMMAND_H_
#define COMMAND_H_


#define CMD_SET_ALL     0x01
#define CMD_SET_SINGLE  0x02

// Acknowledge command signaling "Received and successfully executed" 
#define CMD_ACK 		0x81
// Negative Acknowledge command signaling "Error, cannot execute"
#define CMD_NACK 		0x82

#endif /* COMMAND_H_ */