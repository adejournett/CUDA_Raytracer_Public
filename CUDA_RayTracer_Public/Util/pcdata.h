#pragma once

#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include "light.h"
class SceneData
{
	public:
		SceneData() {};
		SceneData(const char *fname);

		std::vector<std::array<double, 3>> vPos;
		std::vector<float> normx;
		std::vector<float> normy;
		std::vector<float> normz;
		std::vector<std::array<unsigned char, 3>> vColor;
		std::vector<float> radii;
		
		std::vector<std::string> filenames;

		std::vector<int> fverts;
		std::vector<int> mIDs;
		std::vector<float> radius;
		std::vector<Light> lights;
		int numFiles, numVerts, numLights;
		int width, height;
		int depth;

		float3 camPos, lookat;
		bool dynamicCam, dynamicLight;
};

