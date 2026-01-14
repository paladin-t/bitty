/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2026 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "platform.h"
#include "renderer.h"
#include "text_box.h"
#include "theme.h"
#include "window.h"
#include "workspace.h"
#include "../lib/imgui_code_editor/imgui_code_editor.h"
#include "../lib/imgui_sdl/imgui_sdl.h"
#include <SDL.h>
#if defined BITTY_OS_WIN
#	include <SDL_syswm.h>
#endif /* BITTY_OS_WIN */

/*
** {===========================================================================
** Text box
*/

class TextBoxImpl : public TextBox, public ImGui::CodeEditor {
private:
	bool _opened = false;

	std::string _id;

	bool _acquireFocus = false;
	mutable struct {
		std::string text;
		bool overdue = true;

		void clear(void) {
			text.clear();
			overdue = true;
		}
	} _cache;
	mutable RecursiveMutex _lock;

	ImGuiContext* _context = nullptr;
	ImGuiSDL::Device* _device = nullptr;
	Texture* _texture = nullptr;

public:
	TextBoxImpl() {
	}
	virtual ~TextBoxImpl() override {
		assert(!_opened && "Not closed");
	}

	virtual unsigned type(void) const override {
		return TYPE();
	}

	virtual bool clone(Object** ptr) const override { // Non-clonable.
		if (ptr)
			*ptr = nullptr;

		return false;
	}

	virtual void open(const class Project* /* project */, const char* name, Object::Ptr /* obj */, const char* /* ref */) override {
		LockGuard<decltype(_lock)> guard(_lock);

		if (_opened)
			return;
		_opened = true;

		_id = name;

		SetTooltipEnabled(false);

		SetModifiedHandler(std::bind(&TextBoxImpl::modified, this));

		fprintf(stdout, "Text box opened: \"%s\".\n", _id.c_str());
	}
	virtual void close(const class Project* /* project */) override {
		LockGuard<decltype(_lock)> guard(_lock);

		if (!_opened)
			return;
		_opened = false;

		fprintf(stdout, "Text box closed: \"%s\".\n", _id.c_str());

		SetModifiedHandler(nullptr);

		_cache.clear();

		_id.clear();

		if (_context) {
			ImGuiContext* oldContext = ImGui::GetCurrentContext();
			ImGui::SetCurrentContext(_context);
			ImGuiSDL::Device* oldDevice = ImGuiSDL::GetCurrentDevice();
			ImGuiSDL::SetCurrentDevice(_device);
			{
				ImGuiSDL::Deinitialize();

				_device = nullptr;
			}
			ImGuiSDL::SetCurrentDevice(oldDevice);
			ImGui::SetCurrentContext(oldContext);

			ImGui::DestroyContext(_context);
			_context = nullptr;
		}

		if (_texture) {
			Texture::destroy(_texture);
			_texture = nullptr;
		}
	}

	virtual void flush(void) const override {
		// Do nothing.
	}

	virtual const char* text(size_t* len) const override {
		LockGuard<decltype(_lock)> guard(_lock);

		if (_cache.overdue) {
			_cache.text = GetText("\n");
			_cache.overdue = false;
		}
		if (len)
			*len = _cache.text.length();

		return _cache.text.c_str();
	}
	virtual void text(const char* txt, size_t /* len */) override {
		LockGuard<decltype(_lock)> guard(_lock);

		SetText(txt);
	}

	virtual bool readonly(void) const override {
		LockGuard<decltype(_lock)> guard(_lock);

		return IsReadOnly();
	}
	virtual void readonly(bool ro) override {
		LockGuard<decltype(_lock)> guard(_lock);

		SetReadOnly(ro);
	}

	virtual bool hasUnsavedChanges(void) const override {
		LockGuard<decltype(_lock)> guard(_lock);

		return !IsChangesSaved();
	}
	virtual void markChangesSaved(const class Project* /* project */) override {
		LockGuard<decltype(_lock)> guard(_lock);

		SetChangesSaved();
	}

