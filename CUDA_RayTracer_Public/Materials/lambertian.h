#pragma once

#include"material.h"

/*!
	Lambertian (purely diffuse) material type
	note color is computed independent of light and viewing direction (isotropic bsdf)
*/
class Lambertian : public Material
{
public:
	/*!
		Default constructor for lambertian material
	*/
	__host__ __device__ Lambertian() {}
	
	/*!
	   Primary constructor for lambertian material

	   \param a the single scatter albedo of the material
	*/
	__host__ __device__ Lambertian(const float3 &a)
	{
		//reflectance = clamp(2 * a, 0.0f, 1.0f);
		emittance = make_float3(0.0f, 0.0f, 0.0f);
		albedo = a;
		ks = 0;
		index = 0.0f;
	}

	/*!
		sample the lambertian bsdf.
		for our purposes, this returns the un-normalized albedo.
		we do not use a normalzed albedo because of screen resolution

		\param norm the geometric hit normal
		\param w_i the incident light direction
		\param w_o the exitant, scattered light direction
	*/
	__device__ float3 sampleBSDF(const float3 &norm, const float3 &w_i, const float3 &w_o)
	{
		return albedo;
	}
};