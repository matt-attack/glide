/*
	GWEN
	Copyright (c) 2011 Facepunch Studios
	See license in Gwen.h
*/


#pragma once
#ifndef GWEN_UNITTEST_UNITTEST_H
#define GWEN_UNITTEST_UNITTEST_H

#include "Gwen/Gwen.h"
#include "Gwen/Align.h"
#include "Gwen/Utility.h"
#include "Gwen/Controls/WindowControl.h"
#include "Gwen/Controls/TabControl.h"
#include "Gwen/Controls/ListBox.h"
#include "Gwen/Controls/DockBase.h"
#include "Gwen/Controls/StatusBar.h"
#include "TextBoxCode.h"

#include <thread>
#include <mutex>

class IDE;

namespace Gwen
{
	namespace Controls
	{
		class TreeControl;
		class Properties;
		class MenuStrip;
		class TextBoxCode;
	}
}

class IProjectFormat;
class GUnit : public Gwen::Controls::Base
{
	public:

		GWEN_CONTROL_INLINE( GUnit, Gwen::Controls::Base )
		{
			m_pUnitTest = NULL;
		}

		void SetUnitTest(IDE* u) { m_pUnitTest = u; }

		void UnitPrint( Gwen::UnicodeString str );
		void UnitPrint( Gwen::String str );

		void Layout( Gwen::Skin::Base* skin )
		{
			if ( GetDock() != Gwen::Pos::None ) { return; }

			SizeToChildren( true, true );
		}


		IDE* m_pUnitTest;
};

class IDE : public Gwen::Controls::DockBase
{
	public:

		GWEN_CONTROL(IDE, Gwen::Controls::DockBase);

		~IDE();

		void PrintText( const Gwen::UnicodeString & str );

		void Render( Gwen::Skin::Base* skin );


		void MenuItemSelect(Gwen::Controls::Base* pControl);

		void OpenNewTab();//new file not open
		Gwen::Controls::TextBoxCode* OpenTab(const Gwen::String& filename, bool duplicate = true);


		void OnFileTreePress(Gwen::Controls::Base* pControl);

		void Layout(Gwen::Skin::Base* skin);

	private:
		IProjectFormat* active_project = 0;

		void ProcessMessages();

		void OnFileOpen(Gwen::Event::Info info);
		void OnProjectOpen(Gwen::Event::Info info);
		void OnFolderOpen(Gwen::Event::Info info);
		void OnFileSave(Gwen::Event::Info info);
		void OnBreakpointAdd(Gwen::Event::Info info);
		void OnCloseTab(Gwen::Controls::Base* pControl);

		Gwen::Controls::TextBoxCode* GetCurrentEditor();
		
		void OnCategorySelect( Gwen::Event::Info info );

		//debugging stuff
		std::thread debugging;
		HANDLE rpipe = 0, wpipe = 0;

		std::mutex line_mutex;
		std::vector<std::string> lines_to_add;

		//build thread and stuff
		std::thread build;
		HANDLE buildrpipe = 0;

		std::mutex output_mutex;
		std::vector<std::string> output_to_add;

		Gwen::Controls::TabControl*	m_Tabs;
		Gwen::Controls::ListBox*	m_TextOutput;

		Gwen::Controls::ListBox*    m_callstack;
		Gwen::Controls::Properties* m_locals;
		Gwen::Controls::ListBox* m_breakpoints;

		Gwen::Controls::StatusBar*	m_StatusBar;
		Gwen::Controls::TreeControl* file_list;
		unsigned int				m_iFrames;
		float						m_fLastSecond;

		Gwen::Controls::Base*		m_pLastControl;
		Gwen::Font					m_CodeFont;

		Gwen::Controls::MenuStrip*  m_menu;

		
		std::vector<Gwen::Controls::TextBoxCode::breakpoint> breakpoints;

		std::vector<std::pair<Gwen::Controls::TextBoxCode*, Gwen::Controls::TabButton*>> open_files;
};

#define DEFINE_UNIT_TEST( name, displayname ) GUnit* RegisterUnitTest_##name( Gwen::Controls::Base* tab ){ return new name( tab ); }
#endif
