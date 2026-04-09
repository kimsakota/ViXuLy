# KHUNG NỘI DUNG THUYẾT TRÌNH (>= 30 SLIDES)

**Môn học:** Technical Writing and Presentation (Đại học Bách Khoa Hà Nội)
**Đề tài:** Hệ thống Giám sát dòng điện 8 kênh và Điều khiển thiết bị ứng dụng IoT (ATmega16 + 8255 + ADC0804 + ESP32)

> **Cam kết tuân thủ 3 Laws (Technical Writing HUST):**
> 1. **Clear Message (Assertion-Evidence):** Tiêu đề là một câu khẳng định thông điệp ("Slide X: Cảm biến ACS712 trích xuất dòng điện tuyến tính...").
> 2. **Cấu trúc dạng cây (Tree-based Structure):** Top-down mạch lạc tuân theo 9 bước chuẩn của Báo cáo đồ án.
> 3. **Trực quan hóa (Visual Evidence):** Thay thế chữ bằng Đồ thị (Graph), Sơ đồ (Diagram), Phương trình (Equation), Lưu đồ (Flowchart), Gantt Chart.

---

## PHẦN 1: MỞ ĐẦU - PHÂN TÍCH ĐỀ TÀI VÀ KẾ HOẠCH (Tương ứng Bước 1 & 2)

**SLIDE 1: Trang bìa (Title Slide)**
- **Tiêu đề:** Thiết kế Hệ thống Giám sát Dòng điện đa kênh và Cảnh báo Đo lường ứng dụng IoT.
- **Phụ đề:** [Hình ảnh] Ảnh ngoại quan sản phẩm thực tế và Logo HUST.
- **Thông tin:** Tên Sinh viên, MSV, Tên Giảng viên.

**SLIDE 2: Nội dung trình bày tuân theo cấu trúc Top-Down (Agenda)**
- **Nội dung:** [Diagram] Sơ đồ cây (Tree-diagram) các phần chính.
  - Nhánh 1: Phân tích và Lập kế hoạch.
  - Nhánh 2: Sơ đồ khối và Thiết kế nguyên lý chi tiết (Hardware).
  - Nhánh 3: Tối ưu phần mềm và Giao thức hệ thống (Firmware & Software).
  - Nhánh 4: Thiết kế PCB, Hoàn thiện và Kiểm thử (Testing & Packaging).

**SLIDE 3: Chi phí và độ phức tạp hạn chế việc mở rộng giám sát năng lượng trong công nghiệp (Motivation)**
- **Nội dung:** [Graph] Biểu đồ cột/đường so sánh chi phí và số kênh đo của các giải pháp hiện tại vs thiết bị chuyên dụng PLC.

**SLIDE 4: Giải pháp dùng vi điều khiển ATmega16 giải quyết bài toán lớn với chi phí < 500.000đ (Objectives)**
- **Nội dung:** Khẳng định 3 mục tiêu cốt lõi: Đo 8 kênh đồng thời, đóng cắt Relay độc lập, đẩy dữ liệu lên IoT (MQTT) ổn định.

**SLIDE 5: Chỉ tiêu kỹ thuật định cỡ không gian thiết kế của phần cứng và luồng dữ liệu (Specifications)**
- **Nội dung:** [Table] Bảng chỉ tiêu - Số kênh, Phân giải (8-bit ~0.04A/bước), Giao thức kết nối, Chu kỳ lấy mẫu định kì (10Hz).

**SLIDE 6: Kế hoạch thực hiện dự án được quản trị rõ ràng đảm bảo tiến độ mô hình thác nước**
- **Nội dung:** [Gantt Chart] Biểu đồ tiến độ mô tả thời gian phân bổ: (Khảo sát lý thuyết -> Thiết kế phần cứng -> Phương pháp phần mềm -> Code hệ thống -> Kiểm thử).

---

## PHẦN 2: XÂY DỰNG SƠ ĐỒ KHỐI VÀ DATA FLOW (Tương ứng Bước 3)

**SLIDE 7: Kiến trúc tổng thể được phân cấp thành 4 Lớp chức năng phần cứng (Architecture)**
- **Nội dung:** [Diagram] Sơ đồ khối hệ thống (Từ nguồn cấp tĩnh, Lớp thu thập - Sensor, Lớp xử lý trung tâm MCU+8255, Lớp truyền động (Relay), tới Lớp điện toán đám mây).

**SLIDE 8: Luồng dữ liệu dịch chuyển từ thế giới vật lý thành dữ liệu số trên Đám mây (Data Flow)**
- **Nội dung:** [Flowchart] Dòng chảy: Dòng AC vật lý -> Điện áp Analog -> Số hóa 8-bit (D) -> Khung truyền UART (Frame) -> Gói tin JSON/MQTT.

---

## PHẦN 3: THIẾT KẾ CHI TIẾT VÀ MẠCH NGUYÊN LÝ (Tương ứng Bước 4 & 5)

**SLIDE 9: Cảm biến tuyến tính ACS712 trích xuất dòng tĩnh với hệ số 185mV/A (Sensing Unit)**
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

