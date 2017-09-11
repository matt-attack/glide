/*
	GWEN
	Copyright (c) 2010 Facepunch Studios
	See license in Gwen.h
	*/

#include "IDE.h"
#include "Gwen/Controls/DockedTabControl.h"
#include "Gwen/Controls/WindowControl.h"
#include "Gwen/Controls/CollapsibleList.h"
#include "Gwen/Controls/Layout/Position.h"
#include "Gwen/Platform.h"
#include "Gwen/Controls/TreeControl.h"
#include "Gwen/Controls/Properties.h"
#include "Gwen/Controls/Layout/Table.h"
#include "Gwen/Controls/PropertyTree.h"

using namespace Gwen;

#define ADD_UNIT_TEST( name )\
	GUnit* RegisterUnitTest_##name( Gwen::Controls::Base* tab );\
	{\
		Controls::Button* pButton = cat->Add( #name );\
		pButton->SetName( #name );\
		GUnit* test = RegisterUnitTest_##name( pCenter );\
		test->Hide();\
		test->SetUnitTest( this );\
		pButton->onPress.Add( this, &ThisClass::OnCategorySelect, test );\
	}\

Gwen::Controls::TabButton* pButton = NULL;

#include "Gwen/Controls/MenuStrip.h"
#include "Gwen/Controls/MenuItem.h"
#include "Gwen/Controls/TabControl.h"
#include "Gwen/Controls/Dialogs/FileOpen.h"
#include "Gwen/Controls/Dialogs/FileSave.h"
#include "Gwen/Controls/Dialogs/FolderOpen.h"
#include "TextBoxCode.h"

IDE::~IDE()
{
	if (this->debugging.joinable())
	{
		//force quit gdb if its running
		std::string command = "-gdb-exit\n";
		WriteFile(wpipe, command.c_str(), command.length(), 0, 0);
		this->debugging.join();
	}
}

void IDE::OnFileOpen(Gwen::Event::Info info)
{
	auto name = info.String;
	this->OpenTab(name);
}

void find_files(LPCWSTR path, Gwen::Controls::TreeNode* parent, IDE* root){
	HANDLE hFind;
	WIN32_FIND_DATA FindFileData;
	std::wstring spath = path;
	if ((hFind = FindFirstFileW((spath + L"/*").c_str(), &FindFileData)) != INVALID_HANDLE_VALUE){
		do{
			if (FindFileData.cFileName[0] == '.')
				continue;

			auto node = parent->AddNode(FindFileData.cFileName);
			node->UserData.Set<std::wstring>("path", spath + L"\\" + FindFileData.cFileName);

			//node->SetToolTip(spath + L"/" + FindFileData.cFileName);

			node->onDoubleClick.Add(root, &IDE::OnFileTreePress);
			if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				//loop
				find_files((spath + L"\\" + FindFileData.cFileName).c_str(), node, root);
				node->SetImage(L"icons/folder.png");
				//let clicking open the file
			}

		} while (FindNextFile(hFind, &FindFileData));
		FindClose(hFind);
	}
}


void IDE::OnFolderOpen(Gwen::Event::Info info)
{
	Gwen::Controls::TreeNode* pNode = file_list->AddNode(info.String);
	pNode->SetImage(L"icons/folder.png");
	//pNode->setp
	find_files(info.String.GetUnicode().c_str(), pNode, this);
}

void IDE::OnFileSave(Gwen::Event::Info info)
{
	auto newname = info.String;

	auto button = this->m_Tabs->GetCurrentButton();
	auto page = dynamic_cast<Gwen::Controls::TextBoxCode*>(button->GetPage());

	if (page)
	{
		if (page->filename.length() == 0)
			button->SetText(newname.Get() + Gwen::String("       "));
		page->filename = newname;
		page->SaveToFile(newname);
		button->Invalidate();
		button->InvalidateChildren();
		button->GetTabControl()->Invalidate();
		button->GetTabControl()->InvalidateChildren();
	}
}

#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>

class IProjectFormat
{
public:
	virtual ~IProjectFormat() {}

	virtual void Open(const char* filename) = 0;

	virtual std::string GetBuildCommand() = 0;

	virtual std::string GetExecutablePath() { return ""; }

	virtual std::string GetProjectFolder() = 0;

	virtual std::string GetLineNumberRegex() = 0;

	virtual std::string GetColumnNumberRegex() { return ""; }

	virtual std::string GetFileNameRegex() = 0;

	virtual std::vector<std::string> GetDependencies() { return std::vector<std::string>(); }
};

class JetProject : public IProjectFormat
{
	std::string filename;
public:

	virtual void Open(const char* filename)
	{
		this->filename = filename;
	}

	virtual std::string GetBuildCommand()
	{
		return "jet " + this->GetProjectFolder() + " --linker=ld";
	}

	virtual std::string GetExecutablePath()
	{
		std::string project_name = this->GetProjectFolder();
		int off = project_name.find_last_of('\\');
		project_name = project_name.substr(off + 1);
		return this->GetProjectFolder() + "/build/" + project_name + ".exe";
	}

	virtual std::string GetProjectFolder()
	{
		std::string folder = filename;
		int off = folder.find_last_of('\\');
		folder = folder.substr(0, off);
		return folder;
	}

	virtual std::string GetLineNumberRegex()
	{
		return "\d+(?=:)";//one per line
	}

	virtual std::string GetColumnNumberRegex()
	{
		return "(?:\\d)\\d+[^:]";
	}

	virtual std::string GetFileNameRegex()
	{
		return "[\\w/]+.jet";
	}
};


void IDE::OnProjectOpen(Gwen::Event::Info info)
{
	auto name = info.String;

	int offset = name.Get().find_last_of('.');
	std::string extension = name.Get().substr(offset, name.Get().length() - offset);

	if (extension == ".jp")
	{
		auto project = new JetProject;
		project->Open(name.c_str());

		std::string short_name = project->GetProjectFolder();
		int p = short_name.find_last_of('\\')+1;
		if (p > 0)
			short_name = short_name.substr(p, short_name.length() - p);
		Gwen::Controls::TreeNode* pNode = file_list->AddNode(short_name);// project->GetProjectFolder());
		pNode->SetImage(L"icons/folder.png");
		pNode->SetText(pNode->GetText().GetUnicode() + L" (Active)");
		pNode->UserData.Set<std::string>("path", project->GetProjectFolder());

		//	highlight active project in file window and allow changing
		find_files(Gwen::Utility::StringToUnicode(project->GetProjectFolder()).c_str(), pNode, this);

		//keep track of all open projects and set the new one as active
		this->projects.push_back(project);

		this->active_project = project;
	}
	else
	{

	}
}

void IDE::RefreshFiles()
{
	//todo: speed this up or thread it 
	auto children = file_list->GetChildNodes();
	for (auto ii : children)
	{
		auto path = ii->UserData.Get<std::string>("path");
		auto tn = (Gwen::Controls::TreeNode*)ii;
		
		auto nodes = tn->GetChildNodes();
		for (auto p : nodes)
		{
			p->DelayedDelete();
		}

		find_files(Gwen::Utility::StringToUnicode(path).c_str(), (Gwen::Controls::TreeNode*)ii, this);
	}
}


void IDE::MenuItemSelect(Controls::Base* pControl)
{
	Gwen::Controls::MenuItem* pMenuItem = (Gwen::Controls::MenuItem*) pControl;
	if (pMenuItem->GetText() == L"Quit")
	{
		exit(0);
	}
	else if (pMenuItem->GetText() == L"New")
	{
		this->OpenNewTab();
	}
	else if (pMenuItem->GetText() == L"Refresh Files")
	{
		this->RefreshFiles();
	}
	else if (pMenuItem->GetText() == L"Save")
	{
		//need better way of telling active tab
		//Gwen::Dialogs::FileSave
		auto button = this->m_Tabs->GetCurrentButton();
		auto page = this->GetCurrentEditor();// dynamic_cast<Gwen::Controls::TextBoxCode*>(button->GetPage());
		auto text = button->GetText();

		if (page)
		{
			//if we know the filename, do a save as, also if no changes dont save, but we dont have that implemented yet
			if (page->filename.length() == 0)
			{
				Gwen::Dialogs::FileSave(true, page->filename, String(""), String(".h|.hpp|.c|.cpp"), this, &ThisClass::OnFileSave);
				return;
			}

			//ok, just save normally
			if (page->filename.length() == 0)
				button->SetText(button->GetText().Get() + Gwen::String("       "));

			page->SaveToFile(page->filename);
		}
	}
	else if (pMenuItem->GetText() == L"Save As...")
	{
		//Gwen::Dialogs::FileSave
		auto button = this->GetCurrentEditor();// this->m_Tabs->GetCurrentButton();
		auto page = button;// dynamic_cast<Gwen::Controls::TextBoxCode*>(button->GetPage());
		auto text = button->GetText();

		if (page)
		{
			Gwen::Dialogs::FileSave(true, page->filename, String(""), String(".h|*.h|.hpp|*.hpp|.c|*.c|.cpp|*.cpp|All|*.*"), this, &ThisClass::OnFileSave);
		}
	}
	else if (pMenuItem->GetText() == L"Open")
	{
		Gwen::Dialogs::FileOpen(true, String("Open File"), String(""), String("All|*.*|.h|*.h"), this, &ThisClass::OnFileOpen);
	}
	else if (pMenuItem->GetText() == L"Open Folder")
	{
		Gwen::Dialogs::FolderOpen(true, String("Open Folder"), String(""), this, &ThisClass::OnFolderOpen);
	}
	else if (pMenuItem->GetText() == L"Open Project")
	{
		Gwen::Dialogs::FileOpen(true, String("Open Project"), String(""), String(".jp|*.jp|All|*.*"), this, &ThisClass::OnProjectOpen);
	}
	else if (pMenuItem->GetText() == L"Start Debugging")
	{
		if (this->active_project == 0)
			return;

		if (this->debugging.joinable())
			return;

		this->m_TextOutput->Clear();

		HANDLE hPipeRead, hPipeWrite;
		HANDLE hPipeInRead, hPipeInWrite;

		SECURITY_ATTRIBUTES saAttr = { sizeof(SECURITY_ATTRIBUTES) };
		saAttr.bInheritHandle = TRUE;   //Pipe handles are inherited by child process.
		saAttr.lpSecurityDescriptor = NULL;

		// Create a pipe to get results from child's stdout.
		if (!CreatePipe(&hPipeRead, &hPipeWrite, &saAttr, 0))
			return;// strResult;

		SetHandleInformation(hPipeRead, HANDLE_FLAG_INHERIT, 0);

		//create pipe for stdin
		if (!CreatePipe(&hPipeInRead, &hPipeInWrite, &saAttr, 0))
			return;// strResult;

		SetHandleInformation(hPipeInWrite, HANDLE_FLAG_INHERIT, 0);


		std::wstring cmdb = L"gdb.exe ";
		cmdb += Gwen::Utility::StringToUnicode(this->active_project->GetExecutablePath());
		cmdb += L" --interpreter=mi";
		//WCHAR cmd[] = L"gdb.exe ForLoop.exe --interpreter=mi";

		const WCHAR* cmd = cmdb.c_str();
		STARTUPINFO si = { sizeof(STARTUPINFO) };
		si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
		si.hStdOutput = hPipeWrite;
		si.hStdError = hPipeWrite;
		si.hStdInput = hPipeInRead;
		si.wShowWindow = SW_HIDE;       // Prevents cmd window from flashing. Requires STARTF_USESHOWWINDOW in dwFlags.

		PROCESS_INFORMATION pi = { 0 };

		BOOL fSuccess = CreateProcessW(NULL, (LPWSTR)cmd, NULL, NULL, TRUE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi);
		if (!fSuccess)
		{
			CloseHandle(hPipeWrite);
			CloseHandle(hPipeRead);
			CloseHandle(hPipeInRead);
			CloseHandle(hPipeInWrite);
			return;// strResult;
		}

		Sleep(100);


		rpipe = hPipeRead;
		wpipe = hPipeInWrite;

		//set breakpoints
		for (auto& bp : this->breakpoints)
		{
			std::string command = "b \"" + bp.file + "\":" + std::to_string(bp.line) + "\n";
			WriteFile(wpipe, command.c_str(), command.length(), 0, 0);
		}

		std::string command = "set new-console on\nrun\n";
		WriteFile(wpipe, command.c_str(), command.length(), 0, 0);

		debugging = std::thread([this, pi]()
		{
			auto process = pi.hProcess;
			bool bProcessEnded = false;
			for (; !bProcessEnded;)
			{
				// Give some timeslice (50ms), so we won't waste 100% cpu.
				bProcessEnded = WaitForSingleObject(process, 50) == WAIT_OBJECT_0;

				// Even if process exited - we continue reading, if there is some data available over pipe.
				for (;;)
				{
					std::string strResult;

					char buf[16024];
					DWORD dwRead = 0;
					DWORD dwAvail = 0;

					if (!::PeekNamedPipe(rpipe, NULL, 0, NULL, &dwAvail, NULL))
						break;

					if (!dwAvail) // no data available, return
						break;

					if (!::ReadFile(rpipe, buf, Gwen::Utility::Min<int>(sizeof(buf) - 1, dwAvail), &dwRead, NULL) || !dwRead)
						// error, the child process might ended
						break;

					buf[dwRead] = 0;
					strResult += buf;
					OutputDebugStringA(strResult.c_str());
					std::stringstream x(strResult);

					//http://davis.lbl.gov/Manuals/GDB/gdb_24.html#SEC230
					this->line_mutex.lock();
					std::string line;
					while (std::getline(x, line))
					{
						//~ = CLI output
						//@ = output from running target
						//& = gdb internal debug output
						//* = out of band things like breakpoints being hit
						this->lines_to_add.push_back(line);
					}
					this->line_mutex.unlock();
					this->Invalidate();
				}
			}

			OutputDebugString(L"GDB Exitted!\n");
		});
	}
	else if (pMenuItem->GetText() == L"Stop Debugging")
	{
		if (this->debugging.joinable() == false)
			return;
		std::string command = "-gdb-exit\n";
		WriteFile(wpipe, command.c_str(), command.length(), 0, 0);
		this->debugging.join();
	}
	else if (pMenuItem->GetText() == L"Continue")
	{
		if (this->debugging.joinable() == false)
			return;
		std::string command = "-exec-continue\n";
		WriteFile(wpipe, command.c_str(), command.length(), 0, 0);
	}
	else if (pMenuItem->GetText() == L"Pause")
	{
		if (this->debugging.joinable() == false)
			return;
	}
	else if (pMenuItem->GetText() == L"Step Over")
	{
		if (this->debugging.joinable() == false)
			return;

		std::string command = "next\n";
		WriteFile(wpipe, command.c_str(), command.length(), 0, 0);
	}
	else if (pMenuItem->GetText() == L"Step")
	{
		if (this->debugging.joinable() == false)
			return;

		std::string command = "step\n";
		WriteFile(wpipe, command.c_str(), command.length(), 0, 0);
	}
	else if (pMenuItem->GetText() == L"Build Project")
	{
		//if (this->build.joinable())
		//	return;
		//if (this->build.joinable())
		//this->build.detach();

		//we can only build if we have an active project
		//todo show an error if this happens
		if (this->active_project == 0)
			return;

		if (this->build.joinable())
			this->build.join();

		this->m_TextOutput->Clear();

		HANDLE hPipeRead, hPipeWrite;
		HANDLE hPipeInRead, hPipeInWrite;
		/*add error highlighting
			better icons
			actual project build configs
			allow closing projects with context menu
			get context menu in editor to actually work*/
		SECURITY_ATTRIBUTES saAttr = { sizeof(SECURITY_ATTRIBUTES) };
		saAttr.bInheritHandle = TRUE;   //Pipe handles are inherited by child process.
		saAttr.lpSecurityDescriptor = NULL;

		// Create a pipe to get results from child's stdout.
		if (!CreatePipe(&hPipeRead, &hPipeWrite, &saAttr, 0))
			return;// strResult;

		SetHandleInformation(hPipeRead, HANDLE_FLAG_INHERIT, 0);

		//create pipe for stdin
		if (!CreatePipe(&hPipeInRead, &hPipeInWrite, &saAttr, 0))
			return;// strResult;

		SetHandleInformation(hPipeInWrite, HANDLE_FLAG_INHERIT, 0);

		//ok, need to actually provide project build command here
		//	we could probably start with a general project format and built in jet support which would move to a plugin eventually


		//WCHAR cmd[] = L"jet C:/Users/Matthew/Desktop/VM/AsmVM2/AsmVM/ded";

		auto bc = Gwen::Utility::StringToUnicode(this->active_project->GetBuildCommand());

		const WCHAR* cmd = bc.c_str();
		//this->file_list->
		STARTUPINFO si = { sizeof(STARTUPINFO) };
		si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
		si.hStdOutput = hPipeWrite;
		si.hStdError = hPipeWrite;
		si.hStdInput = hPipeInRead;
		si.wShowWindow = SW_HIDE;       // Prevents cmd window from flashing. Requires STARTF_USESHOWWINDOW in dwFlags.

		PROCESS_INFORMATION pi = { 0 };

		BOOL fSuccess = CreateProcessW(NULL, (LPWSTR)cmd, NULL, NULL, TRUE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi);
		if (!fSuccess)
		{
			CloseHandle(hPipeWrite);
			CloseHandle(hPipeRead);
			CloseHandle(hPipeInRead);
			CloseHandle(hPipeInWrite);
			return;// strResult;
		}

		Sleep(100);


		buildrpipe = hPipeRead;
		//wpipe = hPipeInWrite;

		std::string command = "set new-console on\nrun\n";
		WriteFile(wpipe, command.c_str(), command.length(), 0, 0);

		//set breakpoints
		for (auto& bp : this->breakpoints)
		{
			std::string command = "b " + std::to_string(bp.line) + "\n";
			WriteFile(wpipe, command.c_str(), command.length(), 0, 0);
		}

		build = std::thread([this, pi]()
		{
			auto process = pi.hProcess;
			bool bProcessEnded = false;
			for (; !bProcessEnded;)
			{
				// Give some timeslice (50ms), so we won't waste 100% cpu.
				bProcessEnded = WaitForSingleObject(process, 50) == WAIT_OBJECT_0;

				// Even if process exited - we continue reading, if there is some data available over pipe.
				for (;;)
				{
					std::string strResult;

					char buf[16024];
					DWORD dwRead = 0;
					DWORD dwAvail = 0;

					if (!::PeekNamedPipe(buildrpipe, NULL, 0, NULL, &dwAvail, NULL))
						break;

					if (!dwAvail) // no data available, return
						break;

					if (!::ReadFile(buildrpipe, buf, Gwen::Utility::Min<int>(sizeof(buf) - 1, dwAvail), &dwRead, NULL) || !dwRead)
						// error, the child process might ended
						break;

					buf[dwRead] = 0;
					strResult += buf;
					OutputDebugStringA(strResult.c_str());
					std::stringstream x(strResult);

					//http://davis.lbl.gov/Manuals/GDB/gdb_24.html#SEC230
					this->output_mutex.lock();
					std::string line;
					while (std::getline(x, line))
					{
						//~ = CLI output
						//@ = output from running target
						//& = gdb internal debug output
						//* = out of band things like breakpoints being hit
						//if (line[0] == '~')
						//	printf("");
						this->output_to_add.push_back(line);
					}
					this->output_mutex.unlock();
					this->Invalidate();
				}
			}

			OutputDebugString(L"Build Exited!\n");
		});
	}
	//UnitPrint(Utility::Forma(L"Menu Selected: %ls", pMenuItem->GetText().GetUnicode().c_str()));
}

#include <algorithm>
class GdbNode
{
public:
	~GdbNode()
	{
		for (auto ii : children)
			delete ii.second;
	}
	bool is_value = false;
	std::string value;

	std::map<std::string, GdbNode*> children;
};

const char* ParseGdb(const char* str, GdbNode* node)
{
	//eat whitespace
	while (*str == ' ')
		str++;

	if (*str == '[')
	{
		str++;
		if (*str == ']')
		{
			str++;
			return str;
		}

		//parse an array out
		int num = 0;
		while (1)
		{
			//eat whitespace
			while (*str == ' ')
				str++;

			GdbNode* nn = new GdbNode;
			node->children[std::to_string(num++)] = nn;
			str = ParseGdb(str, nn);

			//eat the comma
			if (*str == ',')
			{
				str++;
				continue;
			}
			break;
		}
		if (*str == ']')
			str++;
	}
	else if (*str == '{')
	{
		str++;
		//parse an object
		while (1)
		{
			//eat whitespace
			while (*str == ' ')
				str++;

			//read in the key
			std::string key;
			while (*str != ' ' && *str != '=')
				key += *str++;

			printf("hi");

			while (*str == ' ')
				str++;

			str++;//eat equals sign

			while (*str == ' ')
				str++;

			//start reading value

			GdbNode* nn = new GdbNode;
			node->children[key] = nn;
			str = ParseGdb(str, nn);

			if (*str == ',')
			{
				str++;
				continue;
			}
			break;
		}
		if (*str == '}')
			str++;
	}
	else if (*str == '"')
	{
		str++;
		node->is_value = true;
		//its a string read it in
		while (1)
		{
			if (*str == '"')
				break;
			if (*str == '\\' && *(str + 1) == '"')
			{
				node->value += "\"";
				str += 2;
				continue;
			}
			else if (*str == '\\' && *(str + 1) == '\\')
			{
				node->value += '\\';
				str += 2;
				continue;
			}
			node->value += *str++;
		}
		str++;
		printf("");
	}
	else
	{
		//its something else, just read until we hit a delimiter
		node->is_value = true;
		//its a string read it in
		while (*str != '{' && *str != ',' && *str != '}' && *str != ']')
		{
			node->value += *str++;
		}
		printf("");
	}
	return str;
}

void StringReplace(std::string& subject, const std::string& search, const std::string& replace) {
	size_t pos = 0;
	while ((pos = subject.find(search, pos)) != std::string::npos) {
		subject.replace(pos, search.length(), replace);
		pos += replace.length();
	}
}

void IDE::Layout(Skin::Base* skin)
{
	this->ProcessMessages();

	BaseClass::Layout(skin);
}

void IDE::ProcessMessages()
{
	//process debug updates
	line_mutex.lock();

	if (lines_to_add.size() > 0)
	{
		for (auto ii : lines_to_add)
		{
			if (ii[0] == '~')
				this->PrintText(Gwen::Utility::StringToUnicode(ii));
			else if (ii[0] == '*')
			{
				if (ii[1] == 'r')
				{
					//we are running, remove the indicator
					//also lets look at breakpoint list when rendering and just share a pointer of it to our tabs
					//	that fixes it
					//ok, lets make the line indicator different than breakpoints this is screwing me up a lot
					for (auto ii : this->open_files)
					{
						ii.first->line_indicators.clear();
						ii.first->Redraw();
					}
					continue;
				}
				if (ii[1] != 's')
					continue;

				if (strncmp(ii.c_str(), "*stopped,reason=\"breakpoint", 20) != 0)
				{
					//std::string command = "-gdb-exit\n";
					//WriteFile(wpipe, command.c_str(), command.length(), 0, 0);
					//todo handle this as quit and kill the debugger
					//todo, handle these correctly
					//continue;
				}
				if (strncmp(ii.c_str(), "*stopped,reason=\"exited-normally", 20) == 0
					|| strncmp(ii.c_str(), "*stopped\r", 10) == 0)
				{
					std::string command = "-gdb-exit\n";
					WriteFile(wpipe, command.c_str(), command.length(), 0, 0);
					continue;
				}

				//todo, need to check stopped reason
				//we stopped, enable continue and disable pause
				//this->m_menu->get
				//~"0x77407c04 in KERNEL32!BaseThreadInitThunk () from C:\\Windows\\SysWOW64\\kernel32.dll\n"
				//*stopped, frame = { addr = "0x77407c04", func = "KERNEL32!BaseThreadInitThunk", args = [], from = "C:\\Windows\\SysWOW64\\kernel32.dll" }, thread - id = "1", stopped - threads = "all"

				int pos = ii.find("fullname=\"");
				if (pos == -1)
				{
					//todo actually handle it ok, todo handle when we dont have a fullname because its some random function like above
					std::string command = "-stack-list-locals 1\n-stack-list-frames\n-break-list\n";
					WriteFile(wpipe, command.c_str(), command.length(), 0, 0);
					continue;
				}
				std::string sub = ii.substr(pos + 10, ii.find("\"", pos + 10) - pos - 10);
				StringReplace(sub, "\\\\", "\\");

				pos = ii.find("line=\"");
				std::string linesub = ii.substr(pos + 6, ii.find("\"", pos + 6) - pos - 6);

				auto tab = this->OpenTab(sub, false);
				if (tab)
				{
					int line = std::stoi(linesub) - 1;
					tab->SetCursorPos(line, 0);
					tab->SetCursorEnd(line, 0);
					tab->GoToLine(line - 5);
					tab->RefreshCursorBounds();

					//move scroll to put it on the screen

					tab->Invalidate();
					tab->Redraw();

					//todo, mark this line
					tab->line_indicators.push_back({ line, 0 });
				}

				std::string command = "-stack-list-locals 1\n-stack-list-frames\n-break-list\n";
				WriteFile(wpipe, command.c_str(), command.length(), 0, 0);
			}
			else if (ii[0] == '^')
			{
				//its a result probably
				if (ii.substr(1, 11) == "done,locals")
				{
					this->m_locals->Clear();

					GdbNode start;
					auto str = ii.substr(13);
					ParseGdb(str.c_str(), &start);

					for (auto ii : start.children)
					{
						//for each var parse out the value
						GdbNode value;
						ParseGdb(ii.second->children["value"]->value.c_str(), &value);
						printf("");

						if (ii.second->children["value"]->value[1] == '<')
						{
							this->m_locals->Add(ii.second->children["name"]->value, "<No Debug Info>");
							continue;
						}

						for (auto pp : value.children)
							this->m_locals->Add(ii.second->children["name"]->value + "." + pp.first, pp.second->value);

						if (value.children.size() == 0)
							this->m_locals->Add(ii.second->children["name"]->value, ii.second->children["value"]->value);
					}

					printf("");
					int pos = 0;
					/*while (true)
					{
					pos = ii.find("name=\"", pos);
					if (pos <= 0)
					break;

					std::string sub = ii.substr(pos + 6, ii.find("\"", pos + 6) - pos - 6);

					pos = ii.find("value=\"", pos);
					std::string valuesub = ii.substr(pos + 7, ii.find("\"", pos + 7) - pos - 7);
					//todo actually parse it here
					this->m_locals->Add(sub, valuesub);
					}*/
				}
				else if (ii.substr(1, 4) == "exit")
				{
					//clear out debug info
					this->m_locals->Clear();
					this->m_callstack->Clear();

					//remove indicators (later need to make sure this doesnt remove error related ones)
					for (auto ii : this->open_files)
						ii.first->line_indicators.clear();

					if (this->debugging.joinable())
						this->debugging.join();
				}
				else if (ii.substr(1, 10) == "done,stack")
				{
					this->m_callstack->Clear();

					//need to extract func, fullname, and line
					int pos = 0;
					while (true)
					{
						pos = ii.find("func=\"", pos);
						if (pos <= 0)
							break;

						std::string sub = ii.substr(pos + 6, ii.find("\"", pos + 6) - pos - 6);

						pos = ii.find("fullname=\"", pos);
						std::string filesub = ii.substr(pos + 10, ii.find("\"", pos + 10) - pos - 10);

						pos = ii.find("line=\"", pos);
						std::string linesub = ii.substr(pos + 6, ii.find("\"", pos + 6) - pos - 6);

						this->m_callstack->AddItem(sub + "():" + linesub, "ok");
					}
				}
				else if (ii.substr(1, 20) == "done,BreakpointTable")
				{
					//todo: probably should impose a limit on the number of breakpoints
					this->m_breakpoints->Clear();
					this->breakpoints.clear();

					//this doesnt seem right
					//need to extract func, fullname, and line
					int pos = 0;
					while (true)
					{
						pos = ii.find("func=\"", pos);
						if (pos <= 0)
							break;

						std::string sub = ii.substr(pos + 6, ii.find("\"", pos + 6) - pos - 6);

						pos = ii.find("fullname=\"", pos);
						std::string filesub = ii.substr(pos + 10, ii.find("\"", pos + 10) - pos - 10);
						StringReplace(filesub, "\\\\", "\\");

						pos = ii.find("line=\"", pos);
						std::string linesub = ii.substr(pos + 6, ii.find("\"", pos + 6) - pos - 6);

						this->m_breakpoints->AddItem(sub + "():" + linesub, "ok");

						//add the breakpoint to the editor
						int line = std::atoi(linesub.c_str());
						this->breakpoints.push_back({ line, filesub });
					}
				}
			}

			//*stopped and *running are very useful things to indicate which options should be enabled
			//-break-list returns the breakpoints
			//todo use -stack-list-frames to get backtrace on every stop
			//c for continue
			//b for breakpoints
			//clear to get rid of a breakpoint
			//-stack-list-locals 1 lists the locals and values
			//-gdb-exit quits immediately
		}
	}

	this->lines_to_add.clear();
	line_mutex.unlock();

	//process build updates
	output_mutex.lock();

	if (output_to_add.size() > 0)
	{
		for (auto ii : output_to_add)
		{
			//todo: parse it here for error info
			std::replace(ii.begin(), ii.end(), '\r', ' ');
			this->PrintText(Gwen::Utility::StringToUnicode(ii));
		}
	}

	this->output_to_add.clear();
	output_mutex.unlock();
}


void IDE::OnFileTreePress(Gwen::Controls::Base* pControl)
{
	Gwen::Controls::TreeNode* node = static_cast<Gwen::Controls::TreeNode*>(pControl);
	
	if (node->UserData.Exists("path") == false)
		return;

	std::wstring file = node->UserData.Get<std::wstring>("path");
	this->OpenTab(Gwen::Utility::UnicodeToString(file), false);

	return;
}

GWEN_CONTROL_CONSTRUCTOR(IDE)
{
	//parsing practice
	//ok [ ] indicates array
	// { } indicates struct
	std::string str = "[{name = \"x\", value = \"{data = 0x6d8b5c \\\"new\\\", max_size = 4, cur_size = 4, __vtable = 0x402050 <__string_vtable>, __vtable = 0x0}\"},"
		"{ name = \"y\", value = \"{data = 0x6da334 \\\"new\\\", max_size = 4, cur_size = 4, __vtable = 0x402050 <__string_vtable>, __vtable = 0x0}\" }]";
	str = "[{name = \"x\",value=\"{data = 0x6b8b5c \\\"new\\\", max_size = 4, cur_size = 4, __vtable = 0x402050 <__string_vtable>, __vtable = 0x0}\"},{name=\"y\",value=\"{data = 0x6ba2ec \\\"\\\", max_size = 4, cur_size = 1}]";
	GdbNode start;
	ParseGdb(str.c_str(), &start);

	for (auto ii : start.children)
	{
		//for each var parse out the value
		GdbNode value;
		ParseGdb(ii.second->children["value"]->value.c_str(), &value);
		printf("");
	}
	m_pLastControl = NULL;
	Dock(Pos::Fill);
	SetSize(1024, 768);
	this->SetPadding(Gwen::Padding(0, 0, 0, 0));
	//this->
	//add menu bar
	Gwen::Controls::MenuStrip* menu = new Gwen::Controls::MenuStrip(this->GetParent());
	m_menu = menu;
	menu->Dock(Pos::Top);
	{
		Gwen::Controls::MenuItem* pRoot = menu->AddItem(L"File");
		pRoot->GetMenu()->AddItem(L"New", L"test16.png", "Ctrl+N")->SetAction(this, &ThisClass::MenuItemSelect);

		pRoot->GetMenu()->AddItem(L"Open", L"test16.png", "Ctrl+L")->SetAction(this, &ThisClass::MenuItemSelect);
		pRoot->GetMenu()->AddItem(L"Open Folder", L"test16.png", "")->SetAction(this, &ThisClass::MenuItemSelect);
		pRoot->GetMenu()->AddItem(L"Open Project", L"test16.png", "")->SetAction(this, &ThisClass::MenuItemSelect);

		pRoot->GetMenu()->AddItem(L"Save", "icons/disk.png", "Ctrl+S")->SetAction(this, &ThisClass::MenuItemSelect);
		pRoot->GetMenu()->AddItem(L"Save As...", "", "Ctrl+Shift+S")->SetAction(this, &ThisClass::MenuItemSelect);
		pRoot->GetMenu()->AddItem(L"Quit", "", "Ctrl+Q")->SetAction(this, &ThisClass::MenuItemSelect);

		pRoot->GetMenu()->AddItem(L"Refresh Files", "", "")->SetAction(this, &ThisClass::MenuItemSelect);
	}
	{
		Gwen::Controls::MenuItem* pRoot = menu->AddItem(L"Edit");
		pRoot->GetMenu()->AddItem(L"Undo", "", "Ctrl+Z");
		pRoot->GetMenu()->AddItem(L"Redo", "", "Ctrl+Y");
	}
	{
		Gwen::Controls::MenuItem* pRoot = menu->AddItem(L"Build");
		pRoot->GetMenu()->AddItem(L"Build Project", "", "F7")->SetAction(this, &ThisClass::MenuItemSelect);
	}
	{
		Gwen::Controls::MenuItem* pRoot = menu->AddItem(L"Debug");
		pRoot->GetMenu()->AddItem(L"Start Debugging", "", "")->SetAction(this, &ThisClass::MenuItemSelect);
		pRoot->GetMenu()->AddItem(L"Continue", "", "F5")->SetAction(this, &ThisClass::MenuItemSelect);
		pRoot->GetMenu()->AddItem(L"Pause", "", "")->SetAction(this, &ThisClass::MenuItemSelect);
		pRoot->GetMenu()->AddItem(L"Step", "", "F11")->SetAction(this, &ThisClass::MenuItemSelect);
		pRoot->GetMenu()->AddItem(L"Step Over", "", "F10")->SetAction(this, &ThisClass::MenuItemSelect);

		pRoot->GetMenu()->AddItem(L"Stop Debugging", "", "")->SetAction(this, &ThisClass::MenuItemSelect);
	}
	{
		Gwen::Controls::MenuItem* pRoot = menu->AddItem(L"View");
	}

	m_CodeFont.facename = L"Courier New";
	m_CodeFont.bold = false;
	m_CodeFont.realsize = 14;
	m_CodeFont.size = 14;

	{
		//solution browser experimenting
		Gwen::Controls::TreeControl* ctrl = new Gwen::Controls::TreeControl(this);
		Gwen::Controls::TreeNode* pNode = ctrl->AddNode(L"../");
		pNode->SetImage(L"test16.png");
		pNode->UserData.Set<std::string>("path", "../");

		//todo ok, instead of just format types need to store token types(kinda currently am so no big deal here)
		//then with that can provide hover data as well as auto suggest

		//so that means I need to completely tokenize :S

		//todo also need to add error parsing from compiler output


		//ctrl->SetCacheToTexture();
		ctrl->Dock(Pos::Fill);
		ctrl->ExpandAll();

		find_files(L"../", pNode, this);

		file_list = ctrl;

		GetRight()->GetTabControl()->AddPage("Files", ctrl);
	}
	//fix tab rendering and selection and stuff


	//Controls::CollapsibleList* pList = new Controls::CollapsibleList(this);
	//GetLeft()->GetTabControl()->AddPage("CollapsibleList", pList);
	//GetLeft()->SetWidth(150);
	m_TextOutput = new Controls::ListBox(GetBottom());
	pButton = GetBottom()->GetTabControl()->AddPage("Output", m_TextOutput);
	GetBottom()->SetHeight(200);
	m_StatusBar = new Controls::StatusBar(this->GetParent());
	m_StatusBar->Dock(Pos::Bottom);


	m_StatusBar->SendToBack();



	//this is where locals go
	Gwen::Controls::Properties* props = new Gwen::Controls::Properties(this);
	this->GetBottom()->GetTabControl()->AddPage("Locals", props);
	props->Dock(Pos::Fill);

	this->m_locals = props;

	//ok, we will use listview for this!
	m_callstack = new Controls::ListBox(this);
	GetBottom()->GetTabControl()->AddPage("Callstack", m_callstack);
	m_callstack->SetColumnCount(1);


	m_breakpoints = new Controls::ListBox(this);
	GetBottom()->GetTabControl()->AddPage("Breakpoints", m_breakpoints);
	m_breakpoints->SetColumnCount(1);
	/*auto table = new Controls::Layout::Table(this);
	table->SetColumnCount(3);
	table->SetColumnWidth(1, 100);
	auto row = table->AddRow();
	row->SetCellText(0, "Hi");
	row->SetCellText(1, "Doesaaaaaaaaaaaaaaaaaaaaaa");
	row->SetCellText(2, "This work");
	GetBottom()->GetTabControl()->AddPage("Test", table);
	table->Dock(Pos::Fill);*/

	m_fLastSecond = Gwen::Platform::GetTimeInSeconds();
	m_iFrames = 0;
	//pList->GetNamedChildren("MenuStrip").DoAction();

	//tab view of files
	m_Tabs = this->GetTop()->GetTabControl();// new Controls::TabControl(this);
	m_Tabs->SetTabStripPosition(Pos::Top);
	this->GetTop()->Dock(Pos::Fill);
	m_Tabs->SetSize(500, 500);
	m_Tabs->Dock(Pos::Fill);
	Gwen::Controls::DockedTabControl* tabs = gwen_cast<Gwen::Controls::DockedTabControl>(m_Tabs);
	tabs->GetTitleBar()->Hide();//hide the title bar
	//m_Tabs->Dock(Pos::Fill | Pos::Top);
	//m_Tabs->SetSize(200, 200);

	//auto top = this->GetTop()->GetTabControl()->AddPage(L"Welcome", 0);
	//use this now
	//this->GetTop()->GetTabControl()->AddPage(L"Testing2", 0);

	//ok, add modified indicator to pages and 
	//	close button for when they arent docked in tab control
	//	also fix up scrolling
	//then do selection

	auto page = m_Tabs->AddPage(L"Welcome", 0);

	this->OpenTab(String("test.txt"));

	m_Tabs->SetAllowReorder(true);
}


void IDE::OpenNewTab()
{
	auto page = new Gwen::Controls::TextBoxCode(this);
	page->SetFont(&this->m_CodeFont);
	page->SetText("", 0);

	auto tab = m_Tabs->AddPage("New       ", page);

	//TabTitleBar is what holds the title and where we can put the close button!
	Controls::Button* pButtonC = new Controls::Button(tab);
	pButtonC->SetSize(20, 20);
	pButtonC->SetPos(0, 2);
	pButtonC->Dock(Pos::Right);

	auto parent = tab->GetPage()->GetParent();
	auto dtc = dynamic_cast<Gwen::Controls::DockedTabControl*>(parent);
	auto titlebar = dtc->GetTitleBar();
	//todo ok, lets just integrate the closing into the tab system itself
	//ok, need to add button to the title bar somehow
	//or make tabs closable
	pButtonC->SetText("x");
	//pButtonC->SetImage(L"test16.png");
	pButtonC->onPress.Add(this, &ThisClass::OnCloseTab);

	page->onBreakpoint.Add(this, &ThisClass::OnBreakpointAdd);
	page->Dock(Pos::Fill);

	m_Tabs->OnTabPressed(tab);//bring the tab to the top
}

std::string ToLower(const std::string& str)
{
	std::string out;
	out.resize(str.length());
	for (int i = 0; i < str.length(); i++)
		out[i] = tolower(str[i]);
	return out;
}

#include <fstream>
Gwen::Controls::TextBoxCode* IDE::OpenTab(const Gwen::String& filename, bool duplicate)
{
	//make sure this wont open more than one of the same tab
	if (duplicate == false)
	{
		//try and find it
		//lets just to_lower everything
		//ok, we need to to lower everything for the comparison because we are getting issues
		std::string tmp = ToLower(filename);
		for (int i = 0; i < this->open_files.size(); i++)
		{
			if (ToLower(this->open_files[i].first->filename) == tmp)
				return this->open_files[i].first;
		}
	}

	//read in the whole file
	std::ifstream t(filename, std::ios::binary);
	if (t.is_open() == false)
		return 0;

	auto page = new Gwen::Controls::TextBoxCode(this);
	page->SetFont(&this->m_CodeFont);

	int length;
	t.seekg(0, std::ios::end);    // go to the end
	length = t.tellg();           // report location (this is the length)
	t.seekg(0, std::ios::beg);    // go back to the beginning
	char* buffer = new char[length];    // allocate memory for a buffer of appropriate dimension
	t.read(buffer, length);       // read the whole file into the buffer
	t.close();                    // close file handle
	page->SetText(buffer, length);

	delete[] buffer;

	int p = filename.find_last_of('\\') == -1 ? filename.find_last_of('/') : filename.find_last_of('\\');
	p += 1;
	std::string page_name = filename.substr(p, filename.length() - p);
	auto tab = m_Tabs->AddPage(page_name + "         ", page);
	auto children = tab->GetChildren();
	auto text = dynamic_cast<Gwen::ControlsInternal::Text*>(children.front());
	//text->SetToolTip("hi");
	//tab->SetToolTip(filename);
	//tab->GetTabControl()->SetToolTip(filename);
	Controls::Button* pButtonC = new Controls::Button(tab);
	pButtonC->SetSize(20, 20);
	pButtonC->SetPos(0, 2);
	pButtonC->Dock(Pos::Right);
	pButtonC->SetText("x");
	//pButtonC->SetImage(L"test16.png");
	pButtonC->onPress.Add(this, &ThisClass::OnCloseTab);
	//pButtonC->SetToolTip(filename);
	//ok, lets fix title names to be shortened and get tooltips working on them
	page->filename = filename;

	page->SetWrap(false);
	page->Dock(Pos::Fill);
	page->onBreakpoint.Add(this, &ThisClass::OnBreakpointAdd);

	m_Tabs->OnTabPressed(tab);//bring the tab to the top

	page->breakpoints = &this->breakpoints;
	this->open_files.push_back({ page, tab });
	return page;
}

void IDE::OnBreakpointAdd(Gwen::Event::Info info)
{
	std::string lowered = ToLower(info.String.m_String);
	if (this->debugging.joinable() == false)
	{
		//add/remove from list
		for (int i = 0; i < this->breakpoints.size(); i++)
		{
			if (ToLower(this->breakpoints[i].file) == lowered/*info.String.m_String*/ && this->breakpoints[i].line == info.Integer)
			{
				//remove it
				this->breakpoints.erase(this->breakpoints.begin() + i);
				return;
			}
		}

		//this->m_breakpoints->AddItem()
		//update breakpoint table
		this->breakpoints.push_back({ info.Integer, info.String.m_String });
		//auto file = this->OpenTab(info.String.m_String, false);
		//file->line_indicators.push_back({ info.Integer - 1, 1 });
		return;
	}
	//todo if we arent running just add/remove the breakpoint from a table that will
	//get set when we start debugging
	//need to also use the filename
	//if we already have the breakpoint, remove it with a clear command and then update list
	//auto tab = this->OpenTab(info.String.c_str(), false);
	//we have another case insensitivity issue with breakpoints to fix!
	for (int i = 0; i < this->breakpoints.size(); i++)
	{
		if (this->breakpoints[i].line == info.Integer && ToLower(breakpoints[i].file) == lowered/*info.String.m_String*/)
		{
			std::string command = "clear " + std::to_string(info.Integer) + "\n-break-list\n";
			WriteFile(wpipe, command.c_str(), command.length(), 0, 0);
			return;
		}
	}

	//run the command to add it and refresh
	std::string command = "b \"" + info.String.m_String +"\":" + std::to_string(info.Integer) + "\n-break-list\n";
	WriteFile(wpipe, command.c_str(), command.length(), 0, 0);
}

void IDE::OnCloseTab(Gwen::Controls::Base* pControl)
{
	auto parent = static_cast<Gwen::Controls::TabButton*>(pControl->GetParent());

	auto page = parent->GetPage();
	for (int i = 0; i < this->open_files.size(); i++)
	{
		if (this->open_files[i].first == page)
		{
			this->open_files.erase(open_files.begin() + i);
			break;
		}
	}

	parent->GetTabControl()->RemovePage(parent);
	parent->DelayedDelete();
	page->DelayedDelete();
}

Gwen::Controls::TextBoxCode* IDE::GetCurrentEditor()
{
	//todo: improve me
	auto button = this->m_Tabs->GetCurrentButton();
	return dynamic_cast<Gwen::Controls::TextBoxCode*>(button->GetPage());
}

void IDE::OnCategorySelect(Gwen::Event::Info info)
{
	if (m_pLastControl)
	{
		m_pLastControl->Hide();
	}

	static_cast<Gwen::Controls::Base*>(info.Data)->Show();
	m_pLastControl = static_cast<Gwen::Controls::Base*>(info.Data);
}

void IDE::PrintText(const Gwen::UnicodeString & str)
{
	m_TextOutput->AddItem(str);
	m_TextOutput->ScrollToBottom();
}

int val = 0;
void IDE::Render(Gwen::Skin::Base* skin)
{
	m_iFrames++;
	//show current line number of active tab, also need to figure out how to display active tab
	if (m_fLastSecond < Gwen::Platform::GetTimeInSeconds())
	{
		val = m_iFrames;
		m_fLastSecond = Gwen::Platform::GetTimeInSeconds() + 0.5f;
		m_iFrames = 0;
	}

	//lets update tabs to the right name if we are editted (todo, make this happen less often)
	for (auto ii : this->open_files)
	{
		if (ii.first->IsModified())
		{
			if (ii.second->GetText().c_str()[0] == '*')
				continue;
			std::string str = "*";
			str += ii.second->GetText().Get();
			ii.second->SetText(str);
		}
		else
		{
			if (ii.second->GetText().c_str()[0] != '*')
				continue;
			std::string str = ii.second->GetText().Get().substr(1, ii.second->GetText().Get().length());
			ii.second->SetText(str);
		}
	}

	auto editor = this->GetCurrentEditor();
	if (editor)
		m_StatusBar->SetText(Gwen::Utility::Format(L"\t%i fps\t\t\t\t\tLn %i\t Col %i", val * 2, editor->GetCurrentLine() + 1, editor->GetCurrentColumn() + 1));
	else
		m_StatusBar->SetText(Gwen::Utility::Format(L"\t%i fps\t\t\t\t\tLn %i\t Col %i", val * 2, 0, 0));

	BaseClass::Render(skin);
}

void GUnit::UnitPrint(Gwen::UnicodeString str)
{
	m_pUnitTest->PrintText(str);
}

void GUnit::UnitPrint(Gwen::String str)
{
	UnitPrint(Utility::StringToUnicode(str));
}

