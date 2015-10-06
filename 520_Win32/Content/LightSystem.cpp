#include"pch.h"
#include<vector>
#include<algorithm>
#include<ctime>
#include"LightSystem.h"

using namespace std;
using namespace DirectX;

namespace _Lights
{
	vector<Light> lights;

	int treeDepth;
	int treeSize;
	//vector<AABBNode> shaderLights;

	void init(const float *min, const float *max) {
		lights.resize(NUM_LIGHTS * NUM_LIGHTS * NUM_LIGHTS);
		//shaderLights.resize(lights.size());
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
			light.active = true;
			XMVECTOR vec = XMLoadFloat3(&light.color);
			vec = XMVector3Normalize(vec);
			XMStoreFloat3(&light.color, vec);
		}

		std::random_shuffle(lights.begin(), lights.end());

		// Make the tree node buffer the correct size. i.e. Big enough so that we can have a complete tree with at least N leaves.
		treeDepth = 1;
		int mul = 1;
		while (mul < lights.size()) {
			treeDepth++;
			mul *= 32;
		}
		treeSize = lights.size() + ((1 << 5 * (treeDepth - 1)) - 1) / 31;

		//lightsPerFrame();
	}
	/*
	void lightsPerFrame()
	{
		for (int i = 0; i < lights.size(); i++) {
			if (!lights[i].changed) continue;
			lights[i].changed = false;
			float r = lights[i].r;
			XMFLOAT3 rad = { r, r, r };
			shaderLights[i].center = lights[i].pos;
			shaderLights[i].radius = rad;
			shaderLights[i].index = i;
			shaderLights[i].active = (UINT32)true;
		}
	}*/
}