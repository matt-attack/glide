dofile( "inc/include.lua" )

solution "IDE"

	language "C++"
	location ( os.get() .. "/" .. _ACTION )
	flags { "Unicode", "Symbols", "NoEditAndContinue", "NoPCH",
            "No64BitChecks", "StaticRuntime", "EnableSSE" } -- "NoRTTI"
	targetdir ( "../../gwen/lib/" .. os.get() .. "/" .. _ACTION )
	libdirs { "../../gwen/lib/", "../../gwen/lib/" .. os.get() }

	configurations
	{
		"Release",
		"Debug"
	}

	if ( _ACTION == "vs2010" or _ACTION=="vs2008" ) then
		buildoptions { "/MP"  }
	end



configuration "Release"
	defines { "NDEBUG" }
	flags{ "Optimize", "FloatFast" }
	includedirs { "../../gwen/include/" }

configuration "Debug"
	defines { "_DEBUG" }
	includedirs { "../../gwen/include/" }
	targetsuffix( "d" )

project "GWEN-DLL"
	defines { "GWEN_COMPILE_DLL" }
	files { "../../gwen/src/**.*", "../../gwen/include/Gwen/**.*" }
	kind "SharedLib"
	targetname( "gwen" )

project "GWEN-Static"
	defines { "GWEN_COMPILE_STATIC" }
	files { "../../gwen/src/**.*", "../../gwen/include/Gwen/**.*" }
	flags { "Symbols" }
	kind "StaticLib"
	targetname( "gwen_static" )

--
-- Renderers
--

DefineRenderer( "OpenGL",
                {"../../gwen/Renderers/OpenGL/OpenGL.cpp"} )

DefineRenderer( "OpenGL_DebugFont",
                { "../../gwen/Renderers/OpenGL/OpenGL.cpp",
                  "../../gwen/Renderers/OpenGL/DebugFont/OpenGL_DebugFont.cpp" } )

DefineRenderer( "SFML",
                { "../../gwen/Renderers/SFML/SFML.cpp" },
                SFML_DEFINES )

DefineRenderer( "SFML2",
                { "../../gwen/Renderers/SFML2/SFML2.cpp" },
                SFML2_DEFINES )

DefineRenderer( "Allegro",
                { "../../gwen/Renderers/Allegro/Allegro.cpp" } )

if ( os.get() == "windows" ) then
	DefineRenderer( "DirectX9",
                    { "../../gwen/Renderers/DirectX9/DirectX9.cpp" } )
	configuration( "Release" )
		includedirs { "$(DXSDK_DIR)/Include" }
		libdirs { "$(DXSDK_DIR)/lib/x86" }

	DefineRenderer( "Direct2D",
                    { "../../gwen/Renderers/Direct2D/Direct2D.cpp" } )
	includedirs { "$(DXSDK_DIR)/Include" }
	libdirs { "$(DXSDK_DIR)/lib/x86" }

	DefineRenderer( "GDI",
                    { "../../gwen/Renderers/GDIPlus/GDIPlus.cpp",
                      "../../gwen/Renderers/GDIPlus/GDIPlusBuffered.cpp" } )
end

--
-- Samples
--
			

if ( os.get() == "windows" ) then

	DefineSample( "Direct2D",
                  { "../Samples/Direct2D/Direct2DSample.cpp" },
                  { "UnitTest", "Renderer-Direct2D", "GWEN-Static", "d2d1",
                    "dwrite", "windowscodecs" } )
	includedirs { "$(DXSDK_DIR)/Include" }
	libdirs { "$(DXSDK_DIR)/lib/x86" }

	DefineSample( "DirectX9",
                  { "../Samples/Direct3D/Direct3DSample.cpp" },
                  { "UnitTest", "Renderer-DirectX9", "GWEN-Static" } )
	includedirs { "$(DXSDK_DIR)/Include" }
	libdirs { "$(DXSDK_DIR)/lib/x86" }

	DefineSample( "WindowsGDI",
                  { "../Samples/WindowsGDI/WindowsGDI.cpp" },
                  { "UnitTest", "Renderer-GDI", "GWEN-Static" } )

	DefineSample( "OpenGL",
                  { "../Samples/OpenGL/OpenGLSample.cpp" },
                  { "UnitTest", "Renderer-OpenGL", "GWEN-Static", "FreeImage", "opengl32" } )

	DefineSample( "OpenGL_DebugFont",
                  { "../Samples/OpenGL/OpenGLSample.cpp" },
                  { "UnitTest", "Renderer-OpenGL_DebugFont", "GWEN-Static", "FreeImage", "opengl32" },
                  nil,
                  { "USE_DEBUG_FONT" } )
				  
	DefineSample( "IDE",
                  { "../Direct3DSample.cpp", "../IDE.cpp", "../TextBoxCode.cpp" },
                  { "Renderer-OpenGL_DebugFont", "GWEN-Static", "Renderer-Direct2D", "d2d1"} )
	includedirs { "$(DXSDK_DIR)/Include" }
	libdirs { "$(DXSDK_DIR)/lib/x86" }

end
