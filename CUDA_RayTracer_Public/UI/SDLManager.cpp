
#include<cuda_runtime.h>
// Utilities and timing functio

#include<iostream>
#include<SDL_ttf.h>
//#include<AntTweakBar.h>

#include"SDLManager.h"
#include"scene.h"
#include "image_util.h"

//globals
clock_t start, stop;
int rays = 0;
double timer_seconds;

//kernel wrappers
extern int  cudaRedraw(Scene **d_world, unsigned char *data, int w, int h, int x, int y);
extern void cudaChangeCam(Scene **world, const float3 &eyetrans, const float3 &lookattrans);
extern void cudaChangeCamMouse(Scene **world, float angle, const float3 &axis);
extern void cudaCastSingleRay(Scene **world, int x, int y);

void mouseMotion(int x, int y, int width, int height, bool trackingMouse, float3 &lastPos, float &angle, float3 &axis);
void renderTexture(SDL_Texture *tex, SDL_Renderer *ren, int x, int y, SDL_Texture **mssgs, int mssgCt);
void logSDLError(std::ostream &os, const std::string &msg);

SDLManager::SDLManager(Scene **d_world, UIData *ui, unsigned char *buffer, float s, int w, int h, int x, int y)
{
	guiWidth = 215;
	SCALE = s;
	//data = d;
	WIDTH = w;
	HEIGHT = h;
	tx = x;
	ty = y;
	blocks = dim3(WIDTH / x, HEIGHT / y);
	threads = dim3(x, y);
	fb = buffer;
	//Variables used in the rendering loop
	quit = false;
	leftMouseButtonDown = false;
	angle = 0.0f;
	//float3 axis, trans;
	trackingMouse = false;
	redrawContinue = true;
	trackballMove = false;
	lastPos = make_float3(0.0f, 0.0f, 0.0f);
	uidata = ui;


	// initiate SDL
	if (SDL_Init(SDL_INIT_VIDEO) != 0)
	{
		printf("[ERROR] %s\n", SDL_GetError());
		SDL_Quit();
	}

	// set OpenGL attributes
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

	SDL_GL_SetAttribute(
		SDL_GL_CONTEXT_PROFILE_MASK,
		SDL_GL_CONTEXT_PROFILE_CORE
	);

	std::string glsl_version = "";
#ifdef __APPLE__
	// GL 3.2 Core + GLSL 150
	glsl_version = "#version 150";
	SDL_GL_SetAttribute( // required on Mac OS
		SDL_GL_CONTEXT_FLAGS,
		SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG
	);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
#elif __linux__
	// GL 3.2 Core + GLSL 150
	glsl_version = "#version 150";
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
#elif _WIN32
	// GL 3.0 + GLSL 130
	glsl_version = "#version 130";
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#endif

	SDL_WindowFlags window_flags = (SDL_WindowFlags)(
		SDL_WINDOW_OPENGL
		| SDL_WINDOW_SHOWN
		| SDL_WINDOW_ALLOW_HIGHDPI
		);
	window = SDL_CreateWindow(
		"CUDA BSSRDF Raytracer",
		SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED,
		WIDTH + guiWidth,
		HEIGHT,
		window_flags
	);

	glContext = SDL_GL_CreateContext(window);
	SDL_GL_MakeCurrent(window, glContext);

	// enable VSync
	SDL_GL_SetSwapInterval(1);

	if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
	{
		std::cerr << "[ERROR] Couldn't initialize glad" << std::endl;
	}
	else
	{
		std::cerr << "[INFO] glad initialized\n";
	}

	glViewport(0, 0, WIDTH, HEIGHT);

	// setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	

	// setup Dear ImGui style
	ImGui::StyleColorsDark();

	// setup platform/renderer bindings
	ImGui_ImplSDL2_InitForOpenGL(window, glContext);
	ImGui_ImplOpenGL3_Init(glsl_version.c_str());
}

SDLManager::~SDLManager()
{
	cudaFree(fb);
	//cudaFree(data);
	SDL_DestroyTexture(background);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}

