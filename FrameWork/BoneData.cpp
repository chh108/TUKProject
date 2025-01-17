// BoneData.cpp

#include "stdafx.h"
#include "BoneData.h"
#include "DebugLog.h"
#include "FbxSceneContext.h"

CBoneData::CBoneData(ID3D12Device* pd3dDevice, ID3D12DescriptorHeap* pd3dDescriptorHeap)
	: m_pd3dDevice(pd3dDevice), m_pd3dCbvSrvDescriptorHeap(pd3dDescriptorHeap)
{
	m_descriptorIncrementSize = m_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

CBoneData::~CBoneData() {
	if (m_pd3dBoneOffsetBuffer) m_pd3dBoneOffsetBuffer->Release();
	if (m_pd3dBoneTransformBuffer) m_pd3dBoneTransformBuffer->Release();
}

// 본 관련 함수 추가
void CBoneData::LoadVertexBoneData(FbxMesh* pMesh)
{
	int numVertices = pMesh->GetControlPointsCount();
	std::vector<VertexBoneData> vertexBoneData(numVertices);

	for (int i = 0; i < pMesh->GetDeformerCount(FbxDeformer::eSkin); i++) {
		FbxSkin* pSkin = static_cast<FbxSkin*>(pMesh->GetDeformer(i, FbxDeformer::eSkin));

		for (int j = 0; j < pSkin->GetClusterCount(); j++) {
			FbxCluster* pCluster = pSkin->GetCluster(j);
			std::string boneName = pCluster->GetLink()->GetName();
			int boneIndex = m_BoneNameToIndex[boneName];

			int* pnIndices = pCluster->GetControlPointIndices();
			double* pdWeights = pCluster->GetControlPointWeights();
			int nIndexCount = pCluster->GetControlPointIndicesCount();

			for (int k = 0; k < nIndexCount; k++) {
				int nVertexIndex = pnIndices[k];
				float weight = static_cast<float>(pdWeights[k]);

				for (int count = 0; count < 4; count++) // Max Vertex INFLUENCIES 4
				{
					if (vertexBoneData[nVertexIndex].BoneWeights[count] == 0.0f) {
						vertexBoneData[nVertexIndex].BoneIndices[count] = boneIndex;
						vertexBoneData[nVertexIndex].BoneWeights[count] = weight;

						// Vertex Bone Debug
						debugLog << "[Vertex Bone Assignment] Vertex: " << nVertexIndex
							<< " | Bone Index: " << boneIndex
							<< " | Weight: " << weight << std::endl;
						break;
					}
				}
			}
		}
	}

	// 가중치 정규화
	for (auto& data : vertexBoneData) {
		float fTotalWeight = 0.0f;
		for (int i = 0; i < 4; ++i) {
			fTotalWeight += data.BoneWeights[i];
		}
		if (fTotalWeight > 0.0f) {
			for (int i = 0; i < 4; ++i) {
				data.BoneWeights[i] /= fTotalWeight;
			}
		}
	}

	m_VertexBoneData = vertexBoneData;
}

void CBoneData::LoadBones(FbxNode* pfbxNode, int& parentIndex)
{
	if (!pfbxNode) return;

	// 본 노드인지 확인
	FbxNodeAttribute* pAttr = pfbxNode->GetNodeAttribute();
	if (pAttr && pAttr->GetAttributeType() == FbxNodeAttribute::eSkeleton) {
		std::string boneName = pfbxNode->GetName();

		// 중복 본 로딩 방지
		if (m_BoneNameToIndex.find(boneName) != m_BoneNameToIndex.end()) {
			debugLog << "[Warning] Duplicate Bone: " << boneName << " - Skipping..." << std::endl;
			return;
		}

		BoneInfo boneInfo;
		boneInfo.pBoneNode = pfbxNode;

		// 본 오프셋 행렬 정확하게 계산 (BindPose 기준)
		FbxAMatrix bindPoseMatrix;
		bool hasBindPose = false;

		for (int i = 0; i < pfbxNode->GetScene()->GetPoseCount(); i++) {
			FbxPose* pose = pfbxNode->GetScene()->GetPose(i);
			if (pose->IsBindPose()) {
				for (int j = 0; j < pose->GetCount(); j++) {
					if (pose->GetNode(j) == pfbxNode) {
						FbxMatrix matrix = pose->GetMatrix(j);
						for (int row = 0; row < 4; ++row) {
							for (int col = 0; col < 4; ++col) {
								bindPoseMatrix[row][col] = matrix.Get(row, col);
							}
						}
						hasBindPose = true;
						break;
					}
				}
			}
		}

		if (hasBindPose) {
			boneInfo.boneOffsetMatrix = bindPoseMatrix;
		}
		else {
			boneInfo.boneOffsetMatrix = pfbxNode->EvaluateGlobalTransform();  // Fallback
			debugLog << "[Warning] No BindPose Found for Bone: " << boneName << ". Using Global Transform." << std::endl;
		}

		boneInfo.parentIndex = parentIndex;

		int boneIndex = static_cast<int>(m_BoneInfos.size());
		m_BoneInfos.push_back(boneInfo);
		m_BoneNameToIndex[boneName] = boneIndex;

		debugLog << "[Bone Loaded] " << boneName << " | Index: " << boneIndex
			<< " | Parent Index: " << parentIndex << std::endl;

		// 자식 노드 탐색
		for (int i = 0; i < pfbxNode->GetChildCount(); i++) {
			LoadBones(pfbxNode->GetChild(i), boneIndex);
		}
	}
	else {
		// 본이 아니더라도 자식 노드 탐색
		for (int i = 0; i < pfbxNode->GetChildCount(); i++) {
			LoadBones(pfbxNode->GetChild(i), parentIndex);
		}
	}
}

// 본 버퍼 생성 (통합)
void CBoneData::CreateBoneBuffers(D3D12_CPU_DESCRIPTOR_HANDLE cbvHandle, D3D12_CPU_DESCRIPTOR_HANDLE srvHandle) {
	UINT64 bufferSize = sizeof(XMFLOAT4X4) * m_BoneInfos.size();

	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC bufferDesc = {};
	bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufferDesc.Width = bufferSize;
	bufferDesc.Height = 1;
	bufferDesc.DepthOrArraySize = 1;
	bufferDesc.MipLevels = 1;
	bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	// 1. 본 오프셋 버퍼 (CBV)
	m_pd3dDevice->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&bufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_pd3dBoneOffsetBuffer)
	);

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = m_pd3dBoneOffsetBuffer->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = (UINT)((bufferSize + 255) & ~255);

	m_pd3dDevice->CreateConstantBufferView(&cbvDesc, cbvHandle);
	m_BoneOffsetCBVHandle = cbvHandle;

	// 2. 본 트랜스폼 버퍼 (SRV)
	m_pd3dDevice->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&bufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_pd3dBoneTransformBuffer)
	);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = static_cast<UINT>(m_BoneInfos.size());
	srvDesc.Buffer.StructureByteStride = sizeof(XMFLOAT4X4);
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	m_pd3dDevice->CreateShaderResourceView(m_pd3dBoneTransformBuffer, &srvDesc, srvHandle);
	m_BoneTransformSRVHandle = srvHandle;

	debugLog << "[CreateBoneBuffers] CBV and SRV buffers created." << std::endl;
}

