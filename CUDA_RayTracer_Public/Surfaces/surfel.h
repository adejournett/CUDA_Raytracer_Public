#pragma once

#include"surface.h"
#include"material.h"
#include"hitrec.h"
#include"ray.h"

class Surfel : public Surface
{
	public:
		float3 point;
		float3 normal;
		float radius;
		float radius_sq;
		int mat;
		Material material;
		/*!
			default constructor for Surfels
		*/
		__host__ __device__ Surfel() {}

		/*!
			Primary constructor for Surfels
			
			\param p central position of the surfel
			\param n normal vector of the surfel
			\param r radius of the splat
			\param m material of the surfel
			\param ma materialID 
		*/
		__host__ __device__ Surfel(const float3 &p, const float3 &n, float r, const Material &m, int ma)
		{
			point = p;
			normal = n;
			radius = r;
			material = m;
			radius_sq = radius * radius;
			mat = ma;
		}
		
		/*!
			intersection calculation for the ray and a surfel (disk).

			\param r the ray
			\param rec the hitrecord, which stores geometric and parametric information for a ray hit
		*/
		inline __device__ bool intersect(const Ray &r, HitRec &rec)
		{
			//disk
			float denom = dot(-normal, r.direction());
			if (denom > 0.0f)
			{
				float3 p0l0 = point - r.origin();
				float t = dot(p0l0, -normal) / denom;
				rec.hit_t = t;

				if (t >= 0.0f)
				{
					float3 p = r.point_at_parameter(rec.hit_t);
					float3 v = p - point;
					float d2 = dot(v, v);
					rec.normal = normal;
					return d2 <= (radius_sq);
				}
			}
			return false;
		}
	private:
		/*!
			check if the ray intersects the plane of the given surfel
			
			\param r the ray
			\param t_min the ray's parametric origin
			\param hit_t the ray's parametric hit pos
			\param rec the hitrecord, which stores geometric and parametric information of the surface intersection
		*/
		__device__ bool intersectPlane(Ray &r, float t_min, float hit_t, HitRec &rec)
		{
			// assuming vectors are all normalized
			float denom = dot(normal, r.direction());
			if (denom > 1e-6) {
				float3 p0l0 = point - r.origin();
				float t = dot(p0l0, normal) / denom;
				rec.hit_t = t;
				rec.normal = normalize(normal);
				return (t >= 0);
			}
			rec.hit_t = 0.0f;
			return false;
		}
};
