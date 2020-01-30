#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>

// C RunTime Header Files
#include <stdlib.h>
#include <memory.h>
#include <tchar.h>

#include "IDE.h"
#include "Gwen/Gwen.h"
#include "Gwen/Skins/Simple.h"
#include "Gwen/Skins/TexturedBase.h"
#include "Gwen/Input/Windows.h"

//#include "Gwen/Renderers/DirectX9.h"
//include "Gwen/Renderers/OpenGL_DebugFont.h"
#include "Gwen/Renderers/Direct2D.h"

#include "Gwen/Controls/WindowCanvas.h"
#include "Gwen/Platform.h"

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "Shcore.lib")
//
// Program starts here
//

#include "language_support.h"

Gwen::Skin::TexturedBase* skin = 0;
std::vector<Gwen::Controls::WindowCanvas*> canvases;

Gwen::Controls::WindowCanvas* OpenWindow(const char* name, int w, int h)
{
	Gwen::Renderer::Direct2D* renderer = new Gwen::Renderer::Direct2D();
	//Gwen::Renderer::OpenGL_DebugFont* renderer = new Gwen::Renderer::OpenGL_DebugFont();

	auto skin = new Gwen::Skin::TexturedBase(renderer);

	Gwen::Controls::WindowCanvas* window_canvas = new Gwen::Controls::WindowCanvas(-1, -1, w, h, skin, name);
	window_canvas->SetSizable(true);

	skin->Init("DefaultSkin.png");
	skin->SetDefaultFont(L"Segoe UI", 11);// L"Segoe UI");

	canvases.push_back(window_canvas);

	return window_canvas;
}

#include <Windows.h>

std::string executable_path(const char *argv0)
{
	char buf[1024] = { 0 };
	DWORD ret = GetModuleFileNameA(NULL, buf, sizeof(buf));
	if (ret == 0 || ret == sizeof(buf))
	{
		//return executable_path_fallback(argv0);
	}
	return buf;
}

int main(int argc, char** args)
{
	// Setup details about each language for parsing and color coding
	InitializeLanguages();

	//Gwen::Renderer::DirectX9* renderer = new Gwen::Renderer::DirectX9(0);
	Gwen::Renderer::Direct2D* renderer = new Gwen::Renderer::Direct2D();
	//Gwen::Renderer::OpenGL_DebugFont* renderer = new Gwen::Renderer::OpenGL_DebugFont();

	skin = new Gwen::Skin::TexturedBase(renderer);

	//
	// The window canvas is a cross between a window and a canvas
	// It's cool because it takes care of creating an OS specific
	// window - so we don't have to bother with all that crap.
	//
	//
	//lets fix tabs and their names also add ability to rename/delete files maybe?
	auto window_canvas = new Gwen::Controls::WindowCanvas(-1, -1, 700.0f, 500.0f, skin, "The Return of the Ded-eye");
	window_canvas->SetSizable(true);

	//
	// Now it's safe to set up the skin
	//
	std::string path = executable_path(0);
	path = path.substr(0, path.find_last_of('\\'));
	skin->Init(path + "\\DefaultSkin.png");
	skin->SetDefaultFont(L"Segoe UI", 11);// L"Segoe UI");

	IDE* ppUnit = new IDE(window_canvas);
	ppUnit->SetPos(0, 0);

	//OpenWindow("Test", 200, 200);

	if (argc > 1)
		ppUnit->OpenTab(args[1]);

	while (!window_canvas->WantsQuit())
	{
		//save power if we arent on top
		bool on_top = false;
		if (window_canvas->IsOnTop())
			on_top = true;

		window_canvas->DoThink();

		//update other cavases
		for (int i = 0; i < canvases.size(); i++)
		{
			auto canv = canvases[i];
			if (canv->IsOnTop())
				on_top = true;

			if (canv->WantsQuit())
			{
				//remove it
				canvases.erase(canvases.begin() + i);
				auto render = canv->GetSkin()->GetRender();
				auto skin = canv->GetSkin();
				delete canv;
				delete skin;
				delete render;

				i--;
				continue;
			}

			canv->DoThink();
		}

		if (on_top == false)
			Gwen::Platform::Sleep(500);
	}
	delete window_canvas;
	return 0;
}