	virtual void copy(void) override {
		LockGuard<decltype(_lock)> guard(_lock);

		Copy();
	}
	virtual void cut(void) override {
		LockGuard<decltype(_lock)> guard(_lock);

		if (ReadOnly) {
			copy();

			return;
		}

		Cut();
	}
	virtual bool pastable(void) const override {
		LockGuard<decltype(_lock)> guard(_lock);

		if (ReadOnly)
			return false;

		return Platform::hasClipboardText();
	}
	virtual void paste(void) override {
		LockGuard<decltype(_lock)> guard(_lock);

		if (ReadOnly)
			return;

		Paste();
	}
	virtual void del(void) override {
		LockGuard<decltype(_lock)> guard(_lock);

		if (ReadOnly)
			return;

		Delete();
	}
	virtual bool selectable(void) const override {
		return true;
	}

	virtual const char* redoable(void) const override {
		LockGuard<decltype(_lock)> guard(_lock);

		return CanRedo() ? "" : nullptr;
	}
	virtual const char* undoable(void) const override {
		LockGuard<decltype(_lock)> guard(_lock);

		return CanUndo() ? "" : nullptr;
	}

	virtual void redo(class Asset*) override {
		LockGuard<decltype(_lock)> guard(_lock);

		if (ReadOnly)
			return;

		Redo();
	}
	virtual void undo(class Asset*) override {
		LockGuard<decltype(_lock)> guard(_lock);

		if (ReadOnly)
			return;

		Undo();
	}

