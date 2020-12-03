#pragma once

#include<curand.h>
#include<curand_kernel.h>
#include <thrust/random/linear_congruential_engine.h>
#include <thrust/random/uniform_real_distribution.h>

#define M_PI 3.1415f
class Sampling
{

	public:
		//thrust::minstd_rand rng;
		//thrust::uniform_real_distribution<float> dist = thrust::uniform_real_distribution<float>(0.0F, 1.0F);
		__device__ static float3 rand_hemisphere()
		{
			thrust::minstd_rand rng;
			thrust::uniform_real_distribution<float> dist(0.0f, 1.0f);
			return normalize(2 * make_float3(dist(rng), dist(rng), dist(rng)) - make_float3(1.0f, 1.0f, 1.0f));
			/*float u = dist(rng);
			float v = dist(rng);
			float r = sqrtf(1 - v * v);*/
			/*return make_float3(r * cosf(2 * M_PI * u), v, r * sinf(2 * M_PI * u));*/
		}

		__device__ static float rand()
		{
			thrust::minstd_rand rng;
			thrust::uniform_real_distribution<float> dist(0.0f, 1.0f);
			return dist(rng);
			//return 0;
		}

		/*
		static rand_cos_hemisphere()
		{
			let theta = Math.random(2 * Math.PI);
			let s = Math.random();
			let y = Math.sqrt(s);
			let r = Math.sqrt(1 - y * y);
			return new Vec3f(r * Math.cos(theta), y, r * Math.sin(theta));
		};
		*/
		__device__ static float uniform_sampling()
		{
			return 1.0f / (2.0f * M_PI);
		}

		__device__ static float importance_sampling_lambertian(float3 normal, float3 outgoing_dir)
		{
			return dot(normal, outgoing_dir) / M_PI;
		}

		__device__ static float3 GGXImportanceSample(const float3 &normal)
		{
			return normal + rand_hemisphere(); //change later
		}
};
