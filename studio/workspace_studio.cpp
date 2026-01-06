/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2025 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "loader_studio.h"
#include "theme_studio.h"
#include "widgets_studio.h"
#include "workspace_studio.h"
#include "../src/code.h"
#include "../src/editable.h"
#include "../src/encoding.h"
#include "../src/file_handle.h"
#include "../src/filesystem.h"
#include "../src/operations.h"
#include "../src/platform.h"
#include "../src/primitives.h"
#include "../src/project.h"
#include "../src/recorder.h"
#include "../src/renderer.h"
#include "../src/scripting_lua_api.h"
#include "../src/scripting_lua_api_physics.h"
#include "../src/scripting_lua_api_plugin.h"
#include "../src/scripting_lua_api_promises.h"
#include "scripting_lua_api_studio.h"
#include "../src/window.h"
#include "../lib/imgui/imgui_internal.h"
#include "../lib/imgui_code_editor/imgui_code_editor.h"
#include "../lib/jpath/jpath.hpp"
#include <SDL.h>

/*
** {===========================================================================
** Macros and constants
*/

#ifndef WORKSPACE_AUTORUN_ENABLED
#	define WORKSPACE_AUTORUN_ENABLED 0
#endif /* WORKSPACE_AUTORUN_ENABLED */

#ifndef WORKSPACE_RECENT_FILE_MAX_COUNT
#	define WORKSPACE_RECENT_FILE_MAX_COUNT 10
#endif /* WORKSPACE_RECENT_FILE_MAX_COUNT */

/* ===========================================================================} */

/*
** {===========================================================================
** Studio workspace
*/

WorkspaceStudio::StudioSettings::RecentTouched::RecentTouched() {
}

WorkspaceStudio::StudioSettings::RecentTouched::RecentTouched(Types type_, const std::string &path_) : type(type_), path(path_) {
}

bool WorkspaceStudio::StudioSettings::RecentTouched::operator == (const RecentTouched &other) const {
	return type == other.type && path == other.path;
}

WorkspaceStudio::StudioSettings::StudioSettings() {
}

WorkspaceStudio::StudioSettings &WorkspaceStudio::StudioSettings::operator = (const StudioSettings &other) {
	applicationWindowDisplayIndex = other.applicationWindowDisplayIndex;
	applicationWindowFullscreen = other.applicationWindowFullscreen;
	applicationWindowMaximized = other.applicationWindowMaximized;
	applicationWindowPosition = other.applicationWindowPosition;
	applicationWindowSize = other.applicationWindowSize;
	applicationPauseOnFocusLost = other.applicationPauseOnFocusLost;
	applicationPauseOnEsc = other.applicationPauseOnEsc;

	projectPreference = other.projectPreference;
	projectIgnoreDotFiles = other.projectIgnoreDotFiles;
	projectLoadLastProjectAtStartup = other.projectLoadLastProjectAtStartup;
	projectAutoBackup = other.projectAutoBackup;

	bannerVisible = other.bannerVisible;
	assetsVisible = other.assetsVisible;

	editorIndentRule = other.editorIndentRule;
	editorColumnIndicator = other.editorColumnIndicator;
	editorShowWhiteSpaces = other.editorShowWhiteSpaces;
	editorCaseSensitive = other.editorCaseSensitive;
	editorMatchWholeWord = other.editorMatchWholeWord;
	editorGlobalSearch = other.editorGlobalSearch;
	editorAlwaysShowTransparentBackground = other.editorAlwaysShowTransparentBackground;
	editorAlwaysShowGrids = other.editorAlwaysShowGrids;

	canvasState = other.canvasState;
	canvasFixRatio = other.canvasFixRatio;

	debugVisible = other.debugVisible;

	consoleVisible = other.consoleVisible;
	consoleClearOnStart = other.consoleClearOnStart;

	for (int i = 0; i < INPUT_GAMEPAD_COUNT; ++i)
		inputGamepads[i] = other.inputGamepads[i];
	inputOnscreenGamepadEnabled = other.inputOnscreenGamepadEnabled;
	inputOnscreenGamepadSwapAB = other.inputOnscreenGamepadSwapAB;
	inputOnscreenGamepadScale = other.inputOnscreenGamepadScale;
	inputOnscreenGamepadPadding = other.inputOnscreenGamepadPadding;

	themeStyle = other.themeStyle;

	recentTouched = other.recentTouched;

	return *this;
}

bool WorkspaceStudio::StudioSettings::operator != (const StudioSettings &other) const {
	for (int i = 0; i < INPUT_GAMEPAD_COUNT; ++i) {
		if (inputGamepads[i] != other.inputGamepads[i])
			return true;
	}
	if (inputOnscreenGamepadEnabled != other.inputOnscreenGamepadEnabled ||
		inputOnscreenGamepadSwapAB != other.inputOnscreenGamepadSwapAB ||
		inputOnscreenGamepadScale != other.inputOnscreenGamepadScale ||
		inputOnscreenGamepadPadding != other.inputOnscreenGamepadPadding
	) {
		return true;
	}

	if (applicationWindowDisplayIndex != other.applicationWindowDisplayIndex ||
		applicationWindowFullscreen != other.applicationWindowFullscreen ||
		applicationWindowMaximized != other.applicationWindowMaximized ||
		applicationWindowPosition != other.applicationWindowPosition ||
		applicationWindowSize != other.applicationWindowSize ||
		applicationPauseOnFocusLost != other.applicationPauseOnFocusLost ||
		applicationPauseOnEsc != other.applicationPauseOnEsc
	) {
		return true;
	}

	if (projectPreference != other.projectPreference ||
		projectIgnoreDotFiles != other.projectIgnoreDotFiles ||
		projectLoadLastProjectAtStartup != other.projectLoadLastProjectAtStartup ||
		projectAutoBackup != other.projectAutoBackup
	) {
		return true;
	}

	if (bannerVisible != other.bannerVisible)
		return true;
	if (assetsVisible != other.assetsVisible)
		return true;

	if (editorIndentRule != other.editorIndentRule ||
		editorColumnIndicator != other.editorColumnIndicator ||
		editorShowWhiteSpaces != other.editorShowWhiteSpaces ||
		editorCaseSensitive != other.editorCaseSensitive ||
		editorMatchWholeWord != other.editorMatchWholeWord ||
		editorGlobalSearch != other.editorGlobalSearch ||
		editorAlwaysShowTransparentBackground != other.editorAlwaysShowTransparentBackground ||
		editorAlwaysShowGrids != other.editorAlwaysShowGrids
	) {
		return true;
	}

	if (canvasState != other.canvasState ||
		canvasFixRatio != other.canvasFixRatio
	) {
		return true;
	}

	if (debugVisible != other.debugVisible)
		return true;

	if (consoleVisible != other.consoleVisible || 
		consoleClearOnStart != other.consoleClearOnStart
	) {
		return true;
	}

	if (themeStyle != other.themeStyle)
		return true;

	// `recentTouched` is ignored here.

	return false;
}

WorkspaceStudio::WorkspaceStudio() {
	_theme = new ThemeStudio();

	_loader = new LoaderStudio();
}

WorkspaceStudio::~WorkspaceStudio() {
	delete _loader;
	_loader = nullptr;

	delete _theme;
	_theme = nullptr;
}

