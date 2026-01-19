/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2026 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "encoding.h"
#include "hacks.h"
#include "platform.h"
#include "renderer.h"
#include "text_box.h"
#include "theme.h"
#include "window.h"
#include "workspace.h"
#include "../lib/imgui/imgui_internal.h"
#include "../lib/imgui_code_editor/imgui_code_editor.h"
#include "../lib/imgui_sdl/imgui_sdl.h"
#include "../lib/jpath/jpath.hpp"
#include <SDL.h>
#if defined BITTY_OS_WIN
#	include <SDL_syswm.h>
#endif /* BITTY_OS_WIN */

/*
** {===========================================================================
** Macros and constants
*/

#ifndef TEXT_BOX_FONT_RANGES_DEFAULT_NAME
#	define TEXT_BOX_FONT_RANGES_DEFAULT_NAME "default"
#endif /* TEXT_BOX_FONT_RANGES_DEFAULT_NAME */
#ifndef TEXT_BOX_FONT_RANGES_CHINESE_NAME
#	define TEXT_BOX_FONT_RANGES_CHINESE_NAME "chinese"
#endif /* TEXT_BOX_FONT_RANGES_CHINESE_NAME */
#ifndef TEXT_BOX_FONT_RANGES_JAPANESE_NAME
#	define TEXT_BOX_FONT_RANGES_JAPANESE_NAME "japanese"
#endif /* TEXT_BOX_FONT_RANGES_JAPANESE_NAME */
#ifndef TEXT_BOX_FONT_RANGES_KOREAN_NAME
#	define TEXT_BOX_FONT_RANGES_KOREAN_NAME "korean"
#endif /* TEXT_BOX_FONT_RANGES_KOREAN_NAME */
#ifndef TEXT_BOX_FONT_RANGES_CYRILLIC_NAME
#	define TEXT_BOX_FONT_RANGES_CYRILLIC_NAME "cyrillic"
#endif /* TEXT_BOX_FONT_RANGES_CYRILLIC_NAME */
#ifndef TEXT_BOX_FONT_RANGES_THAI_NAME
#	define TEXT_BOX_FONT_RANGES_THAI_NAME "thai"
#endif /* TEXT_BOX_FONT_RANGES_THAI_NAME */
#ifndef TEXT_BOX_FONT_RANGES_VIETNAMESE_NAME
#	define TEXT_BOX_FONT_RANGES_VIETNAMESE_NAME "vietnamese"
#endif /* TEXT_BOX_FONT_RANGES_VIETNAMESE_NAME */
#ifndef TEXT_BOX_FONT_RANGES_POLISH_NAME
#	define TEXT_BOX_FONT_RANGES_POLISH_NAME "polish"
#endif /* TEXT_BOX_FONT_RANGES_POLISH_NAME */

/* ===========================================================================} */

/*
** {===========================================================================
** Text box
*/

class TextBoxImpl : public TextBox, public ImGui::CodeEditor {
private:
	bool _opened = false;

	std::string _id;

	bool _acquireFocus = false;
	bool _isFocused = false; // By the Lua, graphics threads.
	bool _toColorize = false;
	mutable struct {
		std::string text;
		bool overdue = true;

		void clear(void) {
			text.clear();
			overdue = true;
		}
	} _cache; // By the Lua, graphics threads.
	mutable RecursiveMutex _lock;
	struct Options {
		bool clearBeforeBaking = false;
		bool affectedByCamera = true;
		Math::Vec2i cameraPosition;
		bool contextMenuEnabled = true;

		bool toRefreshStyleColor = false;
		ImVec4 styleColors[ImGuiCol_COUNT];
		bool styleColorDirty[ImGuiCol_COUNT];
		float styleVarScrollbarSize = 14.0f;

		bool toRefreshFont = false;
		rapidjson::Document fontConfig;
		FontData fontData;

		Options() {
			memset(styleColorDirty, 0, sizeof(styleColorDirty));
		}
	} _options;

	ImGuiContext* _context = nullptr;
	ImGuiSDL::Device* _device = nullptr;
	Texture* _texture = nullptr;
	ImFont* _font = nullptr;
	ImGuiMouseCursor _mouseCursor = ImGuiMouseCursor_None;
	ImVec2 _imePosition = ImVec2(-1, -1);

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

		SetPalette(ImGui::CodeEditor::GetLightPalette());

		SetLanguageDefinition(LanguageDefinition::Text());

		SetShowWhiteSpaces(false);

		SetShowLineNumbers(false);

		SetShowLineIndicator(false);

		SetShowModificationStatus(false);

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

