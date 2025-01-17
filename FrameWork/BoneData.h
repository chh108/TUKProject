// BoneData.h
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

class CBoneData {
public:
    CBoneData(ID3D12Device* pd3dDevice, ID3D12DescriptorHeap* pd3dDescriptorHeap);
    ~CBoneData();

    void LoadBones(FbxNode* pfbxNode, int& parentIndex);
    void LoadVertexBoneData(FbxMesh* pMesh);
    void UpdateAndUploadBoneTransforms(FbxTime& fbxCurrentTime);
    void BindBoneBuffers(ID3D12GraphicsCommandList* commandList);
    void CreateBoneBuffers(D3D12_CPU_DESCRIPTOR_HANDLE cbvHandle, D3D12_CPU_DESCRIPTOR_HANDLE srvHandle);

    ID3D12Resource* m_pd3dBoneOffsetBuffer = NULL;
    ID3D12Resource* m_pd3dBoneTransformBuffer = NULL;

private:
    ID3D12Device* m_pd3dDevice = NULL;
    ID3D12DescriptorHeap* m_pd3dCbvSrvDescriptorHeap = NULL;


    D3D12_CPU_DESCRIPTOR_HANDLE m_BoneOffsetCBVHandle = {};
    D3D12_CPU_DESCRIPTOR_HANDLE m_BoneTransformSRVHandle = {};

    std::unordered_map<std::string, int> m_BoneNameToIndex;
    std::vector<BoneInfo> m_BoneInfos;
    std::vector<VertexBoneData> m_VertexBoneData;
    std::vector<XMFLOAT4X4> m_FinalBoneTransforms;

    UINT m_descriptorIncrementSize = 0;
};
