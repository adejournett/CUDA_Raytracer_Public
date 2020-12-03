#include<vector>
#include<fstream>
#include "sahtree.h"

int MAX = 0;
int totalSurfs = 0;
int totalLeafs = 0;
int maxSurfs = 0;
int MAXPRIMS = 10;

AABB *fitBoxesToSurfels(Surfel *surfels, int numSurfels)
{
	AABB *boxes = (AABB *)malloc(sizeof(AABB) * numSurfels);
	for (int i = 0; i < numSurfels; i++)
	{
		//naive box
		
		/*boxes[i] = AABB(surfels[i].point.x - surfels[i].radius, surfels[i].point.x + surfels[i].radius,
						surfels[i].point.y - surfels[i].radius, surfels[i].point.y + surfels[i].radius,
						surfels[i].point.z - surfels[i].radius, surfels[i].point.z + surfels[i].radius);*/
		
		//better box, account for the disk extent using the normal
		float3 bound = make_float3(1.0f, 1.0f, 1.0f) - (surfels[i].normal * surfels[i].normal);
		bound.x = sqrtf(bound.x);
		bound.y = sqrtf(bound.y);
		bound.z = sqrtf(bound.z);
		bound = surfels[i].radius * bound;
		boxes[i] = AABB(surfels[i].point - bound, surfels[i].point + bound);
	}

	return boxes;
}

void SAHNode::inorder(int *nodenum)
{
	if (!this) //null node
		return;
	else
	{
		*nodenum = *nodenum + 1;
		left->inorder(nodenum);

		std::cout << "Node [" << nodenum << "]" << "\n";
		std::cout << "\t" << "Num surfels: " << numSurfels << "\n";
		std::cout << "\t" << "Axis: " << axis << "\n";
		std::cout << "\t" << "Split Pos: " << splitPos << "\n";
		std::cout << "\t" << "Box:" << "\n";
		std::cout << "\t" << "\t" << "X:" << box->min.x << "~" << box->max.x << "\n";
		std::cout << "\t" << "\t" << "Y:" << box->min.y << "~" << box->max.y << "\n";
		std::cout << "\t" << "\t" << "Z:" << box->min.z << "~" << box->max.z << "\n";

		*nodenum = *nodenum + 1;
		right->inorder(nodenum);
	}
}

void SAHNode::flatten(SAHGPUNode *d_tree, AABB *d_AABB, int idx)
{
	if (left != NULL)
		left->flatten(d_tree, d_AABB, 2 * idx);
	if (right != NULL)
		right->flatten(d_tree, d_AABB, 2 * idx + 1);

	
	d_tree[idx] = SAHGPUNode(this);
	if (idx != 1)
		d_AABB[idx] = AABB(box);
}

int SAHTree::size()
{
	return (int)pow(2, MAX + 1) - 1;
}

int SAHNode::size()
{
	if (!this)
		return 0;
	else
		return 1 + this->left->size() + this->right->size();
}