void SDLManager::renderUI(Scene **d_world, GLuint image_texture, int &fIdx, int &tIdx, float &rough, float &ks, float &index, float *omega, float &mcos, int &lIdx, float *lightPos, float *lightColor, float *v1, float *v2, bool lArea)
{
	
	// start the Dear ImGui frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL2_NewFrame(window);
	ImGui::NewFrame();

	// a window is defined by Begin/End pair
	{
		static int counter = 0;
		// position the controls widget in the top-right corner with some margin
		ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
		// here we set the calculated width and also make the height to be
		// be the height of the main window also with some margin
		ImGui::SetNextWindowSize(
			ImVec2(static_cast<float>(guiWidth), static_cast<float>(HEIGHT - 20)),
			ImGuiCond_Always
		);
		// create a window and append into it
		ImGui::Begin("Controls", 0, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

		ImGui::Dummy(ImVec2(0.0f, 1.0f));
		if (ImGui::CollapsingHeader("System Properties"))
		{
			ImGui::Text("%s", SDL_GetPlatform());
			ImGui::Text("CPU cores: %d", SDL_GetCPUCount());
			ImGui::Text("RAM: %.2f GB", SDL_GetSystemRAM() / 1024.0f);
		}

		if (ImGui::CollapsingHeader("Material Properties"))
		{
			int vert = 0; //first vert index in given file
			ImGui::Combo("File", &fIdx, uidata->files, 3);
			if (fIdx != uidata->fIdx)
			{
				uidata->fIdx = fIdx;
				tIdx = uidata->mIDs[uidata->fIdx];
				uidata->tIdx = tIdx;

				for (int i = 0; i < fIdx; i++)
					vert += uidata->fVerts[i];

				rough = uidata->surfels[vert].material.roughness;
				ks = uidata->surfels[vert].material.ks;
				index = uidata->surfels[vert].material.index;
				omega[0] = uidata->surfels[vert].material.extinction;
				omega[1] = uidata->surfels[vert].material.absorbtion;
				omega[2] = uidata->surfels[vert].material.scattering;
				mcos = uidata->surfels[vert].material.mean_cos;
			}

			for (int i = 0; i < fIdx; i++)
				vert += uidata->fVerts[i];


			ImGui::Combo("Type", &tIdx, "Lambertian\0Cook Torrance\0BSSRDF Approx\0BSSRDF Dipole\0Microfacet Single\0Microfacet Multiple\0", 6);
			if (tIdx != uidata->tIdx)
			{
				uidata->tIdx = tIdx;
				redrawContinue = true;

				rough = 0.0f;
				ks = 1.0f;
				index = 1.0f;
				omega[0] = 0.1f;
				omega[1] = 0.1f;
				omega[2] = 0.8f;
				mcos = 0.9f;
			}

			float params[5];
			if (tIdx == 1) //cook torrance
			{
				params[0] = uidata->surfels[vert].material.roughness;
				params[1] = uidata->surfels[vert].material.ks;
				params[2] = uidata->surfels[vert].material.index;

				if (!redrawContinue && (rough != params[0] || ks != params[1] || index != params[2]))
					redrawContinue = true;

				params[0] = rough;
				params[1] = ks;
				params[2] = index;

				ImGui::SliderFloat("Roughness", &rough, 0.0f, 1.0f, "%.3f");
				ImGui::SliderFloat("Specular", &ks, 0.0f, 1.0f, "%.3f");
				ImGui::SliderFloat("Index", &index, 1.0f, 4.0f, "%.3f");


			}
			else if (tIdx == 2) //dssrdf approx
			{
				//std::cerr << vert << "\n";
				params[0] = uidata->surfels[vert].material.index;
				params[1] = uidata->surfels[vert].material.extinction;
				params[2] = uidata->surfels[vert].material.absorbtion;
				params[3] = uidata->surfels[vert].material.scattering;
				params[4] = uidata->surfels[vert].material.mean_cos;

				if (!redrawContinue && (index != params[0] || omega[0] != params[1] || omega[1] != params[2] || omega[2] != params[3] || mcos != params[4]))
					redrawContinue = true;

				params[0] = index;
				params[1] = omega[0];
				params[2] = omega[1];
				params[3] = omega[2];
				params[4] = mcos;


				ImGui::SliderFloat("Index", &index, 1.0f, 4.0f, "%.3f");
				ImGui::SliderFloat3("Coeffs", omega, 0.0f, 1.0f, "%.3f");
				ImGui::SliderFloat("Mean Cos", &mcos, 0.0f, 1.0f, "%.3f");

			}
			else if (tIdx == 3) //BSSRDF dipole
			{
				params[0] = uidata->surfels[vert].material.index;
				params[1] = uidata->surfels[vert].material.extinction;
				params[2] = uidata->surfels[vert].material.absorbtion;
				params[3] = uidata->surfels[vert].material.scattering;
				params[4] = uidata->surfels[vert].material.mean_cos;

				if (!redrawContinue && (index != params[0] || omega[0] != params[1] || omega[1] != params[2] || omega[2] != params[3] || mcos != params[4]))
					redrawContinue = true;

				params[0] = index;
				params[1] = omega[0];
				params[2] = omega[1];
				params[3] = omega[2];
				params[4] = mcos;


				ImGui::SliderFloat("Index", &index, 1.0f, 4.0f, "%.3f");
				ImGui::SliderFloat3("Coeffs", omega, 0.0f, 1.0f, "%.3f");
				ImGui::SliderFloat("Mean Cos", &mcos, 0.0f, 1.0f, "%.3f");
			}
			else if (tIdx == 4)
			{
				float ax = 0.0f;
				ImGui::SliderFloat("Alpha x", &ax, 0.0f, 1.0f, "%.3f");

				float ay = 0.0f;
				ImGui::SliderFloat("Alpha y", &ay, 0.0f, 1.0f, "%.3f");
			}
			else if (tIdx == 5)
			{
				float ax = 0.0f;
				ImGui::SliderFloat("Alpha x", &ax, 0.0f, 1.0f, "%.3f");

				float ay = 0.0f;
				ImGui::SliderFloat("Alpha y", &ay, 0.0f, 1.0f, "%.3f");
			}

			if (redrawContinue)
				uidata->updateSurfels(params);
		}

		if (ImGui::CollapsingHeader("Performance Metrics"))
		{
			ImGui::TextColored(ImVec4(0.0f, 0.0f, 1.0f, 1.0f), "FPS: %.2f", 1.0f / timer_seconds);
			ImGui::TextColored(ImVec4(0.0f, 0.0f, 1.0f, 1.0f), "MRays/Sec: %.2f", rays * (1.0 / timer_seconds) / 1000000);
		}

		if (ImGui::CollapsingHeader("Scene Properties"))
		{
			ImGui::Combo("Light", &lIdx, "Light 1\0", 1);
			if (lIdx != uidata->lIdx)
			{
				uidata->lIdx = lIdx;

				lArea = uidata->lights[lIdx].isArea;
				v1[0] = uidata->lights[lIdx].a.x;
				v1[1] = uidata->lights[lIdx].a.y;
				v1[2] = uidata->lights[lIdx].a.z;
				v2[0] = uidata->lights[lIdx].b.x;
				v2[1] = uidata->lights[lIdx].b.y;
				v2[2] = uidata->lights[lIdx].b.z;
				lightPos[0] = uidata->lights[lIdx].position.x;
				lightPos[1] = uidata->lights[lIdx].position.y;
				lightPos[2] = uidata->lights[lIdx].position.z;
				lightColor[0] = uidata->lights[lIdx].color.x;
				lightColor[1] = uidata->lights[lIdx].color.y;
				lightColor[2] = uidata->lights[lIdx].color.z;

			}
			float params[13];
			params[0] = uidata->lights[lIdx].isArea;
			params[1] = uidata->lights[lIdx].a.x;
			params[2] = uidata->lights[lIdx].a.y;
			params[3] = uidata->lights[lIdx].a.z;
			params[4] = uidata->lights[lIdx].b.x;
			params[5] = uidata->lights[lIdx].b.y;
			params[6] = uidata->lights[lIdx].b.z;
			params[7] = uidata->lights[lIdx].position.x;
			params[8] = uidata->lights[lIdx].position.y;
			params[9] = uidata->lights[lIdx].position.z;
			params[10] = uidata->lights[lIdx].color.x;
			params[11] = uidata->lights[lIdx].color.y;
			params[12] = uidata->lights[lIdx].color.z;

			if (!redrawContinue && (params[0] != lArea || params[1] != v1[0] || params[2] != v1[1] || params[3] != v1[2] || params[4] != v2[0] || params[5] != v2[1] || params[6] != v2[2] || params[7] != lightPos[0] ||
				params[8] != lightPos[1] || params[9] != lightPos[2] || params[10] != lightColor[0] || params[11] != lightColor[1] || params[12] != lightColor[2]))
			{
				redrawContinue = true;
			}

			params[0] = lArea;
			params[1] = v1[0];
			params[2] = v1[1];
			params[3] = v1[2];
			params[4] = v2[0];
			params[5] = v2[1];
			params[6] = v2[2];
			params[7] = lightPos[0];
			params[8] = lightPos[1];
			params[9] = lightPos[2];
			params[10] = lightColor[0];
			params[11] = lightColor[1];
			params[12] = lightColor[2];



			ImGui::Checkbox("Area", &lArea);
			if (lArea)
			{
				ImGui::SliderFloat3("a", v1, 0.0f, 20.0f, "%.3f");
				ImGui::SliderFloat3("b", v2, 0.0f, 20.0f, "%.3f");

			}
			ImGui::SliderFloat3("Pos", lightPos, -20.0f, 20.0f, "%.3f");
			ImGui::SliderFloat3("Color", lightColor, 0.0f, 1.0f, "%.3f");

			if (redrawContinue)
				uidata->updateLights(lightPos, v1, v2, lightColor, lArea);
		}


		if (redrawContinue)
		{
			renderFrame(d_world);
			glEnable(GL_TEXTURE_2D);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, WIDTH, HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, fb);
			glDisable(GL_TEXTURE_2D);
		}
		
		

		ImDrawList *list = ImGui::GetBackgroundDrawList();
		list->AddImage((void*)(intptr_t)image_texture, ImVec2(guiWidth, 0), ImVec2(WIDTH + guiWidth, HEIGHT));


		ImGui::End();
	}
}

