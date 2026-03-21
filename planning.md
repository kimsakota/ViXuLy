# Phân tích Chi tiết Dự án Vi điều khiển Điều khiển IC 8255 qua UART

Tài liệu này giải thích cực kỳ chi tiết về kiến trúc, chức năng của từng file code trong dự án và luồng hoạt động tổng thể của hệ thống.

## 1. Phân tích chi tiết từng file source code

### 1.1 Thư mục `config/`
- **`system_config.h` (Cấu hình hệ thống)**
  - File này định nghĩa các hằng số cấu hình chung cho toàn bộ dự án dưới dạng Macro (`#define`).
  - `F_CPU 8000000UL`: Khai báo tần số hoạt động của thạch anh/xung nhịp vi điều khiển là 8 MHz. Thông số này cực kỳ quan trọng để các hàm tạo trễ như `_delay_us()` hoạt động chính xác theo thời gian thực (đặc biệt cho tốc độ baudrate của Software UART).
  - `UART_USE_SOFT 1`: Cờ báo hiệu sử dụng chế độ Software UART (UART bằng phần mềm thay vì phần cứng) cho dự án.
  - `UART_BAUDRATE 9600`: Tốc độ truyền nhận baud rate quy định là 9600 bps.
  - Ngoài ra còn một vài define như `FRAME_HEADER`, `FRAME_MAX_PAYLOAD` và `DEVICE_COUNT` đã được tạo để chuẩn bị thiết kế mở rộng protocol sau này.

### 1.2 Thư mục `bsp/` (Board Support Package - Gói Định nghĩa bo mạch)
- **`board.h` (Khai báo phần cứng kết nối)**
  - Chức năng: Ánh xạ việc giao tiếp ngoại vi bằng code tới các chân vật lý thực tế của vi điều khiển AVR (`#include <avr/io.h>`). Nếu có thay đổi kết nối dây trên phần cứng mạch in cứng, nhà phát triển cơ bản chỉ cần thay đổi số liệu trong file này.
  - **Module IC 8255 PPI (IC Mở rộng Cổng):**
    - **Bus Dữ liệu (8-bit Data Bus)** được kết nối trực tiếp với **Port D** của vi điều khiển (`DDRD`, `PORTD`).
    - **Các chân Điều khiển (Control Bus)** được điều khiển qua **Port B** (`DDRB`, `PORTB`).
    - Các tín hiệu riêng biệt nối tương ứng: `PPI_WR_PIN` (Ghi - PB0), `PPI_RD_PIN` (Đọc - PB1), cấu hình vùng nhớ địa chỉ lệnh `PPI_A0_PIN` (PB2), `PPI_A1_PIN` (PB3) và kích hoạt Chọn Chíp chip-select `PPI_CS_PIN` (PB4).
  - **Module UART Mềm (Software UART):**
    - Sử dụng **Port C** của vi mạch.
    - Kênh truyền chân phát `UART_TX_PIN` là PC0, chân nhận chờ đọc `UART_RX_PIN` là PC1.

### 1.3 Thư mục `drivers/`
- **`uart.h` & `uart.c` (Driver giao tiếp Software UART)**
  - Xây dựng chuẩn truyền thông nối tiếp thông qua việc "bit-banging" (Sử dụng kỹ thuật lập trình GPIO nhấp nhả mức logic có tính toán thời gian thay vì phần cứng UART UART module tích hợp chuyên dụng).
  - `uart_init(void)`: Hàm cấp độ thấp, định cấu hình TX là ngõ ra (Output), RX là ngõ vào (Input). Đặt trạng thái TX lên mức Cao mặc định (High) làm tín hiệu chờ trạng thái rỗi tĩnh (Idle signal condition) của chuẩn USART.
  - `uart_write_char(char c)`: Hàm đẩy 1 byte dữ liệu c. Đầu tiên báo hiệu bằng 1 Start bit (Mức dòng kéo xuống Low), sử dụng bitwise logic trượt giá trị lặp qua 8 bit dữ liệu tuần tự gửi, và kết thúc chốt bằng 1 Stop bit (High). Khoảng thời gian delay tính toán chuẩn cho thiết lập giữa các bit là `_delay_us(104)`. Thời lượng này tuân theo công thức: 1 giây / 9600 baud = 104.16 µs/bit truyền.
  - `uart_write_string(const char* s)`: Vòng lặp gửi đi văn bản text string chuỗi tuần tự từng kí tự một.
  - `uart_read_char(char* out)`: Đọc byte đơn. Nó là một hàm đọc không chặn (Non-blocking mode). Nó kiểm tra xem chân RX có rớt xuống Low biểu thị Start bit hay không. Nếu có, delay chờ `52µs` để lấy mẫu ở giữa của bit để tránh gai nhiễu điện (noise glitch), sau đó dịch mẫu tuần tự tiếp tục delay đợi đọc 8 bit và cuối cùng gắn dữ liệu trả về biến con trỏ.

