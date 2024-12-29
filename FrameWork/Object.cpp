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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
CAnimationController::CAnimationController(FbxScene *pfbxScene)
{
    FbxArray<FbxString *> fbxAnimationStackNames;
	pfbxScene->FillAnimStackNameArray(fbxAnimationStackNames);

	m_nAnimationStacks = fbxAnimationStackNames.Size();

	m_ppfbxAnimationStacks = new FbxAnimStack*[m_nAnimationStacks];
	m_pfbxStartTimes = new FbxTime[m_nAnimationStacks];
	m_pfbxStopTimes = new FbxTime[m_nAnimationStacks];
	m_pfbxCurrentTimes = new FbxTime[m_nAnimationStacks];

	for (int i = 0; i < m_nAnimationStacks; i++)
	{
		FbxString *pfbxStackName = fbxAnimationStackNames[i];
		FbxAnimStack *pfbxAnimationStack = pfbxScene->FindMember<FbxAnimStack>(pfbxStackName->Buffer());
		m_ppfbxAnimationStacks[i] = pfbxAnimationStack;

		FbxTakeInfo *pfbxTakeInfo = pfbxScene->GetTakeInfo(*pfbxStackName);
		FbxTime fbxStartTime, fbxStopTime;
		if (pfbxTakeInfo)
		{
			fbxStartTime = pfbxTakeInfo->mLocalTimeSpan.GetStart();
			fbxStopTime = pfbxTakeInfo->mLocalTimeSpan.GetStop();
		}
		else
		{
			FbxTimeSpan fbxTimeLineTimeSpan;
			pfbxScene->GetGlobalSettings().GetTimelineDefaultTimeSpan(fbxTimeLineTimeSpan);
			fbxStartTime = fbxTimeLineTimeSpan.GetStart();
			fbxStopTime = fbxTimeLineTimeSpan.GetStop();
		}

		m_pfbxStartTimes[i] = fbxStartTime;
		m_pfbxStopTimes[i] = fbxStopTime;
		m_pfbxCurrentTimes[i] = FbxTime(0);
	}

    FbxArrayDelete(fbxAnimationStackNames);
}

CAnimationController::~CAnimationController()
{
	if (m_ppfbxAnimationStacks) delete[] m_ppfbxAnimationStacks;
	if (m_pfbxStartTimes) delete[] m_pfbxStartTimes;
	if (m_pfbxStopTimes) delete[] m_pfbxStopTimes;
	if (m_pfbxCurrentTimes) delete[] m_pfbxCurrentTimes;
}

void CAnimationController::SetAnimationStack(FbxScene *pfbxScene, int nAnimationStack)
{
	m_nAnimationStack = nAnimationStack;
	pfbxScene->SetCurrentAnimationStack(m_ppfbxAnimationStacks[nAnimationStack]);
}

void CAnimationController::SetPosition(int nAnimationStack, float fPosition)
{
	m_pfbxCurrentTimes[nAnimationStack].SetSecondDouble(fPosition);;
}

void CAnimationController::AdvanceTime(float fTimeElapsed) 
{
	m_fTime += fTimeElapsed; 

	FbxTime fbxElapsedTime;
	fbxElapsedTime.SetSecondDouble(fTimeElapsed);

	debugLog << "Animation Current Time : " << fbxElapsedTime.GetSecondDouble() << std::endl;

	m_pfbxCurrentTimes[m_nAnimationStack] += fbxElapsedTime;
	if (m_pfbxCurrentTimes[m_nAnimationStack] > m_pfbxStopTimes[m_nAnimationStack]) m_pfbxCurrentTimes[m_nAnimationStack] = m_pfbxStartTimes[m_nAnimationStack];
} 

