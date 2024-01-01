/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2024 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "editing.h"
#include "encoding.h"
#include "image.h"
#include "project.h"
#include "theme.h"
#include "workspace.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "../lib/imgui/imgui_internal.h"
#include "../lib/lz4/lib/lz4.h"
#include <SDL.h>

/*
** {===========================================================================
** Utilities
*/

static const char* editingTextOffset(const char* code, int ln, int col) {
	// To line.
	if (ln > 0) {
		while (*code) {
			const char ch = *code;
			if (ch == '\n') {
				if (--ln == 0) {
					++code;

					break;
				}
			}
			++code;
		}
	}

	// To column.
	for (int i = 0; i < col; ++i) {
		if (*code == '\0')
			break;

		const int n = Unicode::expectUtf8(code);
		code += n;
	}

	return code;
}
static const char* editingTextFindForward(const char* txt, const char* const what, int ln, int col, int* lnoff, int* coloff) {
	if (lnoff)
		*lnoff = 0;
	if (coloff)
		*coloff = 0;
	const char* off = editingTextOffset(txt, ln, col);

	const char* mat = strstr(off, what);
	if (mat) { // Found.
		const char* str = off;
		while (str < mat) {
			if (*str == '\0')
				break;
			if (*str == '\n') {
				++(*lnoff);
				*coloff = 0;
				++str;
			} else {
				const int n = Unicode::expectUtf8(str);
				++(*coloff);
				str += n;
			}
		}
	}

	return mat;
}
static const char* editingTextFindBackward(const char* txt, const char* const what, int ln, int col, int* lnoff, int* coloff) {
	if (lnoff)
		*lnoff = 0;
	if (coloff)
		*coloff = 0;
	const char* off = editingTextOffset(txt, ln, col);

	std::string stdcode;
	stdcode.assign(txt, off);
	size_t mat = stdcode.rfind(what, stdcode.length() - 1);

	if (mat != std::string::npos) { // Found.
		const char* str = txt;
		while (str < txt + mat) {
			if (*str == '\0')
				break;
			if (*str == '\n') {
				++(*lnoff);
				*coloff = 0;
				++str;
			} else {
				const int n = Unicode::expectUtf8(str);
				++(*coloff);
				str += n;
			}
		}

		return txt + mat;
	}

	return nullptr;
}

/* ===========================================================================} */

/*
** {===========================================================================
** Editing
*/