	virtual Variant post(unsigned msg, int argc, const Variant* argv) override {
		LockGuard<decltype(_lock)> guard(_lock);

		switch (msg) {
		case SET_THEME_STYLE: {
				const Variant::Int idx = unpack<Variant::Int>(argc, argv, 0, -1);
				switch (idx) {
				case Theme::DARK:
					SetPalette(ImGui::CodeEditor::GetDarkPalette());

					break;
				case Theme::CLASSIC:
					SetPalette(ImGui::CodeEditor::GetRetroBluePalette());

					break;
				case Theme::LIGHT:
					SetPalette(ImGui::CodeEditor::GetLightPalette());

					break;
				}
			}

			return Variant(true);
		case SET_INDENT_RULE: {
				const Int rule = unpack<Int>(argc, argv, 0, (Int)Workspace::Settings::TAB_4);
				switch ((Workspace::Settings::IndentRules)rule) {
				case Workspace::Settings::SPACE_2:
					SetIndentWithTab(false);
					SetTabSize(2);

					break;
				case Workspace::Settings::SPACE_4:
					SetIndentWithTab(false);
					SetTabSize(4);

					break;
				case Workspace::Settings::SPACE_8:
					SetIndentWithTab(false);
					SetTabSize(8);

					break;
				case Workspace::Settings::TAB_2:
					SetIndentWithTab(true);
					SetTabSize(2);

					break;
				case Workspace::Settings::TAB_4:
					SetIndentWithTab(true);
					SetTabSize(4);

					break;
				case Workspace::Settings::TAB_8:
					SetIndentWithTab(true);
					SetTabSize(8);

					break;
				}
			}

			return Variant(true);
		case SET_COLUMN_INDICATOR: {
				const Int rule = unpack<Int>(argc, argv, 0, (Int)Workspace::Settings::COL_80);
				switch ((Workspace::Settings::ColumnIndicator)rule) {
				case Workspace::Settings::COL_NONE:
					SetSafeColumnIndicatorOffset(0);

					break;
				case Workspace::Settings::COL_40:
					SetSafeColumnIndicatorOffset(40);

					break;
				case Workspace::Settings::COL_80:
					SetSafeColumnIndicatorOffset(80);

					break;
				case Workspace::Settings::COL_100:
					SetSafeColumnIndicatorOffset(100);

					break;
				case Workspace::Settings::COL_120:
					SetSafeColumnIndicatorOffset(120);

					break;
				}
			}

			return Variant(true);
		case SET_SHOW_SPACES: {
				const bool show = unpack<bool>(argc, argv, 0, true);
				SetShowWhiteSpaces(show);
			}

			return Variant(true);
		case FOCUS:
			_acquireFocus = true;

			return Variant(true);
		case SELECT_ALL:
			SelectAll();

			return Variant(true);
		case SELECT_WORD:
			SelectWordUnderCursor();

			return Variant(true);
		case INDENT: {
				const bool byKey = unpack<bool>(argc, argv, 0, true);

				Indent(byKey);
			}

			return Variant(true);
		case UNINDENT: {
				const bool byKey = unpack<bool>(argc, argv, 0, true);

				Unindent(byKey);
			}

			return Variant(true);
		case TOGGLE_COMMENT:
			if (HasSelection()) {
				if (GetCommentLines() == GetSelectionLines())
					Uncomment();
				else
					Comment();
			} else {
				if (GetCommentLines() > 0)
					Uncomment();
				else
					Comment();
			}

			return Variant(true);
		case MOVE_UP:
			MoveLineUp();

			return Variant(true);
		case MOVE_DOWN:
			MoveLineDown();

			return Variant(true);
		case GET_CURSOR:
			return Variant((Variant::Int)GetCursorPosition().Line);
		case SET_CURSOR: {
				const Variant::Int ln = unpack<Variant::Int>(argc, argv, 0, -1);
				if (ln < 0 || ln >= GetTotalLines())
					break;

				SetCursorPosition(Coordinates(ln, 0));
			}

			return Variant(true);
		case GET_PROGRAM_POINTER:
			return Variant((Variant::Int)GetProgramPointer());
		case SET_PROGRAM_POINTER: {
				const Variant::Int ln = unpack<Variant::Int>(argc, argv, 0, -1);
				if (ln < 0 || ln >= GetTotalLines()) {
					SetProgramPointer(-1);

					break;
				}

				SetProgramPointer(ln);
			}

			return Variant(true);
		case GET_BREAKPOINT: {
				const Variant::Int ln = unpack<Variant::Int>(argc, argv, 0, -1);
				if (ln < 0 || ln >= GetTotalLines())
					break;

				Breakpoints::iterator it = Brks.find(ln);
				if (it == Brks.end())
					break;
			}

			return Variant(true);
		case SET_BREAKPOINT: {
				const Variant::Int ln = unpack<Variant::Int>(argc, argv, 0, -1);
				const bool brk = unpack<bool>(argc, argv, 1, false);
				const bool enabled = unpack<bool>(argc, argv, 2, true);
				if (ln < 0 || ln >= GetTotalLines())
					break;

				Breakpoints::iterator it = Brks.find(ln);
				if (brk) {
					if (it != Brks.end())
						Brks.erase(it);

					Brks.insert(std::make_pair(ln, enabled));
				} else {
					if (it == Brks.end())
						break;

					Brks.erase(it);
				}
			}

			return Variant(true);
		case GET_BREAKPOINTS: {
				IList::Ptr lst(List::create());
				for (Breakpoints::value_type brk : Brks)
					lst->add((Variant::Int)brk.first);

				return Variant(lst);
			}
		case CLEAR_BREAKPOINTS:
			Brks.clear();

			return Variant(true);
		default: // Do nothing.
			break;
		}

		return Variant(false);
	}
	using Dispatchable::post;

	virtual void update(
		class Window* wnd, class Renderer* rnd,
		class Workspace* ws, const class Project* /* project */, class Executable* /* exec */,
		const char* title,
		float x, float y, float width, float height,
		float /* scaleX */, float /* scaleY */,
		bool /* pending */,
		double /* delta */
	) override {
		if (_acquireFocus) {
			if (!ws->popupBox()) {
				_acquireFocus = false;
				ImGui::SetNextWindowFocus();
			}
		}

		ImFont* fontCode = ws->theme()->fontCode(); // TODO: use font.
		if (fontCode && fontCode->IsLoaded()) {
			ImGui::PushFont(fontCode);
			SetFont(fontCode);
		}
		ImGui::SetCursorPos(ImVec2(x, y));
		{
			LockGuard<decltype(_lock)> guard(_lock);

			Render(title, ImVec2(width, height));
		}
		if (fontCode && fontCode->IsLoaded()) {
			SetFont(nullptr);
			ImGui::PopFont();
		}

		context(wnd, rnd, ws);
	}

