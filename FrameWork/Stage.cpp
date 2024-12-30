//-----------------------------------------------------------------------------
// File: Stage.cpp
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "FbxSceneContext.h"
#include "Texture.h"
#include "Object.h"
#include "Shader.h"
#include "Stage.h"

CStage::CStage(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList,
    ID3D12RootSignature* pd3dGraphicsRootSignature, FbxManager* pfbxSdkManager, CTexture* pTextureManager, FbxScene* pfbxScene)
    :CGameObject(pTextureManager, pd3dDevice), m_pStageTextures(NULL)
{
	m_pfbxScene = pfbxScene;
	if (!m_pfbxScene)
	{
		if (pTextureManager) {
			m_pTextureManager = pTextureManager;
		}
		else {
			// debugLog << "CBlueObject : Texture Manager is NULL!" << std::endl;
		}

		m_pfbxScene = ::LoadFbxSceneFromFile(pd3dDevice, pd3dCommandList, pfbxSdkManager, "Map/ForestMap.fbx");
		std::vector<ID3D12Resource*> textures = m_pTextureManager->ExtractTexturesWithCustom(m_pfbxScene->GetRootNode(), "Map/Objects/Textures/", pd3dCommandList);

		if (!textures.empty()) {
			m_pTexture = textures[0]; // 첫 번째 텍스처를 사용
			//debugLog << "First texture loaded for Player: " << m_pTexture << std::endl;
		}
		else {
			// std::cerr << "No textures loaded for Player." << std::endl;
		}

		::CreateMeshFromFbxNodeHierarchy(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature, m_pfbxScene->GetRootNode());
	}

	//CStageShader* pStageShader = new CStageShader();
	//pStageShader->CreateShader(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature, SHADER_TYPE::Map);
	//pStageShader->CreateShaderVariables(pd3dDevice, pd3dCommandList);
}

CStage::~CStage()
{
    // if (m_pRenderInfo) delete m_pRenderInfo;
    if (m_pTextureManager) delete m_pTextureManager;
    if (m_pFbxScene) m_pFbxScene->Destroy();
}