namespace Editing {

Shortcut::Shortcut(
	int key_,
	bool ctrl_, bool shift_, bool alt_,
	bool numLock_, bool capsLock_,
	bool super_
) : key(key_),
	ctrl(ctrl_), shift(shift_), alt(alt_),
	numLock(numLock_), capsLock(capsLock_),
	super(super_)
{
}

bool Shortcut::pressed(bool repeat) const {
	ImGuiIO &io = ImGui::GetIO();

	const SDL_Keymod mod = SDL_GetModState();
	const bool num = !!(mod & KMOD_NUM);
	const bool caps = !!(mod & KMOD_CAPS);

	if (!key && !ctrl && !shift && !alt && !numLock && !capsLock && !super)
		return false;

	if (key && !ImGui::IsKeyPressed(key, repeat))
		return false;

	if (ctrl && !io.KeyCtrl)
		return false;
	else if (!ctrl && io.KeyCtrl)
		return false;

	if (shift && !io.KeyShift)
		return false;
	else if (!shift && io.KeyShift)
		return false;

	if (alt && !io.KeyAlt)
		return false;
	else if (!alt && io.KeyAlt)
		return false;

	if (numLock && !num)
		return false;

	if (capsLock && !caps)
		return false;

	if (super && !io.KeySuper)
		return false;
	else if (!super && io.KeySuper)
		return false;

	return true;
}

bool Shortcut::released(void) const {
	ImGuiIO &io = ImGui::GetIO();

	const SDL_Keymod mod = SDL_GetModState();
	const bool num = !!(mod & KMOD_NUM);
	const bool caps = !!(mod & KMOD_CAPS);

	if (!key && !ctrl && !shift && !alt && !numLock && !capsLock && !super)
		return false;

	if (key && ImGui::IsKeyReleased(key))
		return true;

	if (ctrl && !io.KeyCtrl)
		return true;
	else if (!ctrl && io.KeyCtrl)
		return true;

	if (shift && !io.KeyShift)
		return true;
	else if (!shift && io.KeyShift)
		return true;

	if (alt && !io.KeyAlt)
		return true;
	else if (!alt && io.KeyAlt)
		return true;

	if (numLock && !num)
		return true;

	if (capsLock && !caps)
		return true;

	if (super && !io.KeySuper)
		return true;
	else if (!super && io.KeySuper)
		return true;

	return false;
}

namespace Tools {

Marker::Coordinates::Coordinates() {
}

Marker::Coordinates::Coordinates(int ln, int col) : line(ln), column(col) {
}

Marker::Coordinates::Coordinates(int idx, int ln, int col) : index(idx), line(ln), column(col) {
}

bool Marker::Coordinates::operator == (const Coordinates &other) const {
	return compare(other) == 0;
}

bool Marker::Coordinates::operator < (const Coordinates &other) const {
	return compare(other) < 0;
}

bool Marker::Coordinates::operator > (const Coordinates &other) const {
	return compare(other) > 0;
}

int Marker::Coordinates::compare(const Coordinates &other) const {
	if (index < other.index)
		return -1;
	else if (index > other.index)
		return 1;

	if (line < other.line)
		return -1;
	else if (line > other.line)
		return 1;

	if (column < other.column)
		return -1;
	else if (column > other.column)
		return 1;

	return 0;
}

bool Marker::Coordinates::empty(void) const {
	return index == -1 && line == -1 && column == -1;
}

void Marker::Coordinates::clear(void) {
	index = -1;
	line = -1;
	column = -1;
}

Marker::Marker() {
}

Marker::Marker(const Coordinates &begin_, const Coordinates &end_) : begin(begin_), end(end_) {
}

const Marker::Coordinates &Marker::min(void) const {
	if (begin < end)
		return begin;

	return end;
}

const Marker::Coordinates &Marker::max(void) const {
	if (begin > end)
		return begin;

	return end;
}

bool Marker::empty(void) const {
	return begin.empty() || end.empty();
}

void Marker::clear(void) {
	begin.clear();
	end.clear();
}

bool jump(
	Renderer* rnd,
	Workspace* ws,
	int* cursor,
	float width,
	bool* initialized, bool* focused,
	int min, int max
) {
	ImGuiIO &io = ImGui::GetIO();
	ImGuiStyle &style = ImGui::GetStyle();

	bool result = false;

	if (focused)
		*focused = false;

	if (!cursor)
		return result;

	const ImVec2 buttonSize(13 * io.FontGlobalScale, 13 * io.FontGlobalScale);

	const float x = ImGui::GetCursorPosX();
	ImGui::Dummy(ImVec2(8, 0));
	ImGui::SameLine();
	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted(ws->theme()->dialogItem_Goto());

	ImGui::PushID("@Jmp");
	do {
		ImGui::SameLine();
		if (!*initialized) {
			ImGui::SetKeyboardFocusHere();
			*initialized = true;
		}
		ImGui::SetNextItemWidth(width - (ImGui::GetCursorPosX() - x) - (buttonSize.x + style.FramePadding.x * 2) * 2);
		char buf[8];
		snprintf(buf, BITTY_COUNTOF(buf), "%d", *cursor + 1);
		const bool edited = ImGui::InputText("", buf, sizeof(buf), ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_AutoSelectAll);
		if (ImGui::GetActiveID() == ImGui::GetID("")) {
			if (focused)
				*focused = true;
		}
		if (!edited)
			break;
		int ln = 0;
		if (!Text::fromString(buf, ln))
			break;
		--ln;
		if (min >= 0 && max >= 0 && (ln < min || ln > max))
			break;

		result = true;

		*cursor = ln;
	} while (false);
	ImGui::PopID();

	ImGui::SameLine();
	if (ImGui::ImageButton(ws->theme()->slicePrevious()->pointer(rnd), buttonSize, ImGui::ColorConvertU32ToFloat4(ws->theme()->style()->iconColor))) {
		result = true;

		--*cursor;
		if (min >= 0 && *cursor < min)
			*cursor = min;
	}

	ImGui::SameLine();
	if (ImGui::ImageButton(ws->theme()->sliceNext()->pointer(rnd), buttonSize, ImGui::ColorConvertU32ToFloat4(ws->theme()->style()->iconColor))) {
		result = true;

		++*cursor;
		if (max >= 0 && *cursor > max)
			*cursor = max;
	}

	return result;
}

bool find(
	Renderer* rnd,
	Workspace* ws,
	Marker* cursor,
	float width,
	bool* initialized, bool* focused,
	const char* text, std::string* what,
	const Marker::Coordinates &max,
	int* direction,
	bool* caseSensitive, bool* wholeWord,
	bool visible,
	TextWordGetter getWord
) {
	ImGuiIO &io = ImGui::GetIO();
	ImGuiStyle &style = ImGui::GetStyle();

	bool result = false;

	if (focused)
		*focused = false;

	if (!cursor || !what)
		return result;

	int step = 0;
	if (direction) {
		step = *direction;
		*direction = 0;
	}

	if (visible) {
		const ImVec2 buttonSize(13 * io.FontGlobalScale, 13 * io.FontGlobalScale);

		const float x = ImGui::GetCursorPosX();
		ImGui::Dummy(ImVec2(8, 0));
		ImGui::SameLine();
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(ws->theme()->dialogItem_Find());

		ImGui::PushID("@Fnd");
		do {
			ImGui::SameLine();
			if (!*initialized) {
				ImGui::SetKeyboardFocusHere();
				*initialized = true;
			}
			ImGui::SetNextItemWidth(width - (ImGui::GetCursorPosX() - x) - (buttonSize.x + style.FramePadding.x * 2) * 4);
			char buf[256]; // Fixed size.
			const size_t n = std::min(BITTY_COUNTOF(buf) - 1, what->length());
			if (n > 0)
				memcpy(buf, what->c_str(), n);
			buf[n] = '\0';
			const ImGuiInputTextFlags flags = ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackAlways | ImGuiInputTextFlags_AllowTabInput;
			const bool edited = ImGui::InputText(
				"", buf, sizeof(buf),
				flags,
				[] (ImGuiInputTextCallbackData* data) -> int {
					std::string* what = (std::string*)data->UserData;
					what->assign(data->Buf, data->BufTextLen);

					return 0;
				},
				what
			);
			if (ImGui::GetActiveID() == ImGui::GetID("")) {
				if (focused)
					*focused = true;
			}
			if (!edited)
				break;

			step = 1;

			*what = buf;
		} while (false);
		ImGui::PopID();

		ImGui::SameLine();
		if (ImGui::ImageButton(ws->theme()->sliceCaseSensitive()->pointer(rnd), buttonSize, ImGui::ColorConvertU32ToFloat4(ws->theme()->style()->iconColor), caseSensitive ? *caseSensitive : false)) {
			if (caseSensitive)
				*caseSensitive = !*caseSensitive;
		}
		if (ImGui::IsItemHovered()) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			ImGui::SetTooltip(ws->theme()->tooltipEditing_CaseSensitive());
		}

		ImGui::SameLine();
		if (ImGui::ImageButton(ws->theme()->sliceWholeWord()->pointer(rnd), buttonSize, ImGui::ColorConvertU32ToFloat4(ws->theme()->style()->iconColor), wholeWord ? *wholeWord : false)) {
			if (wholeWord)
				*wholeWord = !*wholeWord;
		}
		if (ImGui::IsItemHovered()) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			ImGui::SetTooltip(ws->theme()->tooltipEditing_MatchWholeWords());
		}

