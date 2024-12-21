#pragma once

#include "stdafx.h"
#include "Player.h"

class CPlayerManager
{
private:
	CPlayerManager();
	virtual ~CPlayerManager();

public:
	static		CPlayerManager* Get_Instance(void)
	{
		if (!m_pInstance)
		{
			m_pInstance = new CPlayerManager;
		}

		return m_pInstance;
	}

	static	void	Destroy_Instance(void)
	{
		if (m_pInstance)
		{
			delete m_pInstance;
			m_pInstance = nullptr;
		}
	}

public:
	bool Get_Aiming() { return m_bAiming; };
	void Set_Aiming(bool _Aiming) { m_bAiming = _Aiming; }

private:
	static CPlayerManager* m_pInstance;

private:
	bool m_bAiming;

};