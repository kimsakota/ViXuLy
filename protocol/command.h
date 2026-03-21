#ifndef COMMAND_H_
#define COMMAND_H_


#define CMD_SET_ALL     0x01
#define CMD_SET_SINGLE  0x02

// Lệnh phản hồi báo hiệu "Đã nhận và thực hiện lệnh thành công" (Acknowledge) 
#define CMD_ACK 		0x81
// Lệnh phản hồi báo hiệu "Lỗi, không thể thực hiện lệnh" (Negative Acknowledge)
#define CMD_NACK 		0x82

#endif /* COMMAND_H_ */