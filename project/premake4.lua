dofile( "inc/include.lua" )

solution "IDE"

	language "C++"
	location ( os.get() .. "/" .. _ACTION )
	flags { "Unicode", "Symbols", "NoEditAndContinue", "NoPCH",
            "No64BitChecks", "StaticRuntime", "EnableSSE" } -- "NoRTTI"
	targetdir ( "../../GWEN/gwen/lib/" .. os.get() .. "/" .. _ACTION )
	libdirs { "../../GWEN/gwen/lib/", "../../GWEN/gwen/lib/" .. os.get(), "../../GWEN/gwen/lib/" .. os.get() .. "/" .. _ACTION }

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
	includedirs { "../../GWEN/gwen/include/" }

configuration "Debug"
	defines { "_DEBUG" }
	includedirs { "../../GWEN/gwen/include/" }
	targetsuffix( "d" )

--
-- Project
--
			

if ( os.get() == "windows" ) then
				  
	DefineProject( "IDE",
                  { "../main.cpp", "../IDE.cpp", "../TextBoxCode.cpp", "../language_support.cpp" },
                  { "GWEN-Renderer-OpenGL_DebugFont", "gwen_static", "GWEN-Renderer-Direct2D", "d2d1"},
                  { "GWEND-Renderer-OpenGL_DebugFontd", "gwen_staticd", "GWEND-Renderer-Direct2Dd", "d2d1"})
	includedirs { "$(DXSDK_DIR)/Include" }
	libdirs { "$(DXSDK_DIR)/lib/x86" }

end