void build_sahtree2(Surfel *surfels, AABB *boxes, int numSurfels, AABB *container, int axis, std::vector<int> bucket, int bucketSize, int depth, SAHGPUNode *nodes, AABB *d_AABB, int idx)
{
	if (depth > MAX)
		MAX = depth;
	//nodes[idx] = new SAHNode();
	SAHGPUNode *node = &nodes[idx];
	node->axis = axis; // axis;
	//node->box = new AABB(container);
	node->numSurfels = 0;
	node->surfels = NULL;
	d_AABB[idx] = AABB(container);

	if (bucket.size() <= 10 || depth == 1) //create leaf
	{

		node->numSurfels = bucket.size();
		//node->left = NULL;
		//node->right = NULL;
		node->axis = -2;
		node->splitPos = -1.0f;
		cudaMallocManaged((void **)&node->surfels, sizeof(int) * node->numSurfels);
		cudaMemcpy((void *)node->surfels, (void *)bucket.data(), sizeof(int) * bucketSize, cudaMemcpyHostToDevice);

		if (node->numSurfels > maxSurfs)
			maxSurfs = node->numSurfels;
		totalSurfs += node->numSurfels;
		totalLeafs += 1;
	}
	else
	{
		//choose split dimension
		int splitAxis = container->maxExtent();
		node->axis = splitAxis;
		float split_point;
		AABB *leftBox;
		AABB *rightBox;
		//std::cerr << "Num surfels: " << numSurfels << "\n";
		switch (splitAxis)
		{
		case 0:
			//split_point = surfels[xi[numSurfels / 2]].point.x;
			split_point = container->center().x;
			leftBox = new AABB(container->min.x, split_point, container->min.y, container->max.y, container->min.z, container->max.z);
			rightBox = new AABB(split_point, container->max.x, container->min.y, container->max.y, container->min.z, container->max.z);
			break;
		case 1:
			//split_point = surfels[yi[numSurfels / 2]].point.y;
			split_point = container->center().y;
			leftBox = new AABB(container->min.x, container->max.x, container->min.y, split_point, container->min.z, container->max.z);
			rightBox = new AABB(container->min.x, container->max.x, split_point, container->max.y, container->min.z, container->max.z);
			break;
		case 2:
			//split_point = surfels[zi[numSurfels / 2]].point.z;
			split_point = container->center().z;
			leftBox = new AABB(container->min.x, container->max.x, container->min.y, container->max.y, container->min.z, split_point);
			rightBox = new AABB(container->min.x, container->max.x, container->min.y, container->max.y, split_point, container->max.z);
			break;
		}

		//reassign xi, yi, and zi based on split axis 
		//std::cerr << "numSurfels: " << numSurfels << "\n";
		std::vector<int> leftBucket = std::vector<int>(numSurfels);
		std::vector<int> rightBucket = std::vector<int>(numSurfels);
		int leftCt = 0;
		int rightCt = 0;

		for (int i = 0; i < bucket.size(); i++)
		{
			//left box checks
			if (leftBox->contains(surfels[bucket[i]]))
				leftBucket[leftCt++] = bucket[i];
			//right box checks
			if (rightBox->contains(surfels[bucket[i]]))
				rightBucket[rightCt++] = bucket[i];
		}

		//std::cerr << "l: " << leftCt << " " << "r: " << rightCt << "\n";
		leftBucket.resize(leftCt);
		rightBucket.resize(rightCt);

		node->splitPos = split_point;


		//subdivide left box
		build_sahtree2(surfels, boxes, numSurfels, leftBox, splitAxis, leftBucket, leftCt, depth + 1, nodes, d_AABB, 2 * idx);
		//subdivide right box
		build_sahtree2(surfels, boxes, numSurfels, rightBox, splitAxis, rightBucket, rightCt, depth + 1, nodes, d_AABB, 2 * idx + 1);

	}

	nodes[idx] = *node;
}

SAHTree::SAHTree(Surfel *surfels, int numSurfels, std::ifstream &file)
{
	std::vector<int> &bucket = std::vector<int>(numSurfels);
	for (int i = 0; i < numSurfels; i++)
		bucket[i] = i;
	//SAHGPUNode *nodes; //(SAHGPUNode **)malloc(sizeof(SAHNode *) * (pow(2, 16 + 1) - 1));
	cudaMallocManaged((void **)&d_tree, sizeof(SAHGPUNode) * (pow(2, 20 + 1) - 1));
	for (int i = 0; i < (pow(2, 20 + 1) - 1); i++)
		d_tree[i] = SAHGPUNode();
	cudaMallocManaged((void **)&d_AABB, sizeof(AABB) * (pow(2, 20 + 1) - 1));
	for (int i = 0; i < (pow(2, 20 + 1) - 1); i++)
		d_AABB[i] = AABB();
	//read depth
	file.read((char *) &MAX, sizeof(int));
	//iterate over file and read in tree
	int idx = 0;
	while(file)
	{
		file.read((char *)&idx, sizeof(int));
		//std::cerr << "idx: " << idx << "\n";
		file >> d_AABB[idx];
		file >> d_tree[idx];
	}


}

