# Tài liệu ADC & Đo Dòng Điện

> **Mục tiêu:** Bổ sung chức năng đọc dòng điện cho từng thiết bị, trong đó `Port A` của `8255` vẫn giữ vai trò điều khiển tải, còn khối đo dòng `74HC4051 + ADC0804` kết nối vào `8255` (chủ yếu `Port B/Port C`) để `ATmega` đọc lại qua bus hệ thống.

---

## MỤC LỤC

1. [Tổng quan kiến trúc đo dòng](#1-tổng-quan-kiến-trúc-đo-dòng)
2. [Lý thuyết ADC0804](#2-lý-thuyết-adc0804)
3. [Lý thuyết 74HC4051](#3-lý-thuyết-74hc4051)
4. [Sơ đồ kết nối phần cứng (đúng theo wiring mới)](#4-sơ-đồ-kết-nối-phần-cứng-đúng-theo-wiring-mới)
5. [Ánh xạ chân và bit điều khiển trong `board.h`](#5-ánh-xạ-chân-và-bit-điều-khiển-trong-boardh)
6. [Cấu hình `8255` cho chế độ đo dòng](#6-cấu-hình-8255-cho-chế-độ-đo-dòng)
7. [Thiết kế driver `adc0804.c`](#7-thiết-kế-driver-adc0804c)
8. [Tầng service và lệnh giao thức](#8-tầng-service-và-lệnh-giao-thức)
9. [Luồng dữ liệu đầu-cuối](#9-luồng-dữ-liệu-đầu-cuối)
10. [So sánh với phiên bản cũ](#10-so-sánh-với-phiên-bản-cũ)
11. [Lưu ý thực tế khi triển khai](#11-lưu-ý-thực-tế-khi-triển-khai)

---

## 1. Tổng quan kiến trúc đo dòng

### 1.1 Bài toán

- `8255 Port A` đang dùng để bật/tắt thiết bị (relay/đèn).
- Cần đo dòng của từng thiết bị và gửi về PC theo frame UART.
- Không lấy dữ liệu ADC trực tiếp bằng chân GPIO ATmega nữa, mà đi qua `8255`.

### 1.2 Kiến trúc đã áp dụng

```text
Cảm biến dòng kênh 0..7
          │
          ▼
      74HC4051 (chọn 1 kênh analog)
          │
          ▼
       ADC0804 (chuyển đổi A/D 8-bit)
          │ D0..D7
          ▼
       8255 Port B (Input)
          │
          ▼
       ATmega đọc qua bus 8255
```

Phần điều khiển ADC/mux:

```text
ATmega -> 8255 Port C -> (CS#, WR#, RD# của ADC0804) + (A/B/C của 74HC4051)
```

Vai trò từng port của `8255` trong hệ thống mới:

| Port | Chức năng |
|---|---|
| `Port A` | Điều khiển thiết bị (giữ nguyên) |
| `Port B` | Nhận dữ liệu số `D0..D7` từ `ADC0804` |
| `Port C` | Xuất tín hiệu điều khiển `ADC0804` + chọn kênh `74HC4051` |

---

## 2. Lý thuyết ADC0804

`ADC0804` là ADC 8-bit kiểu SAR (Successive Approximation Register):

- Đầu vào: tín hiệu analog (`VIN+`, `VIN-`).
- Đầu ra: dữ liệu số song song `D0..D7`.
- Điều khiển bằng các tín hiệu:
  - `CS#`: chọn chip
  - `WR#`: phát xung bắt đầu chuyển đổi
  - `RD#`: cho phép xuất dữ liệu ra bus

### Trình tự cơ bản

1. Chọn chip (`CS# = 0`).
2. Tạo xung `WR#` thấp -> cao để start conversion.
3. Chờ đủ thời gian chuyển đổi (hoặc chờ `INTR#` nếu có wiring đọc INTR).
4. Kéo `RD# = 0` để ADC xuất dữ liệu `D0..D7`.
5. Đọc dữ liệu.
6. Trả `RD#`, `CS#` về mức không hoạt động.

Trong code hiện tại, driver dùng delay cố định khoảng `120us` (thay cho polling `INTR#`).

---

## 3. Lý thuyết 74HC4051

`74HC4051` là analog multiplexer 8-kênh:

- Có 8 đầu vào analog (`Y0..Y7`), 1 đầu ra chung (`Z`).
- Chọn kênh bằng 3 bit địa chỉ `A/B/C`.
- Đầu ra `Z` nối vào `VIN+` của `ADC0804`.

Bảng chọn kênh theo bit:

| C | B | A | Kênh |
|:---:|:---:|:---:|:---:|
| 0 | 0 | 0 | `Y0` |
| 0 | 0 | 1 | `Y1` |
| 0 | 1 | 0 | `Y2` |
| 0 | 1 | 1 | `Y3` |
| 1 | 0 | 0 | `Y4` |
| 1 | 0 | 1 | `Y5` |
| 1 | 1 | 0 | `Y6` |
| 1 | 1 | 1 | `Y7` |

Trong firmware, `index` thiết bị (0..7) được map trực tiếp sang `A/B/C` của mux.

---

## 4. Sơ đồ kết nối phần cứng (đúng theo wiring mới)

```text
Cảm biến dòng CH0..CH7 -> 74HC4051 -> ADC0804 -> 8255 Port B

8255 Port C bit control:
  PC0 -> ADC0804 CS#
  PC1 -> ADC0804 WR#
  PC2 -> ADC0804 RD#
  PC4 -> 74HC4051 A
  PC5 -> 74HC4051 B
  PC6 -> 74HC4051 C

8255 Port A -> tải/thiết bị (ON/OFF)
```

Điểm quan trọng:

- `Port A` của `8255` **không** dùng cho ADC.
- Dữ liệu ADC không đi thẳng vào `PORTD` của ATmega như thiết kế cũ.
- Dữ liệu đi qua `Port B` của `8255`, sau đó ATmega đọc bằng hàm driver `ppi8255_read_portB()`.

---

## 5. Ánh xạ chân và bit điều khiển trong `board.h`

Các macro dùng trong mã nguồn:

```c
#define PPI_ADC_CS_BIT    0
#define PPI_ADC_WR_BIT    1
#define PPI_ADC_RD_BIT    2

#define PPI_ADC_MUX_A_BIT 4
#define PPI_ADC_MUX_B_BIT 5
#define PPI_ADC_MUX_C_BIT 6
```

Ý nghĩa:

- 3 bit thấp điều khiển phiên đọc/chuyển đổi ADC.
- 3 bit cao chọn kênh analog từ `74HC4051`.
- Dùng cùng một thanh ghi `Port C` của `8255` để đồng bộ luồng đo.

---

## 6. Cấu hình `8255` cho chế độ đo dòng

Trong `ppi8255_init()`, control word đã đổi sang `0x82` (Mode 0):

- `Port A = Output`
- `Port B = Input`
- `Port C upper/lower = Output`

Giải mã `0x82 = 1000 0010b`:

- `D7 = 1`: mode set
- `D6..D5 = 00`: Group A Mode 0
- `D4 = 0`: Port A output
- `D3 = 0`: PC upper output
- `D2 = 0`: Group B Mode 0
- `D1 = 1`: Port B input
- `D0 = 0`: PC lower output

Cấu hình này đúng với kiến trúc: A điều khiển tải, B đọc ADC, C điều khiển ADC/mux.

---

## 7. Thiết kế driver `adc0804.c`

### 7.1 Mục tiêu driver

- Không điều khiển ADC trực tiếp bằng GPIO ATmega.
- Điều khiển qua `ppi8255_write_portC()`.
- Đọc dữ liệu bằng `ppi8255_read_portB()`.

### 7.2 Cơ chế nội bộ

Driver dùng biến `portc_shadow` để giữ trạng thái hiện tại của `Port C`, tránh làm mất bit khi chỉ muốn đổi 1 tín hiệu.

Pseudo-flow `adc0804_read(channel)`:

1. Set bit mux A/B/C theo `channel`.
2. `CS# = 0`, tạo xung `WR#` thấp->cao.
3. Delay `~120us`.
4. `RD# = 0`.
5. `data = ppi8255_read_portB()`.
6. `RD# = 1`, `CS# = 1`.
7. Return `data`.

### 7.3 Vì sao dùng delay thay `INTR#`

Ở wiring hiện tại, tín hiệu `INTR#` chưa được đưa vào một đường input phù hợp để polling qua firmware. Vì vậy dùng delay cố định là phương án đơn giản, ổn định cho bản hiện tại.

---

## 8. Tầng service và lệnh giao thức

### 8.1 `measurement_service`

`measurement_service_read(index)`:

- kiểm tra `index < DEVICE_COUNT`;
- gọi `adc0804_read(index)`;
- trả về giá trị 8-bit.

### 8.2 Lệnh `CMD_READ_CURRENT`

- Request từ PC: `AA 03 01 [index] [checksum]`
- Response từ AVR: `AA 03 02 [index] [value] [checksum]`

`value` là dữ liệu ADC 8-bit của kênh tương ứng.

---

## 9. Luồng dữ liệu đầu-cuối

```text
PC gửi CMD_READ_CURRENT(index)
    -> UART nhận byte
    -> frame_parse_byte() hoàn tất frame hợp lệ
    -> app_task() gọi measurement_service_read(index)
    -> adc0804_read(index)
       -> Port C chọn kênh + start/read ADC
       -> Port B lấy D0..D7
    -> app_task() đóng gói frame phản hồi
    -> UART gửi về PC
```

---

## 10. So sánh với phiên bản cũ

| Nội dung | Cũ | Mới |
|---|---|---|
| Điều khiển ADC | GPIO trực tiếp ATmega (`PORTA`) | Qua `8255 Port C` |
| Đọc dữ liệu ADC | Đọc trực tiếp `PIND` | Đọc qua `8255 Port B` |
| Vai trò `Port A` 8255 | Có nguy cơ lẫn vai trò | Chỉ điều khiển tải |
| Độ đúng với wiring thực tế | Sai với mạch mới | Đúng với mạch mới |

---

## 11. Lưu ý thực tế khi triển khai

1. **Wiring phải đúng mapping bit Port C** như tài liệu.
2. Nếu đọc ADC bị nhiễu, nên:
   - thêm lọc RC ở ngõ vào,
   - lấy trung bình nhiều mẫu trong `measurement_service`.
3. Nếu cần thời gian chính xác hơn, có thể:
   - đưa `INTR#` vào đường input phù hợp,
   - đổi từ delay cố định sang polling `INTR#`.
4. Nếu sau này cần đo nhanh hơn, có thể thêm lệnh đọc batch nhiều kênh trong một frame để giảm overhead UART.
