# Tài liệu ADC & Đo Dòng Điện

> **Mục tiêu:** Đọc giá trị dòng điện tiêu thụ của từng thiết bị trên Port A của 8255 thông qua chuỗi ADC0804 + 74HC4051, sau đó phản hồi kết quả về PC bằng frame UART.

---

## MỤC LỤC

1. [Tổng quan luồng đo dòng](#1-tổng-quan-luồng-đo-dòng)
2. [Lý thuyết ADC0804](#2-lý-thuyết-adc0804)
3. [Lý thuyết 74HC4051 — Analog Mux 8 kênh](#3-lý-thuyết-74hc4051--analog-mux-8-kênh)
4. [Sơ đồ kết nối phần cứng](#4-sơ-đồ-kết-nối-phần-cứng)
5. [Ánh xạ chân — board.h](#5-ánh-xạ-chân--boardh)
6. [Driver — adc0804.c / adc0804.h](#6-driver--adc0804c--adc0804h)
7. [Service — measurement_service.c / measurement_service.h](#7-service--measurement_servicec--measurement_serviceh)
8. [Protocol — CMD_READ_CURRENT (0x03)](#8-protocol--cmd_read_current-0x03)
9. [App — Xử lý lệnh đọc dòng trong app_task()](#9-app--xử-lý-lệnh-đọc-dòng-trong-app_task)
10. [Luồng dữ liệu đầu cuối](#10-luồng-dữ-liệu-đầu-cuối)
11. [Vấn đề chia sẻ bus dữ liệu (PORTD)](#11-vấn-đề-chia-sẻ-bus-dữ-liệu-portd)

---

## 1. Tổng quan luồng đo dòng

Mỗi thiết bị trên Port A của 8255 (bit 0–7) có một cảm biến dòng điện tương tự riêng. Tín hiệu analog từ 8 cảm biến này được đưa vào **74HC4051** (bộ chọn kênh 1-trong-8). Ngõ ra duy nhất của 74HC4051 nối vào ngõ vào analog **VIN+** của **ADC0804** để chuyển đổi thành giá trị số 8-bit. AVR điều khiển cả hai chip qua **PORTA**.

```
Cảm biến dòng CH0 ──┐
Cảm biến dòng CH1 ──┤
Cảm biến dòng CH2 ──┤
Cảm biến dòng CH3 ──┼──► [74HC4051] ──► VIN+ [ADC0804] ──► PORTD ──► ATmega
Cảm biến dòng CH4 ──┤        ▲                  ▲
Cảm biến dòng CH5 ──┤        │                  │
Cảm biến dòng CH6 ──┤    PA4/PA5/PA6        PA0/PA1/PA2/PA3
Cảm biến dòng CH7 ──┘   (A, B, C select)   (CS, WR, RD, INTR)
```

**Ánh xạ kênh ↔ thiết bị:**

| Kênh 74HC4051 | Thiết bị trên Port A 8255 |
|:---:|:---:|
| CH0 | Device 0 (bit 0) |
| CH1 | Device 1 (bit 1) |
| … | … |
| CH7 | Device 7 (bit 7) |

---

## 2. Lý thuyết ADC0804

### 2.1 Giới thiệu chip

ADC0804 là bộ chuyển đổi tương tự – số (ADC) **8-bit**, giao diện song song tương thích TTL/CMOS, hoạt động theo nguyên lý **xấp xỉ liên tiếp (Successive Approximation)**. Điện áp tham chiếu mặc định là **VCC/2**, dải đầu vào 0 – VCC.

- **Độ phân giải:** 8-bit → 256 bước
- **Thời gian chuyển đổi:** ~110 µs (ở CLK ~640 kHz)
- **Giao diện:** song song 8-bit, tương thích với bus dữ liệu của vi điều khiển

### 2.2 Sơ đồ chân quan trọng

| Chân | Tên | Mức tích cực | Chức năng |
|---|---|---|---|
| CS# | Chip Select | LOW | Kích hoạt chip |
| WR# | Write / Start | LOW → HIGH | Cạnh lên kích hoạt bắt đầu chuyển đổi |
| RD# | Read | LOW | Cho phép xuất dữ liệu ra bus |
| INTR# | Interrupt | LOW | Báo hiệu chuyển đổi hoàn thành |
| DB0–DB7 | Data Bus | — | Kết quả 8-bit (shared với PORTD) |
| VIN+ | Analog In (+) | — | Tín hiệu analog cần đo |
| VIN− | Analog In (−) | — | Thường nối GND (single-ended) |

### 2.3 Chuỗi thao tác ghi – đọc (Timing)

**Bước 1 — Khởi động chuyển đổi (Start Conversion):**

```
CS#   ‾‾‾|____________|‾‾‾
WR#   ‾‾‾‾‾‾|____|‾‾‾‾‾‾‾
               ↑
       Cạnh lên WR# → ADC bắt đầu chuyển đổi
```

**Bước 2 — Chờ hoàn thành (Wait INTR#):**

```
INTR# ‾‾‾‾‾‾‾‾‾‾|_______|‾‾‾
                  ↑
         INTR# xuống LOW → chuyển đổi xong (~110µs)
```

**Bước 3 — Đọc kết quả (Read Result):**

```
CS#   ‾‾‾|____________|‾‾‾
RD#   ‾‾‾‾‾‾|____|‾‾‾‾‾‾‾
DATA  --------[VALID]-----
              ↑
      AVR đọc PORTD trong khi RD# = LOW
```

---

## 3. Lý thuyết 74HC4051 — Analog Mux 8 kênh

### 3.1 Giới thiệu chip

**74HC4051** là bộ ghép kênh tương tự (Analog Multiplexer) **8 vào – 1 ra**, dòng CMOS tốc độ cao. Chip chọn 1 trong 8 kênh analog dựa trên 3 bit địa chỉ **A (bit 0), B (bit 1), C (bit 2)**.

- **8 kênh analog** (Y0–Y7)
- **1 ngõ ra chung** (Z) → nối vào VIN+ của ADC0804
- **3 bit select:** A, B, C (tương ứng PA4, PA5, PA6 trên ATmega)
- **Chân Enable (E#):** active LOW — nối GND để luôn bật

### 3.2 Bảng chân trị chọn kênh

| C (PA6) | B (PA5) | A (PA4) | Kênh được chọn |
|:---:|:---:|:---:|:---:|
| 0 | 0 | 0 | Y0 → CH0 (Device 0) |
| 0 | 0 | 1 | Y1 → CH1 (Device 1) |
| 0 | 1 | 0 | Y2 → CH2 (Device 2) |
| 0 | 1 | 1 | Y3 → CH3 (Device 3) |
| 1 | 0 | 0 | Y4 → CH4 (Device 4) |
| 1 | 0 | 1 | Y5 → CH5 (Device 5) |
| 1 | 1 | 0 | Y6 → CH6 (Device 6) |
| 1 | 1 | 1 | Y7 → CH7 (Device 7) |

**Mối liên hệ với code:** `channel` truyền vào `adc0804_read(channel)` được tách bit:

```c
// bit 0 → A (PA4)
// bit 1 → B (PA5)
// bit 2 → C (PA6)
if (channel & 0x01) PA4 = HIGH; else PA4 = LOW;  // A
if (channel & 0x02) PA5 = HIGH; else PA5 = LOW;  // B
if (channel & 0x04) PA6 = HIGH; else PA6 = LOW;  // C
```

---

## 4. Sơ đồ kết nối phần cứng

```
ATmega (PORTA)                 74HC4051              ADC0804
─────────────────────────────────────────────────────────────
PA0 ── CS#  ─────────────────────────────────────► CS#
PA1 ── WR#  ─────────────────────────────────────► WR#
PA2 ── RD#  ─────────────────────────────────────► RD#
PA3 ── INTR# ◄────────────────────────────────────  INTR#
PA4 ── A    ─────────────────► A (pin 11)
PA5 ── B    ─────────────────► B (pin 10)
PA6 ── C    ─────────────────► C (pin 9)
                GND ──────────► E# (pin 6)           ← luôn bật

                               Z (pin 3) ───────────► VIN+ (pin 6)

ATmega (PORTD) ──────────────────────────────────► DB0–DB7
                         ↑ Bus dùng chung với 8255
```

> **Lưu ý chia sẻ bus:** PORTD là bus dữ liệu 8-bit dùng chung giữa **8255** và **ADC0804**. Driver `adc0804_read()` phải chuyển PORTD sang chế độ Input trước khi đọc, và phục hồi về Output sau khi đọc xong để không ảnh hưởng đến 8255.

---

## 5. Ánh xạ chân — board.h

```c
// ===== ADC0804 =====
// ADC0804 shares the data bus (PORTD) with PPI 8255
// Control signals and analog mux (74HC4051) channel select are on PORTA
#define ADC_DDR      DDRA
#define ADC_PORT     PORTA
#define ADC_PIN      PINA

#define ADC_CS_PIN   PA0  // Chip Select  (active low)
#define ADC_WR_PIN   PA1  // Start Conv.  (active low, rising edge triggers conversion)
#define ADC_RD_PIN   PA2  // Read output  (active low)
#define ADC_INTR_PIN PA3  // Conv. done   (input, active low from ADC0804)

// 74HC4051 analog mux channel select (PA4–PA6 → A, B, C → selects 1 of 8 channels)
#define ADC_CH_A_PIN PA4
#define ADC_CH_B_PIN PA5
#define ADC_CH_C_PIN PA6
```

**Tại sao dùng PORTA cho ADC?**
- PORTD đã bị chiếm bởi bus dữ liệu 8-bit (chung với 8255), không thể dùng cho control.
- PORTB đã bị chiếm bởi control bus của 8255 (CS, WR, RD, A0, A1).
- PORTC đã bị chiếm bởi UART phần mềm (TX, RX).
- → PORTA là cổng tự do duy nhất, đủ 7 chân cần thiết (PA0–PA6).

---

## 6. Driver — adc0804.c / adc0804.h

### 6.1 Khai báo (adc0804.h)

```c
// Khởi tạo chân điều khiển ADC0804 (CS, WR, RD là output; INTR là input)
// và chân chọn kênh 74HC4051 (CH_A, CH_B, CH_C).
void adc0804_init(void);

// Chọn kênh analog (0–7) qua 74HC4051, kích hoạt chuyển đổi trên ADC0804,
// chờ INTR xuống LOW rồi đọc và trả về kết quả 8-bit.
// Chia sẻ bus dữ liệu PORTD với PPI 8255; hướng bus được phục hồi sau khi đọc.
uint8_t adc0804_read(uint8_t channel);
```

### 6.2 `adc0804_init()`

```c
void adc0804_init(void)
{
    // CS, WR, RD, CH_A/B/C là output; INTR là input
    ADC_DDR |=  (1 << ADC_CS_PIN) | (1 << ADC_WR_PIN) | (1 << ADC_RD_PIN)
              | (1 << ADC_CH_A_PIN) | (1 << ADC_CH_B_PIN) | (1 << ADC_CH_C_PIN);
    ADC_DDR &= ~(1 << ADC_INTR_PIN);

    // Trạng thái mặc định: CS=HIGH, WR=HIGH, RD=HIGH (tất cả inactive)
    ADC_PORT |=  (1 << ADC_CS_PIN) | (1 << ADC_WR_PIN) | (1 << ADC_RD_PIN);
    // Chọn kênh mặc định: channel 0
    ADC_PORT &= ~((1 << ADC_CH_A_PIN) | (1 << ADC_CH_B_PIN) | (1 << ADC_CH_C_PIN));
}
```

**Giải thích:**
- `ADC_INTR_PIN` (PA3) phải là **input** vì đây là tín hiệu đầu ra từ ADC0804 báo chuyển đổi xong.
- Ba chân còn lại (CS, WR, RD) là **output** vì ATmega chủ động điều khiển ADC.
- Ba chân chọn kênh (CH_A, CH_B, CH_C) là **output** vì ATmega điều khiển 74HC4051.

### 6.3 `adc0804_read(uint8_t channel)`

```c
uint8_t adc0804_read(uint8_t channel)
{
    // Bước 1: Chọn 1 trong 8 kênh analog qua 74HC4051 (A=bit0, B=bit1, C=bit2)
    if (channel & 0x01) ADC_PORT |=  (1 << ADC_CH_A_PIN);
    else                ADC_PORT &= ~(1 << ADC_CH_A_PIN);

    if (channel & 0x02) ADC_PORT |=  (1 << ADC_CH_B_PIN);
    else                ADC_PORT &= ~(1 << ADC_CH_B_PIN);

    if (channel & 0x04) ADC_PORT |=  (1 << ADC_CH_C_PIN);
    else                ADC_PORT &= ~(1 << ADC_CH_C_PIN);

    // Bước 2: Kích hoạt chuyển đổi — CS low, WR low → high (cạnh lên)
    ADC_PORT &= ~(1 << ADC_CS_PIN);
    ADC_PORT &= ~(1 << ADC_WR_PIN);
    _delay_us(1);
    ADC_PORT |=  (1 << ADC_WR_PIN);   // ← cạnh lên WR# kích hoạt ADC

    // Bước 3: Chờ INTR# xuống LOW (chuyển đổi hoàn thành, ~110µs)
    uint8_t timeout = 200;
    while ((ADC_PIN & (1 << ADC_INTR_PIN)) && timeout--)
        _delay_us(1);

    // Bước 4: Đọc kết quả — chuyển PORTD thành input, assert RD low
    PPI_DATA_DDR  = 0x00;   // Bus dữ liệu → Input (tri-state AVR driver)
    PPI_DATA_PORT = 0x00;   // Tắt pull-up nội bộ

    ADC_PORT &= ~(1 << ADC_RD_PIN);
    _delay_us(1);
    uint8_t data = PPI_DATA_PIN;   // Đọc 8-bit kết quả từ PORTD
    ADC_PORT |=  (1 << ADC_RD_PIN);
    ADC_PORT |=  (1 << ADC_CS_PIN);

    // Bước 5: Phục hồi bus dữ liệu về output cho 8255
    PPI_DATA_DDR = 0xFF;

    return data;
}
```

**Cơ chế timeout:** Vòng lặp chờ INTR# xuống LOW tối đa **200 µs** (200 × 1µs). Nếu ADC không phản hồi (chip lỗi, nguồn mất), hàm vẫn thoát ra thay vì treo vô tận — đảm bảo hệ thống tiếp tục hoạt động.

**Tại sao cần `_delay_us(1)` trước khi đọc PORTD?**
Sau khi RD# xuống LOW, ADC0804 cần một khoảng trễ truyền (propagation delay, typ ~135 ns) để dữ liệu ổn định trên bus. `_delay_us(1)` = 8 chu kỳ @ 8MHz đảm bảo dữ liệu đã hợp lệ khi đọc `PPI_DATA_PIN`.

---

## 7. Service — measurement_service.c / measurement_service.h

### 7.1 Khai báo (measurement_service.h)

```c
// Khởi tạo measurement service (gọi adc0804_init() nội bộ).
void measurement_service_init(void);

// Đọc giá trị ADC 8-bit cho thiết bị tại chỉ số đã cho (0–7).
// Chỉ số kênh tương ứng với vị trí bit trên Port A của 8255.
uint8_t measurement_service_read(uint8_t index);
```

### 7.2 Implementation

```c
void measurement_service_init(void)
{
    adc0804_init();
}

uint8_t measurement_service_read(uint8_t index)
{
    if (index >= DEVICE_COUNT)   // DEVICE_COUNT = 8 (từ system_config.h)
        return 0;
    return adc0804_read(index);
}
```

**Vai trò của tầng service:**
- **Kiểm tra biên (boundary check):** `index >= DEVICE_COUNT` → trả về 0 thay vì cho phép truy cập kênh ngoài phạm vi.
- **Che giấu hardware:** Tầng `app` không biết chip ADC là gì, không biết cách giao tiếp — chỉ gọi `measurement_service_read(index)`.
- **Dễ mở rộng:** Nếu sau này thêm bộ lọc trung bình (averaging), hiệu chỉnh offset, hoặc đổi ADC sang I2C, chỉ cần sửa trong service này.

**Ví dụ đọc dòng device 3:**

```
measurement_service_read(3)
    → index = 3 < 8 → hợp lệ
    → adc0804_read(3)
        → 74HC4051: C=0, B=1, A=1 → chọn Y3 (kênh 3)
        → ADC0804: start → wait INTR# → read
        → trả về ví dụ: 0xB2
    → trả về 0xB2 lên app
```

---

## 8. Protocol — CMD_READ_CURRENT (0x03)

### 8.1 Định nghĩa lệnh (command.h)

```c
#define CMD_READ_CURRENT  0x03   // Yêu cầu đọc dòng điện của 1 thiết bị
```

### 8.2 Frame yêu cầu từ PC → AVR

```
┌────────┬──────┬─────┬───────┬──────────┐
│  0xAA  │ 0x03 │ 0x01│ index │ CHECKSUM │
└────────┴──────┴─────┴───────┴──────────┘
```

| Trường | Giá trị | Mô tả |
|---|---|---|
| HEADER | `0xAA` | Byte bắt đầu frame |
| CMD | `0x03` | CMD_READ_CURRENT |
| LEN | `0x01` | 1 byte data |
| DATA[0] | `0x00`–`0x07` | Index thiết bị cần đọc |
| CHECKSUM | `0xAA ^ 0x03 ^ 0x01 ^ index` | XOR checksum |

**Ví dụ — đọc dòng device 5:**
```
Byte:  AA   03   01   05   CS
CS = 0xAA ^ 0x03 ^ 0x01 ^ 0x05 = 0xAD
```

### 8.3 Frame phản hồi từ AVR → PC

```
┌────────┬──────┬─────┬───────┬────────┬──────────┐
│  0xAA  │ 0x03 │ 0x02│ index │ value  │ CHECKSUM │
└────────┴──────┴─────┴───────┴────────┴──────────┘
```

| Trường | Giá trị | Mô tả |
|---|---|---|
| HEADER | `0xAA` | Byte bắt đầu frame |
| CMD | `0x03` | Echo lại CMD_READ_CURRENT |
| LEN | `0x02` | 2 byte data |
| DATA[0] | `index` | Index thiết bị đã đọc |
| DATA[1] | `value` | Giá trị ADC 8-bit (0–255) |
| CHECKSUM | `0xAA ^ 0x03 ^ 0x02 ^ index ^ value` | XOR checksum |

**Phản hồi này được tạo bằng `frame_send()`:**

```c
uint8_t resp[2] = { index, value };
frame_send(CMD_READ_CURRENT, 2, resp);
```

---

## 9. App — Xử lý lệnh đọc dòng trong app_task()

### 9.1 Khởi tạo trong `app_init()`

```c
void app_init() {
    uart_init();
    ppi8255_init();
    memory_init();
    device_service_init();
    measurement_service_init();   // ← Khởi tạo ADC0804 + 74HC4051
    frame_init();

    uart_write_string("System Ready\r\n");
}
```

**Thứ tự:** `measurement_service_init()` được gọi sau `ppi8255_init()` vì cả hai dùng chung PORTD. Việc PPI init trước đặt `PPI_DATA_DDR = 0xFF` (output) là trạng thái mặc định đúng cho cả hai.

### 9.2 Xử lý CMD_READ_CURRENT trong `app_task()`

```c
else if (frame.cmd == CMD_READ_CURRENT && frame.len == 1) {
    uint8_t index = frame.data[0];
    uint8_t value = measurement_service_read(index);
    uint8_t resp[2] = { index, value };
    frame_send(CMD_READ_CURRENT, 2, resp);
}
```

**Điểm khác biệt so với các lệnh điều khiển:**
- Lệnh SET (0x01, 0x02) phản hồi bằng chuỗi ASCII `"ACK\r\n"` (debug).
- Lệnh READ (0x03) phản hồi bằng **frame nhị phân** qua `frame_send()` → cho phép PC/ESP32 parse kết quả tự động.
- Nếu `index` không hợp lệ, `measurement_service_read()` trả về `0`, frame vẫn được gửi với `value = 0`.

---

## 10. Luồng dữ liệu đầu cuối

### Ví dụ: Đọc dòng thiết bị 2 (index = 2)

```
PC gửi: AA 03 01 02 AA
          │
          ▼
    uart_read_char()           ← Đọc từng byte từ chân RX (PC1, bit-banging)
          │
          ▼
    frame_parse_byte()         ← State machine ghép frame, kiểm tra checksum
          │ (frame hợp lệ)
          ▼
    app_task() nhận diện CMD_READ_CURRENT (0x03), len=1
          │
          ▼
    measurement_service_read(2)
          │
          ▼
    adc0804_read(2)
      ├─ 74HC4051: A=0, B=1, C=0 → chọn kênh Y2 (cảm biến device 2)
      ├─ CS=LOW, WR=LOW→HIGH     → ADC bắt đầu chuyển đổi
      ├─ Chờ INTR# = LOW         → chuyển đổi hoàn thành (~110µs)
      ├─ PORTD→Input, RD=LOW     → đọc 8-bit từ PORTD
      └─ PORTD→Output            → phục hồi bus cho 8255
          │ trả về ví dụ: 0x7F
          ▼
    frame_send(0x03, 2, {0x02, 0x7F})
      ├─ uart_write_char(0xAA)   ← Header
      ├─ uart_write_char(0x03)   ← CMD
      ├─ uart_write_char(0x02)   ← LEN
      ├─ uart_write_char(0x02)   ← index
      ├─ uart_write_char(0x7F)   ← value
      └─ uart_write_char(0xD4)   ← checksum
          │
          ▼
PC nhận: AA 03 02 02 7F D4
```

---

## 11. Vấn đề chia sẻ bus dữ liệu (PORTD)

Đây là điểm kỹ thuật quan trọng nhất khi tích hợp ADC0804 vào hệ thống đã có 8255.

### Vấn đề

PORTD (PD0–PD7) vừa là **bus dữ liệu của 8255**, vừa là **bus kết quả của ADC0804**. Nếu cả hai chip cùng drive bus → **xung đột điện (bus contention)** → tín hiệu không xác định, có thể hỏng chip.

### Giải pháp trong adc0804_read()

```c
// ─── TRƯỚC khi đọc ADC ─────────────────────────────────────
// 8255 đang ở trạng thái CS=HIGH → output driver của 8255 ở high-impedance
// Không có xung đột bus.

PPI_DATA_DDR  = 0x00;   // ATmega PORTD → Input (tri-state AVR output driver)
PPI_DATA_PORT = 0x00;   // Tắt pull-up nội bộ

ADC_PORT &= ~(1 << ADC_RD_PIN);  // RD=LOW → ADC0804 drive bus
_delay_us(1);
uint8_t data = PPI_DATA_PIN;      // Đọc 8-bit kết quả
ADC_PORT |=  (1 << ADC_RD_PIN);   // RD=HIGH → ADC0804 thả bus (high-impedance)
ADC_PORT |=  (1 << ADC_CS_PIN);   // CS=HIGH → ADC0804 deselect

// ─── SAU khi đọc ADC ───────────────────────────────────────
PPI_DATA_DDR = 0xFF;   // ATmega PORTD → Output (khôi phục cho 8255)
```

### Bảng trạng thái bus an toàn

| Thao tác | CS# 8255 | CS# ADC | DDR PORTD | Trạng thái bus |
|---|:---:|:---:|:---:|---|
| Ghi 8255 | LOW | HIGH | Output | ATmega → 8255 nhận |
| Đọc 8255 | LOW | HIGH | Input | 8255 drive bus |
| Đọc ADC | HIGH | LOW | Input | ADC0804 drive bus |
| Idle | HIGH | HIGH | Output | ATmega giữ bus |

**Quy tắc bất biến:** `CS# 8255` và `CS# ADC` **không bao giờ** cùng LOW tại một thời điểm. Driver đảm bảo điều này vì giao tiếp là tuần tự hoàn toàn — `ppi8255_write/read()` luôn kéo CS# 8255 về HIGH trước khi thoát, và `adc0804_read()` chỉ kéo CS# ADC xuống sau đó.

---

## Phụ lục: Tóm tắt hàm liên quan đến ADC

| Hàm | File | Mô tả |
|---|---|---|
| `adc0804_init()` | drivers/adc0804.c | Cấu hình chân PORTA cho ADC và 74HC4051 |
| `adc0804_read(ch)` | drivers/adc0804.c | Chọn kênh, kích hoạt chuyển đổi, đọc kết quả |
| `measurement_service_init()` | services/measurement_service.c | Bọc `adc0804_init()` |
| `measurement_service_read(idx)` | services/measurement_service.c | Kiểm tra biên + gọi `adc0804_read()` |
| `frame_send(cmd, len, data)` | protocol/frame.c | Đóng gói và gửi frame phản hồi qua UART |
