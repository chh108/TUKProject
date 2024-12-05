#include "Players.h"

Players::Players()
{
	_x = 0, _y = 0;
	_name[0] = 0;
	_hp = 100;
	_exp = 0;
	_level = 1;
	_last_move_time = 0;
	_sector_x = -1, _sector_y = -1;
	_Pstate = P_IDLE;
	_Sstate = ST_FREE;
}

Players::~Players()
{
}

void Players::send_login_info_packet()
{
	SC_LOGIN_INFO_PACKET p;
	p.id = _id;
	p.size = sizeof(SC_LOGIN_INFO_PACKET);
	p.type = SC_LOGIN_INFO;
	p.x = _x;
	p.y = _y;
	do_send(&p, p.size);
}

void Players::send_login_fail_packet()
{
	SC_LOGIN_FAIL_PACKET p;
	p.size = sizeof(SC_LOGIN_FAIL_PACKET);
	p.type = SC_LOGIN_FAIL;
	do_send(&p, p.size);
}

void Players::send_add_object_packet(Players& player)
{
	SC_ADD_OBJECT_PACKET p;
	p.id = player._id;
	strcpy_s(p.name, player._name);
	p.size = sizeof(SC_ADD_OBJECT_PACKET);
	p.type = SC_ADD_OBJECT;
	p.x = player._x;
	p.y = player._y;
	do_send(&p, p.size);
}

void Players::send_remove_object_packet(int c_id)
{
	SC_REMOVE_OBJECT_PACKET p;
	p.id = c_id;
	p.size = sizeof(SC_REMOVE_OBJECT_PACKET);
	p.type = SC_REMOVE_OBJECT;
	do_send(&p, p.size);
}

void Players::send_move_object_packet(Players& player)
{
	SC_MOVE_OBJECT_PACKET p;
	p.id = player._id;
	p.size = sizeof(SC_MOVE_OBJECT_PACKET);
	p.type = SC_MOVE_OBJECT;
	p.x = player._x;
	p.y = player._y;
	p.move_time = player._last_move_time;
	do_send(&p, p.size);
}

void Players::send_chat_packet(int c_id, char* mess)
{
	SC_CHAT_PACKET p;
	p.id = c_id;
	strcpy_s(p.mess, mess);
	p.type = SC_CHAT;
	int mess_len = strlen(p.mess);
	p.size = sizeof(SC_CHAT_PACKET) - CHAT_SIZE + mess_len + 1;
	do_send(&p, p.size);
}

void Players::send_state_change_packet()
{

}