		ImGui::SameLine();
		if (ImGui::ImageButton(ws->theme()->slicePrevious()->pointer(rnd), buttonSize, ImGui::ColorConvertU32ToFloat4(ws->theme()->style()->iconColor))) {
			if (!what->empty())
				step = -1;
		}

		ImGui::SameLine();
		if (ImGui::ImageButton(ws->theme()->sliceNext()->pointer(rnd), buttonSize, ImGui::ColorConvertU32ToFloat4(ws->theme()->style()->iconColor))) {
			if (!what->empty())
				step = 1;
		}
	}

	auto fill = [] (Marker* cursor, const Marker::Coordinates &nbegin, const Marker::Coordinates &nend, TextWordGetter getWord) -> bool {
		if (getWord) {
			Marker src;
			getWord(nbegin, src);
			if ((src.begin == nbegin && src.end == nend) || (src.begin == nend && src.end == nbegin)) {
				cursor->begin = nbegin;
				cursor->end = nend;

				return true;
			}
		} else {
			cursor->begin = nbegin;
			cursor->end = nend;

			return true;
		}

		return false;
	};

	if (step && what->empty())
		step = 0;
	if (step == 1) {
		std::string pat = *what;
		std::string tmp;
		if (!caseSensitive || !*caseSensitive) {
			Text::toLowerCase(pat);

			tmp = text;
			Text::toLowerCase(tmp);
			text = tmp.c_str();
		}
		const std::wstring wide = Unicode::toWide(what->c_str());
		int lnoff = 0, coloff = 0;
		const char* mat = editingTextFindForward(text, pat.c_str(), cursor->max().line, cursor->max().column, &lnoff, &coloff);
		if (mat) { // Found.
			Marker::Coordinates nbegin(cursor->begin.line + lnoff, coloff);
			Marker::Coordinates nend(cursor->begin.line + lnoff, coloff + (int)wide.length());
			if (lnoff == 0) {
				nbegin.column += cursor->max().column;
				nend.column += cursor->max().column;
			}

			result = fill(cursor, nbegin, nend, wholeWord && *wholeWord ? getWord : nullptr);
		} else { // Not found.
			// Find again from the beginning.
			mat = editingTextFindForward(text, pat.c_str(), 0, 0, &lnoff, &coloff);
			if (mat) { // Found.
				const Marker::Coordinates nbegin(lnoff, coloff);
				const Marker::Coordinates nend(lnoff, coloff + (int)wide.length());

				result = fill(cursor, nbegin, nend, wholeWord && *wholeWord ? getWord : nullptr);
			}
		}
	} else if (step == -1) {
		Marker::Coordinates pos = cursor->min();
		std::string pat = *what;
		std::string tmp;
		if (!caseSensitive || !*caseSensitive) {
			Text::toLowerCase(pat);

			tmp = text;
			Text::toLowerCase(tmp);
			text = tmp.c_str();
		}
		std::wstring wide = Unicode::toWide(what->c_str());
		int lnoff = 0, coloff = 0;
		const char* mat = editingTextFindBackward(text, pat.c_str(), pos.line, pos.column, &lnoff, &coloff);
		if (mat) { // Found.
			const Marker::Coordinates nbegin(lnoff, coloff);
			const Marker::Coordinates nend(lnoff, coloff + (int)wide.length());

			result = fill(cursor, nbegin, nend, wholeWord && *wholeWord ? getWord : nullptr);
		} else if (!max.empty()) { // Not found.
			// Find again from the end.
			pos = max;
			mat = editingTextFindBackward(text, pat.c_str(), pos.line, pos.column, &lnoff, &coloff);
			if (mat) { // Found.
				const Marker::Coordinates nbegin(lnoff, coloff);
				const Marker::Coordinates nend(lnoff, coloff + (int)wide.length());

				result = fill(cursor, nbegin, nend, wholeWord && *wholeWord ? getWord : nullptr);
			}
		}
	}

