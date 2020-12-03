#pragma once

class UIData
{
public:
	UIData() {};
	/*!
		Primary constructor for the UIData class

		\param fVec vector of strings, where each string contains the filename of the program
		\param h_surfels host array of Surfels. These must be in unified or host memory to support interactive material modifications
		\param h_lights host array of lights. These must be in unified or host memory to support interactive lighting modification
		\param h_boxes host array of AABBs for the scene. Stored on host so they can be drawn in debug mode to visualize the kd tree
		\param nf number of files
		\param f vector containing the number of vertices in the given file
		\param m vector containing the materialID of the model on a per-file basis
	*/
	UIData(const std::vector<std::string> &fVec, Surfel *h_surfels, Light *h_lights, AABB *h_boxes, int nf, const std::vector<int> &f, const std::vector<int> &m)
	{
		//initialize fields
		boxes = h_boxes;
		surfels = h_surfels;
		lights = h_lights;
		numFiles = nf;
		fVerts = f;
		mIDs = m;
		fIdx = 0;
		lIdx = -1;
		tIdx = surfels[0].mat;

		int numChars = 0;
		for (int i = 0; i < numFiles; i++)
		{
			numChars += fVec[i].size();
		}

		//modify filenames for display in ImGUI 
		int idx = 0;
		for (int i = 0; i < numFiles; i++)
		{
			const char *str = fVec[i].c_str();
			int j = 0;
			int k = 0;
			for (j = idx; j < idx + fVec[i].size(); ++j)
			{
				files[j] = fVec[i][k];
				k++;
			}
			files[j] = '\0';
			idx = j + 1;
		}
		//add null terminators
		for (int i = idx; i < 100; i++)
			files[i] = '\0';
	}

	/*!
		Update operation for material appearance. 
		uses params array to replace material params as the UI fields change

		\params params the float array of materials params. differs based on materialID 
	*/
	void updateSurfels(float *params)
	{
		int fID = fIdx;
		int mType = tIdx;
		int start = 0;
		for (int i = 0; i < numFiles; i++)
		{
			if (i == fID)
				break;
			start += fVerts[i];
		}
		int end = 0;
		for (int i = 0; i < numFiles; i++)
		{
			end += fVerts[i];
			if (i == fID)
				break;
		}

		//material update loop
		for (int i = start; i < end; i++)
		{
			surfels[i].mat = mType;
		}
	}

	/*!
		update lighting operation. Changes Scene lights to the properties given as parameters

		\param pos light position, length 3
		\param v1 basis vector one for area lighting, length 3
		\param v2 basis vector two for area lighting, length 3
		\param col rgb color, length 3
		\param area signifies area/point lighting
	
	*/
	void updateLights(float *pos, float *v1, float *v2, float *col, bool area)
	{
		float3 p = make_float3(pos[0], pos[1], pos[2]);
		float3 a = make_float3(v1[0], v1[1], v1[2]);
		float3 b = make_float3(v2[0], v2[1], v2[2]);
		float3 color = make_float3(col[0], col[1], col[2]);
		lights[lIdx] = Light(p, color, a, b, area);
	}

	char files[100];
	Surfel *surfels;
	Light *lights;
	AABB *boxes;
	int fIdx;
	int tIdx;
	int lIdx;
	std::vector<int> fVerts;
	std::vector<int> mIDs;
	int numFiles;
	int totalVerts;

};