bool WorkspaceStudio::open(class Window* wnd, class Renderer* rnd, const class Project* project, Executable* exec, class Primitives* primitives, unsigned fps, const Text::Dictionary &options) {
	if (_opened)
		return false;
	_opened = true;

#if defined BITTY_DEBUG
	wnd->title(BITTY_TITLE " v" BITTY_VERSION_STRING " [DEBUG]");
#else /* BITTY_DEBUG */
	wnd->title(BITTY_TITLE " v" BITTY_VERSION_STRING);
#endif /* BITTY_DEBUG */

	beginSplash(wnd, rnd, project);

	_theme->open(rnd);
	_theme->load(rnd);

	ImGuiStyle &style = ImGui::GetStyle();
	memcpy(style.Colors, _theme->style()->builtin, sizeof(style.Colors));

	do {
		LockGuard<RecursiveMutex>::UniquePtr acquired;
		Project* prj = project->acquire(acquired);
		if (!prj)
			break;

		prj->loader(_loader);

		prj->factory(
			Project::Factory(
				[] (Project* project) -> Asset* {
					return new Asset(project);
				},
				[] (Asset* asset) -> void {
					delete asset;
				}
			)
		);
	} while (false);

	if (!Workspace::open(wnd, rnd, project, exec, primitives, fps, options)) {
		endSplash(wnd, rnd, options);

		return false;
	}

	loadProject(wnd, rnd, project, exec, primitives, options);

	_theme->styleIndex((Theme::Styles)_settings.themeStyle);

	switch (_settings.themeStyle) {
	case Theme::DARK:
		consoleTextBox()->SetPalette(ImGui::CodeEditor::GetDarkPalette());

		break;
	case Theme::CLASSIC:
		consoleTextBox()->SetPalette(ImGui::CodeEditor::GetRetroBluePalette());

		break;
	case Theme::LIGHT:
		consoleTextBox()->SetPalette(ImGui::CodeEditor::GetLightPalette());

		break;
	}

	endSplash(wnd, rnd, options);

	Operations::fileRestore(rnd, this, project);

	const std::string ready = _theme->generic_Ready() + '\n';
	print(ready.c_str());

#if defined BITTY_OS_LINUX
	const bool hasFileDialog = Platform::checkProgram("zenity") || Platform::checkProgram("kdialog") || Platform::checkProgram("matedialog") || Platform::checkProgram("qarma");
	if (!hasFileDialog) {
		const std::string msg_ = "Requires zenity/matedialog/qarma/kdialog to show file dialogs.\n(To open or save something.)";
		messagePopupBox(
			msg_,
			nullptr,
			nullptr,
			nullptr
		);
	}
#endif /* BITTY_OS_LINUX */

	return true;
}

bool WorkspaceStudio::close(class Window* wnd, class Renderer* rnd, const class Project* project, Executable* exec) {
	if (!_opened)
		return false;
	_opened = false;

	Operations::fileClean(rnd, this, project);

	unloadProject(project, exec);

	Workspace::close(wnd, rnd, project, exec);

	do {
		LockGuard<RecursiveMutex>::UniquePtr acquired;
		Project* prj = project->acquire(acquired);
		if (!prj)
			break;

		prj->loader(nullptr);
	} while (false);

	_theme->save();
	_theme->close(rnd);

	return true;
}

const WorkspaceStudio::Settings* WorkspaceStudio::settings(void) const {
	return &_settings;
}

WorkspaceStudio::Settings* WorkspaceStudio::settings(void) {
	return &_settings;
}

class Theme* WorkspaceStudio::theme(void) const {
	return _theme;
}

bool WorkspaceStudio::prefer2XScaleForBigDisplay(void) const {
	return true;
}

void WorkspaceStudio::touchedFile(const char* path) {
	addRecentTouched(StudioSettings::RecentTouched::FILE, path);
}

void WorkspaceStudio::touchedDirectory(const char* path) {
	addRecentTouched(StudioSettings::RecentTouched::DIRECTORY, path);
}

void WorkspaceStudio::touchedExample(const char* path) {
	addRecentTouched(StudioSettings::RecentTouched::EXAMPLE, path);
}

bool WorkspaceStudio::load(class Window* wnd, class Renderer* rnd, const class Project* project, class Primitives* primitives) {
	const std::string pref = Path::writableDirectory();
	const std::string fn = std::string(WORKSPACE_PREFERENCES_NAME) + std::string("." BITTY_JSON_EXT);
	const std::string path = Path::combine(pref.c_str(), fn.c_str());

	rapidjson::Document doc;
	File::Ptr file(File::create());
	if (file->open(path.c_str(), Stream::READ)) {
		std::string buf;
		file->readString(buf);
		file->close();
		if (!Json::fromString(doc, buf.c_str()))
			doc.SetNull();
	}
	file.reset();

	if (!load(wnd, rnd, project, primitives, doc))
		return false;

	return true;
}

bool WorkspaceStudio::save(class Window* wnd, class Renderer* rnd, const class Project* project, class Primitives* primitives) {
	rapidjson::Document doc;

	if (!save(wnd, rnd, project, primitives, doc))
		return false;

	const std::string pref = Path::writableDirectory();
	const std::string fn = std::string(WORKSPACE_PREFERENCES_NAME) + std::string("." BITTY_JSON_EXT);
	const std::string path = Path::combine(pref.c_str(), fn.c_str());
	File::Ptr file(File::create());
	if (file->open(path.c_str(), Stream::WRITE)) {
		std::string buf;
		Json::toString(doc, buf);
		file->writeString(buf);

		file->close();
	}
	file.reset();

	return true;
}

unsigned WorkspaceStudio::update(class Window* wnd, class Renderer* rnd, const class Project* project, Executable* exec, class Primitives* primitives, double delta, unsigned fps, bool alive, bool* indicated) {
	// Prepare.
	unsigned result = 0;

	execute(wnd, rnd, project, exec, primitives, delta, alive);
	refresh(wnd, rnd, project, exec, primitives);

	prepare(wnd, rnd, project, exec, primitives);
	shortcuts(wnd, rnd, project, exec, primitives);

	// Dialog boxes.
	dialog(wnd, rnd, project);

	// Head.
	{
		menu(wnd, rnd, project, exec, primitives);
		banner(wnd, rnd, project, exec, primitives);
	}

	// Body.
	{
		assets(wnd, rnd, project, exec, primitives);

		const float assetW = *assetsVisible() ? assetsWidth() : 0.0f;
		bodyArea(Rect(assetW, menuHeight() + bannerHeight(), (float)rnd->width(), (float)rnd->height()));

		editing(wnd, rnd, project, exec, primitives, delta, indicated);
#if BITTY_DEBUG_ENABLED
		debug(wnd, rnd, project, exec, primitives, fps);
#else /* BITTY_DEBUG_ENABLED */
		(void)fps;
#endif /* BITTY_DEBUG_ENABLED */
		if (canvas(wnd, rnd, project, exec, primitives, delta, indicated))
			result = activeFrameRate();
		console(wnd, rnd, project);
		promise(wnd, rnd, project);
	}

	// Plugins.
	plugins(wnd, rnd, project, delta);

	// Finish.
	finish(wnd, rnd, project);

	return result;
}

void WorkspaceStudio::require(Executable* exec) {
	const Executable::Languages lang = exec->language();
	if (lang & Executable::LUA) {
		// Common.
		Lua::Standard::open(exec);
		Lua::Libs::open(exec);
		if (exec->primitives()) {
			Lua::Engine::open(exec);
			Lua::Editor::open(exec);
		}
		Lua::Invoke::open(exec);
		Lua::Application::open(exec);

		// Physics.
		if (exec->primitives()) {
			Lua::Engine::physics(exec);
		}

		// Promise.
		Lua::Standard::promise(exec);
		Lua::Libs::promise(exec);

		// Studio.
		if (exec->primitives()) {
			Lua::Libs::studio(exec);
			Lua::Engine::studio(exec);
			Lua::Application::studio(exec);
		}
		if (exec->editing()) {
			Lua::Application::plugin(exec);
			Lua::Editor::plugin(exec);
			Lua::Libs::studio(exec);
			Lua::Studio::studio(exec);
		}
	}
	if (lang & Executable::NATIVE) {
		// Do nothing.
	}
}

void WorkspaceStudio::focusGained(class Window* wnd, class Renderer* rnd, const class Project* project, Executable* exec, class Primitives* primitives) {
	Workspace::focusGained(wnd, rnd, project, exec, primitives);

	exec->focusGained();
}

void WorkspaceStudio::focusLost(class Window* wnd, class Renderer* rnd, const class Project* project, Executable* exec, class Primitives* primitives) {
	Workspace::focusLost(wnd, rnd, project, exec, primitives);

	exec->focusLost();

	if (!canvasFull() || !_settings.applicationPauseOnFocusLost)
		return;

	if (popupBox())
		return;

	showPaused(wnd, rnd, project, primitives);
}

void WorkspaceStudio::renderTargetsReset(class Window* wnd, class Renderer* rnd, const class Project* project, Executable* exec, class Primitives* primitives) {
	Workspace::renderTargetsReset(wnd, rnd, project, exec, primitives);

	exec->renderTargetsReset();
}

void WorkspaceStudio::fileDropped(class Window* wnd, class Renderer* rnd, const char* const path) {
	Workspace::fileDropped(wnd, rnd, path);

	if (path)
		_droppedFiles.push_back(path);
}

void WorkspaceStudio::dropBegan(class Window* wnd, class Renderer* rnd) {
	Workspace::dropBegan(wnd, rnd);

	_droppedFiles.clear();
}

void WorkspaceStudio::dropEndded(class Window* wnd, class Renderer* rnd, Executable* exec) {
	Workspace::dropEndded(wnd, rnd, exec);

	exec->fileDropped(_droppedFiles);

	_droppedFiles.clear();
}

