// Texture.h
#pragma once

// 20241204 Texture 헤더 파일 수정
#include "stdafx.h"
#include <codecvt>

class CTexture {
public:
    CTexture(ID3D12Device* device, ID3D12CommandQueue* commandQueue, ID3D12DescriptorHeap* descriptorHeap);
    ~CTexture();

    ID3D12Resource* LoadTexture(const std::string& path);
    void ExtractTexturesFromNode(FbxNode* node);

private:
    ID3D12Device* m_pd3dDevice;
    ID3D12CommandQueue* m_pd3dCommandQueue;
    ID3D12DescriptorHeap* m_pd3dDescriptorHeap;
    UINT m_heapIndex = 0;
};