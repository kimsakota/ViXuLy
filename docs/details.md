# Tài liệu Chi tiết Dự án ViXuLy

> **Mục tiêu dự án:** Điều khiển thiết bị ngoại vi (đèn, relay...) thông qua giao tiếp UART từ máy tính, sử dụng vi điều khiển AVR ATmega kết nối với chip PPI Intel 8255.

---

## MỤC LỤC

1. [Tổng quan kiến trúc hệ thống](#1-tổng-quan-kiến-trúc-hệ-thống)
2. [Sơ đồ phân tầng phần mềm](#2-sơ-đồ-phân-tầng-phần-mềm)
3. [Luồng dữ liệu từ đầu đến cuối](#3-luồng-dữ-liệu-từ-đầu-đến-cuối)
4. [config/ — Cấu hình hệ thống](#4-config--cấu-hình-hệ-thống)
5. [bsp/ — Board Support Package](#5-bsp--board-support-package)
6. [core/ — Lõi hệ thống (Bus & Memory)](#6-core--lõi-hệ-thống-bus--memory)
7. [drivers/ — Driver phần cứng](#7-drivers--driver-phần-cứng)
8. [protocol/ — Giao thức truyền thông](#8-protocol--giao-thức-truyền-thông)
9. [services/ — Tầng dịch vụ](#9-services--tầng-dịch-vụ)
10. [app/ — Ứng dụng chính](#10-app--ứng-dụng-chính)
11. [main.c — Điểm khởi động](#11-mainc--điểm-khởi-động)

---

## 1. Tổng quan kiến trúc hệ thống

### Phần cứng liên quan

```
[PC / Terminal]
      │  UART Serial (9600 baud)
      │  TX ──► PC1 (RX của AVR)
      │  RX ◄── PC0 (TX của AVR)
      ▼
[ATmega (AVR MCU)]
      │  Data Bus (8-bit): PORTD (PD0–PD7)
      │  Control Bus:      PORTB
      │    PB0 = WR#  (Write strobe)
      │    PB1 = RD#  (Read strobe)
      │    PB2 = A0   (Address bit 0)
      │    PB3 = A1   (Address bit 1)
      │    PB4 = CS#  (Chip Select)
      ▼
[Intel 8255 PPI]
      │  Port A (PA0–PA7) ──► 8 thiết bị (đèn/relay)
      │  Port B (PB0–PB7) ──► (dự phòng)
      │  Port C (PC0–PC7) ──► (dự phòng)
      │  Control Register  ──► cấu hình chiều I/O
```

### Bản đồ địa chỉ (Memory Map)

Toàn bộ hệ thống dùng không gian địa chỉ 16-bit (64KB), nhưng dữ liệu truyền đi luôn là 8-bit:

```
Địa chỉ (hex)     Vùng             Kích thước   Quyền truy cập
─────────────────────────────────────────────────────────────
0x0000 – 0x3FFF   ROM              16 KB        Chỉ đọc (Read-only)
0x4000 – 0x7FFF   RAM              16 KB        Đọc + Ghi
0x8000            PPI Port A       1 byte       Đọc + Ghi
0x8001            PPI Port B       1 byte       Đọc + Ghi
0x8002            PPI Port C       1 byte       Đọc + Ghi
0x8003            PPI Control Reg  1 byte       Chỉ Ghi
```

---

## 2. Sơ đồ phân tầng phần mềm

```
┌──────────────────────────────────────────────────────────┐
│                        main.c                            │
│            app_init()  →  while(1) { app_task() }        │
└──────────────────────────┬───────────────────────────────┘
                           │ #include "app/app.h"
┌──────────────────────────▼───────────────────────────────┐
│                   app/ (Application Layer)                │
│  app_init(): khởi tạo toàn bộ hệ thống                  │
│  app_task(): vòng lặp chính, đọc UART → parse → điều khiển │
└────┬──────────────┬──────────────┬───────────────────────┘
     │              │              │
     ▼              ▼              ▼
┌─────────┐  ┌──────────┐  ┌───────────────────┐
│protocol/│  │services/ │  │    core/           │
│ frame.c │  │device_   │  │  memory.c          │
│ command │  │service.c │  │  cpu_bus.c         │
└────┬────┘  └────┬─────┘  │  io_map.c          │
     │            │         └─────────┬──────────┘
     │            │                   │
     ▼            ▼                   ▼
┌──────────────────────────────────────────────┐
│               drivers/ (Hardware Drivers)     │
│       uart.c           ppi8255.c             │
└──────────────────────────────────────────────┘
                           │
                           ▼
┌──────────────────────────────────────────────┐
│               bsp/ (Board Support Package)    │
│               board.h (pin mapping)           │
└──────────────────────────────────────────────┘
                           │
                           ▼
┌──────────────────────────────────────────────┐
│           config/system_config.h              │
│         (hằng số toàn hệ thống)              │
└──────────────────────────────────────────────┘
```

---

## 3. Luồng dữ liệu từ đầu đến cuối

### Luồng điều khiển thiết bị (Nominal Flow)

```
PC gửi frame UART
  │
  │  [AA] [CMD] [LEN] [DATA...] [CHECKSUM]
  ▼
uart_read_char()          ← Đọc từng byte từ đường RX (bit-banging)
  │
  ▼
frame_parse_byte()        ← Ghép byte vào frame, kiểm tra checksum
  │ (khi frame hợp lệ)
  ▼
app_task() xử lý lệnh:
  ├── CMD_SET_ALL    → device_service_set_all(data[0])
  └── CMD_SET_SINGLE → device_service_turn_on/off(index)
                │
                ▼
        ppi8255_write_portA(value)
                │
                ▼
        [Vật lý] Ghi dữ liệu 8-bit ra PORTD
        + Toggle CS#, WR# → 8255 chốt dữ liệu vào Port A
                │
                ▼
        8 thiết bị (bit 0–7 của Port A) = ON/OFF
                │
  uart_write_string("ACK\r\n") hoặc "NACK\r\n"
```

---

## 4. config/ — Cấu hình hệ thống

**File:** `config/system_config.h`

File này là **trung tâm cấu hình** cho toàn bộ dự án. Mọi hằng số quan trọng đều khai báo ở đây để dễ thay đổi mà không cần sửa từng file.

```c
#define F_CPU           8000000UL   // Tần số thạch anh AVR: 8 MHz
                                    // Ảnh hưởng trực tiếp đến _delay_us()

#define UART_USE_SOFT   1           // Dùng UART phần mềm (bit-banging, không dùng USART cứng)
#define UART_BAUDRATE   9600        // Tốc độ truyền 9600 baud → mỗi bit = 1/9600 ≈ 104.17 µs

#define FRAME_HEADER    0xAA        // Byte đầu tiên của mọi frame hợp lệ
#define FRAME_MAX_PAYLOAD 8         // Số byte payload tối đa
#define FRAME_MAX_DATA  8           // Số byte data tối đa trong frame

// Bản đồ bộ nhớ
#define ROM_SIZE        0x4000      // 16384 byte = 16 KB
#define RAM_SIZE        0x4000      // 16384 byte = 16 KB

#define MEM_ROM_START   0x0000      // ROM bắt đầu từ 0x0000
#define MEM_ROM_END     0x3FFF      // ROM kết thúc  tại 0x3FFF

#define MEM_RAM_START   0x4000      // RAM bắt đầu từ 0x4000
#define MEM_RAM_END     0x7FFF      // RAM kết thúc  tại 0x7FFF

#define MEM_8255_START  0x8000      // Vùng IO (PPI 8255) bắt đầu 0x8000
#define MEM_8255_END    0x8003      // Vùng IO kết thúc tại 0x8003

#define DEVICE_COUNT    8           // Số thiết bị điều khiển được (8 bit = 8 thiết bị)
```

**Tại sao 8 MHz?**
Giá trị `F_CPU` quyết định độ chính xác của `_delay_us()`. Ở 8 MHz, 1 chu kỳ = 125 ns. Hàm delay của AVR-libC tính số vòng lặp dựa trên `F_CPU`, nên sai giá trị này sẽ làm lệch timing UART và giao tiếp 8255.

---

## 5. bsp/ — Board Support Package

**File:** `bsp/board.h`

BSP là lớp **ánh xạ phần cứng cụ thể** — chỉ cần sửa file này khi thay đổi sơ đồ chân mà không cần sửa driver.

### 5.1 Data Bus (Bus dữ liệu 8-bit)

```c
#define PPI_DATA_DDR    DDRD    // Thanh ghi hướng dữ liệu của PORTD
#define PPI_DATA_PORT   PORTD   // Thanh ghi xuất dữ liệu của PORTD
#define PPI_DATA_PIN    PIND    // Thanh ghi đọc trạng thái chân PORTD
```

PORTD (PD0–PD7) đóng vai trò **data bus 8-bit** giữa AVR và 8255:
- Khi **ghi** → `DDRD = 0xFF` (output), đặt dữ liệu lên `PORTD`
- Khi **đọc** → `DDRD = 0x00` (input), đọc dữ liệu từ `PIND`

### 5.2 Control Bus (Bus điều khiển)

```c
#define PPI_CTRL_DDR    DDRB    // Hướng các chân control
#define PPI_CTRL_PORT   PORTB   // Xuất tín hiệu control

#define PPI_WR_PIN  PB0   // WR# - Write Strobe  (active LOW)
#define PPI_RD_PIN  PB1   // RD# - Read Strobe   (active LOW)
#define PPI_A0_PIN  PB2   // A0  - Address bit 0
#define PPI_A1_PIN  PB3   // A1  - Address bit 1
#define PPI_CS_PIN  PB4   // CS# - Chip Select   (active LOW)
```

| Tín hiệu | Mức tích cực | Chức năng |
|---|---|---|
| CS# | LOW | Kích hoạt chip 8255 |
| WR# | LOW→HIGH (cạnh lên) | Chốt dữ liệu vào 8255 |
| RD# | LOW | Cho phép 8255 xuất dữ liệu |
| A0, A1 | — | Chọn thanh ghi nội bộ 8255 |

### 5.3 UART Pins

```c
#define UART_PORT   PORTC   // Cổng UART
#define UART_DDR    DDRC    // Hướng UART
#define UART_PIN    PINC    // Đọc trạng thái

#define UART_TX_PIN PC0     // TX → PC0 (Output) → nối vào RXD của Virtual Terminal
#define UART_RX_PIN PC1     // RX → PC1 (Input)  ← nối vào TXD của Virtual Terminal
```

**Lưu ý chéo kết nối:** TX của AVR nối vào RXD của terminal, RX của AVR nối vào TXD của terminal.

---

## 6. core/ — Lõi hệ thống (Bus & Memory)

### 6.1 `cpu_bus.h` / `cpu_bus.c` — Giao diện Bus CPU

```c
uint8_t cpu_bus_read(uint16_t addr);
void    cpu_bus_write(uint16_t addr, uint8_t data);
```

**Vai trò:** Đây là **điểm truy cập duy nhất** mà code ứng dụng dùng để đọc/ghi bất kỳ vị trí nào trong hệ thống. Tầng này không biết dữ liệu nằm ở ROM, RAM hay IO — nó chỉ chuyển tiếp yêu cầu xuống `memory.c`.

```c
// Implementation (cpu_bus.c)
uint8_t cpu_bus_read(uint16_t addr) {
    return memory_read(addr);   // ủy quyền hoàn toàn cho memory
}

void cpu_bus_write(uint16_t addr, uint8_t data) {
    memory_write(addr, data);   // ủy quyền hoàn toàn cho memory
}
```

**Tại sao cần tầng này?**
Tách biệt "giao diện CPU nhìn thấy" với "cơ chế bên trong bộ nhớ". Sau này có thể thêm cache, bus arbitration, DMA mà không cần sửa code ứng dụng.

---

### 6.2 `memory.h` / `memory.c` — Bộ nhớ và Memory Map

```c
void    memory_init(void);
uint8_t memory_read(uint16_t addr);
void    memory_write(uint16_t addr, uint8_t data);
```

#### Cấu trúc dữ liệu nội bộ

```c
static const uint8_t rom[ROM_SIZE] = {0};  // ROM: 16KB, chỉ đọc, khởi tạo = 0
static uint8_t       ram[RAM_SIZE];        // RAM: 16KB, đọc ghi tự do
```

Hai mảng này được đặt trong SRAM của AVR. `rom` được khai báo `const` nên compiler sẽ đặt nó trong vùng flash (.rodata).

#### `memory_init()`

```c
void memory_init(void) {
    for (uint16_t i = 0; i < RAM_SIZE; i++)
        ram[i] = 0;
}
```

Xóa toàn bộ RAM về `0`. Thực hiện một lần khi hệ thống khởi động. ROM không cần init vì đã được khởi tạo tĩnh bởi compiler.

#### `memory_read(uint16_t addr)` — Logic phân vùng

```c
uint8_t memory_read(uint16_t addr) {
    if (addr >= MEM_ROM_START && addr <= MEM_ROM_END)
        return rom[addr - MEM_ROM_START];       // Đọc từ ROM

    else if (addr >= MEM_RAM_START && addr <= MEM_RAM_END)
        return ram[addr - MEM_RAM_START];       // Đọc từ RAM

    else if (addr >= MEM_8255_START && addr <= MEM_8255_END)
        return io_map_read(addr);               // Đọc từ thiết bị IO

    return 0xFF;   // Địa chỉ không hợp lệ → bus floating
}
```

**Cơ chế offset:** Vì mảng luôn bắt đầu từ index 0, phải trừ địa chỉ bắt đầu của vùng:
- `addr = 0x4100` → `ram[0x4100 - 0x4000]` = `ram[0x0100]` = `ram[256]` ✓

**Giá trị 0xFF khi lỗi:** Khi bus không được kéo lên (floating), điện áp thường ở mức HIGH → 0xFF là giá trị mặc định hợp lý.

#### `memory_write(uint16_t addr, uint8_t data)` — Bảo vệ ROM

```c
void memory_write(uint16_t addr, uint8_t data) {
    if (addr >= MEM_RAM_START && addr <= MEM_RAM_END)
        ram[addr - MEM_RAM_START] = data;       // Ghi vào RAM

    else if (addr >= MEM_8255_START && addr <= MEM_8255_END)
        io_map_write(addr, data);               // Ghi vào thiết bị IO

    // Ghi vào vùng ROM → bị bỏ qua hoàn toàn (như phần cứng thực)
}
```

ROM không có nhánh ghi → mọi cố gắng ghi vào ROM (0x0000–0x3FFF) đều bị **silently ignore**, đúng với hành vi ROM thực tế.

---

### 6.3 `io_map.h` / `io_map.c` — Bộ giải mã địa chỉ IO

```c
uint8_t io_map_read(uint16_t addr);
void    io_map_write(uint16_t addr, uint8_t data);
```

**Vai trò:** Dịch địa chỉ số sang lệnh cụ thể cho thiết bị ngoại vi. Hiện tại chỉ có PPI 8255 ở vùng 0x8000–0x8003.

#### `io_map_read()`

```c
uint8_t io_map_read(uint16_t addr) {
    switch (addr) {
        case 0x8000: return ppi8255_read_portA();
        case 0x8001: return ppi8255_read_portB();
        case 0x8002: return ppi8255_read_portC();
        default:     return 0xFF;   // 0x8003 (Control) không đọc được
    }
}
```

#### `io_map_write()`

```c
void io_map_write(uint16_t addr, uint8_t data) {
    switch (addr) {
        case 0x8000: ppi8255_write_portA(data); break;
        case 0x8001: ppi8255_write_portB(data); break;
        case 0x8002: ppi8255_write_portC(data); break;
        case 0x8003: ppi8255_write(PPI_CONTROL, data); break;
    }
}
```

**Tại sao 0x8003 chỉ được ghi?**
Thanh ghi Control của Intel 8255 theo datasheet là **write-only**. Đọc từ 0x8003 trả về `0xFF` là hành vi đúng. Nếu sau này thêm thiết bị mới (ví dụ ADC0804), chỉ cần thêm `case 0x8004–0x8007` vào đây.

---

## 7. drivers/ — Driver phần cứng

### 7.1 `ppi8255.h` / `ppi8255.c` — Driver Intel 8255 PPI

#### Lý thuyết chip Intel 8255

Intel 8255 là chip **Programmable Peripheral Interface** có 3 cổng I/O 8-bit (A, B, C) và 1 thanh ghi điều khiển. Giao tiếp với CPU qua:
- **Data bus:** 8 đường dữ liệu hai chiều
- **A0, A1:** 2 bit địa chỉ để chọn thanh ghi
- **CS#, RD#, WR#:** tín hiệu điều khiển

#### Bảng chọn thanh ghi (A1:A0)

| A1 | A0 | Thanh ghi được chọn |
|---|---|---|
| 0 | 0 | Port A (0x8000) |
| 0 | 1 | Port B (0x8001) |
| 1 | 0 | Port C (0x8002) |
| 1 | 1 | Control Register (0x8003) |

#### Control Word (Byte điều khiển)

Byte `0x80` được ghi vào Control Register khi init:

```
Bit 7 = 1        → Mode Set Flag (bắt buộc = 1 khi cấu hình)
Bit 6–5 = 00     → Mode 0 cho Group A (Port A + Port C upper)
Bit 4 = 0        → Port A = Output
Bit 3 = 0        → Port C (upper) = Output
Bit 2 = 0        → Mode 0 cho Group B (Port B + Port C lower)
Bit 1 = 0        → Port B = Output
Bit 0 = 0        → Port C (lower) = Output

→ 0x80 = 1000 0000b: Tất cả Port A, B, C đều là Output, Mode 0
```

#### `ppi8255_init()`

```c
void ppi8255_init(void) {
    // 1. Cấu hình chân control là output
    PPI_CTRL_DDR |= (1<<PPI_WR_PIN)|(1<<PPI_RD_PIN)|
                    (1<<PPI_A0_PIN)|(1<<PPI_A1_PIN)|(1<<PPI_CS_PIN);

    // 2. Cấu hình data bus là output (mặc định)
    PPI_DATA_DDR = 0xFF;

    // 3. Đặt trạng thái mặc định: WR=HIGH, RD=HIGH, CS=HIGH (inactive)
    PPI_CTRL_PORT |= (1<<PPI_WR_PIN);
    PPI_CTRL_PORT |= (1<<PPI_RD_PIN);
    PPI_CTRL_PORT |= (1<<PPI_CS_PIN);

    // 4. Ghi Control Word 0x80: Mode 0, tất cả Output
    ppi8255_write(PPI_CONTROL, 0x80);
}
```

#### `set_addr()` — Hàm nội bộ (static)

```c
static void set_addr(uint8_t addr) {
    // Bit 0 → A0
    if (addr & 0x01)  PPI_CTRL_PORT |=  (1 << PPI_A0_PIN);
    else              PPI_CTRL_PORT &= ~(1 << PPI_A0_PIN);

    // Bit 1 → A1
    if (addr & 0x02)  PPI_CTRL_PORT |=  (1 << PPI_A1_PIN);
    else              PPI_CTRL_PORT &= ~(1 << PPI_A1_PIN);
}
```

Hàm này tách 2 bit thấp của `addr` (0–3) → điều khiển chân A0, A1 chọn đúng thanh ghi của 8255.

#### `ppi8255_write()` — Ghi dữ liệu vào 8255

```c
void ppi8255_write(ppi_reg_t reg, uint8_t data) {
    PPI_DATA_DDR = 0xFF;        // Data bus → Output
    PPI_DATA_PORT = data;       // Đặt dữ liệu lên bus

    set_addr(reg);              // Chọn thanh ghi (A0, A1)

    PPI_CTRL_PORT |=  (1<<PPI_RD_PIN);   // RD# = HIGH (không đọc)
    PPI_CTRL_PORT &= ~(1<<PPI_CS_PIN);   // CS# = LOW  (kích hoạt chip)

    PPI_CTRL_PORT &= ~(1<<PPI_WR_PIN);   // WR# = LOW  (bắt đầu ghi)
    _delay_us(1);                          // Giữ tín hiệu ổn định ≥ 1µs
    PPI_CTRL_PORT |=  (1<<PPI_WR_PIN);   // WR# = HIGH (cạnh lên → 8255 chốt data)

    PPI_CTRL_PORT |=  (1<<PPI_CS_PIN);   // CS# = HIGH (giải phóng chip)
}
```

**Timing diagram ghi:**
```
CS#  ‾‾‾|___________|‾‾‾
WR#  ‾‾‾‾‾‾|___|‾‾‾‾‾‾‾
DATA ----[VALID DATA]----
              ↑
        8255 chốt dữ liệu tại cạnh lên của WR#
```

#### `ppi8255_read()` — Đọc dữ liệu từ 8255

```c
uint8_t ppi8255_read(ppi_reg_t reg) {
    uint8_t data;
    PPI_DATA_DDR  = 0x00;   // Data bus → Input
    PPI_DATA_PORT = 0x00;   // Tắt pull-up (tri-state)

    set_addr(reg);

    PPI_CTRL_PORT |=  (1<<PPI_WR_PIN);   // WR# = HIGH (không ghi)
    PPI_CTRL_PORT &= ~(1<<PPI_CS_PIN);   // CS# = LOW  (kích hoạt chip)

    PPI_CTRL_PORT &= ~(1<<PPI_RD_PIN);   // RD# = LOW  (cho phép 8255 drive bus)
    _delay_us(1);                          // Chờ 8255 xuất dữ liệu ra bus
    data = PPI_DATA_PIN;                   // Đọc dữ liệu từ PIND
    PPI_CTRL_PORT |=  (1<<PPI_RD_PIN);   // RD# = HIGH

    PPI_CTRL_PORT |=  (1<<PPI_CS_PIN);   // CS# = HIGH
    return data;
}
```

**Timing diagram đọc:**
```
CS#  ‾‾‾|___________|‾‾‾
RD#  ‾‾‾‾‾‾|___|‾‾‾‾‾‾‾
DATA --------[VALID]----
                ↑
        AVR đọc PIND ở đây (trong khi RD# = LOW)
```

#### Hàm wrapper (bọc)

```c
void ppi8255_write_portA(uint8_t v) { ppi8255_write(PPI_PORT_A, v); }
void ppi8255_write_portB(uint8_t v) { ppi8255_write(PPI_PORT_B, v); }
void ppi8255_write_portC(uint8_t v) { ppi8255_write(PPI_PORT_C, v); }

uint8_t ppi8255_read_portA(void) { return ppi8255_read(PPI_PORT_A); }
uint8_t ppi8255_read_portB(void) { return ppi8255_read(PPI_PORT_B); }
uint8_t ppi8255_read_portC(void) { return ppi8255_read(PPI_PORT_C); }
```

Các hàm này chỉ là alias ngắn gọn cho tầng IO map gọi xuống mà không cần biết giá trị enum.

---

### 7.2 `uart.h` / `uart.c` — UART phần mềm (Bit-banging)

#### Lý thuyết UART

UART truyền dữ liệu nối tiếp (serial) theo định dạng:

```
IDLE  START  D0  D1  D2  D3  D4  D5  D6  D7  STOP
‾‾‾‾|_____|___|___|___|___|___|___|___|___|‾‾‾‾‾
```

- **IDLE:** TX ở mức HIGH
- **START bit:** kéo xuống LOW trong 1 chu kỳ bit (104µs ở 9600 baud)
- **D0–D7:** 8 bit dữ liệu, LSB trước
- **STOP bit:** kéo lên HIGH trong 1 chu kỳ bit

**Tốc độ baud 9600 → thời gian 1 bit = 1/9600 ≈ 104.17µs**

#### `uart_init()`

```c
void uart_init() {
    UART_DDR |=  (1 << UART_TX_PIN);   // PC0 = Output (TX)
    UART_DDR &= ~(1 << UART_RX_PIN);   // PC1 = Input  (RX)

    UART_PORT |= (1 << UART_TX_PIN);   // TX = HIGH (Idle)
    UART_PORT |= (1 << UART_RX_PIN);   // RX = HIGH (Idle, kéo lên nội bộ)
}
```

#### `uart_write_char(char c)` — Gửi 1 byte

```c
void uart_write_char(char c) {
    // 1. Start bit: kéo TX xuống LOW
    UART_PORT &= ~(1 << UART_TX_PIN);
    _delay_us(104);

    // 2. Gửi 8 bit, LSB trước
    for (uint8_t i = 0; i < 8; i++) {
        if (c & 0x01)   UART_PORT |=  (1 << UART_TX_PIN);  // bit = 1 → HIGH
        else            UART_PORT &= ~(1 << UART_TX_PIN);  // bit = 0 → LOW
        _delay_us(104);
        c >>= 1;   // Dịch phải để lấy bit tiếp theo
    }

    // 3. Stop bit: TX về HIGH
    UART_PORT |= (1 << UART_TX_PIN);
    _delay_us(104);
}
```

**Ví dụ gửi byte `0x41` ('A'):**
```
Binary: 0100 0001
LSB → MSB: 1, 0, 0, 0, 0, 0, 1, 0

TX: ‾‾|_|_|_________________|‾‾‾‾‾‾‾‾|‾‾
      ST D0 D1 D2 D3 D4 D5  D6  D7   SP
```

#### `uart_read_char(char* out)` — Đọc 1 byte (non-blocking)

```c
bool uart_read_char(char* out) {
    // 1. Kiểm tra start bit: RX xuống LOW?
    if (UART_PIN & (1 << UART_RX_PIN))
        return false;   // Chưa có dữ liệu → trả về ngay (non-blocking)

    // 2. Chờ 52µs = nửa chu kỳ start bit → lấy mẫu ở giữa
    _delay_us(52);

    // 3. Xác nhận lại: nếu RX vẫn HIGH → là nhiễu, bỏ qua
    if (UART_PIN & (1 << UART_RX_PIN))
        return false;

    // 4. Nhảy qua hết start bit (thêm 104µs nữa)
    _delay_us(104);

    uint8_t data = 0;

    // 5. Đọc 8 bit, LSB trước
    for (uint8_t i = 0; i < 8; i++) {
        data >>= 1;                                // Dịch phải, chờ bit mới
        if (UART_PIN & (1 << UART_RX_PIN))
            data |= 0x80;                          // Bit = 1 → set bit 7
        _delay_us(104);                            // Chờ sang bit tiếp theo
    }

    *out = data;
    return true;
}
```

**Cơ chế lấy mẫu giữa bit:**

```
RX: ‾‾‾|_____|___|‾‾‾...
        ↑     ↑
     Phát   Lấy mẫu
     hiện   (52µs sau) = giữa start bit
     LOW                → xác nhận đúng start bit

              │ +104µs → giữa bit D0 → đọc D0
              │ +104µs → giữa bit D1 → đọc D1
              │ ...
```

Kỹ thuật lấy mẫu ở giữa bit đảm bảo tránh đọc nhầm ở vùng chuyển tiếp (edge).

#### `uart_write_hex_byte(uint8_t byte)` — In byte dạng HEX

```c
void uart_write_hex_byte(uint8_t byte) {
    const char hex_chars[] = "0123456789ABCDEF";
    uart_write_char(hex_chars[(byte >> 4) & 0x0F]);  // Nibble cao
    uart_write_char(hex_chars[byte & 0x0F]);          // Nibble thấp
}
```

Ví dụ: `byte = 0xB7` → gửi `'B'` rồi `'7'` → terminal hiện `B7`.

---

## 8. protocol/ — Giao thức truyền thông

### 8.1 `command.h` — Định nghĩa lệnh

```c
#define CMD_SET_ALL     0x01   // Đặt trạng thái toàn bộ 8 thiết bị cùng lúc
#define CMD_SET_SINGLE  0x02   // Điều khiển 1 thiết bị đơn lẻ

#define CMD_ACK         0x81   // Phản hồi thành công
#define CMD_NACK        0x82   // Phản hồi lỗi
```

### 8.2 `frame.h` / `frame.c` — Khung dữ liệu (Frame Protocol)

#### Cấu trúc frame

```
┌────────┬─────┬─────┬──────────────────┬──────────┐
│ HEADER │ CMD │ LEN │   DATA (0–8B)    │ CHECKSUM │
│  0xAA  │ 1B  │ 1B  │    LEN bytes     │   1B     │
└────────┴─────┴─────┴──────────────────┴──────────┘
Tổng: 4 + LEN bytes
```

| Trường | Kích thước | Mô tả |
|---|---|---|
| HEADER | 1 byte | Luôn = `0xAA` — đánh dấu bắt đầu frame |
| CMD | 1 byte | Mã lệnh (0x01, 0x02...) |
| LEN | 1 byte | Số byte trong DATA (0–8) |
| DATA | 0–8 bytes | Tham số lệnh |
| CHECKSUM | 1 byte | `HEADER ^ CMD ^ LEN ^ DATA[0] ^ ... ^ DATA[N-1]` |

#### Ví dụ frame thực tế

**Bật tất cả 8 thiết bị (CMD_SET_ALL, data=0xFF):**
```
AA  01  01  FF  CS
│   │   │   │   └─ Checksum = 0xAA ^ 0x01 ^ 0x01 ^ 0xFF = 0x55
│   │   │   └───── Data[0] = 0xFF (tất cả bit = 1)
│   │   └───────── Len = 1 byte data
│   └───────────── CMD_SET_ALL
└───────────────── Header
```

**Bật thiết bị số 3 (CMD_SET_SINGLE, index=3, value=1):**
```
AA  02  02  03  01  CS
│   │   │   │   │   └─ CS = 0xAA ^ 0x02 ^ 0x02 ^ 0x03 ^ 0x01 = 0xA8
│   │   │   │   └───── Value = 1 (bật)
│   │   │   └───────── Index = 3 (device 3)
│   │   └───────────── Len = 2 bytes data
│   └───────────────── CMD_SET_SINGLE
└───────────────────── Header
```

#### `frame_init()`

```c
void frame_init() {
    state = 0;    // Reset về trạng thái đầu (chờ HEADER)
    index = 0;    // Reset con trỏ buffer
}
```

#### `frame_calculate_checksum()`

```c
uint8_t frame_calculate_checksum(uint8_t cmd, uint8_t len, uint8_t* data) {
    uint8_t checksum = FRAME_HEADER ^ cmd ^ len;  // Bắt đầu với header
    for (uint8_t i = 0; i < len; i++)
        checksum ^= data[i];                        // XOR từng byte data
    return checksum;
}
```

**XOR checksum hoạt động thế nào?**
- XOR có tính chất: `A ^ A = 0` và `A ^ 0 = A`
- Nếu 1 bit bị lật trong dữ liệu, checksum sẽ khác → phát hiện lỗi
- Đơn giản, không tốn tài nguyên, phù hợp vi điều khiển nhỏ

#### `frame_parse_byte()` — State Machine

Parser dùng **máy trạng thái (state machine)** với 5 trạng thái:

```
        ┌──────────────────────────────────────────────────────┐
        │              frame_parse_byte() State Machine        │
        └──────────────────────────────────────────────────────┘

[State 0: Chờ HEADER]
  byte == 0xAA? → lưu vào buf[0], index=1, → State 1
  byte != 0xAA? → bỏ qua, ở lại State 0

[State 1: Đọc CMD]
  Lưu byte vào buf[1], index=2 → State 2

[State 2: Đọc LEN]
  Lưu byte vào buf[2], expected_len = byte
  len > 8?  → State 0 (lỗi: quá dài)
  len == 0? → State 4 (không có data, nhảy thẳng checksum)
  len > 0?  → State 3 (đọc data)

[State 3: Đọc DATA]
  Lưu byte vào buf[index++]
  index == 3 + expected_len? → State 4 (đủ data)
  index >  3 + expected_len? → State 0 (lỗi: thừa data)

[State 4: Đọc CHECKSUM]
  Tính checksum từ buf[1], buf[2], &buf[3]
  cs == byte? → copy vào frame_t out, State 0, return TRUE
  cs != byte? → State 0 (lỗi checksum), return FALSE
```

**Biến nội bộ (static):**

```c
static uint8_t state = 0;         // Trạng thái hiện tại (0–4)
static uint8_t buffer[16];        // Bộ đệm tạm frame đang nhận
static uint8_t index = 0;         // Con trỏ vị trí ghi tiếp vào buffer
static uint8_t expected_len = 0;  // Số byte data cần nhận (từ trường LEN)
```

---

## 9. services/ — Tầng dịch vụ

### `device_service.h` / `device_service.c` — Quản lý trạng thái thiết bị

Tầng service **trừu tượng hóa** việc điều khiển thiết bị — code app không gọi thẳng driver mà gọi qua service. Service duy trì **shadow register** là bản sao trạng thái hiện tại của Port A.

#### Shadow Register

```c
static uint8_t device_state = 0;
```

Mỗi bit tương ứng 1 thiết bị:

```
bit 7  bit 6  bit 5  bit 4  bit 3  bit 2  bit 1  bit 0
  D7     D6     D5     D4     D3     D2     D1     D0
```

`device_state = 0x05` = `0000 0101` → thiết bị 0 và 2 đang BẬT.

#### `device_service_init()`

```c
void device_service_init(void) {
    device_state = 0x00;             // Tất cả OFF
    ppi8255_write_portA(device_state);  // Đồng bộ ra hardware
}
```

#### `device_service_set_all(uint8_t value)`

```c
void device_service_set_all(uint8_t value) {
    device_state = value;               // Ghi thẳng toàn bộ 8 bit
    ppi8255_write_portA(device_state);
}
```

Ví dụ: `set_all(0xFF)` → `1111 1111` → 8 thiết bị đều BẬT.

#### `device_service_turn_on(uint8_t index)`

```c
void device_service_turn_on(uint8_t index) {
    if (index < 8) {
        device_state |= (1 << index);       // Set bit thứ `index`
        ppi8255_write_portA(device_state);
    }
}
```

Ví dụ: `turn_on(3)` → `device_state |= 0x08` → bit 3 = 1.

#### `device_service_turn_off(uint8_t index)`

```c
void device_service_turn_off(uint8_t index) {
    if (index < 8) {
        device_state &= ~(1 << index);      // Clear bit thứ `index`
        ppi8255_write_portA(device_state);
    }
}
```

Ví dụ: `turn_off(3)` → `device_state &= ~0x08` → bit 3 = 0, các bit khác không đổi.

#### `device_service_toggle(uint8_t index)`

```c
void device_service_toggle(uint8_t index) {
    if (index < 8) {
        device_state ^= (1 << index);       // Đảo bit thứ `index`
        ppi8255_write_portA(device_state);
    }
}
```

#### `device_service_get_state()`

```c
uint8_t device_service_get_state(void) {
    return device_state;   // Trả về trạng thái hiện tại (không đọc hardware)
}
```

Trả về shadow register, không cần giao tiếp với 8255 → nhanh hơn.

---

## 10. app/ — Ứng dụng chính

### `app.h`

```c
void app_init(void);   // Khởi tạo toàn hệ thống
void app_task(void);   // Vòng lặp xử lý chính
```

### `app.c`

#### `app_init()` — Trình tự khởi tạo

```c
void app_init() {
    uart_init();           // 1. Cấu hình chân TX/RX, đặt idle HIGH
    ppi8255_init();        // 2. Cấu hình control bus, ghi Control Word 0x80
    memory_init();         // 3. Xóa RAM về 0
    device_service_init(); // 4. Đặt device_state=0, tắt tất cả thiết bị
    frame_init();          // 5. Reset state machine frame parser

    uart_write_string("System Ready\r\n");  // 6. Báo hiệu sẵn sàng
}
```

**Thứ tự quan trọng:** UART phải init trước để câu `"System Ready"` được gửi đúng. PPI phải init trước `device_service_init()` vì service sẽ gọi `ppi8255_write_portA()` ngay.

#### `app_task()` — Vòng lặp chính

```c
void app_task() {
    uint8_t byte;
    frame_t frame;

    // Bước 1: Đọc 1 byte từ UART (non-blocking)
    if (uart_read_char((char*)&byte)) {

        // Bước 2: Đưa byte vào frame parser
        if (frame_parse_byte(byte, &frame)) {
            uart_write_string("FOK\r\n");  // Debug: frame hợp lệ

            // Bước 3: Xử lý lệnh
            if (frame.cmd == CMD_SET_ALL && frame.len == 1) {
                device_service_set_all(frame.data[0]);
                uart_write_string("ACK\r\n");
            }
            else if (frame.cmd == CMD_SET_SINGLE && frame.len == 2) {
                uint8_t index = frame.data[0];
                uint8_t value = frame.data[1];
                if (value)  device_service_turn_on(index);
                else        device_service_turn_off(index);
                uart_write_string("ACK\r\n");
            }
            else {
                uart_write_string("NACK\r\n");  // Lệnh không hợp lệ
            }
        }
    }
}
```

**Đặc điểm non-blocking:** `uart_read_char()` trả về `false` ngay nếu không có dữ liệu, nên `app_task()` hoàn thành rất nhanh và không block vòng lặp `while(1)` trong main.

---

## 11. main.c — Điểm khởi động

```c
#include "app/app.h"

int main(void) {
    app_init();          // Khởi tạo một lần

    while(1) {
        app_task();      // Lặp vô tận
    }
}
```

Cấu trúc này là pattern chuẩn của hệ thống nhúng bare-metal:
- **`app_init()`**: Chạy đúng 1 lần, cấu hình toàn bộ phần cứng
- **`while(1) { app_task() }`**: Siêu vòng lặp (super loop), xử lý sự kiện liên tục

---

## Phụ lục: Tóm tắt nhanh các hàm

| Hàm | File | Mô tả ngắn |
|---|---|---|
| `main()` | main.c | Điểm vào chương trình |
| `app_init()` | app.c | Khởi tạo toàn bộ hệ thống |
| `app_task()` | app.c | Xử lý UART + frame + lệnh |
| `uart_init()` | uart.c | Cấu hình chân TX/RX |
| `uart_write_char()` | uart.c | Gửi 1 byte qua bit-banging |
| `uart_write_string()` | uart.c | Gửi chuỗi |
| `uart_read_char()` | uart.c | Đọc 1 byte (non-blocking) |
| `ppi8255_init()` | ppi8255.c | Khởi tạo và cấu hình 8255 |
| `ppi8255_write()` | ppi8255.c | Ghi 1 byte vào thanh ghi 8255 |
| `ppi8255_read()` | ppi8255.c | Đọc 1 byte từ thanh ghi 8255 |
| `memory_init()` | memory.c | Xóa RAM |
| `memory_read()` | memory.c | Đọc từ memory map |
| `memory_write()` | memory.c | Ghi vào memory map |
| `cpu_bus_read()` | cpu_bus.c | Giao diện đọc bus CPU |
| `cpu_bus_write()` | cpu_bus.c | Giao diện ghi bus CPU |
| `io_map_read()` | io_map.c | Đọc thiết bị IO theo địa chỉ |
| `io_map_write()` | io_map.c | Ghi thiết bị IO theo địa chỉ |
| `frame_init()` | frame.c | Reset frame parser |
| `frame_parse_byte()` | frame.c | Đưa byte vào state machine |
| `frame_calculate_checksum()` | frame.c | Tính XOR checksum |
| `device_service_init()` | device_service.c | Tắt tất cả thiết bị |
| `device_service_set_all()` | device_service.c | Đặt trạng thái 8 thiết bị |
| `device_service_turn_on()` | device_service.c | Bật 1 thiết bị theo index |
| `device_service_turn_off()` | device_service.c | Tắt 1 thiết bị theo index |
| `device_service_toggle()` | device_service.c | Đảo trạng thái 1 thiết bị |
| `device_service_get_state()` | device_service.c | Đọc shadow register |
