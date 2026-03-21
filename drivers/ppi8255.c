#include "ppi8255.h"
#include "../bsp/board.h"
#include "../config/system_config.h" 
#include <util/delay.h>

// This function use to select register of the 8255 = A0, A1 


/* Hàm này dùng để cấu hình mức logic trên hai chân phần cứng là A0 và A1 của IC 8255 nhằm "chỉ định" (địa chỉ) nơi dữ liệu sẽ được ghi vào hoặc đọc ra. Các chân này hoạt động như một bộ giải mã địa chỉ hai bit nội bộ bên trong 8255:
    - addr = 0 (A1=0, A0=0) -> Chọn Port A
    - addr = 1 (A1=0, A0=1) -> Chọn Port B
    - addr = 2 (A1=1, A0=0) -> Chọn Port C
    - addr = 3 (A1=1, A0=1) -> Chọn Thanh ghi Điều khiển (Control Register) 
 Ví dụ: addr = 3 => A1A0 = 11 */
static void set_addr(uint8_t addr) {
	// Set A0
    if (addr & 0x01)
        PPI_CTRL_PORT |= (1 << PPI_A0_PIN);
    else
        PPI_CTRL_PORT &= ~(1 << PPI_A0_PIN);

	// Set A1
    if (addr & 0x02)
        PPI_CTRL_PORT |= (1 << PPI_A1_PIN);
    else
        PPI_CTRL_PORT &= ~(1 << PPI_A1_PIN);
}

/* Hàm này thực hiện trình tự đẩy (Ghi) 1 Byte dữ liệu (data) tới một đầu ra cụ thể trên con IC 8255 (được xác định qua addr).
Nó mô phỏng lại đúng Biểu đồ thời gian (Timing Diagram) phần cứng của chu kỳ Ghi mà nhà sản xuất Intel yêu cầu đối với chip 8255: */
static void write(uint8_t addr, uint8_t data) {
	// Set data bus output
    PPI_DATA_PORT = data;

	// Set address
    set_addr(addr);

	// Đảm bảo rằng tín hiệu RD ở mức High (không hoạt động) và CS ở mức Low (chọn chip) trước khi kích hoạt tín hiệu WR để bắt đầu chu kỳ ghi.
    PPI_CTRL_PORT |= (1 << PPI_RD_PIN);
    PPI_CTRL_PORT &= ~(1 << PPI_CS_PIN);

    PPI_CTRL_PORT &= ~(1 << PPI_WR_PIN);
	// Tạo xung WR bằng cách giữ nó ở mức Low trong một khoảng thời gian tối thiểu (theo yêu cầu của nhà sản xuất) để đảm bảo rằng dữ liệu được ghi đúng vào chip. Thời gian này thường là vài micro giây, do đó chúng ta sử dụng
    _delay_us(1); 
    PPI_CTRL_PORT |= (1 << PPI_WR_PIN);

    PPI_CTRL_PORT |= (1 << PPI_CS_PIN);
}

/* Cấu hình hướng xuất nhập ban đầu. Set tất cả Port B điều khiển 8255A 
và Data Bus 8255. Sau đó đặt xung Idle cho chip. Gọi 'write(0, 0x80)' 
vào Register Control Address 3 -> Cấu hình cho 8255A ở Mode 0*/
void ppi8255_init() {
    // set control pins output
    PPI_CTRL_DDR |= (1 << PPI_WR_PIN) | (1 << PPI_RD_PIN) | (1 << PPI_A0_PIN) |
                    (1 << PPI_A1_PIN) | (1 << PPI_CS_PIN);

    // set data bus output
    PPI_DATA_DDR = 0xFF;

    // default state
    PPI_CTRL_PORT |= (1 << PPI_WR_PIN);
    PPI_CTRL_PORT |= (1 << PPI_RD_PIN);
    PPI_CTRL_PORT |= (1 << PPI_CS_PIN);

	// Gửi thanh ghi cấu hình vào bên trong 8255A để thiết lập chế độ hoạt động.
    // Cả 8255 chạy ở Mode 0, và Port A, Port B, Port C đều là output
    write(3, 0x80); // mode 0, all output
}


void ppi8255_write_portA(uint8_t value) { write(0, value); }