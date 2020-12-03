#pragma once
#include"microfacet_util.h"
#include"material.h"
//--------------------------------------------------------------------------------------------------------------------
//CODE BELOW THIS LINE IS A TEMPLATE IN THE PUBLIC VERSION, ONLY INTENDED FOR THE PROGRAM TO BUILD. RESEARCH RELATED CODE HAS BEEN REMOVED. 
//--------------------------------------------------------------------------------------------------------------------
class MicrofacetMultipleScatter : public Material
{
public:
	__host__ __device__ MicrofacetMultipleScatter() {};
	__host__ __device__ MicrofacetMultipleScatter(const float3 &a, const float ax, const float ay)
	{
		albedo = a;
		alpha_x = ax;
		alpha_y = ay;
	};

private:
	float alpha_x, alpha_y;

};