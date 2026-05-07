#pragma once 
#include <iostream>
#include <vector>
#include <cassert>
#include <cstring>
#include "Fcw_Log.hpp"
#define BUFFER_DEFAULT_SIZE 128
class Buffer
{
private:
    std::vector<char> _buffer; // 使⽤vector进⾏内存空间管理
    uint64_t _reader_idx;      // 读偏移
    uint64_t _writer_idx;      // 写偏移
public:
    Buffer() ;
    //buffer的起始位置
    char* bufferBegin();
    //获取当前写⼊起始地址, _buffer的空间起始地址，加上写偏移量
    char *writePos();
     // 获取当前读取起始地址
    char *readPos();
    // 获取缓冲区末尾空闲空间⼤⼩
    uint64_t tailFreeSize();
    // 获取缓冲区起始空闲空间⼤⼩--读偏移之前的空闲空间
    //读完了之后就可以覆盖了
    uint64_t headFreeSize();
    // 获取可读数据⼤⼩ = 写偏移 - 读偏移
    uint64_t readSize();
    // 将读偏移向后移动
    void moveReadOffset(uint64_t len);
    // 将写偏移向后移动
    void moveWriteOffset(uint64_t len);
    // 确保可写空间⾜够（整体空闲空间够了就移动数据，否则就扩容）
    void ensureWriteSpace(uint64_t len);
    // 写⼊数据,这里如果重复调用不会修改写偏移
    //只有追加才会修改写偏移
    void Write(const void *data, uint64_t len);
    void writeAndPush(const void *data, uint64_t len);
    void writeString(const std::string &data);
    void writeStringAndPush(const std::string &data);
    void writeBuffer(Buffer &data);
    void writeBufferAndPush(Buffer &data);
    // 读取数据
    void Read(void *buf, uint64_t len);
    void readAndPop(void *buf, uint64_t len);
    std::string readString(uint64_t len);
    std::string readStringAndPop(uint64_t len);
    char *findCRLF();
    char *findLF();
    char *findCR();
    std::string getLine();
    std::string getLineAndPop();
    // 清空缓冲区
    void Clear();
    //打印调试
    void printBufferText();
};