//
// Created by 20771 on 2024/1/18.
//
#include "tcp_buffer.h"

namespace rocket {

TCPBuffer::TCPBuffer(int size): m_read_index(0), m_write_index(0) {
    m_buffer.resize(size);
}

void TCPBuffer::writeToBuffer(const char* write_buf, int write_buf_sz) {
    if(write_buf_sz > writeAble()) {
        // 调整m_buffer的大小，进行扩容
        int new_sz = static_cast<int>(1.5 * (m_write_index + write_buf_sz));
        resizeBuffer(new_sz);
    }

    std::copy_n(write_buf, write_buf_sz, m_buffer.begin() + m_write_index);
    m_write_index += write_buf_sz;
}

void TCPBuffer::readFromBuffer(std::vector<char>& result, int read_sz) {
    if(readAble() == 0) return;

    // 将需要读取的数据写入至 resul 中
    int read_size = std::min(readAble(), read_sz);
    std::vector<char> tmp(read_size);
    std::copy_n(m_buffer.begin() + m_read_index, read_size, tmp.begin());
    result.swap(tmp);

    m_read_index += read_size;
    adjustBuffer(); // 调整 m_buffer 移除已读取的字节
}

void TCPBuffer::resizeBuffer(int new_size) {
    if(new_size <= m_buffer.size()) return;

    std::vector<char> tmp(new_size);
    int count = readAble(); // 需要拷贝的字节数
    std::copy_n(m_buffer.begin() + m_read_index, count, tmp.begin());
    m_buffer.swap(tmp);

    m_read_index = 0;
    m_write_index = m_read_index + count;
}

void TCPBuffer::adjustBuffer() {
    if(m_read_index < static_cast<int>(m_buffer.size() / 3))
        return;

    std::vector<char> tmp(m_buffer.size());
    int count = readAble();
    std::copy_n(m_buffer.begin() + m_read_index, count, tmp.begin());
    m_buffer.swap(tmp);

    m_read_index = 0;
    m_write_index = m_read_index + count;
}

void TCPBuffer::moveReadIndex(int size) {
    if(m_read_index + size > m_write_index) {
        ERRORLOG("moveReadIndex error, invalid size %d, old_read_index %d, old_write_index %d", size, m_read_index, m_write_index)
        return;
    }
    m_read_index += size;
    adjustBuffer();
}

void TCPBuffer::moveWriteIndex(int size) {
    if(m_write_index + size > m_buffer.size()) {
        ERRORLOG("moveReadIndex error, invalid size %d, old_write_index %d, old_buffer_size %d", size, m_write_index, m_buffer.size())
        return;
    }
    m_write_index += size;
    adjustBuffer();
}

char& TCPBuffer::operator[](int index) {
    if(index >= m_write_index)
        throw std::out_of_range("subscript out of range");
    return m_buffer[index];
}

const char &TCPBuffer::operator[](int index) const {
    if(index >= m_write_index)
        throw std::out_of_range("subscript out of range");
    return m_buffer[index];
}


}