	virtual void bake(
		class Window* wnd, class Renderer* rnd,
		class Workspace* ws,
		float /* x */, float /* y */, float width, float height
	) override {
		if (!_opened)
			return;

		ImGuiContext* ctx = context(wnd, rnd, (int)width, (int)height);
		Texture* tex = texture(wnd, rnd, (int)width, (int)height);

		const ImGuiIO &oldIo = ImGui::GetIO();

		ImGuiContext* oldContext = ImGui::GetCurrentContext();
		ImGui::SetCurrentContext(ctx);

		ImGuiSDL::Device* oldDevice = ImGuiSDL::GetCurrentDevice();
		ImGuiSDL::SetCurrentDevice(_device);

		Texture* oldTarget = rnd->target();
		rnd->target(tex);

		ImGuiIO &io = ImGui::GetIO();
		// TODO: optimize.
		io.DisplaySize                          = oldIo.DisplaySize;
		io.DeltaTime                            = oldIo.DeltaTime;
		io.MouseDoubleClickTime                 = oldIo.MouseDoubleClickTime;
		io.MouseDoubleClickMaxDist              = oldIo.MouseDoubleClickMaxDist;
		io.MouseDragThreshold                   = oldIo.MouseDragThreshold;
		memcpy(io.KeyMap,                         oldIo.KeyMap,                         sizeof(oldIo.KeyMap));
		io.KeyRepeatDelay                       = oldIo.KeyRepeatDelay;
		io.KeyRepeatRate                        = oldIo.KeyRepeatRate;
		io.MousePos                             = oldIo.MousePos;
		memcpy(io.MouseDown,                      oldIo.MouseDown,                      sizeof(oldIo.MouseDown));
		io.MouseWheel                           = oldIo.MouseWheel;
		io.MouseWheelH                          = oldIo.MouseWheelH;
		io.KeyCtrl                              = oldIo.KeyCtrl;
		io.KeyShift                             = oldIo.KeyShift;
		io.KeyAlt                               = oldIo.KeyAlt;
		io.KeySuper                             = oldIo.KeySuper;
		memcpy(io.KeysDown,                       oldIo.KeysDown,                       sizeof(oldIo.KeysDown));
		io.MouseDelta                           = oldIo.MouseDelta;
		io.KeyMods                              = oldIo.KeyMods;
		io.KeyModsPrev                          = oldIo.KeyModsPrev;
		io.MousePosPrev                         = oldIo.MousePosPrev;
		memcpy(io.MouseClickedPos,                oldIo.MouseClickedPos,                sizeof(oldIo.MouseClickedPos));
		memcpy(io.MouseClickedTime,               oldIo.MouseClickedTime,               sizeof(oldIo.MouseClickedTime));
		memcpy(io.MouseClicked,                   oldIo.MouseClicked,                   sizeof(oldIo.MouseClicked));
		memcpy(io.MouseDoubleClicked,             oldIo.MouseDoubleClicked,             sizeof(oldIo.MouseDoubleClicked));
		memcpy(io.MouseReleased,                  oldIo.MouseReleased,                  sizeof(oldIo.MouseReleased));
		memcpy(io.MouseDownOwned,                 oldIo.MouseDownOwned,                 sizeof(oldIo.MouseDownOwned));
		memcpy(io.MouseDownOwnedUnlessPopupClose, oldIo.MouseDownOwnedUnlessPopupClose, sizeof(oldIo.MouseDownOwnedUnlessPopupClose));
		memcpy(io.MouseDownWasDoubleClick,        oldIo.MouseDownWasDoubleClick,        sizeof(oldIo.MouseDownWasDoubleClick));
		memcpy(io.MouseDownDuration,              oldIo.MouseDownDuration,              sizeof(oldIo.MouseDownDuration));
		memcpy(io.MouseDownDurationPrev,          oldIo.MouseDownDurationPrev,          sizeof(oldIo.MouseDownDurationPrev));
		memcpy(io.MouseDragMaxDistanceAbs,        oldIo.MouseDragMaxDistanceAbs,        sizeof(oldIo.MouseDragMaxDistanceAbs));
		memcpy(io.MouseDragMaxDistanceSqr,        oldIo.MouseDragMaxDistanceSqr,        sizeof(oldIo.MouseDragMaxDistanceSqr));
		memcpy(io.KeysDownDuration,               oldIo.KeysDownDuration,               sizeof(oldIo.KeysDownDuration));
		memcpy(io.KeysDownDurationPrev,           oldIo.KeysDownDurationPrev,           sizeof(oldIo.KeysDownDurationPrev));
		io.InputQueueSurrogate                  = oldIo.InputQueueSurrogate;
		io.InputQueueCharacters                 = oldIo.InputQueueCharacters;

		// TODO: translate input.

		//const Color cls(0x2e, 0x32, 0x38, 0xff);
		//rnd->clip(0, 0, rnd->width(), rnd->height());
		//rnd->clear(&cls);
		{
			ImGui::NewFrame();

			ImGuiStyle &style = ImGui::GetStyle();

			VariableGuard<decltype(style.WindowBorderSize)> guardBorderSize(&style.WindowBorderSize, style.WindowBorderSize, 0.0f);
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2());

			const ImGuiWindowFlags flags =
				ImGuiWindowFlags_NoTitleBar |
				ImGuiWindowFlags_NoResize |
				ImGuiWindowFlags_NoMove |
				ImGuiWindowFlags_NoScrollbar |
				ImGuiWindowFlags_NoCollapse |
				ImGuiWindowFlags_NoSavedSettings |
				ImGuiWindowFlags_NoBringToFrontOnFocus |
				ImGuiWindowFlags_NoNav;
			ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
			ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);
			if (ImGui::Begin(_id.c_str(), nullptr, flags)) {
				update(
					wnd, rnd,
					ws, nullptr, nullptr,
					_id.c_str(),
					0, 0, width, height,
					1.0f, 1.0f,
					false,
					0.0
				);

				ImGui::End();
			}

