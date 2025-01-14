//------------------------------------------------------- ----------------------
// File: Object.h
//-----------------------------------------------------------------------------

#pragma once

#include "Mesh.h"
#include "Camera.h"
#include "stdafx.h"
#include "FbxSceneContext.h"
#include "DebugLog.h"
#include <map>
#include <vector>

#define DIR_FORWARD					0x01
#define DIR_BACKWARD				0x02
#define DIR_LEFT					0x04
#define DIR_RIGHT					0x08
#define DIR_UP						0x10
#define DIR_DOWN					0x20
#define MAX_BONES 256

class CShader;
class CGameObject;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
class CAnimationController
{
public:
	CAnimationController(FbxScene* pfbxScene);
	~CAnimationController();

public:
	float m_fTime = 0.0f;
	
	FbxScene* m_pModelScene = NULL;

	std::vector<FbxScene*> m_pAnimationScenes;   // 애니메이션 씬 리스트
	std::vector<FbxAnimStack*> m_pAnimationStacks; // 애니메이션 스택 리스트
	std::vector<FbxTime> m_pfbxStartTimes;       // 애니메이션 시작 시간
	std::vector<FbxTime> m_pfbxStopTimes;        // 애니메이션 종료 시간
	std::vector<FbxTime> m_pfbxCurrentTimes;     // 애니메이션 현재 시간

	int m_nAnimationStack = 0;   // 현재 활성화된 애니메이션 스택

public:
	void LoadAnimation(FbxManager* pFbxManager, const std::string& animationFilePath, FbxScene* pModelScene);
	void LoadAnimations(FbxManager* pFbxManager, const std::vector<std::string>& animationFilePaths, FbxScene* pModelScene);

	void MergeModelAndAnimation(FbxScene* modelScene, FbxScene* animationScene);

	void SetPosition(int nAnimationStack, float fPosition);

	void SetAnimation(int nAnimationStack);
	void AdvanceTime(float fElapsedTime);
	
	void CheckAnimationKeyframes(int nAnimationStack);

	std::vector<FbxScene*> GetAnimationScenes() const {
		return m_pAnimationScenes;
	}
	FbxTime GetCurrentTime() { return m_pfbxCurrentTimes[m_nAnimationStack]; }
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
class CGameObject
{
protected:
	ID3D12Device* m_pd3dDevice = NULL; // Direct3D 디바이스
	ID3D12DescriptorHeap* m_pd3dCbvSrvDescriptorHeap = NULL; // 디스크립터 힙
	ID3D12DescriptorHeap* m_pd3dBoneOffsetSrvDescriptorHeap = NULL; // 본 데이터
	ID3D12DescriptorHeap* m_pd3dBoneTransSrvDescriptorHeap = NULL; // 본 행렬

	CTexture* m_pTextureManager; // Texture 매니저
	CShader* m_pShader; // Shader

private:
	int								m_nReferences = 0;

public:
	void AddRef();
	void Release();

public:
	CGameObject();
	CGameObject(CTexture* pTextureManager, ID3D12Device* pd3dDevice);
	virtual ~CGameObject();

public:
	char							m_pstrFrameName[64];

	FbxScene* m_pfbxScene = NULL;

	XMFLOAT4X4  					m_xmf4x4World;

	// 20241215 TextureResource
	ID3D12Resource* m_pTexture = NULL;
	UINT m_TextureHeapIndex = 0;

	// 20250114 BoneResources
	ID3D12Resource* m_pd3dBoneBuffer = NULL;
	D3D12_CPU_DESCRIPTOR_HANDLE m_BoneSrvHandle = {};
	UINT m_BoneMapIndex = 0;

	//20241216 Animation
	CAnimationController* m_pAnimationController = NULL;
	double							m_dFbxCurrentTime = 0;
	double							m_dAnimationStartTime = 0;
	double							m_dAnimationEndTime = 0;

	static CShader* m_pFbxShader; 
	static CShader* m_pFbxSkinnedShader;
	static CShader* m_pSkyBoxShader;
	static CShader* m_pStageShader;

	virtual void Animate(float fTimeElapsed);
	virtual void OnPrepareRender() {}
	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera);

	static void CGameObject::PrepareShaders(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature);

	void SetFbxShader() { CGameObject::SetShader(m_pFbxShader); }
	void SetFbxSkinnedShader() { CGameObject::SetShader(m_pFbxSkinnedShader); }
	void SetSkyBoxShader() { CGameObject::SetShader(m_pSkyBoxShader); }
	void SetMapShader(){ CGameObject::SetShader(m_pStageShader); }

	virtual void CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList);
	virtual void ReleaseShaderVariables();

	static void UpdateShaderVariable(ID3D12GraphicsCommandList* pd3dCommandList, XMFLOAT4X4* pxmf4x4World);
	static void UpdateShaderVariable(ID3D12GraphicsCommandList* pd3dCommandList, FbxAMatrix* pfbxf4x4World);

	virtual void ReleaseUploadBuffers();

	static void UploadBoneTransformToGPU(ID3D12GraphicsCommandList* pd3dCommandList);
	// 모델 위치 방향 관련 함수들
	XMFLOAT3 GetPosition();
	XMFLOAT3 GetLook();
	XMFLOAT3 GetUp();
	XMFLOAT3 GetRight();

	void SetPosition(float x, float y, float z);
	void SetPosition(XMFLOAT3 xmf3Position);
	void SetScale(float x, float y, float z);

	void MoveStrafe(float fDistance = 1.0f);
	void MoveUp(float fDistance = 1.0f);
	void MoveForward(float fDistance = 1.0f);

	void Rotate(float fPitch = 10.0f, float fYaw = 10.0f, float fRoll = 10.0f);
	void Rotate(XMFLOAT3* pxmf3Axis, float fAngle);
	void Rotate(XMFLOAT4* pxmf4Quaternion);

	void SetShader(CShader* pShader);

public:
	// 20241229 Animation
	void SetAnimationStack(int nAnimationStack) { m_pAnimationController->SetAnimation(nAnimationStack); }
	void ApplyAnimation();

	// 20241215 Texture Func
	void SetTexture(ID3D12Resource* pTexture, UINT textureHeapIndex) {
		m_pTexture = pTexture;
		m_TextureHeapIndex = textureHeapIndex;
	}

	ID3D12Resource* GetTexture() const { return m_pTexture; }
	UINT GetTextureHeapIndex() const { return m_TextureHeapIndex;  }
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
class CBlueObject : public CGameObject
{
private:
	CTexture* m_pObjTextureManager; // TextureManager
	ID3D12Resource* m_pObjTexture = NULL; // Obj Texture
	int m_TextureHeapIndex = -1; // TextureHeapIndex for Descriptor
public:
	CBlueObject(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList,
		ID3D12RootSignature* pd3dGraphicsRootSignature, FbxManager* pfbxSdkManager, CTexture* pTextureManager, FbxScene *pfbxScene);
	virtual ~CBlueObject();
};