bool WorkspaceStudio::load(class Window* wnd, class Renderer* rnd, const class Project* project, class Primitives* primitives, const rapidjson::Document &doc) {
	if (!Workspace::load(wnd, rnd, project, primitives, doc))
		return false;

	Jpath::get(doc, _settings.themeStyle, "theme", "style");

	_settings.recentTouched.clear();
	for (int i = 0; i < WORKSPACE_RECENT_FILE_MAX_COUNT; ++i) {
		unsigned type = 0;
		std::string path;
		if (!Jpath::get(doc, type, "file", "recent", i, "type") || !Jpath::get(doc, path, "file", "recent", i, "path"))
			continue;

		_settings.recentTouched.push_back(StudioSettings::RecentTouched((StudioSettings::RecentTouched::Types)type, path));
	}

	return true;
}

bool WorkspaceStudio::save(class Window* wnd, class Renderer* rnd, const class Project* project, class Primitives* primitives, rapidjson::Document &doc) {
	if (!Workspace::save(wnd, rnd, project, primitives, doc))
		return false;

	Jpath::set(doc, doc, _settings.themeStyle, "theme", "style");

	for (int i = 0; i < WORKSPACE_RECENT_FILE_MAX_COUNT && i < (int)_settings.recentTouched.size(); ++i) {
		const unsigned type = (unsigned)_settings.recentTouched[i].type;
		const std::string &path = _settings.recentTouched[i].path;
		Jpath::set(doc, doc, type, "file", "recent", i, "type");
		Jpath::set(doc, doc, path, "file", "recent", i, "path");
	}

	return true;
}

void WorkspaceStudio::loadProject(class Window* wnd, class Renderer* rnd, const class Project* project, Executable* exec, class Primitives* primitives, const Text::Dictionary &options) {
	promise::Defer start;

	Text::Dictionary::const_iterator pathOpt = options.find(WORKSPACE_OPTION_APPLICATION_DEFAULT_KEY); // The non-flag option indicates initial directory/file path.
	if (pathOpt == options.end()) { // No initial path specified.
#if WORKSPACE_AUTORUN_ENABLED
		auto run = [] (WorkspaceStudio* self, Window* wnd, Renderer* rnd, const Project* project, Executable* exec, Primitives* primitives, const char* name) -> void {
			do {
				LockGuard<RecursiveMutex>::UniquePtr acquired;
				Project* prj = project->acquire(acquired);
				if (!prj)
					break;

				wnd->title(prj->title().c_str());

				prj->readonly(true);

				Asset* configAsset = prj->get(WORKSPACE_CONFIG_NAME "." BITTY_JSON_EXT);
				if (!configAsset)
					break;

				configAsset->prepare(Asset::RUNNING, true);
				Object::Ptr obj = configAsset->object(Asset::RUNNING);
				if (!obj) {
					configAsset->finish(Asset::RUNNING, true);

					break;
				}
				Json::Ptr json = Object::as<Json::Ptr>(obj);
				if (!json) {
					configAsset->finish(Asset::RUNNING, true);

					break;
				}
				rapidjson::Document doc;
				if (!json->toJson(doc)) {
					configAsset->finish(Asset::RUNNING, true);

					break;
				}

				self->load(wnd, rnd, project, primitives, doc);

				json = nullptr;
				obj = nullptr;
				configAsset->finish(Asset::RUNNING, true);
				configAsset = nullptr;
				prj = nullptr;
			} while (false);

			self->autorun(name);

			self->canvasFull(true);

			do {
				LockGuard<decltype(self->consoleLock())> guard(self->consoleLock());

				self->consoleEnabled(false);
			} while (false);

			Operations::projectRun(rnd, self, project, exec, primitives);
		};

		if (Path::existsDirectory(WORKSPACE_AUTORUN_PROJECT_DIR WORKSPACE_AUTORUN_PROJECT_NAME)) {
			// Open the autorun directory.
			start = Operations::fileOpenDirectory(rnd, this, project, exec, WORKSPACE_AUTORUN_PROJECT_DIR WORKSPACE_AUTORUN_PROJECT_NAME)
				.then(
					[this, wnd, rnd, project, exec, primitives, run] (void) -> void {
						run(
							this,
							wnd, rnd,
							project,
							exec,
							primitives,
							WORKSPACE_AUTORUN_PROJECT_DIR WORKSPACE_AUTORUN_PROJECT_NAME
						);
					}
				);
		} else if (Path::existsFile(WORKSPACE_AUTORUN_PROJECT_DIR WORKSPACE_AUTORUN_PROJECT_NAME "." BITTY_PROJECT_EXT)) {
			// Open the autorun file.
			start = Operations::fileOpenFile(rnd, this, project, exec, WORKSPACE_AUTORUN_PROJECT_DIR WORKSPACE_AUTORUN_PROJECT_NAME "." BITTY_PROJECT_EXT)
				.then(
					[this, wnd, rnd, project, exec, primitives, run] (void) -> void {
						run(
							this,
							wnd, rnd,
							project,
							exec,
							primitives,
							WORKSPACE_AUTORUN_PROJECT_DIR WORKSPACE_AUTORUN_PROJECT_NAME "." BITTY_PROJECT_EXT
						);
					}
				);
		}
#else /* WORKSPACE_AUTORUN_ENABLED */
		(void)wnd;
		(void)primitives;
#endif /* WORKSPACE_AUTORUN_ENABLED */

		// Open the last project.
		if (!start && _settings.projectLoadLastProjectAtStartup) {
			for (int i = 0; i < WORKSPACE_RECENT_FILE_MAX_COUNT && i < (int)_settings.recentTouched.size(); ++i) {
				const StudioSettings::RecentTouched::Types type = _settings.recentTouched[i].type;
				const std::string &path = _settings.recentTouched[i].path;
				switch (type) {
				case StudioSettings::RecentTouched::EXAMPLE:
					if (Path::existsFile(path.c_str()))
						start = Operations::fileOpenExample(rnd, this, project, exec, path.c_str());

					break;
				case StudioSettings::RecentTouched::FILE:
					if (Path::existsFile(path.c_str()))
						start = Operations::fileOpenFile(rnd, this, project, exec, path.c_str());

					break;
				case StudioSettings::RecentTouched::DIRECTORY:
					if (Path::existsDirectory(path.c_str()))
						start = Operations::fileOpenDirectory(rnd, this, project, exec, path.c_str());

					break;
				}

				if (start)
					break;
			}
		}

		// Rejection.
		if (!start)
			start = promise::newPromise([] (promise::Defer df) -> void { df.reject(); });
	} else { // Initial path specified.
		// Open the initial directory or file.
		std::string path = pathOpt->second;
		path = Unicode::fromOs(path);
		start = Path::existsDirectory(path.c_str()) ?
			Operations::fileOpenDirectory(rnd, this, project, exec, path.c_str()) :
			Operations::fileOpenFile(rnd, this, project, exec, path.c_str());
	}

	start
		.fail(
			[rnd, this, project, exec] (void) -> void {
				// Create a new project.
				Operations::fileNew(rnd, this, project, exec);
			}
		);
}

void WorkspaceStudio::unloadProject(const class Project* project, Executable* exec) {
	do {
		LockGuard<decltype(consoleLock())> guard(consoleLock());

		consoleEnabled(true);
	} while (false);

	canvasFull(false);

	autorun(nullptr);

	exec->clearBreakpoints(nullptr);

	LockGuard<RecursiveMutex>::UniquePtr acquired;
	Project* prj = project->acquire(acquired);
	if (!prj)
		return;

	prj->unload();
	prj->readonly(false);
}

void WorkspaceStudio::refresh(class Window* wnd, class Renderer*, const class Project*, Executable*, class Primitives*) {
	if (toRefreshWindowTitle()) {
		toRefreshWindowTitle(false);
		wnd->title(toRefreshWindowTitleContent().c_str());
	}
}

void WorkspaceStudio::addRecentTouched(StudioSettings::RecentTouched::Types type, const char* path) {
	if (!path)
		return;

	StudioSettings::RecentTouched record;
	switch (type) {
	case StudioSettings::RecentTouched::FILE:
		record.type = type;
		record.path = path;

		break;
	case StudioSettings::RecentTouched::DIRECTORY:
		record.type = type;
		record.path = path;

		break;
	case StudioSettings::RecentTouched::EXAMPLE:
		record.type = type;
		record.path = path;

		break;
	default:
		return;
	}
	StudioSettings::RecentTouched::Array::iterator it = std::remove(_settings.recentTouched.begin(), _settings.recentTouched.end(), record);
	if (it != _settings.recentTouched.end())
		_settings.recentTouched.erase(it, _settings.recentTouched.end());
	_settings.recentTouched.insert(_settings.recentTouched.begin(), record);
	if (_settings.recentTouched.size() > WORKSPACE_RECENT_FILE_MAX_COUNT)
		_settings.recentTouched.pop_back();
}

