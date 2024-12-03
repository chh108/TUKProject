// ���ϸ�: Texture.h
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

    // �ؽ�ó �ε� �Լ�
    ID3D12Resource* LoadTexture(const std::string& path);

    // FBX ��忡�� �ؽ�ó ����
    std::vector<TextureInfo> ExtractTexturesFromNode(FbxNode* node);

    // ��ũ���� �� �ε��� ����
    void SetHeapIndex(UINT heapIndex);
    UINT GetHeapIndex() const;
};