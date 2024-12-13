#include "stdafx.h"
#include "Texture.h"
#include <WICTextureLoader.h>
#include <ResourceUploadBatch.h>

CTexture::CTexture(ID3D12Device* device, ID3D12CommandQueue* commandQueue, ID3D12DescriptorHeap* descriptorHeap)
    : m_pd3dDevice(device), m_pd3dCommandQueue(commandQueue), m_pd3dDescriptorHeap(descriptorHeap) {
}

CTexture::~CTexture() {}

ID3D12Resource* CTexture::LoadTexture(const std::string& path) {
    std::wstring widePath = std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>>().from_bytes(path);
    ID3D12Resource* textureResource = nullptr;

    std::cout << "Loading Texture from: " << path << std::endl;

    // ResourceUploadBatch를 사용하여 텍스처 로드
    DirectX::ResourceUploadBatch uploadBatch(m_pd3dDevice);
    uploadBatch.Begin();

    HRESULT hr = DirectX::CreateWICTextureFromFile(
        m_pd3dDevice,
        uploadBatch,
        widePath.c_str(),
        &textureResource,
        false // Mipmap 생성 안 함
    );

    if (FAILED(hr)) {
        wprintf(L"Failed to load texture: %s. HRESULT: 0x%08X\n", widePath.c_str(), hr);
        return nullptr;
    }

    // 업로드 작업 완료
    auto finishResult = uploadBatch.End(m_pd3dCommandQueue);
    finishResult.wait();

    // 디스크립터 힙에 텍스처 등록
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = textureResource->GetDesc().Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = textureResource->GetDesc().MipLevels;

    D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = m_pd3dDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    srvHandle.ptr += m_heapIndex * m_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    m_pd3dDevice->CreateShaderResourceView(textureResource, &srvDesc, srvHandle);

    m_heapIndex++; // 디스크립터 힙 인덱스 증가

    return textureResource;
}

void CTexture::ExtractTexturesFromNode(FbxNode* node) {
    int materialCount = node->GetMaterialCount();
    for (int i = 0; i < materialCount; i++) {
        FbxSurfaceMaterial* material = node->GetMaterial(i);
        if (material) {
            FbxProperty prop = material->FindProperty(FbxSurfaceMaterial::sDiffuse);
            if (prop.IsValid()) {
                FbxFileTexture* fileTexture = prop.GetSrcObject<FbxFileTexture>();
                if (fileTexture) {
                    std::string texturePath = fileTexture->GetFileName();
                    LoadTexture(texturePath);
                    std::cout << "Loaded Texture: " << texturePath << std::endl;
                }
            }
        }
    }
}