void WorkspaceStudio::openRecentTouched(class Window*, class Renderer* rnd, const class Project* project, Executable* exec, class Primitives* primitives, StudioSettings::RecentTouched::Types type, int idx, const char* path) {
	bool exists = true;
	switch (type) {
	case StudioSettings::RecentTouched::EXAMPLE:
		exists = Path::existsFile(path);
		if (exists) {
			Operations::projectStop(rnd, this, project, exec, primitives);

			Operations::fileOpenExample(rnd, this, project, exec, path);
		}

		break;
	case StudioSettings::RecentTouched::FILE:
		exists = Path::existsFile(path);
		if (exists) {
			Operations::projectStop(rnd, this, project, exec, primitives);

			Operations::fileOpenFile(rnd, this, project, exec, path);
		}

		break;
	case StudioSettings::RecentTouched::DIRECTORY:
		exists = Path::existsDirectory(path);
		if (exists) {
			Operations::projectStop(rnd, this, project, exec, primitives);

			Operations::fileOpenDirectory(rnd, this, project, exec, path);
		}

		break;
	default:
		exists = false;

		break;
	}

	if (!exists) {
		const std::string &msg = _theme->dialogPrompt_PathDoesntExistRemoveThisRecord();
		ImGui::MessagePopupBox::ConfirmHandler confirm = ImGui::MessagePopupBox::ConfirmHandler(
			[&, idx] (void) -> void {
				_settings.recentTouched.erase(_settings.recentTouched.begin() + idx);

				popupBox(nullptr);
			},
			nullptr
		);
		ImGui::MessagePopupBox::DenyHandler deny = ImGui::MessagePopupBox::DenyHandler(
			[&] (void) -> void {
				popupBox(nullptr);
			},
			nullptr
		);
		const char* confirmText = _theme->generic_Yes().c_str();
		const char* denyText = _theme->generic_No().c_str();

		messagePopupBox(
			msg,
			confirm, deny, nullptr,
			confirmText, denyText, nullptr
		);
	}
}

