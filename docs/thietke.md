
# Tài liệu Thiết Kế Mạch PCB — ATmega16 + 8255 + ESP32

> Tài liệu này mô tả **toàn bộ kết nối phần cứng** để thiết kế mạch PCB.
> Tất cả tên chân lấy trực tiếp từ `board.h` và `system_config.h`.
> IC sử dụng chân cắm **DIP**.

---

## MỤC LỤC

1. [Danh sách linh kiện (BOM)](#1-bom)
2. [Sơ đồ khối tổng thể](#2-so-do-khoi)
3. [ATmega16 — Pinout đầy đủ DIP-40](#3-atmega16-pinout)
4. [ATmega16 ↔ 8255 — Data Bus + Control Bus](#4-atmega16-8255)
5. [8255 Port A → ULN2803 → Relay](#5-port-a-relay)
6. [8255 Port B ← ADC0804 — Data 8-bit](#6-port-b-adc)
7. [8255 Port C → ADC0804 + 74HC4051](#7-port-c-adc-mux)
8. [74HC4051 ← ACS712 — 8 kênh đo dòng](#8-mux-acs712)
9. [ATmega16 ↔ ESP32 — Soft UART + Level Shift](#9-uart-esp32)
10. [Mạch nguồn](#10-nguon)
11. [Mạch thạch anh + Reset ATmega16](#11-crystal-reset)
12. [Tụ bypass toàn board](#12-bypass)
13. [Bảng nối dây tổng hợp — Wire List](#13-wire-list)
14. [Lưu ý thiết kế PCB](#14-luu-y)

---

## 1. Danh sách linh kiện (BOM)

| STT | Linh kiện | Model / Giá trị | Package | SL | Ghi chú |
|-----|----------|-----------------|---------|-----|---------|
| 1 | Vi điều khiển | ATmega16A | DIP-40 | 1 | 8MHz thạch anh ngoài |
| 2 | PPI I/O | Intel 8255A | DIP-40 | 1 | |
| 3 | ADC 8-bit | ADC0804LCN | DIP-20 | 1 | |
| 4 | MUX Analog | 74HC4051 | DIP-16 | 1 | 8 kênh analog |
| 5 | Darlington Array | ULN2803A | DIP-18 | 1 | Driver 8 relay |
| 6 | Cảm biến dòng | ACS712-05B Module | Module | 8 | 185 mV/A, max 5A |
| 7 | WiFi MCU | ESP32 DevKit V1 | Module | 1 | UART bridge → MQTT |
| 8 | Relay | 5V SPDT | PCB mount | 8 | Cuộn 5V, tiếp điểm 10A |
| 9 | Thạch anh | 8 MHz | HC-49S | 1 | ATmega16 |
| 10 | Tụ thạch anh | 22 pF | DIP | 2 | |
| 11 | Tụ bypass IC | 100 nF ceramic | DIP | 10 | Gần chân VCC mỗi IC |
| 12 | Tụ lọc nguồn | 10 µF electrolytic | DIP | 3 | Đầu ra LM7805 |
| 13 | Tụ clock ADC | 150 pF | DIP | 1 | RC clock ADC0804 |
| 14 | R clock ADC | 10 kΩ | DIP | 1 | RC clock ADC0804 |
| 15 | R phân áp VREF/2 | 10 kΩ | DIP | 2 | Tạo 2.5V cho ADC VREF/2 |
| 16 | R chống floating MUX | 10 kΩ | DIP | 8 | Kênh chưa có ACS712 |
| 17 | R level shift TX (trên) | 10 kΩ | DIP | 1 | ATmega TX → ESP32 |
| 18 | R level shift TX (dưới) | 20 kΩ | DIP | 1 | ATmega TX → ESP32 |
| 19 | R pull-up RESET | 10 kΩ | DIP | 1 | ATmega16 RESET |
| 20 | Tụ power-on reset | 100 nF | DIP | 1 | ATmega16 RESET |
| 21 | Nút nhấn RESET | SPST | 6×6 mm | 1 | |
| 22 | LED nguồn | 3 mm xanh | LED | 1 | |
| 23 | R LED | 1 kΩ | DIP | 1 | |
| 24 | Regulator 5V | LM7805 | TO-220 | 1 | |
| 25 | Regulator 3.3V | AMS1117-3.3 | SOT-223 | 1 | Cấp ESP32 nếu không dùng USB |
| 26 | Diode bảo vệ nguồn | 1N4007 | DO-41 | 1 | Chống cắm ngược |
| 27 | Diode flyback relay | 1N4007 | DO-41 | 8 | Mỗi relay 1 cái |
| 28 | Jack nguồn | DC 5.5/2.1 mm | PCB | 1 | Input 7–12V DC |
| 29 | Cuộn choke AVCC | 100 µH | DIP | 1 | Lọc nhiễu nguồn analog |
| 30 | Header ISP | 6-pin | Header | 1 | Nạp firmware AVR |
| 31 | Header UART debug | 3-pin | Header | 1 | GND / TX / RX |
| 32 | Header ACS712 | 3-pin | Connector | 8 | VCC / GND / OUT |

---

## 2. Sơ đồ khối tổng thể

```
[DC 7-12V] → [1N4007] → [LM7805] → [+5V Rail]
                                          │
                              ┌───────────┼───────────────────────┐
                              │           │                       │
                         ATmega16       8255                  ACS712 x8
                         ADC0804      74HC4051                Relay x8
                         ULN2803                              ACS712 VCC

[+5V] → [AMS1117-3.3] → [+3.3V] → ESP32 VIN (nếu không dùng USB)


 ┌─────────────────────────────────────────────────────────────────┐
 │                      ATmega16  (DIP-40)                         │
 │                                                                 │
 │  PORTD PD0..PD7  ◄──────────────────────────►  8255 D0..D7     │
 │  PORTB PB0..PB4  ───────────────────────────►  8255 /WR /RD A0 A1 /CS │
 │                                                                 │
 │  PORTC PC0 (TX)  ──[10kΩ+20kΩ divider]──────►  ESP32 GPIO16 RX │
 │  PORTC PC1 (RX)  ◄────────────────────────────  ESP32 GPIO17 TX │
 └─────────────────────────────────────────────────────────────────┘

 ┌─────────────────────────────────────────────────────────────────┐
 │                       8255  (DIP-40)                            │
 │                                                                 │
 │  Port A PA0..PA7  ──►  ULN2803 IN1..IN8                        │
 │                              │                                  │
 │                        ULN2803 OUT1..OUT8  ──►  Relay coil(-) x8 │
 │                                                                 │
 │  Port B PB0..PB7  ◄──  ADC0804 D0..D7                          │
 │                                                                 │
 │  Port C PC0  ──────────────►  ADC0804 /CS  (pin 1)             │
 │  Port C PC1  ──────────────►  ADC0804 /WR  (pin 3)             │
 │  Port C PC2  ──────────────►  ADC0804 /RD  (pin 2)             │
 │  Port C PC4  ──────────────►  74HC4051 S0/A (pin 11)           │
 │  Port C PC5  ──────────────►  74HC4051 S1/B (pin 12)           │
 │  Port C PC6  ──────────────►  74HC4051 S2/C (pin 13)           │
 └─────────────────────────────────────────────────────────────────┘

 74HC4051 COM/Y (pin 1) ──────────►  ADC0804 VIN+ (pin 6)
 74HC4051 X0..X7        ◄──────────  ACS712 VIOUT kênh 0..7

 ACS712:  IP+ → Dây pha AC vào  |  IP- → Dây pha AC ra tải
```

---

## 3. ATmega16 — Pinout đầy đủ (DIP-40)

```
                       ATmega16A  DIP-40
                  ┌────────────────────────┐
  [8255 /WR    ]  │  1  PB0       PA0  40  │  [không dùng]
  [8255 /RD    ]  │  2  PB1       PA1  39  │  [không dùng]
  [8255 A0     ]  │  3  PB2       PA2  38  │  [không dùng]
  [8255 A1     ]  │  4  PB3       PA3  37  │  [không dùng]
  [8255 /CS    ]  │  5  PB4       PA4  36  │  [không dùng]
  [ISP MOSI    ]  │  6  PB5       PA5  35  │  [không dùng]
  [ISP MISO    ]  │  7  PB6       PA6  34  │  [không dùng]
  [ISP SCK     ]  │  8  PB7       PA7  33  │  [không dùng]
  [RESET mạch  ]  │  9  RESET    AREF  32  │  [để hở]
  [+5V         ]  │ 10  VCC      AVCC  30  │  [+5V qua L=100µH]
  [GND         ]  │ 11  GND       GND  31  │  [GND]
  [XTAL 8MHz   ]  │ 12  XTAL2     PC7  29  │  [không dùng]
  [XTAL 8MHz   ]  │ 13  XTAL1     PC6  28  │  [không dùng]
  [8255 D0     ]  │ 14  PD0       PC5  27  │  [không dùng]
  [8255 D1     ]  │ 15  PD1       PC4  26  │  [không dùng]
  [8255 D2     ]  │ 16  PD2       PC3  25  │  [không dùng]
  [8255 D3     ]  │ 17  PD3       PC2  24  │  [không dùng]
  [8255 D4     ]  │ 18  PD4       PC1  23  │  [UART RX ← ESP32 TX]
  [8255 D5     ]  │ 19  PD5       PC0  22  │  [UART TX → ESP32 RX]
  [8255 D6     ]  │ 20  PD6                │
  [8255 D7     ]  │ 21  PD7                │
                  └────────────────────────┘
```

> **AVCC (pin 30):** +5V qua cuộn choke L=100µH → thêm tụ 100nF + 10µF xuống GND.
> **Port A (PA0..PA7):** Không dùng trong project — để hở hoặc nối GND qua 10kΩ.
> **ISP (PB5/PB6/PB7 + RESET/VCC/GND):** Đưa ra header 6-pin để nạp firmware.

---

## 4. ATmega16 ↔ 8255 (Data Bus + Control Bus)

### 4.1 Data Bus — 8 dây nối thẳng 1-1

| ATmega16 | Pin ATmega | → | 8255 | Pin 8255 |
|----------|-----------|---|------|---------|
| PD0 | 14 | ──► | D0 | 34 |
| PD1 | 15 | ──► | D1 | 33 |
| PD2 | 16 | ──► | D2 | 32 |
| PD3 | 17 | ──► | D3 | 31 |
| PD4 | 18 | ──► | D4 | 30 |
| PD5 | 19 | ──► | D5 | 29 |
| PD6 | 20 | ──► | D6 | 28 |
| PD7 | 21 | ──► | D7 | 27 |

> ⚠️ Bus 2 chiều: firmware set `DDRD=0xFF` khi ghi, `DDRD=0x00` khi đọc.

### 4.2 Control Bus — 5 dây

| Tín hiệu | ATmega | Pin ATmega | → | 8255 | Pin 8255 |
|----------|--------|-----------|---|------|---------|
| /WR | PB0 | 1 | ──► | /WR | 36 |
| /RD | PB1 | 2 | ──► | /RD | 5 |
| A0  | PB2 | 3 | ──► | A0  | 9 |
| A1  | PB3 | 4 | ──► | A1  | 8 |
| /CS | PB4 | 5 | ──► | /CS | 6 |

### 4.3 8255 Power + Reset

| 8255 | Pin | Nối vào |
|------|-----|---------|
| VCC | 26 | +5V |
| GND | 7 | GND |
| RESET | 35 | **GND** *(active HIGH — kéo xuống GND để không reset)* |

---

## 5. 8255 Port A → ULN2803 → Relay

> 8255 Port A = Output. **Bắt buộc dùng ULN2803** làm driver.

### 5.1 Pinout 8255 Port A (DIP-40)

| 8255 Port A | Pin 8255 | Thiết bị |
|-------------|---------|---------|
| PA0 | 4 | Relay 0 |
| PA1 | 3 | Relay 1 |
| PA2 | 2 | Relay 2 |
| PA3 | 1 | Relay 3 |
| PA4 | 40 | Relay 4 |
| PA5 | 39 | Relay 5 |
| PA6 | 38 | Relay 6 |
| PA7 | 37 | Relay 7 |

### 5.2 Bảng nối 8255 PA → ULN2803 → Relay

| 8255 | Pin 8255 | ULN IN | Pin ULN | ULN OUT | Pin ULN | Relay |
|------|---------|--------|---------|---------|---------|-------|
| PA0 | 4  | IN1 | 1 | OUT1 | 16 | Relay 0 coil(−) |
| PA1 | 3  | IN2 | 2 | OUT2 | 15 | Relay 1 coil(−) |
| PA2 | 2  | IN3 | 3 | OUT3 | 14 | Relay 2 coil(−) |
| PA3 | 1  | IN4 | 4 | OUT4 | 13 | Relay 3 coil(−) |
| PA4 | 40 | IN5 | 5 | OUT5 | 12 | Relay 4 coil(−) |
| PA5 | 39 | IN6 | 6 | OUT6 | 11 | Relay 5 coil(−) |
| PA6 | 38 | IN7 | 7 | OUT7 | 10 | Relay 6 coil(−) |
| PA7 | 37 | IN8 | 8 | OUT8 | 9  | Relay 7 coil(−) |

### 5.3 ULN2803 Power

| ULN2803 | Pin | Nối vào |
|---------|-----|---------|
| GND | 9 | GND |
| COM | 18 | +5V *(flyback diode chung — về nguồn relay)* |

### 5.4 Sơ đồ 1 relay (nhân × 8)

```
+5V ─────────── Relay coil (+)
                 Relay coil (−) ── ULN2803 OUTx ── GND

+5V ── Cathode [1N4007] Anode ── ULN2803 OUTx   (diode flyback)

Relay COM ───── Dây pha AC (từ nguồn 220V)
Relay NO  ───── Tải AC (đèn / thiết bị)
Relay NC  ───── để hở
```

---

## 6. 8255 Port B ← ADC0804 (Data 8-bit)

> 8255 Port B = Input. Nhận 8-bit kết quả từ ADC0804.

| 8255 Port B | Pin 8255 | ← | ADC0804 | Pin ADC |
|-------------|---------|---|---------|---------|
| PB0 (LSB) | 18 | ◄── | D0 (LSB) | 18 |
| PB1 | 19 | ◄── | D1 | 17 |
| PB2 | 20 | ◄── | D2 | 16 |
| PB3 | 21 | ◄── | D3 | 15 |
| PB4 | 22 | ◄── | D4 | 14 |
| PB5 | 23 | ◄── | D5 | 13 |
| PB6 | 24 | ◄── | D6 | 12 |
| PB7 (MSB) | 25 | ◄── | D7 (MSB) | 11 |

---

## 7. 8255 Port C → ADC0804 (Control) + 74HC4051 (MUX)

> 8255 Port C = Output hoàn toàn.

### 7.1 Bảng nối Port C

| 8255 Port C | Pin 8255 | Nối tới | Pin đích | Ý nghĩa |
|-------------|---------|---------|---------|---------|
| PC0 | 14 | ADC0804 /CS | 1 | Kích hoạt ADC (active LOW) |
| PC1 | 15 | ADC0804 /WR | 3 | Start conversion (cạnh lên) |
| PC2 | 16 | ADC0804 /RD | 2 | Đọc kết quả (active LOW) |
| PC3 | 17 | không dùng | — | Để hở |
| PC4 | 13 | 74HC4051 S0/A | 11 | Chọn kênh bit 0 |
| PC5 | 12 | 74HC4051 S1/B | 12 | Chọn kênh bit 1 |
| PC6 | 11 | 74HC4051 S2/C | 13 | Chọn kênh bit 2 |
| PC7 | 10 | không dùng | — | Để hở |

### 7.2 ADC0804 — Toàn bộ chân (DIP-20)

| ADC0804 | Pin | Nối vào |
|---------|-----|---------|
| /CS | 1 | 8255 PC0 (pin 14) |
| /RD | 2 | 8255 PC2 (pin 16) |
| /WR | 3 | 8255 PC1 (pin 15) |
| CLK IN | 4 | Junction R=10kΩ + C=150pF |
| /INTR | 5 | +5V *(bỏ qua ngắt)* |
| VIN+ | 6 | 74HC4051 COM/Y (pin 1) |
| VIN− | 7 | GND |
| AGND | 8 | GND analog *(tách riêng GND số)* |
| VREF/2 | 9 | +2.5V *(cầu phân áp 2×10kΩ)* |
| DGND | 10 | GND |
| D7 (MSB) | 11 | 8255 PB7 (pin 25) |
| D6 | 12 | 8255 PB6 (pin 24) |
| D5 | 13 | 8255 PB5 (pin 23) |
| D4 | 14 | 8255 PB4 (pin 22) |
| D3 | 15 | 8255 PB3 (pin 21) |
| D2 | 16 | 8255 PB2 (pin 20) |
| D1 | 17 | 8255 PB1 (pin 19) |
| D0 (LSB) | 18 | 8255 PB0 (pin 18) |
| CLK R | 19 | Junction R=10kΩ + C=150pF |
| VCC | 20 | +5V |

#### Mạch clock ADC0804 — f ≈ 606 kHz

```
                  ┌── CLK IN (pin 4)
+5V ──[R=10kΩ]───┤
                  └── CLK R  (pin 19)
                  │
                 [C=150pF]
                  │
                 GND
```

#### Mạch phân áp VREF/2 = 2.5V

```
+5V ──[R=10kΩ]──┬── ADC0804 VREF/2 (pin 9)
                 │   + tụ 100nF song song xuống GND
                [R=10kΩ]
                 │
                GND
```

---

## 8. 74HC4051 ← ACS712 (8 kênh đo dòng)

### 8.1 74HC4051 — Toàn bộ chân (DIP-16)

| 74HC4051 | Pin | Nối vào |
|----------|-----|---------|
| COM/Y | 1 | ADC0804 VIN+ (pin 6) |
| CH4 (X4) | 2 | ACS712 kênh 4 — VIOUT |
| CH6 (X6) | 3 | ACS712 kênh 6 — VIOUT |
| CH7 (X7) | 4 | ACS712 kênh 7 — VIOUT |
| CH5 (X5) | 5 | ACS712 kênh 5 — VIOUT |
| /INH | 6 | GND *(luôn enable)* |
| VEE | 7 | GND *(single supply 5V)* |
| GND | 8 | GND |
| CH0 (X0) | 9 | ACS712 kênh 0 — VIOUT |
| CH1 (X1) | 10 | ACS712 kênh 1 — VIOUT |
| S0 / A | 11 | 8255 PC4 (pin 13) |
| S1 / B | 12 | 8255 PC5 (pin 12) |
| S2 / C | 13 | 8255 PC6 (pin 11) |
| CH3 (X3) | 14 | ACS712 kênh 3 — VIOUT |
| CH2 (X2) | 15 | ACS712 kênh 2 — VIOUT |
| VCC | 16 | +5V |

### 8.2 Bảng chọn kênh MUX

| PC6 (C) | PC5 (B) | PC4 (A) | Kênh | Pin MUX | Thiết bị |
|---------|---------|---------|------|---------|---------|
| 0 | 0 | 0 | X0 | 9  | Thiết bị 0 |
| 0 | 0 | 1 | X1 | 10 | Thiết bị 1 |
| 0 | 1 | 0 | X2 | 15 | Thiết bị 2 |
| 0 | 1 | 1 | X3 | 14 | Thiết bị 3 |
| 1 | 0 | 0 | X4 | 2  | Thiết bị 4 |
| 1 | 0 | 1 | X5 | 5  | Thiết bị 5 |
| 1 | 1 | 0 | X6 | 3  | Thiết bị 6 |
| 1 | 1 | 1 | X7 | 4  | Thiết bị 7 |

### 8.3 ACS712 Module — 1 module (nhân × 8)

```
Phần tín hiệu thấp:
  ACS712 VCC  ── +5V
  ACS712 GND  ── GND
  ACS712 OUT  ── 74HC4051 CHx

Phần dòng điện AC (tách hoàn toàn khỏi phần số):
  ACS712 IP+  ── Dây pha AC vào (từ nguồn 220V)
  ACS712 IP-  ── Dây pha AC ra  (đến tải: đèn, thiết bị)
```

### 8.4 Chống floating kênh chưa có ACS712

Kênh chưa lắp ACS712 **bắt buộc nối về 2.5V** (tránh nhiễu crosstalk):

```
+5V ──[R=10kΩ]──┬── 74HC4051 CHx
                [R=10kΩ]
                 │
                GND
```

---

## 9. ATmega16 ↔ ESP32 (Soft UART + Level Shift)

> ATmega16 = 5V logic | ESP32 = 3.3V logic
> TX ATmega → ESP32: **phải giảm áp 5V → 3.3V**
> TX ESP32 → ATmega RX: nối thẳng (3.3V > VIH_min AVR 3.0V ✓)

### 9.1 Chân Soft UART ATmega16 (PORTC)

| Tín hiệu | ATmega | Pin |
|----------|--------|-----|
| TX (Output) | PC0 | 22 |
| RX (Input) | PC1 | 23 |

### 9.2 Mạch Level Shift TX: 5V → 3.3V

```
ATmega PC0 (5V) ──[R1=10kΩ]──┬──[R2=20kΩ]── GND
                               │
                               └── ESP32 GPIO16 RX2

Điện áp tại junction = 5 × 20/(10+20) = 3.33V ✓
```

### 9.3 Bảng nối đầy đủ ATmega16 ↔ ESP32

| ATmega16 | Pin | Mạch giữa | ESP32 DevKit | Pin ESP32 |
|----------|-----|-----------|-------------|----------|
| PC0 TX | 22 | 10kΩ + 20kΩ divider | GPIO16 (RX2) | 27 |
| PC1 RX | 23 | nối thẳng | GPIO17 (TX2) | 26 |
| GND | 11 | dây | GND | GND |

> ESP32 code: `Serial2.begin(9600, SERIAL_8N1, 16, 17)`
> Baudrate = 9600 (theo `UART_BAUDRATE` trong `system_config.h`)

### 9.4 Header UART debug (đặt trên PCB)

```
3-pin header:  [GND] – [PC0 TX] – [PC1 RX]
Dùng cắm USB-TTL để debug mà không cần ESP32.
```

---

## 10. Mạch nguồn

### 10.1 Sơ đồ tổng

```
Jack DC 7–12V ──[1N4007]──┬── LM7805 IN
(chống ngược)              │       │ OUT ── +5V Rail
                           │      GND
                           │
                      (dự phòng AMS1117-5.0)

+5V Rail ──[AMS1117-3.3]── +3.3V ── ESP32 VIN (nếu không cấp qua USB)
```

### 10.2 LM7805 — Mạch lọc

```
IN  ──[C=100µF 25V]── GND
OUT ──[C=10µF  16V]── GND
OUT ──[C=100nF    ]── GND
```

### 10.3 Phân phối nguồn

| Domain | Điện áp | Cấp cho |
|--------|---------|---------|
| +5V Digital | 5.0V | ATmega16, 8255, ADC0804, 74HC4051, ULN2803, ACS712, Relay |
| +5V Analog (AVCC) | 5.0V qua L=100µH | ATmega16 AVCC pin 30 |
| +3.3V | 3.3V | ESP32 VIN |

---

## 11. Mạch thạch anh + Reset ATmega16

### 11.1 Thạch anh 8MHz

```
ATmega XTAL1 (pin 13) ──┬── [XTAL 8MHz HC-49S] ──┬── ATmega XTAL2 (pin 12)
                        [C1=22pF]               [C2=22pF]
                         │                       │
                        GND                     GND
```

> **Fuse bits:** `CKSEL=1111`, `SUT=11` → External Crystal 8–16 MHz

### 11.2 Mạch Reset

```
+5V ──[R=10kΩ]──┬───────────────────── ATmega RESET (pin 9)
                 │
                [C=100nF]         [SW_RESET: Nút nhấn]
                 │                 │
                GND ───────────────┘ (nút nối RESET xuống GND)
```

---

## 12. Tụ bypass toàn board

> Đặt sát chân VCC mỗi IC, trace về GND dưới 5 mm.

| IC | Chân VCC | Tụ bypass |
|----|---------|-----------|
| ATmega16 VCC | pin 10 | 100nF + 10µF |
| ATmega16 AVCC | pin 30 | 100nF + 10µF sau L=100µH |
| 8255 VCC | pin 26 | 100nF |
| ADC0804 VCC | pin 20 | 100nF |
| 74HC4051 VCC | pin 16 | 100nF |
| ACS712 VCC | mỗi module | 100nF |

---

## 13. Bảng nối dây tổng hợp (Wire List)

### Data Bus — ATmega16 PORTD → 8255

| ATmega Pin | 8255 Pin |
|-----------|---------|
| PD0 (14) | D0 (34) |
| PD1 (15) | D1 (33) |
| PD2 (16) | D2 (32) |
| PD3 (17) | D3 (31) |
| PD4 (18) | D4 (30) |
| PD5 (19) | D5 (29) |
| PD6 (20) | D6 (28) |
| PD7 (21) | D7 (27) |

### Control Bus — ATmega16 PORTB → 8255

| ATmega Pin | 8255 Pin |
|-----------|---------|
| PB0 /WR (1) | /WR (36) |
| PB1 /RD (2) | /RD (5) |
| PB2 A0  (3) | A0  (9) |
| PB3 A1  (4) | A1  (8) |
| PB4 /CS (5) | /CS (6) |

### 8255 Port B → ADC0804 Data

| 8255 Pin | ADC0804 Pin |
|---------|------------|
| PB0 (18) | D0 (18) |
| PB1 (19) | D1 (17) |
| PB2 (20) | D2 (16) |
| PB3 (21) | D3 (15) |
| PB4 (22) | D4 (14) |
| PB5 (23) | D5 (13) |
| PB6 (24) | D6 (12) |
| PB7 (25) | D7 (11) |

### 8255 Port C → ADC0804 Control

| 8255 Pin | ADC0804 Pin |
|---------|------------|
| PC0 (14) | /CS (1) |
| PC1 (15) | /WR (3) |
| PC2 (16) | /RD (2) |

### 8255 Port C → 74HC4051 MUX Select

| 8255 Pin | 74HC4051 Pin |
|---------|-------------|
| PC4 (13) | S0/A (11) |
| PC5 (12) | S1/B (12) |
| PC6 (11) | S2/C (13) |

### 74HC4051 → ADC0804

| 74HC4051 Pin | ADC0804 Pin |
|-------------|------------|
| COM/Y (1) | VIN+ (6) |

### 8255 Port A → ULN2803 → Relay

| 8255 Pin | ULN IN (Pin) | ULN OUT (Pin) | Relay |
|---------|-------------|--------------|-------|
| PA0 (4)  | IN1 (1) | OUT1 (16) | Relay 0 |
| PA1 (3)  | IN2 (2) | OUT2 (15) | Relay 1 |
| PA2 (2)  | IN3 (3) | OUT3 (14) | Relay 2 |
| PA3 (1)  | IN4 (4) | OUT4 (13) | Relay 3 |
| PA4 (40) | IN5 (5) | OUT5 (12) | Relay 4 |
| PA5 (39) | IN6 (6) | OUT6 (11) | Relay 5 |
| PA6 (38) | IN7 (7) | OUT7 (10) | Relay 6 |
| PA7 (37) | IN8 (8) | OUT8 (9)  | Relay 7 |

### ATmega16 ↔ ESP32

| ATmega Pin | Mạch | ESP32 Pin |
|-----------|------|----------|
| PC0 TX (22) | 10kΩ + 20kΩ divider | GPIO16 RX2 (27) |
| PC1 RX (23) | nối thẳng | GPIO17 TX2 (26) |
| GND (11) | dây | GND |

---

## 14. Lưu ý thiết kế PCB

1. **Star grounding:** Tách AGND (ADC0804 pin 8, 10) và DGND, gặp nhau tại 1 điểm duy nhất ở connector nguồn.
2. **Data Bus trace:** Trace PORTD (8 dây) giữ ngắn, song song nhau. Đặt 8255 gần ATmega nhất.
3. **Relay isolation:** Phần relay và trace AC đặt cách phần analog ADC/ACS712 tối thiểu 5 mm. Dùng khe cắt PCB nếu cần.
4. **Diode flyback relay:** 1N4007 song song mỗi cuộn relay — Cathode nối +5V, Anode nối OUT ULN2803.
5. **Decoupling:** Mỗi IC có 1 tụ 100nF, trace về GND dưới 5 mm.
6. **ISP header 6-pin:** MOSI / MISO / SCK / RESET / VCC / GND — để nạp firmware AVR không cần tháo IC.
7. **ACS712 AC trace:** Trace IP+/IP− rộng ≥ 2 mm, chịu dòng. Tách hoàn toàn khỏi phần tín hiệu số.
8. **XTAL layout:** Thạch anh và 2 tụ 22pF đặt sát XTAL1/XTAL2, trace ngắn nhất có thể, không đi trace khác qua vùng này.
9. **VREF/2 ADC:** Tụ lọc 100nF đặt sát chân VREF/2 (pin 9 ADC0804) để giảm nhiễu.
10. **Nguồn analog AVCC:** Đi riêng đường trace từ +5V qua cuộn L=100µH → tụ lọc → AVCC pin 30 ATmega.

