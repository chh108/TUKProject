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

void CBoneData::UpdateBoneTransforms(FbxTime& fbxCurrentTime)
{
	for (size_t i = 0; i < m_BoneInfos.size(); ++i)
	{
		FbxAMatrix globalTransform = m_BoneInfos[i].pBoneNode->EvaluateGlobalTransform(fbxCurrentTime);

		// 부모-자식 관계를 고려한 트랜스폼
		if (m_BoneInfos[i].parentIndex != -1)
		{
			globalTransform = m_BoneInfos[m_BoneInfos[i].parentIndex].finalTransform * globalTransform;
		}

		m_BoneInfos[i].finalTransform = globalTransform * m_BoneInfos[i].boneOffsetMatrix;

		// 디버그 로그 추가
		//debugLog << "[Bone Updated] Index: " << i
		//	<< " | Transform: ["
		//	<< globalTransform.Get(0, 0) << ", " << globalTransform.Get(0, 1) << ", " << globalTransform.Get(0, 2) << "]"
		//	<< std::endl;
	}
}

const std::vector<XMFLOAT4X4>& CBoneData::GetFinalBoneTransforms() {
	if (m_FinalBoneTransforms.size() != m_BoneInfos.size()) {
		m_FinalBoneTransforms.resize(m_BoneInfos.size());
	}

	for (size_t i = 0; i < m_BoneInfos.size(); ++i) {
		m_FinalBoneTransforms[i] = FbxMatrixToXmFloat4x4Matrix(&m_BoneInfos[i].finalTransform);

		// 디버그 로그 추가 - 본의 최종 트랜스폼 확인
		debugLog << "[Bone Transform] Index: " << i << std::endl;
		debugLog << "Row 0: " << m_FinalBoneTransforms[i]._11 << ", " << m_FinalBoneTransforms[i]._12 << ", "
			<< m_FinalBoneTransforms[i]._13 << ", " << m_FinalBoneTransforms[i]._14 << std::endl;
		debugLog << "Row 1: " << m_FinalBoneTransforms[i]._21 << ", " << m_FinalBoneTransforms[i]._22 << ", "
			<< m_FinalBoneTransforms[i]._23 << ", " << m_FinalBoneTransforms[i]._24 << std::endl;
		debugLog << "Row 2: " << m_FinalBoneTransforms[i]._31 << ", " << m_FinalBoneTransforms[i]._32 << ", "
			<< m_FinalBoneTransforms[i]._33 << ", " << m_FinalBoneTransforms[i]._34 << std::endl;
		debugLog << "Row 3: " << m_FinalBoneTransforms[i]._41 << ", " << m_FinalBoneTransforms[i]._42 << ", "
			<< m_FinalBoneTransforms[i]._43 << ", " << m_FinalBoneTransforms[i]._44 << std::endl;
		debugLog << "--------------------------------------------------" << std::endl;
	}

	return m_FinalBoneTransforms;
}


void CBoneData::UploadBoneTransformsToGPU(ID3D12GraphicsCommandList* commandList)
{
	if (m_FinalBoneTransforms.size() != m_BoneInfos.size()) {
		m_FinalBoneTransforms.resize(m_BoneInfos.size());
	}

	for (size_t i = 0; i < m_BoneInfos.size(); ++i) {
		m_FinalBoneTransforms[i] = FbxMatrixToXmFloat4x4Matrix(&m_BoneInfos[i].finalTransform);
	}

	void* mappedData = nullptr;
	m_pd3dBoneBuffer->Map(0, nullptr, &mappedData);
	memcpy(mappedData, m_FinalBoneTransforms.data(), sizeof(XMFLOAT4X4) * m_FinalBoneTransforms.size());
	m_pd3dBoneBuffer->Unmap(0, nullptr);
}

void CBoneData::BindBoneSRV(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex)
{
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.NumElements = static_cast<UINT>(m_BoneInfos.size());
	srvDesc.Buffer.StructureByteStride = sizeof(XMFLOAT4X4);

	m_pd3dDevice->CreateShaderResourceView(m_pd3dBoneBuffer, &srvDesc, m_BoneSrvHandle);
}
