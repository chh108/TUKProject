#include "stdafx.h"
#include "PlayerManager.h"

CPlayerManager* CPlayerManager::m_pInstance = nullptr;

CPlayerManager::CPlayerManager()
{
	m_bAiming = false;
}

CPlayerManager::~CPlayerManager()
{
}