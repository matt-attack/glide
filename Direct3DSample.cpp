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
//#include "Gwen/Renderers/OpenGL_DebugFont.h"
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

int main(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	//Gwen::Renderer::DirectX9* renderer = new Gwen::Renderer::DirectX9(0);
	Gwen::Renderer::Direct2D* renderer = new Gwen::Renderer::Direct2D();
	//Gwen::Renderer::OpenGL_DebugFont* renderer = new Gwen::Renderer::OpenGL_DebugFont();

	Gwen::Skin::TexturedBase			skin(renderer);

	//
	// The window canvas is a cross between a window and a canvas
	// It's cool because it takes care of creating an OS specific
	// window - so we don't have to bother with all that crap.
	//
	//
	//lets fix tabs and their names also add ability to rename/delete files maybe?
	Gwen::Controls::WindowCanvas window_canvas(-1, -1, 700.0f, 500.0f, &skin, "The Return of the Ded-eye");
	window_canvas.SetSizable(true);
	//window_canvas.SetScale(1.5);
	//Gwen::Controls::WindowCanvas window_canvas2(-1, -1, 700, 500, &skin, "Secondary Window Test");
	//window_canvas.SetSizable(true);

	//
	// Now it's safe to set up the skin
	//
	skin.Init("DefaultSkin.png");
	skin.SetDefaultFont(L"Segoe UI", 11);// L"Segoe UI");

	//
	// Create our unittest control
	//
	//UnitTest* ppUnit2 = new UnitTest(&window_canvas2);
	IDE* ppUnit = new IDE(&window_canvas);
	ppUnit->SetPos(10, 10);

	while (!window_canvas.WantsQuit())
	{
		//save power if we arent on top
		if (window_canvas.IsOnTop() == false)
			Gwen::Platform::Sleep(500);

		window_canvas.DoThink();
		//window_canvas2.DoThink();
	}
	return 0;
}