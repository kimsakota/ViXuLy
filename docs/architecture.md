# Cơ chế hoạt động & Sơ đồ kiến trúc

Tài liệu này trình bày chi tiết luồng dữ liệu, chức năng của 
các khối trong phần mềm và cách dữ liệu được truyền từ thiết 
bị ngoại vi (PC/ESP32) xuống tận chân phần cứng (LED, vi điều
khiển).

## 1. Sơ đồ khối kiến trúc phần mềm

Hệ thống tuân thủ thiết kế phân lớp như sau:

```text
┌─────────────────────────────────────────────────────────┐
│                          MAIN                           │
│        (Entry point, init and infinite super loop)      │
└──────────────────────────┬──────────────────────────────┘
                           │
┌──────────────────────────▼──────────────────────────────┐
│                           APP                           │
│        (Điều phối luồng dữ liệu, polling ngoại vi)      │
└────┬─────────────────────────┬──────────────────────┬───┘
     │                         │                      │
┌────▼──────┐           ┌──────▼───────┐        ┌─────▼──────┐
│ PROTOCOL  │           │   SERVICES   │        │   CORE     │
│ (Frame    │           │ (Device Log- │        │ (CPU Bus,  │
│ Builder,  │           │ ic, Measure) │        │ Memory,    │
│ Commands) │           └──────┬───────┘        │ I/O Map)   │
└────┬──────┘                  │                └─────┬──────┘
     │                         │                      │
┌────▼─────────────────────────▼──────────────────────▼───┐
│                        DRIVERS                          │
│         (UART, 8255 (PPI), ADC0804, timers...)          │
└──────────────────────────┬──────────────────────────────┘
                           │
┌──────────────────────────▼──────────────────────────────┐
│                        BSP & HW                         │
│   (ATmega16, GPIO macros, Board Definitions, Config)    │
└─────────────────────────────────────────────────────────┘
```

---

## 2. Giải thích cơ chế theo từng khối

### 2.1 Lớp Main & App (Điều phối)
- **`main.c`**: Rất mỏng, gọi `app_init()` và chạy vòng lặp `while(1) { app_task(); }`.
- **`app_task()`**: Liên tục thực hiện việc *polling* cho hệ thống (đọc UART, xử lý luồng logic, gửi lại).

### 2.2 Lớp Protocol (Giao thức)
Quy định cách thiết bị giao tiếp với nhau bằng nhị phân thay vì chuỗi ASCII, nhằm tránh sai sót và tích hợp với chuẩn phổ biến.
- **`frame.c`**: State machine gồm các bước: `WAIT_HEADER` -> `READ_CMD` -> `READ_LEN` -> `READ_DATA` -> `READ_CHECKSUM`. 
- Frame chỉ được đưa lên ứng dụng nếu Checksum hợp lệ.

### 2.3 Lớp Services (Trạng thái và logic nghiệp vụ)
- Không cần biết LED nằm trên phần cứng nào (8255 hay chốt I2C). Nó lưu trữ trạng thái của tải (thiết bị).
- Ví dụ: `device_service` lưu 1 biến `static uint8_t device_state;`. Khi có tín hiệu chuyển đổi thiết bị số 3 sang ON, nó chỉnh bit số 3 lên `1`.

### 2.4 Lớp Core (Mô phỏng bản chất vi xử lý)
Tạo ra một không gian địa chỉ Memory-Map (16-bit) để giống bản chất của lý thuyết môn Vi xử lý.
- **CPU Bus**: CPU ảo yêu cầu `cpu_bus_read(addr)` hoặc `cpu_bus_write(addr, data)`.
- **Memory & I/O Map**: Phân giải địa chỉ (`addr`) để xác định CPU muốn thao tác với **ROM**, **RAM** hay là ngoại vi **8255** (0x8000).

### 2.5 Lớp Drivers & BSP (Giao tiếp phần cứng)
- Phụ trách thao tác mức điện áp (High/Low) thực tế theo *Timing Diagram* của nhà sản xuất.
- **PPI 8255 Driver**: Cấu hình xung RD (Read), WR (Write), CS (Chip Select), A0, A1 để xuất / nhận byte dữ liệu.

---

## 3. Cơ chế giao tiếp và Truyền dữ liệu (Ví dụ cụ thể)

Cách dữ liệu truyền qua các lớp khi người dùng yêu cầu bật toàn bộ LED trên Port A (Lệnh CMD = `0x01`).

### Luồng thực thi chi tiết lúc nhận (Receive):

1. **PC/ESP32 Gửi lệnh:**
   Gửi chuỗi byte hex: `AA 01 01 FF 55`
   - `AA` : Header
   - `01` : Command (Set All)
   - `01` : Length (1 byte data)
   - `FF` : Data
   - `55` : Checksum (AA ^ 01 ^ 01 ^ FF)

2. **Lớp Driver (UART):**
   `uart_read_char()` lấy được từng byte khi tín hiệu đi vào chân vi điều khiển.

3. **Lớp Protocol (Frame):**
   `app_task()` liên tục đẩy byte vừa nhận cho `frame_parse_byte()`. Parse xong ráp thành 1 struct `frame_t`.

4. **Lớp App xử lý:**
   ```c
   if (frame.cmd == CMD_SET_ALL) {
       device_service_set_all(frame.data[0]);
   }
   ```
   
5. **Lớp Services (Device):**
   Cập nhật biến trạng thái `device_state = 0xFF;` và gọi lệnh thao tác xuống (ở phiên bản phần cứng 8255 hiện tại hoặc lớp Core sau này).

6. **Lớp Driver & Cơ sở hạ tầng (8255):**
   Driver 8255 chọn địa chỉ Port A (`A1=0`, `A0=0`). Sau đó thực thi trình tự: ghi giá trị `0xFF` lên tín hiệu bus, CS = 0, chớp Write = 0 rồi lên 1. Tất cả thao tác này diễn ra trong vài micro-giây.

7. **Hoàn tất:** LED sáng bừng.

---

## 4. Quá trình phát triển tiếp theo

Kiến trúc này hiện được áp dụng đồng bộ, lớp này chỉ gọi lớp phía dưới nó để đảm bảo mã tái sử dụng. Khi sửa mã, cần tham chiếu file này và duy trì dòng dữ liệu theo mô hình trên.
