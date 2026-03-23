# Coding Rules cho Copilot

Dưới đây là một số luật và quy định bắt buộc khi phát triển thêm mã nguồn trong dự án này:

## 1. Quy ước chung
- Dùng `uint8_t`, `uint16_t`, `bool` thay vì các kiểu số nguyên thông thường.
- Luôn sử dụng header guard (ví dụ: `#ifndef MODULE_H_ / #define MODULE_H_`) cho mọi file `.h`.
- File `.h` chỉ chứa khai báo (declarations), cài đặt thực tế (implementations) phải nằm trong `.c`.
- **Tuyệt đối không** hard-code các chân (pin) trong các file driver hay app. Phải lấy từ `board.h`.
- **Tuyệt đối không** hard-code address map ngoài `system_config.h` và `memory.c`.
- Không gọi trực tiếp các thanh ghi AVR (`PORTx`, `PINx`, v.v..) trong file `app.c`. Luôn giữ abstraction.
- `main.c` phải cực kỳ mỏng, chỉ khởi tạo và gọi `app_task()` trong vòng lặp vô hạn.
- Đảm bảo tính đơn nhiệm (Single Responsibility) cho mỗi module.

## 2. Quy ước tên hàm
- `uart_*` cho module UART.
- `ppi8255_*` cho driver chip 8255.
- `memory_*`, `io_map_*` cho memory layer.
- `cpu_bus_*` cho lớp CPU bus.
- `device_service_*` cho lớp quản lý logic phần mềm thiết bị.
- `frame_*`, `cmd_*` cho phần giao thức (protocol).
- `app_*` cho module điều phối ứng dụng chính.

## 3. Quy ước biến nội bộ
- Mọi biến trạng thái (state) nội bộ của từng module phải được đặt từ khóa `static` bên trong file `.c` để ẩn dữ liệu đóng gói. Cung cấp getter/setter để tương tác (ví dụ `device_state`).
## 4. Quản lý tài liệu (Documentation)
- Khi sửa đổi mã nguồn, tính năng hoặc thêm logic mới, bắt buộc phải đọc các file markdown trong thư mục docs/.
- Nếu có bất kỳ sự thay đổi nào về kiến trúc, luồng hệ thống, memory map, tài liệu (các file .md) cũng phải được xem xét và cập nhật lại cho đúng với hiện trạng thực tế của project.
