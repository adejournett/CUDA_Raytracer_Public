#pragma once

#include "material.h"
/*!
	Cook-Torrance material
	This implementation is based on the paper "A Reflectance Model for computer graphics"
	Simulates roughness on the 1 micron scale via probability distribution functions

	All matghematics can be found in Cook-Torrance.
	The distribution can be found in Foley et. al. Computer Graphics: Principles and Practice 3rd ed.

*/
class Cook_Torrance : public Material
{
public:
	__host__ __device__ Cook_Torrance() {}
	__host__ __device__ Cook_Torrance(const float3 &a, float rough, float k, float ior)
	{
		albedo = a;
		emittance = make_float3(0.0f, 0.0f, 0.0f);
		roughness = rough;
		roughness_sq = roughness * roughness;
		ks = k;
		index = ior;
	}


	/*
		Computes BSDF contribution from diffuse and specular components

		\param norm the geometric normal
		\param w_i the incident light direction
		\param w_o the exitant, scattered light direction
	*/
	__device__ float3 sampleBSDF(const float3 &norm, const float3 &w_i, const float3 &w_o)
	{
		float3 newDir = make_float3(0.0f, 0.0f, 0.0f);
		float3 color = make_float3(0.0f, 0.0f, 0.0f);
		//float3 F0 = (make_float3(1.0f, 1.0f, 1.0f) - index) / (make_float3(1.0f, 1.0f, 1.0f) + index);


		color += albedo / M_PI + ks * albedo * Sample_GGX_Specular(norm, w_o, w_i, newDir);

		//color +=  Sample_GGX_Transmissive(norm, w_o, w_i, newDir);

		return color;
	}

	/*
		Sample the specular BSDF lobe
	*/
	__device__ inline float Sample_GGX_Specular(const float3 &norm, const float3 &w_o, const float3 &w_i, float3 &newDir)
	{
		float specular = 0.0f;
		newDir = w_i - 2 * dot(w_i, norm) * norm; //assumes norm is normalized, sets new direction to perfect mirror reflection
		float dot_norm_eye = dot(norm, w_o);

		float3 halfVector = normalize(w_i + w_o);

		//fresnel term
		float distribution = GGX_Distribution(norm, halfVector);
		float F = fresnel(w_i, norm); //might not need to use cos here
		float geom = GGX_Geometry(w_o, w_i, halfVector, norm);
		float denom = 4.0f * dot(w_i, norm) * dot_norm_eye; //optimize me

		specular += F * geom * distribution / denom; //dot product because of rendering eq

		return specular;
	}

	/*
		Sample transmissive lobe. Not used in practice, but good for comparisons
	*/
	__device__ inline float3 Sample_GGX_Transmissive(const float3 &norm, const float3 &w_o, const float3 &w_i, float3 &newDir)
	{

		float3 transmitted = make_float3(0.0f, 0.0f, 0.0f);

		//float3 sampleVector = Sampling::GGXImportanceSample(norm);
		float3 halfVector = normalize(w_i + w_o); //half vector between scattered and incident directions

		//dot product temp vars for optimization
		float toEye_dot_half = dot(w_o, halfVector);
		float sample_dot_half = dot(w_i, halfVector);

		//refracted vector direction
		newDir = ((1.0f / index) * sample_dot_half * -(dot(w_i, norm) >= 0.0f ? 1.0f : -1.0f) * sqrtf(1.0f + (1.0f / index) * (sample_dot_half * sample_dot_half - 1.0f))) * norm - (1.0f / index) * w_i;

		float firstProduct = (toEye_dot_half * sample_dot_half) / (dot(w_o, norm) * dot(w_i, norm));

		float numerator = index * index * (1.0f - fresnel(w_i, halfVector)) * GGX_Geometry(w_o, w_i, halfVector, norm) * GGX_Distribution(norm, halfVector);
		//incident index omitted (air)
		float temp = (1.0f * toEye_dot_half + index * sample_dot_half);
		float denom = temp * temp;

		transmitted += albedo * firstProduct * numerator * dot(norm, w_i) / denom; //dot product present here from the rendering equation

		return transmitted;
	}
private:
	/*
		GGX normal vector distribution
	*/
	__device__ float GGX_Distribution(const float3 &norm, const float3 &half)
	{
		float dot_prod = dot(norm, half);
		float dot_sq = (dot_prod * dot_prod);
		float numerator = roughness_sq * sign(dot_prod);
		float denom = M_PI * (1.0f + dot_sq * (roughness_sq - 1.0f)) * (1.0f + dot_sq * (roughness_sq - 1.0f));

		return numerator / denom;
	}

	/*
		visible normal term, masks the normal vector by
	*/
	__device__ float GGX_Geometry(const float3 &incoming, const float3 &outgoing, const float3 &dir, const float3 &norm)
	{
		return GGX_Partial_Geometry(incoming, dir, norm) * GGX_Partial_Geometry(outgoing, dir, norm);
	}

	/*
		shadow masking term, this essentially calculates the probability a given microfacet is in shadow from another microfacet
	*/
	__device__ float GGX_Partial_Geometry(const float3 &omega, const float3 &dir, const float3 &norm)
	{
		return 2.0f * dot(omega, norm) / (dot(omega, norm) * (2.0f - roughness) + roughness);
	}

	__device__ float fresnel(float3 w_i, float3 norm)
	{
		float c = dot(w_i, norm);
		float g = sqrtf(index * index - 1.0f + c * c);
		float gmc = g - c;
		float gpc = g + c;


		return ((gmc * gmc) / (2 * gpc * gpc)) * (1.0f + ((c * gpc - 1.0f) * (c * gpc - 1.0f)) / ((c * gmc + 1.0f) * (c * gmc + 1.0f)));
	}

	__device__ float sign(float x)
	{
		return (x > 0) ? 1 : 0;
	}
};