void CAnimationController::CheckAnimationKeyframes(FbxScene* pFbxScene)
{
	int animStackCount = pFbxScene->GetSrcObjectCount<FbxAnimStack>();
	for (int i = 0; i < animStackCount; i++) {
		FbxAnimStack* animStack = pFbxScene->GetSrcObject<FbxAnimStack>(i);
		if (animStack) {
			debugLog << "Animation Stack [" << i << "]: " << animStack->GetName() << std::endl;

			FbxAnimLayer* animLayer = animStack->GetMember<FbxAnimLayer>();
			if (animLayer) {
				FbxAnimCurve* animCurve = pFbxScene->GetRootNode()->LclTranslation.GetCurve(animLayer, FBXSDK_CURVENODE_COMPONENT_X);
				if (animCurve) {
					int keyCount = animCurve->KeyGetCount();
					debugLog << "Total Keyframes: " << keyCount << std::endl;
					for (int k = 0; k < keyCount; k++) {
						FbxTime keyTime = animCurve->KeyGetTime(k);
						debugLog << "Keyframe[" << k << "] Time: " << keyTime.GetSecondDouble() << " seconds" << std::endl;
					}
				}
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
CGameObject::CGameObject() 
	: m_pd3dSrvDescriptorHeap(NULL), m_pd3dDevice(NULL), m_pTextureManager(NULL) {
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
		m_pd3dSrvDescriptorHeap = pTextureManager->GetDescriptorHeap(); // 힙 참조
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

	if (m_pd3dSrvDescriptorHeap) {
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = m_pd3dSrvDescriptorHeap->GetDesc();
		// debugLog << "SRV Descriptor Heap Size: " << heapDesc.NumDescriptors << std::endl;
	}
	else {
		// debugLog << "SRV Descriptor Heap is NULL." << std::endl;
	}
	// 20241216 텍스처 로딩
	if (m_pTexture) {
		D3D12_GPU_DESCRIPTOR_HANDLE srvHandle = m_pd3dSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
		// debugLog << "Texture Resource: " << m_pTexture << std::endl;

		srvHandle.ptr += m_TextureHeapIndex * m_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		UINT64 testNum = m_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		//debugLog << "SRVHandle: (With Texture) " << srvHandle.ptr << 
		//	" | index " << m_TextureHeapIndex <<
		//	" | Descriptor IncrementSize " << testNum << std::endl;

		ID3D12DescriptorHeap* ppHeaps[] = { m_pd3dSrvDescriptorHeap };
		pd3dCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);
		pd3dCommandList->SetGraphicsRootDescriptorTable(2, srvHandle); // Root ParameterIndex 2

		// debugLog << "SetGraphicsRootDescriptorTable: Handle Ptr = " << srvHandle.ptr << std::endl;
	}
	else
	{
		// debugLog << "Failed To Bind SRV.\n";
	}

	// 20241216 애니메이션 작업 시작
	FbxAMatrix fbxf4x4World = ::XmFloat4x4MatrixToFbxMatrix(m_xmf4x4World);
	if (m_pfbxScene) ::RenderFbxNodeHierarchy(pd3dCommandList, m_pfbxScene->GetRootNode(), m_pAnimationController->GetCurrentTime(), fbxf4x4World);
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

void CGameObject::PrintAnimationStackNames(FbxScene* pfbxScene)
{
	FbxArray<FbxString*> animationStackNames;
	pfbxScene->FillAnimStackNameArray(animationStackNames);

	for (int i = 0; i < animationStackNames.Size(); i++) {
		debugLog << "Animation Stack [" << i << "]: " << animationStackNames[i]->Buffer() << std::endl;
	}

	FbxArrayDelete(animationStackNames);
}

bool CGameObject::CreateAnimationStack(FbxScene* pScene, const std::string& animationFilePath)
{
	FbxManager* pFbxSdkManager = pScene->GetFbxManager();
	FbxImporter* pImporter = FbxImporter::Create(pFbxSdkManager, " ");

	if (!pImporter->Initialize(animationFilePath.c_str(), -1, pFbxSdkManager->GetIOSettings())) {
		std::cerr << "Failed to initialize importer for file: " << animationFilePath << std::endl;
		return false;
	}

	FbxScene* pAnimationScene = FbxScene::Create(pFbxSdkManager, "AnimationScene");
	if (!pImporter->Import(pAnimationScene)) {
		std::cerr << "Failed to import animation file: " << animationFilePath << std::endl;
		return false;
	}

	// 애니메이션 병합
	FbxAnimStack* pAnimStack = pAnimationScene->GetMember<FbxAnimStack>();
	if (pAnimStack) {
		pScene->AddMember(pAnimStack);
		std::cout << "Successfully added animation stack: " << pAnimStack->GetName() << std::endl;
	}
	else {
		std::cerr << "No animation stack found in file: " << animationFilePath << std::endl;
		return false;
	}

	pImporter->Destroy();
	return true;
}

void CGameObject::CheckAnimationStack(FbxScene* pfbxScene)
{
	FbxAnimStack* pAnimStack = pfbxScene->GetCurrentAnimationStack();
	if (pAnimStack)
	{
		debugLog << "Current Animation Stack: " << pAnimStack->GetName() << std::endl;

		FbxTime startTime, endTime;
		pAnimStack->GetLocalTimeSpan();
		debugLog << "Animation Time Range: Start = " << startTime.GetSecondDouble()
			<< ", End = " << endTime.GetSecondDouble() << std::endl;
	}
	else
	{
		debugLog << "No Animation Stack Found!" << std::endl;
	}
}

void CGameObject::CheckAllAnimationStacks(FbxScene* pfbxScene)
{
	int stackCount = pfbxScene->GetSrcObjectCount<FbxAnimStack>();
	debugLog << "Total Animation Stacks: " << stackCount << std::endl;

	for (int i = 0; i < stackCount; ++i)
	{
		FbxAnimStack* pAnimStack = pfbxScene->GetSrcObject<FbxAnimStack>(i);
		if (pAnimStack)
		{
			debugLog << "Animation Stack [" << i << "]: " << pAnimStack->GetName() << std::endl;

			FbxTime startTime, endTime;
			FbxTakeInfo* takeInfo = pfbxScene->GetTakeInfo(pAnimStack->GetName());
			if (takeInfo)
			{
				startTime = takeInfo->mLocalTimeSpan.GetStart();
				endTime = takeInfo->mLocalTimeSpan.GetStop();
				debugLog << "Time Range: Start = " << startTime.GetSecondDouble()
					<< ", End = " << endTime.GetSecondDouble() << std::endl;
			}
			else
			{
				debugLog << "No Time Range Available for Stack [" << i << "]" << std::endl;
			}
		}
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
		// m_pAnimationController->CheckAnimationKeyframes(m_pfbxScene);
		// CreateAnimationStack(m_pfbxScene, "Model/Character/Animations/IDLE.fbx");
		CheckAllAnimationStacks(m_pfbxScene);
		PrintAnimationStackNames(m_pfbxScene);
		m_pAnimationController->SetAnimationStack(m_pfbxScene, 0);
	}
}

CBlueObject::~CBlueObject()
{
}

