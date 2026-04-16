# KHUNG NỘI DUNG THUYẾT TRÌNH (>= 30 SLIDES)

**Môn học:** Kỹ thuật Vi xử lý & Technical Writing and Presentation (Đại học Bách Khoa Hà Nội)
**Đề tài:** Ứng dụng hệ vi xử lý AVR (ATmega16) và các IC ngoại vi trong giám sát dòng điện 8 kênh & điều khiển thiết bị IoT

> **Cam kết tuân thủ 3 Laws (Technical Writing HUST):**
> 1. **Clear Message (Assertion-Evidence):** Tiêu đề là một câu khẳng định thông điệp ("Slide X: Cảm biến dòng xuyến trích xuất dòng điện tuyến tính...").
> 2. **Cấu trúc dạng cây (Tree-based Structure):** Top-down mạch lạc tuân theo 9 bước chuẩn của Báo cáo đồ án.
> 3. **Trực quan hóa (Visual Evidence):** Thay thế chữ bằng Đồ thị (Graph), Sơ đồ (Diagram), Phương trình (Equation), Lưu đồ (Flowchart), Gantt Chart.

---

## [SLIDE CHUYỂN PHẦN 1] MỞ ĐẦU - PHÂN TÍCH ĐỀ TÀI VÀ KẾ HOẠCH (Tương ứng Bước 1 & 2)
*(Ghi chú: Slide này chỉ để 1 tiêu đề lớn lùi giữa màn hình để Hội đồng biết luồng thuyết trình bắt đầu đi vào Phần 1)*

**SLIDE 1: Trang bìa (Title Slide)**
- **Tiêu đề:** Nghiên cứu và Thiết kế Hệ thống Vi xử lý ứng dụng trong Giám sát dòng điện và Điều khiển đa kênh.
- **Phụ đề:** [Hình ảnh] Ảnh ngoại quan sản phẩm thực tế và Logo HUST.
- **Thông tin:** Tên Sinh viên, MSV, Tên Giảng viên.

**SLIDE 2: Nội dung trình bày (Agenda)**
- **Nội dung:** [Diagram] Sơ đồ cây (Tree-diagram) 4 hạng mục cốt lõi:
  1. Đặt vấn đề và Mục tiêu
  2. Thiết kế Phần cứng
  3. Lập trình Phần mềm
  4. PCB và Kiểm thử

**SLIDE 3: Chi phí và độ phức tạp hạn chế việc mở rộng giám sát năng lượng trong công nghiệp (Motivation)**
- **Nội dung:** [Graph] Biểu đồ cột/đường so sánh chi phí và số kênh đo của các giải pháp hiện tại vs thiết bị chuyên dụng PLC.

**SLIDE 4: Giải pháp dùng hệ vi xử lý (ATmega16 + 8255) tối ưu hóa tài nguyên IO với chi phí < 500.000đ (Objectives)**
- **Nội dung:** Khẳng định 3 mục tiêu cốt lõi: Thiết kế hệ vi xử lý chuẩn với Bus dữ liệu ngoài, thu thập 8 kênh dòng điện, và truyền số liệu qua UART/IoT.

**SLIDE 5: Chỉ tiêu kỹ thuật định cỡ không gian thiết kế của phần cứng và luồng dữ liệu (Specifications)**
- **Nội dung:** [Table] Bảng chỉ tiêu - Số kênh, Phân giải (8-bit ~0.04A/bước), Giao thức kết nối, Chu kỳ lấy mẫu định kì (10Hz).

**SLIDE 6: Kế hoạch thực hiện dự án được quản trị rõ ràng đảm bảo tiến độ mô hình thác nước**
- **Nội dung:** [Gantt Chart] Biểu đồ tiến độ mô tả thời gian phân bổ: (Khảo sát lý thuyết -> Thiết kế phần cứng -> Phương pháp phần mềm -> Code hệ thống -> Kiểm thử).

---

## [SLIDE CHUYỂN PHẦN 2] XÂY DỰNG SƠ ĐỒ KHỐI VÀ DATA FLOW (Tương ứng Bước 3)
*(Ghi chú: Tiêu đề lớn giữa màn hình "Thiết kế Kiến trúc Phần cứng Vi xử lý" để báo hiệu chuyển mục 2)*

**SLIDE 7: Kiến trúc tổng thể được phân cấp thành 4 Lớp chức năng phần cứng (Architecture)**
- **Nội dung:** [Diagram] Sơ đồ khối hệ thống (Từ nguồn cấp tĩnh, Lớp thu thập - Sensor, Lớp xử lý trung tâm MCU+8255, Lớp truyền động (Relay), tới Lớp điện toán đám mây).

**SLIDE 8: Luồng dữ liệu dịch chuyển từ thế giới vật lý thành dữ liệu số trên Đám mây (Data Flow)**
- **Nội dung:** [Flowchart] Dòng chảy: Dòng AC vật lý -> Điện áp Analog -> Số hóa 8-bit (D) -> Khung truyền UART (Frame) -> Gói tin JSON/MQTT.

---