- **`ppi8255.h` & `ppi8255.c` (Driver điều khiển IC mở rộng I/O Intel 8255)**
  - Thư viện C cho phép giao tiếp ra lệnh và ghi Data cho thiết bị ngoại vi chíp Intel 8255 mở rộng cổng.
  - `set_addr(uint8_t addr)`: Thiết lập cấu hình vật lý Address Selector A0 và A1 bằng việc ép kiểu mức logic điều khiển tùy thuộc vào biến số `addr`. Việc cấu hình địa chỉ nhắm mục tiêu chuẩn xác vào các cổng bên trong IC (Port A, B, C hoặc thanh ghi ghi dịch lệnh Command Register Control).
  - `write(uint8_t addr, uint8_t data)`: Lệnh gốc (Raw Write Instruction) cho 8255. Ghi mã nhị phân Data data buffer ra Data bus (Port D của vi điều khiển), rồi set chân định tuyến bộ đệm CS xuống làm cho chip cho phép viết. Sau đó cắn xung Write Low chân `PPI_WR_PIN`, làm trễ chờ `_delay_us(5)` cho 8255 kịp cập nhật chuỗi lên thanh ghi, rồi kéo lại logic High đóng nhả lệnh. Cuối cùng thả cấm Chip CS.
  - `ppi8255_init(void)`: Cấu hình hướng xuất nhập ban đầu. Set tất cả Port B điều khiển 8255 và Data Bus 8255 (DDRD = 0xFF tức output 100%). Sau đó đặt xung Idle cho chip. BƯỚC QUAN TRỌNG: Gọi thủ tục `write(3, 0x80)` gửi lệnh Command Byte mã hex `0x80` vào Register Control Address 3 -> **Định cấu hình cơ bản cho 8255 ở Mode 0 (Chế độ Cơ bản Basic Input/Output), đặt sẵn tất cả các Port A, B, C xuất tín hiệu là ngõ ra phát Output**.
  - `ppi8255_write_portA(uint8_t value)`: Lệnh thân thiện trừu tượng (API func). Cụ thể hóa đặc tả việc gửi Data giá trị cụ thể trỏ trực tiếp nhắm bắn riêng biệt cổng Port A bằng hàm ghi write với port destination Address là 0.

