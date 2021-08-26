/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2021 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "effects.h"
#include "platform.h"
#include "primitives.h"
#include "renderer.h"
#include "theme.h"
#include "widgets_sketchbook.h"
#include "window.h"
#include "../lib/curl/include/curl/curl.h"
#include "../lib/lua/src/lua.hpp"
#include "../lib/mongoose/mongoose.h"
#include "../lib/zlib/zlib.h"
#include <SDL.h>
#include <SDL_mixer.h>

/*
** {===========================================================================
** Sketchbook widgets
*/

namespace ImGui {

namespace Sketchbook {

PreferencesPopupBox::PreferencesPopupBox(
	class Primitives* primitives, class Theme* theme,
	const std::string &title,
	WorkspaceSketchbook::SketchbookSettings &settings,
	bool editable,
	const ConfirmHandler &confirm, const CancelHandler &cancel, const ApplyHandler &applyHandler,
	const char* confirmTxt, const char* cancelTxt, const char* applyTxt
) : _primitives(primitives), _theme(theme),
	_title(title),
	_settings(settings),
	_editable(editable),
	_confirmHandler(confirm), _cancelHandler(cancel), _applyHandler(applyHandler)
{
	_settingsShadow = _settings;

	if (confirmTxt)
		_confirmText = confirmTxt;
	if (cancelTxt)
		_cancelText = cancelTxt;
	if (applyTxt)
		_applyText = applyTxt;
}

PreferencesPopupBox::~PreferencesPopupBox() {
}

void PreferencesPopupBox::update(void) {
	bool isOpen = true;
	bool toConfirm = false;
	bool toApply = false;
	bool toCancel = false;

	if (_init.begin())
		OpenPopup(_title);

	SetNextWindowSize(ImVec2(384, 0), ImGuiCond_Always);
	if (BeginPopupModal(_title, nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoNav)) {
		if (BeginTabBar("@Pref")) {
			if (_editable && BeginTabItem(_theme->tabPreferences_Editor(), nullptr, ImGuiTabItemFlags_NoTooltip, _theme->style()->tabTextColor)) {
				PushID(_theme->windowPreferences_Editor_Project());
				{
					TextUnformatted(_theme->windowPreferences_Editor_Project());

					AlignTextToFramePadding();

					TextUnformatted(_theme->windowPreferences_Editor_PackageFormat());

					SameLine();

					const char* items[] = { _theme->generic_Text().c_str(), _theme->generic_Binary().c_str() };
					int pref = (int)_settingsShadow.projectPreference;
					SetNextItemWidth(GetContentRegionAvail().x);
					if (Combo("", &pref, items, BITTY_COUNTOF(items)))
						_settingsShadow.projectPreference = pref;

					Checkbox(_theme->windowPreferences_Editor_IgnoreDotFiles(), &_settingsShadow.projectIgnoreDotFiles);

					SameLine();

					TextUnformatted(_theme->windowPreferences_NeedToReopen());
				}
				PopID();

				Separator();

				PushID(_theme->windowPreferences_Editor_TextEditor());
				{
					TextUnformatted(_theme->windowPreferences_Editor_TextEditor());

					Checkbox(_theme->windowPreferences_Editor_ShowWhiteSpaces(), &_settingsShadow.editorShowWhiteSpaces);

					TextUnformatted(_theme->windowPreferences_Editor_Console());

					Checkbox(_theme->windowPreferences_Editor_ClearOnStart(), &_settingsShadow.consoleClearOnStart);
				}
				PopID();

				EndTabItem();
			}
			if (BeginTabItem(_theme->tabPreferences_Graphics(), nullptr, ImGuiTabItemFlags_NoTooltip, _theme->style()->tabTextColor)) {
#if defined BITTY_OS_WIN || defined BITTY_OS_MAC || defined BITTY_OS_LINUX
				TextUnformatted(_theme->windowPreferences_Graphics_Application());

				Checkbox(_theme->windowPreferences_Graphics_Fullscreen(), &_settingsShadow.applicationWindowFullscreen);

				Separator();
#endif /* Platform macro. */

				TextUnformatted(_theme->windowPreferences_Graphics_Canvas());

				Checkbox(_theme->windowPreferences_Graphics_FixCanvasRatio(), &_settingsShadow.canvasFixRatio);

				EndTabItem();
			}
			if (BeginTabItem(_theme->tabPreferences_Input(), nullptr, ImGuiTabItemFlags_NoTooltip, _theme->style()->tabTextColor)) {
				TextUnformatted(_theme->windowPreferences_Input_Gamepads());

				if (_activeGamepadIndex == -1)
					TextUnformatted(_theme->windowPreferences_Input_ClickToChange());
				else
					TextUnformatted(_theme->windowPreferences_Input_ClickAgainToCancelBackspaceToClear());
				ConfigGamepads(
					_primitives->input(),
					_settingsShadow.inputGamepads, INPUT_GAMEPAD_COUNT,
					&_activeGamepadIndex, &_activeButtonIndex,
					_theme->windowPreferences_Input_WaitingForInput().c_str()
				);

				EndTabItem();
			}
			if (BeginTabItem(_theme->tabPreferences_Onscreen(), nullptr, ImGuiTabItemFlags_NoTooltip, _theme->style()->tabTextColor)) {
				TextUnformatted(_theme->windowPreferences_Onscreen_Gamepad());

				ConfigOnscreenGamepad(
					&_settingsShadow.inputOnscreenGamepadEnabled,
					&_settingsShadow.inputOnscreenGamepadSwapAB,
					&_settingsShadow.inputOnscreenGamepadScale,
					&_settingsShadow.inputOnscreenGamepadPadding.x, &_settingsShadow.inputOnscreenGamepadPadding.y,
					_theme->windowPreferences_Onscreen_Enabled().c_str(),
					_theme->windowPreferences_Onscreen_SwapAB().c_str(),
					_theme->windowPreferences_Onscreen_Scale().c_str(), _theme->windowPreferences_Onscreen_PaddingX().c_str(), _theme->windowPreferences_Onscreen_PaddingY().c_str()
				);

				EndTabItem();
			}
			if (!_editable) {
				if (BeginTabItem(_theme->tabPreferences_Misc(), nullptr, ImGuiTabItemFlags_NoTooltip, _theme->style()->tabTextColor)) {
					TextUnformatted(_theme->windowPreferences_Misc_Application());

					Checkbox(_theme->windowPreferences_Misc_PauseOnFocusLost(), &_settingsShadow.applicationPauseOnFocusLost);

					EndTabItem();
				}
			}

			EndTabBar();
		}

		const char* confirm = _confirmText.c_str();
		const char* apply = _applyText.empty() ? "Apply" : _applyText.c_str();
		const char* cancel = _cancelText.empty() ? "Cancel" : _cancelText.c_str();

		const bool appliable = _settings != _settingsShadow;

		if (_confirmText.empty()) {
			confirm = "Ok";
		}

		CentralizeButton(3);

		if (Button(confirm, ImVec2(WIDGETS_BUTTON_WIDTH, 0)) || IsKeyReleased(SDL_SCANCODE_RETURN) || IsKeyReleased(SDL_SCANCODE_Y)) {
			toConfirm = true;

			CloseCurrentPopup();
		}

		SameLine();
		if (Button(cancel, ImVec2(WIDGETS_BUTTON_WIDTH, 0)) || IsKeyReleased(SDL_SCANCODE_ESCAPE)) {
			toCancel = true;

			CloseCurrentPopup();
		}

		SameLine();
		if (appliable) {
			if (Button(apply, ImVec2(WIDGETS_BUTTON_WIDTH, 0))) {
				toApply = true;
			}
		} else {
			BeginDisabled();
			Button(apply, ImVec2(WIDGETS_BUTTON_WIDTH, 0));
			EndDisabled();
		}

		if (!_init.begin() && !_init.end())
			CentralizeWindow();

		EnsureWindowVisible();

		EndPopup();
	}

	if (isOpen)
		_init.update();

	if (!isOpen)
		toCancel = true;

	if (toConfirm) {
		_init.reset();

		if (!_confirmHandler.empty())
			_confirmHandler(_settingsShadow);
	}
	if (toApply) {
		if (!_applyHandler.empty())
			_applyHandler(_settingsShadow);
	}
	if (toCancel) {
		_init.reset();

		if (!_cancelHandler.empty())
			_cancelHandler();
	}
}

AboutPopupBox::AboutPopupBox(
	class Window* wnd, class Renderer* rnd, class Primitives* primitives,
	const std::string &title,
	const ConfirmHandler &confirm,
	const char* confirmTxt
) : _primitives(primitives),
	_title(title),
	_confirmHandler(confirm)
{
#if BITTY_TRIAL_ENABLED
	_desc = "Trial v" BITTY_VERSION_STRING " - An itty bitty game engine";
#else /* BITTY_TRIAL_ENABLED */
	_desc = "v" BITTY_VERSION_STRING " - An itty bitty game engine";
#endif /* BITTY_TRIAL_ENABLED */

	_specs += "Built for " BITTY_OS ", ";
	_specs += Platform::isLittleEndian() ? "little-endian" : "big-endian";
	_specs += ", with " BITTY_CP "\n";
	_specs += "\n";

	_specs += "Libraries:\n";
	_specs += "        Lua v" LUA_VERSION_MAJOR "." LUA_VERSION_MINOR "." LUA_VERSION_RELEASE "\n";
	_specs += "        SDL v" + Text::toString(SDL_MAJOR_VERSION) + "." + Text::toString(SDL_MINOR_VERSION) + "." + Text::toString(SDL_PATCHLEVEL) + "\n";
	_specs += "  SDL mixer v" + Text::toString(SDL_MIXER_MAJOR_VERSION) + "." + Text::toString(SDL_MIXER_MINOR_VERSION) + "." + Text::toString(SDL_MIXER_PATCHLEVEL) + "\n";
	_specs += "      ImGui v" IMGUI_VERSION "\n";
#if !defined BITTY_OS_HTML
	_specs += "   Mongoose v" MG_VERSION "\n";
	_specs += "       cURL v" LIBCURL_VERSION "\n";
#endif /* BITTY_OS_HTML */
	_specs += "  RapidJSON v" RAPIDJSON_VERSION_STRING "\n";
	_specs += "       zlib v" ZLIB_VERSION "\n";
	_specs += "\n";

	_specs += "Driver:\n";
	_specs += "  ";
	_specs += rnd->driver();
	_specs += "\n";

#if BITTY_EFFECTS_ENABLED
	Effects* effects = _primitives->effects();
	_specs += "Effects supported:\n";
	_specs += "  ";
	_specs += (effects && effects->valid()) ? "true" : "false";
	_specs += "\n";
#endif /* BITTY_EFFECTS_ENABLED */

	_specs += "Render target supported:\n";
	_specs += "  ";
	_specs += Text::toString(rnd->renderTargetSupported());
	_specs += "\n";

	_specs += "Max texture size:\n";
	_specs += "  ";
	_specs += Text::toString(rnd->maxTextureWidth());
	_specs += "x";
	_specs += Text::toString(rnd->maxTextureHeight());
	_specs += "\n";

	const int dspIdx = wnd->displayIndex();
	float ddpi = 0, hdpi = 0, vdpi = 0;
	SDL_GetDisplayDPI(dspIdx, &ddpi, &hdpi, &vdpi);
	_specs += "DPI:\n";
	_specs += "  (DDPI) ";
	_specs += Text::toString(ddpi);
	_specs += ", (HDPI) ";
	_specs += Text::toString(hdpi);
	_specs += ", (VDPI) ";
	_specs += Text::toString(vdpi);
	_specs += "\n";

	_specs += "Chunk decoders:\n";
	_specs += "  ";
	for (int i = 0; i < Mix_GetNumChunkDecoders(); ++i) {
		_specs += Mix_GetChunkDecoder(i);
		if (i != Mix_GetNumChunkDecoders() - 1)
			_specs += ", ";
	}
	_specs += "\n";

	_specs += "Music decoders:\n";
	_specs += "  ";
	for (int i = 0; i < Mix_GetNumMusicDecoders(); ++i) {
		_specs += Mix_GetMusicDecoder(i);
		if (i != Mix_GetNumMusicDecoders() - 1)
			_specs += ", ";
	}
	_specs += "\n";

	if (confirmTxt)
		_confirmText = confirmTxt;
}

AboutPopupBox::~AboutPopupBox() {
}

void AboutPopupBox::update(void) {
	ImGuiIO &io = GetIO();
	ImGuiStyle &style = ImGui::GetStyle();

	bool isOpen = true;
	bool toConfirm = false;
	bool toCancel = false;

	if (_init.begin())
		OpenPopup(_title);

	if (BeginPopupModal(_title, nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoNav)) {
		Url(BITTY_TITLE, "https://paladin-t.github.io/bitty/");
		SameLine();
		TextUnformatted(_desc);

		{
			VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2());

			TextUnformatted("  by ");
			SameLine();
			Url("Tony Wang", "https://paladin-t.github.io/");
			SameLine();
			TextUnformatted(", 2020 - 2021");
			NewLine();
		}
		Separator();

		InputTextMultiline(
			"",
			(char*)_specs.c_str(), _specs.length(),
			ImVec2(460 * io.FontGlobalScale, 200 * io.FontGlobalScale),
			ImGuiInputTextFlags_ReadOnly
		);

		const char* confirm = _confirmText.c_str();

		if (_confirmText.empty()) {
			confirm = "Ok";
		}

		CentralizeButton();

		if (Button(confirm, ImVec2(WIDGETS_BUTTON_WIDTH, 0)) || IsKeyReleased(SDL_SCANCODE_RETURN) || IsKeyReleased(SDL_SCANCODE_Y)) {
			toConfirm = true;

			CloseCurrentPopup();
		}

		if (IsKeyReleased(SDL_SCANCODE_ESCAPE)) {
			toCancel = true;

			CloseCurrentPopup();
		}

		if (!_init.begin() && !_init.end())
			CentralizeWindow();

		EnsureWindowVisible();

		EndPopup();
	}

