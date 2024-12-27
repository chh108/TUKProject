// CTexture.h
#pragma once

// 20241204 Texture 헤더 파일 수정
// 20241214 Unorderd_map 사용을 통한 중복 방지

#include "stdafx.h"
#include <codecvt>
#include <unordered_map>

// 20241213 Texture 클래스 수정

class CTexture {
public:
    CTexture(ID3D12Device* device, ID3D12CommandQueue* commandQueue, ID3D12DescriptorHeap* descriptorHeap);
    ~CTexture();

    ID3D12Resource* LoadTexture(const std::string& path, ID3D12GraphicsCommandList* pd3dCommandList);
    std::vector<ID3D12Resource*>  ExtractTexturesWithCustom(FbxNode* pNode, const std::string& path, ID3D12GraphicsCommandList* pd3dCommandList);
    std::string ConvertExtensionToLowerCase(const std::string & fileName);

    void SetRootParameterIndex(int index, UINT rootParameterIndex);
    UINT GetRootParameterIndex(int index) const;

    D3D12_GPU_DESCRIPTOR_HANDLE GetGpuDescriptorHandle(int index) const;

private:
    ID3D12Device* m_pd3dDevice;
    ID3D12CommandQueue* m_pd3dCommandQueue;
    ID3D12DescriptorHeap* m_pd3dDescriptorHeap;

    UINT m_heapIndex = 0;
    std::unordered_map<std::string, ID3D12Resource*> m_textureMap;
    std::unordered_map<std::string, D3D12_GPU_DESCRIPTOR_HANDLE> m_textureHandles;
    std::vector<UINT> m_rootParameterIndices;
};