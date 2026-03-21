# Phân tích dự án điều khiển `8255` qua `UART` (phiên bản hiện tại)

Tài liệu này mô tả đúng theo mã nguồn đang có trong workspace.

## 1) Mục tiêu dự án

- Vi điều khiển `ATmega16` nhận lệnh qua `Software UART` (`9600 bps`).
- Lệnh được đóng gói theo frame nhị phân.
- Khi frame hợp lệ, firmware điều khiển IC `8255` để xuất dữ liệu ra `Port A` (điều khiển LED).

---

## 2) Kiến trúc thư mục và vai trò chính

### `main.c`
- Điểm vào chương trình.
- Gọi `app_init()` một lần.
- Chạy vòng lặp vô hạn `app_task()`.

### `config/system_config.h`
- Cấu hình hệ thống:
  - `F_CPU = 8000000UL`
  - `UART_BAUDRATE = 9600`
  - `FRAME_HEADER = 0xAA`
  - `DEVICE_COUNT = 8`

### `bsp/board.h`
- Ánh xạ chân phần cứng:
  - Bus dữ liệu `8255` dùng `PORTD`.
  - Bus điều khiển `8255` dùng `PORTB` (`WR`, `RD`, `A0`, `A1`, `CS`).
  - Soft UART dùng `PORTC`:
    - `PC0` = `TX`
    - `PC1` = `RX`

### `drivers/uart.c`, `drivers/uart.h`
- Driver `Software UART` (bit-banging):
  - `uart_init()`
  - `uart_write_char()`, `uart_write_string()`
  - `uart_read_char()` (non-blocking)

### `protocol/frame.c`, `protocol/frame.h`
- Parser frame theo state machine.
- Định dạng frame:
  - `[HEADER][CMD][LEN][DATA...][CHECKSUM]`
- `HEADER = 0xAA`
- `CHECKSUM = XOR(HEADER, CMD, LEN, tất cả DATA)`
- API chính:
  - `frame_init()`
  - `frame_parse_byte()`
  - `frame_calculate_checksum()`

### `protocol/command.h`
- Mã lệnh hiện dùng:
  - `CMD_SET_ALL = 0x01`
  - `CMD_SET_SINGLE = 0x02`
- Có define `CMD_ACK`, `CMD_NACK` nhưng hiện tại app phản hồi bằng text (`"ACK\r\n"`, `"NACK\r\n"`).

### `drivers/ppi8255.c`, `drivers/ppi8255.h`
- Driver ghi dữ liệu sang `8255`:
  - `ppi8255_init()` cấu hình `8255` mode 0, các port output (`write(3, 0x80)`).
  - `ppi8255_write_portA(value)` ghi trực tiếp ra `Port A`.

### `services/device_service.c`, `services/device_service.h`
- Lớp quản lý trạng thái thiết bị `8-bit`:
  - `device_service_set_all(value)`
  - `device_service_turn_on(index)`
  - `device_service_turn_off(index)`
  - `device_service_get_state()`

### `app/app.c`, `app/app.h`
- Luồng ứng dụng chính:
  - `app_init()`:
    - init UART, 8255, device service, frame parser
    - gửi `"System Ready\r\n"`
  - `app_task()`:
    - đọc từng byte UART
    - parse frame
    - xử lý lệnh
    - trả `ACK/NACK`

---

## 3) Luồng xử lý lệnh trong `app_task()`

1. Nếu `uart_read_char()` có byte mới:
   - (hiện đang có marker debug tạm thời) gửi `"RB\r\n"`.
2. Byte được đưa vào `frame_parse_byte()`.
3. Nếu parse thành công frame hoàn chỉnh:
   - (debug tạm thời) gửi `"FOK\r\n"`.
   - Nếu `cmd = CMD_SET_ALL` và `len = 1`:
     - `device_service_set_all(data[0])`
     - gửi `"ACK\r\n"`
   - Nếu `cmd = CMD_SET_SINGLE` và `len = 2`:
     - `index = data[0]`, `value = data[1]`
     - `value != 0` => bật, ngược lại => tắt
     - gửi `"ACK\r\n"`
   - Trường hợp khác:
     - gửi `"NACK\r\n"`

---

## 4) Frame mẫu

### Lệnh bật/tắt toàn bộ (`CMD_SET_ALL`)
- Ví dụ gửi `0xFF`:
  - `AA 01 01 FF 55`

Giải thích checksum:
- `0xAA ^ 0x01 ^ 0x01 ^ 0xFF = 0x55`

---

## 5) Công cụ test PC `tool.ps1`

Script `tool.ps1`:
- Tạo frame đúng chuẩn checksum.
- Gửi qua cổng COM.
- Đọc phản hồi từ MCU (`ACK/NACK` hoặc text debug).
- Hỗ trợ lệnh:
  - `all <hex>`
  - `single <index> <0|1>`
  - `raw <cmd_hex> <data_hex...>`

---

## 6) Lưu ý kỹ thuật khi mô phỏng/triển khai

- Soft UART nhạy với sai số clock: cần bảo đảm `F_CPU` và clock mô phỏng trùng nhau (`8MHz`).
- Cần đúng mức tín hiệu UART (TTL/RS232) và đúng chiều `TX/RX`.
- Nếu nhận được `System Ready` nhưng lệnh không chạy, nên kiểm tra theo tầng:
  1. Có nhận byte UART chưa (`RB`)
  2. Có parse thành frame hợp lệ chưa (`FOK`)
  3. Có xung ghi sang `8255` và LED wiring/reset đúng chưa

---

## 7) Các file hiện còn trống/chưa dùng

- `protocol/command.c`
- `bsp/board.c`
- `drivers/adc0804.c`

Các file này có thể để dành cho mở rộng sau.
