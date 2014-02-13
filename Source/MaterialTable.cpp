// Copyright (c) 2013-2014, Stoyan Nikolov
// All rights reserved.
// This sample is governed by a permissive BSD-style license. See LICENSE.
// Note: The Voxels Library itself has a separate license. Check out it's
// website for more information

#include "stdafx.h"

#include "MaterialTable.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

bool MaterialTable::Load(const std::string& filename)
{
	using boost::property_tree::ptree;
    ptree jsonTree;

	try {
		read_json(filename, jsonTree);
	} catch(std::exception& ex) {
		SLLOG(Sev_Error, Fac_Rendering, "Error reading materials file: ", ex.what());
	}

	std::unordered_map<std::string, TextureProperties> propertiesSet;
	try {
		auto properties = jsonTree.get_child("properties");
		
		std::for_each(properties.begin(), properties.end(), [&](ptree::value_type& property) {
			auto texObj = property.second.begin();
			auto texName = texObj->first.data();
			TextureProperties props;
			props.UVScale = texObj->second.get_child("UVScale").get_value<float>();
			propertiesSet.insert(std::make_pair(texName, props));
		});

	} catch(std::exception& ex) {
		SLLOG(Sev_Error, Fac_Rendering, "Error parsing texture properties: ", ex.what());
		return false;
	}

	TextureNames localTextures;
	TexturePropertiesVec localProperties;
	std::unordered_map<std::string, unsigned> texturesSet;
	auto addTexture = [&](const std::string& texture)->unsigned {
		auto it = texturesSet.find(texture);
		if(it != texturesSet.end())
			return it->second;

		localTextures.push_back(texture);
		auto id = localTextures.size()-1;
		texturesSet.insert(std::make_pair(texture, id));

		TextureProperties properties;
		auto props = propertiesSet.find(texture);
		if(props != propertiesSet.end()) {
			properties = props->second;
		}

		localProperties.push_back(properties);

		return id;
	};

	MaterialsVec localMeterials;
	try {
		auto materials = jsonTree.get_child("materials");
		const auto count = materials.count("");
		
		if(!count) {
			SLOG(Sev_Error, Fac_Rendering, "No materials specified in the table!");
			return false;
		}
				
		localMeterials.reserve(count);

		std::for_each(materials.begin(), materials.end(), [&](ptree::value_type& material) {
			Material output;
			auto tex0 = material.second.get_child("diffuse0").begin();
			auto tex1 = material.second.get_child("diffuse1").begin();
			for(auto i = 0; i < 3; ++i, ++tex0, ++tex1) {
				output.DiffuseIds0[i] = addTexture(tex0->second.data());
				output.DiffuseIds1[i] = addTexture(tex1->second.data());
			}
			localMeterials.emplace_back(output);
		});
	} catch(std::exception& ex) {
		SLLOG(Sev_Error, Fac_Rendering, "Error parsing materials: ", ex.what());
		return false;
	}
	
	std::swap(localMeterials, m_Materials);
	std::swap(localTextures, m_DiffuseTextures);
	std::swap(localProperties, m_TextureProperties);

	return true;
}

const MaterialTable::TextureNames& MaterialTable::GetDiffuseTextureList() const
{
	return m_DiffuseTextures;
}

const MaterialTable::TexturePropertiesVec& MaterialTable::GetDiffuseTextureProperties() const
{
	return m_TextureProperties;
}

MaterialTable::Material* MaterialTable::GetMaterial(unsigned char id) const
{
	if(m_Materials.size() <= id)
		return nullptr;

	return const_cast<Material*>(&(m_Materials[id]));
}

unsigned MaterialTable::GetMaterialsCount() const
{
	return m_Materials.size();
}
