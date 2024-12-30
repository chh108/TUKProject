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
	CreateCBV(pd3dDevice, pd3dCommandList);
	CStageShader* pStageShader = new CStageShader();
	pStageShader->CreateShader(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature, SHADER_TYPE::Map);
	pStageShader->CreateShaderVariables(pd3dDevice, pd3dCommandList);
}

CStage::~CStage()
{
	// if (m_pRenderInfo) delete m_pRenderInfo;
	if (m_pTextureManager) delete m_pTextureManager;
	if (m_pFbxScene) m_pFbxScene->Destroy();
}

void CStage::CreateCBV(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	debugLog << "Start Create CBV " << std::endl;

	// Map CBV 크기 계산 (256의 배수로 맞춤)
	UINT ncbElementBytes = ((sizeof(VS_CB_STAGE_INFO) + 255) & ~255);
	debugLog << "CBV Elements : " << ncbElementBytes << std::endl;
	// CBV 생성
	m_pd3dcbStage = ::CreateBufferResource(
		pd3dDevice, pd3dCommandList, NULL, ncbElementBytes,
		D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, NULL);

	// Map CBV 매핑
	m_pd3dcbStage->Map(0, NULL, (void**)& m_pcbMappedStageInfo);

	if (!m_pcbMappedStageInfo)
	{
		debugLog << "Failed to map CBV data for stage." << std::endl;
	}
	// CBV 디스크립터 생성
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = m_pd3dcbStage->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = ncbElementBytes;

	// 디스크립터 힙에서 b3 위치에 추가
	D3D12_CPU_DESCRIPTOR_HANDLE cbvHandle = m_pd3dCbvSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	cbvHandle.ptr += 3 * m_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	pd3dDevice->CreateConstantBufferView(&cbvDesc, cbvHandle);

	debugLog << "Map CBV created for b3, GPU Address: " << cbvDesc.BufferLocation << std::endl;
}

void CStage::UpdateCBV(ID3D12GraphicsCommandList* pd3dCommandList)
{
	// gMapWorldMatrix를 CBV에 복사
	XMFLOAT4X4 xmfStageWorldMatrix; // 맵의 월드 행렬 계산
	::memcpy(m_pcbMappedStageInfo, &xmfStageWorldMatrix, sizeof(XMFLOAT4X4));

	// CBV 바인딩
	D3D12_GPU_VIRTUAL_ADDRESS d3dGpuVirtualAddress = m_pd3dcbStage->GetGPUVirtualAddress();
	pd3dCommandList->SetGraphicsRootConstantBufferView(3, d3dGpuVirtualAddress); // b3에 설정
}