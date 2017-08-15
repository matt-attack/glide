/*
	GWEN
	Copyright (c) 2010 Facepunch Studios
	See license in Gwen.h
	*/


#include "Gwen/Gwen.h"
#include "TextBoxCode.h"
#include "Gwen/Skin.h"
#include "Gwen/Anim.h"
#include "Gwen/Utility.h"
#include "Gwen/Platform.h"
#include "Gwen/Controls/Menu.h"
#include "Gwen/Controls/ListBox.h"
#include <math.h>

using namespace Gwen;
using namespace Gwen::Controls;

#ifndef GWEN_NO_ANIMATION
class ChangeCaretColor2 : public Gwen::Anim::Animation
{
public:

	virtual void Think()
	{
		gwen_cast<TextBoxCode> (m_Control)->UpdateCaretColor();
	}
};
#endif


GWEN_CONTROL_CONSTRUCTOR(TextBoxCode)
{
	SetSize(200, 20);
	SetMouseInputEnabled(true);
	SetKeyboardInputEnabled(true);
	SetAlignment(Pos::Left | Pos::CenterV);
	SetPadding(Padding(0, 0, 0, 0));// Padding(4, 2, 4, 2));
	m_iCursorPos = 0;
	m_iCursorEndPos = 0;
	m_iCursorLine = 0;
	m_iCursorEndLine = 0;
	m_bEditable = true;
	m_bSelectAll = false;
	SetTextColor(Gwen::Color(50, 50, 50, 255));   // TODO: From Skin
	SetTabable(true);
	AddAccelerator(L"Ctrl + C", &TextBoxCode::OnCopy);
	AddAccelerator(L"Ctrl + X", &TextBoxCode::OnCut);
	AddAccelerator(L"Ctrl + V", &TextBoxCode::OnPaste);
	AddAccelerator(L"Ctrl + A", &TextBoxCode::OnSelectAll);
	AddAccelerator(L"Ctrl + Z", &TextBoxCode::OnUndo);
	Gwen::Anim::Add(this, new ChangeCaretColor2());
	m_scroll = 0;
	m_hscroll = 0;
	SetWrap(true);
	SetAlignment(Pos::Left | Pos::Top);

	m_vbar = new Gwen::Controls::VerticalScrollBar(this);
	m_vbar->Dock(Pos::Right);
	m_vbar->SetViewableContentSize(1);
	m_vbar->SetContentSize(5);
	m_vbar->onBarMoved.Add(this, &ThisClass::onVScroll);

	m_hbar = new Gwen::Controls::HorizontalScrollBar(this);
	m_hbar->Dock(Pos::Bottom);
	m_hbar->SetViewableContentSize(1);
	m_hbar->SetContentSize(1);
	m_hbar->onBarMoved.Add(this, &ThisClass::onHScroll);
	modified = false;

	this->icon_font.facename = L"Courier";
	this->icon_font.bold = false;
	this->icon_font.realsize = 18;
	this->icon_font.size = 18;

	this->ac_menu = new Gwen::Controls::ListBox(this);
	ac_menu->AddItem("A");
	ac_menu->AddItem("B");
	ac_menu->AddItem("C");
	ac_menu->AddItem("D");
	ac_menu->AddItem("E");
	ac_menu->AddItem("F");
	ac_menu->AddItem("G");
	ac_menu->Hide();

	//use this
	auto menu = new Gwen::Controls::Menu(this);
	menu->AddItem("Cut")->SetAction(this, &ThisClass::onMenuItemSelect);
	menu->AddItem("Copy")->SetAction(this, &ThisClass::onMenuItemSelect);
	menu->AddItem("Paste")->SetAction(this, &ThisClass::onMenuItemSelect);
	menu->Hide();
	this->m_context_menu = menu;
}

TextBoxCode::~TextBoxCode()
{
	delete this->m_vbar;
	delete this->m_hbar;
	delete this->m_context_menu;
	delete this->ac_menu;
}

void TextBoxCode::OnUndo(Gwen::Controls::Base*)
{
	if (this->action_list.size() == 0)
	{
		this->modified = false;
		return;
	}

	//undo the last action
	auto action = this->action_list.back();

	//do the opposite
	if (action.length > 0)
	{
		//delete
		this->DeleteText(action.pos, action.line, action.length);
	}
	else
	{
		//add
		auto oldcp = this->m_iCursorPos;
		auto oldcl = this->m_iCursorLine;

		this->m_iCursorLine = this->m_iCursorEndLine = action.line;
		this->m_iCursorPos = this->m_iCursorEndPos = action.pos;
		this->InsertText(action.text);

		//this->m_iCursorPos = oldcp;
		//this->m_iCursorLine = oldcl;
	}
	this->action_list.pop_back();

	if (this->action_list.size() == 0)
	{
		this->modified = false;
		return;
	}
}

void TextBoxCode::onMenuItemSelect(Controls::Base* pControl)
{
	Gwen::Controls::MenuItem* pMenuItem = (Gwen::Controls::MenuItem*) pControl;
	if (pMenuItem->GetText() == L"Cut")
		this->OnCut(0);
	else if (pMenuItem->GetText() == L"Copy")
		this->OnCopy(0);
	else if (pMenuItem->GetText() == L"Paste")
		this->OnPaste(0);
}

void TextBoxCode::onVScroll(Controls::Base* pControl)
{
	float scroll = static_cast<Gwen::Controls::VerticalScrollBar*>(pControl)->GetScrolledAmount();
	this->m_scroll = scroll*(float)m_lines.size();
}

void TextBoxCode::onHScroll(Controls::Base* pControl)
{
	auto bar = static_cast<Gwen::Controls::HorizontalScrollBar*>(pControl);
	float scroll = static_cast<Gwen::Controls::HorizontalScrollBar*>(pControl)->GetScrolledAmount();
	this->m_hscroll = scroll*(this->TextWidth() - (this->Width() - this->m_hbar->GetSize().x - this->GetPadding().left));
}

std::list<TextBoxCode::Line>::iterator TextBoxCode::GetLine(unsigned int line)
{
	auto t = this->m_lines.begin();
	for (int i = 0; i < m_iCursorLine; i++)
		t++;

	return t;
}


bool TextBoxCode::OnChar(Gwen::UnicodeChar c)
{
	if (c == '\t') { return false; }
	
	ac_menu->Show();
	UnicodeString sub = this->GetLine(m_iCursorLine)->m_Unicode.substr(0, m_iCursorPos);
	//need to implement own measure function to account for tabs
	Gwen::PointF p = GetSkin()->GetRender()->MeasureText(GetFont(), sub);
	//ok, lets set it correctly on x axis then when user hits enter hide it
	ac_menu->SetPos(p.x + this->GetSidebarWidth(), (m_iCursorLine - m_scroll + 1)*this->GetFont()->size + 2);
	ac_menu->SetSize(100, 100);

	Gwen::UnicodeString str;
	str += c;

	this->AddUndoableAction({ this->m_iCursorLine, this->m_iCursorPos, 1, str });

	InsertText(str);
	return true;
}

bool TextBoxCode::OnMouseWheeled(int iDelta)
{
	//todo: make this even better, it kinda works but font rendering doesnt handle it very well
	if (Gwen::Input::IsControlDown())
	{
		if (this->m_Text->GetFont()->size + iDelta < 4)
			return true;

		this->m_Text->GetFont()->size += iDelta;
		//this->m_Text->GetFont()->realsize += iDelta;

		RefreshCursorBounds();

		return true;
	}

	this->m_scroll -= iDelta;

	if (this->m_scroll < 0)
		this->m_scroll = 0;
	if (this->m_scroll >= this->m_lines.size() - 1)
		this->m_scroll = this->m_lines.size() - 1;

	RefreshCursorBounds();

	return true;
}

