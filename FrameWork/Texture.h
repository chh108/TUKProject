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

    ID3D12Resource* LoadTexture(const std::string& path);

    void ExtractTexturesFromNode(FbxNode* node);

private:
    ID3D12Device* m_pd3dDevice;
    ID3D12CommandQueue* m_pd3dCommandQueue;
    ID3D12DescriptorHeap* m_pd3dDescriptorHeap;

    UINT m_heapIndex = 0;
    std::unordered_map<std::string, ID3D12Resource*> m_textureMap;
};