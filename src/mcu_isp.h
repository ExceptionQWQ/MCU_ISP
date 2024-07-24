/*
 * @author BusyBox
 * @date 2024/7/24
 * @version 1.0
 */

#pragma once

#include <stdio.h>
#include <unistd.h>
#include <cstdint>
#include <cstddef>
#include <cstdarg>
#include <chrono>
#include <thread>
#include <memory>
#include <fstream>
#include <boost/endian.hpp>

#include "io_stream.h"

class MCU_ISP
{
public:
    ~MCU_ISP();
    MCU_ISP(std::shared_ptr<IOStream> io_stream);

    bool waitACK();
    void sync();
    uint8_t checksum(uint8_t* data, size_t len);

    void get_command();
    void get_id_command();
    void erase_all();
    void write_block(uint32_t addr, uint8_t* data, size_t len);
    void read_block(uint32_t addr, uint8_t* buff, size_t len);
    void write_bin(const std::string& bin_path);
    void verify(const std::string& bin_path);

private:
    std::shared_ptr<IOStream> _io_stream;
};

