#pragma once

#include"material.h"

/*!
	This is the "advanced" material demonstration in the public release of the CUDA BSSRDF Raytracer
	The implementation here is a BRDF reduction of the BSSRDF given in \cite{Jensen Practical 2001}
	Please note there is also a BSDF reduction given in \cite{A Physically-Based BSDF for Modeling the appearance of Paper}
*/
class BSSRDF_Approx : public Material
{
public:
	/*!
		Default constructor for BSSRDF_Approx material
	*/
	__host__ __device__ BSSRDF_Approx() {};

	/*!
		Primary constructor for BSSRDF_approx material

		\param a the single-scatter albedo
		\param ind the real component of the index of refraction
		\param ext the extinction coefficient
		\param abs the absorbtion coefficient
		\param sca the scattering coefficient
		\param g the HG single variate phase function parameter
	*/
	__host__ __device__ BSSRDF_Approx(const float3 &a, float ind, float ext, float abs, float sca, float g)
	{
		albedo = a;
		index = ind;
		extinction = ext;
		absorbtion = abs;
		scattering = sca;
		mean_cos = g;

		//compute reduced coefficients as seen in jensen 2001
		r_sca = scattering * (1.0f - mean_cos);
		r_ext = r_sca + absorbtion;
		r_alb = (r_sca / r_ext);

		A = (1.0f + index) / (1.0f - index);
	};
	float r_ext, r_sca, A;
	float r_alb;


	__device__ float3 sampleBSDF(const float3 &norm, const float3 &w_i, const float3 &w_o)
	{
		float F = fresnel(w_o, norm);

		float single = F * (phaseFunction(w_i, w_o) / (dot(norm, w_i) + dot(norm, w_o)));
		float sq = sqrtf(3.0f * (1.0f - (r_sca / r_ext)));
		float rdiff = (r_alb / 2.0f) * (1.0f + expf(-(4.0f / 3.0f) * A * sq)) * expf(-sq);
		float diff = F * rdiff / M_PI;

		return albedo * (single + diff);
	}

private:
	__device__ float phaseFunction(const float3 &w_i, const float3 &w_o)
	{
		return 1 / (4 * M_PI); //isotropic
	}

	__device__ float fresnel(float3 w_i, float3 norm)
	{
		float c = dot(w_i, norm);
		float g = sqrtf(index * index - 1.0f + c * c);
		float gmc = g - c;
		float gpc = g + c;


		return ((gmc * gmc) / (2 * gpc * gpc)) * (1.0f + ((c * gpc - 1.0f) * (c * gpc - 1.0f)) / ((c * gmc + 1.0f) * (c * gmc + 1.0f)));
	}

};
