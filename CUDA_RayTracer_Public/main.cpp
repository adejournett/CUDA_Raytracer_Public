#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <assert.h>
#include <float.h>
#include <iostream>
#include <fstream>
#include <string>
#include<cuda_runtime.h>

#include"happly.h"
#include"scene.h"
#include"pcdata.h"
#include"sahtree.h"
#include"cook_torrance.h"
#include"lambertian.h"
#include"bssrdf_approx.h"

#include"SDLManager.h"

//helpers
bool SetGPU();
void processTree(SAHGPUNode *arr, AABB *boxes, int arr_size, int idx, int *ropes);

//kernel wrappers 
extern void launch_scene_kernel(Scene **d_world, SAHGPUNode *arr, AABB *boxes, Surfel *surfels, Rope *ropes, Light *light, int arr_size, SceneData data);
extern unsigned char *init_fb(int w, int h);
extern void print_gpuw(KDNode *d_tree, Surfel *surfels, AABB *boxes, int arr_size);


int main(int argc, char **argv) 
{
	//create a scenedata object from argv[1], which is presumably the JSON scene filename
	SceneData data = SceneData(argv[1]);
	int tx = 8;
	int ty = 8;
	dim3 blocks, threads;
	blocks = dim3(data.width / tx, data.height / ty); //partition the image dimensions into blocks for cuda to render 
	threads = dim3(tx, ty);

	//set the gpu to an NVIDIA device. If multiple devices are selected, this function selects the newest GPU
	SetGPU();

	std::cerr << "Rendering a " << data.width << "x" << data.height << " image with " << 1 << " samples/pixel ";
	std::cerr << "in " << tx << "x" << ty << " blocks.\n";

	int num_pixels = data.width * data.height;
	size_t fb_size = 3 * num_pixels * sizeof(unsigned char);

	// allocate FB in unified memory so both device and host can access
	unsigned char *fb;
	cudaMallocManaged((void **)&fb, fb_size);
	
	//allocate surfels in unified memory for the same reason
	//surfels will later be converted to device memory when the tree is built
	std::cerr << "Allocating " << sizeof(Surfel) * (data.numVerts) << " bytes of unified memory for surfels" << "\n";
	Surfel *surfels;
	cudaMallocManaged(&surfels, sizeof(Surfel) * data.numVerts);
	int f = 0;
	int mID;

	//build the surfels in parallel. this is a data independent, embarassingly parallel operation
#pragma omp parallel for
	for (int i = 0; i < data.numVerts; i++)
	{
		float3 pos = make_float3(float(data.vPos[i][0]), float(data.vPos[i][1]), float(data.vPos[i][2]));
		float3 norm = normalize(make_float3(data.normx[i], data.normy[i], data.normz[i]));//93 200
		float3 color = make_float3(float(data.vColor[i][0]) / 255.0f, float(data.vColor[i][1]) / 255.0f, float(data.vColor[i][2]) / 255.0f);
		mID = data.mIDs[f];
		Material material;
		if (mID == 0)
			material = Lambertian(color);
		else if (mID == 1)
			material = Cook_Torrance(color, 0.027f, 0.9f, 1.52f);
		else if (mID == 2)
			material = BSSRDF_Approx(color, 1.52f, 0.1f, 0.1f, 0.8f, 0.9f);
		surfels[i] = Surfel(pos, norm, data.radii[i], material, mID); //data.radius[f] for standard 
	}
	

	SAHTree *sahtree;
	SAHGPUNode *d_tree;
	AABB *d_AABB;
	int tree_size;
	int extIdx = data.filenames[0].find(".");
	std::string treename = data.filenames[0].substr(0, extIdx).append(".tree");
	std::cerr << "treename: " << treename << "\n";
	std::ifstream treefile(treename, std::ios::in | std::ios::binary);
	if (treefile.good()) //check if the tree has been created and cached
	{
		std::cerr << "Reading SAHTree from file..." << "\n";
		sahtree = new SAHTree(surfels, data.numVerts, treefile);
		treefile.close();
		tree_size = sahtree->size();
		d_tree = sahtree->d_tree;
		d_AABB = sahtree->d_AABB;
	}
	else //create a tree file from scratch using the data in the associated PLY file
	{
		treefile.close();
		clock_t start, stop;
		start = clock();
		std::cerr << "Constructing SAHTree..." << "\n";
		sahtree = new SAHTree(surfels, data.numVerts, data.depth, treename);
		stop = clock();
		double timer_seconds = ((double)(stop - start)) / CLOCKS_PER_SEC;
		std::cerr << "Built SAHTree: " << timer_seconds << "s\n";
		tree_size = sahtree->size();
		d_tree = sahtree->d_tree;
		d_AABB = sahtree->d_AABB;

		int *ropes;
		cudaMallocManaged(&ropes, sizeof(int) * 6);
		for (int z = 0; z < 6; z++)
			ropes[z] = -1;
		std::cerr << "Building Ropes and AABB bounds..." << "\n";
		processTree(d_tree, d_AABB, tree_size + 1, 1, ropes); //build tree ropes
		cudaDeviceSynchronize();
		std::cerr << "Tree Complete!" << "\n";

		std::cerr << "Writing tree to file..." << "\n";
		sahtree->save(treename);
	}
	/*
		List the number of nodes in the tree and the dimensions of the scene container AABB
		this is useful for profiling and debugging purposes
	*/
	std::cerr << "Num SAH nodes: " << tree_size << "\n";
	std::cerr << "X:" << d_AABB[1].min.x << "~" << d_AABB[1].max.x << "\n";
	std::cerr << "Y:" << d_AABB[1].min.y << "~" << d_AABB[1].max.y << "\n";
	std::cerr << "Z:" << d_AABB[1].min.z << "~" << d_AABB[1].max.z << "\n";
	std::cout << "X:" << d_AABB[1].min.x << "~" << d_AABB[1].max.x << "\n";
	std::cout << "Y:" << d_AABB[1].min.y << "~" << d_AABB[1].max.y << "\n";
	std::cout << "Z:" << d_AABB[1].min.z << "~" << d_AABB[1].max.z << "\n";
	
	
	Rope *r;
	const float SCALE = length(d_AABB[1].min - d_AABB[1].max);
	std::cerr << "Scale: " << SCALE << "\n";
	
	//create world

	//copy the surfels from unified memory to device memory, this improves performance
	Surfel *d_surfels;
	cudaMalloc((void **) &d_surfels, sizeof(Surfel) * data.numVerts);
	cudaMemcpy(d_surfels, surfels, sizeof(Surfel) * data.numVerts, cudaMemcpyDefault);
	
	//create a scene pointer on device memory
	Scene **d_world;
	cudaMalloc((void **)&d_world, sizeof(Scene));
	
	//create a light array on device memory
	Light *d_light;
	cudaMallocManaged((void **)&d_light, sizeof(Light));
	cudaDeviceSynchronize(); //barrier sync after mallocs
	
	//create the Scene via a kernel
	launch_scene_kernel(d_world, d_tree, d_AABB, surfels, r, d_light, tree_size + 1, data);
	cudaDeviceSynchronize();
	
	//begin rendering
	UIData *uidata = new UIData(data.filenames, surfels, d_light, d_AABB, data.numFiles, data.fverts, data.mIDs);
	SDLManager manager = SDLManager(d_world, uidata, fb, SCALE, data.width, data.height, tx, ty);
	manager.beginRender(d_world);

	//cleanup
	cudaDeviceReset(); //reset GPU so launching program twice in a row doesn't leave the GPU in a bad state or cause crashes
	return 0;
}