/*!
	timing wrapper to record frameTime (the time to render an image and swap buffers)
	calls the cudaRedraw kernel to generate a single frame

	\param d_world the scene to be rendered
*/
void SDLManager::renderFrame(Scene **d_world)
{
	start = clock();
	rays = cudaRedraw(d_world, fb, WIDTH, HEIGHT, tx, ty); //draw new frame of scene

	stop = clock();
	timer_seconds = ((double)(stop - start)) / CLOCKS_PER_SEC;

	redrawContinue = false;
}

/*!
	Render loop for SDL. Sets up the GL image to display on the ImGUI context, then begins the renderFrame CUDA calls

	\param d_world device copy of the scene to be rendered
*/
void SDLManager::beginRender(Scene **d_world)
{
	// colors are set in RGBA, but as float
	ImVec4 bkg = ImVec4(35 / 255.0f, 35 / 255.0f, 35 / 255.0f, 1.00f);

	glClearColor(bkg.x, bkg.y, bkg.z, bkg.w);

	// Create a OpenGL texture identifier
	GLuint image_texture;
	glGenTextures(1, &image_texture);
	glBindTexture(GL_TEXTURE_2D, image_texture);

	// Setup filtering parameters for display
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Upload pixels into texture
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	

	//material props
	int fIdx = 0;
	int tIdx = 0;
	float rough = 0.0f;
	float ks = 0.0f;
	float index = 1.0f;

	float omega[3];
	omega[0] = 0.1f;
	omega[1] = 0.1f;
	omega[2] = 0.8f;
	float mcos = 0.9f;

	//scene props
	int lIdx = 0;
	float lightPos[3];
	float lightColor[3];
	float v1[3];
	float v2[3];
	bool lArea = false;

	bool loop = true;
	while (loop)
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			// without it you won't have keyboard input and other things
			ImGui_ImplSDL2_ProcessEvent(&event);
			// you might also want to check io.WantCaptureMouse and io.WantCaptureKeyboard
			// before processing events
			int x, y;
			switch (event.type)
			{
			case SDL_QUIT:
				loop = false;
				break;
			case SDL_MOUSEBUTTONUP:
				x = event.motion.x;
				y = event.motion.y;
				if (event.button.button == SDL_BUTTON_LEFT)
				{
					leftMouseButtonDown = false;
					stopMotion(x, y);
				}
				else if (event.button.button == SDL_BUTTON_RIGHT && x >= guiWidth)
				{
					std::cerr << "Right click: (" << x - guiWidth << ", " << y << ")" << "\n";
					cudaCastSingleRay(d_world, x - guiWidth, y);
				}

				break;
			case SDL_MOUSEBUTTONDOWN:
				x = event.motion.x;
				y = event.motion.y;
				std::cerr << "(" << x << ", " << y << ")" << "\n";
				if (event.button.button == SDL_BUTTON_LEFT && x >= guiWidth)
				{
					leftMouseButtonDown = true;
					y = HEIGHT - y;
					startMotion(x, y);
				}
				break;
			case SDL_MOUSEMOTION:
				x = event.motion.x;
				y = event.motion.y;
				if (leftMouseButtonDown)
				{
					mouseMotion(x, y);
					if (trackballMove)
					{
						cudaChangeCamMouse(d_world, angle, normalize(axis));
						redrawContinue = true;
					}
				}
				break;
			case SDL_KEYDOWN:
				handleKeyPress(event, d_world);
				break;
			}
		}

		renderUI(d_world, image_texture, fIdx, tIdx, rough, ks, index, omega, mcos, lIdx, lightPos, lightColor, v1, v2, lArea);

		// rendering
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		//SDL_RenderPresent(renderer);
		SDL_GL_SwapWindow(window);
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	SDL_GL_DeleteContext(glContext);
	SDL_DestroyWindow(window);
	SDL_Quit();
}

