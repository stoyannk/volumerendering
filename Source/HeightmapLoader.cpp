// Copyright (c) 2013-2014, Stoyan Nikolov
// All rights reserved.
// This sample is governed by a permissive BSD-style license. See LICENSE.
// Note: The Voxels Library itself has a separate license. Check out it's
// website for more information

#include "stdafx.h"
#include "HeightMapLoader.h"

#include "lodepng.h"
#include <Utilities/MathInlines.h>

using namespace DirectX;

bool HeightMapLoader::Load(const std::string& filename, unsigned& w, std::shared_ptr<char>& heightmap, float heightScale)
{
	unsigned imageW = 0, imageH = 0;
	std::vector<unsigned char> image;
	auto ec = lodepng::decode(image, imageW, imageH, filename);
	if(ec != 0) {
		SLLOG(Sev_Error, Fac_Rendering, "Unable to decode ", filename, " error: ", lodepng_error_text(ec));
		return false;
	}

	if(imageW != imageH) {
		SLOG(Sev_Error, Fac_Rendering, "Only square heightmaps are supported. Failed generation on ", filename);
		return false;
	}

	heightmap = std::shared_ptr<char>(new char[imageW*imageH], std::default_delete<char[]>());
	w = imageW;

	const auto heightPtr = heightmap.get();
	const auto sz = image.size();
	for (auto pixel = 0u, hid = 0u; pixel < sz; pixel += 4, ++hid)
	{
		heightPtr[hid] = (char)StMath::clamp_value<int>(((int)image[pixel] - 127) * heightScale, -127, 127);
	}

	return true;
}