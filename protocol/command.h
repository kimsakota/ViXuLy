#ifndef COMMAND_H_
#define COMMAND_H_

#define CMD_SET_ALL     0x01   // Đặt trạng thái toàn bộ 8 thiết bị cùng lúc (data[0] = bitmask)
#define CMD_SET_SINGLE  0x02   // Điều khiển 1 thiết bị (data[0]=index, data[1]=0/1)
#define CMD_READ_SENSOR 0x03   // Yêu cầu đọc cảm biến ngay lập tức
#define CMD_SENSOR_DATA 0x04   // AVR push: 8 byte dòng điện + 1 byte device_state = 9 byte

#define CMD_ACK         0x81   // Phản hồi thành công
#define CMD_NACK        0x82   // Phản hồi lỗi

#endif /* COMMAND_H_ */