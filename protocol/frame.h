#ifndef FRAME_H_
#define FRAME_H_

#include <stdint.h>
#include <stdbool.h>

// Biến thể FRAME_HEADER được định nghĩa với giá trị 0xAA, đại diện cho byte đầu tiên của một khung dữ liệu (frame) hợp lệ trong giao thức truyền thông. Khi hệ thống nhận được byte này, nó sẽ hiểu rằng một khung dữ liệu mới đang bắt đầu và sẽ bắt đầu quá trình xây dựng khung dựa trên các byte tiếp theo. Việc sử dụng một header cố định giúp hệ thống dễ dàng xác định điểm bắt đầu của mỗi khung dữ liệu và phân biệt giữa các khung khác nhau trong luồng dữ liệu liên tục.
#define FRAME_HEADER 0xAA
// Biến thể FRAME_MAX_DATA được định nghĩa với giá trị 8, đại diện cho số lượng byte dữ liệu tối đa có thể chứa trong một khung dữ liệu (frame). Điều này có nghĩa là mỗi khung dữ liệu chỉ có thể chứa tối đa 8 byte thông tin thực tế (data), ngoài các thành phần khác như cmd và len. Việc giới hạn số lượng byte dữ liệu giúp đơn giản hóa quá trình xử lý và đảm bảo rằng khung dữ liệu không trở nên quá lớn hoặc phức tạp để quản lý.
#define FRAME_MAX_DATA 8

// Cấu trúc dữ liệu để lưu trữ thông tin của một khung dữ liệu (frame) đã được phân tích từ các byte nhận được. Nó bao gồm: dòng cmd (1 byte) để xác định loại lệnh hoặc hành động mà khung dữ liệu đại diện, dòng len (1 byte) để chỉ ra số lượng byte dữ liệu thực tế có trong khung, và một mảng data (FRAME_MAX_DATA byte) để chứa dữ liệu thực tế của khung. Cấu trúc này cho phép hệ thống dễ dàng truy cập và xử lý thông tin từ khung dữ liệu sau khi nó đã được xây dựng hoàn chỉnh.
typedef struct {
	uint8_t cmd;
	uint8_t len;
	uint8_t data[FRAME_MAX_DATA];
} frame_t;

// Hàm này khởi tạo bất kỳ cấu trúc dữ liệu hoặc trạng thái cần thiết nào để xử lý các khung dữ liệu (frames) trong giao thức truyền thông. Nó có thể thiết lập các biến toàn cục, bộ đệm, hoặc trạng thái ban đầu để đảm bảo rằng hệ thống sẵn sàng nhận và xử lý các khung dữ liệu một cách chính xác.
void frame_init();

// Hàm này được gọi mỗi khi một byte mới được nhận qua giao tiếp nối tiếp (ví dụ: UART). Nó sẽ xử lý byte đó và cố gắng xây dựng một khung dữ liệu hoàn chỉnh dựa trên các byte đã nhận trước đó. Nếu byte mới giúp hoàn thành một khung dữ liệu hợp lệ (đúng định dạng, có header, cmd, len, data phù hợp), thì hàm sẽ trả về true và điền thông tin của khung vào biến out. Nếu byte mới không hoàn thành một khung hợp lệ hoặc nếu có lỗi trong quá trình xây dựng khung, hàm sẽ trả về false.
bool frame_parse_byte(uint8_t byte, frame_t* out);

// Hàm này tính toán giá trị checksum cho một khung dữ liệu dựa trên các thành phần của nó: cmd, len, và data. Checksum là một giá trị được sử dụng để kiểm tra tính toàn vẹn của dữ liệu trong khung. Hàm sẽ sử dụng một thuật toán cụ thể (ví dụ: XOR, cộng modulo, v.v.) để kết hợp các thành phần của khung và trả về giá trị checksum tương ứng. Giá trị này sau đó có thể được sử dụng để xác minh rằng dữ liệu đã nhận không bị lỗi trong quá trình truyền.
uint8_t frame_calculate_checksum(uint8_t cmd, uint8_t len, uint8_t* data);

#endif /* FRAME_H_ */