/*
 * @author BusyBox
 * @date 2024/7/24
 * @version 1.0
 */
#include <iostream>
#include <memory>
#include "serial_port.h"
#include "mcu_isp.h"


int main()
{
    std::string path = "/dev/tty.usbserial-14210";
    auto serial = std::make_shared<SerialPort>(path, 115200);

    auto isp = std::make_shared<MCU_ISP>(serial);

    isp->sync();

    isp->get_command();
    isp->get_id_command();

    // isp->write_bin("gd32H7_gcc.bin");
    // isp->verify("gd32H7_gcc.bin");

    uint8_t data[4] = {0x12, 0x34, 0x56, 0x78};
    isp->write_block(0x08000000, data, 4);

    uint8_t buff[4];
    isp->read_block(0x08000000, buff, 4);
    for (int i = 0; i < 4; ++i) {
        printf("%x \n", buff[i]);
    }

    return 0;
}