## [SLIDE CHUYỂN PHẦN 3] THIẾT KẾ CHI TIẾT VÀ MẠCH NGUYÊN LÝ (Tương ứng Bước 4 & 5)
*(Ghi chú: Tiếp tục nhấn mạnh phần cứng ngoại vi ghép nối với hệ thống vi xử lý ATmega16)*

**SLIDE 9: Cảm biến dòng xuyến trích xuất dòng tĩnh với hệ số 185mV/A (Sensing Unit)**
- **Nội dung:** [Equation] $I_{measure} = (V_{out} - 2.5) / 0.185$. Biểu diễn đồ thị độ dốc [Graph].
- **Key point:** Cách ly Galvanic giữ an toàn tuyệt đối cho board điều khiển.

**SLIDE 10: IC MUX 74HC4051 triệt tiêu nút thắt cổ chai về tài nguyên chân ADC (Multiplexing)**
- **Nội dung:** [Diagram] Ghép nối phân kênh 8 đường vào COM thông qua tín hiệu A,B,C từ vi điều khiển.

**SLIDE 11: Mạch DAC nội tại duy trì xung nhịp cho tiến trình chuyển đổi của ADC0804 (ADC Conversion)**
- **Nội dung:** [Equation] Chu kì $T \approx 1.1 \times R \times C$ cùng giản đồ lấy mẫu theo các bậc [Graph].

**SLIDE 12: Cụm ngoại vi 8255 PPI mở rộng năng lực xử lý song song cho ATmega16**
- **Nội dung:** [Diagram] Data Bus kết nối PORTD, Control Bus (RD, WR, CS) qua PORTB.

**SLIDE 13: Relay phân ly vùng điều khiển với tải 10A tránh xung nhiễu (Actuation)**
- **Nội dung:** [Diagram] Mạch ULN2803 và đi-ốt dập xung điện cảm (Flyback Diode) triệt tiêu việc nhiễu Reset MCU.

---

## [SLIDE CHUYỂN PHẦN 4] KIẾN TRÚC PHẦN MỀM VÀ KỸ THUẬT LẬP TRÌNH ĐIỀU KHIỂN (Tương ứng Bước 6 - Firmware Highlight)
*(Ghi chú: Tiêu đề lớn giữa màn hình "Kỹ thuật Lập trình và Tổ chức luồng dữ liệu" báo hiệu hệ thống đã hoàn thiện phần cứng và sang phần viết code Vi xử lý)*

**SLIDE 14: Mã nguồn C nhúng áp dụng mô hình Đa tầng (Layered Architecture) tiêu chuẩn công nghiệp**
- **Nội dung:** [Diagram] Trực quan hóa cây thư mục dự án:
  1. **Core & BSP:** Xử lý Hardware-level (`cpu_bus.c`, `board.c`)
  2. **Drivers:** Điều khiển vi mạch rời (`ppi8255.c`, `adc0804.c`)
  3. **Services & Protocol:** Logic tính toán & Giao thức (`measurement_service.c`, `frame.c`)
  4. **App:** Kịch bản luồng chính (`app.c`)
- **Key point:** Triệt tiêu hoàn toàn cách viết cục bộ "Spaghetti Code".

**SLIDE 15: Kỹ thuật phân gian bộ nhớ (Memory-Mapped I/O) mở rộng ngoại vi theo chuẩn kiến trúc Vi xử lý**
- **Nội dung:** [Diagram] Phân bổ Memory Map (thể hiện trong `core/memory.c`). Vi xử lý (Core) giao tiếp trực tiếp với không gian địa chỉ của 8255, ADC qua Address/Data Bus & Control Bus thay vì I/O Port thông thường.

**SLIDE 16: Ứng dụng ngắt ngoài (External Interrupt) tối ưu hóa chu kỳ lấy mẫu ADC và tiết kiệm CPU**
- **Nội dung:** [Flowchart] Lưu đồ tuần tự trong `measurement_service.c`: VXL Set MUX -> Trigger WR của ADC -> CPU làm việc khác -> Chờ ngắt ngoài (INTR) -> Đọc giá trị Dòng điện $\rightarrow$ Chuyển kênh. CPU không bị block vòng lặp chờ.

**SLIDE 17: Giản đồ định thời (Timing Diagram) rập khuôn tốc độ lấy mẫu bắt buộc của phần cứng**
- **Nội dung:** [Graph] Sự tương quan thời gian độ trễ đóng MUX (Delay) với độ dài chu kì ADC biến đổi. Tránh xung đột luồng thu thập.

**SLIDE 18: Bộ lọc Trung bình động (Moving Average) bằng phần mềm cứu vớt nhiễu lượng tử ADC**
- **Nội dung:** [Equation & Graph] Lấy số kỳ $I_{filtered} = \frac{1}{N} \sum I_{raw}$. Trực quan độ mượt trên biểu đồ giá trị trước và sau khi đi qua bộ vi lọc.

**SLIDE 19: Dữ liệu truyền tải được cấu trúc thành Framework hoàn chỉnh bảo vệ bằng CheckSum**
- **Nội dung:** [Diagram] Cấu trúc Frame protocol (`protocol/frame.c`): `[STX | CMDID | LENGTH | PAYLOAD(8 bytes) | CHKSUM | ETX]`. Phát hiện và chặn sai lệch bản tin do nhiễu UART.

