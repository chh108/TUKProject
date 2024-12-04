// Texture.h
#pragma once

// 20241204 Texture 헤더 파일 수정
#include "stdafx.h"
#include <codecvt>

class CTexture {
public:
    CTexture(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, ID3D12DescriptorHeap* descriptorHeap);
    ~CTexture();

    // 텍스처 로드 및 디스크립터 힙 등록
    ID3D12Resource* LoadTexture(const std::string& path);

    // FBX 노드에서 텍스처 추출 및 로드
    void ExtractTexturesFromNode(FbxNode* node);

private:
    ID3D12Device* m_pd3dDevice;
    ID3D12GraphicsCommandList* m_pd3dCommandList;
    ID3D12DescriptorHeap* m_pd3dDescriptorHeap;
    UINT m_heapIndex = 0;
};