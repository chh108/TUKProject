//-----------------------------------------------------------------------------
// File: Object.cpp
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "Object.h"
#include "Shader.h"
#include "Mesh.h"
#include "Scene.h"
#include "DebugLog.h"
#include "texture.h"

std::vector<std::string> ObjectAnimations = {
	"Model/Character/Animations/IDLE.fbx",
	"Model/Character/Animations/WALK.fbx"
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
CAnimationController::CAnimationController(FbxScene* pfbxScene) : m_pModelScene(pfbxScene)
{
	if (!m_pModelScene)
	{
		debugLog << "Model Scene is NULL." << std::endl;
	}
}

CAnimationController::~CAnimationController() 
{
	for (FbxScene* scene : m_pAnimationScenes)
	{
		scene->Destroy();
	}
	m_pAnimationScenes.clear();
	m_pAnimationStacks.clear();
	m_pfbxStartTimes.clear();
	m_pfbxStopTimes.clear();
	m_pfbxCurrentTimes.clear();
};

void CAnimationController::LoadAnimation(FbxManager* pFbxManager, const std::string& animationFilePath)
{
	FbxScene* pAnimationScene = LoadFbxSceneFromFile(nullptr, nullptr, pFbxManager, animationFilePath.c_str());
	if (!pAnimationScene) {
		std::cerr << "Failed to load animation FBX: " << animationFilePath << std::endl;
		return;
	}

	m_pAnimationScenes.push_back(pAnimationScene);

	FbxArray<FbxString*> fbxAnimationStackNames;
	pAnimationScene->FillAnimStackNameArray(fbxAnimationStackNames);

	for (int i = 0; i < fbxAnimationStackNames.Size(); i++) {
		FbxAnimStack* pAnimStack = pAnimationScene->FindMember<FbxAnimStack>(fbxAnimationStackNames[i]->Buffer());
		m_pAnimationStacks.push_back(pAnimStack);

		FbxTakeInfo* pTakeInfo = pAnimationScene->GetTakeInfo(*fbxAnimationStackNames[i]);
		if (pTakeInfo) {
			m_pfbxStartTimes.push_back(pTakeInfo->mLocalTimeSpan.GetStart());
			m_pfbxStopTimes.push_back(pTakeInfo->mLocalTimeSpan.GetStop());
		}
		else {
			FbxTimeSpan defaultTimeSpan;
			pAnimationScene->GetGlobalSettings().GetTimelineDefaultTimeSpan(defaultTimeSpan);
			m_pfbxStartTimes.push_back(defaultTimeSpan.GetStart());
			m_pfbxStopTimes.push_back(defaultTimeSpan.GetStop());
		}
		m_pfbxCurrentTimes.push_back(m_pfbxStartTimes.back());
	}

	FbxArrayDelete(fbxAnimationStackNames);
}

void CAnimationController::LoadAnimations(FbxManager* pFbxManager, const std::vector<std::string>& animationFilePaths)
{
	for (const std::string& filePath : animationFilePaths)
	{
		LoadAnimation(pFbxManager, filePath);
	}

	for (size_t i = 0; i < m_pAnimationStacks.size(); ++i)
	{
		std::string fileName = animationFilePaths[i].substr(animationFilePaths[i].find_last_of("/\\") + 1);
		fileName = fileName.substr(0, fileName.find_last_of("."));  // 확장자 제거

		m_pAnimationStacks[i]->SetName(fileName.c_str());

		debugLog << "[Loaded Animations] Loaded Animation Stack [" << i << "]: "
			<< m_pAnimationStacks[i]->GetName() << std::endl;
		debugLog << "START TIME: " << m_pfbxStartTimes[i].GetSecondDouble() << "s, "
			<< "STOP TIME: " << m_pfbxStopTimes[i].GetSecondDouble() << "s" << std::endl;
	}
}

void CAnimationController::SetPosition(int nAnimationStack, float fPosition)
{
	m_pfbxCurrentTimes[nAnimationStack].SetSecondDouble(fPosition);;
}

void CAnimationController::SetAnimation(int nAnimationStack)
{
	if (nAnimationStack < 0 || nAnimationStack >= static_cast<int>(m_pAnimationStacks.size())) {
		debugLog << "[SetAnimation] Invalid animation stack index: " << nAnimationStack << std::endl;
		return;
	}

	m_nAnimationStack = nAnimationStack;
	m_pModelScene->SetCurrentAnimationStack(m_pAnimationStacks[nAnimationStack]);

	debugLog << "[SetAnimation] Current Animation Stack Set: "
		<< m_pAnimationStacks[nAnimationStack]->GetName() << std::endl;
}

void CAnimationController::AdvanceTime(float fElapsedTime)
{
	FbxTime fbxElapsedTime;
	fbxElapsedTime.SetSecondDouble(fElapsedTime);

	m_pfbxCurrentTimes[m_nAnimationStack] += fbxElapsedTime;

	if (m_pfbxCurrentTimes[m_nAnimationStack] > m_pfbxStopTimes[m_nAnimationStack]) {
		m_pfbxCurrentTimes[m_nAnimationStack] = m_pfbxStartTimes[m_nAnimationStack];
	}
}

void CAnimationController::CheckAnimationKeyframes(int nAnimationStack)
{
	if (nAnimationStack < 0 || nAnimationStack >= static_cast<int>(m_pAnimationStacks.size())) {
		std::cerr << "Invalid animation stack index: " << nAnimationStack << std::endl;
		return;
	}

	FbxAnimStack* animStack = m_pAnimationStacks[nAnimationStack];
	FbxAnimLayer* animLayer = animStack->GetMember<FbxAnimLayer>();
	if (animLayer)
	{
		FbxAnimCurve* animCurve = m_pModelScene->GetRootNode()->LclTranslation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_X);
		if (animCurve)
		{
			int keyCount = animCurve->KeyGetCount();
			std::cout << "Total Keyframes: " << keyCount << std::endl;
			for (int k = 0; k < keyCount; k++)
			{
				FbxTime keyTime = animCurve->KeyGetTime(k);
				std::cout << "Keyframe[" << k << "] Time: " << keyTime.GetSecondDouble() << " seconds" << std::endl;
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
CGameObject::CGameObject() 
	: m_pd3dCbvSrvDescriptorHeap(NULL), m_pd3dBoneOffsetSrvDescriptorHeap(NULL),
	m_pd3dBoneTransSrvDescriptorHeap(NULL), m_pd3dDevice(NULL), m_pTextureManager(NULL) {
	m_xmf4x4World = Matrix4x4::Identity();
}

CGameObject::CGameObject(CTexture* pTextureManager, ID3D12Device* pd3dDevice)
	: m_pTextureManager(pTextureManager), m_pd3dDevice(pd3dDevice) {

	// debugLog << "CGameObject Constructor - Device: " << pd3dDevice << std::endl;

	if (!m_pTextureManager) {
		// debugLog << "CGameObject: Texture Manager is NULL during initialization." << std::endl;
	}
	else {
		// debugLog << "CGameObject: Texture Manager successfully initialized." << std::endl;
		m_pd3dCbvSrvDescriptorHeap = pTextureManager->GetDescriptorHeap(); // 힙 참조
	}
	m_xmf4x4World = Matrix4x4::Identity();
}

CGameObject::~CGameObject()
{
#ifndef _WITH_FBX_SCENE_INSTANCING
	if (m_pfbxScene) ::ReleaseMeshFromFbxNodeHierarchy(m_pfbxScene->GetRootNode());
	if (m_pfbxScene) m_pfbxScene->Destroy();
#endif
	if (m_pAnimationController) delete m_pAnimationController;
}

void CGameObject::AddRef() 
{ 
	m_nReferences++; 
}

void CGameObject::Release() 
{ 
	if (--m_nReferences <= 0) delete this; 
}

void CGameObject::Animate(float fTimeElapsed)
{
	if (m_pAnimationController)
	{
		m_pAnimationController->AdvanceTime(fTimeElapsed);	
		FbxTime fbxCurrentTime = m_pAnimationController->GetCurrentTime();
		::AnimateFbxNodeHierarchy(m_pfbxScene->GetRootNode(), fbxCurrentTime);
	}
}

void CGameObject::Render(ID3D12GraphicsCommandList *pd3dCommandList, CCamera *pCamera)
{
	if (m_pTexture) {
		// debugLog << "CGameObject::Render - Texture valid: " << m_pTexture << std::endl;
	}
	else {
		// debugLog << "CGameObject::Render - Texture is NULL." << std::endl;
	}

	OnPrepareRender();

	ID3D12DescriptorHeap* ppHeaps[] = { m_pd3dCbvSrvDescriptorHeap, m_pd3dBoneOffsetSrvDescriptorHeap, m_pd3dBoneTransSrvDescriptorHeap };
	pd3dCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	// 20241216 텍스처 로딩
	if (m_pTexture) {
		D3D12_GPU_DESCRIPTOR_HANDLE srvHandle = m_pd3dCbvSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
		srvHandle.ptr += m_TextureHeapIndex * m_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		pd3dCommandList->SetGraphicsRootDescriptorTable(2, srvHandle); // Root ParameterIndex 2
	}
	else
	{
		debugLog << "Failed To Bind SRV.\n";
	}

	// 20241216 애니메이션 작업
	if (m_pfbxScene && m_pAnimationController)
	{
		D3D12_GPU_DESCRIPTOR_HANDLE boneOffsetSrvHandle = m_pd3dBoneOffsetSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
		D3D12_GPU_DESCRIPTOR_HANDLE boneTransformSrvHandle = m_pd3dBoneTransSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();

		pd3dCommandList->SetGraphicsRootDescriptorTable(6, boneOffsetSrvHandle); // Root ParameterIndex 6
		pd3dCommandList->SetGraphicsRootDescriptorTable(7, boneTransformSrvHandle); // Root ParameterIndex 7
		debugLog << "Set Bone SRVS" << std::endl;

		// Animation Set
		ApplyAnimation();
		FbxAMatrix fbxf4x4World = ::XmFloat4x4MatrixToFbxMatrix(m_xmf4x4World);
		::RenderFbxNodeHierarchy(pd3dCommandList, m_pfbxScene->GetRootNode(), m_pAnimationController->GetCurrentTime(), fbxf4x4World);
	}
}

CShader* CGameObject::m_pFbxShader = NULL;
CShader* CGameObject::m_pFbxSkinnedShader = NULL;
CShader* CGameObject::m_pSkyBoxShader = NULL;
CShader* CGameObject::m_pStageShader = NULL;


void CGameObject::PrepareShaders(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature)
{
	m_pFbxShader = new CFbxModelShader();
	m_pFbxShader->CreateShader(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature, SHADER_TYPE::FbxModel);
	m_pFbxShader->CreateShaderVariables(pd3dDevice, pd3dCommandList);

	m_pFbxSkinnedShader = new CFbxSkinnedModelShader();
	m_pFbxSkinnedShader->CreateShader(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature, SHADER_TYPE::FbxSkinnedModel);
	m_pFbxSkinnedShader->CreateShaderVariables(pd3dDevice, pd3dCommandList);

	//m_pSkyBoxShader = new CSkyBoxShader();
	//m_pSkyBoxShader->CreateShader(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature, SHADER_TYPE::SkyBox);
	//m_pSkyBoxShader->CreateShaderVariables(pd3dDevice, pd3dCommandList);

	//m_pStageShader = new CStageShader();
	//m_pStageShader->CreateShader(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature, SHADER_TYPE::Stage);
	//m_pStageShader->CreateShaderVariables(pd3dDevice, pd3dCommandList);
}

void CGameObject::CreateShaderVariables(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList)
{
}

void CGameObject::UpdateShaderVariables(ID3D12GraphicsCommandList *pd3dCommandList)
{
}

void CGameObject::UpdateShaderVariable(ID3D12GraphicsCommandList *pd3dCommandList, XMFLOAT4X4 *pxmf4x4World)
{
	XMFLOAT4X4 xmf4x4World;
	XMStoreFloat4x4(&xmf4x4World, XMMatrixTranspose(XMLoadFloat4x4(pxmf4x4World)));
	pd3dCommandList->SetGraphicsRoot32BitConstants(1, 16, &xmf4x4World, 0);
}

void CGameObject::UpdateShaderVariable(ID3D12GraphicsCommandList *pd3dCommandList, FbxAMatrix *pfbxf4x4World)
{
	XMFLOAT4X4 xmf4x4World = ::FbxMatrixToXmFloat4x4Matrix(pfbxf4x4World);
	XMStoreFloat4x4(&xmf4x4World, XMMatrixTranspose(XMLoadFloat4x4(&xmf4x4World)));
	pd3dCommandList->SetGraphicsRoot32BitConstants(1, 16, &xmf4x4World, 0);
}

void CGameObject::ReleaseShaderVariables()
{
}

void CGameObject::ReleaseUploadBuffers()
{
#ifndef _WITH_FBX_SCENE_INSTANCING
	if (m_pfbxScene) ::ReleaseUploadBufferFromFbxNodeHierarchy(m_pfbxScene->GetRootNode());
#endif
}

void CGameObject::SetPosition(float x, float y, float z)
{
	m_xmf4x4World._41 = x;
	m_xmf4x4World._42 = y;
	m_xmf4x4World._43 = z;
}

void CGameObject::SetPosition(XMFLOAT3 xmf3Position)
{
	SetPosition(xmf3Position.x, xmf3Position.y, xmf3Position.z);
}

void CGameObject::SetScale(float x, float y, float z)
{
	XMMATRIX mtxScale = XMMatrixScaling(x, y, z);
	m_xmf4x4World = Matrix4x4::Multiply(mtxScale, m_xmf4x4World);
}

XMFLOAT3 CGameObject::GetPosition()
{
	return(XMFLOAT3(m_xmf4x4World._41, m_xmf4x4World._42, m_xmf4x4World._43));
}

XMFLOAT3 CGameObject::GetLook()
{
	return(Vector3::Normalize(XMFLOAT3(m_xmf4x4World._31, m_xmf4x4World._32, m_xmf4x4World._33)));
}

XMFLOAT3 CGameObject::GetUp()
{
	return(Vector3::Normalize(XMFLOAT3(m_xmf4x4World._21, m_xmf4x4World._22, m_xmf4x4World._23)));
}

XMFLOAT3 CGameObject::GetRight()
{
	return(Vector3::Normalize(XMFLOAT3(m_xmf4x4World._11, m_xmf4x4World._12, m_xmf4x4World._13)));
}

void CGameObject::MoveStrafe(float fDistance)
{
	XMFLOAT3 xmf3Position = GetPosition();
	XMFLOAT3 xmf3Right = GetRight();
	xmf3Position = Vector3::Add(xmf3Position, xmf3Right, fDistance);
	CGameObject::SetPosition(xmf3Position);
}

void CGameObject::MoveUp(float fDistance)
{
	XMFLOAT3 xmf3Position = GetPosition();
	XMFLOAT3 xmf3Up = GetUp();
	xmf3Position = Vector3::Add(xmf3Position, xmf3Up, fDistance);
	CGameObject::SetPosition(xmf3Position);
}

void CGameObject::MoveForward(float fDistance)
{
	XMFLOAT3 xmf3Position = GetPosition();
	XMFLOAT3 xmf3Look = GetLook();
	xmf3Position = Vector3::Add(xmf3Position, xmf3Look, fDistance);
	CGameObject::SetPosition(xmf3Position);
}

void CGameObject::Rotate(float fPitch, float fYaw, float fRoll)
{
	XMMATRIX mtxRotate = XMMatrixRotationRollPitchYaw(XMConvertToRadians(fPitch), XMConvertToRadians(fYaw), XMConvertToRadians(fRoll));
	m_xmf4x4World = Matrix4x4::Multiply(mtxRotate, m_xmf4x4World);
}

void CGameObject::Rotate(XMFLOAT3 *pxmf3Axis, float fAngle)
{
	XMMATRIX mtxRotate = XMMatrixRotationAxis(XMLoadFloat3(pxmf3Axis), XMConvertToRadians(fAngle));
	m_xmf4x4World = Matrix4x4::Multiply(mtxRotate, m_xmf4x4World);
}

void CGameObject::Rotate(XMFLOAT4 *pxmf4Quaternion)
{
	XMMATRIX mtxRotate = XMMatrixRotationQuaternion(XMLoadFloat4(pxmf4Quaternion));
	m_xmf4x4World = Matrix4x4::Multiply(mtxRotate, m_xmf4x4World);
}

void CGameObject::SetShader(CShader* pShader)
{
	if (m_pShader) m_pShader->Release();
	m_pShader = pShader;

	if (m_pShader) m_pShader->AddRef();
}

void CGameObject::ApplyAnimation()
{
	if (m_pfbxScene && m_pAnimationController) // Scene & Animation
	{
		FbxTime currentTime = m_pAnimationController->GetCurrentTime();

		AnimateFbxNodeHierarchy(m_pfbxScene->GetRootNode(), currentTime);
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
CBlueObject::CBlueObject(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList,
	ID3D12RootSignature* pd3dGraphicsRootSignature, FbxManager* pfbxSdkManager, CTexture* pTextureManager, FbxScene *pfbxScene)
	: CGameObject(pTextureManager, pd3dDevice), m_pObjTexture(NULL)
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

		m_pfbxScene = ::LoadFbxSceneFromFile(pd3dDevice, pd3dCommandList, pfbxSdkManager, "Model/BluePlayer.fbx");

		std::vector<ID3D12Resource*> textures = m_pTextureManager->ExtractTexturesWithCustom(m_pfbxScene->GetRootNode(), "Model/Character/Textures/", pd3dCommandList);

		if (!textures.empty()) {
			m_pTexture = textures[0]; // 첫 번째 텍스처를 사용
			//debugLog << "First texture loaded for Player: " << m_pTexture << std::endl;
		}
		else {
			// std::cerr << "No textures loaded for Player." << std::endl;
		}
		::CreateMeshFromFbxNodeHierarchy(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature, m_pfbxScene->GetRootNode());
	}
	SetTexture(m_pTexture, 0);

	m_pAnimationController = new CAnimationController(m_pfbxScene);

	if (m_pAnimationController) {
		m_pAnimationController->LoadAnimations(pfbxSdkManager, ObjectAnimations);
	}
}

CBlueObject::~CBlueObject()
{
}