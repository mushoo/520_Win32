struct AABBNode
{
	float3 max;
	float3 min;
	uint index;
	uint zCode;
};

cbuffer ConstantBuffer : register(b0)
{
	uint depth;
};

RWStructuredBuffer<AABBNode> treeNodes : register(u0);

//http://devblogs.nvidia.com/parallelforall/thinking-parallel-part-iii-tree-construction-gpu/
// Expands a 10-bit integer into 30 bits
// by inserting 2 zeros after each bit.
unsigned int expandBits(uint v)
{
	v = (v * 0x00010001u) & 0xFF0000FFu;
	v = (v * 0x00000101u) & 0x0F00F00Fu;
	v = (v * 0x00000011u) & 0xC30C30C3u;
	v = (v * 0x00000005u) & 0x49249249u;
	return v;
}

// Calculates a 30-bit Morton code for the
// given 3D point located within the unit cube [0,1].
uint morton3D(float3 p)
{
	p.x = min(max(p.x * 1024.0f, 0.0f), 1023.0f);
	p.y = min(max(p.y * 1024.0f, 0.0f), 1023.0f);
	p.z = min(max(p.z * 1024.0f, 0.0f), 1023.0f);
	uint xx = expandBits((uint)p.x);
	uint yy = expandBits((uint)p.y);
	uint zz = expandBits((uint)p.z);
	return xx * 4 + yy * 2 + zz;
}

[numthreads(32, 32, 1)]
void main(
	uint3 Gid : SV_GroupID,
	uint3 GTid : SV_GroupThreadID
	)
{
	uint lane = GTid.x;
	uint warpid = GTid.y;
	uint index = 1024 * Gid.x + 32 * warpid + lane;
	uint offset = ((1 << 5 * depth) - 1) / 31;

	float3 p = (treeNodes[index + offset].max - treeNodes[0].min) / (treeNodes[0].max - treeNodes[0].min);

	treeNodes[index + offset].zCode = morton3D(p);
}