#include "pcdata.h"
#include "happly.h"
#include<json.h>
#include<cuda_runtime.h>
#include "cuda_util.h"
#include<thrust/sort.h>
#include<thrust/execution_policy.h>
SceneData::SceneData(const char *fname)
{

	vPos = std::vector<std::array<double, 3>>();
	normx = std::vector<float>();
	normy = std::vector<float>();
	normz = std::vector<float>();
	vColor = std::vector<std::array<unsigned char, 3>>();
	filenames = std::vector<std::string>();

	std::ifstream jfile(fname, std::ios::in);
	Json::Reader reader;
	Json::Value obj;
	reader.parse(jfile, obj);

	width = obj["width"].asUInt();
	height = obj["height"].asUInt();
	depth = obj["depth"].asUInt();
	numVerts = 0;
	const Json::Value &lightV = obj["lights"];
	numLights = lightV.size();
	for (int i = 0; i < numLights; i++)
	{
		float3 lPos = make_float3(lightV[i]["pos"][0].asFloat(), lightV[i]["pos"][1].asFloat(), lightV[i]["pos"][2].asFloat());
		float3 lColor = make_float3(lightV[i]["color"][0].asFloat(), lightV[i]["color"][1].asFloat(), lightV[i]["color"][2].asFloat());
		bool type = lightV[i]["area"].asBool();
		float3 v1 = make_float3(0.0f, 0.0f, 0.0f);
		float3 v2 = make_float3(0.0f, 0.0f, 0.0f);

		if (type)
		{
			v1 = make_float3(lightV[i]["a"][0].asFloat(), lightV[i]["a"][1].asFloat(), lightV[i]["a"][1].asFloat());
			v2 = make_float3(lightV[i]["b"][0].asFloat(), lightV[i]["b"][1].asFloat(), lightV[i]["b"][1].asFloat());
		}
		lights.push_back(Light(lPos, lColor, v1, v2, type));
	}

	const Json::Value &fileV = obj["files"];
	numFiles = fileV.size();
	for (int i = 0; i < numFiles; i++)
	{
		std::cerr << "File: " << fileV[i]["name"].asString() << "\n";
		filenames.push_back(fileV[i]["name"].asString());
		//open file
		happly::PLYData data(fileV[i]["name"].asString());
		//get attribs
		std::vector<std::array<double, 3>> tPos = data.getVertexPositions();
		std::vector<std::array<unsigned char, 3>> tCol = data.getVertexColors();
		std::vector<float> tnx = data.getElement("vertex").getProperty<float>("nx");
		std::vector<float> tny = data.getElement("vertex").getProperty<float>("ny");
		std::vector<float> tnz = data.getElement("vertex").getProperty<float>("nz");

		if (data.getElement("vertex").hasProperty("radius"))
		{
			std::cerr << "radius detected" << "\n";
			radii = data.getElement("vertex").getProperty<float>("radius");
		}

		//append attribs
		vPos.insert(vPos.end(), tPos.begin(), tPos.end());
		normx.insert(normx.end(), tnx.begin(), tnx.end());
		normy.insert(normy.end(), tny.begin(), tny.end());
		normz.insert(normz.end(), tnz.begin(), tnz.end());
		vColor.insert(vColor.end(), tCol.begin(), tCol.end());
		//other constants
		fverts.push_back(data.getVertexPositions().size());
		numVerts += fverts[i];
		radius.push_back(fileV[i]["radius"].asFloat());
		//make material
		if (fileV[i]["material"]["type"].asString().compare("Lambertian") == 0)
			mIDs.push_back(0);
		else if (fileV[i]["material"]["type"].asString().compare("Cook_Torrance") == 0)
			mIDs.push_back(1);
		else if (fileV[i]["material"]["type"].asString().compare("BSSRDF_Approx") == 0)
			mIDs.push_back(2);
	}
	dynamicCam = obj["cam"]["dynamic"].asBool();
	camPos = make_float3(obj["cam"]["pos"][0].asFloat(), obj["cam"]["pos"][1].asFloat(), obj["cam"]["pos"][2].asFloat());
	lookat = make_float3(obj["cam"]["lookat"][0].asFloat(), obj["cam"]["lookat"][1].asFloat(), obj["cam"]["lookat"][2].asFloat());
	
	dynamicLight = numLights == 0 ? true : false;
}