void WorkspaceStudio::shortcuts(class Window* wnd, class Renderer* rnd, const class Project* project, Executable* exec, class Primitives* primitives) {
	// Prepare.
	ImGuiIO &io = ImGui::GetIO();

	const bool esc = ImGui::IsKeyPressed(SDL_SCANCODE_ESCAPE);

	do {
		if (!esc)
			break;
		if (!canvasFull() || !_settings.applicationPauseOnEsc)
			break;

		if (popupBox())
			break;

		showPaused(wnd, rnd, project, primitives);

		return;
	} while (false);

	if (!canUseShortcuts())
		return;

	bool prjPersisted = false;
	projectStates(project, nullptr, &prjPersisted, nullptr, nullptr);

	// Get key states.
	const bool f1       = ImGui::IsKeyPressed(SDL_SCANCODE_F1);
	const bool f3       = ImGui::IsKeyPressed(SDL_SCANCODE_F3);
	const bool f5       = ImGui::IsKeyPressed(SDL_SCANCODE_F5);
	const bool f6       = ImGui::IsKeyPressed(SDL_SCANCODE_F6);
	const bool f7       = ImGui::IsKeyPressed(SDL_SCANCODE_F7);
	const bool f8       = ImGui::IsKeyPressed(SDL_SCANCODE_F8);
	const bool f9       = ImGui::IsKeyPressed(SDL_SCANCODE_F9);
	const bool a        = ImGui::IsKeyPressed(SDL_SCANCODE_A);
	const bool c        = ImGui::IsKeyPressed(SDL_SCANCODE_C);
	const bool e        = ImGui::IsKeyPressed(SDL_SCANCODE_E);
	const bool f        = ImGui::IsKeyPressed(SDL_SCANCODE_F);
	const bool g        = ImGui::IsKeyPressed(SDL_SCANCODE_G);
	const bool n        = ImGui::IsKeyPressed(SDL_SCANCODE_N);
	const bool o        = ImGui::IsKeyPressed(SDL_SCANCODE_O);
	const bool q        = ImGui::IsKeyPressed(SDL_SCANCODE_Q);
	const bool r        = ImGui::IsKeyPressed(SDL_SCANCODE_R);
	const bool s        = ImGui::IsKeyPressed(SDL_SCANCODE_S);
	const bool v        = ImGui::IsKeyPressed(SDL_SCANCODE_V);
	const bool w        = ImGui::IsKeyPressed(SDL_SCANCODE_W);
	const bool x        = ImGui::IsKeyPressed(SDL_SCANCODE_X);
	const bool y        = ImGui::IsKeyPressed(SDL_SCANCODE_Y);
	const bool z        = ImGui::IsKeyPressed(SDL_SCANCODE_Z);
	const bool tab      = ImGui::IsKeyPressed(SDL_SCANCODE_TAB);
	const bool period   = ImGui::IsKeyPressed(SDL_SCANCODE_PERIOD);
	const bool slash    = ImGui::IsKeyPressed(SDL_SCANCODE_SLASH);
	const bool del      = ImGui::IsKeyPressed(SDL_SCANCODE_DELETE);
	const bool up       = ImGui::IsKeyPressed(SDL_SCANCODE_UP);
	const bool down     = ImGui::IsKeyPressed(SDL_SCANCODE_DOWN);
	const bool pgup     = ImGui::IsKeyPressed(SDL_SCANCODE_PAGEUP);
	const bool pgdown   = ImGui::IsKeyPressed(SDL_SCANCODE_PAGEDOWN);
#if WORKSPACE_MODIFIER_KEY == WORKSPACE_MODIFIER_KEY_CTRL
	const bool modifier = io.KeyCtrl;
#elif WORKSPACE_MODIFIER_KEY == WORKSPACE_MODIFIER_KEY_CMD
	const bool modifier = io.KeySuper;
#endif /* WORKSPACE_MODIFIER_KEY */

	bool toRun = false;
	bool toStop = false;
	bool toResume = false;
	switch (currentState()) {
	case Executable::READY:
		toRun = (f5 && !io.KeyShift) || (r && modifier);

		break;
	case Executable::RUNNING:
		toStop = (f5 && io.KeyShift) || (period && modifier);

		break;
	case Executable::PAUSED:
		toStop = (f5 && io.KeyShift) || (period && modifier);
		toResume = (f5 && !io.KeyShift);

		break;
	default: // Do nothing.
		break;
	}

	// File operations.
	if (n && modifier && !io.KeyShift) {
		Operations::projectStop(rnd, this, project, exec, primitives);

		Operations::fileNew(rnd, this, project, exec);
	} else if (o && modifier && !io.KeyShift) {
		Operations::projectStop(rnd, this, project, exec, primitives);

		Operations::fileOpenFile(rnd, this, project, exec);
	} else if (o && modifier && io.KeyShift) {
		Operations::projectStop(rnd, this, project, exec, primitives);

		Operations::fileOpenDirectory(rnd, this, project, exec);
	} else if (s && modifier) {
		Operations::fileSaveAsset(rnd, this, project, assetsEditingIndex());
	} else if (s && modifier && io.KeyShift) {
		do {
			LockGuard<RecursiveMutex>::UniquePtr acquired;
			Project* prj = project->acquire(acquired);
			if (!prj)
				break;

			if (prj->archived())
				Operations::fileSaveFile(rnd, this, project, false);
			else
				Operations::fileSaveDirectory(rnd, this, project, false);
		} while (false);
	}

	// Edit operations.
	if (z && modifier) {
		withEditingAsset(
			project,
			[] (Asset* asset, Editable* editor) -> void {
				editor->undo(asset);
			}
		);
	} else if (y && modifier) {
		withEditingAsset(
			project,
			[] (Asset* asset, Editable* editor) -> void {
				editor->redo(asset);
			}
		);
	} else if (c && modifier && !assetsFocused() && !canvasFocused() && !consoleFocused()) {
		withEditingAsset(
			project,
			[] (Asset*, Editable* editor) -> void {
				editor->copy();
			}
		);
	} else if (x && modifier && !assetsFocused() && !canvasFocused() && !consoleFocused()) {
		withEditingAsset(
			project,
			[] (Asset*, Editable* editor) -> void {
				editor->cut();
			}
		);
	} else if (v && modifier && !assetsFocused() && !canvasFocused() && !consoleFocused()) {
		withEditingAsset(
			project,
			[] (Asset*, Editable* editor) -> void {
				editor->paste();
			}
		);
	} else if (del && !assetsFocused() && !canvasFocused() && !consoleFocused()) {
		withEditingAsset(
			project,
			[] (Asset*, Editable* editor) -> void {
				editor->del();
			}
		);
	} else if (a && modifier && !assetsFocused() && !canvasFocused() && !consoleFocused()) {
		withEditingAsset(
			project,
			[] (Asset*, Editable* editor) -> void {
				editor->post(Editable::SELECT_ALL);
			}
		);
	} else if (q && modifier && !io.KeyShift && !assetsFocused() && !canvasFocused() && !consoleFocused()) {
		withEditingAsset(
			project,
			[] (Asset*, Editable* editor) -> void {
				editor->post(Editable::SELECT_WORD);
			}
		);
	} else if (tab && !io.KeyCtrl && !io.KeyShift && !assetsFocused() && !canvasFocused() && !consoleFocused()) {
		withEditingAsset(
			project,
			[] (Asset*, Editable* editor) -> void {
				editor->post(Editable::INDENT, true);
			}
		);
	} else if (tab && io.KeyShift && !assetsFocused() && !canvasFocused() && !consoleFocused()) {
		withEditingAsset(
			project,
			[] (Asset*, Editable* editor) -> void {
				editor->post(Editable::UNINDENT, true);
			}
		);
	} else if (slash && modifier && !io.KeyShift && !assetsFocused() && !canvasFocused() && !consoleFocused()) {
		withEditingAsset(
			project,
			[] (Asset*, Editable* editor) -> void {
				editor->post(Editable::TOGGLE_COMMENT);
			}
		);
	} else if (up && !modifier && !io.KeyShift && io.KeyAlt && !assetsFocused() && !canvasFocused() && !consoleFocused()) {
		withEditingAsset(
			project,
			[] (Asset*, Editable* editor) -> void {
				editor->post(Editable::MOVE_UP);
			}
		);
	} else if (down && !modifier && !io.KeyShift && io.KeyAlt && !assetsFocused() && !canvasFocused() && !consoleFocused()) {
		withEditingAsset(
			project,
			[] (Asset*, Editable* editor) -> void {
				editor->post(Editable::MOVE_DOWN);
			}
		);
	} else if (f && modifier && !canvasFocused() && !consoleFocused()) {
		withEditingAsset(
			project,
			[] (Asset*, Editable* editor) -> void {
				editor->post(Editable::FIND);
			}
		);
	} else if (f3 && !io.KeyShift && !canvasFocused() && !consoleFocused()) {
		withEditingAsset(
			project,
			[] (Asset*, Editable* editor) -> void {
				editor->post(Editable::FIND_NEXT);
			}
		);
	} else if (f3 && io.KeyShift && !canvasFocused() && !consoleFocused()) {
		withEditingAsset(
			project,
			[] (Asset*, Editable* editor) -> void {
				editor->post(Editable::FIND_PREVIOUS);
			}
		);
	} else if (g && modifier && !canvasFocused() && !consoleFocused()) {
		withEditingAsset(
			project,
			[] (Asset*, Editable* editor) -> void {
				editor->post(Editable::GOTO);
			}
		);
	} else if (pgup && modifier && !io.KeyShift) {
		do {
			LockGuard<RecursiveMutex>::UniquePtr acquired;
			Project* prj = project->acquire(acquired);
			if (!prj)
				break;

			Asset::List::Index index = assetsEditingIndex();
			if (index == -1)
				index = 0;
			Asset* asset = prj->get(index);
			if (!asset)
				break;

			prj->foreach(
				[&] (Asset* &asset_, Asset::List::Index index_) -> void {
					if (asset == asset_)
						index = index_;
				}
			);
			if (--index < 0)
				index = prj->count() - 1;
			asset = prj->get(index);
			if (!asset)
				break;

			Asset::States* states = asset->states();
			const Asset::States::Activity activity = states->activity();
			if (activity == Asset::States::CLOSED)
				states->activate(Asset::States::INSPECTABLE);

			states->focus();
		} while (false);
	} else if (pgdown && modifier && !io.KeyShift) {
		do {
			LockGuard<RecursiveMutex>::UniquePtr acquired;
			Project* prj = project->acquire(acquired);
			if (!prj)
				break;

			Asset::List::Index index = assetsEditingIndex();
			if (index == -1)
				index = 0;
			Asset* asset = prj->get(index);
			if (!asset)
				break;

			prj->foreach(
				[&] (Asset* &asset_, Asset::List::Index index_) -> void {
					if (asset == asset_)
						index = index_;
				}
			);
			if (++index >= prj->count())
				index = 0;
			asset = prj->get(index);
			if (!asset)
				break;

			Asset::States* states = asset->states();
			const Asset::States::Activity activity = states->activity();
			if (activity == Asset::States::CLOSED)
				states->activate(Asset::States::INSPECTABLE);

			states->focus();
		} while (false);
	} else if (tab && io.KeyCtrl && !io.KeyShift) {
		Operations::editSwitchAsset(rnd, this, project);
	} else if (w && io.KeyCtrl) {
		editingClosing(true);
	}

	// Project operations.
	if (e && modifier) {
		assetsFiltering(!assetsFiltering());
		assetsFilteringInitialized(false);
	} else if (n && modifier && io.KeyShift) {
		Operations::projectAddAsset(rnd, this, project, assetsSelectedIndex());
	} else if (a && modifier && io.KeyShift) {
		Operations::projectAddFile(rnd, this, project, assetsSelectedIndex());
	} else if (r && modifier && io.KeyShift) {
		if (prjPersisted) {
			Operations::projectStop(rnd, this, project, exec, primitives);

			Operations::projectReload(rnd, this, project, exec);
		}
	} else {
		if (toRun)
			Operations::projectRun(rnd, this, project, exec, primitives);
		if (toStop)
			Operations::projectStop(rnd, this, project, exec, primitives);
	}

	// Debug operations.
	if (toResume)
		Operations::debugContinue(this, project, exec);
	else if (f9)
		Operations::debugToggleBreakpoint(this, project, exec);

	// Window operations.
	if (f6 && !recorder()->recording())
		recorder()->start(1);
	else if (f7 && !recorder()->recording())
		recorder()->start(activeFrameRate() * 60); // 1 minute.
	else if (f8 && recorder()->recording())
		recorder()->stop();

	// Help operations.
	if (f1)
		toggleManual(nullptr);
}

