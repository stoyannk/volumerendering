// Copyright (c) 2013-2014, Stoyan Nikolov
// All rights reserved.
// This sample is governed by a permissive BSD-style license. See LICENSE.
// Note: The Voxels Library itself has a separate license. Check out it's
// website for more information

cbuffer PerFrame : register(b0)
{
	matrix View;
	matrix Projection;
};

cbuffer PerBlockBuffer : register(b1)
{
	matrix World;
	matrix InvTranspWorld;
	vector Properties; // x - adj. flags, y - lod level(debug)
};

// Scales the UV coordinates of textures
cbuffer TextureProperties : register(b2)
{
	float4 UVScale[256 / 4];
};

struct VSInput
{
    float4 Pos : POSITION0;
	float4 SecondaryPos : POSITION1;
	float3 Normal : TEXCOORD0;
	uint2 TextureIndices : TEXCOORD1;
};

// Use this define to visualize the LOD levels in color
// NB: this define is incompatible with the normal rendering ones
//#define DRAW_LOD_LEVEL

// If you don't draw textures - the normals will be shown
#define DRAW_TEXTURES
#define DRAW_LIGHTING

struct VSOutput
{
    float4 Pos : SV_POSITION;
#ifdef DRAW_LOD_LEVEL
	float3 Color : TEXCOORD0;
#else
	float3 Normal : TEXCOORD0;
	float3 ObjectPosition : TEXCOORD1;
	uint2 TextureIndices : TEXCOORD2;
	float TexBlend : TEXCOORD3;

	float3 Tangent1 : TEXCOORD4;
	float3 Tangent2 : TEXCOORD5;
#endif
};

SamplerState texSampler : register(s0);
Texture2DArray diffuseTextures : register(t0);
Texture2DArray normalTextures : register(t1);

float4 SelectPositionCell(int vertexAdj, int blockAdj, float4 primary, float4 secondary) {
	if(vertexAdj && ((vertexAdj & blockAdj) == vertexAdj)) {
		return float4(secondary.xyz, 1);
	} else {
		return primary;		
	}
}

#ifdef DRAW_LOD_LEVEL
// Debug LOD level rendering
VSOutput DrawLODOutput(VSInput input, float4 inPos) {
	VSOutput output = (VSOutput)0;

	output.Pos = mul(inPos, World);
	output.Pos = mul(output.Pos, View);
	output.Pos = mul(output.Pos, Projection);

	float3 lodColors[] =
	{
		float3(0, 0, 0),
		float3(0.5f, 0.5f, 0.5f),
		float3(1.0f, 0.0f, 0.0f),
		float3(0.0f, 1.0f, 0.0f),
		float3(0.0f, 0.0f, 1.0f),
		float3(0.0f, 1.0f, 1.0f),
		float3(1.0f, 1.0f, 0.0f),
		float3(1.0f, 0.0f, 1.0f)
	};

	int lodlevel = asint(Properties.y);

	output.Color = lodColors[lodlevel];

	return output;
}

#else

#define TBN_EPSILON 0.03125f
// Normal rendering
VSOutput CalculateOutput(VSInput input, float4 inPos) {
	VSOutput output = (VSOutput)0;

	output.ObjectPosition = inPos;

	output.Pos = mul(inPos, World);
	output.Pos = mul(output.Pos, View);
	output.Pos = mul(output.Pos, Projection);

	output.Normal = mul(input.Normal, InvTranspWorld);
	
	const float t1denom = max(sqrt(output.Normal.x * output.Normal.x + output.Normal.y * output.Normal.y), TBN_EPSILON);
	output.Tangent1 = float3(output.Normal.y / t1denom, -output.Normal.x / t1denom, 0);

	const float t2denom = max(sqrt(output.Normal.x * output.Normal.x + output.Normal.z * output.Normal.z), TBN_EPSILON);
	output.Tangent2 = float3(-output.Normal.z / t2denom, 0, output.Normal.x / t2denom);

	output.TextureIndices = input.TextureIndices;
	output.TexBlend = ((input.TextureIndices[0] >> 8) & 0xFF) / 255.f;

	return output;
}
#endif // DRAW_LOD_LEVEL

VSOutput VS(VSInput input)
{
	int vertexAdj = asint(input.SecondaryPos.w);
	int blockAdj = asint(Properties.x);
	
	float4 inPos = SelectPositionCell(vertexAdj, blockAdj, input.Pos, input.SecondaryPos);
#ifdef DRAW_LOD_LEVEL // For debug puposes
	return DrawLODOutput(input, inPos);
#else
	return CalculateOutput(input, inPos);
#endif
}

VSOutput VSTransition(VSInput input)
{
	int vertexAdj = asint(input.SecondaryPos.w);
	int blockAdj = asint(Properties.x);
	
	float4 inPos = SelectPositionCell(vertexAdj, blockAdj, input.Pos, input.SecondaryPos);
#ifdef DRAW_LOD_LEVEL
	return DrawLODOutput(input, inPos);
#else
	return CalculateOutput(input, inPos);
#endif
}