	return result;
}

bool find(
	Renderer* rnd,
	Workspace* ws,
	Marker* cursor,
	float width,
	bool* initialized, bool* focused,
	const TextPages* textPages, std::string* what,
	const Marker::Coordinates &max,
	int* direction,
	bool* caseSensitive, bool* wholeWord, bool* globalSearch,
	bool visible,
	TextWordGetter getWord
) {
	typedef std::function<void(int &)> FindViewHandler;

	auto findOne = [] (
		Renderer* rnd,
		Workspace* ws,
		Marker* cursor,
		bool* focused,
		const TextPages* textPages, std::string* what,
		const Marker::Coordinates &max,
		int* direction,
		bool* caseSensitive, bool* wholeWord, bool* globalSearch,
		FindViewHandler findView,
		TextWordGetter getWord
	) -> bool {
		(void)rnd;
		(void)ws;

		bool result = false;

		if (focused)
			*focused = false;

		if (!cursor || !what)
			return result;

		int step = 0;
		if (direction) {
			step = *direction;
			*direction = 0;
		}

		findView(step);

		auto at = [textPages, caseSensitive] (int page, std::string &cache) -> const std::string* {
			const std::string* txt = (*textPages)[page];
			if (!txt)
				return nullptr;

			if (!caseSensitive || !*caseSensitive) {
				cache = *txt;
				Text::toLowerCase(cache);
				txt = &cache;
			}

			return txt;
		};
		auto fill = [] (Marker* cursor, const Marker::Coordinates &nbegin, const Marker::Coordinates &nend, TextWordGetter getWord) -> bool {
			if (getWord) {
				Marker src;
				getWord(nbegin, src);
				if ((src.begin == nbegin && src.end == nend) || (src.begin == nend && src.end == nbegin)) {
					cursor->begin = nbegin;
					cursor->end = nend;

					return true;
				}
			} else {
				cursor->begin = nbegin;
				cursor->end = nend;

				return true;
			}

			return false;
		};

		if (step && what->empty())
			step = 0;
		if (step == 1) { // Forward.
			std::string pat = *what;
			if (!caseSensitive || !*caseSensitive) {
				Text::toLowerCase(pat);
			}
			const std::wstring widepat = Unicode::toWide(what->c_str());
			int page = cursor->begin.index;
			std::string tmp;
			const std::string* text = at(page, tmp);
			int lnoff = 0, coloff = 0;
			const char* mat = editingTextFindForward(text->c_str(), pat.c_str(), cursor->max().line, cursor->max().column, &lnoff, &coloff);
			if (mat) { // Found.
				Marker::Coordinates nbegin(page, cursor->begin.line + lnoff, coloff);
				Marker::Coordinates nend(page, cursor->begin.line + lnoff, coloff + (int)widepat.length());
				if (lnoff == 0) {
					nbegin.column += cursor->max().column;
					nend.column += cursor->max().column;
				}

				result = fill(cursor, nbegin, nend, wholeWord && *wholeWord ? getWord : nullptr);
			} else { // Not found.
				// Open next page for global search.
				if (*globalSearch && textPages->size() > 1) {
					if (++page >= (int)textPages->size())
						page = 0;
					text = at(page, tmp);
					while (text == nullptr) {
						if (++page >= (int)textPages->size())
							page = 0;
						text = at(page, tmp);
					}
				}

				// Find again from the beginning.
			_forwardAgain:
				mat = editingTextFindForward(text->c_str(), pat.c_str(), 0, 0, &lnoff, &coloff);
				if (mat) { // Found.
					const Marker::Coordinates nbegin(page, lnoff, coloff);
					const Marker::Coordinates nend(page, lnoff, coloff + (int)widepat.length());

					result = fill(cursor, nbegin, nend, wholeWord && *wholeWord ? getWord : nullptr);
				} else { // Not found.
					if (page != cursor->begin.index) {
						page = cursor->begin.index;
						text = at(page, tmp);

						goto _forwardAgain;
					}
				}
			}
		} else if (step == -1) { // Backward.
			Marker::Coordinates pos = cursor->min();
			std::string pat = *what;
			if (!caseSensitive || !*caseSensitive) {
				Text::toLowerCase(pat);
			}
			std::wstring widepat = Unicode::toWide(what->c_str());
			int page = cursor->begin.index;
			std::string tmp;
			const std::string* text = at(page, tmp);
			int lnoff = 0, coloff = 0;
			const char* mat = editingTextFindBackward(text->c_str(), pat.c_str(), pos.line, pos.column, &lnoff, &coloff);
			if (mat) { // Found.
				const Marker::Coordinates nbegin(page, lnoff, coloff);
				const Marker::Coordinates nend(page, lnoff, coloff + (int)widepat.length());

				result = fill(cursor, nbegin, nend, wholeWord && *wholeWord ? getWord : nullptr);
			} else if (!max.empty()) { // Not found.
				// Open previous page for global search.
				if (*globalSearch && textPages->size() > 1) {
					if (--page < 0)
						page = (int)textPages->size() - 1;
					text = at(page, tmp);
					while (text == nullptr) {
						if (--page < 0)
							page = (int)textPages->size() - 1;;
						text = at(page, tmp);
					}
				}

				// Find again from the end.
				pos = max;
			_backwardAgain:
				mat = editingTextFindBackward(text->c_str(), pat.c_str(), pos.line, pos.column, &lnoff, &coloff);
				if (mat) { // Found.
					const Marker::Coordinates nbegin(page, lnoff, coloff);
					const Marker::Coordinates nend(page, lnoff, coloff + (int)widepat.length());

					result = fill(cursor, nbegin, nend, wholeWord && *wholeWord ? getWord : nullptr);
				} else { // Not found.
					if (page != cursor->begin.index) {
						page = cursor->begin.index;
						text = at(page, tmp);

						goto _backwardAgain;
					}
				}
			}
		} else /* if (step == 0) */ { // Error.
			// Do nothing.
		}

		return result;
	};

	ImGuiIO &io = ImGui::GetIO();
	ImGuiStyle &style = ImGui::GetStyle();

	auto findView = [&] (int &step) -> void {
		if (!visible)
			return;

		const ImVec2 buttonSize(13 * io.FontGlobalScale, 13 * io.FontGlobalScale);

		const float x = ImGui::GetCursorPosX();
		ImGui::Dummy(ImVec2(8, 0));
		ImGui::SameLine();
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(ws->theme()->dialogItem_Find());

		ImGui::PushID("@Fnd");
		do {
			ImGui::SameLine();
			if (!*initialized) {
				ImGui::SetKeyboardFocusHere();
				*initialized = true;
			}
			ImGui::SetNextItemWidth(width - (ImGui::GetCursorPosX() - x) - (buttonSize.x + style.FramePadding.x * 2) * 5);
			char buf[256]; // Fixed size.
			const size_t n = std::min(BITTY_COUNTOF(buf) - 1, what->length());
			if (n > 0)
				memcpy(buf, what->c_str(), n);
			buf[n] = '\0';
			const ImGuiInputTextFlags flags = ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackAlways | ImGuiInputTextFlags_AllowTabInput;
			const bool edited = ImGui::InputText(
				"", buf, sizeof(buf),
				flags,
				[] (ImGuiInputTextCallbackData* data) -> int {
					std::string* what = (std::string*)data->UserData;
					what->assign(data->Buf, data->BufTextLen);

					return 0;
				},
				what
			);
			if (ImGui::GetActiveID() == ImGui::GetID("")) {
				if (focused)
					*focused = true;
			}
			if (!edited)
				break;

			step = 1;

			*what = buf;
		} while (false);
		ImGui::PopID();

		ImGui::SameLine();
		if (ImGui::ImageButton(ws->theme()->sliceCaseSensitive()->pointer(rnd), buttonSize, ImGui::ColorConvertU32ToFloat4(ws->theme()->style()->iconColor), caseSensitive ? *caseSensitive : false)) {
			if (caseSensitive)
				*caseSensitive = !*caseSensitive;
		}
		if (ImGui::IsItemHovered()) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			ImGui::SetTooltip(ws->theme()->tooltipEditing_CaseSensitive());
		}

		ImGui::SameLine();
		if (ImGui::ImageButton(ws->theme()->sliceWholeWord()->pointer(rnd), buttonSize, ImGui::ColorConvertU32ToFloat4(ws->theme()->style()->iconColor), wholeWord ? *wholeWord : false)) {
			if (wholeWord)
				*wholeWord = !*wholeWord;
		}
		if (ImGui::IsItemHovered()) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			ImGui::SetTooltip(ws->theme()->tooltipEditing_MatchWholeWords());
		}

		ImGui::SameLine();
		if (ImGui::ImageButton(ws->theme()->sliceGlobal()->pointer(rnd), buttonSize, ImGui::ColorConvertU32ToFloat4(ws->theme()->style()->iconColor), globalSearch ? *globalSearch : false)) {
			if (globalSearch)
				*globalSearch = !*globalSearch;
		}
		if (ImGui::IsItemHovered()) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			ImGui::SetTooltip(ws->theme()->tooltipEditing_GlobalSearchForCode());
		}

		ImGui::SameLine();
		if (ImGui::ImageButton(ws->theme()->slicePrevious()->pointer(rnd), buttonSize, ImGui::ColorConvertU32ToFloat4(ws->theme()->style()->iconColor))) {
			if (!what->empty())
				step = -1;
		}

		ImGui::SameLine();
		if (ImGui::ImageButton(ws->theme()->sliceNext()->pointer(rnd), buttonSize, ImGui::ColorConvertU32ToFloat4(ws->theme()->style()->iconColor))) {
			if (!what->empty())
				step = 1;
		}
	};

	const bool result = findOne(
		rnd,
		ws,
		cursor,
		focused,
		textPages, what,
		max,
		direction,
		caseSensitive, wholeWord, globalSearch,
		findView,
		getWord
	);

	return result;
}

}

