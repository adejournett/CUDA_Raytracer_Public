#pragma once


#include"light.h"
#include"sphere.h"
#include"camera.h"
#include"surfel.h"
#include"AABB.h"
#include"kdtree.h"
#include"sahtree.h"
#include"happly.h"
#include<iostream>
#include<time.h>

class Scene
{
	public:
		Light *lights;
		Surfel *surfels;
		Rope *ropes;
		SAHGPUNode *tree;
		AABB *boxes;
		Camera *camera;
		int numLights;
		int size;
		int numSurfels;

		/*!
		* Primary constructor for the Scene class
		*
		* \param l pointer to the array of lights in the scene. Should be in device memory
		* \param nl number of lights in the scene
		* \param c camera object, also stored in device memory
		* \param s pointer to array of surfels in the scene
		* \param t flattened SAHTree, can be stored in unified or device memory
		* \param b AABBs corresponding to flattened tree, can be stored in unified or device memory
		* \param r Rope objects corresponding to flattened tree
		* \param sz size of the flattened array
		* \param ns size of the surfel array
		*/
		__device__ Scene(Light *l, int nl, Camera *c, Surfel *s, SAHGPUNode *t, AABB *b, Rope *r, int sz, int ns)
		{
			lights = l;
			surfels = s;
			tree = t;
			boxes = b;
			camera = c;
			numLights = nl;
			size = sz;
			ropes = r;
			numSurfels = ns;
		}

		/*!
			Shadow ray calculation. Returns false if the point is in shadow.
			Much of the code is similar to intersectTree, but as an optimization instead of checking for the closest ray hit, we check for the first ray hit and return at 
			that point

			\param toLight the ray to check
			\param lightT the parametric value of the ray which gives the origin. Typically this is the hitT of a ray that has hit an object already
		*/
		inline __device__ bool lineOfSight(const Ray &toLight, float lightT)
		{
			int idx = 1;
			SAHGPUNode *node;
			float t_entry = 0.0f;
			float t_exit = FLT_MAX;

			while (idx > 0 && idx < size)
			{
				node = &tree[idx];
				float3 p_entry = toLight.point_at_parameter(t_entry);
				//this loop descends to a leaf node from the current node
				while (idx < size && node->numSurfels < 0) //while node exists and is NOT leaf
				{

					//descend to a leaf node according to split axis and position of the entry point as seen in Popov et al
					switch (node->axis)
					{
					case 0:
						idx = (p_entry.x < node->splitPos) ? (2 * idx) : (2 * idx + 1);
						break;
					case 1:
						idx = (p_entry.y < node->splitPos) ? (2 * idx) : (2 * idx + 1);
						break;
					case 2:
						idx = (p_entry.z < node->splitPos) ? (2 * idx) : (2 * idx + 1);
						break;
					}

					//printf("New idx: %d\n", idx);
					node = &tree[idx];
				}

				float3 farPos = (boxes[idx].generateFar(toLight) - toLight.origin()) * toLight.invDir;
				t_exit = fminf(farPos.x, fminf(farPos.y, farPos.z));
				if (t_exit < 0.0f)
					return false;
				//now that we've reached a leaf, iterate over all surfels within and check intersections
				HitRec temp_rec;
				for (int j = 0; j < node->numSurfels; j++)
				{
					//intersect ray with all surfels in AABB
					Surfel s = surfels[node->surfels[j]];
					if (s.intersect(toLight, temp_rec) && temp_rec.hit_t < lightT)
					{
						return false;
					}
				}
				if (t_exit < t_entry)
					return false;
				if (t_exit == farPos.x)
				{
					idx = 0;
				}
				else if (t_exit == farPos.y)
				{
					idx = 1;
				}
				else if (t_exit == farPos.z)
				{
					idx = 2;
				}

				//idx = (t_exit == farPos.x) || (t_exit == farPos.y) || (2);
				idx = node->ropes[idx + 3 * toLight.sign[idx]];
				t_entry = t_exit;
				//iter_ct++;
			}
			
			return true;
		}

