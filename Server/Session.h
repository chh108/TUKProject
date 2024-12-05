#pragma once
#include "EXP_Over.h"

class Session
{
public:
	Session();
	~Session();

public:
	void do_recv();
	void do_send(void* packet, int packet_size);

public:
	int					_id;
	SOCKET				_client;
	int					_remain_data;
	Exp_Over			_exp_over;
};