void WorkspaceStudio::menu(class Window* wnd, class Renderer* rnd, const class Project* project, Executable* exec, class Primitives* primitives) {
	if (canvasFull())
		return;

	if (immersive() && !headVisible()) {
		if (ImGui::IsMouseHoveringRect(ImVec2(0.0f, 0.0f), ImVec2((float)rnd->width(), menuHeight()), false)) {
			ImGuiWindow* imwnd = ImGui::FindWindowByName("##MainMenuBar");
			if (!imwnd)
				imwnd = ImGui::FindWindowByName("##menubar");
			if (imwnd)
				ImGui::BringWindowToDisplayFront(imwnd);
		} else {
			return;
		}
	}

	headVisible(false);
	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu(_theme->menuFile())) {
			headVisible(true);

			bool prjDirty = false;
			bool prjPersisted = false;
			bool prjArchived = false;
			projectStates(project, &prjDirty, &prjPersisted, &prjArchived, nullptr);

			bool dirty = false;
			editingAssetStates(project, nullptr, nullptr, nullptr, &dirty, nullptr, nullptr, nullptr, nullptr);

			if (ImGui::MenuItem(_theme->menuFile_New(), WORKSPACE_MODIFIER_KEY_NAME "+N")) {
				Operations::projectStop(rnd, this, project, exec, primitives);

				Operations::fileNew(rnd, this, project, exec);
			}
			if (ImGui::MenuItem(_theme->menuFile_Open(), WORKSPACE_MODIFIER_KEY_NAME "+O")) {
				Operations::projectStop(rnd, this, project, exec, primitives);

				Operations::fileOpenFile(rnd, this, project, exec);
			}
			if (ImGui::MenuItem(_theme->menuFile_OpenDirectory(), WORKSPACE_MODIFIER_KEY_NAME "+Shift+O")) {
				Operations::projectStop(rnd, this, project, exec, primitives);

				Operations::fileOpenDirectory(rnd, this, project, exec);
			}
			if (!examples().empty()) {
				if (ImGui::BeginMenu(_theme->menuFile_OpenExamples())) {
					std::string path;
					if (ImGui::ExampleMenu(project, examples(), path)) {
						path = Path::combine(WORKSPACE_EXAMPLE_PROJECT_DIR, path.c_str());

						Operations::projectStop(rnd, this, project, exec, primitives);

						Operations::fileOpenExample(rnd, this, project, exec, path.c_str());
					}

					ImGui::EndMenu();
				}
			}
			if (ImGui::BeginMenu(_theme->menuFile_OpenRecent(), !_settings.recentTouched.empty())) {
				for (int i = 0; i < WORKSPACE_RECENT_FILE_MAX_COUNT && i < (int)_settings.recentTouched.size(); ++i) {
					const unsigned type = (unsigned)_settings.recentTouched[i].type;
					const std::string &path = _settings.recentTouched[i].path;
					std::string name = path;
					if (type == (unsigned)StudioSettings::RecentTouched::EXAMPLE) {
						FileInfo::Ptr fileInfo = FileInfo::make(path.c_str());
						name = fileInfo->fileName() + "." + fileInfo->extName();
					} else if (name.length() > BITTY_MAX_PATH) {
						name = name.substr(0, BITTY_MAX_PATH);
					}
					if (ImGui::MenuItem(name)) {
						openRecentTouched(wnd, rnd, project, exec, primitives, (StudioSettings::RecentTouched::Types)type, i, path.c_str());

						break;
					}
				}
				ImGui::Separator();
				if (ImGui::MenuItem(_theme->menuFile_Clear())) {
					_settings.recentTouched.clear();
				}
				ImGui::EndMenu();
			}
			ImGui::Separator();
			if (ImGui::MenuItem(_theme->menuFile_Close())) {
				Operations::projectStop(rnd, this, project, exec, primitives);

				Operations::fileClose(rnd, this, project, exec);
			}
			ImGui::Separator();
			if (ImGui::MenuItem(_theme->menuFile_SaveAsset(), WORKSPACE_MODIFIER_KEY_NAME "+S", nullptr, dirty)) {
				Operations::fileSaveAsset(rnd, this, project, assetsEditingIndex());
			}
			if (prjArchived) {
				if (ImGui::MenuItem(_theme->menuFile_Save(), WORKSPACE_MODIFIER_KEY_NAME "+Shift+S", nullptr, prjDirty)) {
					Operations::fileSaveFile(rnd, this, project, false);
				}
				if (ImGui::MenuItem(_theme->menuFile_SaveAs(), nullptr, nullptr, (prjDirty && !prjPersisted) || prjPersisted)) {
					Operations::fileSaveFile(rnd, this, project, true);
				}
				if (ImGui::MenuItem(_theme->menuFile_SaveAsDirectory(), nullptr, nullptr, (prjDirty && !prjPersisted) || prjPersisted)) {
					Operations::fileSaveDirectory(rnd, this, project, false);
				}
			} else {
				if (ImGui::MenuItem(_theme->menuFile_Save(), WORKSPACE_MODIFIER_KEY_NAME "+Shift+S", nullptr, prjDirty)) {
					Operations::fileSaveDirectory(rnd, this, project, false);
				}
				if (ImGui::MenuItem(_theme->menuFile_SaveAs(), nullptr, nullptr, (prjDirty && !prjPersisted) || prjPersisted)) {
					Operations::fileSaveDirectory(rnd, this, project, true);
				}
				if (ImGui::MenuItem(_theme->menuFile_SaveAsFile(), nullptr, nullptr, (prjDirty && !prjPersisted) || prjPersisted)) {
					Operations::fileSaveFile(rnd, this, project, false);
				}
			}
			ImGui::Separator();
			if (ImGui::MenuItem(_theme->menuFile_Preferences())) {
				showPreferences(wnd, rnd, project, primitives);
			}
			ImGui::Separator();
			if (ImGui::MenuItem(_theme->menuFile_Quit(), "Alt+F4")) {
				SDL_Event evt;
				evt.type = SDL_QUIT;
				SDL_PushEvent(&evt);
			}

			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu(_theme->menuEdit())) {
			headVisible(true);

			bool any = false;
			unsigned type = 0;
			unsigned referencing = 0;
			bool pastable = false;
			bool selectable = false;
			const char* undoable = nullptr;
			const char* redoable = nullptr;
			editingAssetStates(project, &any, &type, &referencing, nullptr, &pastable, &selectable, &undoable, &redoable);

			if (ImGui::MenuItem(_theme->menuEdit_Undo(), WORKSPACE_MODIFIER_KEY_NAME "+Z", nullptr, !!undoable)) {
				withEditingAsset(
					project,
					[] (Asset* asset, Editable* editor) -> void {
						editor->undo(asset);
					}
				);
			}
			if (ImGui::MenuItem(_theme->menuEdit_Redo(), WORKSPACE_MODIFIER_KEY_NAME "+Y", nullptr, !!redoable)) {
				withEditingAsset(
					project,
					[] (Asset* asset, Editable* editor) -> void {
						editor->redo(asset);
					}
				);
			}
			ImGui::Separator();
			if (ImGui::MenuItem(_theme->menuEdit_Cut(), WORKSPACE_MODIFIER_KEY_NAME "+X", nullptr, any)) {
				withEditingAsset(
					project,
					[] (Asset*, Editable* editor) -> void {
						editor->cut();
					}
				);
			}
			if (ImGui::MenuItem(_theme->menuEdit_Copy(), WORKSPACE_MODIFIER_KEY_NAME "+C", nullptr, any)) {
				withEditingAsset(
					project,
					[] (Asset*, Editable* editor) -> void {
						editor->copy();
					}
				);
			}
			if (ImGui::MenuItem(_theme->menuEdit_Paste(), WORKSPACE_MODIFIER_KEY_NAME "+V", nullptr, any && pastable)) {
				withEditingAsset(
					project,
					[] (Asset*, Editable* editor) -> void {
						editor->paste();
					}
				);
			}
			if (ImGui::MenuItem(_theme->menuEdit_Delete(), nullptr, nullptr, any)) {
				withEditingAsset(
					project,
					[] (Asset*, Editable* editor) -> void {
						editor->del();
					}
				);
			}
			ImGui::Separator();
			if (ImGui::MenuItem(_theme->menuEdit_SelectAll(), WORKSPACE_MODIFIER_KEY_NAME "+A", nullptr, any && selectable)) {
				withEditingAsset(
					project,
					[] (Asset*, Editable* editor) -> void {
						editor->post(Editable::SELECT_ALL);
					}
				);
			}
			if (type == Code::TYPE()) {
				ImGui::Separator();
				if (ImGui::MenuItem(_theme->menuEdit_IncreaseIndent(), "Tab")) {
					withEditingAsset(
						project,
						[] (Asset*, Editable* editor) -> void {
							editor->post(Editable::INDENT, false);
						}
					);
				}
				if (ImGui::MenuItem(_theme->menuEdit_DecreaseIndent())) {
					withEditingAsset(
						project,
						[] (Asset*, Editable* editor) -> void {
							editor->post(Editable::UNINDENT, false);
						}
					);
				}
				ImGui::Separator();
				if (ImGui::MenuItem(_theme->menuEdit_Find(), WORKSPACE_MODIFIER_KEY_NAME "+F")) {
					withEditingAsset(
						project,
						[] (Asset*, Editable* editor) -> void {
							editor->post(Editable::FIND);
						}
					);
				}
				if (ImGui::MenuItem(_theme->menuEdit_FindNext(), "F3")) {
					withEditingAsset(
						project,
						[] (Asset*, Editable* editor) -> void {
							editor->post(Editable::FIND_NEXT);
						}
					);
				}
				if (ImGui::MenuItem(_theme->menuEdit_FindPrevious(), "Shift+F3")) {
					withEditingAsset(
						project,
						[] (Asset*, Editable* editor) -> void {
							editor->post(Editable::FIND_PREVIOUS);
						}
					);
				}
				if (ImGui::MenuItem(_theme->menuEdit_GotoLine(), WORKSPACE_MODIFIER_KEY_NAME "+G")) {
					withEditingAsset(
						project,
						[] (Asset*, Editable* editor) -> void {
							editor->post(Editable::GOTO);
						}
					);
				}
			}
			if (type == Image::TYPE()) {
				if (referencing == 0) {
					ImGui::Separator();
					if (ImGui::MenuItem(_theme->menuEdit_AlwaysShowBackground(), nullptr, &_settings.editorAlwaysShowTransparentBackground)) {
						// Do nothing.
					}
					if (ImGui::MenuItem(_theme->menuEdit_AlwaysShowGrids(), nullptr, &_settings.editorAlwaysShowGrids)) {
						// Do nothing.
					}
					if (ImGui::MenuItem(_theme->menuEdit_ResizeImage())) {
						Asset::List::Index idx = assetsEditingIndex();
						if (idx != -1)
							resizeAsset(wnd, rnd, project, idx);
					}
					if (ImGui::MenuItem(_theme->menuEdit_ResizeGrid())) {
						Asset::List::Index idx = assetsEditingIndex();
						if (idx != -1)
							resizeAssetGrid(wnd, rnd, project, idx);
					}
				} else {
					if (ImGui::MenuItem(_theme->menuEdit_ResizeGrid())) {
						Asset::List::Index idx = assetsEditingIndex();
						if (idx != -1)
							resizeAssetGrid(wnd, rnd, project, idx);
					}
				}
			}
			if (referencing != 0) {
				ImGui::Separator();
				if (type == Map::TYPE()) {
					if (ImGui::MenuItem(_theme->menuEdit_AlwaysShowBackground(), nullptr, &_settings.editorAlwaysShowTransparentBackground)) {
						// Do nothing.
					}
					if (ImGui::MenuItem(_theme->menuEdit_AlwaysShowGrids(), nullptr, &_settings.editorAlwaysShowGrids)) {
						// Do nothing.
					}
					if (ImGui::MenuItem(_theme->menuEdit_ResizeMap())) {
						Asset::List::Index idx = assetsEditingIndex();
						if (idx != -1)
							resizeAsset(wnd, rnd, project, idx);
					}
					if (ImGui::MenuItem(_theme->menuEdit_ResizeTile())) {
						Asset::List::Index idx = assetsEditingIndex();
						if (idx != -1)
							resizeAssetTile(wnd, rnd, project, idx);
					}
				}
				if (ImGui::MenuItem(_theme->menuEdit_ResolveRef())) {
					Asset::List::Index idx = assetsEditingIndex();
					if (idx != -1)
						rebindAssetRef(wnd, rnd, project, idx);
				}
			}

			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu(_theme->menuProject())) {
			headVisible(true);

			bool prjPersisted = false;
			const char* url = nullptr;
			projectStates(project, nullptr, &prjPersisted, nullptr, &url);

			if (ImGui::MenuItem(_theme->menuProject_Run(), "F5", nullptr, !executing() || paused())) {
				Operations::projectRun(rnd, this, project, exec, primitives);
			}
			if (ImGui::MenuItem(_theme->menuProject_Stop(), "Shift+F5", nullptr, executing())) {
				Operations::projectStop(rnd, this, project, exec, primitives);
			}
			ImGui::Separator();
			if (ImGui::MenuItem(_theme->menuProject_NewAsset(), WORKSPACE_MODIFIER_KEY_NAME "+Shift+N")) {
				Operations::projectAddAsset(rnd, this, project, assetsSelectedIndex());
			}
			if (ImGui::MenuItem(_theme->menuProject_RemoveAsset(), nullptr, nullptr, assetsSelectedIndex() >= 0)) {
				Operations::projectRemoveAsset(rnd, this, project, exec, assetsSelectedIndex());
			}
			if (ImGui::MenuItem(_theme->menuProject_RenameAsset(), nullptr, nullptr, assetsSelectedIndex() >= 0)) {
				Operations::projectRenameAsset(rnd, this, project, assetsSelectedIndex());
			}
			bool filtering = assetsFiltering();
			if (ImGui::MenuItem(_theme->menuProject_FilterAssets(), WORKSPACE_MODIFIER_KEY_NAME "+E", &filtering)) {
				assetsFiltering(filtering);
				assetsFilteringInitialized(false);
			}
			ImGui::Separator();
			if (ImGui::MenuItem(_theme->menuProject_AddFile(), WORKSPACE_MODIFIER_KEY_NAME "+Shift+A")) {
				Operations::projectAddFile(rnd, this, project, assetsSelectedIndex());
			}
			if (ImGui::MenuItem(_theme->menuProject_Import())) {
				Operations::projectImport(rnd, this, project);
			}
			if (ImGui::MenuItem(_theme->menuProject_Export())) {
				Operations::projectExport(rnd, this, project);
			}
			ImGui::Separator();
			if (ImGui::MenuItem(_theme->menuProject_Reload(), WORKSPACE_MODIFIER_KEY_NAME "+Shift+R", nullptr, prjPersisted)) {
				Operations::projectStop(rnd, this, project, exec, primitives);

				Operations::projectReload(rnd, this, project, exec);
			}
			if (ImGui::MenuItem(_theme->menuProject_Browse(), nullptr, nullptr, prjPersisted)) {
				Operations::projectBrowse(rnd, this, project);
			}
			if (url) {
				if (ImGui::MenuItem(_theme->menuProject_Explore())) {
					const std::string osstr = Unicode::toOs(url);

					Platform::surf(osstr.c_str());
				}
			}
			if (pluginsMenuProjectItemCount() > 0) {
				ImGui::Separator();

				Plugin* plugin = nullptr;
				if (ImGui::PluginMenu(project, plugins(), PLUGIN_MENU_PROJECT_NAME, plugin)) {
					Operations::pluginRunMenuItem(rnd, this, project, plugin, "");
				}
			}

			ImGui::EndMenu();
		}