		/*!
			This function calculates the nearest surfel a ray intersects, and then computes a normal vector and a hit position
			based on the sum of gaussians given in the Adamson-Alexa surface model, according to the Wald-Seidel ray-marching optimization. 

			\cite{Interactive Ray-tracing of Point-based Models}
			\cite{Stackless KD-Tree Traversal for High Performance GPU Ray-Tracing}
			\cite{Robust BVH Tree Traversal}

			\param rec the hit record. Stores the normal, hitPos, and hitT
			\param r the ray being cast into the scene
			\param surf the surfel the ray hits
			\param t_entry the parametric entry point of the ray. this lies exactly on one face of the scenes outermost bounding box
			\param t_exit the parametric exit point of the ray, this lies on the opposite face as t_entry
			\param isFrame a bool used to determine if the program is meant to only render a single frame. Used in debugging, omitted in release build
		*/
		inline __device__ bool intersectTree(HitRec &rec, const Ray &r, Surfel &surf, float t_entry, float t_exit, bool isFrame)
		{
			//t_entry is the entry value the ray intersects the root bounding box, 
			//t_exit is the exit value with which the ray leaves the root bounding box
			int idx = 1;
			SAHGPUNode *node; //start at root
			float main_exit = t_exit;
			//while ray has not exited the roots bounding box
			int iter_ct = 0;
			int max_iters = 4000;
			while (idx > 0 && idx < size && tree[idx].axis != -1 && t_entry < main_exit && iter_ct < max_iters) //continue checking nodes until the ray finds a hit, exits the tree, or loses all signifcant energy
			{
				node = &tree[idx];
				float3 p_entry = r.point_at_parameter(t_entry); //calculate entry point of the current AABB
				//this loop descends to a leaf node from the current node
				while (node->numSurfels == 0 && node->axis != -2) //while node exists and is NOT leaf
				{
					//descend to a leaf node according to split axis and position of the entry point as seen in Popov et al
					switch (node->axis)
					{
					case 0:
						idx = (p_entry.x < node->splitPos) ? (2 * idx) : (2 * idx + 1);
						break;
					case 1:
						idx = (p_entry.y < node->splitPos) ? (2 * idx) : (2 * idx + 1);
						break;
					case 2:
						idx = (p_entry.z < node->splitPos) ? (2 * idx) : (2 * idx + 1);
						break;
					case -1:
						return false;
					}
					node = &tree[idx];
				}

				float3 farPos = (boxes[idx].generateFar(r) - r.origin()) * r.invDir;
				t_exit = fminf(main_exit, fminf(farPos.x, fminf(farPos.y, farPos.z)));
				if (t_exit * 1.00000024f <= t_entry) //floating point error correction mentioned in Ropes++ paper
					return false;

				//apply adamson-alexa intersection to current leaf
				//this is the non-SIMD implementation given in Wald-Seidel
				int k = 100; //number of samples along the ray
				float oldF = 0;
				float oldT = t_entry;
				for (int i = 0; i < k; i++)
				{
					float t = lerp(t_entry, t_exit, (float)i / (float)(k - 1)); //interpolate here
					float3 x = r.point_at_parameter(t);
					float W = 0;
					float3 N = make_float3(0.0f, 0.0f, 0.0f);
					float3 P = make_float3(0.0f, 0.0f, 0.0f);
					//iterate over every splat in the leaf (without a tree this is every splat in the model)
					for (int splat = 0; splat < node->numSurfels; splat++)
					{
						surf = surfels[node->surfels[splat]];
						float w = (1.0f / (surf.radius * sqrtf(2.0f  * M_PI))) * expf(-0.5f * powf(((length(x - surf.point)) - 0.0f) / surf.radius, 2));
						w = (w >= 0.001f) ? w : 0.0f; //check if the current surfel contributes to the surface in any significant way
						if (w == 0.0f)
							continue;
						W += w;
						N += w * surf.normal;
						P += w * surf.point;
					}
					if (W == 0.0f)
						continue;
					float F = dot((W * x - P), N); //check if the approximate plane of the surface is facing in the same direction as the ray
					if (F * oldF < 0.0f)
					{
						rec.hit_t = lerp(oldT, t, (oldF / (oldF - F))); //interpolate
						x = r.point_at_parameter(rec.hit_t);
						float3 C = make_float3(0.0f, 0.0f, 0.0f);
						rec.normal = make_float3(0.0f, 0.0f, 0.0f);
						for (int splat = 0; splat < node->numSurfels; splat++)
						{
							surf = surfels[node->surfels[splat]]; //(length(x - surf.point) > 2.0f * surf.radius) ? 0.0f : 
							float w = (1.0f / (surf.radius * sqrtf(2.0f  * M_PI))) * expf(-0.5f * powf(((length(x - surf.point)) - 0.0f) / surf.radius, 2));
							w = (w >= 0.001f) ? w : 0.0f;
							if (w == 0.0f)
								continue;
							W += w;
							rec.normal += w * surf.normal;
							C += w * surf.material.albedo;
						}

						//set and return final geometric and color contributions of the ray hit
						surf.material.albedo = (C / W);
						rec.normal = rec.normal / W;
						return true;
					}
					oldF = F;
					oldT = t;
				}

				//choose the rope to travel along for the next box the ray intersects
				if (fabsf(t_exit - farPos.x) <= fabsf(t_exit - farPos.y) && fabsf(t_exit - farPos.x) <= fabsf(t_exit - farPos.z))// >= farPos.x - eps && t_exit <= farPos.x + eps) //x-axis
				{
					idx = 0;
					idx = node->ropes[idx + 3 * r.sign[idx]];
				}
				else if (fabsf(t_exit - farPos.y) <= fabsf(t_exit - farPos.x) && fabsf(t_exit - farPos.y) <= fabsf(t_exit - farPos.z)) // farPos.y - eps && t_exit <= farPos.y + eps) //y-axis
				{
					idx = 1;
					idx = node->ropes[idx + 3 * r.sign[idx]];
				}
				else if (fabsf(t_exit - farPos.z) <= fabsf(t_exit - farPos.x) && fabsf(t_exit - farPos.z) <= fabsf(t_exit - farPos.y))// farPos.z - eps && t_exit <= farPos.z + eps) //z-axis
				{
					idx = 2;
					idx = node->ropes[idx + 3 * r.sign[idx]];
				}
				else //signals a floating point error, just restart at the top of the tree
				{
					idx = 1;
				}

				t_entry = t_exit;
				iter_ct++;
			}
			return false;
		}
};