#pragma once
#include<cuda.h>
#include<cuda_runtime.h>
class MicrosurfaceHeight
{

public:
	__host__ __device__ MicrosurfaceHeight() {};

};

class MicrosurfaceSlope
{
public:
	__host__ __device__ MicrosurfaceSlope() {};
	__host__ __device__ MicrosurfaceSlope(const float ax, const float ay)
	{
		alpha_x = ax;
		alpha_y = ay;
	}

private:


	float alpha_x;
	float alpha_y;
};