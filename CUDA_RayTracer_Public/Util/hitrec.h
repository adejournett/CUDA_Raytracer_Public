#pragma once

#include<cuda_runtime.h>

class HitRec
{
	public:
		float hit_t;
		float3 normal;
		__device__ HitRec() 
		{
			hit_t = 0.0f;
			//normal = NULL;
		}
		__device__ HitRec(float h, float3 n)
		{
			hit_t = h;
			normal = n;
		}
};