namespace Data {

Checkpoint::Checkpoint() {
}

bool Checkpoint::empty(void) const {
	return !bytes;
}

void Checkpoint::fill(void) {
	bytes = Bytes::Ptr(Bytes::create());
}

void Checkpoint::clear(void) {
	bytes = nullptr;
}

bool toCheckpoint(const Project* project, const char* name, Checkpoint &checkpoint) {
	bool result = false;

	Bytes::Ptr bytes = checkpoint.bytes;

	bytes->clear();

	LockGuard<RecursiveMutex>::UniquePtr acquired;
	Project* prj = project->acquire(acquired);
	if (!prj)
		return result;

	Asset* asset = prj->get(name);
	if (!asset)
		return result;

	checkpoint.compressed = asset->type() != Image::TYPE(); // Do not compress encoded image.
	if (!checkpoint.compressed) {
		std::string ext = asset->extName();
		Text::toLowerCase(ext);
		if (ext == BITTY_IMAGE_EXT)
			checkpoint.compressed = true;
	}
	if (checkpoint.compressed) {
		Bytes::Ptr cache(Bytes::create());
		result = asset->save(Asset::EDITING, cache.get());
		int n = LZ4_compressBound((int)cache->count());
		bytes->resize((size_t)n);
		n = LZ4_compress_default(
			(const char*)cache->pointer(), (char*)bytes->pointer(),
			(int)cache->count(), (int)bytes->count()
		);
		assert(n);
		if (n)
			bytes->resize((size_t)n);

		checkpoint.originalSize = cache->count();

		fprintf(stdout, "Saved checkpoint in %d bytes, compressed to %d bytes.\n", (int)cache->count(), (int)bytes->count());
	} else {
		result = asset->save(Asset::EDITING, bytes.get());

		checkpoint.originalSize = bytes->count();

		fprintf(stdout, "Saved checkpoint in %d bytes.\n", (int)bytes->count());
	}

	bytes->poke(0);

	return result;
}

bool fromCheckpoint(const Project* project, const char* name, Checkpoint &checkpoint) {
	bool result = false;

	Bytes::Ptr bytes = checkpoint.bytes;

	const size_t pos = bytes->peek();
	bytes->poke(0);

	LockGuard<RecursiveMutex>::UniquePtr acquired;
	Project* prj = project->acquire(acquired);
	if (!prj)
		return result;

	Asset* asset = prj->get(name);
	if (!asset)
		return result;

	if (checkpoint.compressed) {
		Bytes::Ptr cache(Bytes::create());
		cache->resize(checkpoint.originalSize);
		const int n = LZ4_decompress_safe(
			(const char*)bytes->pointer(), (char*)cache->pointer(),
			(int)bytes->count(), (int)cache->count()
		);
		(void)n;
		assert(n == (int)checkpoint.originalSize);
		result = asset->reload(Asset::EDITING, cache.get(), nullptr, false);

		fprintf(stdout, "Restored checkpoint from %d bytes, decompressed from %d bytes.\n", (int)cache->count(), (int)bytes->count());
	} else {
		result = asset->reload(Asset::EDITING, bytes.get(), nullptr, false);

		fprintf(stdout, "Restored checkpoint from %d bytes.\n", (int)bytes->count());
	}

	bytes->poke(pos);

	return result;
}

}

}

/* ===========================================================================} */
