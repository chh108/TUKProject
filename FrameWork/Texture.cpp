// CTexture.cpp

#include "stdafx.h"
#include "Texture.h"
#include <WICTextureLoader.h>
#include <ResourceUploadBatch.h>
#include <unordered_map>            // 20241214 Unorderd_map 사용을 통한 중복 방지
#include "DebugLog.h"

CTexture::CTexture()
    : m_pd3dDevice(NULL), m_pd3dCommandQueue(NULL), m_pd3dCbvSrvDescriptorHeap(NULL), m_descriptorIncrementSize(0), m_heapIndex(0) {
}

CTexture::~CTexture() {
     // Release All Textures
    for (auto& pair : m_textureMap) {
        if (pair.second) pair.second->Release();
    }
}

void CTexture::Initialize(ID3D12Device* pd3dDevice, ID3D12CommandQueue* pd3dCommandQueue, ID3D12DescriptorHeap* pd3dCbvSrvDescriptorHeap, UINT descriptorIncrementSize)
{
    m_pd3dDevice = pd3dDevice;
    m_pd3dCommandQueue = pd3dCommandQueue;
    m_pd3dCbvSrvDescriptorHeap = pd3dCbvSrvDescriptorHeap;
    m_descriptorIncrementSize = descriptorIncrementSize;

    debugLog << "CTexture::Initialize called - Device: " << m_pd3dDevice << std::endl;
}

ID3D12Resource* CTexture::LoadTexture(const std::string& path, ID3D12GraphicsCommandList* pd3dCommandList) {
    
    if (m_textureMap.empty())
    {
       debugLog << "m_TextureMap is empty." << std::endl;
    }
    // Check if texture is already loaded to avoid duplicates
    if (m_textureMap.find(path) != m_textureMap.end()) {
        // debugLog << "Texture Already Loaded: " << path << std::endl;
        return m_textureMap[path]; // Return the already loaded texture
    }

    // Convert string path to wide string
    std::wstring widePath = std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>>().from_bytes(path);
    ID3D12Resource* textureResource = NULL;

    // Validate the device
    if (!m_pd3dDevice) {
        debugLog << "Device is NULL!" << std::endl;
        return NULL;
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
    uploadBatch.End(m_pd3dCommandQueue).wait();


    if (SUCCEEDED(hr) && textureResource) {
        debugLog << "Texture resource created successfully: " << textureResource << std::endl;
    }
    else {
        debugLog << "Failed to create texture resource. HRESULT: " << hr << std::endl;
    }

    if (FAILED(hr)) {
        wprintf(L"CreateWICTextureFromFile failed with HRESULT: 0x%08X\n", hr);
        if (hr == E_NOINTERFACE) {
            debugLog << "E_NOINTERFACE: No such interface supported. Check WIC or DirectX environment." << std::endl;
        }
        return NULL;
    }

    // textureResource 상태 점검
    if (!textureResource) {
        debugLog << "Texture resource is NULL!" << std::endl;
        return NULL;
    }

    // 텍스처 맵으로 관리
    if (textureResource) {
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = textureResource->GetDesc().Format;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = textureResource->GetDesc().MipLevels;

        D3D12_CPU_DESCRIPTOR_HANDLE srvHandle(m_pd3dCbvSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
        debugLog << "SRV Handle Address (Before) : " << srvHandle.ptr << std::endl;

        srvHandle.ptr += m_heapIndex * m_descriptorIncrementSize;

        m_pd3dDevice->CreateShaderResourceView(textureResource, &srvDesc, srvHandle);

        debugLog << "Create SRV : " << srvHandle.ptr << "with " << textureResource << std::endl;

        m_textureMap[path] = textureResource;
        // 디스크립터 힙 인덱스 증가
        m_heapIndex++;
    }

    m_textureMap[path] = textureResource;
    m_heapIndex++;

    return textureResource;
}

std::vector<ID3D12Resource*> CTexture::ExtractTexturesWithCustom(FbxNode* pNode, const std::string& path, ID3D12GraphicsCommandList* pd3dCommandList) {
    std::vector<ID3D12Resource*> textureResources;

    if (!pNode) return textureResources;

    int materialCount = pNode->GetMaterialCount();
    for (int i = 0; i < materialCount; i++) {
        FbxSurfaceMaterial* material = pNode->GetMaterial(i);
        if (material) {
            FbxProperty prop = material->FindProperty(FbxSurfaceMaterial::sDiffuse);
            if (prop.IsValid()) {
                FbxFileTexture* fileTexture = prop.GetSrcObject<FbxFileTexture>();
                if (fileTexture) {
                    std::string originalPath = fileTexture->GetFileName();

                    // 파일 이름만 추출
                    std::string fileName = originalPath.substr(originalPath.find_last_of("/\\") + 1);
                    fileName = ConvertExtensionToLowerCase(fileName);

                    // 경로에 텍스처가 있다고 가정하고 새 경로 생성
                    std::string customPath = path + fileName;

                    // 변경된 경로로 텍스처 로드
                    ID3D12Resource* textureResource = LoadTexture(customPath, pd3dCommandList);
                    if (textureResource) {
                        textureResources.push_back(textureResource);
                        //debugLog << "Loaded Custom Texture: " << customPath << std::endl;
                        //debugLog << "Texture Nums : " << textureResources.size() << std::endl;
                    }
                    else {
                        debugLog << "Failed to load texture: " << customPath << std::endl;
                    }
                }
            }
        }
    }

    // 재귀적으로 자식 노드 탐색
    int childCount = pNode->GetChildCount();
    for (int i = 0; i < childCount; i++) {
        std::vector<ID3D12Resource*> childResources = ExtractTexturesWithCustom(pNode->GetChild(i), path, pd3dCommandList);
        textureResources.insert(textureResources.end(), childResources.begin(), childResources.end());
    }

    return textureResources;
}

std::string CTexture::ConvertExtensionToLowerCase(const std::string& fileName) {
    size_t dotPos = fileName.find_last_of('.');
    if (dotPos == std::string::npos) {
        // 확장자가 없으면 그대로 반환
        return fileName;
    }

    std::string namePart = fileName.substr(0, dotPos); // 파일 이름 부분
    std::string extPart = fileName.substr(dotPos);    // 확장자 부분

    // 확장자를 소문자로 변환
    std::transform(extPart.begin(), extPart.end(), extPart.begin(),
        [](unsigned char c) { return std::tolower(c); });

    return namePart + extPart;
}