	if (isOpen)
		_init.update();

	if (!isOpen)
		toCancel = true;

	if (toConfirm || toCancel) {
		_init.reset();

		if (!_confirmHandler.empty())
			_confirmHandler();
	}
}

PausedPopupBox::PausedPopupBox(
	class Renderer* rnd,
	const ResumeHandler &resume, const OptionsHandler &options, const AboutHandler &about,
	const std::string &resumeTxt, const std::string &optionsTxt, const std::string &aboutTxt
) : _renderer(rnd),
	_resumeHandler(resume), _optionsHandler(options), _aboutHandler(about),
	_resumeText(resumeTxt), _optionsText(optionsTxt), _aboutText(aboutTxt)
{
}

PausedPopupBox::~PausedPopupBox() {
}

void PausedPopupBox::update(void) {
	bool isOpen = true;
	bool isResume = false;
	bool isOptions = false;
	bool isAbout = false;

	if (_init.begin())
		OpenPopup("@Paused");

	const float wndWidth = 384;
	SetNextWindowSize(ImVec2(wndWidth, 0), ImGuiCond_Always);
	SetNextWindowPos(
		ImVec2(
			(_renderer->width() - wndWidth) * 0.5f,
			(_renderer->height() - _windowHeight) * 0.5f
		),
		ImGuiCond_Always
	);
	const ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize;
	if (BeginPopupModal("@Paused", nullptr, flags)) {
		if (Button(_resumeText, ImVec2(wndWidth, 0))) {
			isResume = true;

			CloseCurrentPopup();
		}

		if (Button(_optionsText, ImVec2(wndWidth, 0))) {
			isOptions = true;

			CloseCurrentPopup();
		}

		if (Button(_aboutText, ImVec2(wndWidth, 0))) {
			isAbout = true;

			CloseCurrentPopup();
		}

		_windowHeight = GetWindowHeight();

		EndPopup();
	}

	if (isOpen)
		_init.update();

	if (!isOpen)
		_init.reset();

	if (isResume) {
		_init.reset();

		if (!_resumeHandler.empty())
			_resumeHandler();
	}
	if (isOptions) {
		_init.reset();

		if (!_optionsHandler.empty())
			_optionsHandler();
	}
	if (isAbout) {
		_init.reset();

		if (!_aboutHandler.empty())
			_aboutHandler();
	}
}

}

}

/* ===========================================================================} */
