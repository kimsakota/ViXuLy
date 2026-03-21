#ifndef UART_H_
#define UART_H_

#include <stdbool.h>
/* Xây dựng chuẩn truyền thông nối tiếp thông qua việc bit-banging
(sử dụng kỹ thuật lập trình GPIO nhấp nhả mức logic có tính toán 
thời gian thay vì phần cứng UART module tích hợp)*/

/* Hàm này cấu hình TX là ngõ ra, RX là ngõ vào. Đặt trạng thái TX 
lên múc Hight (default) làm tín hiệu chờ trạng thái rỗi tĩnh của 
chuẩn USART */
void uart_init();

/* Hàm đẩy 1 byte dữ liệu c. Đầu tiên báo hiệu bằng 1 Start bit
(Mức dòng kéo xuống Low), sử dụng bitwise logic trượt giá trị lặp qua 8 bit dữ liệu tuần 
tự gửi, và kết thúc chốt bằng 1 Stop bit (High). Khoảng thời gian
delay tính toán chuẩn cho thiết lập giữa các bit là 104. Thời gian
này tuân theo công thức 1s / 9600 baud = 104.15 µs/bit truyền */
void uart_write_char(char c);

/* Vòng lặp gửi đi văn bản text string chuỗi tuần tự từng kí tự một*/
void uart_write_string(const char* s);

/* Đọc byte đơn. Nó là một hàm đọc không chặn (Non-blocking mode). 
Nó kiểm tra xem chân RX có rớt xuống Low biểu thị Start bit hay không. 
Nếu có, delay chờ '52us' để lấy mẫu ở giữa của bit để tránh gia nhiễu
điện (noise glitch), sau đó dịch mẫu tuần tự tiếp tục delay đợi
đọc 8 bit và cuối cùng gắn dữ liệu về biến con trở */
bool uart_read_char(char* c);

#endif /* UART_H_ */