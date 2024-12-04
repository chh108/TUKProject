// Texture.h
#pragma once

// 20241204 Texture ��� ���� ����
#include "stdafx.h"
#include <codecvt>

class CTexture {
public:
    CTexture(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, ID3D12DescriptorHeap* descriptorHeap);
    ~CTexture();

    // �ؽ�ó �ε� �� ��ũ���� �� ���
    ID3D12Resource* LoadTexture(const std::string& path);

    // FBX ��忡�� �ؽ�ó ���� �� �ε�
    void ExtractTexturesFromNode(FbxNode* node);

private:
    ID3D12Device* m_pd3dDevice;
    ID3D12GraphicsCommandList* m_pd3dCommandList;
    ID3D12DescriptorHeap* m_pd3dDescriptorHeap;
    UINT m_heapIndex = 0;
};