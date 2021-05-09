/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2021 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "code.h"
#include "editable.h"
#include "encoding.h"
#include "file_handle.h"
#include "filesystem.h"
#include "loader.h"
#include "operations.h"
#include "platform.h"
#include "primitives.h"
#include "project.h"
#include "recorder.h"
#include "renderer.h"
#include "scripting_lua_api.h"
#include "scripting_lua_api_promises.h"
#include "theme_sketchbook.h"
#include "widgets_sketchbook.h"
#include "window.h"
#include "workspace_sketchbook.h"
#include "../lib/imgui/imgui_internal.h"
#include "../lib/imgui_code_editor/imgui_code_editor.h"
#include <SDL.h>

/*
** {===========================================================================
** Macros and constants
*/

#if BITTY_TRIAL_ENABLED
#	pragma message("Trial enabled.")
#endif /* BITTY_TRIAL_ENABLED */

#ifndef WORKSPACE_PREFERENCES_NAME
#	define WORKSPACE_PREFERENCES_NAME "preferences"
#endif /* WORKSPACE_PREFERENCES_NAME */

/* ===========================================================================} */

/*
** {===========================================================================
** Sketchbook workspace
*/

WorkspaceSketchbook::SketchbookSettings::SketchbookSettings() {
}

WorkspaceSketchbook::SketchbookSettings &WorkspaceSketchbook::SketchbookSettings::operator = (const WorkspaceSketchbook::SketchbookSettings &other) {
	applicationWindowDisplayIndex = other.applicationWindowDisplayIndex;
	applicationWindowFullscreen = other.applicationWindowFullscreen;
	applicationWindowMaximized = other.applicationWindowMaximized;
	applicationWindowSize = other.applicationWindowSize;
	applicationPauseOnFocusLost = other.applicationPauseOnFocusLost;

	projectPreference = other.projectPreference;
	projectIgnoreDotFiles = other.projectIgnoreDotFiles;

	bannerVisible = other.bannerVisible;
	assetsVisible = other.assetsVisible;

	editorShowWhiteSpaces = other.editorShowWhiteSpaces;
	editorCaseSensitive = other.editorCaseSensitive;
	editorMatchWholeWord = other.editorMatchWholeWord;

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

	return *this;
}

bool WorkspaceSketchbook::SketchbookSettings::operator != (const SketchbookSettings &other) const {
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
		applicationWindowSize != other.applicationWindowSize ||
		applicationPauseOnFocusLost != other.applicationPauseOnFocusLost
	) {
		return true;
	}

	if (projectPreference != other.projectPreference ||
		projectIgnoreDotFiles != other.projectIgnoreDotFiles
	) {
		return true;
	}

	if (bannerVisible != other.bannerVisible)
		return true;
	if (assetsVisible != other.assetsVisible)
		return true;

	if (editorShowWhiteSpaces != other.editorShowWhiteSpaces ||
		editorCaseSensitive != other.editorCaseSensitive ||
		editorMatchWholeWord != other.editorMatchWholeWord
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

	return false;
}

WorkspaceSketchbook::WorkspaceSketchbook() {
	_theme = new ThemeSketchbook();

	_loader = new Loader();
}

WorkspaceSketchbook::~WorkspaceSketchbook() {
	delete _loader;
	_loader = nullptr;

	delete _theme;
	_theme = nullptr;
}

