# Tài liệu Chi tiết: Đo Dòng Điện AC — Từng Cơ Chế, Từng Hàm

> Tài liệu này mô tả **toàn bộ luồng dữ liệu** từ khi dòng điện AC chạy qua tải,
> qua cảm biến ACS712, qua mux 74HC4051, qua ADC0804, qua bus 8255, đến ATmega xử lý,
> rồi truyền về PC qua UART. Bao gồm cơ chế CPU, RAM, ROM và từng hàm trong firmware.

---

## MỤC LỤC

1. [Tổng quan pipeline](#1-tổng-quan-pipeline)
2. [Phần cứng: ACS712 - 74HC4051 - ADC0804 - 8255](#2-phần-cứng)
3. [Kiến trúc bộ nhớ: ROM, RAM, I/O Map](#3-kiến-trúc-bộ-nhớ)
4. [Tầng CPU Bus](#4-tầng-cpu-bus)
5. [Driver 8255 — ppi8255.c](#5-driver-8255)
6. [Driver ADC0804 — adc0804.c](#6-driver-adc0804)
7. [Tầng Service đo dòng — measurement_service.c](#7-measurement-service)
8. [Tầng App — app_task()](#8-tầng-app)
9. [Truyền dữ liệu về PC — frame_send() và UART](#9-truyền-uart)
10. [Phía PC: nhận và tính toán — tool.ps1](#10-phía-pc)
11. [Tóm tắt toàn bộ pipeline](#11-tóm-tắt)
12. [Bảng thời gian thực thi](#12-bảng-thời-gian)
13. [Lưu ý và giới hạn](#13-lưu-ý)

---

## 1. Tổng quan pipeline

```text
[Dòng điện AC qua tải]
        |
        v
[ACS712] — cảm biến dòng, chuyển dòng -> điện áp
        |  VIOUT = VCC/2 + sensitivity x I_AC
        v
[74HC4051] — mux analog 8-kênh, chọn kênh bằng Port C của 8255
        |  X (output chung) -> VIN+ của ADC0804
        v
[ADC0804] — ADC 8-bit SAR, convert analog -> digital
        |  D0..D7 -> Port B của 8255
        v
[8255 PPI] — chip I/O mở rộng, giao tiếp qua Data Bus + Control Bus
        |  Data Bus: PORTD (PD0..PD7) của ATmega
        |  Control : PORTB (WR#, RD#, A0, A1, CS#)
        v
[ATmega] — đọc ADC qua bus 8255, tính I_peak, gói frame UART
        |
        v
[PC / tool.ps1] — nhận frame, tính I_rms = I_peak/sqrt(2), P = V_rms x I_rms
```

---

## 2. Phần cứng

### 2.1 ACS712 — Cảm biến dòng điện

ACS712 là cảm biến dòng hiệu ứng Hall. Dòng điện AC chạy qua tải tạo từ trường,
ACS712 chuyển thành điện áp tuyến tính:

```
VIOUT = VCC/2 + Sensitivity x I

Với VCC = 5V, Sensitivity = 185 mV/A (ACS712-05B):
  I = 0A   -> VIOUT = 2.500 V  -> ADC = 128
  I = +9.19A -> VIOUT = 4.20 V -> ADC = 215  (đỉnh dương, I_rms = 6.5A)
  I = -9.19A -> VIOUT = 0.80 V -> ADC = 41   (đỉnh âm)
```

### 2.2 74HC4051 — Analog Multiplexer 8-kênh

Ba bit **A (PC4), B (PC5), C (PC6)** từ 8255 Port C chọn kênh:

```
C  B  A  |  Kênh      C  B  A  |  Kênh
---------+---------   ---------+---------
0  0  0  |  X0        1  0  0  |  X4
0  0  1  |  X1        1  0  1  |  X5
0  1  0  |  X2        1  1  0  |  X6
0  1  1  |  X3        1  1  1  |  X7
```

Kết nối:
- Đầu ra **X** -> `ADC0804 VIN+` (pin 6)
- **INH** -> GND (luôn enable)
- Kênh không dùng (X1..X7) -> VREF/2 (2.5V) qua điện trở 10k để tránh floating

### 2.3 ADC0804 — Bộ chuyển đổi 8-bit SAR

SAR (Successive Approximation Register): so sánh 8 lần từ MSB xuống LSB.
Clock nội: R=10kΩ, C=150pF -> f ≈ 606 kHz -> 1 conversion ≈ 13µs.
Delay firmware dùng 120µs để có margin an toàn.

Chân điều khiển từ 8255 Port C:

```c
#define PPI_ADC_CS_BIT  0   // PC0 -> ADC CS#  (active LOW)
#define PPI_ADC_WR_BIT  1   // PC1 -> ADC WR#  (start conv, rising edge)
#define PPI_ADC_RD_BIT  2   // PC2 -> ADC RD#  (output enable, active LOW)
```

### 2.4 8255 PPI — Kết nối với ATmega

```
ATmega PORTD (PD0..PD7) <-> 8255 Data Bus (8-bit, bidirectional)
ATmega PORTB:
  PB0 (WR#) -> 8255 WR#
  PB1 (RD#) -> 8255 RD#
  PB2 (A0)  -> 8255 A0
  PB3 (A1)  -> 8255 A1
  PB4 (CS#) -> 8255 CS#
```

Bảng chọn thanh ghi 8255:

```
A1  A0  |  Thanh ghi       |  Địa chỉ io_map
--------+-----------------+------------------
 0   0  |  Port A (Output) |  0x8000  (điều khiển thiết bị)
 0   1  |  Port B (Input)  |  0x8001  (nhận data ADC)
 1   0  |  Port C (Output) |  0x8002  (điều khiển ADC/mux)
 1   1  |  Control Reg     |  0x8003  (chỉ ghi)
```

---

## 3. Kiến trúc bộ nhớ

Hệ thống dùng không gian địa chỉ 16-bit (64KB):

```
Địa chỉ (hex)     Vùng                  Kích thước
--------------------------------------------------
0x0000 - 0x00FF   ROM (Flash PROGMEM)   256 bytes  [chỉ đọc]
0x4000 - 0x40FF   RAM (SRAM)            256 bytes  [đọc/ghi]
0x8000            8255 Port A           1 byte
0x8001            8255 Port B           1 byte     [đọc data ADC]
0x8002            8255 Port C           1 byte
0x8003            8255 Control Reg      1 byte     [chỉ ghi]
```

### 3.1 ROM — Flash PROGMEM

```c
static const uint8_t rom[ROM_SIZE] PROGMEM = {0};
```

- `PROGMEM`: GCC đặt mảng vào **Flash memory** (không dùng SRAM)
- Đọc bắt buộc dùng `pgm_read_byte()` vì Flash không map trực tiếp vào SRAM space
- `pgm_read_byte()` phát lệnh `LPM` (Load Program Memory) của AVR,
  dùng thanh ghi Z (ZH:ZL) làm địa chỉ Flash

```c
uint8_t memory_read(uint16_t addr) {
    if (addr >= MEM_ROM_START && addr <= MEM_ROM_END)
        return pgm_read_byte(&rom[addr - MEM_ROM_START]);
```

### 3.2 RAM — SRAM

```c
static uint8_t ram[RAM_SIZE];

void memory_init() {
    for (uint16_t i = 0; i < RAM_SIZE; i++)
        ram[i] = 0;
}
```

- `ram[]` nằm trong **SRAM của ATmega** (segment `.bss`)
- `memory_init()` gọi trong `app_init()` để zero-fill
- Truy cập: `ram[addr - 0x4000]`

### 3.3 I/O Map — Address Decoder

```c
// io_map.c
uint8_t io_map_read(uint16_t addr) {
    switch (addr) {
        case 0x8000: return ppi8255_read_portA();
        case 0x8001: return ppi8255_read_portB();  // <- đọc data ADC
        case 0x8002: return ppi8255_read_portC();
        default:     return 0xFF;
    }
}

void io_map_write(uint16_t addr, uint8_t data) {
    switch (addr) {
        case 0x8000: ppi8255_write_portA(data); break; // bật/tắt thiết bị
        case 0x8002: ppi8255_write_portC(data); break; // điều khiển ADC/mux
        case 0x8003: ppi8255_write(PPI_CONTROL, data); break;
    }
}
```

---

## 4. Tầng CPU Bus

```c
uint8_t cpu_bus_read(uint16_t addr)             { return memory_read(addr); }
void    cpu_bus_write(uint16_t addr, uint8_t d) { memory_write(addr, d); }
```

Lớp trừu tượng hóa bus: bất kỳ module nào cũng dùng địa chỉ 16-bit mà
không cần biết bên dưới là Flash, SRAM hay 8255.

Luồng đọc Port B (data ADC):

```
cpu_bus_read(0x8001)
  -> memory_read(0x8001)
       -> io_map_read(0x8001)          [0x8001 trong vùng 0x8000..0x8003]
            -> ppi8255_read_portB()
                 -> ppi8255_read(PPI_PORT_B)
                      -> toggle CS#, RD# trên PORTB
                      -> đọc PIND
                      -> return data
```

---

## 5. Driver 8255

### 5.1 ppi8255_init() — Khởi tạo

```c
void ppi8255_init(void) {
    // Set HIGH trước DDR để tránh glitch (CS#/WR# không kích hoạt sai)
    PPI_CTRL_PORT |= (1<<PPI_WR_PIN) | (1<<PPI_RD_PIN) | (1<<PPI_CS_PIN);

    // Set DDR: các chân PORTB trở thành Output
    PPI_CTRL_DDR |= (1<<PPI_WR_PIN) | (1<<PPI_RD_PIN) |
                    (1<<PPI_A0_PIN) | (1<<PPI_A1_PIN) | (1<<PPI_CS_PIN);

    PPI_DATA_DDR = 0xFF;  // Data bus PORTD: Output mặc định
    _delay_ms(1);         // Chờ 8255 ổn định

    // Control Word 0x82 = 1000 0010b:
    //   D7=1: Mode Set Flag (bắt buộc)
    //   D4=0: Port A = Output (thiết bị)
    //   D3=0: Port C upper = Output (mux/ADC control)
    //   D1=1: Port B = Input (data ADC)
    //   D0=0: Port C lower = Output (CS#/WR#/RD# ADC)
    ppi8255_write(PPI_CONTROL, 0x82);
}
```

### 5.2 set_addr() — Đặt địa chỉ A0, A1

```c
static void set_addr(uint8_t addr) {
    if (addr & 0x01) PPI_CTRL_PORT |=  (1 << PPI_A0_PIN);  // A0 = bit0 của reg
    else             PPI_CTRL_PORT &= ~(1 << PPI_A0_PIN);
    if (addr & 0x02) PPI_CTRL_PORT |=  (1 << PPI_A1_PIN);  // A1 = bit1 của reg
    else             PPI_CTRL_PORT &= ~(1 << PPI_A1_PIN);
}
```

Ví dụ: `set_addr(PPI_PORT_B)` tức `addr=1` -> A0=1, A1=0 -> 8255 chọn Port B.

### 5.3 ppi8255_write() — Ghi dữ liệu

```c
void ppi8255_write(ppi_reg_t reg, uint8_t data) {
    PPI_DATA_DDR  = 0xFF;           // PORTD -> Output
    PPI_DATA_PORT = data;           // Đặt data lên bus

    set_addr(reg);                  // Chọn thanh ghi (A0, A1)
    _delay_us(1);                   // Chờ address ổn định

    PPI_CTRL_PORT |=  (1<<PPI_RD_PIN);    // RD# = HIGH (không đọc)
    PPI_CTRL_PORT &= ~(1<<PPI_CS_PIN);    // CS# = LOW  -> kích hoạt 8255
    _delay_us(1);

    PPI_CTRL_PORT &= ~(1<<PPI_WR_PIN);    // WR# = LOW  -> bắt đầu ghi
    _delay_us(2);                          // Giữ >= 300ns (datasheet), dùng 2µs
    PPI_CTRL_PORT |=  (1<<PPI_WR_PIN);    // WR# = HIGH -> 8255 CHỐT DATA (rising edge)
    _delay_us(1);

    PPI_CTRL_PORT |=  (1<<PPI_CS_PIN);    // CS# = HIGH -> kết thúc
}
```

Timing: `CS#` bao quanh, `WR#` pulse bên trong. 8255 chốt data tại cạnh lên WR#.

```
DATA  ──[  VALID  ]──────────────────────────
CS#   ‾|___________________________|‾‾‾‾‾‾‾‾
WR#   ‾‾‾‾‾‾‾‾‾|_________|‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
                          ^ 8255 chốt tại đây
```

### 5.4 ppi8255_read() — Đọc dữ liệu

```c
uint8_t ppi8255_read(ppi_reg_t reg) {
    uint8_t data;

    PPI_DATA_DDR  = 0x00;   // PORTD -> Input (nhường bus cho 8255)
    PPI_DATA_PORT = 0x00;   // Tắt pull-up (tri-state)

    set_addr(reg);
    _delay_us(1);

    PPI_CTRL_PORT |=  (1<<PPI_WR_PIN);    // WR# = HIGH (không ghi)
    PPI_CTRL_PORT &= ~(1<<PPI_CS_PIN);    // CS# = LOW  -> kích hoạt 8255
    _delay_us(1);

    PPI_CTRL_PORT &= ~(1<<PPI_RD_PIN);    // RD# = LOW  -> 8255 drive data bus
    _delay_us(20);                         // Chờ 8255 đẩy data ra bus (20µs cho Proteus)

    data = PPI_DATA_PIN;   // Đọc PIND (physical pin level)

    PPI_CTRL_PORT |=  (1<<PPI_RD_PIN);    // RD# = HIGH
    _delay_us(2);
    PPI_CTRL_PORT |=  (1<<PPI_CS_PIN);    // CS# = HIGH
    return data;
}
```

**Cơ chế quan trọng:** Khi `DDR=0x00`, PORTD ở chế độ Input, ATmega không drive bus.
Khi `RD#=LOW` và `CS#=LOW`, 8255 tự drive Port B (dữ liệu ADC) lên bus.
`PIND` đọc mức điện áp vật lý trên chân — chính xác là byte từ ADC0804.

```
PORTD  ──────────[  8255 drive: ADC data  ]──
CS#    ‾‾|__________________________|‾‾‾‾‾‾
WR#    ‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾‾
RD#    ‾‾‾‾‾‾‾‾|___________________|‾‾‾‾‾‾
                         ^ ATmega đọc PIND ở đây
```

---

## 6. Driver ADC0804

### 6.1 adc0804_init() — Khởi tạo

```c
void adc0804_init(void) {
    portc_shadow = (1<<PPI_ADC_CS_BIT) | (1<<PPI_ADC_WR_BIT) | (1<<PPI_ADC_RD_BIT);
    ppi8255_write_portC(portc_shadow);
    // CS#=1, WR#=1, RD#=1: tất cả inactive
}
```

`portc_shadow` là **shadow register trong SRAM**: lưu bản sao trạng thái Port C.
Mỗi lần thay 1 bit: đọc shadow (nhanh), cập nhật, ghi ra 8255 (chậm).
Cần thiết vì đọc Output port 8255 không tin cậy bằng đọc biến SRAM.

### 6.2 adc0804_read(channel) — Đọc 1 mẫu ADC (~201µs)

#### Bước 1 — Chọn kênh mux (1 lần ghi, không glitch)

```c
uint8_t mux_mask = (1<<4)|(1<<5)|(1<<6);   // bits PC4, PC5, PC6
portc_shadow = (portc_shadow & ~mux_mask)
             | (uint8_t)((channel & 0x07) << 4);
ppi8255_write_portC(portc_shadow);  // 1 lần ghi duy nhất
_delay_us(10);   // Chờ mux chuyển kênh + tụ VIN+ ổn định
```

Nếu set A, B, C riêng lẻ (3 lần ghi riêng): mux trải qua trạng thái sai trung gian.
Ví dụ chọn kênh 3 (011): set A=1 -> mux chọn kênh 1 (glitch!), rồi set B=1 -> kênh 3.
Ghi 1 lần cả 3 bit loại bỏ hoàn toàn glitch này.

#### Bước 2 — Kích hoạt chuyển đổi

```c
set_portc_bit(PPI_ADC_CS_BIT, 0);   // CS# = LOW  -> chọn ADC0804
set_portc_bit(PPI_ADC_WR_BIT, 0);   // WR# = LOW  (hold)
_delay_us(1);
set_portc_bit(PPI_ADC_WR_BIT, 1);   // WR# = HIGH -> rising edge: START CONVERSION
```

ADC0804 nhận **cạnh lên của WR#** làm tín hiệu Start of Conversion.
SAR bắt đầu so sánh 8 lần từ MSB (D7) xuống LSB (D0), mỗi lần 1 clock:

```
Clock cycle:  1   2   3   4   5   6   7   8
Bit so sánh: D7  D6  D5  D4  D3  D2  D1  D0
```

#### Bước 3 — Chờ conversion hoàn thành

```c
_delay_us(120);
```

f_clk ≈ 606 kHz -> 8 cycles -> 13µs. Dùng 120µs để bao gồm overhead và margin.

#### Bước 4 — Đọc kết quả

```c
set_portc_bit(PPI_ADC_RD_BIT, 0);    // ADC RD# = LOW -> ADC drive D0..D7
_delay_us(1);
uint8_t data = ppi8255_read_portB(); // 8255 đọc Port B -> PIND của ATmega
set_portc_bit(PPI_ADC_RD_BIT, 1);
set_portc_bit(PPI_ADC_CS_BIT, 1);
return data;
```

Đường dữ liệu:
```
ADC0804 D0..D7
  -> 8255 Port B (configured Input)
      -> khi CS#(8255)=LOW, RD#(8255)=LOW:
           8255 kết nối Port B ra data bus
               -> PORTD ATmega (DDR=0x00)
                   -> ATmega đọc PIND
```

---

## 7. Measurement Service

### 7.1 Vấn đề: tại sao không đọc 1 mẫu?

Dòng AC là sóng sin: `i(t) = I_peak x sin(2*pi*50*t)`.
1 mẫu tức thời cho kết quả tùy thuộc phase (đỉnh, đáy, zero) -> không đại diện.

Với I_rms = 6.5A (I_peak = 9.19A), ADC dao động:
```
ADC raw: 128 -> 215 -> 128 -> 41 -> 128 -> ... (lặp 50 lần/giây)
```

### 7.2 Thuật toán Peak Detection

```
I_rms = I_peak / sqrt(2)  (đúng với sóng sin thuần)
```

Ưu điểm so với tính RMS trực tiếp:

| Tiêu chí | RMS trực tiếp | Peak detection |
|---|---|---|
| Phụ thuộc sampling phase | Có (±10%) | Không |
| Cần sqrt() trên MCU | Có | Không |
| Ổn định | Thấp nếu window != N cycles | Cao |

### 7.3 Code chi tiết

```c
#define PEAK_SAMPLE_COUNT 100  // 100 x 201µs = 20.1ms >= 1 chu kỳ 50Hz (20ms)
#define ADC_ZERO_OFFSET   128  // VCC/2 = 2.5V -> ADC = 128

uint8_t measurement_service_read(uint8_t index) {
    if (index >= DEVICE_COUNT) return 0;

    uint8_t peak = 0;
    for (uint8_t i = 0; i < PEAK_SAMPLE_COUNT; i++) {
        uint8_t raw = adc0804_read(index);
        int16_t diff = (int16_t)raw - ADC_ZERO_OFFSET;
        uint8_t abs_diff = (diff < 0) ? (uint8_t)(-diff) : (uint8_t)diff;
        if (abs_diff > peak) peak = abs_diff;
    }
    return peak;   // I_peak_lsb (0..128)
}
```

Phân tích từng mẫu với 6.5A RMS:

```
raw=215: diff=+87, abs=87  -> peak=87
raw=128: diff=0,   abs=0   -> peak=87 (không đổi)
raw=41:  diff=-87, abs=87  -> peak=87 (không đổi)
Kết quả cuối: peak=87
```

Sai số bỏ lỡ đỉnh: Δφ = 2π x 50 x (101µs) = 1.82° -> sai số = 1-cos(1.82°) = 0.05%.

---

## 8. Tầng App

```c
void app_task() {
    // ... poll UART ~485ms ...

    uint8_t payload[9];
    uint8_t device_state = device_service_get_state();  // đọc shadow trong SRAM

    for (uint8_t i = 0; i < DEVICE_COUNT; i++) {
        // Gate: chỉ đo kênh đang BẬT
        // (kênh không có cảm biến vật lý sẽ cho kết quả sai)
        payload[i] = (device_state & (1 << i))
                     ? measurement_service_read(i)  // ~20.1ms/kênh
                     : 0;
    }
    payload[8] = device_state;   // bitmask 8 thiết bị

    // Frame: [AA][04][09][peak0..7][state][CS] = 13 bytes
    frame_send(CMD_SENSOR_DATA, 9, payload);
}
```

---

## 9. Truyền UART

### 9.1 frame_send() — Đóng gói

```c
void frame_send(uint8_t cmd, uint8_t len, uint8_t* data) {
    uint8_t cs = FRAME_HEADER ^ cmd ^ len;
    for (uint8_t i = 0; i < len; i++) cs ^= data[i];

    uart_write_char(0xAA);   // Header
    uart_write_char(cmd);    // 0x04 = CMD_SENSOR_DATA
    uart_write_char(len);    // 9
    for (uint8_t i=0; i<len; i++) uart_write_char(data[i]);
    uart_write_char(cs);
}
```

### 9.2 uart_write_char() — Bit-banging 9600 baud

```c
void uart_write_char(char c) {
    UART_PORT &= ~(1 << UART_TX_PIN);  // Start bit: TX = LOW
    _delay_us(104);                     // 1 bit = 1/9600 = 104µs

    for (uint8_t i = 0; i < 8; i++) {
        if (c & 0x01) UART_PORT |=  (1 << UART_TX_PIN);
        else          UART_PORT &= ~(1 << UART_TX_PIN);
        _delay_us(104);
        c >>= 1;   // LSB trước
    }

    UART_PORT |= (1 << UART_TX_PIN);   // Stop bit: TX = HIGH
    _delay_us(104);
}
```

1 byte = 10 bits x 104µs = 1.04ms. Frame 13 bytes = 13.5ms.

---

## 10. Phía PC

```powershell
# Frame nhận: [AA][04][09][p0..p7][state][CS] = 13 bytes
$peakLsb = $frameBytes[3]        # Ví dụ: 87

$sensitivityVA = 0.185           # ACS712-05B: 185mV/A
$sqrt2 = [math]::Sqrt(2)

$I_peak_A = $peakLsb * 5.0 / 256.0 / $sensitivityVA
#         = 87 * 5 / 256 / 0.185 = 9.19 A

$I_rms_A  = $I_peak_A / $sqrt2
#         = 9.19 / 1.414 = 6.50 A

$P_W      = $I_rms_A * 220.0
#         = 6.50 * 220 = 1430 W
```

Công thức tổng quát:

```
V_sensor  = peak_lsb * (VCC / 256)          [Volt đỉnh tại cảm biến]
I_peak_A  = V_sensor / Sensitivity           [Ampere đỉnh]
I_rms_A   = I_peak_A / sqrt(2)              [Ampere RMS, sin thuần]
P_W       = V_rms * I_rms_A                 [Watt, tải thuần trở]
```

---

## 11. Tóm tắt

```
[Dòng AC 6.5A RMS qua tải]
  -> ACS712: VIOUT dao động 0.80V..4.20V xung quanh 2.5V
  -> 74HC4051: chọn kênh qua PC4/PC5/PC6 (1 lần ghi, không glitch)
  -> ADC0804: trigger bởi WR# pulse (PC1), convert 8-bit SAR ~13µs
  -> D0..D7 -> 8255 Port B (Input)
  -> 8255 RD# (PB1) + CS# (PB4) -> drive data bus PORTD
  -> ATmega PIND <- raw byte (41..215)
  -> 100 lần lặp: peak = max(|raw - 128|) = 87 LSB  [trong SRAM]
  -> payload[0] = 87, payload[8] = device_state      [trong SRAM]
  -> frame_send: [AA 04 09 57 00..00 01 CS] = 13 bytes
  -> uart bit-bang 9600 baud: 13.5ms phát
  -> PC nhận: 87 -> 9.19A -> 6.50A / 1430W
```

---

## 12. Bảng thời gian

| Giai đoạn | Thời gian |
|---|---|
| 1 lần adc0804_read() | ~201µs |
| 100 mẫu peak 1 kênh | ~20.1ms |
| 8 kênh (tất cả ON) | ~161ms |
| Gửi frame 13 bytes UART | ~13.5ms |
| Tổng 1 vòng app_task() | ~660ms |

---

## 13. Lưu ý

1. **Peak detection chỉ đúng với sóng sin thuần.**
   Tải phi tuyến (động cơ, biến tần) -> dạng sóng méo -> I_rms != I_peak/sqrt(2).

2. **Công suất biểu kiến, không phải công suất thực.**
   P = V_rms x I_rms bỏ qua hệ số công suất cos(phi).
   Tải cảm kháng: cos(phi) < 1 -> P_thực < P_tính được.

3. **Kênh không có cảm biến phải nối về VREF/2 (2.5V).**
   Input hở của 74HC4051 bị crosstalk trong Proteus.
   Giải pháp: X1..X7 -> R 10k -> midpoint(VCC/2) <- R 10k -> GND.

4. **Gate đo theo device_state là workaround cho prototype 1 cảm biến.**
   Khi lắp đủ 8 ACS712 thực tế: xóa điều kiện `(device_state & (1<<i))`
   trong app_task() để phát hiện ngắn mạch khi thiết bị TẮT.
