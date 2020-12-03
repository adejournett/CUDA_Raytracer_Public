#pragma once
#include"material.h"
//--------------------------------------------------------------------------------------------------------------------
//CODE BELOW THIS LINE IS A TEMPLATE IN THE PUBLIC VERSION, ONLY INTENDED FOR THE PROGRAM TO BUILD. RESEARCH RELATED CODE HAS BEEN REMOVED. 
//--------------------------------------------------------------------------------------------------------------------
class BSSRDF_Dipole : public Material
{
public:
	__host__ __device__ BSSRDF_Dipole() {};
	__host__ __device__ BSSRDF_Dipole(const float3 &a, float ind, float ext, float abs, float sca, float g)
	{
		albedo = a;
		index = ind;
		extinction = ext;
		absorbtion = abs;
		scattering = sca;
		mean_cos = g;

		r_sca = scattering * (1.0f - mean_cos);
		r_ext = r_sca + absorbtion;
		r_alb = (r_sca / r_ext);

		A = (1.0f + index) / (1.0f - index);
	};

	__device__ float3 sampleBSSRDF(const float3 &norm, const float3 &w_i, const float3 &w_o, const float3 &x_i)
	{
		float3 color = make_float3(0.1f, 0.8f, 0.1f);


		return clamp(color, 0.0f, 1.0f);
	}

private:
	float r_ext, r_sca, A;
	float r_alb;

};