#include "Socket.h"
namespace Socket
{
	Socket::Socket() noexcept :fd(-1){this->dotip="0.0.0.0";this->port=0;}
	Socket::Socket(const int _fd) noexcept: fd(_fd) {}
	//�ƶ�����
	Socket::Socket(Socket&& rhs) noexcept: fd(rhs.fd),dotip(std::move(rhs.dotip)),port(rhs.port)  {
		rhs.fd = -1;
		rhs.port=0;
	}
	
	bool Socket::open() noexcept
	{
		this->close();
		this->fd = ::socket(AF_INET, SOCK_STREAM, 0);
		return this->is_open();
	}

	bool Socket::bind(const uint16_t port) noexcept
	{
		auto net_port = ::htons(port);
		auto net_addr = ::htonl(INADDR_ANY);
		::sockaddr_in sock_addr{ AF_INET,net_port,{net_addr},{0}};
		return ::bind(this->fd, reinterpret_cast<struct sockaddr*>(&sock_addr), sizeof(sock_addr)) != -1;
	}

	bool Socket::listen() noexcept
	{
		return ::listen(this->fd, SOMAXCONN)!=-1;
	}
	
	bool Socket::close() noexcept
	{
		if (is_open())
		{
			int result = ::close(this->fd);
			if (result == 0)
			{
				this->fd = -1;
				return true;
			}
		}
		return false;
	}
	
	int Socket::get_fd() const noexcept
	{
		return this->fd;
	}

	bool Socket::is_open() const noexcept
	{
		return this->fd != -1;
	}

	Socket Socket::accept() const noexcept
	{
		sockaddr_in client_addr;
		socklen_t len;
		auto client_fd = ::accept(this->fd,nullptr, nullptr);
		Socket res=Socket(client_fd);
		res.port=client_addr.sin_port;
		res.dotip=inet_ntoa(client_addr.sin_addr);
		return res;
	}
	Socket Socket::nonblock_accept(const std::chrono::milliseconds& timeout) const noexcept
	{
		int client_fd = -1;
		struct ::pollfd evnt {this->fd,POLLIN,0};
		if (::poll(&evnt, 1, static_cast<int>(timeout.count())) && evnt.revents)
		{
			return this->accept();
		}
		return Socket(-1);
	}


	long Socket::recv(void* buf, const size_t length) const noexcept
	{
		long ret = -1;
		while (1)
		{
			ret = ::recv(this->fd, buf, length, MSG_WAITALL);
			if (ret < 0 && (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR))
				continue;
			else
				break;
		}
		return ret;
	}
	long Socket::recv(std::string& strbuf) const noexcept
	{
		char* buf=&(*strbuf.begin());
		return this->recv(reinterpret_cast<void*>(buf), strbuf.size());
	}
	long Socket::nonblock_recv(void* buf, const size_t length,std::chrono::milliseconds& timeout) const noexcept
	{
		struct ::pollfd evnt{this->fd,POLLIN,0};
		if (::poll(&evnt, 1, static_cast<int>(timeout.count())) && evnt.revents)
		{
			// no MSG_WAITALL
			return ::recv(this->fd, buf, length, 0);
		}
		return -1;
	}
	long Socket::nonblock_recv(std::string& strbuf,std::chrono::milliseconds& timeout) const noexcept
	{
		char* buf=&(*strbuf.begin());
		return this->nonblock_recv(reinterpret_cast<void*>(buf),strbuf.size(), timeout);
	}


	long Socket::send(void *buf, const size_t length) const noexcept
	{
		size_t pos = 0;
		//block send 
		while (pos < length)
		{
			auto sent_size = ::send(this->fd, reinterpret_cast<const char*>(buf) + pos, length - pos, 0);
			if (sent_size < 0)
			{
				return -1;
			}
			pos += sent_size;
		}
		return static_cast<long>(pos);
	}
	long Socket::send(std::string& strbuf) const noexcept
	{
		char* buf=&(*strbuf.begin());
		return send(reinterpret_cast<void*>(buf), strbuf.size());
	}

	bool Socket::tcp_nodelay() const noexcept
	{
		int flag=1;
		if(::setsockopt(this->fd,IPPROTO_TCP,TCP_NODELAY,static_cast<void*>(&flag),sizeof(flag))==-1){
			return false;
		}
		return true;
	}

}