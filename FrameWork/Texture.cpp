#include "stdafx.h"
#include "Texture.h"

Texture::Texture(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, ID3D12DescriptorHeap* descriptorHeap)
    : m_pd3dDevice(device), m_pd3dCommandList(commandList), m_pd3dDescriptorHeap(descriptorHeap) {}

Texture::~Texture() {}

ID3D12Resource* Texture::LoadTexture(const std::string& path) {
    std::wstring widePath = std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>>().from_bytes(path);
    ID3D12Resource* textureResource = nullptr;
    ID3D12Resource* uploadBuffer = nullptr;

    // 텍스처 로드
    textureResource = ::CreateTextureResourceFromWICFile(
        m_pd3dDevice,
        m_pd3dCommandList,
        const_cast<wchar_t*>(widePath.c_str()),
        &uploadBuffer,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    );

    if (!textureResource) {
        wprintf(L"Failed to load texture: %s\n", widePath.c_str());
        return nullptr;
    }

    // 디스크립터 힙에 텍스처 등록
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = textureResource->GetDesc().Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = textureResource->GetDesc().MipLevels;

    // HEAP 계산
    D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = m_pd3dDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    srvHandle.ptr += m_heapIndex * m_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    m_pd3dDevice->CreateShaderResourceView(textureResource, &srvDesc, srvHandle);

    m_heapIndex++; // 디스크립터 힙 인덱스 증가

    if (uploadBuffer) uploadBuffer->Release(); // 업로드 버퍼 해제

    return textureResource;
}

std::vector<Texture::TextureInfo> Texture::ExtractTexturesFromNode(FbxNode* node) {
    std::vector<TextureInfo> textureInfos;

    int materialCount = node->GetMaterialCount();
    for (int i = 0; i < materialCount; i++) {
        FbxSurfaceMaterial* material = node->GetMaterial(i);
        if (material) {
            FbxProperty prop = material->FindProperty(FbxSurfaceMaterial::sDiffuse);
            if (prop.IsValid()) {
                FbxFileTexture* fileTexture = prop.GetSrcObject<FbxFileTexture>();
                if (fileTexture) {
                    std::string texturePath = fileTexture->GetFileName();
                    if (!texturePath.empty()) {
                        TextureInfo textureInfo;
                        textureInfo.texturePath = texturePath;
                        textureInfo.textureResource = LoadTexture(texturePath);
                        textureInfos.push_back(textureInfo);

                        std::cout << "Loaded Texture: " << texturePath << std::endl;
                    }
                }
            }
        }
    }
    return textureInfos;
}

void Texture::SetHeapIndex(UINT heapIndex) {
    m_heapIndex = heapIndex;
}

UINT Texture::GetHeapIndex() const {
    return m_heapIndex;
}
