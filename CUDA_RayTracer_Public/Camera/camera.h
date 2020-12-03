#pragma once

//definitions
#ifndef M_PI
#define M_PI 3.1415f
#endif // !M_PI

#include"ray.h"


class Camera {
	public:
		__device__ Camera() {};
		__device__ Camera(float3 lookfrom, float3 lookatv, float3 vup, float vfov, int wid, int hei) 
		{
			lookat = lookatv;
			width = wid;
			height = hei;
			float aspect = float(wid) / float(hei);
			
			float3 u, v, w;
			float theta = (vfov * M_PI) / 180.0f;
			float half_height = tanf(theta / 2);
			float half_width = aspect * half_height;
			eye = lookfrom;
			w = normalize(lookfrom - lookat);
			u = normalize(cross(vup, w));
			v = cross(w, u);
			lower_left_corner = eye - half_width * u - half_height * v - w;
			horizontal = 2 * half_width*u;
			vertical = 2 * half_height*v;
		}
		__device__ Ray computeEyeRay(float u, float v) 
		{ 
			return Ray(eye, lower_left_corner + (u/width) * horizontal + (v/height) * vertical - eye); 
		}

		int width, height;
		float3 eye;
		float3 lower_left_corner;
		float3 horizontal;
		float3 vertical;
		float3 lookat;
};


