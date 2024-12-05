#pragma once

#include "stdafx.h"
#include "EXP_Over.h"
//#include "DBConnect.h"

class GameServer
{
private:
	GameServer() {};
	~GameServer() {};
public:
	static void Init();
	static void Clear();

	static void Print_Error(const char* msg, int err_no);

	static SOCKET CreateSocket();
	static void BindAnyAddress(SOCKET socket);
	static void Listen(SOCKET socket);

	static void DisConnect(int id);
	static void CloseSocket(SOCKET& socket);

	static HANDLE CreateHandle();
	static void RegisterSocket(HANDLE& iocp, SOCKET& socket, int id);

	//static bool DBInit(DBConnect* db);
	//static void LoadMap(const char* filename, int maptile[W_WIDTH][W_HEIGHT]);
};