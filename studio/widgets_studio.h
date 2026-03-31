/*
** Bitty
**
** An itty bitty 2D game engine.
**
** Copyright (C) 2020 - 2026 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __WIDGETS_STUDIO_H__
#define __WIDGETS_STUDIO_H__

#include "workspace_studio.h"

/*
** {===========================================================================
** Forward declaration
*/

namespace Bitty {

class Primitives;
class Renderer;
class ThemeStudio;
class Window;

}

/* ===========================================================================} */

/*
** {===========================================================================
** Studio widgets
**
** @note Specialized widgets for Pro version.
*/

namespace ImGui {

namespace Studio {

class PreferencesPopupBox : public PopupBox {
public:
	struct ConfirmHandler : public Bitty::Handler<ConfirmHandler, void, const Bitty::WorkspaceStudio::StudioSettings &> {
		using Handler::Handler;
	};
	struct CancelHandler : public Bitty::Handler<CancelHandler, void> {
		using Handler::Handler;
	};
	struct ApplyHandler : public Bitty::Handler<ApplyHandler, void, const Bitty::WorkspaceStudio::StudioSettings &> {
		using Handler::Handler;
	};

private:
	Bitty::Primitives* _primitives = nullptr;
	Bitty::ThemeStudio* _theme = nullptr;
	std::string _title;
	Bitty::WorkspaceStudio::StudioSettings &_settings;
	Bitty::WorkspaceStudio::StudioSettings _settingsShadow;
	int _activeGamepadIndex = -1;
	int _activeButtonIndex = -1;
	bool _editable = true;

	ConfirmHandler _confirmHandler = nullptr;
	std::string _confirmText;
	CancelHandler _cancelHandler = nullptr;
	std::string _cancelText;
	ApplyHandler _applyHandler = nullptr;
	std::string _applyText;

	Initializer _init;

public:
	PreferencesPopupBox(
		Bitty::Primitives* primitives, Bitty::ThemeStudio* theme,
		const std::string &title,
		Bitty::WorkspaceStudio::StudioSettings &settings,
		bool editable,
		const ConfirmHandler &confirm, const CancelHandler &cancel, const ApplyHandler &applyHandler,
		const char* confirmTxt /* nullable */, const char* cancelTxt /* nullable */, const char* applyTxt /* nullable */
	);
	virtual ~PreferencesPopupBox() override;

	virtual void update(void) override;
};

class AboutPopupBox : public PopupBox {
public:
	struct ConfirmHandler : public Bitty::Handler<ConfirmHandler, void> {
		using Handler::Handler;
	};

protected:
	Bitty::Primitives* _primitives = nullptr;
	std::string _title;
	std::string _prefix;
	std::string _name;
	std::string _desc;
	std::string _specs;

	ConfirmHandler _confirmHandler = nullptr;
	std::string _confirmText;

	Initializer _init;

public:
	AboutPopupBox(
		Bitty::Window* wnd, Bitty::Renderer* rnd, Bitty::Primitives* primitives,
		const std::string &title,
		const ConfirmHandler &confirm,
		const char* confirmTxt /* nullable */
	);
	virtual ~AboutPopupBox() override;

	virtual void update(void) override;
};

class PausedPopupBox : public PopupBox {
public:
	struct ResumeHandler : public Bitty::Handler<ResumeHandler, void> {
		using Handler::Handler;
	};
	struct OptionsHandler : public Bitty::Handler<OptionsHandler, void> {
		using Handler::Handler;
	};
	struct AboutHandler : public Bitty::Handler<AboutHandler, void> {
		using Handler::Handler;
	};

private:
	Bitty::Renderer* _renderer = nullptr;
	float _windowHeight = 0.0f;

	ResumeHandler _resumeHandler = nullptr;
	std::string _resumeText;
	OptionsHandler _optionsHandler = nullptr;
	std::string _optionsText;
	AboutHandler _aboutHandler = nullptr;
	std::string _aboutText;

	Initializer _init;

public:
	PausedPopupBox(
		Bitty::Renderer* rnd,
		const ResumeHandler &resume, const OptionsHandler &options, const AboutHandler &about,
		const std::string &resumeTxt, const std::string &optionsTxt, const std::string &aboutTxt
	);
	virtual ~PausedPopupBox() override;

	virtual void update(void) override;
};

}

}

/* ===========================================================================} */

#endif /* __WIDGETS_STUDIO_H__ */