	virtual bool option(const std::string &key, const Variant &val) override {
		if (key == "language_definition") {
			std::string val_ = (std::string)val;
			Text::toLowerCase(val_);

			LockGuard<decltype(_lock)> guard(_lock);

			if (val_ == "text") {
				SetLanguageDefinition(LanguageDefinition::Text());
				_toColorize = true;

				return true;
			} else if (val_ == "json") {
				SetLanguageDefinition(LanguageDefinition::Json());
				_toColorize = true;

				return true;
			} else if (val_ == "c") {
				SetLanguageDefinition(LanguageDefinition::C());
				_toColorize = true;

				return true;
			} else if (val_ == "c++") {
				SetLanguageDefinition(LanguageDefinition::CPlusPlus());
				_toColorize = true;

				return true;
			} else if (val_ == "lua") {
				SetLanguageDefinition(LanguageDefinition::Lua());
				_toColorize = true;

				return true;
			} else if (val_ == "sql") {
				SetLanguageDefinition(LanguageDefinition::SQL());
				_toColorize = true;

				return true;
			}

			return false;
		} else if (key == "colorization_enabled") {
			const bool val_ = (bool)val;

			LockGuard<decltype(_lock)> guard(_lock);

			SetColorizationEnabled(val_);
			_toColorize = true;

			return true;
		} else if (key == "cursor_line") {
			const Int val_ = (Int)val;

			LockGuard<decltype(_lock)> guard(_lock);

			Coordinates pos = GetCursorPosition();
			pos.Line = val_;
			SetCursorPosition(pos);
			EnsureCursorVisible();

			return true;
		} else if (key == "cursor_column") {
			const Int val_ = (Int)val;

			LockGuard<decltype(_lock)> guard(_lock);

			Coordinates pos = GetCursorPosition();
			pos.Column = val_;
			SetCursorPosition(pos);
			EnsureCursorVisible();

			return true;
		} else if (key == "indent_with_tab") {
			const bool val_ = (bool)val;

			LockGuard<decltype(_lock)> guard(_lock);

			SetIndentWithTab(val_);

			return true;
		} else if (key == "tab_size") {
			const Int val_ = (Int)val;

			LockGuard<decltype(_lock)> guard(_lock);

			SetTabSize(val_);

			return true;
		} else if (key == "head_size") {
			const float val_ = (float)(Double)val;

			LockGuard<decltype(_lock)> guard(_lock);

			SetHeadSize(val_);

			return true;
		} else if (key == "overwrite") {
			const bool val_ = (bool)val;

			LockGuard<decltype(_lock)> guard(_lock);

			SetOverwrite(val_);

			return true;
		} else if (key == "readonly") {
			const bool val_ = (bool)val;

			LockGuard<decltype(_lock)> guard(_lock);

			SetReadOnly(val_);

			return true;
		} else if (key == "show_line_numbers") {
			const bool val_ = (bool)val;

			LockGuard<decltype(_lock)> guard(_lock);

			SetShowLineNumbers(val_);

			return true;
		} else if (key == "sticky_line_numbers") {
			const bool val_ = (bool)val;

			LockGuard<decltype(_lock)> guard(_lock);

			SetStickyLineNumbers(val_);

			return true;
		} else if (key == "show_line_indicator") {
			const bool val_ = (bool)val;

			LockGuard<decltype(_lock)> guard(_lock);

			SetShowLineIndicator(val_);

			return true;
		} else if (key == "show_modification_status") {
			const bool val_ = (bool)val;

			LockGuard<decltype(_lock)> guard(_lock);

			SetShowModificationStatus(val_);

			return true;
		} else if (key == "show_scrollbars") {
			const bool val_ = (bool)val;

			LockGuard<decltype(_lock)> guard(_lock);

			SetShowScrollBars(val_);

			return true;
		} else if (key == "show_spaces") {
			const bool val_ = (bool)val;

			LockGuard<decltype(_lock)> guard(_lock);

			SetShowWhiteSpaces(val_);

			return true;
		} else if (key == "column_indicator") {
			const Int val_ = (Int)val;

			LockGuard<decltype(_lock)> guard(_lock);

			SetSafeColumnIndicatorOffset(val_);

			return true;
		}

		if (key == "clear_before_baking") {
			const bool val_ = (bool)val;
			_options.clearBeforeBaking = val_;

			return true;
		} else if (key == "affected_by_camera") {
			const bool val_ = (bool)val;
			_options.affectedByCamera = val_;

			return true;
		} else if (key == "context_menu_enabled") {
			const bool val_ = (bool)val;
			_options.contextMenuEnabled = val_;

			return true;
		}

		if (key == "style_default") {
			const std::string val_ = (std::string)val;
			Color col;
			if (!col.fromString(val_))
				return false;

			LockGuard<decltype(_lock)> guard(_lock);

			Palette plt = GetPalette();
			plt[(int)PaletteIndex::Default] = col.toRGBA();
			SetPalette(plt);
			_toColorize = true;

			return true;
		} else if (key == "style_keyword") {
			const std::string val_ = (std::string)val;
			Color col;
			if (!col.fromString(val_))
				return false;

			LockGuard<decltype(_lock)> guard(_lock);

			Palette plt = GetPalette();
			plt[(int)PaletteIndex::Keyword] = col.toRGBA();
			SetPalette(plt);
			_toColorize = true;

			return true;
		} else if (key == "style_number") {
			const std::string val_ = (std::string)val;
			Color col;
			if (!col.fromString(val_))
				return false;

			LockGuard<decltype(_lock)> guard(_lock);

			Palette plt = GetPalette();
			plt[(int)PaletteIndex::Number] = col.toRGBA();
			SetPalette(plt);
			_toColorize = true;

			return true;
		} else if (key == "style_string") {
			const std::string val_ = (std::string)val;
			Color col;
			if (!col.fromString(val_))
				return false;

			LockGuard<decltype(_lock)> guard(_lock);

			Palette plt = GetPalette();
			plt[(int)PaletteIndex::String] = col.toRGBA();
			SetPalette(plt);
			_toColorize = true;

			return true;
		} else if (key == "style_char_literal") {
			const std::string val_ = (std::string)val;
			Color col;
			if (!col.fromString(val_))
				return false;

			LockGuard<decltype(_lock)> guard(_lock);

			Palette plt = GetPalette();
			plt[(int)PaletteIndex::CharLiteral] = col.toRGBA();
			SetPalette(plt);
			_toColorize = true;

			return true;
		} else if (key == "style_punctuation") {
			const std::string val_ = (std::string)val;
			Color col;
			if (!col.fromString(val_))
				return false;

			LockGuard<decltype(_lock)> guard(_lock);

			Palette plt = GetPalette();
			plt[(int)PaletteIndex::Punctuation] = col.toRGBA();
			SetPalette(plt);
			_toColorize = true;

			return true;
		} else if (key == "style_preprocessor") {
			const std::string val_ = (std::string)val;
			Color col;
			if (!col.fromString(val_))
				return false;

			LockGuard<decltype(_lock)> guard(_lock);

			Palette plt = GetPalette();
			plt[(int)PaletteIndex::Preprocessor] = col.toRGBA();
			SetPalette(plt);
			_toColorize = true;

			return true;
		} else if (key == "style_symbol") {
			const std::string val_ = (std::string)val;
			Color col;
			if (!col.fromString(val_))
				return false;

			LockGuard<decltype(_lock)> guard(_lock);

			Palette plt = GetPalette();
			plt[(int)PaletteIndex::Symbol] = col.toRGBA();
			SetPalette(plt);
			_toColorize = true;

			return true;
		} else if (key == "style_identifier") {
			const std::string val_ = (std::string)val;
			Color col;
			if (!col.fromString(val_))
				return false;

			LockGuard<decltype(_lock)> guard(_lock);

			Palette plt = GetPalette();
			plt[(int)PaletteIndex::Identifier] = col.toRGBA();
			SetPalette(plt);
			_toColorize = true;

			return true;
		} else if (key == "style_known_identifier") {
			const std::string val_ = (std::string)val;
			Color col;
			if (!col.fromString(val_))
				return false;

			LockGuard<decltype(_lock)> guard(_lock);

			Palette plt = GetPalette();
			plt[(int)PaletteIndex::KnownIdentifier] = col.toRGBA();
			SetPalette(plt);
			_toColorize = true;

			return true;
		} else if (key == "style_preproc_identifier") {
			const std::string val_ = (std::string)val;
			Color col;
			if (!col.fromString(val_))
				return false;

			LockGuard<decltype(_lock)> guard(_lock);

			Palette plt = GetPalette();
			plt[(int)PaletteIndex::PreprocIdentifier] = col.toRGBA();
			SetPalette(plt);
			_toColorize = true;

			return true;
		} else if (key == "style_comment") {
			const std::string val_ = (std::string)val;
			Color col;
			if (!col.fromString(val_))
				return false;

			LockGuard<decltype(_lock)> guard(_lock);

			Palette plt = GetPalette();
			plt[(int)PaletteIndex::Comment] = col.toRGBA();
			SetPalette(plt);
			_toColorize = true;

			return true;
		} else if (key == "style_multiline_comment") {
			const std::string val_ = (std::string)val;
			Color col;
			if (!col.fromString(val_))
				return false;

			LockGuard<decltype(_lock)> guard(_lock);

			Palette plt = GetPalette();
			plt[(int)PaletteIndex::MultiLineComment] = col.toRGBA();
			SetPalette(plt);
			_toColorize = true;

			return true;
		} else if (key == "style_space") {
			const std::string val_ = (std::string)val;
			Color col;
			if (!col.fromString(val_))
				return false;

			LockGuard<decltype(_lock)> guard(_lock);

			Palette plt = GetPalette();
			plt[(int)PaletteIndex::Space] = col.toRGBA();
			SetPalette(plt);
			_toColorize = true;

			return true;
		} else if (key == "style_background") {
			const std::string val_ = (std::string)val;
			Color col;
			if (!col.fromString(val_))
				return false;

			_options.toRefreshStyleColor = true;
			_options.styleColors[ImGuiCol_ChildBg] = ImVec4(col.r / 255.0f, col.g / 255.0f, col.b / 255.0f, col.a / 255.0f);
			_options.styleColorDirty[ImGuiCol_ChildBg] = true;

			LockGuard<decltype(_lock)> guard(_lock);

			Palette plt = GetPalette();
			plt[(int)PaletteIndex::Background] = col.toRGBA();
			SetPalette(plt);
			_toColorize = true;

			return true;
		} else if (key == "style_cursor") {
			const std::string val_ = (std::string)val;
			Color col;
			if (!col.fromString(val_))
				return false;

			LockGuard<decltype(_lock)> guard(_lock);

			Palette plt = GetPalette();
			plt[(int)PaletteIndex::Cursor] = col.toRGBA();
			SetPalette(plt);
			_toColorize = true;

			return true;
		} else if (key == "style_selection") {
			const std::string val_ = (std::string)val;
			Color col;
			if (!col.fromString(val_))
				return false;

			LockGuard<decltype(_lock)> guard(_lock);

			Palette plt = GetPalette();
			plt[(int)PaletteIndex::Selection] = col.toRGBA();
			SetPalette(plt);
			_toColorize = true;

			return true;
		} else if (key == "style_line_number") {
			const std::string val_ = (std::string)val;
			Color col;
			if (!col.fromString(val_))
				return false;

			LockGuard<decltype(_lock)> guard(_lock);

			Palette plt = GetPalette();
			plt[(int)PaletteIndex::LineNumber] = col.toRGBA();
			SetPalette(plt);
			_toColorize = true;

			return true;
		} else if (key == "style_current_line_fill") {
			const std::string val_ = (std::string)val;
			Color col;
			if (!col.fromString(val_))
				return false;

			LockGuard<decltype(_lock)> guard(_lock);

			Palette plt = GetPalette();
			plt[(int)PaletteIndex::CurrentLineFill] = col.toRGBA();
			SetPalette(plt);
			_toColorize = true;

			return true;
		} else if (key == "style_current_line_fill_inactive") {
			const std::string val_ = (std::string)val;
			Color col;
			if (!col.fromString(val_))
				return false;

			LockGuard<decltype(_lock)> guard(_lock);

			Palette plt = GetPalette();
			plt[(int)PaletteIndex::CurrentLineFillInactive] = col.toRGBA();
			SetPalette(plt);
			_toColorize = true;

			return true;
		} else if (key == "style_current_line_edge") {
			const std::string val_ = (std::string)val;
			Color col;
			if (!col.fromString(val_))
				return false;

			LockGuard<decltype(_lock)> guard(_lock);

			Palette plt = GetPalette();
			plt[(int)PaletteIndex::CurrentLineEdge] = col.toRGBA();
			SetPalette(plt);
			_toColorize = true;

			return true;
		} else if (key == "style_line_edited") {
			const std::string val_ = (std::string)val;
			Color col;
			if (!col.fromString(val_))
				return false;

			LockGuard<decltype(_lock)> guard(_lock);

			Palette plt = GetPalette();
			plt[(int)PaletteIndex::LineEdited] = col.toRGBA();
			SetPalette(plt);
			_toColorize = true;

			return true;
		} else if (key == "style_line_edited_saved") {
			const std::string val_ = (std::string)val;
			Color col;
			if (!col.fromString(val_))
				return false;

			LockGuard<decltype(_lock)> guard(_lock);

			Palette plt = GetPalette();
			plt[(int)PaletteIndex::LineEditedSaved] = col.toRGBA();
			SetPalette(plt);
			_toColorize = true;

			return true;
		} else if (key == "style_line_edited_reverted") {
			const std::string val_ = (std::string)val;
			Color col;
			if (!col.fromString(val_))
				return false;

			LockGuard<decltype(_lock)> guard(_lock);

			Palette plt = GetPalette();
			plt[(int)PaletteIndex::LineEditedReverted] = col.toRGBA();
			SetPalette(plt);
			_toColorize = true;

			return true;
		} else if (key == "style_text") {
			const std::string val_ = (std::string)val;
			Color col;
			if (!col.fromString(val_))
				return false;

			_options.toRefreshStyleColor = true;
			_options.styleColors[ImGuiCol_Text] = ImVec4(col.r / 255.0f, col.g / 255.0f, col.b / 255.0f, col.a / 255.0f);
			_options.styleColorDirty[ImGuiCol_Text] = true;

			return true;
		} else if (key == "style_popup_background") {
			const std::string val_ = (std::string)val;
			Color col;
			if (!col.fromString(val_))
				return false;

			_options.toRefreshStyleColor = true;
			_options.styleColors[ImGuiCol_PopupBg] = ImVec4(col.r / 255.0f, col.g / 255.0f, col.b / 255.0f, col.a / 255.0f);
			_options.styleColorDirty[ImGuiCol_PopupBg] = true;

			return true;
		} else if (key == "style_border") {
			const std::string val_ = (std::string)val;
			Color col;
			if (!col.fromString(val_))
				return false;

			_options.toRefreshStyleColor = true;
			_options.styleColors[ImGuiCol_Border] = ImVec4(col.r / 255.0f, col.g / 255.0f, col.b / 255.0f, col.a / 255.0f);
			_options.styleColorDirty[ImGuiCol_Border] = true;

			return true;
		} else if (key == "style_scrollbar_background") {
			const std::string val_ = (std::string)val;
			Color col;
			if (!col.fromString(val_))
				return false;

			_options.toRefreshStyleColor = true;
			_options.styleColors[ImGuiCol_ScrollbarBg] = ImVec4(col.r / 255.0f, col.g / 255.0f, col.b / 255.0f, col.a / 255.0f);
			_options.styleColorDirty[ImGuiCol_ScrollbarBg] = true;

			return true;
		} else if (key == "style_scrollbar_grab") {
			const std::string val_ = (std::string)val;
			Color col;
			if (!col.fromString(val_))
				return false;

			_options.toRefreshStyleColor = true;
			_options.styleColors[ImGuiCol_ScrollbarGrab] = ImVec4(col.r / 255.0f, col.g / 255.0f, col.b / 255.0f, col.a / 255.0f);
			_options.styleColorDirty[ImGuiCol_ScrollbarGrab] = true;

			return true;
		} else if (key == "style_scrollbar_grab_hovered") {
			const std::string val_ = (std::string)val;
			Color col;
			if (!col.fromString(val_))
				return false;

			_options.toRefreshStyleColor = true;
			_options.styleColors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(col.r / 255.0f, col.g / 255.0f, col.b / 255.0f, col.a / 255.0f);
			_options.styleColorDirty[ImGuiCol_ScrollbarGrabHovered] = true;

			return true;
		} else if (key == "style_scrollbar_grab_active") {
			const std::string val_ = (std::string)val;
			Color col;
			if (!col.fromString(val_))
				return false;

			_options.toRefreshStyleColor = true;
			_options.styleColors[ImGuiCol_ScrollbarGrabActive] = ImVec4(col.r / 255.0f, col.g / 255.0f, col.b / 255.0f, col.a / 255.0f);
			_options.styleColorDirty[ImGuiCol_ScrollbarGrabActive] = true;

			return true;
		} else if (key == "style_header") {
			const std::string val_ = (std::string)val;
			Color col;
			if (!col.fromString(val_))
				return false;

			_options.toRefreshStyleColor = true;
			_options.styleColors[ImGuiCol_Header] = ImVec4(col.r / 255.0f, col.g / 255.0f, col.b / 255.0f, col.a / 255.0f);
			_options.styleColorDirty[ImGuiCol_Header] = true;

			return true;
		} else if (key == "style_header_hovered") {
			const std::string val_ = (std::string)val;
			Color col;
			if (!col.fromString(val_))
				return false;

			_options.toRefreshStyleColor = true;
			_options.styleColors[ImGuiCol_HeaderHovered] = ImVec4(col.r / 255.0f, col.g / 255.0f, col.b / 255.0f, col.a / 255.0f);
			_options.styleColorDirty[ImGuiCol_HeaderHovered] = true;

			return true;
		} else if (key == "style_header_active") {
			const std::string val_ = (std::string)val;
			Color col;
			if (!col.fromString(val_))
				return false;

			_options.toRefreshStyleColor = true;
			_options.styleColors[ImGuiCol_HeaderActive] = ImVec4(col.r / 255.0f, col.g / 255.0f, col.b / 255.0f, col.a / 255.0f);
			_options.styleColorDirty[ImGuiCol_HeaderActive] = true;

			return true;
		} else if (key == "style_separator") {
			const std::string val_ = (std::string)val;
			Color col;
			if (!col.fromString(val_))
				return false;

			_options.toRefreshStyleColor = true;
			_options.styleColors[ImGuiCol_Separator] = ImVec4(col.r / 255.0f, col.g / 255.0f, col.b / 255.0f, col.a / 255.0f);
			_options.styleColorDirty[ImGuiCol_Separator] = true;

			return true;
		}

		if (key == "style_scrollbar_size") {
			const float val_ = (float)(Double)val;

			_options.styleVarScrollbarSize = std::max(val_, 0.0f);

			return true;
		}

		return false;
	}