### 1.4 Thư mục `app/` (Lớp Ứng dụng - Application Layer)
- **`app.h` & `app.c`**
  - Chứa thuật toán Logic vận hành điều khiển trung tâm toàn bộ thiết bị dự án.
  - `app_init(void)`: Hàm setup ban đầu, gọi lên 2 constructor init cho driver phụ trợ UART và IC 8255. Đồng thời gửi ra màn terminal kết nối chuỗi `"System Ready\r\n"` xác nhận module bắt đầu thành công sẵn sàng nhận lệnh tiếp theo.
  - `app_task(void)`: Task thực thi nằm trong hàm điều hành trung tâm sử dụng kỹ thuật "Non-blocking delay Polling loop" lặp chạy siêu tốc chia phần ảo chức năng thời vụ theo dạng Multitasking đơn thuần:
    1.  **Vòng lặp trỏ Polling xử lý đọc lệnh Terminal Command Console - Đợi khoảng mô phỏng ~500ms:** Có hai vòng for() lồng ghép nhau duyệt qua biến i, j với tổng chu kỳ chạy `500 x 100 = 50.000` lần. Mỗi khoảng trống chờ `10us`. Hàm giả lập việc vừa tạo biến dừng ~500ms vừa **"Cảnh sát Liên tục Lắng Nghe - Interrupt/Polling" đường TX/RX thông qua software UART:**
        - Nếu có dữ liệu phím gõ được gửi vào biến đệm đụng `uart_read_char()`, hệ thống decode lệnh ngay lập tức dựa trên Ascii Kí số:
        - Nhập số `'1'`: Xuất byte mã toàn số điện áp High (`0xFF` / `11111111`) cho Port A IC 8255 (Lệnh dùng để Bật sáng hết tất cả 8 thiết bị/LED gắn vào). Reply lại UART cho người dùng text `"ALL ON"`.
        - Nhập số `'0'`: Tắt toàn bộ (`0x00`) cho Port A. Reply báo `"ALL OFF"`.
        - Nhập phím `'A'`: Gửi byte cấu hình `0xAA` (nhị phân 10101010, thường dùng để định danh trạng thái LED chớp giãn cách sáng kẽ). In log hiển thị `"AA"`.
        - Nhập phím `'B'`: Gửi byte xoay chiều phiên mã `0x55` (nhị phân 01010101, sáng so le luân phiên nghịch chân). In log text phản hồi `"55"`.
    2.  **Khối chức năng Effect Chớp tắt LED tĩnh (Blink Demo):** Quá trình thứ 2 diễn ra kế tiếp mỗi khi timer rỗng 500ms bên trên đi hết vòng, hàm truy xuất và lật chốt biến tĩnh bộ máy trạng thái `static uint8_t state`. Ban đầu `state=0`, nó truyền mã `0xAA` cho Cổng A của chíp 8255 và tăng state lên số 1. Sang lượt chu kỳ của 0.5s tiếp theo sau quá trình chờ Polling, vì `state` giờ là trạng thái 1, block else được nạp mã `0x55` cho Port A và giáng `state` phục hồi trở về lại số 0. Code tiếp tục tạo **Một Effect chuyển đổi trạng thái chớp đảo ngược sáng chân chẵn/lẻ tự động chu kỳ chớp ngầm mỗi 500ms một lần**.

### 1.5 Cấu trúc tổng thư mục gốc `main.c`
- Hàm chuẩn khởi động. Nó gọi duy nhất định danh `app_init()` cho việc cấu hình vi điều khiển ban đầu và thả ứng dụng chính vòng xoáy Event điều khiển vô tận bằng lệnh `while(1) { app_task(); }` liên tục lặp luân hồi mãi mãi. Đây là cấu trúc vòng lặp điều phối "Super Loop Pattern" chuẩn chỉ rất phổ biến tại các vi hệ thống Nhúng Embedded System đơn nhân.

---

## 2. Tổng kết Chức năng Tóm lược Hiện tại của Dự án C-Code:
Dự án được viết cho kiến trúc phần mềm vi điều khiển thông dụng (dòng chip MCU AVR) sử dụng để mở rộng Cổng/LED giao tiếp các ngoại vi Input/Output thiết bị qua bộ mở rộng kênh dữ liệu nổi tiếng IC Intel 8255 PPI, chạy xung nhịp cơ sở thiết lập là 8MHz. Toàn bộ thiết bị cho phép kỹ sư/người sử dụng có thể thao tác với bo mạch từ phần mềm trên trình ảo hóa máy tính (Terminal Command Line Emulator) tốc độ baud standard (9600 bps) thông qua driver "Software UART" được lập trình trực tiếp bằng code do không dùng phần cứng rảnh rỗi trên MCU thay thế cho UART gốc.

**Dự án có 2 luồng chức năng lớn chạy đồng thời cùng lúc (giả lập đa luồng):**

Mặc định trên hệ thống thiết lập Port A của thiết bị 8255 kết nối xuất tín hiệu đèn LED, chương trình có 2 tính năng như sau:
1.  **Quá trình Chớp/Tắt LED định kỳ tự hành (Auto-Blinker State Machine Effect):** Cứ duy trì sau một khoảng vòng trễ không cố định khoảng 0.5 Giây, chương trình lại chớp đổi hiệu ứng. Nó tạo ra quá trình truyền nhị phân đính giá trị nhấp nháy luân phiên qua lại hai mã `0xAA` (Mã đèn sáng dồn xen kẽ 10101010) và giá trị `0x55` (Mã đèn sáng đổi bù trừ 01010101) ra module đèn LED đang cắm trên ngõ điều ra cổng xuất I/O ngoài. Tạo nên hiệu ứng dàn chân LED chớp nháy so le rất đẹp mắt.
2.  **Kênh giám sát giao tiếp Tương tác Chờ thời gian thực (Remote Operator Controllable Event):** Ngay trong qua trình đếm lùi quá trình ở trên, thì ứng dụng vẫn đồng hành kiểm duyệt kênh "lắng nghe bằng tai" sóng UART cho lệnh của người bấm. Người vận hành chỉ cần dùng terminal phím bàn phím đài chỉ huy quăng chữ/số điều khiển, có thể **Mép chèn chặn ngang (Interrupt / Override)** để ép đổi tín hiệu trực tiếp ngắt bỏ dòng luồng chớp tắt. Giúp người dùng đóng/nháy mở toàn Port (Tất các ngõ) `1` hoặc tắt ngẫu nhiền tắt mịt mù ngõ bằng `0`, ép hiện hiệu ứng tĩnh bất biến nào tùy thích `A`, `B`. Tuy nhiên, sau khi ra lệnh thủ công thì ngay tại chu kỳ ~0.5s tiếp nối sau ứng dụng lại quay lại hiệu ứng chớp tắt mặc định Auto Blink! Lặp mãi mãi.

## 3. Phân tích Chi tiết Logic cấp độ mã nguồn của các hàm (.c files)

### 3.1 `app.c` - Logic vòng lặp chính
**Hàm `app_task()`:**
- **Giải thuật Polling Delay (Tạo trễ không chặn - Non-blocking):** Hàm sử dụng 2 vòng lặp `for` lồng nhau. Vòng ngoài chạy 500 lần (`i`), vòng trong chạy 100 lần (`j`). Ở mỗi lần lặp nhỏ nhất của vòng trong, chương trình sẽ gọi hàm `uart_read_char(&c)` để kiểm tra "chớp nhoáng" xem có ký tự nào được truyền vào thanh ghi đệm từ terminal hay không. Ngay sau đó là lệnh dừng chỉ vỏn vẹn `_delay_us(10)`.
  - Tổng thời gian mô phỏng một chu kỳ "đứng yên": `500 * 100 * 10 µs = 500,000 µs = 500 ms = 0.5s`.
  - Phân tích sâu: Thay vì dùng lệnh `_delay_ms(500)` trực tiếp sẽ làm khối hệ thống vi điều khiển "đứng hình" bất động (Blocking) khóa toàn bộ các hàm nhận UART trong đúng 0.5 giây đó, thì kĩ thuật phân nhỏ và nhồi truy vấn I/O này giúp chương trình vừa có khả năng delay chính xác, lại vừa liên tục "cảnh giác" ngóng nghe bất kì tương tác nào do người sử dụng ấn, với độ đo kiểm soát đến từng chu kì 10 micro giây quét phím.
