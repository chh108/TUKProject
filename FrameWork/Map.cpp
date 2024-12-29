//-----------------------------------------------------------------------------
// File: Map.cpp
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "FbxSceneContext.h"
#include "Texture.h"
#include "Object.h"
#include "Shader.h"
#include "Map.h"

CMap::CMap(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, FbxManager* pFbxManager)
{
    m_pd3dCommandList = pd3dCommandList;
    m_pd3dGraphicsRootSignature = pd3dGraphicsRootSignature;
    m_pTextureManager = new CTexture();
}

CMap::~CMap()
{
    // if (m_pRenderInfo) delete m_pRenderInfo;
    if (m_pTextureManager) delete m_pTextureManager;
    if (m_pTerrainBuffer) m_pTerrainBuffer->Release();
    if (m_pFbxScene) m_pFbxScene->Destroy();
}

void CMap::LoadMap(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList,
    ID3D12RootSignature* pd3dGraphicsRootSignature, FbxManager* pfbxSdkManager, 
    const std::string& fbxFilePath, const std::string& texturePath, const std::string& terrainFilePath)
{
    // Load FBX Scene
    m_pFbxScene = LoadFbxSceneFromFile(pd3dDevice, pd3dCommandList, pfbxSdkManager, const_cast<char*>(fbxFilePath.c_str()));
    if (!m_pFbxScene) {
        printf("Failed to load FBX file: %s\n", fbxFilePath.c_str());
        return;
    }

    // Create Meshes from FBX Scene
    CreateMeshFromFbxNodeHierarchy(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature, m_pFbxScene->GetRootNode());

    // Load Textures
    m_pTextureManager->LoadTexture(texturePath, m_pd3dCommandList);

    // Load Terrain Data
    LoadTerrain(terrainFilePath);
}

void CMap::LoadTerrain(const std::string& terrainFilePath)
{
    // RAW 파일 로드
    FILE* pFile = fopen(terrainFilePath.c_str(), "rb");
    if (!pFile) {
        printf("Failed to load terrain file: %s\n", terrainFilePath.c_str());
        return;
    }

    // Terrain 높이 데이터를 읽어옴
    const int width = 256;
    const int height = 256;
    float* heightData = new float[width * height];
    fread(heightData, sizeof(float), width * height, pFile);
    fclose(pFile);

    // Terrain Vertex Buffer 생성
    // (실제 구현 시, Vertex 및 Index 데이터를 생성하고 Direct3D 버퍼를 생성합니다.)

    delete[] heightData;
}

void CMap::Render(ID3D12GraphicsCommandList* pd3dCommandList)
{
    if (!m_pFbxScene) return;

    // Render FBX Node Hierarchy
    FbxTime time;
    RenderFbxNodeHierarchy(pd3dCommandList, m_pFbxScene->GetRootNode(), time, {});

    // Render Terrain (추가 구현 필요)
}