	virtual bool useFont(const Json::Ptr &json, const FontData &fontData) override {
		if (!json)
			return false;

		if (!json->toJson(_options.fontConfig))
			return false;

		_options.toRefreshFont = true;
		_options.fontData = fontData;

		return true;
	}

	virtual bool focused(void) const override {
		LockGuard<decltype(_lock)> guard(_lock);

		return _isFocused;
	}
	virtual void focus(void) override {
		_acquireFocus = true;
	}

	virtual void selectAll(void) override {
		SelectAll();
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
		int /* scale */,
		bool /* pending */,
		double /* delta */
	) override {
		if (_acquireFocus) {
			if (!ws->popupBox()) {
				_acquireFocus = false;
				EditorFocused = true;
				ImGui::SetNextWindowFocus();
			}
		}
		if (_toColorize) {
			_toColorize = false;
			Colorize();
		}

		if (_font && _font->IsLoaded()) {
			ImGui::PushFont(_font);
			SetFont(_font);
		}
		ImGui::SetCursorPos(ImVec2(x, y));
		{
			LockGuard<decltype(_lock)> guard(_lock);

			_isFocused = IsEditorFocused();

			Render(title, ImVec2(width, height));
		}
		if (_font && _font->IsLoaded()) {
			SetFont(nullptr);
			ImGui::PopFont();
		}

		context(wnd, rnd, ws);
	}

