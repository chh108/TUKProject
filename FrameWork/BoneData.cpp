#include "stdafx.h"
#include "BoneData.h"
#include "DebugLog.h"
#include "FbxSceneContext.h"

CBoneData::CBoneData(ID3D12Device* device, ID3D12DescriptorHeap* heap)
	: m_pd3dDevice(device), m_pd3dCbvSrvDescriptorHeap(heap) {
}

CBoneData::~CBoneData() {
	if (m_pd3dBoneBuffer) m_pd3dBoneBuffer->Release();
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
		BoneInfo boneInfo;
		boneInfo.pBoneNode = pfbxNode;
		boneInfo.boneOffsetMatrix = pfbxNode->EvaluateGlobalTransform();
		boneInfo.parentIndex = parentIndex;


		int boneIndex = static_cast<int>(m_BoneInfos.size());
		m_BoneInfos.push_back(boneInfo);
		m_BoneNameToIndex[pfbxNode->GetName()] = boneIndex;

		debugLog << "[Bone Loaded] " << pfbxNode->GetName() << " | Index: " << boneIndex
			<< " | Parent Index: " << parentIndex << std::endl;

		// 자식 노드 탐색
		for (int i = 0; i < pfbxNode->GetChildCount(); i++) {
			LoadBones(pfbxNode->GetChild(i), boneIndex);
		}
	}
	else {
		for (int i = 0; i < pfbxNode->GetChildCount(); i++) {
			LoadBones(pfbxNode->GetChild(i), parentIndex);
		}
	}

}

// 본 버퍼 생성 (통합)
void CBoneData::CreateBoneBuffer(D3D12_CPU_DESCRIPTOR_HANDLE d3dBoneCpuHandle) {
	UINT64 bufferSize = (sizeof(XMFLOAT4X4) * m_BoneInfos.size() + 255) & ~255;

	D3D12_HEAP_PROPERTIES heapProps = {};
	heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

	D3D12_RESOURCE_DESC bufferDesc = {};
	bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufferDesc.Width = bufferSize;
	bufferDesc.Height = 1;
	bufferDesc.DepthOrArraySize = 1;
	bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	m_pd3dDevice->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&bufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_pd3dBoneBuffer)
	);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.FirstElement = 0;
	srvDesc.Buffer.NumElements = static_cast<UINT>(m_BoneInfos.size());
	srvDesc.Buffer.StructureByteStride = sizeof(XMFLOAT4X4);
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	m_pd3dDevice->CreateShaderResourceView(m_pd3dBoneBuffer, &srvDesc, d3dBoneCpuHandle);

	debugLog << "[CreateBoneBuffer] Unified Bone Buffer Created." << std::endl;
}

// 본 트랜스폼 업데이트 및 GPU 업로드
void CBoneData::UpdateAndUploadBoneTransforms(FbxTime& fbxCurrentTime, D3D12_CPU_DESCRIPTOR_HANDLE d3dBoneCpuHandle) {
	for (size_t i = 0; i < m_BoneInfos.size(); ++i) {
		FbxAMatrix globalTransform = m_BoneInfos[i].pBoneNode->EvaluateGlobalTransform(fbxCurrentTime);
		if (m_BoneInfos[i].parentIndex != -1) {
			globalTransform = m_BoneInfos[m_BoneInfos[i].parentIndex].finalTransform * globalTransform;
		}
		m_BoneInfos[i].finalTransform = globalTransform * m_BoneInfos[i].boneOffsetMatrix;
		m_FinalBoneTransforms[i] = FbxMatrixToXmFloat4x4Matrix(&m_BoneInfos[i].finalTransform);
	}

	void* mappedData = nullptr;
	D3D12_RANGE readRange = { 0, 0 };

	HRESULT hr = m_pd3dBoneBuffer->Map(0, &readRange, &mappedData);
	if (SUCCEEDED(hr) && mappedData) {
		memcpy(mappedData, m_FinalBoneTransforms.data(), sizeof(XMFLOAT4X4) * m_FinalBoneTransforms.size());
		m_pd3dBoneBuffer->Unmap(0, nullptr);
	}
	else {
		debugLog << "[Error] Failed to upload bone transforms!" << std::endl;
	}
}

// SRV 바인딩
void CBoneData::BindBoneSRV(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE d3dBoneGpuHandle) {
	commandList->SetGraphicsRootDescriptorTable(rootParameterIndex, d3dBoneGpuHandle);
	debugLog << "[BindBoneSRV] Bone SRV Bound at Root Index: " << rootParameterIndex << std::endl;
}