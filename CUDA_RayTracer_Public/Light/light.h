#pragma once
#include<cuda_runtime.h>
#include<cuda.h>

class Light
{
	public:
		float3 position;
		float3 color;
		float3 a;
		float3 b;
		bool isArea;
		__host__ __device__ Light() {}
		__host__ __device__ Light(const float3 &pos, const float3 &col, const float3 &v1, const float3 &v2, bool area)
		{
			a = v1;
			b = v2;
			isArea = area;
			position = pos;
			color = col;
		}
};