#include "stdafx.h"
#include "GameServer.h"
#include "Players.h"
#include "Packet.h"
//#include "DBConnect.h"

HANDLE g_hIocp;
std::array<Players, MAX_USER> users;
Exp_Over ex_over;
SOCKET g_s_socket, g_c_socket;
Packet* packet = new Packet;
int maptile[W_WIDTH][W_HEIGHT];

void worker(SOCKET server)
{
	//DBConnect* db = new DBConnect;
	//GameServer::DBInit(db);

	while (true)
	{
		DWORD num_bytes;
		ULONG_PTR key;
		LPOVERLAPPED over = nullptr;
		BOOL ret = GetQueuedCompletionStatus(g_hIocp, &num_bytes, &key, &over, INFINITE);
		Exp_Over* ex_over = reinterpret_cast<Exp_Over*>(over);

		if (FALSE == ret)
		{
			if (ex_over->_comp_type == C_ACCEPT)
			{
				std::cout << "Accept_Error";
				exit(-1);
			}
			else
			{
				std::cout << "GQCS Error on client " << key << "\n";
				for (auto& pl : users) {
					{
						std::lock_guard<std::mutex> ll{ pl._s_lock };
						if (pl._Sstate != ST_INGAME)continue;
					}
					if (pl._id == key)continue;
					pl.send_remove_object_packet(key);
				}
				GameServer::DisConnect(static_cast<int>(key));
				if (ex_over->_comp_type == C_SEND) delete ex_over;
				return;
			}
		}

		switch (ex_over->_comp_type) {
		case C_ACCEPT:
		{
			int cl_id = 0;
			for (int i = 0; i < MAX_USER; ++i) {
				if (users[i]._Sstate == ST_FREE) {
					cl_id = i;
					break;
				}
			}
			if (cl_id != -1)
			{
				{
					std::lock_guard<std::mutex> ll(users[cl_id]._s_lock);
					users[cl_id]._Sstate = ST_ALLOC;
				}
				users[cl_id]._client = g_c_socket;
				users[cl_id]._id = cl_id;
				GameServer::RegisterSocket(g_hIocp, g_c_socket, cl_id);
				users[cl_id].do_recv();
				g_c_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
			}
			else
			{
				std::cout << "Max user exceeded.\n";
				closesocket(g_c_socket);
			}
			ZeroMemory(&ex_over->_over, sizeof(OVERLAPPED));
			AcceptEx(server, g_c_socket, ex_over->_buf,
				0, sizeof(SOCKADDR) + 16, sizeof(SOCKADDR) + 16, 0, &ex_over->_over);
			break;
		}
		case C_RECV:
		{
			int remain_data = num_bytes + users[key]._remain_data;
			char* p = ex_over->_buf;
			while (remain_data > 0) {
				unsigned short packet_size = *reinterpret_cast<unsigned short*>(p);
				if (packet_size <= remain_data) {
					packet->process_packet(static_cast<int>(key), p, users/* db, maptile*/);
					p = p + packet_size;
					remain_data = remain_data - packet_size;
				}
				else break;
			}
			users[key]._remain_data = remain_data;
			if (remain_data > 0) {
				memcpy(ex_over->_buf, p, remain_data);
			}
			users[key].do_recv();
			break;
		}
		case C_SEND:
		{
			delete ex_over;
			break;
		}
		}
	}
}

int main()
{
	//GameServer::LoadMap("map.txt", maptile);
	GameServer::Init();
	SOCKET sv_socket = GameServer::CreateSocket();
	GameServer::BindAnyAddress(sv_socket);
	GameServer::Listen(sv_socket);

	g_hIocp = GameServer::CreateHandle();
	GameServer::RegisterSocket(g_hIocp, sv_socket, 9999);

	g_c_socket = GameServer::CreateSocket();
	ex_over._comp_type = C_ACCEPT;
	int res = AcceptEx(sv_socket, g_c_socket, ex_over._buf,
		0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16,
		nullptr, &ex_over._over);

	std::vector<std::thread>worker_threads;
	int num_threads = std::thread::hardware_concurrency();

	for (int i = 0; i < num_threads; ++i)
		worker_threads.emplace_back(worker, sv_socket);

	for (auto& w_th : worker_threads)
		w_th.join();

	GameServer::Clear();
}