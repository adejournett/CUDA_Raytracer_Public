#pragma once

#include "glad/glad.h"
#include <SDL.h>
#include <string>
#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"

#include"scene.h"
#include"uidata.h"

class SDLManager
{
	public:
		SDLManager(Scene **d_world, UIData *ui, unsigned char *fb, float s, int w, int h, int x, int y);
		~SDLManager();
		void beginRender(Scene **d_world);
		void renderFrame(Scene **d_world);
		void renderUI(Scene **d_world, GLuint image_texture, int &fIdx, int &tIdx, float &rough, float &ks, float &index, float *omega, float &mcos, int &lIdx, float *lightPos, float *lightColor, float *v1, float *v2, bool lArea);
	private:
		SDL_Renderer *renderer;
		SDL_Texture *background;
		SDL_Window *window;
		SDL_GLContext glContext;

		UIData *uidata;

		int WIDTH, HEIGHT, tx, ty, guiWidth;
		bool quit, redrawContinue, trackingMouse, leftMouseButtonDown, trackballMove;
		float angle;
		dim3 blocks, threads;
		float3 axis, trans, lastPos;
		unsigned char *fb;
		float SCALE;
		int curx, cury, startX, startY;
		//unsigned char *data;


		void handleKeyPress(SDL_Event event, Scene **d_world);
		void processEvent(SDL_Event event, Scene **d_world);
		void startMotion(int x, int y);
		void stopMotion(int x, int y);
		void mouseMotion(int x, int y);
};
