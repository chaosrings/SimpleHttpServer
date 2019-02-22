#include <algorithm>
#include <vector>
#include <string>
#include <assert.h>

using std::string;
using std::vector;

class Buffer
{
public:
    static const size_t kCheapPrepend = 8;
    static const size_t kInitialSize = 1024;
    explicit Buffer(size_t initialSize = kInitialSize)
        : buffer(kCheapPrepend + initialSize),
          readerIndex(kCheapPrepend),
          writerIndex(kCheapPrepend)
    {
        assert(readableBytes() == 0);
        assert(writableBytes() == initialSize);
        assert(prependableBytes() == kCheapPrepend);
    }

    bool empty() const {return readerIndex==writerIndex;}
    void swap(Buffer &rhs)
    {
        buffer.swap(rhs.buffer);
        std::swap(readerIndex, rhs.readerIndex);
        std::swap(writerIndex, rhs.writerIndex);
    }
    //可读大小
    size_t readableBytes() const
    {
        return writerIndex - readerIndex;
    }
    //可写大小
    size_t writableBytes() const
    {
        return buffer.size() - writerIndex;
    }
    //可在头部添加大小
    size_t prependableBytes() const
    {
        return readerIndex;
    }
    //有效数据起点
    const char *peek() const
    {
        return begin() + readerIndex;
    }
    //寻找\r\n\r\n
    const char *findCRLFCRLF() const
    {
        const char *crlf = std::search(peek(), beginWrite(), kCRLFCRLF, kCRLFCRLF + 4);
        return crlf == beginWrite() ? NULL : crlf;
    }

    // retrieve returns void, to prevent
    // string str(retrieve(readableBytes()), readableBytes());
    // the evaluation of two functions are unspecified
    void retrieve(size_t len)
    {
        assert(len <= readableBytes());
        if (len < readableBytes())
        {
            readerIndex += len;
        }
        else
        {
            retrieveAll();
        }
    }

    void retrieveUntil(const char *end)
    {
        assert(peek() <= end);
        assert(end <= beginWrite());
        retrieve(end - peek());
    }

    void retrieveAll()
    {
        readerIndex = kCheapPrepend;
        writerIndex = kCheapPrepend;
    }

    string retrieveAllAsString()
    {
        return retrieveAsString(readableBytes());
    }

    string retrieveAsString(size_t len)
    {
        assert(len <= readableBytes());
        string result(peek(), len);
        retrieve(len);
        return result;
    }

    string toStringPiece() const
    {
        return string(peek(), static_cast<int>(readableBytes()));
    }

    void append(const string &str)
    {
        append(str.data(), str.size());
    }

    void append(const char *  data, size_t len)
    {
        ensureWritableBytes(len);
        std::copy(data, data + len, beginWrite());
        hasWritten(len);
    }

    void append(const void * data, size_t len)
    {
        append(static_cast<const char *>(data), len);
    }

    void ensureWritableBytes(size_t len)
    {
        if (writableBytes() < len)
        {
            makeSpace(len);
        }
        assert(writableBytes() >= len);
    }

    char *beginWrite()
    {
        return begin() + writerIndex;
    }

    const char *beginWrite() const
    {
        return begin() + writerIndex;
    }
    void hasWritten(size_t len)
    {
        assert(len <= writableBytes());
        writerIndex += len;
    }
    void unwrite(size_t len)
    {
        assert(len <= readableBytes());
        writerIndex -= len;
    }
  

    void shrink(size_t reserve)
    {
        Buffer other;
        other.ensureWritableBytes(readableBytes() + reserve);
        other.append(toStringPiece());
        swap(other);
    }

    size_t internalCapacity() const
    {
        return buffer.capacity();
    }

    /// Read data directly into buffer.
    ///
    /// It may implement with readv(2)
    /// @return result of read(2), @c errno is saved
    ssize_t readFd(int fd, int *savedErrno);

private:
    char *begin()
    {
        return &*buffer.begin();
    }
    const char *begin() const
    {
        return &*buffer.begin();
    }
    void makeSpace(size_t len)
    {
        if (writableBytes() + prependableBytes() < len + kCheapPrepend)
        {
            //空余的尾长度和头长度小于所需要的大小,在尾部分配
            buffer.resize(writerIndex + len);
        }
        else
        {
            //空余的空间大于要分配，buffer前移
            assert(kCheapPrepend < readerIndex);
            size_t readable = readableBytes();
            std::copy(begin() + readerIndex,
                      begin() + writerIndex,
                      begin() + kCheapPrepend);
            readerIndex= kCheapPrepend;
            writerIndex = readerIndex + readable;
            assert(readable == readableBytes());
        }
    }
private:
    std::vector<char> buffer;
    size_t readerIndex;
    size_t writerIndex;
    static const char kCRLFCRLF[];
};
