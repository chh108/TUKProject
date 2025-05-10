#pragma once
//-----------------------------------------------------------------------------
// File: SkyBox.h
//-----------------------------------------------------------------------------

#include "Mesh.h"
#include "Object.h"
#include "stdafx.h"

class CSkyBox : public CGameObject
{
private:
	CTexture* m_pTextureManager; // TextureManager
	ID3D12Resource* m_pTexture = NULL; // Player Texture
	int m_TextureHeapIndex = -1; // TextureHeapIndex for Descriptor

public:
	CSkyBox(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, CTexture* pTexture);
	virtual ~CSkyBox();

	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera = NULL);
};