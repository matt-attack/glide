/*
	GWEN
	Copyright (c) 2010 Facepunch Studios
	See license in Gwen.h
	*/

#pragma once
#ifndef GWEN_CONTROLS_TEXTBOX_H
#define GWEN_CONTROLS_TEXTOX_H

#include "Gwen/BaseRender.h"
#include "Gwen/Controls/Base.h"
#include "Gwen/Controls/Label.h"
#include "Gwen/Controls/ScrollControl.h"

namespace Gwen
{
	namespace Controls
	{
		class Menu;
		class GWEN_EXPORT TextBoxCode : public Label
		{
			GWEN_CONTROL(TextBoxCode, Label);

			virtual ~TextBoxCode();
			
			struct Line
			{
				Gwen::UnicodeString	m_Unicode;
				std::vector<int> styles;
				bool dirty = true;
				bool is_comment = false;
			};
			std::list<Line> m_lines;

			virtual void Render(Skin::Base* skin);
			virtual void RenderFocus(Gwen::Skin::Base* /*skin*/) {};
			virtual void Layout(Skin::Base* skin);
			virtual void PostLayout(Skin::Base* skin);

#ifndef GWEN_NO_ANIMATION
			virtual void UpdateCaretColor();
#endif

			virtual bool OnChar(Gwen::UnicodeChar c);

			virtual void InsertText(const Gwen::UnicodeString & str);
			virtual void DeleteText(int iStartPos, int iStartLine, int iLength);

			virtual void RefreshCursorBounds();

			virtual bool OnKeyReturn(bool bDown);
			virtual bool OnKeyBackspace(bool bDown);
			virtual bool OnKeyDelete(bool bDown);
			virtual bool OnKeyRight(bool bDown);
			virtual bool OnKeyLeft(bool bDown);
			virtual bool OnKeyHome(bool bDown);
			virtual bool OnKeyEnd(bool bDown);
			virtual bool OnKeyTab(bool bDown);


			virtual void SetText(const char* text, unsigned int len);

			virtual int TextWidth();


			//virtual bool OnKeyReturn(bool bDown);
			//virtual void Render(Skin::Base* skin);
			//virtual void MakeCaratVisible();

			//virtual bool OnKeyHome(bool bDown);
			//virtual bool OnKeyEnd(bool bDown);
			virtual bool OnKeyUp(bool bDown);
			virtual bool OnKeyDown(bool bDown);

			virtual int GetCurrentLine();
			virtual int GetCurrentColumn();

			virtual bool AccelOnlyFocus() { return true; }

			virtual void OnPaste(Gwen::Controls::Base* pCtrl);
			virtual void OnCopy(Gwen::Controls::Base* pCtrl);
			virtual void OnCut(Gwen::Controls::Base* pCtrl);
			virtual void OnSelectAll(Gwen::Controls::Base* pCtrl);
			virtual void OnUndo(Gwen::Controls::Base* pCtrl);

			virtual void OnMouseDoubleClickLeft(int x, int y);

			virtual void EraseSelection();
			virtual bool HasSelection();
			virtual UnicodeString GetSelection();

			virtual void SetCursorPos(int line, int pos);
			virtual void SetCursorEnd(int line, int pos);

			virtual void OnMouseClickRight(int /*x*/, int /*y*/, bool /*bDown*/);
			virtual void OnMouseClickLeft(int x, int y, bool bDown);
			virtual void OnMouseMoved(int x, int y, int deltaX, int deltaY);

			virtual void SetEditable(bool b) { m_bEditable = b; }

			virtual void SetSelectAllOnFocus(bool b) { m_bSelectAll = b; if (b) { OnSelectAll(this); } }

			void GetCharacterAtPoint(int x, int y, int& line, int& column);

			virtual void MakeCaratVisible();

			virtual void OnEnter();

			virtual bool NeedsInputChars() { return true; }

			virtual void MoveCaretToEnd();
			virtual void MoveCaretToStart();

			void GoToLine(int l);

			void SaveToFile(const Gwen::String& filename);

			//virtual Gwen::Rect GetCharacterPosition(int hl, int vl);

			Event::Caller	onTextChanged;
			Event::Caller	onReturnPressed;
			Event::Caller	onBreakpoint;


			Gwen::String filename;

			struct indicator
			{
				int line;
				int type;
			};
			struct breakpoint
			{
				int line;
				std::string file;
			};
			std::vector<indicator> line_indicators;
			std::vector<breakpoint>* breakpoints = 0;

			bool IsModified()
			{
				return this->modified;
			}

		protected:

			bool needs_width_update = true;
			float text_width;

			void onMenuItemSelect(Controls::Base* pControl);

			int GetSidebarWidth();

			void onHScroll(Controls::Base* pControl);
			void onVScroll(Controls::Base* pControl);
			virtual bool OnMouseWheeled(int iDelta);
			virtual void OnTextChanged();
			virtual bool IsTextAllowed(const Gwen::UnicodeString & /*str*/, int /*iPos*/) { return true; }

			bool m_bEditable;
			bool m_bSelectAll;
			bool modified;

			int m_iCursorPos;
			int m_iCursorEndPos;
			int m_iCursorEndLine;
			int m_iCursorLine;

			int m_scroll;
			int m_hscroll;//number of pixels offset on h axis

			Gwen::Font icon_font;

			Gwen::Rect m_rectSelectionBounds;
			Gwen::Rect m_rectCaretBounds;

			float m_fNextCaretColorChange;
			Gwen::Color	m_CaretColor;

			Gwen::Controls::HorizontalScrollBar* m_hbar;
			Gwen::Controls::VerticalScrollBar* m_vbar;
			Gwen::Controls::Menu* m_context_menu;

			//for undo/redo
			struct Action
			{
				int line;
				int pos;
				int length;//if < 0 it was deleted
				std::wstring text;//the text either added or deleted
			};
			std::deque<Action> action_list;

			void AddUndoableAction(Action a);
		};
	}
}
#endif
