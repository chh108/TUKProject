// 파일명: Texture.h
#pragma once

#include "stdafx.h"
#include <codecvt>

class Texture {
public:
    struct TextureInfo {
        std::string texturePath;
        ID3D12Resource* textureResource = nullptr;
    };

private:
    ID3D12Device* m_pd3dDevice = nullptr;
    ID3D12GraphicsCommandList* m_pd3dCommandList = nullptr;
    ID3D12DescriptorHeap* m_pd3dDescriptorHeap = nullptr;
    UINT m_heapIndex = 0;

public:
    Texture(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, ID3D12DescriptorHeap* descriptorHeap);
    ~Texture();

    // 텍스처 로드 함수
    ID3D12Resource* LoadTexture(const std::string& path);

    // FBX 노드에서 텍스처 추출
    std::vector<TextureInfo> ExtractTexturesFromNode(FbxNode* node);

    // 디스크립터 힙 인덱스 관리
    void SetHeapIndex(UINT heapIndex);
    UINT GetHeapIndex() const;
};