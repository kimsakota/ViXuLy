# Phân tích dự án điều khiển hệ vi xử lý 8-bit và 8255 qua UART

Tài liệu này mô tả chi tiết kiến trúc, mục tiêu và lộ trình phát 
triển của dự án firmware vi xử lý 8-bit dùng ATmega16.

## 1. Mục tiêu hệ thống phần mềm

Thiết kế firmware cho hệ vi xử lý 8-bit dùng ATmega16 làm CPU, có:
- Không gian địa chỉ 16-bit
- ROM mô phỏng
- RAM mô phỏng
- 8255 memory-mapped I/O
- UART giao tiếp với lớp ngoài (Terminal trước, ESP32 sau)
- Đọc dòng điện thiết bị qua ADC0804 + 74HC4051 (thông qua 8255 Port B/C)
- Kiến trúc module rõ ràng, dễ mở rộng

Hệ thống cần hỗ trợ 2 mức:
- **Mức 1:** chạy demo cơ bản (điều khiển LED qua 8255, gửi/nhận 
  UART, test command).
- **Mức 2:** đúng bản chất vi xử lý (có memory map, có RAM/ROM mô 
  phỏng, có giải mã địa chỉ bằng phần mềm, có truy cập bus CPU kiểu 
  read/write).

## 2. Triết lý kiến trúc

Hệ thống được thiết kế theo 6 lớp để mô phỏng một vi xử lý hoàn chỉnh:

`main` -> `app` -> `protocol` & `services` -> `core` -> `drivers` -> `bsp / config`

**Ý nghĩa các lớp:**
- **config**: Cấu hình toàn hệ thống.
- **bsp**: Map chân phần cứng (Board Support Package).
- **drivers**: Nói chuyện trực tiếp với chip / ngoại vi (UART, 8255, ADC0804).
- **core**: Bản chất vi xử lý, giải mã địa chỉ, mô phỏng RAM/ROM, Memory-mapped I/O, và CPU bus.
- **services**: Logic hệ thống, quản lý và điều khiển trạng thái thiết bị (Device, Measurement).
- **protocol**: Xử lý UART frame, định nghĩa các command.
- **app**: Điều phối toàn bộ hoạt động.
- **main**: Entry point.

## 3. Cấu trúc thư mục

```text
src/
├── config/
│   └── system_config.h
├── bsp/
│   ├── board.h / .c
│   └── gpio.h / .c
├── drivers/
│   ├── uart.h / .c
│   ├── ppi8255.h / .c
│   └── adc0804.h / .c
├── core/
│   ├── memory.h / .c
│   ├── io_map.h / .c
│   └── cpu_bus.h / .c
├── services/
│   ├── device_service.h / .c
│   └── measurement_service.h / .c
├── protocol/
│   ├── command.h / .c
│   └── frame.h / .c
├── app/
│   ├── app.h / .c
└── main.c
```

## 4. Kế hoạch phát triển theo giai đoạn

- **Giai đoạn 1 — Driver cơ bản và app tối thiểu:** Chạy được ATmega16 -> 8255 -> LED; UART gửi/nhận với Terminal; Code đã tách module.
- **Giai đoạn 2 — Device service:** Không điều khiển bằng hex rời rạc nữa; Quản lý trạng thái thiết bị theo bitmask.
- **Giai đoạn 3 — Memory map, RAM/ROM mô phỏng:** Đúng bản chất môn vi xử lý, có không gian địa chỉ 16-bit, có ROM, RAM, I/O mapped.
- **Giai đoạn 4 — CPU bus layer:** Tạo lớp trung gian mô phỏng CPU truy cập bus.
- **Giai đoạn 5 — Protocol frame UART:** Thay lệnh text bằng frame có cấu trúc để chuẩn bị nối ESP32.
- **Giai đoạn 6 — ADC và measurement ✓:** Đọc ADC0804 qua 8255 Port B/C, 74HC4051 chọn kênh, gửi kết quả về PC bằng frame `CMD_READ_CURRENT (0x03)`.

## 5. Memory map đề xuất

| Địa chỉ             | Kích thước | Chức năng               |
|---------------------|------------|-------------------------|
| `0x0000` - `0x3FFF` | 16KB       | ROM mô phỏng            |
| `0x4000` - `0x7FFF` | 16KB       | RAM mô phỏng            |
| `0x8000` - `0x8003` | 4 byte     | 8255 Mapped I/O         |
| `0x9000` - `0x90FF` | tùy chọn   | Thanh ghi hệ thống/debug|
| Còn lại             | -          | Reserved                |

### Mapping 8255
- `0x8000`: Port A
- `0x8001`: Port B
- `0x8002`: Port C
- `0x8003`: Control Register

## 6. Quy ước command UART

**Format Frame:** `[HEADER][CMD][LEN][DATA...][CHECKSUM]`  
- `HEADER = 0xAA`
- `CHECKSUM = XOR(HEADER, CMD, LEN, Data...)`

**Mã lệnh (CMD):**
- `0x01`: Set all devices — đặt trạng thái toàn bộ 8 thiết bị
- `0x02`: Set single device — bật/tắt 1 thiết bị theo index
- `0x03`: Read current — đọc dòng điện 1 thiết bị qua ADC0804
- `0x81`: ACK — phản hồi thành công (ASCII debug)
- `0x82`: NACK — phản hồi lỗi (ASCII debug)

*Lưu ý: Mọi quy ước về code style sẽ được tham chiếu trong tài liệu docs/agents.md.*