bool WorkspaceSketchbook::open(class Window* wnd, class Renderer* rnd, const class Project* project, Executable* exec, class Primitives* primitives, const Text::Dictionary &options) {
	if (_opened)
		return false;
	_opened = true;

#if BITTY_TRIAL_ENABLED
#	if defined BITTY_DEBUG
	wnd->title(BITTY_TITLE " Trial v" BITTY_VERSION_STRING " [DEBUG]");
#	else /* BITTY_DEBUG */
	wnd->title(BITTY_TITLE " Trial v" BITTY_VERSION_STRING);
#	endif /* BITTY_DEBUG */
#else /* BITTY_TRIAL_ENABLED */
#	if defined BITTY_DEBUG
	wnd->title(BITTY_TITLE " v" BITTY_VERSION_STRING " [DEBUG]");
#	else /* BITTY_DEBUG */
	wnd->title(BITTY_TITLE " v" BITTY_VERSION_STRING);
#	endif /* BITTY_DEBUG */
#endif /* BITTY_TRIAL_ENABLED */

	beginSplash(wnd, rnd, project);

	_theme->open(rnd);
	_theme->load(rnd);

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

	if (!Workspace::open(wnd, rnd, project, exec, primitives, options)) {
		endSplash(wnd, rnd);

		return false;
	}

	consoleTextBox()->SetPalette(ImGui::CodeEditor::GetLightPalette());

	loadProject(wnd, rnd, project, exec, primitives, options);

	endSplash(wnd, rnd);

	Operations::fileRestore(rnd, this, project);

	const std::string ready = theme()->generic_Ready() + '\n';
	print(ready.c_str());

	return true;
}

