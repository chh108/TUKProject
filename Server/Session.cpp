#include "Session.h"
#include "GameServer.h"

Session::Session()
{
	_id = -1;
	_client = INVALID_SOCKET;
	_remain_data = 0;
}

Session::~Session()
{
}

void Session::do_recv()
{
	DWORD recv_flag = 0;
	ZeroMemory(&_exp_over._over, sizeof(OVERLAPPED));
	_exp_over._wsabuf.len = CHAT_SIZE - _remain_data;	//버퍼사이즈만큼? 처리하고 남은 패킷 조각이 버퍼에 들어있으면 그거 빼고 받아야됨
	_exp_over._wsabuf.buf = _exp_over._buf + _remain_data; //남은 사이즈가 있으면 잘린 조각 뒤부터 받아야됨
	int res = WSARecv(_client, &_exp_over._wsabuf, 1, 0, &recv_flag,
		&_exp_over._over, 0);
	if (0 != res) {
		int err_no = WSAGetLastError();
		if (WSA_IO_PENDING != err_no)
			GameServer::Print_Error("WSARecv", WSAGetLastError());
	}
}

void Session::do_send(void* packet, int packet_size)
{
	Exp_Over* data = new Exp_Over{ reinterpret_cast<unsigned char*>(packet),packet_size };
	WSASend(_client, &data->_wsabuf, 1, 0, 0, &data->_over, 0);
}