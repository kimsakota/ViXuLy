#ifndef DEVICE_SERVICE_H
#define DEVICE_SERVICE_H

#include <stdint.h>

// Hàm khởi tạo dịch vụ thiết bị, có thể thiết lập trạng thái ban đầu hoặc cấu hình cần thiết.
void device_service_init();

// Hàm để thiết lập trạng thái của tất cả thiết bị cùng một lúc, có thể dùng để bật hoặc tắt tất cả.
void device_service_set_all(uint8_t value);

// Hàm để thiết lập trạng thái của một thiết bị cụ thể dựa trên chỉ số (index), có thể dùng để bật hoặc tắt thiết bị đó.
void device_service_turn_on(uint8_t index);

// Hàm để tắt một thiết bị cụ thể dựa trên chỉ số (index).
void device_service_turn_off(uint8_t index);

// Hàm để chuyển đổi trạng thái của một thiết bị cụ thể dựa trên chỉ số (index), nếu đang bật sẽ tắt và ngược lại.
void device_service_toggle(uint8_t index);

// Hàm để lấy trạng thái hiện tại của tất cả thiết bị, có thể trả về một giá trị tổng hợp hoặc trạng thái của từng thiết bị.
uint8_t device_service_get_state();

#endif /* DEVICE_SERVICE_H_ */