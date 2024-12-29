//-----------------------------------------------------------------------------
// File: Map.h
//-----------------------------------------------------------------------------
#pragma once

#include "stdafx.h"
#include "FbxSceneContext.h"
#include "Texture.h"
#include "Object.h"
#include "Shader.h"

class CMap {
public:
    CMap(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, FbxManager* pFbxManager);
    ~CMap();

    void LoadMap(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList,
        ID3D12RootSignature* pd3dGraphicsRootSignature, FbxManager* pfbxSdkManager,
        const std::string& fbxFilePath, const std::string& texturePath, const std::string& terrainFilePath);
    void Render(ID3D12GraphicsCommandList* pd3dCommandList);

private:
    // CFbxRenderInfo* m_pRenderInfo = NULL; // FBX 렌더링 정보
    CTexture* m_pTextureManager = NULL;   // 텍스처 관리
    ID3D12Resource* m_pTerrainBuffer = NULL; // Terrain 데이터 버퍼
    ID3D12GraphicsCommandList* m_pd3dCommandList = NULL;
    ID3D12RootSignature* m_pd3dGraphicsRootSignature = NULL;

    FbxScene* m_pFbxScene = NULL; // FBX 씬 데이터

    void LoadTerrain(const std::string& terrainFilePath);
};
