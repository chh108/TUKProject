#include "Packet.h"

void Packet::process_packet(int c_id, char* packet, std::array<Players, MAX_USER>& users
	/*DBConnect* db, int maptile[W_WIDTH][W_HEIGHT]*/)
{
	switch (packet[2]) {
	case CS_LOGIN:
	{
		CS_LOGIN_PACKET* p = reinterpret_cast<CS_LOGIN_PACKET*>(packet);
		strcpy_s(users[c_id]._name, p->name);

		//if (db->Login(users[c_id]._name, users[c_id]))
		//{
		//	users[c_id].send_login_info_packet();
		//	{
		//		std::lock_guard<std::mutex> ll{ users[c_id]._s_lock };
		//		users[c_id]._Sstate = ST_INGAME;
		//	}

		//	//set_sector(users[c_id]._x, users[c_id]._y, c_id, users);
		//}
		//else
		//	users[c_id].send_login_fail_packet();

	}
	break;

	case CS_MOVE:
	{
		CS_MOVE_PACKET* p = reinterpret_cast<CS_MOVE_PACKET*>(packet);
		users[c_id]._last_move_time = p->move_time;
		short x = users[c_id]._x;
		short y = users[c_id]._y;
		//switch (p->direction) {
		//case 0: if (y > 0)
		//{
		//	if (maptile[x][y - 1] != 1)
		//		y--;
		//	break;
		//}
		//case 1: if (y < W_HEIGHT - 1)
		//{
		//	if (maptile[x][y + 1] != 1)
		//		y++;
		//	break;
		//}
		//case 2: if (x > 0)
		//{
		//	if (maptile[x - 1][y] != 1)
		//		x--;
		//	break;
		//}
		//case 3: if (x < W_WIDTH - 1)
		//{
		//	if (maptile[x + 1][y] != 1)
		//		x++;
		//	break;
		//}
		//}
		//users[c_id]._x = x;
		//users[c_id]._y = y;

		////set_sector(x, y, c_id, users);

		users[c_id].send_move_object_packet(users[c_id]);
		for (int& id : users[c_id]._near)
		{
			users[c_id].send_move_object_packet(users[id]);
			users[id].send_move_object_packet(users[c_id]);
		}
	}
	break;

	case CS_CHAT:
	{
		CS_CHAT_PACKET* p = reinterpret_cast<CS_CHAT_PACKET*>(packet);
		for (auto& pl : users) {
			if (pl._Sstate != ST_INGAME) continue;
			if (pl._id == c_id) continue;
			pl.send_chat_packet(c_id, p->mess);
		}
	}
	break;

	case CS_ATTACK:
		break;

	case CS_TELEPORT:
	{
		CS_TELEPORT_PACKET* p = reinterpret_cast<CS_TELEPORT_PACKET*>(packet);
		while (true)
		{
			users[c_id]._x = rand() % 2000;
			users[c_id]._y = rand() % 2000;
			if (maptile[users[c_id]._x][users[c_id]._y] != 1)
				break;
		}
		if (db->AddPlayer(users[c_id]))
		{
			users[c_id].send_login_info_packet();
			{
				std::lock_guard<std::mutex> ll{ users[c_id]._s_lock };
				users[c_id]._Sstate = ST_INGAME;
			}

			//set_sector(users[c_id]._x, users[c_id]._y, c_id, users);
		}
	}
	break;

	case CS_LOGOUT:
		break;
	}
}

//void Packet::set_sector(int x, int y, int c_id, std::array<Player, MAX_USER>& users)
//{
//	if (users[c_id]._sector_x == x / sector_size && users[c_id]._sector_y == y / sector_size)
//	{
//		if (users[c_id]._sector_x != -1 && users[c_id]._sector_y != -1)
//		{
//			int dist = 0;
//			std::lock_guard<std::mutex> lock(sectors[users[c_id]._sector_x][users[c_id]._sector_x].mtx);
//			sector& new_sector = sectors[users[c_id]._sector_x][users[c_id]._sector_x];
//
//			std::vector<int> old_near = std::move(users[c_id]._near);
//			users[c_id]._near.clear();
//
//			for (const int& id : new_sector.sessions)
//			{
//				dist = (users[id]._x - users[c_id]._x) * (users[id]._x - users[c_id]._x) +
//					(users[id]._y - users[c_id]._y) * (users[id]._y - users[c_id]._y);
//				if (dist <= view_range * view_range)
//					users[c_id]._near.emplace_back(id);
//			}
//
//			for (const int& id : old_near)
//			{
//				if (std::find(users[c_id]._near.begin(), users[c_id]._near.end(), id) == users[c_id]._near.end())
//				{
//					users[c_id].send_remove_object_packet(id);
//					users[id].send_remove_object_packet(c_id);
//				}
//			}
//
//			for (const int& id : users[c_id]._near)
//			{
//				if (std::find(old_near.begin(), old_near.end(), id) == old_near.end())
//					users[c_id].send_add_object_packet(users[id]);
//			}
//		}
//		return;
//	}
//
//	int new_sector_x = x / sector_size;
//	int new_sector_y = y / sector_size;
//
//	if (users[c_id]._sector_x != -1 && users[c_id]._sector_y != -1)
//	{
//		std::lock_guard<std::mutex> lock(sectors[users[c_id]._sector_x][users[c_id]._sector_y].mtx);
//		sector& old_sector = sectors[users[c_id]._sector_x][users[c_id]._sector_y];
//		old_sector.sessions.erase(std::remove(old_sector.sessions.begin(), old_sector.sessions.end(), c_id), old_sector.sessions.end());
//	}
//
//	{
//		std::lock_guard<std::mutex> lock(sectors[new_sector_x][new_sector_y].mtx);
//		sector& new_sector = sectors[new_sector_x][new_sector_y];
//		sectors[new_sector_x][new_sector_y].sessions.emplace_back(c_id);
//
//		users[c_id]._sector_x = new_sector_x;
//		users[c_id]._sector_y = new_sector_y;
//
//		int dist = 0;
//		std::vector<int> old_near = std::move(users[c_id]._near);
//		users[c_id]._near.clear();
//		for (const int& id : new_sector.sessions)
//		{
//			dist = (users[id]._x - users[c_id]._x) * (users[id]._x - users[c_id]._x) +
//				(users[id]._y - users[c_id]._y) * (users[id]._y - users[c_id]._y);
//			if (dist <= view_range * view_range)
//				users[c_id]._near.emplace_back(id);
//		}
//
//		for (const int& id : old_near)
//		{
//			if (std::find(users[c_id]._near.begin(), users[c_id]._near.end(), id) == users[c_id]._near.end())
//				users[c_id].send_remove_object_packet(id);
//		}
//
//		for (const int& id : users[c_id]._near)
//		{
//			if (std::find(old_near.begin(), old_near.end(), id) == old_near.end())
//				users[c_id].send_add_object_packet(users[id]);
//		}
//	}
//}