**SLIDE 20: Vi điều khiển ESP32 đóng vai trò GateWay IoT định tuyến dữ liệu lên MQTT Broker**
- **Nội dung:** [Flowchart] Phân luồng ESP: Nhận Frame UART -> bóc tách thư viện thành dạng JSON chuẩn -> đẩy lên Server qua WiFi.

---

## [SLIDE CHUYỂN PHẦN 5] THIẾT KẾ PCB, GIA CÔNG & THỰC NGHIỆM ĐÁNH GIÁ (Tương ứng Bước 7 & Bước 8)
*(Ghi chú: Chuyển sang phần minh chứng kỹ thuật là board mạch chạy thực tế đo lường đụng chạm tới tải xoay chiều)*

**SLIDE 21: Layout Mạch in (PCB) được thiết kế cắt rãnh chống rò điện (Milling) gia tăng tuổi thọ**
- **Nội dung:** [Diagram] Khằng định quy tắc chia tách Polygon/Plane: Nền cao áp AC (lưới Relay) cách ly tuyệt đối với nền DC Digital GND của vi điều khiển.

**SLIDE 22: Thiết kế cơ cấu hộp bằng in 3D (Packaging) cô lập triệt để nguy cơ giật điện môi trường**
- **Nội dung:** [Image] Hình hộp in 3D, vỏ thiết bị, và các cụm socket bọc cứng cáp.

**SLIDE 23: Mô hình kiểm thử phần cứng động với các tải điện xoay chiều công suất**
- **Nội dung:** [Image/Diagram] Chụp Bench-test: Thiết bị đo (Ampe Kìm chuyên dụng) đấu nối tiếp hộp tải 300W - 1000W đi qua Board mạch thiết kế.

**SLIDE 24: Kế Quả Phân tích tuyến tính so chiếu kết quả chứng thực sự chính xác > 95%**
- **Nội dung:** [Graph] Đồ thị đường thẳng XY, Trục Y máy đo mẫu, Trục X giá trị báo trên Web/Ứng dụng IoT. Hai đường thẳng đi song song sát nhau.

**SLIDE 25: Tự phát triển Giải pháp Phần mềm Giám sát trạng thái và Điều khiển đóng cắt độc lập (Dashboard HMI/IoT)**
- **Nội dung:** [Image] Ảnh giao diện Web/App tự xây dựng với Panel số liệu Real-time đo lường dòng điện tĩnh, Biểu đồ thống kê 8 kênh đồng thời, và hệ thống nút bấm quản lý thay đổi trạng thái thiết bị (Relay) chủ động.

**SLIDE 26: Quản trị Vật tư (BOM Costing) khẳng định lợi thế giá thành cực thấp trong môi trường thực tiễn**
- **Nội dung:** [Table] Các linh kiện chính yếu (ATmega16 55k, Hệ ADC+MUX 40k, IC 8255...). Bỏ xa giá 1 trạm PLC + Module Analog mở rộng hàng chục triệu đồng.

---

## [SLIDE CHUYỂN PHẦN 6] TỔNG KẾT & PHẢN BIỆN (Tương ứng Bước 9)
*(Ghi chú: Đóng lại vấn đề, nhấn mạnh về tính khả thi của hệ vi xử lý AVR khi giải một bài toán công nghiệp vừa và nhỏ)*

**SLIDE 27: Kết luận Hoàn tất toàn diện 3 khía cạnh: Thiết kế lõi Vi xử lý - Tối ưu điều khiển ngoại vi - Tích hợp IoT**
- **Nội dung:** Khẳng định 3 kết quả thực tiễn đạt được sát với mục tiêu ban đầu từ Slide 4.

**SLIDE 28: Huấn luyện AI nhận dạng dị thường năng lượng trong định hướng phát triển tương lai (Future Works)**
- **Nội dung:** 
  - **Phần cứng:** Nâng cấp độ phân giải ADC lên 12/16-bit, kết hợp cảm biến ZMPT101B để tính Công Suất (W). 
  - **Phần mềm (AI/ML):** Thu thập tập dữ liệu dao động của dòng tổng để Train mô hình Trí tuệ nhân tạo (AI) $\rightarrow$ Phân tích và nhận diện chính xác tải nào đang hoạt động $\rightarrow$ Nhận diện sớm dấu hiệu dị thường (nguy cơ chập cháy, phóng điện) để đưa ra quyết định "Tự động ngắt bảo vệ" thay vì chỉ đóng / cắt thủ công.

**SLIDE 29: Video DEMO Thiết bị thực tế hoạt động lưu loát không giật lag**
- **Nội dung:** Click Auto-play video 30s ngắn gọn tải dòng giả lập.

**SLIDE 30: Trình diễn Q&A (Thank you!)**
- **Nội dung:** Cảm ơn Hội đồng Giảng viên. Sinh viên sẵn sàng bảo vệ lập luận với các thông số minh chứng và chất lượng mã nguồn thực tế.
