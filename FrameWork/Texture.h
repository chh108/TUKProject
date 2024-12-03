#pragma once
// CTexture.h

#pragma once
#include "stdafx.h"

class CTexture
{
public:
	CTexture();
	~CTexture();

public:
	static std::string GetTexturePath(FbxSurfaceMaterial* material);
	static void ExtractTextures(FbxNode* node, std::vector<std::string>& texturePaths);
};