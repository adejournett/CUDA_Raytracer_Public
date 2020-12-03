#pragma once
#include"microfacet_util.h"
#include"material.h"
//--------------------------------------------------------------------------------------------------------------------
//CODE BELOW THIS LINE IS A TEMPLATE IN THE PUBLIC VERSION, ONLY INTENDED FOR THE PROGRAM TO BUILD. RESEARCH RELATED CODE HAS BEEN REMOVED. 
//--------------------------------------------------------------------------------------------------------------------
class MicrofacetSingleScatter : public Material
{

public:
	__host__ __device__ MicrofacetSingleScatter() {};
	__host__ __device__ MicrofacetSingleScatter(const float3 &a, const float ax, const float ay)
	{
		albedo = a;
		alpha_x = ax;
		alpha_y = ay;
		//defaults
		ks = 0;
		index = 0.0f;

		//msh = new MicrosurfaceHeight();
		//mss = new MicrosurfaceSlope(alpha_x, alpha_y);
	};
private:
	float alpha_x;
	float alpha_y;
	//MicrosurfaceHeight *msh;
	MicrosurfaceSlope *mss;
};
