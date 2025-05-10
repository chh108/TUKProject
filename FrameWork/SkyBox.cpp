//-----------------------------------------------------------------------------
// File: SkyBox.cpp
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "Shader.h"
#include "Texture.h"
#include "SkyBox.h"

CSkyBox::CSkyBox(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, CTexture* pTextureManager)
	: m_pTextureManager(pTextureManager)
{
	CreateShaderVariables(pd3dDevice, pd3dCommandList);
}

CSkyBox::~CSkyBox()
{
	if (m_pTexture) m_pTexture->Release();
}

void CSkyBox::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	XMFLOAT3 xmf3CameraPos = pCamera->GetPosition();
	SetPosition(xmf3CameraPos.x, xmf3CameraPos.y, xmf3CameraPos.z);

	CGameObject::Render(pd3dCommandList, pCamera);
}
