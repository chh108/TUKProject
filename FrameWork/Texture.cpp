// CTexture.cpp

#include "stdafx.h"
#include "Texture.h"
#include <WICTextureLoader.h>
#include <ResourceUploadBatch.h>
#include <unordered_map>            // 20241214 Unorderd_map 사용을 통한 중복 방지

CTexture::CTexture(ID3D12Device* device, ID3D12CommandQueue* commandQueue, ID3D12DescriptorHeap* descriptorHeap)
    : m_pd3dDevice(device), m_pd3dCommandQueue(commandQueue), m_pd3dDescriptorHeap(descriptorHeap), m_heapIndex(0) {
}

CTexture::~CTexture() {
    // Release All Textures
    for (auto& pair : m_textureMap) {
        if (pair.second) pair.second->Release();
    }
}

ID3D12Resource* CTexture::LoadTexture(const std::string& path) {
    
    // Check if texture is already loaded to avoid duplicates
    if (m_textureMap.find(path) != m_textureMap.end()) {
        return m_textureMap[path]; // Return the already loaded texture
    }

    // Convert string path to wide string
    std::wstring widePath = std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>>().from_bytes(path);
    ID3D12Resource* textureResource = nullptr;

    // Validate the device
    if (!m_pd3dDevice) {
        std::cerr << "Device is NULL!" << std::endl;
        return nullptr;
    }

    // Validate the descriptor heap
    if (!m_pd3dDescriptorHeap) {
        std::cerr << "Descriptor Heap is NULL!" << std::endl;
        return nullptr;
    }

    // Use ResourceUploadBatch to load texture
    DirectX::ResourceUploadBatch uploadBatch(m_pd3dDevice);
    uploadBatch.Begin();

    HRESULT hr = DirectX::CreateWICTextureFromFile(
        m_pd3dDevice,
        uploadBatch,
        widePath.c_str(),
        &textureResource,
        false // Do not generate mipmaps
    );

    if (FAILED(hr)) {
        wprintf(L"CreateWICTextureFromFile failed with HRESULT: 0x%08X\n", hr);
        if (hr == E_NOINTERFACE) {
            std::cerr << "E_NOINTERFACE: No such interface supported. Check WIC or DirectX environment." << std::endl;
        }
        return nullptr;
    }

    if (!m_pd3dDescriptorHeap) {
        std::cerr << "Descriptor Heap is NULL!" << std::endl;
        return nullptr;
    }

    // textureResource 상태 점검
    if (!textureResource) {
        std::cerr << "Texture resource is NULL!" << std::endl;
        return nullptr;
    }

    // 디스크립터 힙 정보 가져오기
    D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = m_pd3dDescriptorHeap->GetDesc();
    if (m_heapIndex >= descHeapDesc.NumDescriptors) {
        std::cerr << "Heap index exceeds descriptor heap size!" << std::endl;
        return nullptr;
    }

    // 디스크립터 핸들 설정
    D3D12_CPU_DESCRIPTOR_HANDLE srvHandle = m_pd3dDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
    srvHandle.ptr += m_heapIndex * m_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // Shader Resource View 생성
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = textureResource->GetDesc().Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = textureResource->GetDesc().MipLevels;

    m_pd3dDevice->CreateShaderResourceView(textureResource, &srvDesc, srvHandle);

    // 디스크립터 힙 인덱스 증가
    m_heapIndex++;

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