#if BITTY_DEBUG_ENABLED
		if (ImGui::BeginMenu(_theme->menuDebug())) {
			headVisible(true);

			if (ImGui::MenuItem(_theme->menuDebug_Break(), nullptr, nullptr, executing() && !paused())) {
				Operations::debugBreak(this, project, exec);
			}
			if (ImGui::MenuItem(_theme->menuDebug_Continue(), "F5", nullptr, paused())) {
				Operations::debugContinue(this, project, exec);
			}
			ImGui::Separator();
			if (ImGui::MenuItem(_theme->menuDebug_Step(), "F10", nullptr, paused())) {
				Operations::debugStepOver(this, project, exec);
			}
			if (ImGui::MenuItem(_theme->menuDebug_StepInto(), "F11", nullptr, paused())) {
				Operations::debugStepInto(this, project, exec);
			}
			if (ImGui::MenuItem(_theme->menuDebug_StepOut(), "Shift+F11", nullptr, paused())) {
				Operations::debugStepOut(this, project, exec);
			}
			ImGui::Separator();
			if (ImGui::MenuItem(_theme->menuDebug_ToggleBreakpoint(), "F9")) {
				Operations::debugToggleBreakpoint(this, project, exec);
			}

			ImGui::EndMenu();
		}
#endif /* BITTY_DEBUG_ENABLED */
		if (pluginsMenuPluginsItemCount() > 0) {
			if (ImGui::BeginMenu(_theme->menuPlugins())) {
				Plugin* plugin = nullptr;
				if (ImGui::PluginMenu(project, plugins(), PLUGIN_MENU_PLUGIN_NAME, plugin)) {
					Operations::pluginRunMenuItem(rnd, this, project, plugin, "");
				}

				ImGui::EndMenu();
			}
		}
		if (ImGui::BeginMenu(_theme->menuWindow())) {
			headVisible(true);

			if (ImGui::BeginMenu(_theme->menuWindow_Screen())) {
				if (ImGui::MenuItem(_theme->menuWindow_Screen_ShootCanvas(), "F6", nullptr, !recorder()->recording() && executing() && !paused())) {
					recorder()->start(1);
				}
				if (ImGui::MenuItem(_theme->menuWindow_Screen_RecordCanvas(), "F7", nullptr, !recorder()->recording() && executing() && !paused())) {
					recorder()->start(activeFrameRate() * 60); // 1 minute.
				}
				if (ImGui::MenuItem(_theme->menuWindow_Screen_StopRecording(), "F8", nullptr, recorder()->recording() && executing() && !paused())) {
					recorder()->stop();
				}

				ImGui::EndMenu();
			}
			ImGui::Separator();
			if (ImGui::BeginMenu(_theme->menuWindow_Application())) {
				if (ImGui::MenuItem(_theme->menuWindow_Application_Fullscreen(), nullptr, _settings.applicationWindowFullscreen)) {
					toggleFullscreen(wnd);
				}
				if (ImGui::MenuItem(_theme->menuWindow_Application_Maximized(), nullptr, _settings.applicationWindowMaximized)) {
					toggleMaximized(wnd);
				}

				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu(_theme->menuWindow_Canvas())) {
				if (*canvasState() == POPUP) {
					ImGui::MenuItem(_theme->menuWindow_Canvas_Popup(), nullptr, true);
				} else {
					if (ImGui::MenuItem(_theme->menuWindow_Canvas_Popup()))
						*canvasState() = POPUP;
				}
				if (*canvasState() == FRAME) {
					ImGui::MenuItem(_theme->menuWindow_Canvas_Frame(), nullptr, true);
				} else {
					if (ImGui::MenuItem(_theme->menuWindow_Canvas_Frame()))
						*canvasState() = FRAME;
				}
				if (*canvasState() == MAXIMIZED) {
					ImGui::MenuItem(_theme->menuWindow_Canvas_Maximized(), nullptr, true);
				} else {
					if (ImGui::MenuItem(_theme->menuWindow_Canvas_Maximized()))
						*canvasState() = MAXIMIZED;
				}

				ImGui::EndMenu();
			}
			ImGui::MenuItem(_theme->menuWindow_Buttons(), nullptr, bannerVisible());
			ImGui::MenuItem(_theme->menuWindow_Assets(), nullptr, assetsVisible());
#if BITTY_DEBUG_ENABLED
			ImGui::MenuItem(_theme->menuWindow_Debug(), nullptr, debugVisible());
#endif /* BITTY_DEBUG_ENABLED */
			ImGui::MenuItem(_theme->menuWindow_Console(), nullptr, consoleVisible());

			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu(_theme->menuHelp())) {
			headVisible(true);

			if (!documents().empty()) {
				std::string path;
				if (ImGui::ExampleMenu(project, documents(), path)) {
					toggleManual(path.c_str());
				}
			}
			ImGui::Separator();
			if (ImGui::MenuItem(_theme->menuHelp_About())) {
				showAbout(wnd, rnd, primitives);
			}
			if (pluginsMenuHelpItemCount() > 0) {
				ImGui::Separator();

				Plugin* plugin = nullptr;
				if (ImGui::PluginMenu(project, plugins(), PLUGIN_MENU_HELP_NAME, plugin)) {
					Operations::pluginRunMenuItem(rnd, this, project, plugin, "");
				}
			}

			ImGui::EndMenu();
		}

		menuHeight(ImGui::GetItemRectSize().y);

		ImGui::EndMainMenuBar();
	}
}