void TextBoxCode::InsertText(const Gwen::UnicodeString & strInsert)
{
	if (!m_bEditable) return;
	//fix this a bit

	needs_width_update = true;

	if (HasSelection())
		EraseSelection(false);

	this->modified = true;

	auto line = this->m_lines.begin();
	for (int i = 0; i < m_iCursorLine; i++)
		line++;
	auto len = line->m_Unicode.length();
	if (m_iCursorPos > line->m_Unicode.length())
		m_iCursorPos = line->m_Unicode.length();

	if (!IsTextAllowed(strInsert, m_iCursorPos))
		return;

	auto t = this->m_lines.begin();
	for (int i = 0; i < m_iCursorLine; i++)
		t++;

	for (int i = 0; i < strInsert.length(); i++)
	{
		if (strInsert[i] == '\n')
		{
			m_iCursorLine++;
			if (m_iCursorPos < t->m_Unicode.length())
			{
				//push rest onto next line
				auto rest = t->m_Unicode.substr(m_iCursorPos, t->m_Unicode.length() - m_iCursorPos);
				t->m_Unicode = t->m_Unicode.substr(0, m_iCursorPos);
				t->dirty = true;
				t->width_dirty = true;
				Line n;
				n.m_Unicode = rest;
				n.dirty = true;
				n.width_dirty = true;
				n.is_comment = t->is_comment;
				t = m_lines.insert(t, n);
				m_iCursorPos = 0;
			}
			else
			{
				Line n;
				n.is_comment = t->is_comment;
				t = m_lines.insert(++t, n);
			}

			m_iCursorPos = 0;
		}
		else if (strInsert[i] == '\r')
		{

		}
		else
		{
			t->m_Unicode.insert(m_iCursorPos++, strInsert.substr(i, 1));
			t->dirty = true;
			t->width_dirty = true;
		}
	}

	m_iCursorEndPos = m_iCursorPos;
	m_iCursorEndLine = m_iCursorLine;

	RefreshCursorBounds();
}



#ifndef GWEN_NO_ANIMATION
void TextBoxCode::UpdateCaretColor()
{
	if (m_fNextCaretColorChange > Gwen::Platform::GetTimeInSeconds()) { return; }

	if (!HasFocus()) { m_fNextCaretColorChange = Gwen::Platform::GetTimeInSeconds() + 0.5f; return; }

	Gwen::Color targetcolor = Gwen::Color(230, 230, 230, 255);

	if (m_CaretColor == targetcolor)
	{
		targetcolor = Gwen::Color(20, 20, 20, 255);
	}

	m_fNextCaretColorChange = Gwen::Platform::GetTimeInSeconds() + 0.5;
	m_CaretColor = targetcolor;
	Redraw();
}
#endif

void TextBoxCode::RefreshCursorBounds()
{
	m_fNextCaretColorChange = Gwen::Platform::GetTimeInSeconds() + 1.5f;
	m_CaretColor = Gwen::Color(30, 30, 30, 255);
	MakeCaratVisible();
	//Gwen::Rect pA = GetCharacterPosition(m_iCursorPos);
	//Gwen::Rect pB = GetCharacterPosition(m_iCursorEnd);
	if (m_iCursorLine >= this->m_lines.size())
		m_iCursorLine = this->m_lines.size() - 1;
	if (m_iCursorLine < 0)
		m_iCursorLine = 0;
	auto t = this->m_lines.begin();
	for (int i = 0; i < m_iCursorLine; i++)
		t++;

	if (m_iCursorPos > t->m_Unicode.length())
		m_iCursorPos = t->m_Unicode.length();

	auto te = this->m_lines.begin();
	for (int i = 0; i < m_iCursorEndLine; i++)
		te++;

	if (m_iCursorEndPos > te->m_Unicode.length())
		m_iCursorEndPos = te->m_Unicode.length();

	if (m_iCursorPos == 0)
	{
		m_rectCaretBounds.x = 4;
	}
	else
	{
		UnicodeString sub = t->m_Unicode.substr(0, m_iCursorPos);
		//need to implement own measure function to account for tabs
		Gwen::PointF p = GetSkin()->GetRender()->MeasureText(GetFont(), sub);
		m_rectCaretBounds.x = 4/*GetPadding().left*/ + p.x - this->m_hscroll;

		if (m_rectCaretBounds.x > this->Width() - this->m_vbar->GetSize().x - this->GetPadding().left)
		{
			this->m_hscroll += 10;
			this->RefreshCursorBounds();
			return;
		}
		if (m_rectCaretBounds.x < 0)
		{
			this->m_hscroll -= 10;
			if (this->m_hscroll < 0)
				this->m_hscroll = 0;
			this->RefreshCursorBounds();
			return;
		}
	}
	m_rectCaretBounds.y = m_iCursorLine*this->GetFont()->size + 2;
	m_rectCaretBounds.w = 1;
	m_rectCaretBounds.h = this->GetFont()->size;

	/*m_rectSelectionBounds.x = Utility::Min(pA.x, pB.x);
	m_rectSelectionBounds.y = m_Text->Y() - 1;
	m_rectSelectionBounds.w = Utility::Max(pA.x, pB.x) - m_rectSelectionBounds.x;
	m_rectSelectionBounds.h = m_Text->Height() + 2;
	m_rectCaretBounds.x = pA.x;
	m_rectCaretBounds.y = pA.y;
	m_rectCaretBounds.w = 1;
	m_rectCaretBounds.h = pA.h;*/
	//update scrollbars?

	int text_width = this->TextWidth();
	//if (text_width < this->Width() - this->m_vbar->GetSize().x && this->m_vbar->GetScrolledAmount() == 0.0f)
	//	m_hbar->Hide();
	//else
	//m_hbar->Show();


	m_vbar->SetContentSize(this->m_lines.size());
	m_hbar->SetContentSize(text_width);

	m_vbar->SetScrolledAmount((float)m_scroll / (float)m_lines.size(), false);
	m_hbar->SetScrolledAmount((float)m_hscroll / (float)text_width, false);
	//(this->TextWidth() - (this->Width() - this->m_vbar->GetSize().x - this->GetPadding().left)), false);// text_width, false);

	Redraw();
}

int TextBoxCode::TextWidth()
{
	if (this->needs_width_update)
	{
		//just check each line on modifications
		int width = 0;
		auto t = this->m_lines.begin();
		//todo: lets remove the need to iterate through all lines
		for (int i = 0; i < this->m_lines.size(); i++)
		{
			if (t->width_dirty)
			{
				//update line width
				Gwen::PointF p = GetSkin()->GetRender()->MeasureText(GetFont(), t->m_Unicode);
				t->width = p.x;
				t->width_dirty = false;
			}
			//need to implement own measure function to account for tabs
			if (width < t->width)
				width = t->width;
			t++;
		}
		this->text_width = width;
		this->needs_width_update = false;
		return width;
	}
	//float width = this->GetSkin()->GetRender()->MeasureText(GetFont(), "W").x;
	return this->text_width;// *;
}