/*!
*	Choose a CUDA enabled GPU to utilize.
*	Looks for a desired device GeForce GTX 1050 TI, the card I hard installed on my machine
*	Selects any other CUDA recognized gpu if desired device is not found
*/
bool SetGPU()
{
	int devicesCount;
	cudaGetDeviceCount(&devicesCount);
	std::string desiredDeviceName = "GeForce GTX 1050 Ti";
	for (int deviceIndex = 0; deviceIndex < devicesCount; ++deviceIndex)
	{
		cudaDeviceProp deviceProperties;
		cudaGetDeviceProperties(&deviceProperties, deviceIndex);
		std::cerr << "Name: " << deviceProperties.name << "\n";
		if (deviceProperties.name == desiredDeviceName)
		{
			cudaSetDevice(deviceIndex);
			return true;
		}
	}

	return false;
}

/*!
*	This function creates the ropes defined in \cite{Stackless KD-Tree Traversal for High Performance GPU Ray-tracing}
*	The ropes are then optimized to allow for the minimum number of node checks per ray. 
*	On average, this causes a ray to intersect a maximum of 50 surfels for any scene depth, so long as the tree is build sufficiently deep
*
*	
*	\param arr The flattened SAHTree in device memory
*	\param boxes The AABB array corresponding to the flattened tree
*	\param arr_size size of \param{arr}, equivalent to the number of nodes in a full, complete tree of depth d + 1
*	\param idx internal recursion parameter. Function should be called with idx = 1
*	\param ropes pointer to the ropes for a given node. There are 6 ropes per node
*/
void processTree(SAHGPUNode *arr, AABB *boxes, int arr_size, int idx, int *ropes)
{
	if (idx >= arr_size || arr[idx].axis == -1) // outside tree
	{
		return;
	}
	AABB box = boxes[idx];
	SAHGPUNode *node = &arr[idx];
	if (node->numSurfels >= 0 && node->axis == -2) // node is leaf
	{
		//optimize here
		for (int i = 0; i < 6; i++)
		{
			if (ropes[i] > 0)
			{
				int temp = ropes[i];
				SAHGPUNode n = arr[ropes[i]];
				AABB nbox = boxes[ropes[i]];
				while (n.numSurfels == 0 && ropes[i] != temp) //while node not leaf, continue pushing ropes down to a leaf node
				{
					if (n.axis == 0) //x-axis case
					{
						if (n.splitPos <= box.min.x) //reassign to left node if split pos < box.min.x
							ropes[i] = 2 * ropes[i] + 1;
						else if (n.splitPos >= box.max.x) //else reassign to right node
							ropes[i] = 2 * ropes[i];
						else //leaf node reached
							break;
					}
					else if (n.axis == 1) //y-axis case
					{
						if (n.splitPos <= box.min.y)
							ropes[i] = 2 * ropes[i] + 1;
						else if (n.splitPos >= box.max.y)
							ropes[i] = 2 * ropes[i];
						else
							break;
					}
					else if (n.axis == 2)
					{
						if (n.splitPos <= box.min.z)
							ropes[i] = 2 * ropes[i] + 1;
						else if (n.splitPos >= box.max.z)
							ropes[i] = 2 * ropes[i];
						else
							break;
					}
					n = arr[ropes[i]];
					nbox = boxes[ropes[i]];
				}
			}
		}

		cudaMemcpy((void *)node->ropes, (void *)ropes, sizeof(int) * 6, cudaMemcpyHostToDevice);
	}
	else
	{

		int Sl, Sr; //left and right side rope indices
		switch (node->axis)
		{
		case 0: //x-axis partition
			Sl = 0; //left and right side indicies
			Sr = 3;
			break;
		case 1: //y-axis partition
			Sl = 1; //top and bottom side indicies
			Sr = 4;
			break;
		case 2://z-axis partition
			Sl = 2; //front and back side indices
			Sr = 5;
			break;
		default: //axis undefined, fallthrough to error
			std::cerr << "ERROR" << "idx: " << idx << "surf: " << node->numSurfels << "axis: " << node->axis << "\n";
			break;
		}
		//initialize left ropes
		int left_ropes[6];
		for (int i = 0; i < 6; i++)
			left_ropes[i] = ropes[i];
		left_ropes[Sr] = 2 * idx + 1; //assign left box's right node to the current right node. may be pushed down later in base case
		processTree(arr, boxes, arr_size, 2 * idx, left_ropes);

		//initialize right ropes
		int right_ropes[6];
		for (int i = 0; i < 6; i++)
			right_ropes[i] = ropes[i];
		right_ropes[Sl] = 2 * idx; //assign right box's left node to the current left node. may be pushed down later in base case
		processTree(arr, boxes, arr_size, 2 * idx + 1, right_ropes);
	}
}