	virtual void bake(
		class Window* wnd, class Renderer* rnd,
		class Workspace* ws,
		float x, float y, float width, float height
	) override {
		// Prepare.
		if (!_opened)
			return;

		const int scale = rnd->scale() / wnd->scale();
		const Math::Rectf clientArea = ws->canvasClientArea();
		const Math::Vec2i canvasSize = ws->canvasSize();

		// Touch the context and texture.
		ImGuiContext* currentContext = context(wnd, rnd, (int)width, (int)height);
		Texture* targetTexture = texture(wnd, rnd, (int)width, (int)height);

		// Reserve the old context.
		const ImGuiIO &oldIo = ImGui::GetIO();

		ImGuiContext* oldContext = ImGui::GetCurrentContext();
		ImGuiSDL::Device* oldDevice = ImGuiSDL::GetCurrentDevice();
		Texture* oldTarget = rnd->target();

		// Switch to the local context.
		ImGui::SetCurrentContext(currentContext);

		ImGuiSDL::SetCurrentDevice(_device);

		rnd->target(targetTexture);

		// Initialize the local context.
		ImGuiIO &io = ImGui::GetIO();

		do {
			io.DeltaTime                            = oldIo.DeltaTime;
			io.DisplaySize                          = ImVec2(width, height);
			io.DisplayFramebufferScale              = oldIo.DisplayFramebufferScale;

			io.KeyCtrl                              = oldIo.KeyCtrl;
			io.KeyShift                             = oldIo.KeyShift;
			io.KeyAlt                               = oldIo.KeyAlt;
			io.KeySuper                             = oldIo.KeySuper;
			io.KeyMods                              = oldIo.KeyMods;
			io.KeyModsPrev                          = oldIo.KeyModsPrev;
			memcpy(io.KeysDown,                       oldIo.KeysDown,                       sizeof(oldIo.KeysDown));

			memcpy(io.KeyMap,                         oldIo.KeyMap,                         sizeof(oldIo.KeyMap));
			io.KeyRepeatDelay                       = oldIo.KeyRepeatDelay;
			io.KeyRepeatRate                        = oldIo.KeyRepeatRate;
			memcpy(io.KeysDownDuration,               oldIo.KeysDownDuration,               sizeof(oldIo.KeysDownDuration));
			memcpy(io.KeysDownDurationPrev,           oldIo.KeysDownDurationPrev,           sizeof(oldIo.KeysDownDurationPrev));

			io.MouseDoubleClickTime                 = oldIo.MouseDoubleClickTime;
			io.MouseDoubleClickMaxDist              = oldIo.MouseDoubleClickMaxDist;
			io.MouseDragThreshold                   = oldIo.MouseDragThreshold;
			io.MouseWheel                           = oldIo.MouseWheel;
			io.MouseWheelH                          = oldIo.MouseWheelH;
			io.MouseDelta                           = oldIo.MouseDelta;
			io.MousePosPrev                         = oldIo.MousePosPrev;
			memcpy(io.MouseDown,                      oldIo.MouseDown,                      sizeof(oldIo.MouseDown));
			memcpy(io.MouseClickedPos,                oldIo.MouseClickedPos,                sizeof(oldIo.MouseClickedPos));
			memcpy(io.MouseClickedTime,               oldIo.MouseClickedTime,               sizeof(oldIo.MouseClickedTime));
			memcpy(io.MouseClicked,                   oldIo.MouseClicked,                   sizeof(oldIo.MouseClicked));
			memcpy(io.MouseReleased,                  oldIo.MouseReleased,                  sizeof(oldIo.MouseReleased));
			memcpy(io.MouseDownOwned,                 oldIo.MouseDownOwned,                 sizeof(oldIo.MouseDownOwned));
			memcpy(io.MouseDownOwnedUnlessPopupClose, oldIo.MouseDownOwnedUnlessPopupClose, sizeof(oldIo.MouseDownOwnedUnlessPopupClose));
			memcpy(io.MouseDownWasDoubleClick,        oldIo.MouseDownWasDoubleClick,        sizeof(oldIo.MouseDownWasDoubleClick));
			memcpy(io.MouseDownDuration,              oldIo.MouseDownDuration,              sizeof(oldIo.MouseDownDuration));
			memcpy(io.MouseDownDurationPrev,          oldIo.MouseDownDurationPrev,          sizeof(oldIo.MouseDownDurationPrev));
			memcpy(io.MouseDragMaxDistanceAbs,        oldIo.MouseDragMaxDistanceAbs,        sizeof(oldIo.MouseDragMaxDistanceAbs));
			memcpy(io.MouseDragMaxDistanceSqr,        oldIo.MouseDragMaxDistanceSqr,        sizeof(oldIo.MouseDragMaxDistanceSqr));

			io.InputQueueSurrogate                  = oldIo.InputQueueSurrogate;
			io.InputQueueCharacters                 = oldIo.InputQueueCharacters;
		} while (false);

		refreshFonts(wnd, rnd, io);

		// Prepare for rendering.
		if (_options.clearBeforeBaking) {
			const Color cls(0x2e, 0x32, 0x38, 0xff);
			rnd->clip(0, 0, rnd->width(), rnd->height());
			rnd->clear(&cls);
		}

		// Render the edit area.
		{
			ImGui::NewFrame();

			ImGuiStyle &style = ImGui::GetStyle();

			if (_options.toRefreshStyleColor) {
				_options.toRefreshStyleColor = false;
				for (int i = 0; i < ImGuiCol_COUNT; ++i) {
					if (_options.styleColorDirty[i]) {
						_options.styleColorDirty[i] = false;
						style.Colors[i] = _options.styleColors[i];
					}
				}
			}

			io.MousePos                             = oldIo.MousePos;
			memcpy(io.MouseDoubleClicked,             oldIo.MouseDoubleClicked,             sizeof(oldIo.MouseDoubleClicked));

			do {
				Math::Vec2i pos((int)(io.MousePos.x - x), (int)(io.MousePos.y - y));
				fromScreenPosition(pos, clientArea, canvasSize, 1); // Translate mouse position from screen space to local space.
				if (_options.affectedByCamera) {
					pos.x += _options.cameraPosition.x;
					pos.y += _options.cameraPosition.y;
				}
				io.MousePos                         = ImVec2((float)pos.x, (float)pos.y);
			} while (false);

			do {
				VariableGuard<decltype(style.WindowBorderSize)> guardBorderSize(&style.WindowBorderSize, style.WindowBorderSize, 0.0f);
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2());
				VariableGuard<decltype(style.ScrollbarSize)> guardScrollbarSize(&style.ScrollbarSize, style.ScrollbarSize, _options.styleVarScrollbarSize);

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
						scale,
						false,
						0.0
					);

					ImGui::End();
				}
			} while (false);

			ImGui::Render();

			ImGuiSDL::Render(ImGui::GetDrawData());
		}

		// Finish rendering.
		if (_options.clearBeforeBaking) {
			rnd->clip();
		}

		rnd->target(oldTarget);

		// Save the mouse cursor and IME states for later populating.
		_mouseCursor = ImGui::GetMouseCursor();

		do {
			Math::Vec2i pos((int)(currentContext->PlatformImePos.x + x), (int)(currentContext->PlatformImePos.y + y));
			if (_options.affectedByCamera) {
				pos.x -= _options.cameraPosition.x;
				pos.y -= _options.cameraPosition.y;
			}
			toScreenPosition(pos, clientArea, canvasSize, 1); // Translate IME position from local space to screen space.
			_imePosition = ImVec2((float)pos.x, (float)pos.y);
		} while (false);

		// Switch to the old context.
		ImGuiSDL::SetCurrentDevice(oldDevice);

		ImGui::SetCurrentContext(oldContext);
	}
	virtual void render(
		class Window* wnd, class Renderer* rnd,
		class Workspace* /* ws */,
		float x, float y, float width, float height
	) override {
		// Prepare.
		ImGuiContext* context = ImGui::GetCurrentContext();

		if (!_opened)
			return;

		Texture* tex = texture(wnd, rnd, (int)width, (int)height);

		// Render the baked texture to the target.
		const Math::Recti dstRect = Math::Recti::byXYWH((int)x, (int)y, (int)width, (int)height);
		rnd->render(
			tex,
			nullptr, &dstRect,
			nullptr, nullptr,
			false, false,
			nullptr, false, false
		);

		// Populate the mouse cursor and IME states.
		if (_mouseCursor != ImGuiMouseCursor_None && _mouseCursor != ImGuiMouseCursor_Arrow)
			ImGui::SetMouseCursor(_mouseCursor);

		if (_imePosition.x >= 0 && _imePosition.y >= 0)
			context->PlatformImePos = _imePosition;
	}

	virtual void translate(int &x0, int &y0, int &x1, int &y1, int camX, int camY) override {
		if (_options.affectedByCamera) {
			_options.cameraPosition = Math::Vec2i(camX, camY);
			x0 -= camX;
			y0 -= camY;
			x1 -= camX;
			y1 -= camY;
		}
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
		if (!_options.contextMenuEnabled)
			return;

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
		// Prepare.
		if (_context)
			return _context;

		const ImGuiIO &oldIo = ImGui::GetIO();

		// Create a new ImGui context.
		_context = ImGui::CreateContext();

		// Switch to the new context.
		ImGuiContext* oldContext = ImGui::GetCurrentContext();
		ImGui::SetCurrentContext(_context);
		ImGuiSDL::Device* oldDevice = ImGuiSDL::GetCurrentDevice();

		// Setup the new context.
		{
			ImGuiStyle &style = ImGui::GetStyle();
			ImGuiIO &io = ImGui::GetIO();

			style.ScrollbarRounding                      = 0;
			style.TabRounding                            = 0;

			style.Colors[ImGuiCol_Text]                  = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
			style.Colors[ImGuiCol_TextDisabled]          = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
			style.Colors[ImGuiCol_WindowBg]              = ImVec4(0.94f, 0.94f, 0.94f, 1.00f);
			style.Colors[ImGuiCol_ChildBg]               = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
			style.Colors[ImGuiCol_PopupBg]               = ImVec4(1.00f, 1.00f, 1.00f, 0.98f);
			style.Colors[ImGuiCol_Border]                = ImVec4(0.00f, 0.00f, 0.00f, 0.30f);
			style.Colors[ImGuiCol_BorderShadow]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
			style.Colors[ImGuiCol_FrameBg]               = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
			style.Colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
			style.Colors[ImGuiCol_FrameBgActive]         = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
			style.Colors[ImGuiCol_TitleBg]               = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
			style.Colors[ImGuiCol_TitleBgActive]         = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
			style.Colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(1.00f, 1.00f, 1.00f, 0.51f);
			style.Colors[ImGuiCol_MenuBarBg]             = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
			style.Colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.98f, 0.98f, 0.98f, 0.53f);
			style.Colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.69f, 0.69f, 0.69f, 0.80f);
			style.Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.49f, 0.49f, 0.49f, 0.80f);
			style.Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.49f, 0.49f, 0.49f, 1.00f);
			style.Colors[ImGuiCol_CheckMark]             = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
			style.Colors[ImGuiCol_SliderGrab]            = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
			style.Colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.46f, 0.54f, 0.80f, 0.60f);
			style.Colors[ImGuiCol_Button]                = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
			style.Colors[ImGuiCol_ButtonHovered]         = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
			style.Colors[ImGuiCol_ButtonActive]          = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
			style.Colors[ImGuiCol_Header]                = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
			style.Colors[ImGuiCol_HeaderHovered]         = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
			style.Colors[ImGuiCol_HeaderActive]          = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
			style.Colors[ImGuiCol_Separator]             = ImVec4(0.39f, 0.39f, 0.39f, 0.62f);
			style.Colors[ImGuiCol_SeparatorHovered]      = ImVec4(0.14f, 0.44f, 0.80f, 0.78f);
			style.Colors[ImGuiCol_SeparatorActive]       = ImVec4(0.14f, 0.44f, 0.80f, 1.00f);
			style.Colors[ImGuiCol_ResizeGrip]            = ImVec4(0.80f, 0.80f, 0.80f, 0.56f);
			style.Colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
			style.Colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
			style.Colors[ImGuiCol_Tab]                   = ImLerp(style.Colors[ImGuiCol_Header],    style.Colors[ImGuiCol_TitleBgActive], 0.90f);
			style.Colors[ImGuiCol_TabHovered]            = ImVec4(0.00f, 0.48f, 0.80f, 1.00f);
			style.Colors[ImGuiCol_TabActive]             = ImVec4(0.00f, 0.42f, 0.74f, 1.00f);
			style.Colors[ImGuiCol_TabUnfocused]          = ImLerp(style.Colors[ImGuiCol_Tab],       style.Colors[ImGuiCol_TitleBg], 0.80f);
			style.Colors[ImGuiCol_TabUnfocusedActive]    = ImLerp(style.Colors[ImGuiCol_TabActive], style.Colors[ImGuiCol_TitleBg], 0.40f);
			style.Colors[ImGuiCol_PlotLines]             = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
			style.Colors[ImGuiCol_PlotLinesHovered]      = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
			style.Colors[ImGuiCol_PlotHistogram]         = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
			style.Colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(1.00f, 0.45f, 0.00f, 1.00f);
			style.Colors[ImGuiCol_TableHeaderBg]         = ImVec4(0.78f, 0.87f, 0.98f, 1.00f);
			style.Colors[ImGuiCol_TableBorderStrong]     = ImVec4(0.57f, 0.57f, 0.64f, 1.00f);
			style.Colors[ImGuiCol_TableBorderLight]      = ImVec4(0.68f, 0.68f, 0.74f, 1.00f);
			style.Colors[ImGuiCol_TableRowBg]            = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
			style.Colors[ImGuiCol_TableRowBgAlt]         = ImVec4(0.30f, 0.30f, 0.30f, 0.09f);
			style.Colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
			style.Colors[ImGuiCol_DragDropTarget]        = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
			style.Colors[ImGuiCol_NavHighlight]          = style.Colors[ImGuiCol_HeaderHovered];
			style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.70f, 0.70f, 0.70f, 0.70f);
			style.Colors[ImGuiCol_NavWindowingDimBg]     = ImVec4(0.20f, 0.20f, 0.20f, 0.20f);
			style.Colors[ImGuiCol_ModalWindowDimBg]      = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);

			io.IniFilename                               = nullptr;
			io.ConfigFlags                               = oldIo.ConfigFlags;
			io.BackendFlags                              = oldIo.BackendFlags;

			io.SetClipboardTextFn                        = oldIo.SetClipboardTextFn;
			io.GetClipboardTextFn                        = oldIo.GetClipboardTextFn;
			io.ClipboardUserData                         = oldIo.ClipboardUserData;

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

		// Switch to the old context.
		ImGuiSDL::SetCurrentDevice(oldDevice);
		ImGui::SetCurrentContext(oldContext);

		// Return the new created context.
		return _context;
	}
	Texture* texture(Window* /* wnd */, Renderer* rnd, int width, int height) {
		// Invalidate the texture if the desired size has been changed.
		if (_texture && (_texture->width() != width || _texture->height() != height))
			_texture = nullptr;

		// Reuse the cached texture if possible.
		if (_texture)
			return _texture;

		// Create a new texture.
		_texture = Texture::create();
		Byte* pixels = new Byte[width * height * sizeof(Color)];
		memset(pixels, 0, width * height * sizeof(Color));
		_texture->fromBytes(rnd, Texture::TARGET, pixels, width, height, 0, Texture::NEAREST);
		_texture->blend(Texture::BLEND);
		delete [] pixels;

		// Return the new created texture.
		return _texture;
	}
	bool refreshFonts(Window* /* wnd */, Renderer* rnd, ImGuiIO &io) {
		auto rebuild = [] (Renderer* rnd, ImGuiIO &io) -> void {
			if (io.Fonts->TexID) {
				ImGuiSDLHack::Texture* texture = static_cast<ImGuiSDLHack::Texture*>(io.Fonts->TexID);
				delete texture;
				io.Fonts->TexID = nullptr;
			}

			unsigned char* pixels = nullptr;
			int width = 0, height = 0;
			io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
			ImGuiSDLHack::Texture* texture = new ImGuiSDLHack::Texture(rnd, pixels, width, height);
			io.Fonts->TexID = (void*)texture;
		};

		if (!_options.toRefreshFont) {
			if (!_font) {
				io.Fonts->Clear();
				_font = io.Fonts->AddFontDefault();

				rebuild(rnd, io);
			}

			return true;
		}

		const rapidjson::Value* fonts = nullptr;
		Jpath::get(_options.fontConfig, fonts, "fonts");
		if (fonts && fonts->IsArray()) {
			for (int i = 0; i < (int)fonts->Capacity(); ++i) {
				std::string operation = "merge";
				std::string usage = "generic";
				std::string path;
				float size = 0;
				std::string ranges;
				Math::Vec2i oversample(0, 0);
				Math::Vec2f glyphOffset(0, 0);

				Jpath::get(*fonts, operation, i, "operation");
				Jpath::get(*fonts, usage, i, "usage"); // Trivial for `TextBox` font.
				Jpath::get(*fonts, path, i, "path");
				Jpath::get(*fonts, size, i, "size");
				Jpath::get(*fonts, ranges, i, "ranges");
				Jpath::get(*fonts, oversample.x, i, "oversample", 0);
				Jpath::get(*fonts, oversample.y, i, "oversample", 1);
				Jpath::get(*fonts, glyphOffset.x, i, "glyph_offset", 0);
				Jpath::get(*fonts, glyphOffset.y, i, "glyph_offset", 1);

				size = Math::clamp(size, 4.0f, 96.0f);
				oversample.x = Math::clamp(oversample.x, (Int)1, (Int)8);
				oversample.y = Math::clamp(oversample.y, (Int)1, (Int)8);
				glyphOffset.x = Math::clamp(glyphOffset.x, (Real)-96, (Real)96);
				glyphOffset.y = Math::clamp(glyphOffset.y, (Real)-96, (Real)96);

				const ImWchar* glyphRanges = io.Fonts->GetGlyphRangesDefault();
				ImVector<ImWchar> rangesVector;
				if (ranges == TEXT_BOX_FONT_RANGES_DEFAULT_NAME) {
					glyphRanges = io.Fonts->GetGlyphRangesDefault();
				} else if (ranges == TEXT_BOX_FONT_RANGES_CHINESE_NAME) {
					glyphRanges = io.Fonts->GetGlyphRangesChineseSimplifiedCommon();
				} else if (ranges == TEXT_BOX_FONT_RANGES_JAPANESE_NAME) {
					glyphRanges = io.Fonts->GetGlyphRangesJapanese();
				} else if (ranges == TEXT_BOX_FONT_RANGES_KOREAN_NAME) {
					glyphRanges = io.Fonts->GetGlyphRangesKorean();
				} else if (ranges == TEXT_BOX_FONT_RANGES_CYRILLIC_NAME) {
					glyphRanges = io.Fonts->GetGlyphRangesCyrillic();
				} else if (ranges == TEXT_BOX_FONT_RANGES_THAI_NAME) {
					glyphRanges = io.Fonts->GetGlyphRangesThai();
				} else if (ranges == TEXT_BOX_FONT_RANGES_VIETNAMESE_NAME) {
					glyphRanges = io.Fonts->GetGlyphRangesVietnamese();
				} else if (ranges == TEXT_BOX_FONT_RANGES_POLISH_NAME) {
					static constexpr const ImWchar RANGES_POLISH[] = {
						0x0020, 0x00ff, // Basic Latin + Latin supplement.
						0x0100, 0x017f, // Polish alphabet.
						0
					};
					glyphRanges = RANGES_POLISH;
				} else if (!ranges.empty()) {
					const std::wstring wstr = Unicode::toWide(ranges);
					if (!wstr.empty() && wstr.size() % 2 == 0) {
						for (int j = 0; j < (int)wstr.length(); j += 2) {
							const wchar_t ch0 = wstr[j];
							const wchar_t ch1 = wstr[j + 1];
							if (ch0 > ch1) {
								rangesVector.clear();

								break;
							}
							rangesVector.push_back(ch0);
							rangesVector.push_back(ch1);
						}
						if (!rangesVector.empty()) {
							if (rangesVector.back() != 0)
								rangesVector.push_back(0);
							glyphRanges = &rangesVector.front();
						}
					}
				}

				ImFontConfig fontCfg;
				fontCfg.FontDataOwnedByAtlas = false;
				fontCfg.OversampleH = (int)oversample.x; fontCfg.OversampleV = (int)oversample.y;
				fontCfg.GlyphOffset = ImVec2((float)glyphOffset.x, (float)glyphOffset.y);
				fontCfg.MergeMode = operation == "merge";
				if (operation == "set" || operation == "merge") {
					const bool setDefault = operation == "set" /* && usage == "generic" */ && glyphRanges == io.Fonts->GetGlyphRangesDefault();
					if (setDefault)
						io.Fonts->Clear();

					do {
						FontData::iterator it = _options.fontData.find(path);
						if (it == _options.fontData.end())
							break;

						const Bytes::Ptr &bytes = it->second;
						if (!bytes)
							break;

						ImFont* font = io.Fonts->AddFontFromMemoryTTF(
							(void*)bytes->pointer(), (int)bytes->count(),
							size,
							&fontCfg, glyphRanges
						);
						if (setDefault && !font)
							io.Fonts->AddFontDefault();
						// if (usage == "code")
						_font = font;
					} while (false);
				} else if (operation == "clear") {
					io.Fonts->Clear();
					io.Fonts->AddFontDefault();
				}
			}
		}

		rebuild(rnd, io);

		_options.toRefreshFont = false;
		_options.fontData.clear();

		return true;
	}

	static bool fromScreenPosition(Math::Vec2i &pos, const Math::Rectf &clientArea, const Math::Vec2i &canvasSize, int scale) {
		const double dstW = (double)canvasSize.x;
		const double dstH = (double)canvasSize.y;
		double fx = 0, fy = 0;
		if (scale == 1) {
			fx = (double)(pos.x - clientArea.xMin()) / clientArea.width() * dstW;
			fy = (double)(pos.y - clientArea.yMin()) / clientArea.height() * dstH;
		} else {
			fx = (double)(pos.x / (double)scale - clientArea.xMin()) / clientArea.width() * dstW;
			fy = (double)(pos.y / (double)scale - clientArea.yMin()) / clientArea.height() * dstH;
		}
		if (fx >= 0 && fx < canvasSize.x && fy >= 0 && fy < canvasSize.y) {
			pos.x = (int)fx;
			pos.y = (int)fy;

			return true;
		}
		pos.x = -1;
		pos.y = -1;

		return false;
	}
	static bool toScreenPosition(Math::Vec2i &pos, const Math::Rectf &clientArea, const Math::Vec2i &canvasSize, int scale) {
		const double dstW = (double)canvasSize.x;
		const double dstH = (double)canvasSize.y;
		double fx = pos.x, fy = pos.y;
		if (scale == 1) {
			fx = fx / dstW * clientArea.width() + clientArea.xMin();
			fy = fy / dstH * clientArea.height() + clientArea.yMin();
		} else {
			fx = (fx / dstW * clientArea.width() + clientArea.xMin() * (double)scale);
			fy = (fy / dstH * clientArea.height() + clientArea.yMin() * (double)scale);
		}
		pos = Math::Vec2i((int)fx, (int)fy);

		return true;
	}
};

bool TextBox::parseFont(const Json::Ptr &json, FontData &fontData, FontResolver resolveFont) {
	fontData.clear();

	if (!json)
		return false;

	rapidjson::Document doc;
	if (!json->toJson(doc))
		return false;

	const rapidjson::Value* fonts = nullptr;
	Jpath::get(doc, fonts, "fonts");
	if (!fonts || !fonts->IsArray())
		return false;

	for (int i = 0; i < (int)fonts->Capacity(); ++i) {
		std::string path;

		Jpath::get(*fonts, path, i, "path");
		if (path.empty())
			continue;

		Bytes::Ptr bytes = resolveFont(path);
		if (!bytes)
			continue;

		fontData[path] = bytes;
	}

	return true;
}

TextBox* TextBox::create(void) {
	TextBoxImpl* result = new TextBoxImpl();

	return result;
}

void TextBox::destroy(TextBox* ptr) {
	TextBoxImpl* impl = static_cast<TextBoxImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */
