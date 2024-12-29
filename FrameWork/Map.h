//-----------------------------------------------------------------------------
// File: Map.h
//-----------------------------------------------------------------------------
#pragma once

#include "stdafx.h"
#include "FbxSceneContext.h"
#include "Texture.h"
#include "Object.h"
#include "Shader.h"

class CMap : public CGameObject
{
public:
    CMap(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, 
        FbxManager* pfbxSdkManager, CTexture* pTextureManager, FbxScene* pfbxScene);
    ~CMap();

protected:
    // CFbxRenderInfo* m_pRenderInfo = NULL; // FBX 렌더링 정보
    FbxScene* m_pFbxScene = NULL; // FBX 씬 데이터
    CTexture* m_pTextureManager = NULL;   // 텍스처 관리
    std::vector<ID3D12Resource*> m_pMapTextures; // 로드한 맵의 텍스처들

    ID3D12GraphicsCommandList* m_pd3dCommandList = NULL;
    ID3D12RootSignature* m_pd3dGraphicsRootSignature = NULL;
};