void TextBoxCode::OnPaste(Gwen::Controls::Base* /*pCtrl*/)
{
	auto text = Platform::GetClipboardText();

	this->AddUndoableAction({ this->m_iCursorLine, this->m_iCursorPos, text.length(), text });

	//todo: remove all /r
	InsertText(text);
}

void TextBoxCode::OnCopy(Gwen::Controls::Base* /*pCtrl*/)
{
	if (!HasSelection()) { return; }

	Platform::SetClipboardText(GetSelection());
}

void TextBoxCode::OnCut(Gwen::Controls::Base* /*pCtrl*/)
{
	if (!HasSelection()) { return; }

	Platform::SetClipboardText(GetSelection());
	EraseSelection();
}

void TextBoxCode::OnSelectAll(Gwen::Controls::Base* /*pCtrl*/)
{
	m_iCursorEndPos = 0;
	m_iCursorEndLine = 0;
	m_iCursorPos = 0;//todo make this end of line
	m_iCursorLine = this->m_lines.size();
	RefreshCursorBounds();
}

void TextBoxCode::GetCharacterAtPoint(int x, int y, int& line, int& column)
{
	auto pos = m_Text->CanvasPosToLocal(Gwen::Point(x, y));
	line = pos.y / this->GetFont()->size;
	line += this->m_scroll;
	float f_width = this->GetSkin()->GetRender()->MeasureText(this->GetFont(), "W").x;
	int iChar = 0;

	if (line >= this->m_lines.size())
		line = this->m_lines.size() - 1;

	//ok, lets look at the line to see where we are in it
	auto t = this->m_lines.begin();
	for (int i = 0; i < line; i++)
		t++;

	int offset = 0;
	for (int i = 0; i < t->m_Unicode.length(); i++)
	{
		WCHAR c = t->m_Unicode[i];
		if (c == '\t')
			offset = offset + (4 - offset % 4);//then round to the next greater 4th space

		float xoff = m_hscroll + offset++*f_width;
		if (xoff > pos.x - f_width)// / 4000)
		{
			iChar = i - 1;// offset - 1;
			break;
		}
		else if (i == t->m_Unicode.length() - 1)
			if (xoff + f_width > pos.x)// - f_width / 4000)
				iChar = i;
			else
				iChar = i + 1;
	}
	if (iChar < 0)
		iChar = 0;

	column = iChar;
}

//another useful feature would be to select other words on the page like the one you are inside of
void TextBoxCode::OnMouseDoubleClickLeft(int x, int y)
{
	//todo: select the whole word
	int line, column;
	this->GetCharacterAtPoint(x, y, line, column);

	m_iCursorPos = column;
	m_iCursorEndPos = column;
	m_iCursorLine = m_iCursorEndLine = line;

	auto t = this->m_lines.begin();
	for (int i = 0; i < line; i++)
		t++;

	//search left
	for (int i = column; i >= 0; i--)
	{
		if (isalnum(t->m_Unicode[i]) || t->m_Unicode[i] == '_')
			m_iCursorPos--;
		else
			break;
	}
	m_iCursorPos += 1;

	//search right
	for (int i = column; i < t->m_Unicode.length(); i++)
	{
		if (isalnum(t->m_Unicode[i]) || t->m_Unicode[i] == '_')
			m_iCursorEndPos++;
		else
			break;
	}

	RefreshCursorBounds();
	//todo make me actually work
	//then can look to the left and right of this for a non alphanumeric or _ to stop at
	//	then select all the text inside of it
	//	if we already have this as the selection, just deselect it
	//OnSelectAll(this);
}

UnicodeString TextBoxCode::GetSelection()
{
	if (!HasSelection()) { return L""; }

	int iStart = Utility::Min(m_iCursorPos, m_iCursorEndPos);
	int iEnd = Utility::Max(m_iCursorPos, m_iCursorEndPos);
	int iStartLine = Utility::Min(m_iCursorEndLine, m_iCursorLine);
	int iEndLine = Utility::Max(m_iCursorEndLine, m_iCursorLine);

	auto line = this->m_lines.begin();
	for (int i = 0; i < iStartLine; i++)
		line++;

	if (iStartLine == iEndLine)
		return line->m_Unicode.substr(iStart, iEnd - iStart);

	if (iStartLine == m_iCursorLine)
	{
		iStart = m_iCursorPos;
		iEnd = m_iCursorEndPos;
	}
	else
	{
		iStart = m_iCursorEndPos;
		iEnd = m_iCursorPos;
	}


	UnicodeString str = line->m_Unicode.substr(iStart, line->m_Unicode.length() - iStart);
	line++;
	for (int i = iStartLine + 1; i < iEndLine; i++)
	{
		str.append(L"\n");
		str.append(line->m_Unicode);
		line++;
	}
	str.append(L"\n");
	str.append(line->m_Unicode.substr(0, iEnd));
	return str;
}

bool TextBoxCode::OnKeyBackspace(bool bDown)
{
	if (!bDown) { return true; }

	if (HasSelection())
	{
		EraseSelection();
		return true;
	}

	if (m_iCursorPos == 0 && m_iCursorLine == 0) { return true; }

	if (Gwen::Input::IsControlDown())
	{
		auto line = this->m_lines.begin();
		for (int i = 0; i < m_iCursorLine; i++)
			line++;
		int p = 1;
		while (true)
		{
			if (m_iCursorPos - p <= 0)
			{
				p = m_iCursorPos;
				break;
			}
			if (line->m_Unicode.at(m_iCursorPos - p) == ' ' || line->m_Unicode.at(m_iCursorPos - p) == '\t')
			{
				p--;
				break;
			}
			p++;
		}
		if (p > 0)
			DeleteText(m_iCursorPos - p, m_iCursorLine, p);
	}
	else
	{
		//todo actually get this right
		//	have delete text return the deleted string?
		TextBoxCode::Action act;
		act.line = m_iCursorLine;
		act.pos = m_iCursorPos-1;
		act.length = -1;
		
		Gwen::UnicodeString removed = DeleteText(m_iCursorPos - 1, m_iCursorLine, 1);

		act.text = removed;
		this->AddUndoableAction(act);
	}
	return true;
}

bool TextBoxCode::OnKeyDelete(bool bDown)
{
	if (!bDown) { return true; }

	if (HasSelection())
	{
		EraseSelection();
		return true;
	}

	auto line = this->m_lines.begin();
	for (int i = 0; i < m_iCursorLine; i++)
		line++;
	if (m_iCursorPos > line->m_Unicode.length())
		return true;

	//if (m_iCursorPos >= TextLength()) { return true; }

	DeleteText(m_iCursorPos, m_iCursorLine, 1);
	return true;
}

bool TextBoxCode::OnKeyLeft(bool bDown)
{
	if (!bDown) { return true; }

	//auto line = this->m_lines.begin();
	//for (int i = 0; i < m_iCursorLine; i++)
	//	line++;
	//if (line->m_Unicode[m_iCursorPos] >= 0xDC00)
	//	m_iCursorPos--;

	if (m_iCursorPos == 0)
	{
		this->m_hscroll -= 10;
		if (this->m_hscroll < 0)
			this->m_hscroll = 0;
	}

	if (m_iCursorPos > 0)
	{
		m_iCursorPos--;
	}
	else if (m_iCursorLine != 0)
	{
		auto line = this->m_lines.begin();
		for (int i = 0; i < m_iCursorLine - 1; i++)
			line++;
		m_iCursorLine -= 1;
		m_iCursorPos = line->m_Unicode.length();
	}

	//implement control left and right
	if (!Gwen::Input::IsShiftDown())
	{
		m_iCursorEndPos = m_iCursorPos;
		m_iCursorEndLine = m_iCursorLine;
	}

	RefreshCursorBounds();
	return true;
}