/*!
	event polling loop for SDL. Scans for an event, then calls the correct dispatch function for the event type. 

	\param event an event storage state
	\param d_world device copy of the scene, as some operations directly modify the world parameters (camera) 
*/
void SDLManager::processEvent(SDL_Event event, Scene **d_world)
{

	while (SDL_PollEvent(&event))
	{
		if (event.type == SDL_QUIT)
		{
			quit = true;
		}

		//Use number input to select which clip should be drawn
		if (event.type == SDL_KEYDOWN)
		{
			handleKeyPress(event, d_world);
		}
		else if (event.type == SDL_MOUSEBUTTONUP)
		{
			int x = event.motion.x;
			int y = event.motion.y;
			if (event.button.button == SDL_BUTTON_LEFT)
			{
				leftMouseButtonDown = false;
				stopMotion(x, y);
			}

		}
		else if (event.type == SDL_MOUSEBUTTONDOWN)
		{
			int x = event.motion.x;
			int y = event.motion.y;
			if (event.button.button == SDL_BUTTON_LEFT)
			{
				leftMouseButtonDown = true;
				y = HEIGHT - y;
				startMotion(x, y);
			}
		}
		else if (event.type == SDL_MOUSEMOTION)
		{
			int x = event.motion.x;
			int y = event.motion.y;
			if (leftMouseButtonDown)
			{
				mouseMotion(x, y);
				if (trackballMove)
				{
					cudaChangeCamMouse(d_world, angle, normalize(axis));
					redrawContinue = true;
				}
			}
		}
	}
}