bool WorkspaceSketchbook::close(class Window* wnd, class Renderer* rnd, const class Project* project, Executable* exec) {
	if (!_opened)
		return false;
	_opened = false;

	unloadProject(project, exec);

	Operations::fileClean(rnd, this, project);

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

const WorkspaceSketchbook::Settings* WorkspaceSketchbook::settings(void) const {
	return &_settings;
}

WorkspaceSketchbook::Settings* WorkspaceSketchbook::settings(void) {
	return &_settings;
}

class Theme* WorkspaceSketchbook::theme(void) const {
	return _theme;
}

bool WorkspaceSketchbook::load(class Window* wnd, class Renderer* rnd, const class Project* project, class Primitives* primitives) {
	const std::string pref = Path::writableDirectory();
	const std::string path = Path::combine(pref.c_str(), WORKSPACE_PREFERENCES_NAME "." BITTY_JSON_EXT);

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

bool WorkspaceSketchbook::save(class Window* wnd, class Renderer* rnd, const class Project* project, class Primitives* primitives) {
	rapidjson::Document doc;

	if (!save(wnd, rnd, project, primitives, doc))
		return false;

	const std::string pref = Path::writableDirectory();
	const std::string path = Path::combine(pref.c_str(), WORKSPACE_PREFERENCES_NAME "." BITTY_JSON_EXT);
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

unsigned WorkspaceSketchbook::update(class Window* wnd, class Renderer* rnd, const class Project* project, Executable* exec, class Primitives* primitives, double delta, unsigned fps, bool alive, bool* indicated) {
	// Prepare.
	unsigned result = 0;

	execute(wnd, rnd, project, exec, primitives, delta, alive);

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

void WorkspaceSketchbook::require(Executable* exec) {
	switch (exec->language()) {
	case Executable::LUA:
		// Common.
		Lua::Standard::open(exec);
		Lua::Libs::open(exec);
		if (exec->primitives()) {
			Lua::Engine::open(exec);
		}
		Lua::Application::open(exec);

		// Promise.
		Lua::Standard::promise(exec);
		Lua::Libs::promise(exec);

		break;
	default:
		assert(false && "Not supported.");

		break;
	}
}

void WorkspaceSketchbook::focusGained(class Window* wnd, class Renderer* rnd, const class Project* project, Executable* exec, class Primitives* primitives) {
	Workspace::focusGained(wnd, rnd, project, exec, primitives);

	exec->focusGained();
}

void WorkspaceSketchbook::focusLost(class Window* wnd, class Renderer* rnd, const class Project* project, Executable* exec, class Primitives* primitives) {
	Workspace::focusLost(wnd, rnd, project, exec, primitives);

	exec->focusLost();

	if (!canvasFull() || !settings()->applicationPauseOnFocusLost)
		return;

	if (popupBox())
		return;

	showPaused(wnd, rnd, project, primitives);
}

void WorkspaceSketchbook::renderTargetsReset(class Window* wnd, class Renderer* rnd, const class Project* project, Executable* exec, class Primitives* primitives) {
	Workspace::renderTargetsReset(wnd, rnd, project, exec, primitives);

	exec->renderTargetsReset();
}

void WorkspaceSketchbook::shortcuts(class Window* wnd, class Renderer* rnd, const class Project* project, Executable* exec, class Primitives* primitives) {
	// Prepare.
	ImGuiIO &io = ImGui::GetIO();

	const bool esc = ImGui::IsKeyPressed(SDL_SCANCODE_ESCAPE);

	do {
		if (!esc)
			break;
		if (!canvasFull())
			break;

		if (popupBox())
			break;

		showPaused(wnd, rnd, project, primitives);

		return;
	} while (false);

	if (!canUseShortcuts())
		return;

	// Get key states.
	const bool f1 = ImGui::IsKeyPressed(SDL_SCANCODE_F1);
	const bool f3 = ImGui::IsKeyPressed(SDL_SCANCODE_F3);
	const bool f5 = ImGui::IsKeyPressed(SDL_SCANCODE_F5);
	const bool f6 = ImGui::IsKeyPressed(SDL_SCANCODE_F6);
	const bool f7 = ImGui::IsKeyPressed(SDL_SCANCODE_F7);
	const bool f8 = ImGui::IsKeyPressed(SDL_SCANCODE_F8);
	const bool f9 = ImGui::IsKeyPressed(SDL_SCANCODE_F9);
	const bool a = ImGui::IsKeyPressed(SDL_SCANCODE_A);
	const bool c = ImGui::IsKeyPressed(SDL_SCANCODE_C);
	const bool e = ImGui::IsKeyPressed(SDL_SCANCODE_E);
	const bool f = ImGui::IsKeyPressed(SDL_SCANCODE_F);
	const bool g = ImGui::IsKeyPressed(SDL_SCANCODE_G);
	const bool n = ImGui::IsKeyPressed(SDL_SCANCODE_N);
	const bool o = ImGui::IsKeyPressed(SDL_SCANCODE_O);
	const bool r = ImGui::IsKeyPressed(SDL_SCANCODE_R);
	const bool s = ImGui::IsKeyPressed(SDL_SCANCODE_S);
	const bool v = ImGui::IsKeyPressed(SDL_SCANCODE_V);
	const bool w = ImGui::IsKeyPressed(SDL_SCANCODE_W);
	const bool x = ImGui::IsKeyPressed(SDL_SCANCODE_X);
	const bool y = ImGui::IsKeyPressed(SDL_SCANCODE_Y);
	const bool z = ImGui::IsKeyPressed(SDL_SCANCODE_Z);
	const bool tab = ImGui::IsKeyPressed(SDL_SCANCODE_TAB);
	const bool period = ImGui::IsKeyPressed(SDL_SCANCODE_PERIOD);
	const bool del = ImGui::IsKeyPressed(SDL_SCANCODE_DELETE);
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
	}
	if (o && modifier && !io.KeyShift) {
		Operations::projectStop(rnd, this, project, exec, primitives);

		Operations::fileOpenFile(rnd, this, project, exec);
	}
	if (o && modifier && io.KeyShift) {
		Operations::projectStop(rnd, this, project, exec, primitives);

		Operations::fileOpenDirectory(rnd, this, project, exec);
	}
#if !BITTY_TRIAL_ENABLED
	if (s && modifier) {
		Operations::fileSaveAsset(rnd, this, project, assetsEditingIndex());
	}
	if (s && modifier && io.KeyShift) {
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
#else /* BITTY_TRIAL_ENABLED */
	(void)s;
#endif /* BITTY_TRIAL_ENABLED */

	// Edit operations.
	if (z && modifier) {
		withEditingAsset(
			project,
			[] (Asset* asset, Editable* editor) -> void {
				editor->undo(asset);
			}
		);
	}
	if (y && modifier) {
		withEditingAsset(
			project,
			[] (Asset* asset, Editable* editor) -> void {
				editor->redo(asset);
			}
		);
	}
	if (c && modifier && !assetsFocused() && !canvasFocused() && !consoleFocused()) {
		withEditingAsset(
			project,
			[] (Asset*, Editable* editor) -> void {
				editor->copy();
			}
		);
	}
	if (x && modifier && !assetsFocused() && !canvasFocused() && !consoleFocused()) {
		withEditingAsset(
			project,
			[] (Asset*, Editable* editor) -> void {
				editor->cut();
			}
		);
	}
	if (v && modifier && !assetsFocused() && !canvasFocused() && !consoleFocused()) {
		withEditingAsset(
			project,
			[] (Asset*, Editable* editor) -> void {
				editor->paste();
			}
		);
	}
	if (del && !assetsFocused() && !canvasFocused() && !consoleFocused()) {
		withEditingAsset(
			project,
			[] (Asset*, Editable* editor) -> void {
				editor->del();
			}
		);
	}
	if (a && modifier && !assetsFocused() && !canvasFocused() && !consoleFocused()) {
		withEditingAsset(
			project,
			[] (Asset*, Editable* editor) -> void {
				editor->post(Editable::SELECT_ALL);
			}
		);
	}
	if (tab && !io.KeyCtrl && !io.KeyShift && !assetsFocused() && !canvasFocused() && !consoleFocused()) {
		withEditingAsset(
			project,
			[] (Asset*, Editable* editor) -> void {
				editor->post(Editable::INDENT, true);
			}
		);
	}
	if (tab && io.KeyShift && !assetsFocused() && !canvasFocused() && !consoleFocused()) {
		withEditingAsset(
			project,
			[] (Asset*, Editable* editor) -> void {
				editor->post(Editable::UNINDENT, true);
			}
		);
	}
	if (f && modifier && !canvasFocused() && !consoleFocused()) {
		withEditingAsset(
			project,
			[] (Asset*, Editable* editor) -> void {
				editor->post(Editable::FIND);
			}
		);
	}
	if (f3 && !io.KeyShift && !canvasFocused() && !consoleFocused()) {
		withEditingAsset(
			project,
			[] (Asset*, Editable* editor) -> void {
				editor->post(Editable::FIND_NEXT);
			}
		);
	}
	if (f3 && io.KeyShift && !canvasFocused() && !consoleFocused()) {
		withEditingAsset(
			project,
			[] (Asset*, Editable* editor) -> void {
				editor->post(Editable::FIND_PREVIOUS);
			}
		);
	}
	if (g && modifier && !canvasFocused() && !consoleFocused()) {
		withEditingAsset(
			project,
			[] (Asset*, Editable* editor) -> void {
				editor->post(Editable::GOTO);
			}
		);
	}
	if (tab && io.KeyCtrl && !io.KeyShift)
		Operations::editSwitchAsset(rnd, this, project);
	if (w && io.KeyCtrl)
		editingClosing(true);

	// Project operations.
	if (e && modifier) {
		assetsFiltering(!assetsFiltering());
		assetsFilteringInitialized(false);
	}
	if (n && modifier && io.KeyShift)
		Operations::projectAddAsset(rnd, this, project, assetsSelectedIndex());
	if (a && modifier && io.KeyShift)
		Operations::projectAddFile(rnd, this, project, assetsSelectedIndex());
	if (toRun)
		Operations::projectRun(rnd, this, project, exec, primitives);
	if (toStop)
		Operations::projectStop(rnd, this, project, exec, primitives);

	// Debug operations.
	if (toResume)
		Operations::debugContinue(this, project, exec);
	if (f9)
		Operations::debugToggleBreakpoint(this, project, exec);

	// Window operations.
	if (f6 && !recorder()->recording())
		recorder()->start(1);
	if (f7 && !recorder()->recording())
		recorder()->start(BITTY_ACTIVE_FRAME_RATE * 60); // 1 minute.
	if (f8 && recorder()->recording())
		recorder()->stop();

	// Help operations.
	if (f1)
		toggleManual(nullptr);
}

void WorkspaceSketchbook::menu(class Window* wnd, class Renderer* rnd, const class Project* project, Executable* exec, class Primitives* primitives) {
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
			editingAssetStates(project, nullptr, nullptr, nullptr, &dirty, nullptr, nullptr, nullptr);

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
			ImGui::Separator();
			if (ImGui::MenuItem(_theme->menuFile_Close())) {
				Operations::projectStop(rnd, this, project, exec, primitives);

				Operations::fileClose(rnd, this, project, exec);
			}
			ImGui::Separator();
#if !BITTY_TRIAL_ENABLED
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
#endif /* BITTY_TRIAL_ENABLED */
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
			const char* undoable = nullptr;
			const char* redoable = nullptr;
			editingAssetStates(project, &any, &type, &referencing, nullptr, &pastable, &undoable, &redoable);

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
			if (ImGui::MenuItem(_theme->menuEdit_SelectAll(), WORKSPACE_MODIFIER_KEY_NAME "+A", nullptr, any)) {
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
			if (ImGui::MenuItem(theme()->menuProject_FilterAssets(), WORKSPACE_MODIFIER_KEY_NAME "+E", &filtering)) {
				assetsFiltering(filtering);
				assetsFilteringInitialized(false);
			}
			ImGui::Separator();
			if (ImGui::MenuItem(_theme->menuProject_AddFile(), WORKSPACE_MODIFIER_KEY_NAME "+Shift+A")) {
				Operations::projectAddFile(rnd, this, project, assetsSelectedIndex());
			}
#if !BITTY_TRIAL_ENABLED
			if (ImGui::MenuItem(_theme->menuProject_Import())) {
				Operations::projectImport(rnd, this, project);
			}
			if (ImGui::MenuItem(_theme->menuProject_Export())) {
				Operations::projectExport(rnd, this, project);
			}
#endif /* BITTY_TRIAL_ENABLED */
			ImGui::Separator();
			if (ImGui::MenuItem(_theme->menuProject_Reload(), nullptr, nullptr, prjPersisted)) {
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
					Operations::pluginRunMenuItem(rnd, this, project, plugin);
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
					Operations::pluginRunMenuItem(rnd, this, project, plugin);
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
					recorder()->start(BITTY_ACTIVE_FRAME_RATE * 60); // 1 minute.
				}
				if (ImGui::MenuItem(_theme->menuWindow_Screen_StopRecording(), "F8", nullptr, recorder()->recording() && executing() && !paused())) {
					recorder()->stop();
				}

				ImGui::EndMenu();
			}
			ImGui::Separator();
			if (ImGui::BeginMenu(_theme->menuWindow_Application())) {
				if (ImGui::MenuItem(_theme->menuWindow_Application_Fullscreen(), nullptr, settings()->applicationWindowFullscreen)) {
					toggleFullscreen(wnd);
				}
				if (ImGui::MenuItem(_theme->menuWindow_Application_Maximized(), nullptr, settings()->applicationWindowMaximized)) {
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
			ImGui::MenuItem(_theme->menuWindow_Debug(), nullptr, debugVisible(), executing());
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
					Operations::pluginRunMenuItem(rnd, this, project, plugin);
				}
			}

			ImGui::EndMenu();
		}

		menuHeight(ImGui::GetItemRectSize().y);

		ImGui::EndMainMenuBar();
	}
}

void WorkspaceSketchbook::loadProject(class Window* wnd, class Renderer* rnd, const class Project* project, Executable* exec, class Primitives* primitives, const Text::Dictionary &options) {
	promise::Defer start;

	Text::Dictionary::const_iterator pathOpt = options.find(WORKSPACE_OPTION_APPLICATION_DEFAULT_KEY); // The non-flag option indicates initial directory/file path.
	if (pathOpt == options.end()) {
		auto run = [] (WorkspaceSketchbook* self, Window* wnd, Renderer* rnd, const Project* project, Executable* exec, Primitives* primitives, const char* /* name */) -> void {
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
				configAsset->finish(Asset::RUNNING, true);
				Json::Ptr json = Object::as<Json::Ptr>(obj);

				if (!json)
					break;
				rapidjson::Document doc;
				if (!json->toJson(doc))
					break;

				self->load(wnd, rnd, project, primitives, doc);
			} while (false);

			self->canvasFull(true);

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
		} else {
			// Rejection.
			start = promise::newPromise([] (promise::Defer df) -> void { df.reject(); });
		}
	} else {
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

void WorkspaceSketchbook::unloadProject(const class Project* project, Executable* exec) {
	canvasFull(false);

	exec->clearBreakpoints(nullptr);

	LockGuard<RecursiveMutex>::UniquePtr acquired;
	Project* prj = project->acquire(acquired);
	if (!prj)
		return;

	prj->unload();
	prj->readonly(false);
}

void WorkspaceSketchbook::showPreferences(class Window* wnd, class Renderer*, const class Project* project, class Primitives* primitives) {
	auto set = [wnd, this, project, primitives] (const WorkspaceSketchbook::SketchbookSettings &sets) -> void {
		do {
			LockGuard<RecursiveMutex>::UniquePtr acquired;
			Project* prj = project->acquire(acquired);
			if (!prj)
				break;

			if (sets.projectPreference != prj->preference()) {
				prj->preference(sets.projectPreference);
				prj->archive(nullptr);
			}
			if (sets.projectIgnoreDotFiles != prj->ignoreDotFiles()) {
				prj->ignoreDotFiles(sets.projectIgnoreDotFiles);
			}

			if (sets.editorShowWhiteSpaces != settings()->editorShowWhiteSpaces) {
				prj->foreach(
					[&] (Asset* &asset, Asset::List::Index) -> void {
						Editable* editor =  asset->editor();
						if (editor)
							editor->post(Editable::SET_SHOW_SPACES, sets.editorShowWhiteSpaces);
					}
				);
			}

			primitives->input()->config(sets.inputGamepads, INPUT_GAMEPAD_COUNT);

			if (sets.applicationWindowFullscreen != settings()->applicationWindowFullscreen) {
				toggleFullscreen(wnd);
			}
			if (sets.applicationWindowMaximized != settings()->applicationWindowMaximized) {
				toggleMaximized(wnd);
			}
		} while (false);

		_settings = sets;
	};

	ImGui::Sketchbook::PreferencesPopupBox::ConfirmHandler confirm(
		[this, set] (const WorkspaceSketchbook::SketchbookSettings &sets) -> void {
			set(sets);

			popupBox(nullptr);
		},
		nullptr
	);
	ImGui::Sketchbook::PreferencesPopupBox::CancelHandler cancel(
		[this] (void) -> void {
			popupBox(nullptr);
		},
		nullptr
	);
	ImGui::Sketchbook::PreferencesPopupBox::ApplyHandler apply(
		[set] (const WorkspaceSketchbook::SketchbookSettings &sets) -> void {
			set(sets);
		},
		nullptr
	);
	popupBox(
		ImGui::PopupBox::Ptr(
			new ImGui::Sketchbook::PreferencesPopupBox(
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

void WorkspaceSketchbook::showAbout(class Window* wnd, class Renderer* rnd, class Primitives* primitives) {
	popupBox(
		ImGui::PopupBox::Ptr(
			new ImGui::Sketchbook::AboutPopupBox(
				wnd, rnd, primitives,
				_theme->windowAbout(),
				ImGui::Sketchbook::AboutPopupBox::ConfirmHandler([&] (void) -> void { popupBox(nullptr); }, nullptr),
				_theme->generic_Ok().c_str()
			)
		)
	);
}

void WorkspaceSketchbook::showPaused(class Window* wnd, class Renderer* rnd, const class Project* project, class Primitives* primitives) {
	ImGui::Sketchbook::PausedPopupBox::ResumeHandler resume(
		[this] (void) -> void {
			popupBox(nullptr);
		},
		nullptr
	);
	ImGui::Sketchbook::PausedPopupBox::OptionsHandler options(
		[wnd, rnd, this, project, primitives] (void) -> void {
			showPreferences(wnd, rnd, project, primitives);
		},
		nullptr
	);
	ImGui::Sketchbook::PausedPopupBox::AboutHandler about(
		[wnd, rnd, this, primitives] (void) -> void {
			showAbout(wnd, rnd, primitives);
		},
		nullptr
	);
	popupBox(
		ImGui::PopupBox::Ptr(
			new ImGui::Sketchbook::PausedPopupBox(
				rnd,
				resume, options, about,
				_theme->windowPaused_Resume(), _theme->windowPaused_Options(), _theme->windowAbout()
			)
		)
	);
}

/* ===========================================================================} */