bool TextBoxCode::OnKeyRight(bool bDown)
{
	if (!bDown) { return true; }

	auto line = this->m_lines.begin();
	for (int i = 0; i < m_iCursorLine; i++)
		line++;

	//skip over low surrogate pair
	if (line->m_Unicode[m_iCursorPos] & 0xD800)
		m_iCursorPos++;

	if (m_iCursorPos < line->m_Unicode.length())
	{
		m_iCursorPos++;
	}
	else
	{
		if (m_iCursorLine < this->m_lines.size() - 1)
		{
			m_iCursorLine++;
			m_iCursorPos = 0;
		}
	}

	if (!Gwen::Input::IsShiftDown())
	{
		m_iCursorEndPos = m_iCursorPos;
		m_iCursorEndLine = m_iCursorLine;
	}

	RefreshCursorBounds();
	return true;
}

void TextBoxCode::SetCursorPos(int line, int i)
{
	//if (m_iCursorPos == i) { return; }

	if (i < 0)
		m_iCursorPos = 0;
	else
		m_iCursorPos = i;

	m_iCursorLine = line;
	RefreshCursorBounds();
}

void TextBoxCode::SetCursorEnd(int line, int i)
{
	//if (m_iCursorEndPos == i) { return; }

	m_iCursorEndLine = line;
	m_iCursorEndPos = i;
	RefreshCursorBounds();
}


Gwen::UnicodeString TextBoxCode::DeleteText(int iStartPos, int iStartLine, int iLength)
{
	if (!m_bEditable) return Gwen::UnicodeString(L"");

	needs_width_update = true;
	this->modified = true;
	
	Gwen::UnicodeString removed;

	if (iStartPos < 0)
		iStartLine--;

	auto t = this->m_lines.begin();
	for (int i = 0; i < iStartLine; i++)
		t++;

	if (iStartPos < 0)
		iStartPos = t->m_Unicode.length() + iStartPos + 1;

	//start at the position then delete to the right
	if (iLength + iStartPos > t->m_Unicode.length())// && iStartLine != 0)
	{
		for (int i = 0; i < iLength; i++)
		{
			if (iStartPos >= t->m_Unicode.length())
			{
				if (m_iCursorLine == 0)
					continue;
				//remove a line
				auto line = *t;
				this->m_lines.erase(t++);
				unsigned int pos = line.m_Unicode.length();// t->m_Unicode.length();
				t->m_Unicode = line.m_Unicode + t->m_Unicode;// .append(line);
				t->dirty = true;
				t->width_dirty = true;
				this->m_iCursorLine--;
				this->m_iCursorPos = pos + 1;
			}
			else
			{
				t->m_Unicode.erase(iStartPos, 1);
				t->dirty = true;
				t->width_dirty = true;
			}
		}
	}
	else
	{
		removed = t->m_Unicode.substr(iStartPos, iLength);
		t->m_Unicode.erase(iStartPos, iLength);
		t->dirty = true;
	}
	//SetText(str);

	if (m_iCursorPos > iStartPos)
	{
		this->m_iCursorPos = m_iCursorPos - iLength;
		//SetCursorPos(m_iCursorPos - iLength);
	}

	SetCursorEnd(m_iCursorLine, m_iCursorPos);
	return removed;
}

bool TextBoxCode::HasSelection()
{
	return (m_iCursorPos != m_iCursorEndPos) || (m_iCursorLine != m_iCursorEndLine);
}

void TextBoxCode::EraseSelection(bool undoable)
{
	needs_width_update = true;

	//make start and end right
	int iStart = Utility::Min(m_iCursorPos, m_iCursorEndPos);
	int iEnd = Utility::Max(m_iCursorPos, m_iCursorEndPos);
	int iStartLine = Utility::Min(m_iCursorLine, m_iCursorEndLine);
	int iEndLine = Utility::Max(m_iCursorLine, m_iCursorEndLine);

	if (m_iCursorLine != m_iCursorEndLine)
	{
		if (iEndLine == m_iCursorEndLine)
		{
			iEnd = m_iCursorEndPos;
			iStart = m_iCursorPos;
		}
		else
		{
			iStart = m_iCursorEndPos;
			iEnd = m_iCursorPos;
		}
	}

	if (m_iCursorEndLine == m_iCursorLine)
	{
		Gwen::UnicodeString str = DeleteText(iStart, iStartLine, iEnd - iStart);
		if (undoable)
		{
			Action act;
			act.line = iStartLine;
			act.pos = iStart;
			act.text = str;
			act.length = -(iEnd - iStart);
			this->AddUndoableAction(act);
		}
	}
	else
	{
		auto t = this->m_lines.begin();
		for (int i = 0; i < iStartLine; i++)
			t++;

		t->m_Unicode.erase(iStart, t->m_Unicode.length() - iStart);
		t->dirty = true;
		//DeleteText(iStart, iStartLine, t->length() - iStart);

		auto start_line = t;
		t++;

		//just remove the rows in the middle
		for (int i = iStartLine + 1; i < iEndLine; i++)
		{
			//delete the line
			auto next = t;
			next++;
			this->m_lines.erase(t);
			t = next;
		}

		//delete last line
		t->dirty = true;
		t->m_Unicode.erase(0, iEnd);
		//DeleteText(0, iStartLine+1, iEnd);

		//merge start and end lines
		start_line->m_Unicode.append(t->m_Unicode);
		start_line->dirty = true;
		this->m_lines.erase(t);

		//cursor goes to start position
		this->m_iCursorLine = this->m_iCursorEndLine = iStartLine;
		this->m_iCursorPos = this->m_iCursorEndPos = iStart;
	}

	this->RefreshCursorBounds();
	// Move the cursor to the start of the selection,
	// since the end is probably outside of the string now.
	//m_iCursorPos = iStart;
	//m_iCursorEndPos = iStart;
}

void TextBoxCode::GoToLine(int l)
{
	if (l < 0)
		l = 0;
	if (l >= this->m_lines.size())
		l = this->m_lines.size() - 1;
	this->m_scroll = l;
	this->RefreshCursorBounds();
}

void TextBoxCode::OnMouseClickRight(int x, int y, bool /*bDown*/)
{
	auto pos = m_Text->CanvasPosToLocal(Gwen::Point(x, y));
	pos.x += 63;
	this->m_context_menu->Show();
	this->m_context_menu->SetPos(pos);

	this->ac_menu->Hide();
}

const int bp_bar_size = 18;
void TextBoxCode::OnMouseClickLeft(int x, int y, bool bDown)
{
	this->ac_menu->Hide();
	this->m_context_menu->Hide();

	if (m_bSelectAll)
	{
		OnSelectAll(this);
		m_bSelectAll = false;
		return;
	}

	int line, iChar;
	this->GetCharacterAtPoint(x, y, line, iChar);
	auto pos = m_Text->CanvasPosToLocal(Gwen::Point(x, y));
	//make it so people can add breakpoints then call
	//but dont add the breakpoint icon here, let our parent do that after the command completes
	if (pos.x < 0 && bDown)
	{
		Gwen::Event::Information info;
		info.Integer = line + 1;
		info.String = this->filename;
		info.ControlCaller = this;
		this->onBreakpoint.Call(this, info);
	}

	if (bDown)
	{
		this->m_iCursorLine = line;
		this->m_iCursorPos = iChar;

		if (!Gwen::Input::IsShiftDown())
		{
			this->m_iCursorEndPos = m_iCursorPos;
			this->m_iCursorEndLine = m_iCursorLine;
		}

		this->RefreshCursorBounds();
		Gwen::MouseFocus = this;
	}
	else
	{
		if (Gwen::MouseFocus == this)
		{
			this->m_iCursorPos = iChar;

			//SetCursorPos(iChar);
			this->m_iCursorLine = line;
			this->RefreshCursorBounds();
			Gwen::MouseFocus = NULL;
		}
	}
}

