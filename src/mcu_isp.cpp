/*
 * @author BusyBox
 * @date 2024/7/24
 * @version 1.0
 */

#include "mcu_isp.h"

MCU_ISP::~MCU_ISP()
{

}

MCU_ISP::MCU_ISP(std::shared_ptr<IOStream> io_stream)
{
    this->_io_stream = io_stream;
}

bool MCU_ISP::waitACK()
{
    uint8_t recv;
    try {
        std::size_t recv_bytes = _io_stream->read(&recv, 1, 100);
        if (recv == 0x79) return true;
    } catch (const IOStream::TimeOutException e) {

    }
    return false;
}

void MCU_ISP::sync()
{
    int cnt = 0;
    uint8_t data = 0x7f, recv;
    while (true) {
        _io_stream->write(&data, 1);
        if (waitACK()) break;
        ++cnt;
        if (cnt > 3) {
            throw std::runtime_error{"MCU_ISP::sync() timeout!"};
        }
    }
    printf("MCU_ISP::sync() success!\n");
}

uint8_t MCU_ISP::checksum(uint8_t* data, size_t len)
{
    uint8_t sum = 0;
    for (size_t i = 0; i != len; ++i) {
        sum ^= data[i];
    }
    return sum;
}

void MCU_ISP::get_command()
{
    uint8_t data[2] = {0x00, 0xff};
    _io_stream->write(data, 2);
    if (!waitACK()) throw std::runtime_error{"MCU_ISP::get_command() waitACK timeout"};

    uint8_t recv[256];
    std::size_t index = 0;
    while (true) {
        std::size_t recv_bytes = _io_stream->read(recv + index, 1, 100);
        if (recv[index] == 0x79) break;
        ++index;
        if (index > 255) throw std::runtime_error{"MCU_ISP::get_command() out of memory"};
    }
    printf("bootloader version: %x\n", recv[1]);
    printf("commands support: ");
    for (uint8_t i = 0; i != recv[0]; ++i) {
        printf("%x ", recv[2 + i]);
    }
    printf("\n");
}

void MCU_ISP::get_id_command()
{
    uint8_t data[2] = {0x02, 0xfd};
    _io_stream->write(data, 2);
    if (!waitACK()) throw std::runtime_error{"MCU_ISP::get_id_command() waitACK timeout"};

    uint8_t recv[256];
    std::size_t index = 0;
    while (true) {
        std::size_t recv_bytes = _io_stream->read(recv + index, 1, 100);
        if (recv[index] == 0x79) break;
        ++index;
        if (index > 255) throw std::runtime_error{"MCU_ISP::get_id_command() out of memory"};
    }
    printf("id: ");
    for (uint8_t i = 0; i != recv[0]; ++i) {
        printf("%x", recv[1 + i]);
    }
    printf("\n");
}

void MCU_ISP::erase_all()
{
    uint8_t data[2] = {0x43, 0xbc};
    _io_stream->write(data, 2);
    if (!waitACK()) throw std::runtime_error{"MCU_ISP::erase_all() waitACK timeout 1"};
    uint8_t data2[2] = {0xff, 0x00};
    _io_stream->write(data2, 2);
    if (!waitACK()) throw std::runtime_error{"MCU_ISP::erase_all() waitACK timeout 2"};
}

void MCU_ISP::write_block(uint32_t addr, uint8_t* data, size_t len)
{
    printf("writing block %p size:%d ... ", addr, len);

    addr = boost::endian::endian_reverse(addr);

    uint8_t cmd[2] = {0x31, 0xce};
    _io_stream->write(cmd, 2);
    if (!waitACK()) throw std::runtime_error{"MCU_ISP::write_block() waitACK timeout 1"};

    _io_stream->write(reinterpret_cast<uint8_t*>(&addr), 4);
    uint8_t sum = checksum(reinterpret_cast<uint8_t*>(&addr), 4);
    _io_stream->write(&sum, 1);
    if (!waitACK()) throw std::runtime_error{"MCU_ISP::write_block() waitACK timeout 2"};

    uint8_t len2 = len - 1;
    _io_stream->write(&len2, 1);
    _io_stream->write(data, len);
    sum = len2 ^ checksum(data, len);
    _io_stream->write(&sum, 1);
    if (!waitACK()) throw std::runtime_error{"MCU_ISP::write_block() waitACK timeout 3"};

    printf("done!\n");
}

void MCU_ISP::read_block(uint32_t addr, uint8_t* buff, size_t len)
{
    printf("reading block %p size:%d ... ", addr, len);

    addr = boost::endian::endian_reverse(addr);

    uint8_t cmd[2] = {0x11, 0xee};
    _io_stream->write(cmd, 2);
    if (!waitACK()) throw std::runtime_error{"MCU_ISP::read_block() waitACK timeout 1"};

    _io_stream->write(reinterpret_cast<uint8_t*>(&addr), 4);
    uint8_t sum = checksum(reinterpret_cast<uint8_t*>(&addr), 4);
    _io_stream->write(&sum, 1);
    if (!waitACK()) throw std::runtime_error{"MCU_ISP::read_block() waitACK timeout 2"};

    uint8_t len2 = len - 1;
    _io_stream->write(&len2, 1);
    len2 = ~len2;
    _io_stream->write(&len2, 1);
    if (!waitACK()) throw std::runtime_error{"MCU_ISP::read_block() waitACK timeout 3"};
    std::size_t read_bytes = _io_stream->read(buff, len, 1000);
    printf("done!\n");
}

void MCU_ISP::write_bin(const std::string& bin_path)
{
    printf("writing %s\n", bin_path.c_str());
    FILE* fp = fopen(bin_path.c_str(), "rb");
    if (fp == nullptr) throw std::runtime_error{"cannot open " + bin_path};

    uint8_t buff[256];
    uint32_t offset = 0;
    while (true) {
        int read_bytes = fread(buff, 1, 256, fp);
        if (read_bytes == 0 || feof(fp)) break;
        write_block(0x08000000 + offset, buff, read_bytes);
        offset += read_bytes;
    }
    printf("writing done!\n");
}

void MCU_ISP::verify(const std::string& bin_path)
{
    printf("verifing %s\n", bin_path.c_str());
    FILE* fp = fopen(bin_path.c_str(), "rb");
    if (fp == nullptr) throw std::runtime_error{"cannot open " + bin_path};

    uint8_t buff[256], buff2[256];
    uint32_t offset = 0;
    while (true) {
        int read_bytes = fread(buff, 1, 256, fp);
        if (read_bytes == 0 || feof(fp)) break;
        read_block(0x08000000 + offset, buff2, read_bytes);
        for (int i = 0; i < 256; ++i) {
            printf("%x %x\n", buff[i], buff2[i]);
        }
        if (memcmp(buff, buff2, read_bytes) != 0) throw std::runtime_error{"MCU_ISP::verify() error"};
        offset += read_bytes;
    }
    printf("verify done!\n");
}