void WorkspaceStudio::showPreferences(class Window* wnd, class Renderer*, const class Project* project, class Primitives* primitives) {
	auto set = [wnd, this, project, primitives] (const WorkspaceStudio::StudioSettings &sets) -> void {
		do {
			LockGuard<RecursiveMutex>::UniquePtr acquired;
			Project* prj = project->acquire(acquired);
			if (!prj)
				break;

			if (sets.themeStyle != _settings.themeStyle) {
				_theme->styleIndex((Theme::Styles)sets.themeStyle);

				switch (sets.themeStyle) {
				case Theme::DARK:
					consoleTextBox()->SetPalette(ImGui::CodeEditor::GetDarkPalette());

					break;
				case Theme::CLASSIC:
					consoleTextBox()->SetPalette(ImGui::CodeEditor::GetRetroBluePalette());

					break;
				case Theme::LIGHT:
					consoleTextBox()->SetPalette(ImGui::CodeEditor::GetLightPalette());

					break;
				}

				prj->foreach(
					[&] (Asset* &asset, Asset::List::Index) -> void {
						Editable* editor =  asset->editor();
						if (editor)
							editor->post(Editable::SET_THEME_STYLE, (Variant::Int)_theme->styleIndex());
					}
				);
			}

			if (sets.projectPreference != prj->preference()) {
				prj->preference(sets.projectPreference);
				prj->archive(nullptr);
			}
			if (sets.projectIgnoreDotFiles != prj->ignoreDotFiles()) {
				prj->ignoreDotFiles(sets.projectIgnoreDotFiles);
			}
			if (sets.projectLoadLastProjectAtStartup != _settings.projectLoadLastProjectAtStartup) {
				_settings.projectLoadLastProjectAtStartup = sets.projectLoadLastProjectAtStartup;
			}
			if (sets.projectAutoBackup != _settings.projectAutoBackup) {
				_settings.projectAutoBackup = sets.projectAutoBackup;
			}

			if (sets.editorIndentRule != _settings.editorIndentRule) {
				prj->foreach(
					[&] (Asset* &asset, Asset::List::Index) -> void {
						Editable* editor =  asset->editor();
						if (editor)
							editor->post(Editable::SET_INDENT_RULE, (Variant::Int)sets.editorIndentRule);
					}
				);
			}
			if (sets.editorColumnIndicator != _settings.editorColumnIndicator) {
				prj->foreach(
					[&] (Asset* &asset, Asset::List::Index) -> void {
						Editable* editor =  asset->editor();
						if (editor)
							editor->post(Editable::SET_COLUMN_INDICATOR, (Variant::Int)sets.editorColumnIndicator);
					}
				);
			}
			if (sets.editorShowWhiteSpaces != _settings.editorShowWhiteSpaces) {
				prj->foreach(
					[&] (Asset* &asset, Asset::List::Index) -> void {
						Editable* editor =  asset->editor();
						if (editor)
							editor->post(Editable::SET_SHOW_SPACES, sets.editorShowWhiteSpaces);
					}
				);
			}

			primitives->input()->config(sets.inputGamepads, INPUT_GAMEPAD_COUNT);

			if (sets.applicationWindowFullscreen != _settings.applicationWindowFullscreen) {
				toggleFullscreen(wnd);
			}
			if (sets.applicationWindowMaximized != _settings.applicationWindowMaximized) {
				toggleMaximized(wnd);
			}
		} while (false);

		_settings = sets;
	};

	ImGui::Studio::PreferencesPopupBox::ConfirmHandler confirm(
		[this, set] (const WorkspaceStudio::StudioSettings &sets) -> void {
			set(sets);

			popupBox(nullptr);
		},
		nullptr
	);
	ImGui::Studio::PreferencesPopupBox::CancelHandler cancel(
		[this] (void) -> void {
			popupBox(nullptr);
		},
		nullptr
	);
	ImGui::Studio::PreferencesPopupBox::ApplyHandler apply(
		[set] (const WorkspaceStudio::StudioSettings &sets) -> void {
			set(sets);
		},
		nullptr
	);
	popupBox(
		ImGui::PopupBox::Ptr(
			new ImGui::Studio::PreferencesPopupBox(
				primitives,
				_theme,
				_theme->windowPreferences(),
				_settings,
				!canvasFull(),
				confirm, cancel, apply,
				_theme->generic_Ok().c_str(), _theme->generic_Cancel().c_str(), _theme->generic_Apply().c_str()
			)
		)
	);
}

void WorkspaceStudio::showAbout(class Window* wnd, class Renderer* rnd, class Primitives* primitives) {
	popupBox(
		ImGui::PopupBox::Ptr(
			new ImGui::Studio::AboutPopupBox(
				wnd, rnd, primitives,
				_theme->windowAbout(),
				ImGui::Studio::AboutPopupBox::ConfirmHandler([&] (void) -> void { popupBox(nullptr); }, nullptr),
				_theme->generic_Ok().c_str()
			)
		)
	);
}

void WorkspaceStudio::showPaused(class Window* wnd, class Renderer* rnd, const class Project* project, class Primitives* primitives) {
	ImGui::Studio::PausedPopupBox::ResumeHandler resume(
		[this] (void) -> void {
			popupBox(nullptr);
		},
		nullptr
	);
	ImGui::Studio::PausedPopupBox::OptionsHandler options(
		[wnd, rnd, this, project, primitives] (void) -> void {
			showPreferences(wnd, rnd, project, primitives);
		},
		nullptr
	);
	ImGui::Studio::PausedPopupBox::AboutHandler about(
		[wnd, rnd, this, primitives] (void) -> void {
			showAbout(wnd, rnd, primitives);
		},
		nullptr
	);
	popupBox(
		ImGui::PopupBox::Ptr(
			new ImGui::Studio::PausedPopupBox(
				rnd,
				resume, options, about,
				_theme->windowPaused_Resume(), _theme->windowPaused_Options(), _theme->windowAbout()
			)
		)
	);
}

/* ===========================================================================} */