void TextBoxCode::OnMouseMoved(int x, int y, int /*deltaX*/, int /*deltaY*/)
{
	if (Gwen::MouseFocus != this) { return; }

	auto pos = m_Text->CanvasPosToLocal(Gwen::Point(x, y));
	int line = pos.y / this->GetFont()->size;
	line += this->m_scroll;
	float f_width = ((float)this->GetSkin()->GetRender()->MeasureText(this->GetFont(), "W").x) / 1.0f;
	int iChar = 0;
	//meh, lets assume fixed 
	if (line >= this->m_lines.size())
		this->m_iCursorLine = this->m_lines.size() - 1;
	else
		this->m_iCursorLine = line;

	//ok, lets look at the line to see where we are in it
	auto t = this->m_lines.begin();
	for (int i = 0; i < m_iCursorLine; i++)
		t++;

	int offset = 0;
	for (int i = 0; i < t->m_Unicode.length(); i++)
	{
		WCHAR c = t->m_Unicode[i];
		if (c == '\t')
			offset = offset + (4 - offset % 4);//then round to the next greater 4th space

		float xoff = m_hscroll + offset++*f_width;
		if (xoff > pos.x + f_width / 4)
		{
			iChar = i - 1;// offset - 1;
			break;
		}
		else if (i == t->m_Unicode.length() - 1)
			if (xoff + f_width > pos.x + f_width / 4)
				iChar = i;
			else
				iChar = i + 1;
	}
	if (iChar < 0)
		iChar = 0;


	SetCursorPos(this->m_iCursorLine, iChar);
	//int iChar = m_Text->GetClosestCharacter(m_Text->CanvasPosToLocal(Gwen::Point(x, y)));
	//SetCursorPos(iChar);
}

void TextBoxCode::Layout(Skin::Base* skin)
{
	BaseClass::Layout(skin);

	m_vbar->SetViewableContentSize((this->Height() - 2) / this->GetFont()->size);
	m_hbar->SetViewableContentSize(this->Width());

	RefreshCursorBounds();
}

void TextBoxCode::PostLayout(Skin::Base* skin)
{
}

void TextBoxCode::OnTextChanged()
{
	if (m_iCursorPos > TextLength()) { m_iCursorPos = TextLength(); }

	if (m_iCursorEndPos > TextLength()) { m_iCursorEndPos = TextLength(); }

	onTextChanged.Call(this);
}

void TextBoxCode::OnEnter()
{
	onReturnPressed.Call(this);
}

void TextBoxCode::MoveCaretToEnd()
{
	m_iCursorPos = TextLength();
	m_iCursorEndPos = TextLength();
	RefreshCursorBounds();
}

void TextBoxCode::MoveCaretToStart()
{
	m_iCursorPos = 0;
	m_iCursorEndPos = 0;
	m_iCursorLine = 0;
	m_iCursorEndLine = 0;
	RefreshCursorBounds();
}

void TextBoxCode::AddUndoableAction(Action a)
{
	action_list.push_back(a);

	if (action_list.size() > 100)
		action_list.pop_front();
}

bool TextBoxCode::OnKeyReturn(bool bDown)
{
	//todo, if the ac menu is showing add the selected item
	ac_menu->Hide();
	if (bDown)
	{
		this->AddUndoableAction({ this->m_iCursorLine, this->m_iCursorPos, 1, L"\n" });
		InsertText(L"\n");
	}

	return true;
}


#include <iostream>
#include <string>
#include <codecvt>
#include <locale>
void TextBoxCode::SetText(const char* text, unsigned int len)
{
	this->m_lines.push_back(Line());

	std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> converter;

	auto str = converter.from_bytes(text, text + len);

	auto ptr = str.c_str();
	//also need to indicate if file has changed in the tab name
	//need to convert to utf16

	if (len == 0)
		return;

	int i = 0;
	do
	{
		if (*ptr == '\n')
		{
			this->m_lines.push_back(Line());
		}
		else if (*ptr == '\r')
		{

		}
		else
		{
			this->m_lines.back().m_Unicode.push_back(*ptr);
		}
		i++;
		ptr++;
	} while (i < len);// *(ptr++) != 0);
	return;
}


int TextBoxCode::GetSidebarWidth()
{
	float fsize = this->GetFont()->size;
	float hoffset = fsize + 2;
	hoffset += bp_bar_size;//give some space for the side bar
	int num_lines = this->m_lines.size();
	//if (num_lines >= 10)
	hoffset += fsize*0.7;
	if (num_lines >= 100)
		hoffset += fsize*0.7;
	if (num_lines >= 1000)
		hoffset += fsize*0.7;
	return hoffset;
}

#include <regex>
#include <map>

std::wregex e(L"-?(?:\\d*\\.)?\\d+", std::regex_constants::optimize);
std::wregex num(L"\\b(for|do|if|else|while|case|break|return|fun|let|struct|extern|typedef|continue|union|match|default|this)");
std::wregex comment(L"\\/{2}.*");

std::map<WCHAR, std::vector<std::wstring>> keywords = {
	{ 'f', { L"for", L"fun", L"free" } },
	{ 'i', { L"if" } },
	{ 'd', { L"do", L"default" } },
	{ 'l', { L"let" } },
	{ 'm', { L"match" } },
	{ 's', { L"struct" } },
	{ 'c', { L"continue" } },
	{ 'b', { L"break" } },
	{ 'w', { L"while" } },
	{ 'u', { L"union" } },
	{ 'n', { L"namespace", L"new" } },
	{ 'e', { L"extern", L"else" } },
	{ 't', { L"this", L"typedef", L"trait" } },
	{ 'r', { L"return" } },
	{ '#', { L"#include", L"#define" } }
};

extern std::string ToLower(const std::string& str);

