#pragma once

#include"pch.h"
#include<vector>
#include<algorithm>
#include"MortonKey\mortonkeys.h"
#include<ctime>

using namespace std;

namespace _Lights
{
#define MAX_NORMALISED_COORD 2097151 //21 bits.
#define NUM_LIGHTS 16
#define RADIUS (0.25)

	vector<UINT64> zCodes;

	struct Light
	{
		XMFLOAT3 pos;
		XMFLOAT3 color;
		float r;
		float padding;
	};

	struct AABBNode {
		XMFLOAT3 center;
		XMFLOAT3 radius;
		UINT32 index;
		UINT32 active; 
		
		void addZCode(const float sub[3], const float div[3])
		{
			UINT32 intPos[3];
			float floatPos[3] = { center.x, center.y, center.z };
			for (int i = 0; i < 3; i++) {
				float norm = floatPos[i] * div[i] - sub[i];
				intPos[i] = norm * MAX_NORMALISED_COORD;
			}
			zCodes[index] = MortonKey::encodeMortonKey(intPos[0], intPos[1], intPos[2]);
		}

		bool operator<(const AABBNode &a) const
		{
			return zCodes[index] < zCodes[a.index];
		}
	};

	struct ConstantBuffer
	{
		UINT32 val[8];
	};

	vector<Light> lights;

	int treeDepth;
	int treeSize;
	vector<AABBNode> shaderLights;

	void init(const float *min, const float *max) {
		MortonKey::morton<256, 0>::add_values(MortonKey::mortonkeyX);
		MortonKey::morton<256, 1>::add_values(MortonKey::mortonkeyY);
		MortonKey::morton<256, 2>::add_values(MortonKey::mortonkeyZ);
		
		lights.resize(NUM_LIGHTS * NUM_LIGHTS * NUM_LIGHTS);
		for (int i = 0; i < NUM_LIGHTS; i++) {
			int id = i*NUM_LIGHTS*NUM_LIGHTS;
			for (int j = 0; j < NUM_LIGHTS; j++) {
				int jd = j*NUM_LIGHTS;
				for (int k = 0; k < NUM_LIGHTS; k++) {
					float pos[3] = { (float)i / (NUM_LIGHTS - 1), (float)j / (NUM_LIGHTS - 1), (float)k / (NUM_LIGHTS - 1) };
					for (int d = 0; d < 3; d++) {
						pos[d] = pos[d] * (max[d] - min[d]) + min[d];
					}
					lights[id + jd + k].pos.x = pos[0];
					lights[id + jd + k].pos.y = pos[1];
					lights[id + jd + k].pos.z = pos[2];
				}
			}
		}

		srand(static_cast <unsigned> (time(0)));

		for (Light &light : lights) {
			light.r = RADIUS;
			light.color.x = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
			light.color.y = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
			light.color.z = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
			XMVECTOR vec = XMLoadFloat3(&light.color);
			vec = XMVector3Normalize(vec);
			XMStoreFloat3(&light.color, vec);
		}
		
		// Make the tree node buffer the correct size. i.e. Big enough so that we can have a complete tree with at least N leaves.
		treeDepth = 1;
		int mul = 1;
		while (mul <= lights.size()) {
			treeDepth++;
			mul *= 32;
		}
		treeSize = ((1 << 5 * treeDepth) - 1) / 31;
	}

	void lightsPerFrame(XMMATRIX &viewMatrix)
	{
		zCodes.resize(lights.size());
		shaderLights.resize(lights.size());

		for (int i = 0; i < lights.size(); i++) {
			XMVECTOR vec = XMLoadFloat3(&lights[i].pos);
			vec = XMVector3Transform(vec, viewMatrix);
			XMStoreFloat3(&shaderLights[i].center, vec);
			float r = lights[i].r;
			XMFLOAT3 rad = { r, r, r };
			shaderLights[i].radius = rad;
			shaderLights[i].index = i;
			shaderLights[i].active = (UINT32)true;
		}
		
		float max[3] = { -D3D11_FLOAT32_MAX, -D3D11_FLOAT32_MAX, -D3D11_FLOAT32_MAX };
		float min[3] = { D3D11_FLOAT32_MAX, D3D11_FLOAT32_MAX, D3D11_FLOAT32_MAX };
		for (AABBNode &light : shaderLights) {
			float floatPos[3] = { light.center.x, light.center.y, light.center.z };
			for (int i = 0; i < 3; i++) {
				max[i] = max(floatPos[i], max[i]);
				min[i] = min(floatPos[i], min[i]);
			}
		}
		float sub[3], div[3];
		for (int i = 0; i < 3; i++){
			div[i] = 1.0f / (max[i] - min[i]);
			sub[i] = min[i] * div[i];
		}
		for (AABBNode &light : shaderLights)
			light.addZCode(sub, div);

		sort(shaderLights.begin(), shaderLights.end());
	}
}