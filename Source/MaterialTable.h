// Copyright (c) 2013-2014, Stoyan Nikolov
// All rights reserved.
// This sample is governed by a permissive BSD-style license. See LICENSE.
// Note: The Voxels Library itself has a separate license. Check out it's
// website for more information
#pragma once

#include "../Voxels/include/MaterialMap.h"

// Implements a MaterialMap for the voxel grid.
// The textures are loaded from JSON descriptions
class MaterialTable : public Voxels::MaterialMap
{
public:
	typedef std::vector<std::string> TextureNames;

	struct TextureProperties
	{
		TextureProperties()
			: UVScale(1)
		{}
		float UVScale;
	};
	typedef std::vector<TextureProperties> TexturePropertiesVec;

	bool Load(const std::string& filename);

	const TextureNames& GetDiffuseTextureList() const;
	const TexturePropertiesVec& GetDiffuseTextureProperties() const;

	virtual Material* GetMaterial(unsigned char id) const override;
	unsigned GetMaterialsCount() const;

private:
	typedef std::vector<Material> MaterialsVec;

	TextureNames m_DiffuseTextures;
	TexturePropertiesVec m_TextureProperties;
	MaterialsVec m_Materials;
};
