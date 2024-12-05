#pragma once
#include "Session.h"
#include "stdafx.h"

enum P_STATE { P_IDLE, P_MOVE, P_ATTACK, P_DIE };
enum S_STATE { ST_FREE, ST_ALLOC, ST_INGAME };
class Players : public Session
{
public:
	Players();
	~Players();

public:
	void send_login_info_packet();
	void send_login_fail_packet();
	void send_add_object_packet(Players& player);
	void send_remove_object_packet(int c_id);
	void send_move_object_packet(Players& player);
	void send_chat_packet(int c_id, char* mess);
	void send_state_change_packet();

public:
	int			_x, _y;
	char		_name[NAME_SIZE];
	int			_hp;
	int			_exp;
	int			_level;
	int			_last_move_time;
	int			_sector_x, _sector_y;
	P_STATE		_Pstate;
	S_STATE		_Sstate;
	std::mutex  _s_lock;
	std::vector	<int> _near;
};