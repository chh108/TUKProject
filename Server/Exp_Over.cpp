#include "Exp_Over.h"

Exp_Over::Exp_Over()
{
	_wsabuf.len = CHAT_SIZE;
	_wsabuf.buf = _buf;
	_comp_type = C_RECV;
	memset(_buf, 0, sizeof(_buf));
	ZeroMemory(&_over, sizeof(_over));
}

Exp_Over::Exp_Over(unsigned char* packet, int packet_size)
{
	_wsabuf.len = packet_size;
	_wsabuf.buf = _buf;
	ZeroMemory(&_over, sizeof(_over));
	_comp_type = C_SEND;
	memcpy(_buf, packet, packet_size);
}