- **Phân nhánh lệnh (Switching/If-else):** Ngay trong vòng for con, khi `uart_read_char` bắt thành công dữ liệu phím, nó trả ra `true` và chạy vào cây điều kiện `if (c == ...)`. Mã lệnh sẽ gọi trực tiếp module `ppi8255_write_portA(mã_hex)` xuất tín hiệu lập tức, xen ngang vòng lặp tự động. Đồng thời xác nhận chuỗi phản hồi trên màn hình (`uart_write_string()`).
- **Máy trạng thái chớp tắt tĩnh (Blink State Machine):** Khai báo một biến tĩnh (`static uint8_t state = 0;`). Nhờ từ khóa `static`, bộ nhớ biến `state` không bị Reset mất dữ liệu mỗi lần hàm lặp vòng mới ở hàm `while(1)` trong `main.c`. Cứ sau khi thoát hết 500ms delay của 2 vòng for ở trên, mã lệnh sẽ soi xét giá trị `state`. Nếu `state` đang là `0` thì gửi tín hiệu nháy mã nhị phân xen kẽ `0xAA` qua PortA rồi lật nhích `state=1`. Nếu không (đang là 1), thì phát bù tín hiệu logic mã `0x55` và kéo thụt `state` về lại `0`.  

### 3.2 `uart.c` - Kỹ thuật Software UART chuyên sâu (Bit-Banging)
**Hàm `uart_init(void)`:**
- Khởi tạo giao thức vật lý hai chân truyền nối (`TX_PIN`) chiều ra (Output) và chân thu thập nối tiếp (`RX_PIN`) chiều vô (Input).
- Khác biệt: Trong MCU AVR, để tiết kiệm linh kiện ngoài, `UART_PORT |= (1 << UART_RX_PIN)` khi Pin cấu hình làm Input sẽ kích hoạt điện trở kéo lên cấu hình ẩn bên trong chíp (Internal Pull-up Resistor), điều này giữ dòng RX luôn ở mức logic 1 điện áp cao tĩnh để khử bỏ hiện tượng nhiễu Floating mạch. TX cũng được kéo High đặt cấu hình mặc định là chờ nhàn rỗi (Idle state của chuẩn UART).

**Hàm `uart_write_char(char c)`:**
- **Khởi tạo truyền (Start bit):** Để đánh tiếng báo hiệu bắt đầu khung truyền mới, chương trình chủ động ghì mức áp trên vi điều khiển qua chân `TX_PIN` xuống mức 0 (Low). Phải theo sau đó là hàm delay `104us`. Tức là duy trì trạng thái đánh điện chìm này y hệt 1 khoảng bit theo độ rộng (`1 giây / 9600 baud = ~104.16 µs/bit rộng`).
- **Mã hóa (Data frame payload):** Sử dụng vòng lặp 8 lần ứng với bit 0 đến bit 7. Kĩ thuật `c & 0x01` trích xuất mặt nạ bắt giữ bit đứng thấp nhất (LSB). Nếu nó mang kết quả 1 thì đẩy chân `TX` lên High và nếu là 0 thì ghì nó xuống Low (`UART_PORT &= ...`). Ngay sau việc duy trì áp 104us này, biến cấu trúc bị xê dịch (`c >>= 1` dịch bit phải) để nhồi nhét vòng lặp bit tiếp theo đẩy vào mảng trích xuất cho đợt xử lý kế tiếp.
- **Chốt Frame (Stop bit):** Kết thúc rào khuôn với việc nhả ngõ đẩy mức áp lên High trở lại kéo dài 104us coi như băm thành một khung truyền hoàn hảo gửi đi.