## PHẦN 4: KIẾN TRÚC PHẦN MỀM VÀ LẬP TRÌNH NHÚNG (Tương ứng Bước 6 - Software Highlight)

**SLIDE 14: Mã nguồn C nhúng áp dụng mô hình Đa tầng (Layered Architecture) tiêu chuẩn công nghiệp**
- **Nội dung:** [Diagram] Trực quan hóa cây thư mục dự án:
  1. **Core & BSP:** Xử lý Hardware-level (`cpu_bus.c`, `board.c`)
  2. **Drivers:** Điều khiển vi mạch rời (`ppi8255.c`, `adc0804.c`)
  3. **Services & Protocol:** Logic tính toán & Giao thức (`measurement_service.c`, `frame.c`)
  4. **App:** Kịch bản luồng chính (`app.c`)
- **Key point:** Triệt tiêu hoàn toàn cách viết cục bộ "Spaghetti Code".

**SLIDE 15: Phương pháp Ánh xạ Bộ nhớ (Memory-Mapped I/O) khai phóng sức mạnh xử lý thanh ghi**
- **Nội dung:** [Diagram] Phân bổ Memory Map (thể hiện trong `core/memory.c`). Thay vì bit-bang chậm chạp, CPU Bus giao tiếp trực tiếp với không gian địa chỉ 8255 siêu nhanh.

**SLIDE 16: Module Measurement Service phân luồng dữ liệu đo lường liên tục qua định thời máy trạng thái**
- **Nội dung:** [Flowchart] Lưu đồ tuần tự trong `measurement_service.c`: Set MUX -> Trigger WR của ADC -> Wait ngắt INTR -> Tính toán Dòng điện $\rightarrow$ Thay đổi logic App.

**SLIDE 17: Giản đồ định thời (Timing Diagram) rập khuôn tốc độ lấy mẫu bắt buộc của phần cứng**
- **Nội dung:** [Graph] Sự tương quan thời gian độ trễ đóng MUX (Delay) với độ dài chu kì ADC biến đổi. Tránh xung đột luồng thu thập.

**SLIDE 18: Bộ lọc Trung bình động (Moving Average) bằng phần mềm cứu vớt nhiễu lượng tử ADC**
- **Nội dung:** [Equation & Graph] Lấy số kỳ $I_{filtered} = \frac{1}{N} \sum I_{raw}$. Trực quan độ mượt trên biểu đồ giá trị trước và sau khi đi qua bộ vi lọc.

**SLIDE 19: Dữ liệu truyền tải được cấu trúc thành Framework hoàn chỉnh bảo vệ bằng CheckSum**
- **Nội dung:** [Diagram] Cấu trúc Frame protocol (`protocol/frame.c`): `[STX | CMDID | LENGTH | PAYLOAD(8 bytes) | CHKSUM | ETX]`. Phát hiện và chặn sai lệch bản tin do nhiễu UART.

**SLIDE 20: Vi điều khiển ESP32 đóng vai trò GateWay IoT định tuyến dữ liệu lên MQTT Broker**
- **Nội dung:** [Flowchart] Phân luồng ESP: Nhận Frame UART -> bóc tách thư viện thành dạng JSON chuẩn -> đẩy lên Server qua WiFi.

---

## PHẦN 5: THIẾT KẾ PCB, GIA CÔNG & KIỂM THỬ ĐÁNH GIÁ (Tương ứng Bước 7 & Bước 8)

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

## PHẦN 6: TỔNG KẾT & PHẢN BIỆN (Tương ứng Bước 9)

**SLIDE 27: Kết luận Hoàn tất toàn diện 3 khía cạnh: Lắp ráp phần cứng - Tối ưu phần mềm nhúng - Kết nối không dây**
- **Nội dung:** Khẳng định 3 kết quả thực tiễn đạt được sát với mục tiêu ban đầu từ Slide 4.

**SLIDE 28: Huấn luyện AI nhận dạng dị thường năng lượng trong định hướng phát triển tương lai (Future Works)**
- **Nội dung:** 
  - **Phần cứng:** Nâng cấp độ phân giải ADC lên 12/16-bit, kết hợp cảm biến ZMPT101B để tính Công Suất (W). 
  - **Phần mềm (AI/ML):** Thu thập tập dữ liệu dao động của dòng tổng để Train mô hình Trí tuệ nhân tạo (AI) $\rightarrow$ Phân tích và nhận diện chính xác tải nào đang hoạt động $\rightarrow$ Nhận diện sớm dấu hiệu dị thường (nguy cơ chập cháy, phóng điện) để đưa ra quyết định "Tự động ngắt bảo vệ" thay vì chỉ đóng / cắt thủ công.

**SLIDE 29: Video DEMO Thiết bị thực tế hoạt động lưu loát không giật lag**
- **Nội dung:** Click Auto-play video 30s ngắn gọn tải dòng giả lập.

**SLIDE 30: Trình diễn Q&A (Thank you!)**
- **Nội dung:** Cảm ơn Hội đồng Giảng viên. Sinh viên sẵn sàng bảo vệ lập luận với các thông số minh chứng và chất lượng mã nguồn thực tế.
