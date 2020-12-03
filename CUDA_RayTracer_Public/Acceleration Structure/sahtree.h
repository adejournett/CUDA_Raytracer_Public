#pragma once

#include"surfel.h"
#include"AABB.h"
class SAHGPUNode;
class SAHNode;

class SAHNode
{
	public:
		SAHNode() {};
		SAHNode(SAHNode *node)
		{
			axis = node->axis;
			splitPos = node->splitPos;
			//left and right are null because thisis used when converting to array indexing
			left = NULL;
			right = NULL;
			numSurfels = node->numSurfels;
			surfels = node->surfels;
			box = NULL; //can't copy box to gpu, recompute in postprocess
		}

		int size();
		void inorder(int *nodenum);
		void flatten(SAHGPUNode *d_tree, AABB *d_AABB, int idx);

		AABB *box;
		SAHNode *left;
		SAHNode *right;
		int *surfels;
		int numSurfels;
		int axis;
		float splitPos;
};

class SAHGPUNode
{
	public:
		SAHGPUNode()
		{
			axis = -1;
			for (int i = 0; i < 6; i++)
				ropes[i] = -1;
		}
		SAHGPUNode(SAHNode *node)
		{
			axis = node->axis;
			splitPos = node->splitPos;
			numSurfels = node->numSurfels;
			surfels = node->surfels;
			for (int i = 0; i < 6; i++)
				ropes[i] = -1;
		}


		friend std::ostream& operator<<(std::ostream &os, const SAHGPUNode &obj)
		{
			os.write((char *)&obj.axis, sizeof(int));
			os.write((char *)&obj.splitPos, sizeof(float));
			os.write((char *)&obj.numSurfels, sizeof(int));
			//for (int i = 0; i < obj.numSurfels; i++)
			//{
			os.write((char *) obj.surfels, sizeof(int) * obj.numSurfels);
			//}

			//for (int i = 0; i < 6; i++)
			//{
				os.write((char *) obj.ropes, sizeof(int) * 6);
			//}

			return os;
		}

		friend std::istream& operator>>(std::istream &is, SAHGPUNode &obj)
		{
			is.read((char *) &obj.axis, sizeof(int));
			is.read((char *) &obj.splitPos, sizeof(float));
			is.read((char *) &obj.numSurfels, sizeof(int));

			cudaMallocManaged((void **)&obj.surfels, sizeof(int) * obj.numSurfels);
			//for (int i = 0; i < obj.numSurfels; i++)
			//{
				is.read((char *) obj.surfels, sizeof(int) * obj.numSurfels);
			//}

			//for (int i = 0; i < 6; i++)
			//{
				//obj.ropes[i] = -1;
				is.read((char *) obj.ropes, sizeof(int) * 6);
			//}

			return is;
		}
		int *surfels;
		int numSurfels;
		int axis;
		float splitPos;
		int ropes[6]; //store ropes in SAHGPUNode to improve cache hitrate
};
class SAHTree
{
	public:
		SAHTree() {};
		SAHTree(Surfel *points, int numSurfels, int maxDepth, std::string &treename);
		SAHTree(Surfel *surfels, int numSurfels, std::ifstream &file);
		int size();
		void save(std::string &treename);
		SAHNode *root;
		SAHGPUNode *d_tree;
		AABB *d_AABB;
	private:
		SAHNode *build_sahtree(Surfel *surfels, AABB *boxes, int numSurfels, AABB *container, int axis, const std::vector<int> &bucket, int depth);
};

