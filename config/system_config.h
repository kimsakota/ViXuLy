#ifndef SYSTEM_CONFIG_H_
#define SYSTEM_CONFIG_H_

#define F_CPU				8000000UL // Tần số thạch anh 8 MHz

#define UART_USE_SOFT		1 // Sử dụng UART phần mềm (bit-banging)
#define UART_BAUDRATE		9600 // Tốc độ baud cho UART, tương ứng với khoảng 104us cho mỗi bit

#define ROM_SIZE             0x100 // 256 bytes ROM, lưu trữ chương trình và dữ liệu tĩnh
#define RAM_SIZE             0x100 // 256 bytes RAM, lưu trữ dữ liệu động và biến tạm thời

#define MEM_ROM_START        0x0000 // Địa chỉ bắt đầu của ROM trong không gian địa chỉ 16-bit
#define MEM_ROM_END          (MEM_ROM_START + ROM_SIZE - 1) // Địa chỉ kết thúc của ROM, tính bằng địa chỉ bắt đầu + kích thước - 1

#define MEM_RAM_START        0x4000 // Địa chỉ bắt đầu của RAM, tách biệt với ROM để tránh xung đột địa chỉ
#define MEM_RAM_END          (MEM_RAM_START + RAM_SIZE - 1) // Địa chỉ kết thúc của RAM, tính bằng địa chỉ bắt đầu + kích thước - 1

#define MEM_8255_START       0x8000 // Địa chỉ bắt đầu của 8255 PPI, thường được ánh xạ vào một vùng địa chỉ riêng để truy cập các cổng I/O
#define MEM_8255_END         0x8003 // Địa chỉ kết thúc của 8255 PPI, bao gồm 3 cổng dữ liệu và 1 cổng điều khiển

#define  DEVICE_COUNT		8 // Số lượng thiết bị được điều khiển, tương ứng với 8 bit của cổng A, B, C của 8255 PPI

#endif /* SYSTEM_CONFIG_H_ */