// 본 트랜스폼 업데이트 및 GPU 업로드
void CBoneData::UpdateAndUploadBoneTransforms(FbxTime& fbxCurrentTime) {
	for (size_t i = 0; i < m_BoneInfos.size(); ++i) {
		FbxAMatrix globalTransform = m_BoneInfos[i].pBoneNode->EvaluateGlobalTransform(fbxCurrentTime);
		if (m_BoneInfos[i].parentIndex != -1) {
			globalTransform = m_BoneInfos[m_BoneInfos[i].parentIndex].finalTransform * globalTransform;
		}
		m_BoneInfos[i].finalTransform = globalTransform * m_BoneInfos[i].boneOffsetMatrix;
		m_FinalBoneTransforms[i] = FbxMatrixToXmFloat4x4Matrix(&m_BoneInfos[i].finalTransform);
	}

	// GPU에 트랜스폼 데이터 업로드 (SRV)
	void* mappedData = nullptr;
	D3D12_RANGE readRange = { 0, 0 };

	HRESULT hr = m_pd3dBoneTransformBuffer->Map(0, &readRange, &mappedData);
	if (SUCCEEDED(hr) && mappedData) {
		memcpy(mappedData, m_FinalBoneTransforms.data(), sizeof(XMFLOAT4X4) * m_FinalBoneTransforms.size());
		m_pd3dBoneTransformBuffer->Unmap(0, nullptr);
		debugLog << "[UpdateAndUploadBoneTransforms] : Success!!" << std::endl;
	}
	else {
		debugLog << "[Error] Failed to upload bone transforms!" << std::endl;
	}
}

// SRV 바인딩
void CBoneData::BindBoneBuffers(ID3D12GraphicsCommandList* commandList) {
	// CBV (Bone Offset Buffer) → Root Parameter Index 6
	commandList->SetGraphicsRootDescriptorTable(6, m_pd3dCbvSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

	// SRV (Bone Transform Buffer) → Root Parameter Index 7
	commandList->SetGraphicsRootDescriptorTable(7, m_pd3dCbvSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

	debugLog << "[BindBoneBuffers] CBV (t3) and SRV (t4) bound to the pipeline." << std::endl;
}