void TextBoxCode::Render(Skin::Base* skin)
{
	if (ShouldDrawBackground()) skin->DrawTextBox(this);

	//if (!HasFocus()) return;

	float fsize = this->GetFont()->size;

	float hoffset = this->GetSidebarWidth();

	this->SetPadding(Gwen::Padding(hoffset, 0, 0, 0));
	if (/*HasFocus() &&*/ (m_iCursorPos != m_iCursorEndPos || m_iCursorEndLine != m_iCursorLine))
	{
		int iCursorStartLine = m_iCursorLine;// m_Text->GetLineFromChar(m_iCursorPos);
		int iCursorEndLine = m_iCursorEndLine;// m_Text->GetLineFromChar(m_iCursorEnd);

		if (iCursorStartLine > m_lines.size() - 1) iCursorStartLine = m_Text->NumLines() - 1;
		if (iCursorEndLine > m_lines.size() - 1) iCursorEndLine = m_Text->NumLines() - 1;

		int iSelectionStartLine = (iCursorStartLine < iCursorEndLine) ? iCursorStartLine : iCursorEndLine;
		int iSelectionEndLine = (iCursorStartLine < iCursorEndLine) ? iCursorEndLine : iCursorStartLine;

		int iSelectionStartPos = (m_iCursorLine < m_iCursorEndLine) ? m_iCursorPos : m_iCursorEndPos;
		int iSelectionEndPos = (m_iCursorLine < m_iCursorEndLine) ? m_iCursorEndPos : m_iCursorPos;

		if (iSelectionStartLine == iSelectionEndLine)
		{
			iSelectionStartPos = (m_iCursorPos < m_iCursorEndPos) ? m_iCursorPos : m_iCursorEndPos;
			iSelectionEndPos = (m_iCursorEndPos < m_iCursorPos) ? m_iCursorPos : m_iCursorEndPos;
		}

		int iFirstChar = 0;
		int iLastChar = 0;
		skin->GetRender()->SetDrawColor(Gwen::Color(50, 170, 255, 200));
		m_rectSelectionBounds.h = m_Text->GetFont()->size + 2;

		float f_width = ((float)this->GetSkin()->GetRender()->MeasureText(this->GetFont(), "WWWWWWWWWW").x) / 10.0f;

		for (int iLine = iSelectionStartLine; iLine <= iSelectionEndLine; ++iLine)
		{
			Gwen::Rect box;
			box.w = this->Width();
			box.h = fsize;
			box.x = iSelectionStartPos*f_width;
			box.y = (iLine - m_scroll)*fsize + 2;

			if (iLine == iSelectionStartLine)
			{
				auto t = this->m_lines.begin();
				for (int i = 0; i < iSelectionStartLine; i++)
					t++;

				int off = 0;
				for (int i = 0; i < iSelectionStartPos; i++)
				{
					if (t->m_Unicode[i] == '\t')
						off = off + (4 - off % 4);
					else
						off += 1;
				}
				box.x = off*f_width;
			}
			else
			{
				//m_rectSelectionBounds.x = box.x;
				//m_rectSelectionBounds.y = box.y - 1;
				box.x = 0;// 4 + hoffset;
				//box.y 
			}
			//would also be good to update this outside of rendering, but whatever
			//todo: abstract the get text position at point function out then need to do a measure here
			if (iLine == iSelectionEndLine)
			{
				auto t = this->m_lines.begin();
				for (int i = 0; i < iSelectionEndLine; i++)
					t++;

				int off = 0;
				for (int i = 0; i < iSelectionEndPos; i++)
				{
					if (t->m_Unicode[i] == '\t')
						off = off + (4 - off % 4);
					else
						off += 1;
				}
				box.w = off*f_width - box.x;
			}
			else
			{
				box.w = this->Width();
				//m_rectSelectionBounds.w = box.x + box.w - m_rectSelectionBounds.x;
			}
			/*if (m_rectSelectionBounds.w < 1)
			{
			m_rectSelectionBounds.w = 1;
			}*/

			box.x += 4 + hoffset;// m_Text->X();
			box.y += m_Text->Y();

			skin->GetRender()->DrawFilledRect(box);// m_rectSelectionBounds);
		}
	}

	// Draw selection.. if selected..
	if (m_iCursorPos != m_iCursorEndPos)
	{
		skin->GetRender()->SetDrawColor(Gwen::Color(50, 170, 255, 200));
		skin->GetRender()->DrawFilledRect(m_rectSelectionBounds);
	}

	// Draw caret
	if (HasFocus())
	{
		skin->GetRender()->SetDrawColor(m_CaretColor);
		skin->GetRender()->DrawFilledRect(m_rectCaretBounds + Gwen::Rect(hoffset, -m_scroll*this->GetFont()->size, 0, 0));
	}

	//draw line numbers
	skin->GetRender()->SetDrawColor(Gwen::Color(100, 100, 100, 255));
	skin->GetRender()->DrawLinedRect(Gwen::Rect(/*this->GetPos().x*/ 0, /*this->GetPos().y*/ 0, hoffset, this->GetSize().y));



	if (m_lines.size() < m_scroll)
		m_scroll = m_lines.size();
	int numbers2draw = Min<int>((this->Height() - 2) / this->GetFont()->size, this->m_lines.size());
	auto line_iterator = this->m_lines.begin();
	auto prev_iterator = this->m_lines.begin();
	for (int i = 0; i < m_scroll; i++)
	{
		if (i == m_scroll - 1)
			prev_iterator = line_iterator;
		line_iterator++;
	}
	int pos = 0;

	skin->GetRender()->SetDrawColor(Gwen::Color(0, 0, 0, 255));

	enum Styles
	{
		None = 0,
		Keyword = 1,
		String = 2,
		Number = 2,
		Comment = 3,
	};
	const Gwen::Color styles[] = { Gwen::Color(0, 0, 0, 255), Gwen::Color(0, 0, 255, 255), Gwen::Color(255, 0, 0, 255), Gwen::Color(0, 153, 0, 255) };

	Gwen::UnicodeString str;
	str.push_back(0);
	float width = skin->GetRender()->MeasureText(GetFont(), "W").x;
	int max_chars = this->Width() / width + 2;
	for (int i = m_scroll; i < Min<int>(m_scroll + numbers2draw, this->m_lines.size()); i++)
	{
		//highlight the text, then we can render it below
		if (line_iterator->dirty)
		{
			if (i == 0)
				line_iterator->is_comment = false;
			else
				line_iterator->is_comment = prev_iterator->is_comment;

			line_iterator->styles.resize(line_iterator->m_Unicode.length());
			for (int i = 0; i < line_iterator->m_Unicode.length(); i++)
				line_iterator->styles[i] = Styles::None;

			//run regexes
			/*std::wsmatch m;
			std::wstring strr = line_iterator->m_Unicode;
			int offset = 0;

			while (std::regex_search(strr, m, e)) {
			auto p = m.position() + offset;
			int l = m.length();
			strr = m.suffix().str();
			for (int i = p; i < p + l; i++)
			line_iterator->styles[i] = 2;
			offset += m.position() + l;
			}

			strr = line_iterator->m_Unicode;
			offset = 0;
			while (std::regex_search(strr, m, num)) {
			auto p = m.position() + offset;
			int l = m.length();
			strr = m.suffix().str();
			for (int i = p; i < p + l; i++)
			line_iterator->styles[i] = 1;
			offset += m.position() + l;
			}
			//"[a-z\s]*"
			//line comments
			strr = line_iterator->m_Unicode;
			offset = 0;
			while (std::regex_search(strr, m, comment)) {
			auto p = m.position() + offset;
			int l = m.length();
			strr = m.suffix().str();
			for (int i = p; i < p + l; i++)
			line_iterator->styles[i] = 3;
			offset += m.position() + l;
			}*/
			//SETUP GIT FINALLY
			//add unsaved icon
			//	fix editing stuff
			//	more undo stuff
			//parse debug var info
			//need to do a pass looking for numbers
			bool last_was_letter = false;
			for (int i = 0; i < line_iterator->m_Unicode.length(); i++)
			{
				auto c = line_iterator->m_Unicode[i];
				if (c >= '0' && c <= '9' && last_was_letter == false)
					line_iterator->styles[i] = Styles::Number;
				if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
					last_was_letter = true;
				else
					last_was_letter = false;
			}

			//do a pass looking for keywords
			bool was_space = true;
			for (int i = 0; i < line_iterator->m_Unicode.length(); i++)
			{
				auto c = line_iterator->m_Unicode[i];

				if (was_space)
				{
					auto res = keywords.find(c);
					if (res != keywords.end())
					{
						//do a search
						for (auto kw : res->second)
						{
							if (line_iterator->m_Unicode.length() - i < kw.length())
								continue;

							if (memcmp(&line_iterator->m_Unicode.c_str()[i], kw.c_str(), kw.length() * 2) == 0)
							{
								for (int p = i; p < i + kw.length(); p++)
									line_iterator->styles[p] = Styles::Keyword;

								i += kw.length() - 1;
								break;
							}
						}
					}
				}

				if (c == '\t' || c == ' ' || c == '(')
					was_space = true;
				else
					was_space = false;
				//std::wregex num(L"\\b(for|do|if|else|while|case|break|return|fun|let|struct|extern|typedef|continue|union|match|default|this)");
			}

			//look for comments and strings
			bool in_line_comment = false;
			bool in_string = false;
			for (int i = 0; i < line_iterator->m_Unicode.length(); i++)
			{
				auto c = line_iterator->m_Unicode[i];
				if (c == '\\' && i + 1 < line_iterator->m_Unicode.length() && line_iterator->m_Unicode[i + 1] == '"')
				{
					line_iterator->styles[i] = Styles::String;
					i++;
				}
				else if (c == '\\' && i + 1 < line_iterator->m_Unicode.length() && line_iterator->m_Unicode[i + 1] == '\\')
				{
					line_iterator->styles[i] = Styles::String;
					i++;
				}
				else if (c == '"')
				{
					in_string = !in_string;
					line_iterator->styles[i] = Styles::String;
				}

				if (line_iterator->is_comment == false && c == '/' && i + 1 < line_iterator->m_Unicode.length() && line_iterator->m_Unicode[i + 1] == '/')
				{
					in_line_comment = true;
				}
				else if (in_line_comment == false && in_string == false && c == '/' && i + 1 < line_iterator->m_Unicode.length() && line_iterator->m_Unicode[i + 1] == '*')
				{
					line_iterator->styles[i++] = Styles::Comment;
					line_iterator->is_comment = true;
				}
				else if (line_iterator->is_comment && c == '*' && i + 1 < line_iterator->m_Unicode.length() && line_iterator->m_Unicode[i + 1] == '/')
				{
					line_iterator->styles[i] = Styles::Comment;
					line_iterator->styles[++i] = Styles::Comment;
					line_iterator->is_comment = false;
				}

				if (line_iterator->is_comment || in_line_comment)
					line_iterator->styles[i] = Styles::Comment;
				else if (in_string)
					line_iterator->styles[i] = Styles::String;
			}


			//check if next line
			auto next = line_iterator; next++;
			if (next != this->m_lines.end())
			{
				if (next->is_comment != line_iterator->is_comment)
				{
					next->is_comment = line_iterator->is_comment;
					next->dirty = true;
				}
			}
			line_iterator->dirty = false;
		}

		//draw each character
		
		int offset = 0;
		for (int i = 0; i < line_iterator->m_Unicode.length(); i++)
		{
			str[0] = line_iterator->m_Unicode[i];
			if (str[0] == '\t')
			{
				//then round to the next greater 4th space
				offset = offset + (4 - offset % 4);
				continue;
			}
			else if (str[0] == ' ')
			{
				offset++;
				continue;
			}

			float xoff = 4 + hoffset - this->m_hscroll + offset++*width;
			if (xoff < 0)
				continue;
			if (xoff > this->Width())
				break;
			skin->GetRender()->SetDrawColor(styles[line_iterator->styles[i]]);
			skin->GetRender()->RenderText(GetFont(), Gwen::PointF(xoff, 2 + pos*fsize), str);
		}
		//skin->GetRender()->RenderText(GetFont(), Gwen::PointF(4 + hoffset - this->m_hscroll, 2 + pos*fsize), line_iterator->m_Unicode);

		if (i != 0)
			prev_iterator++;
		line_iterator++;
		pos++;
	}

	skin->GetRender()->SetDrawColor(Gwen::Color(255, 255, 255, 255));
	skin->GetRender()->DrawFilledRect(Gwen::Rect(1, 1, hoffset - 2 - bp_bar_size, this->GetSize().y - 2));

	//draw line for breakpoints
	skin->GetRender()->SetDrawColor(Gwen::Color(150, 150, 150, 255));
	skin->GetRender()->DrawFilledRect(Gwen::Rect(/*this->GetPos().x*/ hoffset - bp_bar_size - 1, /*this->GetPos().y*/ 0, bp_bar_size, this->GetSize().y));

	skin->GetRender()->SetDrawColor(Gwen::Color(0, 0, 0, 255));

	//draw breakpoints and the like
	skin->GetRender()->SetDrawColor(Gwen::Color(255, 0, 0, 255));
	//draw breakpoints
	if (this->breakpoints)
	{
		for (int i = 0; i < this->breakpoints->size(); i++)
		{
			//check if its in range
			if (this->breakpoints->at(i).line < m_scroll || this->breakpoints->at(i).line > Min<int>(m_scroll + numbers2draw, this->m_lines.size()))
				continue;

			if (ToLower(this->breakpoints->at(i).file) != ToLower(this->filename))
				continue;

			int line = this->breakpoints->at(i).line - 1;
			skin->GetRender()->SetDrawColor(Gwen::Color(255, 0, 0, 255));

			skin->GetRender()->RenderText(&this->icon_font, Gwen::PointF(hoffset - bp_bar_size - 1, (line - m_scroll)*fsize - 2), L"\u270B");//\u21e8");
		}
	}

	//draw breakpoints and the like
	skin->GetRender()->SetDrawColor(Gwen::Color(255, 255, 255, 255));
	for (int i = 0; i < this->line_indicators.size(); i++)
	{
		int line = line_indicators[i].line;

		//if (line_indicators[i].type == 1)
		//	skin->GetRender()->RenderText(&this->icon_font, Gwen::Point(hoffset - bp_bar_size - 1, (line - m_scroll)*fsize - 2), L"\u270B");//\u21e8");
		//else
		skin->GetRender()->RenderText(&this->icon_font, Gwen::PointF(hoffset - bp_bar_size - 1, (line - m_scroll)*fsize - 2), L"\u27A5");//\u21e8");
	}


	//draw line numbers
	skin->GetRender()->SetDrawColor(Gwen::Color(0, 0, 0, 255));

	pos = 0;
	for (int i = m_scroll; i < Min<int>(m_scroll + numbers2draw, this->m_lines.size()); i++)
		skin->GetRender()->RenderText(GetFont(), Gwen::PointF(4, 2 + pos++*fsize), std::to_string(i + 1));// m_String.GetUnicode());
}

