/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2022 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __WIDGETS_SKETCHBOOK_H__
#define __WIDGETS_SKETCHBOOK_H__

#include "workspace_sketchbook.h"

/*
** {===========================================================================
** Sketchbook widgets
**
** @note Specialized widgets.
*/

namespace ImGui {

namespace Sketchbook {

class PreferencesPopupBox : public PopupBox {
public:
	struct ConfirmHandler : public Handler<ConfirmHandler, void, const WorkspaceSketchbook::SketchbookSettings &> {
		using Handler::Handler;
	};
	struct CancelHandler : public Handler<CancelHandler, void> {
		using Handler::Handler;
	};
	struct ApplyHandler : public Handler<ApplyHandler, void, const WorkspaceSketchbook::SketchbookSettings &> {
		using Handler::Handler;
	};

private:
	class Primitives* _primitives = nullptr;
	class Theme* _theme = nullptr;
	std::string _title;
	WorkspaceSketchbook::SketchbookSettings &_settings;
	WorkspaceSketchbook::SketchbookSettings _settingsShadow;
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
		class Primitives* primitives, class Theme* theme,
		const std::string &title,
		WorkspaceSketchbook::SketchbookSettings &settings,
		bool editable,
		const ConfirmHandler &confirm, const CancelHandler &cancel, const ApplyHandler &applyHandler,
		const char* confirmTxt /* nullable */, const char* cancelTxt /* nullable */, const char* applyTxt /* nullable */
	);
	virtual ~PreferencesPopupBox() override;

	virtual void update(void) override;
};

class AboutPopupBox : public PopupBox {
public:
	struct ConfirmHandler : public Handler<ConfirmHandler, void> {
		using Handler::Handler;
	};

private:
	class Primitives* _primitives = nullptr;
	std::string _title;
	std::string _desc;
	std::string _specs;

	ConfirmHandler _confirmHandler = nullptr;
	std::string _confirmText;

	Initializer _init;

public:
	AboutPopupBox(
		class Window* wnd, class Renderer* rnd, class Primitives* primitives,
		const std::string &title,
		const ConfirmHandler &confirm,
		const char* confirmTxt /* nullable */
	);
	virtual ~AboutPopupBox() override;

	virtual void update(void) override;
};

class PausedPopupBox : public PopupBox {
public:
	struct ResumeHandler : public Handler<ResumeHandler, void> {
		using Handler::Handler;
	};
	struct OptionsHandler : public Handler<OptionsHandler, void> {
		using Handler::Handler;
	};
	struct AboutHandler : public Handler<AboutHandler, void> {
		using Handler::Handler;
	};

private:
	class Renderer* _renderer = nullptr;
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
		class Renderer* rnd,
		const ResumeHandler &resume, const OptionsHandler &options, const AboutHandler &about,
		const std::string &resumeTxt, const std::string &optionsTxt, const std::string &aboutTxt
	);
	virtual ~PausedPopupBox() override;

	virtual void update(void) override;
};

}

}

/* ===========================================================================} */

#endif /* __WIDGETS_SKETCHBOOK_H__ */