/*!
	PtoV opteration as given in Trackball Camra guide

	\param x current x position
	\param y current y position
	\param width screen width
	\param height screen height
	\param v return vector
*/
void trackball_ptov(int x, int y, int width, int height, float3 &v)
{
	float d, a;
	/* project x,y onto a hemisphere centered within width, height ,
   note z is up here*/
	v.x = (2.0*x - width) / width;
	v.y = (height - 2.0F*y) / height;
	d = sqrt(v.x * v.x + v.y * v.y);
	v.z = cos((M_PI / 2.0) * ((d < 1.0) ? d : 1.0));
	a = 1.0 / length(v);
	v *= a;
}

/*!
	Axis computation for moust motion when MOUSE_DRAG event is detected
	Computes the axis based on the initially saved (x,y) and the current (x,y) as params

	\param x the current screen-space x pos
	\param y the current screen-space y pos
*/
void SDLManager::mouseMotion(int x, int y)
{
	float3 curPos;
	float3 d;
	/* compute position on hemisphere */
	trackball_ptov(x, y, WIDTH, HEIGHT, curPos);
	//std::cerr << "Cur: " << curPos.x << ", " << curPos.y << ", " << curPos.z << "\n";
	if (trackingMouse)
	{
		/* compute the change in position
		on the hemisphere */
		d = curPos - lastPos;
		if (d.x || d.y || d.z)
		{
			/* compute theta and cross product */
			angle =  90.0f * length(d) * M_PI / 180.0f; //in radians, but why this formula
			axis.x = (lastPos.y * curPos.z) - (lastPos.z * curPos.y);
			axis.y = (lastPos.z * curPos.x) - (lastPos.x * curPos.z);
			axis.z = (lastPos.x * curPos.y) - (lastPos.y * curPos.x);
			axis.x = (axis.x < 0.0f) ? -axis.x : axis.x;
			axis.y = (axis.y < 0.0f) ? -axis.y : axis.y;
			axis.z = (axis.z < 0.0f) ? -axis.z : axis.z;
			/* update position */
			lastPos = curPos;
		}
	}
}

