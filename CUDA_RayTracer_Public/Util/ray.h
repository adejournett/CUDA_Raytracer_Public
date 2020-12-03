#pragma once
#include<limits>
#include<cuda.h>
//#include<cuda_runtime.h>
#include"cuda_util.h"
//#include<device_launch_parameters.h>
//#include<device_functions.h>

class Ray
{
	public:
		__device__ Ray() {}
		__device__ Ray(float3 &a, float3 &b) 
		{
			A = a;
			B = b;
			invDir = make_float3(1.0f, 1.0f, 1.0f) / (B + 10.0f * make_float3(FLT_EPSILON, FLT_EPSILON, FLT_EPSILON));
			sign[0] = (b.x >= 0);
			sign[1] = (b.y >= 0);
			sign[2] = (b.z >= 0);
		}
		inline __device__ float3 origin() const { return A; }
		inline __device__ float3 direction() const { return B; }
		inline __device__ float3 invDirection() const { return invDir; }
		inline __device__ float3 point_at_parameter(float t) const { return A + t * B; }
		
		float3 A;
		float3 B;
		float3 invDir;
		int sign[3];
};
