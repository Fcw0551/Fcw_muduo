#include "../include/Fcw_Buffer.hpp"
    Buffer::Buffer() 
    : _reader_idx(0)
    , _writer_idx(0)
    , _buffer(BUFFER_DEFAULT_SIZE) 
    {}
    //buffer的起始位置
    char* Buffer::bufferBegin(){ 
        return &*_buffer.begin(); 
    }
    //获取当前写⼊起始地址, _buffer的空间起始地址，加上写偏移量
    char* Buffer::writePos(){
         return bufferBegin() + _writer_idx; 
    }
     // 获取当前读取起始地址
    char* Buffer::readPos(){
        return bufferBegin() + _reader_idx; 
    }
    // 获取缓冲区末尾空闲空间⼤⼩
    uint64_t Buffer::tailFreeSize(){
        return _buffer.size() - _writer_idx;
    }
    // 获取缓冲区起始空闲空间⼤⼩--读偏移之前的空闲空间
    //读完了之后就可以覆盖了
    uint64_t Buffer::headFreeSize(){ 
        return _reader_idx;
    }
    // 获取可读数据⼤⼩ = 写偏移 - 读偏移
    uint64_t Buffer::readSize(){
        return _writer_idx - _reader_idx;
    }
    // 将读偏移向后移动
    void Buffer::moveReadOffset(uint64_t len){
        if (len == 0)
            return;
        // 向后移动的⼤⼩，必须⼩于可读数据⼤⼩
        assert(len <= readSize());
        _reader_idx += len;
    }
    // 将写偏移向后移动
    void Buffer::moveWriteOffset(uint64_t len){
        // 向后移动的⼤⼩，必须⼩于当前后边的空闲空间⼤⼩
        assert(len <= tailFreeSize());
        _writer_idx += len;
    }
    // 确保可写空间⾜够（整体空闲空间够了就移动数据，否则就扩容）
    void Buffer::ensureWriteSpace(uint64_t len){
        // 如果末尾空闲空间⼤⼩⾜够，直接返回
        if (tailFreeSize() >= len)
        {
            return;
        }
        // 末尾空闲空间不够
        //看头和尾的空闲空间一起够不够
        if (len <= tailFreeSize() + headFreeSize())
        {
            // 将数据移动到起始位置
            uint64_t rsz = readSize();                            // 把当前数据⼤⼩先保存起来
            std::copy(readPos(), readPos() + rsz, bufferBegin()); // 把可读数据拷⻉到起始位置
            _reader_idx = 0; // 将读偏移归0 
            _writer_idx = rsz; //将写位置置为可读数据⼤⼩， 因为当前的可读数据⼤⼩就是写偏移量
        }
        else
        {
            // 总体空间不够，则需要扩容，不移动数据，直接给写偏移之后扩容⾜够空间即可
            //优化空间：可以一次性扩容多一点
            _buffer.resize(_writer_idx + len);
        }
    }
    // 写⼊数据,这里如果重复调用不会修改写偏移
    //只有追加才会修改写偏移
    void Buffer::Write(const void *data, uint64_t len){
        // 1. 保证有⾜够空间，2. 拷⻉数据进去
        if (len == 0)
            return;
        ensureWriteSpace(len);
        const char *d = (const char *)data;
        std::copy(d, d + len, writePos());
    }
    void Buffer::writeAndPush(const void *data, uint64_t len){
        Write(data, len);
        moveWriteOffset(len);
    }
    void Buffer::writeString(const std::string &data){
        return Write(data.c_str(), data.size());
    }
    void Buffer::writeStringAndPush(const std::string &data){
        writeString(data);
        moveWriteOffset(data.size());
    }
    void Buffer::writeBuffer(Buffer &data){
        return Write(data.readPos(), data.readSize());
    }
    void Buffer::writeBufferAndPush(Buffer &data){
        writeBuffer(data);
        moveWriteOffset(data.readSize());
    }
    // 读取数据
    void Buffer::Read(void *buf, uint64_t len){
        // 要求要获取的数据⼤⼩必须⼩于可读数据⼤⼩
        assert(len <= readSize());
        std::copy(readPos(), readPos() + len, (char *)buf);
    }
    void Buffer::readAndPop(void *buf, uint64_t len){
        Read(buf, len);
        moveReadOffset(len);
    }
    std::string Buffer::readString(uint64_t len){
        // 要求要获取的数据⼤⼩必须⼩于可读数据⼤⼩
        assert(len <= readSize());
        std::string str;
        str.resize(len);
        Read(&str[0], len);
        return str;
    }
    std::string Buffer::readStringAndPop(uint64_t len){
        assert(len <= readSize());
        std::string str = readString(len);
        moveReadOffset(len);
        return str;
    }
    char* Buffer::findCRLF(){
        // memmem：这个函数能够不忽略\0,在内存块中找指定字节序列（\r\n，长度2）
        char *res = (char *)memmem(readPos(), readSize(), "\r\n", 2);
        return res;
    }
    char *Buffer::findLF(){
        char *res = (char *)memchr(readPos(),'\r', readSize());
        return res;
    }
    char *Buffer::findCR(){
        char *res = (char *)memchr(readPos(),'\r', readSize());
        return res;
    }
    //通常获取⼀⾏数据    
    std::string Buffer::getLine(){
        char *pos = findCRLF();//获取\r\n
        if (pos == NULL)
        {
            return "";
        }
        // +1是为了把\n字符也取出来。
        return readString(pos - readPos() + 2);
    }
    std::string Buffer::getLineAndPop(){
        std::string str = getLine();
        moveReadOffset(str.size());
        return str;
    }
    // 清空缓冲区
    void Buffer::Clear(){
        // 只需要将偏移量归0即可
        _reader_idx = 0;
        _writer_idx = 0;
    }
    
    //打印文本格式（核心：提取有效数据，构造 std::string 打印）//调试用
    void Buffer::printBufferText(){
        // 1. 获取有效数据的长度和起始地址
        uint64_t valid_len = readSize();
        char *valid_data = readPos();

        // 2. 处理空数据情况，避免打印乱码
        if (valid_len == 0)
        {
            DBG_LOG("Buffer 内容：[空，无有效数据]");
            return;
        }

        // 3. 构造 std::string（注意：如果数据不含 \0，需要指定长度，避免截断）
        std::string content(valid_data, valid_len);

        // 4. 打印
        //DBG_LOG("Buffer 有效数据长度：%lu 字节，内容：\n%s", valid_len, content.c_str());
    }