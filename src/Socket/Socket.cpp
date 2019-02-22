#include "Socket.h"
#include<string>
#include <fcntl.h>

Socket::Socket() noexcept : fd(-1)
{
	this->dotip = "0.0.0.0";
	this->port = 0;
}
Socket::Socket(const int _fd) noexcept : fd(_fd) {}
Socket::Socket(Socket &&rhs) noexcept : fd(rhs.fd), dotip(std::move(rhs.dotip)), port(rhs.port)
{
	rhs.fd = -1;
	rhs.port = 0;
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
	::sockaddr_in sock_addr{AF_INET, net_port, {net_addr}, {0}};
	return ::bind(this->fd, reinterpret_cast<struct sockaddr *>(&sock_addr), sizeof(sock_addr)) != -1;
}

bool Socket::listen() noexcept
{
	return ::listen(this->fd, SOMAXCONN) != -1;
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
	auto client_fd = ::accept(this->fd, nullptr, nullptr);
	 struct sockaddr_in sa;
	socklen_t len = sizeof(sa);
	::getpeername(client_fd,(struct sockaddr *)&sa,&len);
	Socket res = Socket(client_fd);
	res.port = sa.sin_port;
	res.dotip = inet_ntoa(sa.sin_addr);
	return res;
}

std::string Socket::getMessage() const noexcept
{
	std::string res="client ip : "+std::string(dotip)+" port: "+std::to_string(port)+" fd: "+std::to_string(this->fd); 
	return res;
}
long Socket::recv(void *buf, const size_t length) const noexcept
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
long Socket::recv(std::string &strbuf) const noexcept
{
	char *buf = &(*strbuf.begin());
	return this->recv(reinterpret_cast<void *>(buf), strbuf.size());
}

long Socket::send(void *buf, const size_t length) const noexcept
{
	size_t pos = 0;
	//block send
	while (pos < length)
	{
		auto sent_size = ::send(this->fd, reinterpret_cast<const char *>(buf) + pos, length - pos, 0);
		if (sent_size < 0)
		{
			return -1;
		}
		pos += sent_size;
	}
	return static_cast<long>(pos);
}
long Socket::send(std::string &strbuf) const noexcept
{
	char *buf = &(*strbuf.begin());
	return send(reinterpret_cast<void *>(buf), strbuf.size());
}

bool Socket::tcp_nodelay() const noexcept
{
	int flag = 1;
	if (::setsockopt(this->fd, IPPROTO_TCP, TCP_NODELAY, static_cast<void *>(&flag), sizeof(flag)) == -1)
	{
		return false;
	}
	return true;
}
bool Socket::tcp_nonblock() const noexcept
{
	return fcntl(fd, fcntl(fd, F_GETFL, 0) | O_NONBLOCK) != 1;
}