SAHTree::SAHTree(Surfel *surfels, int numSurfels, int maxDepth, std::string &treename)
{
	float c_isec = 2.0f;
	float c_trav = 1.0f;

	//calculate containing bounds and then an AABB for each surfel
	AABB *surfBoxes = fitBoxesToSurfels(surfels, numSurfels);
	AABB *container = new AABB(surfBoxes[0]);
	for (int i = 0; i < numSurfels; i++)
	{
		container = container->join(surfBoxes[i]);
	}

	std::vector<int> &bucket = std::vector<int>(numSurfels);
	for (int i = 0; i < numSurfels; i++)
		bucket[i] = i;
	SAHGPUNode *nodes; //(SAHGPUNode **)malloc(sizeof(SAHNode *) * (pow(2, 16 + 1) - 1));
	cudaMallocManaged((void **)&nodes, sizeof(SAHGPUNode) * (pow(2, 20 + 1) - 1));
	for (int i = 0; i < (pow(2, 20 + 1) - 1); i++)
		nodes[i] = SAHGPUNode();
	
	//setup boxes
	cudaMallocManaged((void **)&d_AABB, sizeof(AABB) * (pow(2, 20 + 1) - 1));
	for (int i = 0; i < (pow(2, 20 + 1) - 1); i++)
		d_AABB[i] = new AABB();
	
	build_sahtree2(surfels, surfBoxes, numSurfels, container, 0, bucket, numSurfels, 0, nodes, d_AABB, 1);
	
	
	d_tree = nodes;
	std::cerr << "MAX DEPTH OF SAHTREE: " << MAX << "\n";
	std::cerr << "MAX SURFS IN A LEAF: " << maxSurfs << "\n";
	std::cerr << "NUM LEAVES: " << totalLeafs << "\n";
	std::cerr << "AVG SURFS PER LEAF: " << totalSurfs / totalLeafs << "\n";
	free(surfBoxes);
}

void SAHTree::save(std::string &treename)
{
	std::ofstream  treefile(treename.c_str(), std::ios::out | std::ios::binary);
	std::cerr << "Writing tree to: " << treename.c_str() << "\n";
	if (treefile.good())
	{
		//write depth
		treefile.write((char *)&MAX, sizeof(int));
		//write the gpunodes to file
		for (int i = 0; i < (pow(2, 20 + 1) - 1); i++)
		{
			if (d_tree[i].axis != -1) //if node exists
			{
				treefile.write((char *)&i, sizeof(int));
				treefile << d_AABB[i];
				treefile << d_tree[i];
			}
		}
	}
	treefile.close();
}

