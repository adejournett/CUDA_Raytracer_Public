#pragma once

#include<float.h>
class AABB
{

public:
	__host__ __device__ AABB() 
	{
		min = make_float3(0.0f, 0.0f, 0.0f);
		max = make_float3(0.0f, 0.0f, 0.0f);
	}
	__host__ __device__ AABB(float3 mi, float3 ma)
	{
		min = mi;
		max = ma;
	}
	__host__ __device__ AABB(AABB *box)
	{
		min.x = box->min.x;
		max.x = box->max.x;
		min.y = box->min.y;
		max.y = box->max.y;
		min.z = box->min.z;
		max.z = box->max.z;
	}
	__host__ __device__ AABB(float xmi, float xma, float ymi, float yma, float zmi, float zma)
	{
		min = make_float3(xmi, ymi, zmi);// -make_float3(1e-6f, 1e-6f, 1e-6f);
		max = make_float3(xma, yma, zma);// +make_float3(1e-6f, 1e-6f, 1e-6f);
	}
	__host__ __device__ bool contains(const Surfel &surfel)
	{
		float radius = surfel.radius;
		float x = surfel.point.x;
		float y = surfel.point.y;
		float z = surfel.point.z;
		float eps = FLT_EPSILON;// 1e-2f;
		//radius = radius * 1.1f;
		if (x >= (min.x - radius - eps) && x <= (max.x + radius + eps) && y >= (min.y - radius - eps) && y <= (max.y + radius + eps) && z >= (min.z - radius - eps) && z <= (max.z + radius + eps))
			return true;
		else
			return false;
	}

	__host__ __device__ bool intersectBox(AABB box)
	{
		return (max.x >= box.min.x && box.max.x >= min.x &&
				max.y >= box.min.y && box.max.y >= min.y &&
				max.z >= box.min.z && box.max.z >= min.z);
	}

	__host__ __device__ int maxExtent()
	{
		float3 diag = max - min;
		if (diag.x > diag.y && diag.x > diag.z)
			return 0;
		else if (diag.y > diag.z)
			return 1;
		else
			return 2;
	}

	__host__ __device__ float3 center()
	{
		return (min + max) / 2.0f;
	}

	__host__ __device__ float offset(float3 center, int dim)
	{
		float3 o = center - min;
		if (max.x > min.x) o.x /= max.x - min.x;
		if (max.y > min.y) o.y /= max.y - min.y;
		if (max.z > min.z) o.z /= max.z - min.z;
		if (dim == 0) //x axis
		{
			return o.x;
		}
		else if (dim == 1) //y axis
		{
			return o.y;
		}
		else //z axis
		{
			return o.z;
		}
	}

	__host__ __device__ AABB *join(AABB box)
	{
		float3 mi = make_float3(fminf(min.x, box.min.x), fminf(min.y, box.min.y), fminf(min.z, box.min.z));
		float3 ma = make_float3(fmaxf(max.x, box.max.x), fmaxf(max.y, box.max.y), fmaxf(max.z, box.max.z));
		AABB *b = new AABB(mi, ma);
		return b;
	}

	__host__ __device__ float surfaceArea()
	{
		float3 diag = max - min;
		return 2.0f * (diag.x * diag.y + diag.x * diag.z + diag.y * diag.z);
	}

	__host__ __device__ bool containsPoint(float3 point)
	{
		if (point.x >= min.x && point.x <= max.x &&
			point.y >= min.y && point.y <= max.y &&
			point.z >= min.z && point.z <= max.z)
			return true;
		else
			return false;
	}

	__host__ __device__ AABB divide(int num, int denom, int axis)
	{
		AABB box;
		if (axis == 0)
		{
			float minsplitPt = min.x + (num - 1) * (fabsf(max.x - min.x) / (float)denom);
			float maxsplitPt = min.x + (num) * (fabsf(max.x - min.x) / (float)denom);
			box.min = make_float3(minsplitPt, min.y, min.z);
			box.max = make_float3(maxsplitPt, max.y, max.z);
		}
		else if (axis == 1)
		{
			float minsplitPt = min.y + (num - 1) * (fabsf(max.y - min.y) / (float)denom);
			float maxsplitPt = min.y + (num) * (fabsf(max.y - min.y) / (float)denom);
			box.min = make_float3(min.x, minsplitPt, min.z);
			box.max = make_float3(max.x, maxsplitPt, max.z);
		}
		else
		{
			float minsplitPt = min.z + (num - 1) * (fabsf(max.z - min.z) / (float)denom);
			float maxsplitPt = min.z + (num) * (fabsf(max.z - min.z) / (float)denom);
			box.min = make_float3(min.x, min.y, minsplitPt);
			box.max = make_float3(min.x, max.y, maxsplitPt);
		}

		return box;
	}

	__device__ inline float3 generateFar(const Ray & r)
	{
		return make_float3((r.sign[0] == 1) ? max.x : min.x, (r.sign[1] == 1) ? max.y : min.y, (r.sign[2] == 1) ? max.z : min.z);
	}
	/*
		Ray-AABB intersection code as seen in A Ray-Box Intersection Algorithm and Efficient Dynamic Voxel Rendering (NVIDIA Research 2018)
	*/
	__device__ bool intersect(const Ray &r, float &tmin, float &tmax)
	{
		float3 t0s = (min - r.origin()) * r.invDir;
		float3 t1s = (max - r.origin()) * r.invDir;

		float3 tsmaller = fminf(t0s, t1s);
		float3 tbigger = fmaxf(t0s, t1s);

		tmin = fmaxf(tmin, fmaxf(tsmaller.x, fmaxf(tsmaller.y, tsmaller.z)));
		tmax = fminf(tmax, fminf(tbigger.x, fminf(tbigger.y, tbigger.z)));

		tmax *= 1.00000024f;
		return (tmin <= tmax);
	}


	friend std::ostream& operator<<(std::ostream& os, const AABB& obj)
	{
		os.write((char *)&obj.min.x, sizeof(float));
		os.write((char *)&obj.min.y, sizeof(float));
		os.write((char *)&obj.min.z, sizeof(float));

		os.write((char *)&obj.max.x, sizeof(float));
		os.write((char *)&obj.max.y, sizeof(float));
		os.write((char *)&obj.max.z, sizeof(float));

		return os;
	}

	friend std::istream& operator>>(std::istream& is, AABB& obj)
	{
		is.read((char *)&obj.min.x, sizeof(float));
		is.read((char *)&obj.min.y, sizeof(float));
		is.read((char *)&obj.min.z, sizeof(float));

		is.read((char *)&obj.max.x, sizeof(float));
		is.read((char *)&obj.max.y, sizeof(float));
		is.read((char *)&obj.max.z, sizeof(float));

		return is;
	}

	float3 min;
	float3 max;

};