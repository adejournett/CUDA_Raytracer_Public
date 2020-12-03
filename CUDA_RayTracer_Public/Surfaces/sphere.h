#pragma once

#include"ray.h"
#include"hitrec.h"
#include "surface.h"
#include"lambertian.h"

class Sphere : public Surface
{
	public: 
		float3 position;
		Lambertian material;
		float radius;

		__device__ Sphere() {}
		__device__ Sphere(const float3 &p, float r, const Lambertian &m)
		{
			position = p;
			radius = r;
			material = m;
		}

		__device__ bool intersect(Ray r, float t_min, float hit_t, HitRec &rec)
		{
			float3 oc = r.origin() - position;
			float a = dot(r.direction(), r.direction());
			float b = 2.0f * dot(oc, r.direction());
			float c = dot(oc, oc) - radius * radius;
			float discriminant = b * b - 4.0f*a*c;
			
			if (discriminant > 0)
			{
				float t1 = ((-b + sqrtf(discriminant)) / (2 * a));
				float t2 = ((-b - sqrtf(discriminant)) / (2 * a));

				float t = (t1 < t2) ? t1 : t2;
				t = (t > hit_t) ? hit_t : t;

				rec.hit_t = t;
				rec.normal = ((r.origin() + (r.direction() * t)) - position) / radius;
				return true;
			}
			
			rec.hit_t = t_min;
			return false;
		}
};