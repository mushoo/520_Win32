#pragma once

#include"pch.h"
#include<vector>

using namespace DirectX;

namespace _Lights
{
#define NUM_LIGHTS 32
#define RADIUS (0.14)

	struct Light
	{
		XMFLOAT3 pos;
		XMFLOAT3 color;
		float r;
		UINT32 active;
	};

	struct AABBNode {
		XMFLOAT3 center;
		XMFLOAT3 radius;
		UINT32 index;
		UINT32 active; 
	};

	struct ConstantBuffer
	{
		UINT32 val[8];
	};

	extern std::vector<Light> lights;

	extern int treeDepth;
	extern int treeSize;
	extern std::vector<AABBNode> shaderLights;

	void init(const float *min, const float *max);

	//void lightsPerFrame();
}