void TextBoxCode::MakeCaratVisible()
{
	if (m_Text->Height() < Height())
	{
		m_Text->Position(m_iAlign);
	}
	else
	{
		//const Rect& bounds = GetInnerBounds();

		//if ( pos & Pos::Top ) y = bounds.y + ypadding;
		//if ( pos & Pos::Bottom ) y = bounds.y + ( bounds.h - Height() - ypadding );
		//if ( pos & Pos::CenterV ) y = bounds.y + ( bounds.h - Height() )  * 0.5;

		Rect pos = m_Text->GetCharacterPosition(m_iCursorPos);
		int iCaratPos = pos.y;// + pos.h;
		int iRealCaratPos = iCaratPos + m_Text->Y();
		//int iSlidingZone =  m_Text->GetFont()->size; //Width()*0.1f

		// If the carat is already in a semi-good position, leave it.
		int mi = GetPadding().top;
		int ma = Height() - pos.h - GetPadding().bottom;
		if (iRealCaratPos >= GetPadding().top && iRealCaratPos <= Height() - pos.h - GetPadding().bottom)
			return;

		int y = 0;

		// bottom of carat too low
		if (iRealCaratPos > Height() - pos.h - GetPadding().bottom)
		{
			//align bottom
			y = Height() - iCaratPos - pos.h - GetPadding().bottom;
		}

		// top of carat too low
		if (iRealCaratPos < GetPadding().top)
		{
			y = -iCaratPos + GetPadding().top;
		}

		// Don't show too much whitespace to the bottom
		if (y + m_Text->Height() < Height() - GetPadding().bottom)
			y = -m_Text->Height() + (Height() - GetPadding().bottom);

		// Or the top
		if (y > GetPadding().top)
			y = GetPadding().top;

		int x = 0;
		if (m_iAlign & Pos::Left) x = GetPadding().left;
		if (m_iAlign & Pos::Right) x = Width() - m_Text->Width() - GetPadding().right;
		if (m_iAlign & Pos::CenterH) x = (Width() - m_Text->Width()) * 0.5;

		m_Text->SetPos(x, y);
	}
}

