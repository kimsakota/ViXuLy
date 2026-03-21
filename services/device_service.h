#ifndef DEVICE_SERVICE_H
#define DEVICE_SERVICE_H

#include <stdint.h>

// Hàm này được gọi để khởi tạo trạng thái ban đầu của thiết bị, thường là tắt tất cả các thiết bị được điều khiển bởi Port A của 8255. Nó đảm bảo rằng hệ thống bắt đầu ở một trạng thái xác định và an toàn.
void device_service_init();

// Hàm này cho phép thiết lập trạng thái của tất cả các thiết bị được điều khiển bởi Port A của 8255 cùng một lúc. Tham số 'value' là một byte (8 bit) mà mỗi bit đại diện cho trạng thái của một thiết bị cụ thể (1 = ON, 0 = OFF). Ví dụ, nếu value = 0x05 (00000101 trong nhị phân), thì thiết bị 0 và thiết bị 2 sẽ được bật, trong khi các thiết bị khác sẽ tắt.
void device_service_set_all(uint8_t value);

// Hàm này cho phép bật một thiết bị cụ thể được điều khiển bởi Port A của 8255 dựa trên chỉ số (index) của nó. Tham số 'index' là một số nguyên từ 0 đến 7, tương ứng với các bit trong byte điều khiển. Ví dụ, nếu index = 3, thì thiết bị thứ 3 sẽ được bật (bit thứ 3 của Port A sẽ được đặt thành 1).
void device_service_turn_on(uint8_t index);

// Hàm này cho phép tắt một thiết bị cụ thể được điều khiển bởi Port A của 8255 dựa trên chỉ số (index) của nó. Tham số 'index' là một số nguyên từ 0 đến 7, tương ứng với các bit trong byte điều khiển. Ví dụ, nếu index = 3, thì thiết bị thứ 3 sẽ được tắt (bit thứ 3 của Port A sẽ được đặt thành 0).
void device_service_turn_off(uint8_t index);

// Hàm này trả về trạng thái hiện tại của tất cả các thiết bị được điều khiển bởi Port A của 8255 dưới dạng một byte (8 bit). Mỗi bit trong byte này đại diện cho trạng thái của một thiết bị cụ thể (1 = ON, 0 = OFF). Ví dụ, nếu hàm trả về 0x05 (00000101 trong nhị phân), thì thiết bị 0 và thiết bị 2 đang bật, trong khi các thiết bị khác đang tắt.
uint8_t device_service_get_state();

#endif /* DEVICE_SERVICE_H_ */