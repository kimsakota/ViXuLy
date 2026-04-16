# Kịch bản thuyết trình SeaOrder (Khoảng 5 - 6 phút)

**Thời gian:** ~5 - 6 phút
**Người trình bày:** Kim Sa Kota
**Slide thực hiện:** Từ Slide 1 đến Slide 8 (Bao quát Giới thiệu, Mục tiêu, Kiến trúc, ERD, Luồng tạo đơn và Màn hình Chờ giao)

---

## 1. Mở đầu & Lời chào (Slide 1: Cover)
*(Giao tiếp bằng mắt, phong thái tự tin, mỉm cười)*
"Kính chào Thầy/Cô và các bạn. Mình là Kim Sa Kota. Hôm nay, mình rất vinh dự được đứng đây để trình bày báo cáo đồ án phần mềm với đề tài: **SeaOrder – Ứng dụng Quản lý Bán Hải Sản trên di động**. Đây là một hệ thống được mình xây dựng trên nền tảng đa nền tảng .NET MAUI, kết hợp với công nghệ realtime SignalR và kiến trúc REST API."

*(Click sang slide)*

## 2. Điểm qua nội dung (Slide 2: Nội dung trình bày)
"Bài thuyết trình của mình hôm nay sẽ đi qua 5 phần chính: Bắt đầu từ việc Đặt vấn đề và Mục tiêu, sau đó là Phân tích thiết kế hệ thống, tiếp theo mình sẽ giới thiệu các Tính năng và Giao diện nổi bật, điểm qua một chút về Công nghệ & Bảo mật áp dụng, và cuối cùng là phần kết luận."

*(Click sang slide)*

## 3. Bối cảnh & Mục tiêu (Slide 3 & 4: Đặt vấn đề & Mục tiêu)
"Bước vào phần đầu tiên, bối cảnh thực tế. Nếu các bạn từng quan sát các đại lý hay cửa hàng hải sản, việc ghi chép sổ sách thủ công vẫn diễn ra rất phổ biến. Cách làm này không những tốn thời gian, dễ tính toán sai lệch, mà còn dẫn đến việc quản lý công nợ khách hàng vô cùng khó khăn, nợ cũ nợ mới lẫn lộn, và gần như không có báo cáo doanh thu chính xác.

Từ bài toán đó, mục tiêu của SeaOrder là số hóa toàn bộ quy trình bán hàng. Thứ nhất, mình muốn mọi thao tác tạo đơn, cân đo, tính tiền đều được làm trên điện thoại một cách nhanh chóng. Thứ hai, ứng dụng phải có khả năng thông báo tức thời qua SignalR – khi nhân viên lên đơn, chủ quán ở nhà cũng nhận được ngay. Đồng thời, hệ thống cung cấp Dashboard thống kê rõ ràng doanh thu, công nợ, và được bảo mật an toàn với hệ thống token JWT cùng xác thực vân tay sinh trắc học."

*(Click sang slide)*

## 4. Kiến trúc hệ thống (Slide 5: Kiến trúc tổng thể)
"Để hiện thực hóa các mục tiêu trên, mình đã xây dựng kiến trúc hệ thống chia làm 3 tầng rõ rệt:
- **Tầng Client (Mobile App):** Dựng khung bởi .NET MAUI 10 nhắm tới nền tảng Android, áp dụng chặt chẽ mô hình MVVM để tách biệt giao diện và logic (CommunityToolkit.Mvvm).
- **Tầng Backend/API:** Sử dụng ASP.NET Core cung cấp các REST API. Điểm nhấn ở đây là mình tích hợp SignalR Hub để xử lý các luồng dữ liệu thời gian thực (real-time broadcasting).
- **Tầng Database:** Xử lý và lưu trữ thông qua SQL Server cùng Entity Framework Core.

Luồng tương tác rất trơn tru: Mobile App gửi request lên API, API thay đổi dữ liệu trong Database, sau đó thông báo đến SignalR Hub để đẩy (push) sự kiện cập nhật về lại **tất cả** các thiết bị client đang online. Không cần người dùng phải thao tác kéo thả (pull-to-refresh) để làm mới trang."

*(Click sang slide)*

## 5. Mô hình Dữ liệu Core (Slide 6: Mô hình ERD)
"Tiếp theo là mô hình dữ liệu cốt lõi (ERD). Giữ cho hệ thống đơn giản nhưng đáp ứng đúng nghiệp vụ thực tế, mình có các thực thể chính: Khách hàng (Customer), Sản phẩm (Product) và Đơn hàng (Order - OrderItem).
Đặc thù của ngành bán hải sản là có loại bán theo số kg (như cua, ghẹ), có loại lại đếm con hoặc tách riêng con lớn/con nhỏ. Vì vậy, ở bảng Product mình thiết kế thêm các cờ như `IsCountable` hay `IsSeparated` để app biết tự động mở đúng giao diện nhập liệu. 
Công thức tính tổng tiền cũng được linh hoạt cấu hình theo công thức thực tế của dân buôn hải sản: `Tổng = Giá × (Trọng lượng + Số lượng × Đơn giá cộng thêm)`."

*(Click sang slide)*

## 6. Luồng xử lý nghiệp vụ & Giao diện tạo đơn (Slide 7 & 8: Luồng tạo đơn & Màn hình chờ giao)
"Nhờ vào mô hình dữ liệu đó, luồng tạo đơn hàng mới trên SeaOrder diễn ra cực kỳ tốc độ.
Bắt đầu từ việc người bán mở App, hệ thống sẽ kiểm tra bảo mật Session (JWT Token). Nhân viên chọn Khách hàng, chọn Sản phẩm. Nếu loại hải sản đó cần phần cân nặng, App lập tức hiển thị `WeighingPopup` để nhập số kg. Ngay khi ấn xác nhận, ứng dụng lưu qua API. Đơn hàng được lưu, sự kiện `NewOrder` được bắn qua SignalR đến thiết bị của thu ngân, màn hình Đơn Chờ Giao (PendingOrders) sẽ nổi lên Card dữ liệu mới ngay tức thì, rất trơn tru và trực quan. Tại đây, thao tác vuốt để xóa, sửa trọng lượng hay gạt sang để xác nhận "Giao ngay" đều được thiết kế thân thiện, giúp tối giản hóa công sức cho người dùng thao tác."

*(Dừng lại một nhịp, mỉm cười)*
"Đến đây, mình xin phép đi sâu hơn vào phần hệ thống lịch sử và thống kê công nợ ở các slide tiếp theo..."
