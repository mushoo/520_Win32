#pragma once

#include "ShaderStructures.h"
#include "objLoader\tiny_obj_loader.h"
#include "pch.h"

using namespace DirectX;

namespace _520 {
	
	struct SimpleMesh
	{
	public:
		int m_vertexOffset;
		int m_nVertices;
		int m_triIndexOffset;
		int m_nTriIndices;
	};

}