/*!
	Saves the necessary state when a MOUSE_DRAG event is detected

	\param x the initial screen-space x position
	\param y then initial screen-space y position
*/
void SDLManager::startMotion(int x, int y)
{
	trackingMouse = true;
	redrawContinue = false;
	startX = x;
	startY = y;
	curx = x;
	cury = y;
	trackball_ptov(x, y, WIDTH, HEIGHT, lastPos);
	trackballMove = true;
}

/*!
	stopping state for mouse motion

	\param x the screen-space x position
	\param y the screen-space y position
*/
void SDLManager::stopMotion(int x, int y)
{
	trackingMouse = false;
	/* check if position has changed */
	if (startX != x || startY != y)
		redrawContinue = true;
	else
	{
		angle = 0.0;
		redrawContinue = false;
		trackballMove = false;
	}
}

/*!
	Function to handle key press. calls scene modification kernels as necessary to translate the camera in WASD directions

	\param event the presumably SDL_KEYPRESS event
	\param d_world the device-only copy of the Scene
*/
void SDLManager::handleKeyPress(SDL_Event event, Scene **d_world)
{
	switch (event.key.keysym.sym)
	{
	case SDLK_ESCAPE:
		quit = true;
		break;
	case SDLK_w:
		cudaChangeCam(d_world, make_float3(0.0f, 0.0f, -0.01f * SCALE), make_float3(0.0f, 0.0f, 0.0f));
		redrawContinue = true;
		break;
	case SDLK_s:
		cudaChangeCam(d_world, make_float3(0.0f, 0.0f, 0.01f * SCALE), make_float3(0.0f, 0.0f, 0.0f));
		redrawContinue = true;
		break;
	case SDLK_a:
		cudaChangeCam(d_world, make_float3(-0.01f * SCALE, 0.0f, 0.0f), make_float3(-0.01f * SCALE, 0.0f, 0.0f));
		redrawContinue = true;
		break;
	case SDLK_d:
		cudaChangeCam(d_world, make_float3(0.01f * SCALE, 0.0f, 0.0f), make_float3(0.01f * SCALE, 0.0f, 0.0f));
		redrawContinue = true;
		break;
	case SDLK_UP:
		cudaChangeCam(d_world, make_float3(0.0f, 0.01f * SCALE, 0.0f), make_float3(0.0f, 0.01f * SCALE, 0.0f));
		cudaDeviceSynchronize();
		redrawContinue = true;
		break;
	case SDLK_DOWN:
		cudaChangeCam(d_world, make_float3(0.0f, -0.01f * SCALE, 0.0f), make_float3(0.0f, -0.01f * SCALE, 0.0f));
		redrawContinue = true;
		break;
	default:
		break;
	}
}

/*
	Log an SDL error with some error message to the output stream of our
	choice

	\param os The output stream to write the message to
	\param msg The error message to write, SDL_GetError() appended to it
*/
void logSDLError(std::ostream &os, const std::string &msg) {
	os << msg << " error: " << SDL_GetError() << std::endl;
}


/*
	Draw an SDL_Texture to an SDL_Renderer at position x, y, preserving
	the texture's width and height
	
	\param tex The source texture we want to draw
	\param ren The renderer we want to draw to
	\param x The x coordinate to draw to
	\param y The y coordinate to draw to
*/
void renderTexture(SDL_Texture *tex, SDL_Renderer *ren, int x, int y, SDL_Texture **mssgs, int mssgCt) {
	//Setup the destination rectangle to be at the position we want
	SDL_Rect dst;
	dst.x = x;
	dst.y = y;

	//Query the texture to get its width and height to use
	SDL_QueryTexture(tex, NULL, NULL, &dst.w, &dst.h);
	SDL_RenderCopy(ren, tex, NULL, &dst);


	SDL_Rect mdst;
	mdst.x = 0;
	mdst.y = 0;
	mdst.w = 150;
	mdst.h = 25;

	for (int i = 0; i < mssgCt; i++)
	{
		SDL_RenderCopy(ren, mssgs[i], NULL, &mdst);
		mdst.y += mdst.h; //draw downwards
	}
}
