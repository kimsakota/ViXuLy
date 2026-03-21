#ifndef PPI8255_H_
#define PPI8255_H_

#include <stdint.h>

// Driver điều khiển IC mở rộng I/O Intel 8255 

// Hàm này được gọi để khởi tạo IC 8255, thiết lập các chân điều khiển và data bus, và gửi cấu hình mặc định vào thanh ghi điều khiển của 8255 để thiết lập chế độ hoạt động. Cụ thể, nó cấu hình 8255 ở Mode 0 với tất cả các cổng (Port A, Port B, Port C) được thiết lập làm output. Điều này có nghĩa là tất cả các cổng sẽ xuất tín hiệu ra ngoài để điều khiển các thiết bị được kết nối với chúng.
void ppi8255_init(void);

// Hàm này được sử dụng để ghi một byte dữ liệu (value) vào Port A của IC 8255. Port A là một trong ba cổng I/O chính của 8255, và việc ghi dữ liệu vào đó sẽ điều khiển trạng thái của các thiết bị được kết nối với Port A. Ví dụ, nếu value = 0xFF (11111111 trong nhị phân), thì tất cả các thiết bị được điều khiển bởi Port A sẽ được bật, trong khi nếu value = 0x00 (00000000 trong nhị phân), thì tất cả các thiết bị sẽ được tắt.
void ppi8255_write_portA(uint8_t value);

#endif /* PPI8255_H_ */