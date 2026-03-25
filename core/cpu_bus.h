#ifndef CPU_BUS_H
#define CPU_BUS_H

#include <stdint.h>

// Hàm này được gọi bởi CPU để đọc dữ liệu từ một địa chỉ cụ thể trên bus. Nó sẽ trả về giá trị 8-bit được lưu trữ tại địa chỉ đó, có thể là từ ROM, RAM hoặc I/O map tùy thuộc vào phạm vi địa chỉ.
uint8_t cpu_bus_read(uint16_t addr);

// Hàm này được gọi bởi CPU để ghi dữ liệu vào một địa chỉ cụ thể trên bus. Nó sẽ ghi giá trị 8-bit vào địa chỉ đó, có thể là RAM hoặc I/O map tùy thuộc vào phạm vi địa chỉ. Việc ghi vào ROM sẽ bị bỏ qua hoặc
void cpu_bus_write(uint16_t addr, uint8_t data);

#endif
