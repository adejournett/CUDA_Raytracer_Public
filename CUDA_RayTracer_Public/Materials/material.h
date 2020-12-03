#pragma once

#include<cuda.h>
#include<cuda_runtime.h>
#include"cuda_util.h"
#include"sampling.h"


/*
	The base class for Material definition
	For purposes of evaluation, the fields in this class correspond to any material that the raytracer supports
	this allows the material model evaluation to be swapped out on the fly in the GPU, since some versions of CUDA don't support inheritance or virtual functions
*/
class Material
{
	public:
		float3 emittance; //emittance of material, used for cherenkov radiation, bioluminescents, and light sources
		float3 albedo;
		float ks; //specular coefficient
		float index; //index of refraction for transparent materials
		float roughness; //cook-torrance roughness
		float roughness_sq; //optimization param
		float absorbtion, scattering, extinction, mean_cos; //bssrdf

		__host__ __device__ Material() {}
		__device__ virtual float3 sampleBSDF(const float3 &norm, const float3 &w_i, const float3 &w_o) { return make_float3(0.0f, 0.0f, 0.0f); }
};