**Hàm `uart_read_char(char* out)`:**
- **Bắt mạch (Start bit detection):** Vừa vào hàm là rà xét mạch ngay. Chỉ kiểm tra nhẹ xem chân `RX_PIN` có đang đứng đầu sóng High rớt xuống điểm Low hay không.  Nếu không rớt, `return false` thoát luôn trả tài nguyên vòng lặp CPU cho app chính (Luồng Không tắc nghẽn / Non-blocking). 
- **Khử nhiễu lấy mẫu chẵn lẻ theo sườn (Half-bit delay compensation):** Nếu thấy Low, không lấy liền mà Delay một nhịp 52us (nửa chu kỳ tín hiệu - do full là 104us). Việc cố ý đợi `52us` mục đích là dịch chuyển nhảy cóc vào chẻ điểm đọc rơi đúng vào trung tâm chính giữa của thành Start Bit rồi mới thực hiện đo trắc lại. Nếu đo lại lúc này mà mạch đột nhiên dựng ngược về High thì kết luận là rác nhiễu Noise/Glitch điện thế, nếu đúng Low thì chuẩn tín hiệu Start! Nó sẽ giúp chống đứt chân truyền và đọc nhầm nhiễu cực kì hiệu quả.
- **Tiêu hóa dữ liệu (Sampling):** Đợi tiếp `104us` cho điểm giữa chừng bụng bit đầu tiên. Lấy mẫu qua 8 vòng lặp, dữ liệu mới sẽ được dập nén vào qua phép dịch `data >>= 1`. Nếu chân `RX_PIN` trả mức điện cao, mã chốt thêm `data |= 0x80` dập con số `1` vào mã nhị phân ở vị trí tận cùng MSB (Most Significant Bit - vì tín hiệu UART truyền LSB trước, nên cứ đọc xong thì nhét nó vào đầu trái cao nhất rồi dịch nó chầm chậm lùi về lấp đầy bên phải của Byte). Tiếp tục trễ thêm 104us cho móc bit tiếp theo đến khi nhận đủ byte.
- Hàm đóng gói kết quả thu lượm trả về thông qua mảng vùng bộ nhớ con trỏ `*out = data`.

### 3.3 `ppi8255.c` - Vi điều khiển cấu trúc Data Bus ngoại vi
**Hàm `write(uint8_t addr, uint8_t data)`:**
- Đây là hàm cơ sở hạ tầng nền móng cấp thấp nhất để nhét nhị phân vào thanh ghi IC 8255 qua cổng vật lý.
- `PPI_DATA_PORT = data`: Bước đổ sập giá trị mã 8-bit Data một lượt vào toàn bộ 8 chân (PORTD được khai làm 8 đường DataBus D0-D7).
- `set_addr(addr)`: Hàm Helper Set địa chỉ chíp tĩnh, gọi tác động các chân định tuyến chọn cổng đích `A0` và `A1` gạt áp dựa vào đối số truyền `addr`. Nếu truyền vào port 0 (`A0=0`, `A1=0`), IC hiểu dữ liệu nhắm tới Port A. Phân tách port 3 (`A0=1`, `A1=1`) sẽ nện trực chỉ đánh vào Control Register chứa mã lệnh chỉ đạo sinh sát quy trình của con 8255.
- **Ban hành Lệnh Thời gian thực (Control Bus Sequence Timing):**
  - Mặc định RD kéo lên 1 cấm đọc. Kéo `CS_PIN` (Enable Chip Select) về mốc sóng Low kích hoạt thức tỉnh con IC báo có kẻ giao tiếp.
  - Sập mức Low cho chân `WR_PIN` (Write Strobe) - ra lệnh Ghi trạng thái cho IC 8255. Lúc này chip sẽ mở cổng hút thông tin trên mâm Data Bus.
  - `_delay_us(5)`: Thêm một khoảng thời gian chờ neo trễ nhỏ tí tẹo (Delay 5 micro giây). Tại sao phải cần? Do IC thế hệ 8255 là kiến trúc vi mạch bán dẫn thời cổ (mã logic TTL) nó yêu cầu 1 độ trễ xung nhịp vật lí khoảng "vài trăm nano giây" theo datasheet để kịp copy và "thấm" lưu mẫu Data an toàn trên Bus vào Core, không có dòng lệnh tạo chênh lệch cực nhanh của MCU AVR siêu tốc sẽ làm mất nhịp lệch bit không nhận diện nổi lệnh.
  - Hoàn tất khóa van đẩy lệnh lại High cho WR. Kết thúc nhả Cấm chíp `CS`. Toàn bộ dữ liệu Data Bus được IC nhận gọn gàng.
