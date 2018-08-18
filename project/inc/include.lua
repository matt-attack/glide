
dofile( os.get() .. ".lua" )

function DefineProject( name, filetable, linktable, linktabled, definestable )

	if ( linktabled == nil ) then linktabled = linktable end
	
	project( name )
	targetdir ( "../bin" )
	
	if ( debugdir) then
		debugdir ( "../bin" )
	end
	
	if ( definestable ) then defines( definestable ) end
	files { filetable }
	
	kind "WindowedApp"
		
	configuration( "Release" )
		targetname( name )
		links( linktable )
		
	configuration "Debug"
		targetname( name .. "_D" )
		links( linktabled )

end

