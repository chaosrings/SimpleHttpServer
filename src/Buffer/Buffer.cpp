#include "Buffer.h"

#include <errno.h>
#include <sys/uio.h>

const char Buffer::kCRLFCRLF[]="\r\n\r\n";

ssize_t Buffer::readFd(int fd,int* savedErrorno)
{
    char extrabuf[65536];
    struct iovec vec[2];
    const size_t writable=writableBytes();
    //在数据尾部添加
    vec[0].iov_base=begin()+writerIndex;
    vec[0].iov_len=writable;
    vec[1].iov_base=extrabuf;
    vec[1].iov_len=sizeof extrabuf;
    int iovcnt=(writable<sizeof extrabuf)? 2:1;
    const ssize_t n=::readv(fd,vec,iovcnt);
    if(n<0)
    {
        *savedErrorno=errno;
    }
    else if(static_cast<size_t>(n)<=writable)
    {
        //不需要用到extrabuf
        writerIndex+=n;
    }
    else
    {
        //writerIndex置于尾部,再调用append历程
        writerIndex=buffer.size();
        this->append(extrabuf,n-writable);
    }
    return n;
}