			ImGui::Render();

			ImGuiSDL::Render(ImGui::GetDrawData());
		}

		rnd->flush();

		rnd->target(oldTarget);

		ImGuiSDL::SetCurrentDevice(oldDevice);

		ImGui::SetCurrentContext(oldContext);
	}
	virtual void render(
		class Window* wnd, class Renderer* rnd,
		class Workspace* /* ws */,
		float x, float y, float width, float height
	) override {
		if (!_opened)
			return;

		Texture* tex = texture(wnd, rnd, (int)width, (int)height);
		const Math::Recti dstRect = Math::Recti::byXYWH((int)x, (int)y, (int)width, (int)height);

		rnd->render(
			tex,
			nullptr, &dstRect,
			nullptr, nullptr,
			false, false,
			nullptr, false, false
		);
	}

	virtual void played(class Renderer* /* rnd */, const class Project* /* project */) override {
		// Do nothing.
	}
	virtual void stopped(class Renderer* /* rnd */, const class Project* /* project */) override {
		// Do nothing.
	}

	virtual void resized(class Renderer* /* rnd */, const class Project* /* project */) override {
		// Do nothing.
	}

	virtual void lostFocus(class Renderer* /* rnd */, const class Project* /* project */) override {
		// Do nothing.
	}
	virtual void gainFocus(class Renderer* /* rnd */, const class Project* /* project */) override {
		// Do nothing.
	}

