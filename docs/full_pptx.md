# HƯỚNG DẪN THIẾT KẾ PPT CHI TIẾT TỪNG SLIDE (BẢN FULL)

> Dùng cho đề tài: **Hệ vi xử lý ATmega16 + 8255 + ADC0804 + 74HC4051** để điều khiển 8 tải và giám sát dòng điện qua UART.
> Nội dung bám theo mã nguồn hiện tại và các tài liệu trong `docs`.

---

## 0) Quy chuẩn chung cho toàn bộ deck

### 0.1 Tông màu + font
- Nền: trắng hoặc xanh rất nhạt.
- Màu nhấn:
  - Xanh dương: phần mềm
  - Cam: phần cứng
  - Đỏ: cảnh báo/lỗi
  - Xanh lá: kết quả đạt
- Font: `Segoe UI` hoặc `Calibri`
  - Tiêu đề: 34–40
  - Nội dung: 20–24
  - Chú thích hình: 14–16

### 0.2 Bố cục cố định
- Mỗi slide chỉ giữ **1 thông điệp chính**.
- Tỷ lệ 16:9.
- 1 hình chính + tối đa 3 bullet.

### 0.3 Điểm cần đúng với code hiện tại
- Frame: `[AA][CMD][LEN][DATA...][CHECKSUM]` (không ETX).
- Command:
  - `0x01`: `CMD_SET_ALL`
  - `0x02`: `CMD_SET_SINGLE`
  - `0x03`: `CMD_READ_SENSOR`
  - `0x04`: `CMD_SENSOR_DATA`
- `measurement_service_read()` dùng peak 100 mẫu.
- `ROM_SIZE` và `RAM_SIZE` hiện tại là 256 bytes.

---

## SLIDE 1 — Trang bìa

**Text đặt lên slide:**
- `Thiết kế hệ vi xử lý 8-bit giám sát dòng điện và điều khiển 8 kênh thiết bị`
- `ATmega16 + Intel 8255 + ADC0804 + 74HC4051 + UART`
- `Sinh viên: ... | GVHD: ... | Năm: ...`

**Hình cần có:**
- 1 ảnh hero board/proteus.
- 4 icon: MCU, ADC, Relay, UART.

---

## SLIDE 2 — Agenda

**Text:**
1. Bài toán và mục tiêu
2. Kiến trúc phần cứng
3. Kiến trúc phần mềm nhiều lớp
4. Giao thức và luồng dữ liệu
5. Kết quả kiểm thử
6. Kết luận và hướng phát triển

**Hình:** Sơ đồ cây 6 nhánh.

---

## SLIDE 3 — Đặt vấn đề

**Tiêu đề:** `Giải pháp thương mại mạnh nhưng chi phí cao cho bài toán 8 kênh cơ bản`

**Bullet:**
- Cần đo dòng + điều khiển tải + giám sát từ xa.
- Cần nền tảng học thuật đúng bản chất vi xử lý.
- Cần chi phí thấp, dễ mở rộng.

**Hình:** Biểu đồ cột chi phí tương đối.

---

## SLIDE 4 — Mục tiêu đề tài

**Tiêu đề:** `Đề tài giải quyết đồng thời bus, điều khiển và đo lường`

**Text:**
- M1: Bus 8-bit + memory-mapped I/O với 8255.
- M2: Điều khiển 8 tải qua Port A.
- M3: Đo dòng 8 kênh qua MUX + ADC và gửi UART frame.

**Hình:** 3 card icon.

---

## SLIDE 5 — Chỉ tiêu kỹ thuật

**Bảng nội dung:**
- `ATmega16`, `F_CPU=8MHz`
- UART mềm `9600bps`
- `8255 + ADC0804 + 74HC4051`
- `8` kênh điều khiển, `8` kênh đo
- Frame XOR checksum

**Hình:** bảng + icon linh kiện.

---

## SLIDE 6 — Kế hoạch thực hiện

**Text:**
- Driver cơ bản
- Service tầng nghiệp vụ
- Memory map
- CPU bus
- Protocol frame
- ADC measurement

**Hình:** Gantt chart 6 pha.

---

## SLIDE 7 — Kiến trúc phần cứng tổng thể

**Tiêu đề:** `Hệ thống tách 4 khối: xử lý, đo lường, chấp hành, giao tiếp`

**Hình bắt buộc:**
- `PC/ESP32 <-> UART <-> ATmega16 <-> 8255`
- `Port A -> ULN2803 -> Relay x8`
- `Port B <- ADC0804`
- `Port C -> ADC control + MUX select`
- `74HC4051 <- 8 cảm biến -> ADC0804`

---

## SLIDE 8 — Data flow đầu-cuối

**Flow text:**
1. Dòng AC qua cảm biến
2. 74HC4051 chọn kênh
3. ADC0804 chuyển đổi
4. 8255 Port B đọc dữ liệu
5. ATmega tính peak
6. Gửi frame UART

**Hình:** Flowchart 6 bước.

---

## SLIDE 9 — Mô hình đo dòng

**Tiêu đề:** `Dòng điện được biểu diễn qua độ lệch quanh mức giữa ADC`

**Công thức:**
- `diff = raw - 128`
- `peak_lsb = max(|diff|)`
- `I_peak = peak_lsb * VCC / 256 / sensitivity`

**Hình:** Sóng sin offset quanh 2.5V.

---

## SLIDE 10 — Vai trò MUX 74HC4051

**Text:**
- MUX ghép 8 đầu vào vào 1 ADC.
- A/B/C lấy từ `Port C` (`PC4/PC5/PC6`).
- Ghi đồng thời 3 bit để tránh glitch kênh trung gian.

**Hình:** sơ đồ `X0..X7 -> COM`.

---

## SLIDE 11 — Chuỗi điều khiển ADC0804

**Text:**
1. `CS#=0`
2. `WR# 0->1` start conversion
3. Delay ~`120us`
4. `RD#=0`, đọc `Port B`

**Hình:** timing diagram `CS#/WR#/RD#`.

---

## SLIDE 12 — 8255 PPI

**Tiêu đề:** `8255 phân vai rõ ràng cho điều khiển và đo lường`

**Text:**
- `Port A`: output điều khiển relay
- `Port B`: input đọc ADC
- `Port C`: output điều khiển ADC + chọn MUX
- `Control word`: `0x82`

---

## SLIDE 13 — Khối relay công suất

**Text:**
- Port A chỉ xuất logic.
- ULN2803 kéo dòng coil relay.
- Diode dập xung bảo vệ.

**Hình:** sơ đồ 1 kênh relay nhân x8.

---

## SLIDE 14 — Kiến trúc phần mềm phân lớp

**Text:**
- `main -> app -> protocol/services -> core -> drivers -> bsp/config`

**Hình:** layer diagram.

---

## SLIDE 15 — Cấu trúc thư mục

**Text hiển thị:**
- `app/`, `bsp/`, `config/`, `core/`, `drivers/`, `protocol/`, `services/`, `main.c`

**Hình:** cây thư mục.

---

## SLIDE 16 — Super-loop

**Text:**
- `app_init(); while(1){ app_task(); }`
- Init một lần, task lặp vô hạn.

**Hình:** flow vòng lặp.

---

## SLIDE 17 — State machine parser

**Text:**
- State: header -> cmd -> len -> data -> checksum.
- Checksum XOR để phát hiện lỗi.

**Hình:** state diagram.

---

## SLIDE 18 — Command protocol trong code

**Bảng:**
- `0x01` set all
- `0x02` set single
- `0x03` read sensor
- `0x04` sensor data (9 byte payload)

**Ví dụ frame:**
- `AA 01 01 FF 55`

---

## SLIDE 19 — `device_service`

**Text:**
- Trạng thái 8 thiết bị lưu bằng bitmask `uint8_t`.
- `turn_on/off/set_all/get_state`.
- Mỗi cập nhật ghi xuống `Port A`.