SAHNode *SAHTree::build_sahtree(Surfel *surfels, AABB *boxes, int numSurfels, AABB *container, int axis, const std::vector<int> &bucket, int depth)
{
	if (depth > MAX)
		MAX = depth;
	SAHNode *node = new SAHNode();
	//node->axis = axis; // axis;
	node->box = container;
	node->numSurfels = 0;
	node->surfels = NULL;

	if (bucket.size() <= MAXPRIMS || depth == 12) //create leaf
	{

		node->numSurfels = bucket.size();
		node->left = NULL;
		node->right = NULL;
		node->axis = -2;
		node->splitPos = -1.0f;
		if (node->numSurfels > 0)
		{
			cudaMallocManaged((void **)&node->surfels, sizeof(int) * node->numSurfels);
			cudaMemcpy((void *)node->surfels, (void *)bucket.data(), sizeof(int) * bucket.size(), cudaMemcpyHostToDevice);
		}

		if (node->numSurfels > maxSurfs)
			maxSurfs = node->numSurfels;
		totalSurfs += node->numSurfels;
		totalLeafs += 1;

		if (node->numSurfels == 0)
			std::cerr << "ZERO SURFS 1" << "\n";
	}
	else
	{
		//choose split dimension
		int splitAxis = container->maxExtent();
		node->axis = splitAxis;
		const int numBuckets = 12; //same as PBRT uses, dunno why
		AABB bucketBounds[numBuckets];
		int bucketPrimitives[numBuckets];
		float costs[numBuckets - 1]; //init all to 0.0f
		for (int i = 0; i < numBuckets; i++)
		{
			//bucketBounds[i] = AABB();
			bucketPrimitives[i] = 0;
			if (i < numBuckets - 1)
				costs[i] = 0.0f;
		}


		//bin the current surfels
		bool marked = false;
		for (int i = 0; i < bucket.size(); ++i) //iterate through the surfels in the bucket, place all boxes in their bins
		{
			int b = numBuckets * container->offset(boxes[bucket[i]].center(), splitAxis);
			if (b == numBuckets)
				b--;
			bucketPrimitives[b]++;
			bucketBounds[b] = *(bucketBounds[b].join(boxes[bucket[i]]));
		}
		//std::cerr << "Joined Buckets" << "\n";
		//calculate costs of each bin
		for (int i = 0; i < numBuckets - 1; ++i) {
			AABB b0, b1;
			int count0 = 0, count1 = 0;
			for (int j = 0; j <= i; ++j) {
				b0 = *(b0.join(bucketBounds[j]));
				count0 += bucketPrimitives[j];
			}
			for (int j = i + 1; j < numBuckets; ++j) {
				b1 = *(b1.join(bucketBounds[j]));
				count1 += bucketPrimitives[j];
			}
			costs[i] = .125f + (count0 * b0.surfaceArea() +
				count1 * b1.surfaceArea()) / container->surfaceArea();
		}

		//delete[] bucketBounds;
		//free(bucketBounds);
		//select the best bucket
		float minCost = FLT_MAX;
		int minCostSplitBucket = -1;
		for (int i = 0; i < numBuckets - 1; ++i) {
			if (costs[i] < minCost) {
				minCost = costs[i];
				minCostSplitBucket = i;
			}
		}

		float leafCost = bucket.size();
		if ((bucket.size() > MAXPRIMS || minCost < leafCost)) //split the node
		{
			AABB *leftBox = new AABB(container);
			AABB *rightBox = new AABB(container);
			//compute left and right box bounds by merging boxes on each side of the min cost box
			if (splitAxis == 0)
			{
				node->splitPos = container->min.x + minCostSplitBucket * (fabsf(container->max.x - container->min.x) / (float)numBuckets);
				leftBox->max.x = node->splitPos;
				rightBox->min.x = node->splitPos;
			}
			else if (splitAxis == 1)
			{
				node->splitPos = container->min.y + minCostSplitBucket * (fabsf(container->max.y - container->min.y) / (float)numBuckets);
				leftBox->max.y = node->splitPos;
				rightBox->min.y = node->splitPos;
			}
			else
			{
				node->splitPos = container->min.z + minCostSplitBucket * (fabsf(container->max.z - container->min.z) / (float)numBuckets);
				leftBox->max.z = node->splitPos;
				rightBox->min.z = node->splitPos;
			}

			std::vector<int> leftBucket;
			std::vector<int> rightBucket;
			int leftCt = 0;
			int rightCt = 0;

			for (int i = 0; i < bucket.size(); i++)
			{
				//left box checks
				if (leftBox->contains(surfels[bucket[i]]))
					leftBucket.push_back(bucket[i]);
				//right box checks
				if (rightBox->contains(surfels[bucket[i]]))
					rightBucket.push_back(bucket[i]);
			}
			leftCt = leftBucket.size();
			rightCt = rightBucket.size();


			node->left = build_sahtree(surfels, boxes, numSurfels, leftBox, splitAxis, leftBucket, depth + 1);
			//node->left = nodes[2 * idx];

			node->right = build_sahtree(surfels, boxes, numSurfels, rightBox, splitAxis, rightBucket, depth + 1);
			//node->right = nodes[2 * idx + 1];
		}
		else //create a leaf
		{
			node->numSurfels = bucket.size();
			node->axis = -2;
			node->splitPos = -1.0f;
			node->left = NULL;
			node->right = NULL;
			if (node->numSurfels > 0)
			{
				cudaMallocManaged((void **)&node->surfels, sizeof(int) * node->numSurfels);
				cudaMemcpy((void *)node->surfels, (void *)bucket.data(), sizeof(int) * bucket.size(), cudaMemcpyHostToDevice);
			}

			if (node->numSurfels > maxSurfs)
				maxSurfs = node->numSurfels;
			totalSurfs += node->numSurfels;
			totalLeafs += 1;

			if (node->numSurfels == 0)
				std::cerr << "ZERO SURFS 2" << "\n";
		}
	}

	return node;
}