float4 PS(VSOutput input) : SV_Target
{
#ifdef DRAW_LOD_LEVEL
	return float4(input.Color, 1.0f);
#else
	static const float3 SUN_DIRECTION = normalize(float3(.5, 1, 0.5));
	static const float AMBIENT = 0.15f;

	const float delta = 0.5f;
	const float m = 4.f;

#ifdef DRAW_TEXTURES
	const float3 normal = normalize(input.Normal);
	float3 blend = saturate(abs(normal) - delta);
	blend = pow(blend, m);
	blend /= dot(blend, float3(1.0f, 1.0f, 1.0f));
	
	// Txz, Uxz
	float2 material1 = float2(input.TextureIndices[0] >> 24, (input.TextureIndices[0] >> 16) & 0xFF);
	// Tpy, Tny, Upy, Uny
	float4 material2 = float4(input.TextureIndices[1] >> 24
							, (input.TextureIndices[1] >> 16) & 0xFF
							, (input.TextureIndices[1] >> 8) & 0xFF
							, (input.TextureIndices[1]) & 0xFF);

	float3 flip = float3(normal.x < 0.0, normal.y < 0.0, normal.z >= 0.0);

	float2 zindex = lerp(material2.xz, material2.yw, flip.y);
	
	float3 p = input.ObjectPosition;

	float3 s = lerp(p.zxx, -p.zxx, flip.xzy);
	float3 t = p.yyz;

	// Scale UV coordinates for artistic freedom
	float scalingFactor0xz = UVScale[floor(material1.x / 4)][int(material1.x) % 4];
	float scalingFactor0y  = UVScale[floor(zindex.x / 4)][int(zindex.x) % 4];
	float scalingFactor1xz = UVScale[floor(material1.y / 4)][int(material1.y) % 4];
	float scalingFactor1y  = UVScale[floor(zindex.y / 4)][int(zindex.y) % 4];

	// Sample the diffuse color
	const float3 cx0 = diffuseTextures.Sample(texSampler, float3(float2(s.x, t.x) / scalingFactor0xz, material1.x)).xyz;
	const float3 cz0 = diffuseTextures.Sample(texSampler, float3(float2(s.y, t.y) / scalingFactor0xz, material1.x)).xyz;
	const float3 cy0 = diffuseTextures.Sample(texSampler, float3(float2(s.z, t.z) / scalingFactor0y, zindex.x)).xyz;

	const float3 cx1 = diffuseTextures.Sample(texSampler, float3(float2(s.x, t.x) / scalingFactor1xz, material1.y)).xyz;
	const float3 cz1 = diffuseTextures.Sample(texSampler, float3(float2(s.y, t.y) / scalingFactor1xz, material1.y)).xyz;
	const float3 cy1 = diffuseTextures.Sample(texSampler, float3(float2(s.z, t.z) / scalingFactor1y, zindex.y)).xyz;

	float3 albedo = lerp((blend.x * cx0 + blend.y * cy0 + blend.z * cz0), (blend.x * cx1 + blend.y * cy1 + blend.z * cz1), input.TexBlend);

	// Calculate tangent & bitangent
	const float3 tangent1 = normalize(input.Tangent1);
	const float3 tangent2 = normalize(input.Tangent2);

	const float3 bitangent1 = normalize(cross(tangent1, normal));
	const float3 bitangent2 = normalize(cross(tangent2, normal));

	// Fetch and calculate normals
	const float3 nx0 = normalTextures.Sample(texSampler, float3(float2(s.x, t.x) / scalingFactor0xz, material1.x)).xyz;
	const float3 nz0 = normalTextures.Sample(texSampler, float3(float2(s.y, t.y) / scalingFactor0xz, material1.x)).xyz;
	const float3 ny0 = normalTextures.Sample(texSampler, float3(float2(s.z, t.z) / scalingFactor0y, zindex.x)).xyz;

	const float3 nx1 = normalTextures.Sample(texSampler, float3(float2(s.x, t.x) / scalingFactor1xz, material1.y)).xyz;
	const float3 nz1 = normalTextures.Sample(texSampler, float3(float2(s.y, t.y) / scalingFactor1xz, material1.y)).xyz;
	const float3 ny1 = normalTextures.Sample(texSampler, float3(float2(s.z, t.z) / scalingFactor1y, zindex.y)).xyz;

	const float3 nx = normalize(lerp(nx0, nx1, input.TexBlend) * 2.0f - 1.0f);
	const float3 ny = normalize(lerp(ny0, ny1, input.TexBlend) * 2.0f - 1.0f);
	const float3 nz = normalize(lerp(nz0, nz1, input.TexBlend) * 2.0f - 1.0f);

	const float3x3 tbn1 = transpose(float3x3(tangent1, bitangent1, normal));
	const float3x3 tbn2 = transpose(float3x3(tangent2, bitangent2, normal));

	const float3 sun_dir1 = mul(SUN_DIRECTION, tbn1);
	const float3 sun_dir2 = mul(SUN_DIRECTION, tbn2);

	float3 nl_vec = float3(saturate(dot(nx, sun_dir1)),
						saturate(dot(ny, sun_dir1)), 
						saturate(dot(nz, sun_dir2)));

	float nl = dot(nl_vec, blend);

	#ifdef DRAW_LIGHTING
	albedo = saturate(albedo * (nl + AMBIENT));
	#endif

	return float4(albedo, 1.0f);
#else
	return float4(input.Normal, 1.0f);
#endif // DRAW_TEXTURES

#endif // DRAW_LOD_LEVELS
}
