#include "stdafx.h"
#include "Texture.h"
#include <iostream>

// Save Texture Path
struct TextureInfo {
	std::string texturePath;
};

// Model Data
struct ModelData {
	std::vector<TextureInfo> textures;
};

CTexture::CTexture()
{
}

CTexture::~CTexture()
{
}

std::string CTexture::GetTexturePath(FbxSurfaceMaterial* material) {
	if (material) {
		FbxProperty prop = material->FindProperty(FbxSurfaceMaterial::sDiffuse);
		if (prop.IsValid()) {
			FbxFileTexture* fileTexture = prop.GetSrcObject<FbxFileTexture>();
			if (fileTexture) {
				return fileTexture->GetFileName();
			}
		}
	}
	return "";
}

void CTexture::ExtractTextures(FbxNode* node, std::vector<std::string>& texturePaths) {
	int materialCount = node->GetMaterialCount();
	for (int i = 0; i < materialCount; i++) {
		FbxSurfaceMaterial* material = node->GetMaterial(i);
		std::string texturePath = GetTexturePath(material);
		if (!texturePath.empty()) {
			texturePaths.push_back(texturePath);
			std::cout << "Extracted Texture Path: " << texturePath << std::endl;
		}
	}
}