int TextBoxCode::GetCurrentLine()
{
	return this->m_iCursorLine;
}

int TextBoxCode::GetCurrentColumn()
{
	return this->m_iCursorPos;
}

bool TextBoxCode::OnKeyHome(bool bDown)
{
	if (!bDown) { return true; }

	if (Gwen::Input::IsControlDown())
	{
		m_iCursorLine = 0;
		m_iCursorPos = 0;
		RefreshCursorBounds();
		return true;
	}

	//implement me
	int iCurrentLine = GetCurrentLine();
	int iChar = 0;// m_Text->GetStartCharFromLine(iCurrentLine);
	//m_iCursorLine = 0;
	m_iCursorPos = iChar;

	if (!Gwen::Input::IsShiftDown())
	{
		m_iCursorEndPos = m_iCursorPos;
		m_iCursorEndLine = m_iCursorLine;
	}

	RefreshCursorBounds();
	return true;
}

bool TextBoxCode::OnKeyTab(bool bDown)
{
	if (bDown)
	{
		//todo make this handle when we have a selection
		this->AddUndoableAction({ this->m_iCursorLine, this->m_iCursorPos, 1, L"\t" });
		this->InsertText(L"\t");

		RefreshCursorBounds();
	}

	return true;
}

bool TextBoxCode::OnKeyEnd(bool bDown)
{
	if (!bDown) { return true; }

	if (Gwen::Input::IsControlDown())
	{
		m_iCursorLine = this->m_lines.size();
		m_iCursorPos = 10000000;
		RefreshCursorBounds();
		return true;
	}

	//auto t = this->m_lines.begin();
	//for (int i = 0; i < line; i++)
	//	t++;

	//also implement me
	int iCurrentLine = GetCurrentLine();
	int iChar = 1000000;// m_Text->GetEndCharFromLine(iCurrentLine);
	//m_iCursorLine = 0;
	m_iCursorPos = iChar;

	int iLastLine = m_Text->NumLines() - 1;

	//if (iCurrentLine < iLastLine && iChar > 0)
	//	m_iCursorPos = iChar - 1; // NAUGHTY
	//else
	//	m_iCursorPos = m_Text->Length();

	if (!Gwen::Input::IsShiftDown())
	{
		m_iCursorEndPos = m_iCursorPos;
		m_iCursorEndLine = m_iCursorLine;
	}

	RefreshCursorBounds();
	return true;
}

bool TextBoxCode::OnKeyUp(bool bDown)
{
	if (!bDown) { return true; }

	//if ( m_iCursorLine == 0 )
	//m_iCursorPo
	//m_iCursorLine = m_Text->GetCharPosOnLine(m_iCursorPos);

	//int iLine = m_Text->GetLineFromChar(m_iCursorPos);
	if (this->m_iCursorLine < this->m_scroll)
		this->m_scroll--;

	if (m_iCursorLine == 0) { return true; }

	m_iCursorLine--;

	auto line = this->m_lines.begin();
	for (int i = 0; i < m_iCursorLine; i++)
		line++;
	if (m_iCursorPos > line->m_Unicode.length())
		m_iCursorPos = line->m_Unicode.length();
	/*m_iCursorPos = m_Text->GetStartCharFromLine(iLine - 1);
	m_iCursorPos += Clamp(m_iCursorLine, 0, m_Text->GetLine(iLine - 1)->Length() - 1);
	m_iCursorPos = Clamp(m_iCursorPos, 0, m_Text->Length());*/

	if (!Gwen::Input::IsShiftDown())
	{
		m_iCursorEndPos = m_iCursorPos;
		m_iCursorEndLine = m_iCursorLine;
	}



	RefreshCursorBounds();
	return true;
}

bool TextBoxCode::OnKeyDown(bool bDown)
{
	if (!bDown) { return true; }

	//if ( m_iCursorLine == 0 )
	/*m_iCursorLine = m_Text->GetCharPosOnLine(m_iCursorPos);

	int iLine = m_Text->GetLineFromChar(m_iCursorPos);
	int iLastLine = m_Text->NumLines() - 1;
	if (iLine >= iLastLine || iLastLine < 1) return true;

	m_iCursorPos = m_Text->GetStartCharFromLine(iLine + 1);
	if (iLine + 1 >= iLastLine)
	{
	m_iCursorPos += Clamp(m_iCursorLine, 0, m_Text->GetLine(iLine + 1)->Length());
	}
	else
	{
	m_iCursorPos += Clamp(m_iCursorLine, 0, m_Text->GetLine(iLine + 1)->Length() - 1);
	}
	m_iCursorPos = Clamp(m_iCursorPos, 0, m_Text->Length());*/
	if (m_iCursorLine - this->m_scroll + 1 >= ((this->Height() - 2) / this->GetFont()->size))
		this->m_scroll++;


	if (m_iCursorLine >= this->m_lines.size() - 1)
		return true;

	m_iCursorLine++;

	auto line = this->m_lines.begin();
	for (int i = 0; i < m_iCursorLine; i++)
		line++;
	if (m_iCursorPos > line->m_Unicode.length())
		m_iCursorPos = line->m_Unicode.length();

	if (!Gwen::Input::IsShiftDown())
	{
		m_iCursorEndPos = m_iCursorPos;
		m_iCursorEndLine = m_iCursorLine;
	}


	RefreshCursorBounds();
	return true;
}

#include <fstream>
void TextBoxCode::SaveToFile(const Gwen::String& filename)
{
	this->modified = false;

	std::ofstream f(filename, std::ios::binary);
	int line_num = 0;
	for (auto line : m_lines)
	{
		//f.write((char*)line.m_Unicode.c_str(), line.m_Unicode.size()*2);

		int utf8len = WideCharToMultiByte(CP_UTF8, 0, line.m_Unicode.data(), line.m_Unicode.length(), 0, 0, 0, 0);

		char* data = new char[utf8len];
		//todo decode
		//ok, need to optimize this allocation wise. This is silly.
		WideCharToMultiByte(CP_UTF8, 0, line.m_Unicode.data(), line.m_Unicode.length(), data, utf8len, 0, 0);

		f.write(data, utf8len);

		delete[] data;

		if (line_num++ != (m_lines.size() - 1))
			f.put('\n');
	}
}