private:
	void context(Window* /* wnd */, Renderer* /* rnd */, Workspace* ws) {
		ImGuiStyle &style = ImGui::GetStyle();

		if (ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows) && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
			if (!HasSelection())
				SelectWordUnderMouse();

			ImGui::OpenPopup("@Ed/Ctx");
		}

		VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(8, 8));
		VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(8, 4));

		if (ImGui::BeginPopup("@Ed/Ctx")) {
			if (ImGui::MenuItem(ws->theme()->menuEdit_Cut(), nullptr, nullptr, HasSelection())) {
				cut();
			}
			if (ImGui::MenuItem(ws->theme()->menuEdit_Copy(), nullptr, nullptr, HasSelection())) {
				copy();
			}
			if (ImGui::MenuItem(ws->theme()->menuEdit_Paste(), nullptr, nullptr, pastable())) {
				paste();
			}
			if (ImGui::MenuItem(ws->theme()->menuEdit_Delete(), nullptr, nullptr, HasSelection())) {
				del();
			}
			ImGui::Separator();
			if (ImGui::MenuItem(ws->theme()->menuEdit_SelectAll())) {
				post(Editable::SELECT_ALL);
			}
			ImGui::Separator();
			if (ImGui::MenuItem(ws->theme()->menuEdit_IncreaseIndent())) {
				post(Editable::INDENT, false);
			}
			if (ImGui::MenuItem(ws->theme()->menuEdit_DecreaseIndent())) {
				post(Editable::UNINDENT, false);
			}

			ImGui::EndPopup();
		}
	}

	void modified(void) {
		LockGuard<decltype(_lock)> guard(_lock);

		_cache.overdue = true;
	}

	ImGuiContext* context(Window* wnd, Renderer* rnd, int width, int height) {
		if (_context)
			return _context;

		const ImGuiIO &oldIo = ImGui::GetIO();

		_context = ImGui::CreateContext();

		ImGuiContext* oldContext = ImGui::GetCurrentContext();
		ImGui::SetCurrentContext(_context);
		ImGuiSDL::Device* oldDevice = ImGuiSDL::GetCurrentDevice();
		{
			ImGuiStyle &style = ImGui::GetStyle();
			ImGuiIO &io = ImGui::GetIO();

			style.ScrollbarRounding = 0;
			style.TabRounding = 0;

			io.IniFilename = nullptr;
			io.ConfigFlags                          = oldIo.ConfigFlags;
			io.BackendFlags                         = oldIo.BackendFlags;

			io.SetClipboardTextFn                   = oldIo.SetClipboardTextFn;
			io.GetClipboardTextFn                   = oldIo.GetClipboardTextFn;
			io.ClipboardUserData                    = oldIo.ClipboardUserData;

#if defined BITTY_OS_WIN
			SDL_EventState(SDL_SYSWMEVENT, SDL_ENABLE);

			SDL_SysWMinfo wmInfo;
			SDL_VERSION(&wmInfo.version);
			SDL_GetWindowWMInfo((SDL_Window*)wnd->pointer(), &wmInfo);

			const HWND hwnd = wmInfo.info.win.window;
			io.ImeWindowHandle = hwnd;
#else /* BITTY_OS_WIN */
			(void)wnd;

			io.ImeSetInputScreenPosFn = Platform::inputScreenPosition;
#endif /* BITTY_OS_WIN */

			ImGuiSDL::Initialize((SDL_Renderer*)rnd->pointer(), width, height);
			_device = ImGuiSDL::GetCurrentDevice();
		}
		ImGuiSDL::SetCurrentDevice(oldDevice);
		ImGui::SetCurrentContext(oldContext);

		return _context;
	}
	Texture* texture(Window* /* wnd */, Renderer* rnd, int width, int height) {
		if (_texture && (_texture->width() != width || _texture->height() != height))
			_texture = nullptr;

		if (_texture)
			return _texture;

		_texture = Texture::create();
		Byte* pixels = new Byte[width * height * sizeof(Color)];
		memset(pixels, 0, width * height * sizeof(Color));
		_texture->fromBytes(rnd, Texture::TARGET, pixels, width, height, 0, Texture::NEAREST);
		_texture->blend(Texture::BLEND);
		delete [] pixels;

		return _texture;
	}
};

TextBox* TextBox::create(void) {
	TextBoxImpl* result = new TextBoxImpl();

	return result;
}

void TextBox::destroy(TextBox* ptr) {
	TextBoxImpl* impl = static_cast<TextBoxImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */
