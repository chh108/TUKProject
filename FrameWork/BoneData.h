#pragma once

#include <vector>
#include <unordered_map>
#include <string>
#include <fbxsdk.h>
#include "stdafx.h"
// 본 정보 구조체
struct BoneInfo {
    FbxNode* pBoneNode = nullptr;
    FbxAMatrix boneOffsetMatrix;
    FbxAMatrix finalTransform;
    int parentIndex = -1;
};

// 버텍스 본 데이터 구조체
struct VertexBoneData {
    UINT BoneIndices[4] = { 0, 0, 0, 0 };
    float BoneWeights[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
};

// 본 데이터 클래스
class CBoneData {
public:
    CBoneData(ID3D12Device* device, ID3D12DescriptorHeap* heap);
    ~CBoneData();

    // Upload Bone Data from FBX File and Transform Set.
   void LoadBones(FbxNode* pfbxNode, int& parentIndex);
   void LoadVertexBoneData(FbxMesh* pMesh);
   void UpdateBoneTransforms(FbxTime& fbxCurrentTime);
   void UploadBoneTransformsToGPU(ID3D12GraphicsCommandList* commandList);
   void BindBoneSRV(ID3D12GraphicsCommandList* commandList, UINT rootParameterIndex);

   const std::vector<XMFLOAT4X4>& GetFinalBoneTransforms();

private:
    ID3D12Device* m_pd3dDevice = NULL;
    ID3D12DescriptorHeap* m_pd3dCbvSrvDescriptorHeap = NULL;

    ID3D12Resource* m_pd3dBoneBuffer = NULL;
    D3D12_CPU_DESCRIPTOR_HANDLE m_BoneSrvHandle = {};

    std::unordered_map<std::string, int> m_BoneNameToIndex;
    std::vector<BoneInfo> m_BoneInfos;
    std::vector<VertexBoneData> m_VertexBoneData;
    std::vector<XMFLOAT4X4> m_FinalBoneTransforms;

    UINT m_BoneHeapIndex = 0;
    UINT m_descriptorIncrementSize = 0;
};
