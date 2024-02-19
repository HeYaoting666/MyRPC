//
// Created by 20771 on 2024/1/18.
//

#ifndef ROCKET_TCP_BUFFER_H
#define ROCKET_TCP_BUFFER_H

#include <vector>
#include <algorithm>
#include "../common/log.h"

namespace rocket {

class TCPBuffer {
public:
    typedef std::shared_ptr<TCPBuffer> spointer;

private:
    int m_read_index;
    int m_write_index;

public:
    std::vector<char> m_buffer;

public:
    explicit TCPBuffer(int size);

public:
    // 返回可读字节数
    int readAble() const { return m_write_index - m_read_index; }

    // 返回可写的字节数
    int writeAble() const { return static_cast<int>(m_buffer.size()) - m_write_index; }

    int readIndex() const { return m_read_index; }

    int writeIndex() const { return m_write_index; }

    void writeToBuffer(const char* buf, int size);

    void readFromBuffer(std::vector<char>& result, int read_sz);

    void resizeBuffer(int new_size);

    void moveReadIndex(int size);

    void moveWriteIndex(int size);

    char& operator[](int index);

    const char& operator[](int index) const;

private:
    void adjustBuffer();
};

}

#endif //ROCKET_TCP_BUFFER_H