**Hình:** byte map bit0..bit7.

---

## SLIDE 20 — `measurement_service`

**Text:**
- `PEAK_SAMPLE_COUNT = 100`
- `ADC_ZERO_OFFSET = 128`
- Trả giá trị peak LSB theo kênh.

**Hình:** đồ thị điểm mẫu + peak.

---

## SLIDE 21 — `adc0804.c` và `portc_shadow`

**Text:**
- Shadow giữ trạng thái Port C.
- Chỉ sửa bit cần thiết.
- Ghi lại 8255 sau mỗi thay đổi.

**Hình:** bitfield `PC6..PC0`.

---

## SLIDE 22 — `ppi8255.c` read/write cycle

**Text:**
- Write: set data bus output, pulse `WR#`.
- Read: set data bus input, kéo `RD#`, đọc `PIND`.
- `A0/A1` chọn thanh ghi 8255.

**Hình:** timing mini read/write.

---

## SLIDE 23 — Memory map thực thi

**Text:**
- `0x0000–0x00FF`: ROM (`PROGMEM`)
- `0x4000–0x40FF`: RAM
- `0x8000–0x8003`: 8255 I/O

**Hình:** dải địa chỉ màu.

---

## SLIDE 24 — `cpu_bus` + `io_map`

**Text:**
- CPU chỉ gọi `cpu_bus_read/write(addr)`.
- `memory`/`io_map` phân tuyến tới RAM/ROM/8255.
- Ví dụ: `0x8001 -> ppi8255_read_portB()`.

**Hình:** call chain arrows.

---

## SLIDE 25 — Luồng `app_task()`

**Text:**
- Poll UART, parse frame, xử lý command.
- Trả `ACK/NACK`.
- Push định kỳ `CMD_SENSOR_DATA`.

**Hình:** flow 2 nhánh receive + push.

---

## SLIDE 26 — Đối chiếu trạng thái và dòng

**Text:**
- Payload: `I0..I7 + state`.
- Ma trận logic:
  - ON + có dòng: bình thường
  - ON + không dòng: lỗi
  - OFF + không dòng: bình thường
  - OFF + có dòng: cảnh báo

**Hình:** bảng 2x2.

---

## SLIDE 27 — Công cụ test `tool.ps1`

**Text:**
- Lệnh: `all`, `single`, `status`, `monitor`, `raw`.
- Tự tạo frame XOR checksum.
- Thuận tiện test nhanh end-to-end.

**Hình:** screenshot terminal.

---

## SLIDE 28 — Kết quả kiểm thử

**Text:**
- Relay phản hồi đúng lệnh.
- Frame parse ổn định.
- Sensor data gửi đều theo chu kỳ.

**Hình:** ảnh bench/proteus + biểu đồ thời gian.

---

## SLIDE 29 — Hạn chế và nâng cấp

**Text:**
- Hạn chế: UART mềm, ADC 8-bit, chưa RTOS.
- Hướng nâng cấp: USART cứng, ADC cao hơn, lọc RMS nâng cao, IoT gateway hoàn chỉnh.

**Hình:** bảng `Current vs Next`.

---

## SLIDE 30 — Kết luận & Q&A

**Text:**
- `Đề tài đã hiện thực chuỗi đo lường–điều khiển đầy đủ trên nền kiến trúc vi xử lý nhiều lớp.`
- `Xin cảm ơn Hội đồng — Q&A`

**Hình:** nền board + icon hỏi đáp.

---

## PHỤ LỤC — Checklist trước khi nộp

- [ ] Không ghi nhầm `CMD_READ_CURRENT`; đúng là `CMD_READ_SENSOR`.
- [ ] Không thêm `ETX` vào frame mô tả.
- [ ] Memory map trình bày đúng bản code hiện tại (ROM/RAM 256B).
- [ ] Mọi pin mapping khớp `board.h`.
