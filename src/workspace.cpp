/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2022 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "bytes.h"
#include "document.h"
#include "datetime.h"
#include "editable.h"
#include "encoding.h"
#include "file_handle.h"
#include "filesystem.h"
#include "operations.h"
#include "platform.h"
#include "primitives.h"
#include "project.h"
#include "recorder.h"
#include "renderer.h"
#include "theme.h"
#include "window.h"
#include "workspace.h"
#include "resource/inline_resource.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "../lib/imgui/imgui_internal.h"
#include "../lib/imgui_code_editor/imgui_code_editor.h"
#include "../lib/jpath/jpath.hpp"
#define SDL_MAIN_HANDLED
#include <SDL.h>
#if defined BITTY_OS_HTML
#	include <emscripten.h>
#endif /* BITTY_OS_HTML */

/*
** {===========================================================================
** Macros and constants
*/

#ifndef WORKSPACE_SPLASH_FILE
#	define WORKSPACE_SPLASH_FILE "../splash.png"
#endif /* WORKSPACE_SPLASH_FILE */

#if !defined IMGUI_DISABLE_OBSOLETE_FUNCTIONS
#	error "IMGUI_DISABLE_OBSOLETE_FUNCTIONS not defined."
#endif /* IMGUI_DISABLE_OBSOLETE_FUNCTIONS */

static_assert(sizeof(ImDrawIdx) == sizeof(unsigned int), "Wrong ImDrawIdx size.");

/* ===========================================================================} */

/*
** {===========================================================================
** Utilities
*/

#if defined BITTY_OS_HTML
EM_JS(
	bool, workspaceGetPlayButtonEnabled, (), {
		if (typeof getPlayButtonEnabled != 'function')
			return true;

		return getPlayButtonEnabled();
	}
);
#endif /* BITTY_OS_HTML */

#if BITTY_SPLASH_ENABLED
#if defined BITTY_OS_HTML
static void workspaceSleep(int ms) {
	emscripten_sleep((unsigned)ms);
}
#else /* BITTY_OS_HTML */
static void workspaceSleep(int ms) {
	DateTime::sleep(ms);
}
#endif /* BITTY_OS_HTML */
static void workspaceCreateSplash(Window*, Renderer* rnd, Workspace* ws) {
	if (ws->splashBitty()) {
		ws->theme()->destroyTexture(rnd, ws->splashBitty());
		ws->splashBitty(nullptr);
	}

	if (ws->splashEngine()) {
		ws->theme()->destroyTexture(rnd, ws->splashEngine());
		ws->splashEngine(nullptr);
	}

	File::Ptr file(File::create());
	if (file->open(WORKSPACE_SPLASH_FILE, Stream::READ)) {
		Bytes::Ptr bytes(Bytes::create());
		file->readBytes(bytes.get());
		file->close();

		ws->splashBitty(ws->theme()->createTexture(rnd, bytes->pointer(), bytes->count()));
	}
}
static void workspaceCreateSplash(Window*, Renderer* rnd, Workspace* ws, int index) {
	constexpr const Byte* const IMAGES[] = {
		RES_TOAST_BITTY0, RES_TOAST_BITTY1, RES_TOAST_BITTY2, RES_TOAST_BITTY3, RES_TOAST_BITTY4, RES_TOAST_BITTY5, RES_TOAST_BITTY6
	};
	constexpr const size_t LENS[] = {
		BITTY_COUNTOF(RES_TOAST_BITTY0), BITTY_COUNTOF(RES_TOAST_BITTY1), BITTY_COUNTOF(RES_TOAST_BITTY2), BITTY_COUNTOF(RES_TOAST_BITTY3), BITTY_COUNTOF(RES_TOAST_BITTY4), BITTY_COUNTOF(RES_TOAST_BITTY5), BITTY_COUNTOF(RES_TOAST_BITTY6)
	};

	if (ws->splashBitty()) {
		ws->theme()->destroyTexture(rnd, ws->splashBitty());
		ws->splashBitty(nullptr);
	}

	ws->splashBitty(ws->theme()->createTexture(rnd, IMAGES[index], LENS[index]));

	if (!ws->splashEngine()) {
		ws->splashEngine(ws->theme()->createTexture(rnd, RES_TOAST_ENGINE, BITTY_COUNTOF(RES_TOAST_ENGINE)));
	}
}
static void workspaceRenderSplash(Window*, Renderer* rnd, Workspace* ws, std::function<void(Renderer*, Workspace*)> post) {
	const Color cls(0x00, 0x00, 0x00, 0x00);
	rnd->clear(&cls);

	if (ws->splashBitty()) {
		const Math::Recti dstBitty = Math::Recti::byXYWH(
			(rnd->width() - ws->splashBitty()->width()) / 2,
			(rnd->height() - ws->splashBitty()->height()) / 2,
			ws->splashBitty()->width(),
			ws->splashBitty()->height()
		);
		rnd->render(ws->splashBitty(), nullptr, &dstBitty, nullptr, nullptr, false, false, nullptr, false, false);

		if (ws->splashEngine()) {
			const Math::Recti dstEngine = Math::Recti::byXYWH(
				(rnd->width() - ws->splashEngine()->width()) / 2,
				dstBitty.yMax() + 16,
				ws->splashEngine()->width(),
				ws->splashEngine()->height()
			);
			rnd->render(ws->splashEngine(), nullptr, &dstEngine, nullptr, nullptr, false, false, nullptr, false, false);
		}
	}

	if (post)
		post(rnd, ws);

	rnd->flush();
}
static void workspaceWaitSplash(Window* wnd, Renderer* rnd, Workspace* ws, const Project* project) {
#if defined BITTY_OS_HTML
	if (!workspaceGetPlayButtonEnabled())
		return;

	static bool ran = false;
	if (ran)
		return;

	ran = true;

	Primitives* primitives = Primitives::create(false);
	primitives->open(wnd, rnd, project, nullptr, nullptr);
	primitives->autoCls(false);
	bool pressed = false;
	while (true) {
		constexpr const int STEP = 10;
		workspaceSleep(STEP);
		Platform::idle();

		bool finished = false;
		workspaceRenderSplash(
			wnd, rnd, ws,
			[&] (Renderer* rnd, Workspace*) -> void {
				primitives->newFrame();

				const Math::Vec2f rndSize(rnd->width(), rnd->height());
				auto collides = [] (const Math::Vec3f &circ, float x, float y, float canw, float canh, float dispw, float disph) -> bool {
					x = x / dispw * canw;
					y = y / disph * canh;
					const Real dx = x - circ.x;
					const Real dy = y - circ.y;
					const Real dist = std::sqrt(dx * dx + dy * dy);

					return dist <= circ.z;
				};
				const Math::Vec3f range(rnd->width() * 0.5f, rnd->height() * 0.5f, rnd->height() * 0.5f);
#if defined BITTY_DEBUG
				{
					Math::Vec2f p0(range.x, range.y);
					const Color debugCol(255, 0, 0);
					primitives->circ((int)p0.x, (int)p0.y, (int)range.z, false, &debugCol);
				}
#endif /* BITTY_DEBUG */

				bool touched = false;
				int mouseX = 0, mouseY = 0;
				bool mouseB0 = false;
				if (primitives->mouse(0, &mouseX, &mouseY, &mouseB0, nullptr, nullptr, nullptr, nullptr) && mouseB0) {
					if (collides(range, (float)mouseX, (float)mouseY, (float)rndSize.x, (float)rndSize.y, (float)rndSize.x, (float)rndSize.y))
						touched = true;
				}

				const float offsetX = 0;
				const float offsetY = 96;
				const float cornerX = 10 * 3;
				const float cornerY = 12 * 3;
				const Math::Vec2f p0(rnd->width() * 0.5f + cornerX + offsetX, rnd->height() * 0.5f + offsetY);
				const Math::Vec2f p1(rnd->width() * 0.5f - cornerX + offsetX, rnd->height() * 0.5f - cornerY + offsetY);
				const Math::Vec2f p2(rnd->width() * 0.5f - cornerX + offsetX, rnd->height() * 0.5f + cornerY + offsetY);
				const Color fillTriCol = touched ? Color(45, 39, 41, 128) : Color(128, 128, 128, 128);
				const Color triCol = touched ? Color(255, 255, 255, 235) : Color(255, 255, 255, 235);
				primitives->tri(p0, p1, p2, true, &fillTriCol);
				primitives->tri(p0, p1, p2, false, &triCol);

				primitives->commit();
				const int scale = rnd->scale() / wnd->scale();
				const Math::Rectf clientArea = Math::Rectf::byXYWH(0, 0, rnd->width(), rnd->height());
				const Math::Vec2i canvasSz(rnd->width(), rnd->height());
				primitives->update(&clientArea, &canvasSz, scale, Math::EPSILON<double>(), true, nullptr);

				if (!pressed) {
					if (touched)
						pressed = true;
				} else {
					if (!touched)
						finished = true;
				}
			}
		);

		if (finished)
			break;
	}
	primitives->close();
	Primitives::destroy(primitives);
#else /* BITTY_OS_HTML */
	(void)wnd;
	(void)rnd;
	(void)ws;
	(void)project;
#endif /* BITTY_OS_HTML */
}
#endif /* BITTY_SPLASH_ENABLED */

/* ===========================================================================} */

/*
** {===========================================================================
** Workspace
*/

Workspace::Settings::Settings() {
	static_assert(INPUT_GAMEPAD_COUNT >= 2, "Wrong size.");

	inputGamepads[0].buttons[Input::LEFT] = Input::Button(Input::KEYBOARD, 0, SDL_SCANCODE_A);
	inputGamepads[0].buttons[Input::RIGHT] = Input::Button(Input::KEYBOARD, 0, SDL_SCANCODE_D);
	inputGamepads[0].buttons[Input::UP] = Input::Button(Input::KEYBOARD, 0, SDL_SCANCODE_W);
	inputGamepads[0].buttons[Input::DOWN] = Input::Button(Input::KEYBOARD, 0, SDL_SCANCODE_S);
	inputGamepads[0].buttons[Input::A] = Input::Button(Input::KEYBOARD, 0, SDL_SCANCODE_J);
	inputGamepads[0].buttons[Input::B] = Input::Button(Input::KEYBOARD, 0, SDL_SCANCODE_K);

	inputGamepads[1].buttons[Input::LEFT] = Input::Button(Input::JOYSTICK, 0, 0, -1);
	inputGamepads[1].buttons[Input::RIGHT] = Input::Button(Input::JOYSTICK, 0, 0, 1);
	inputGamepads[1].buttons[Input::UP] = Input::Button(Input::JOYSTICK, 0, 1, -1);
	inputGamepads[1].buttons[Input::DOWN] = Input::Button(Input::JOYSTICK, 0, 1, 1);
	inputGamepads[1].buttons[Input::A] = Input::Button(Input::JOYSTICK, 0, 0);
	inputGamepads[1].buttons[Input::B] = Input::Button(Input::JOYSTICK, 0, 1);
}

Workspace::SourcePosition::SourcePosition() {
}

void Workspace::SourcePosition::set(const std::string &src, int ln) {
	LockGuard<decltype(_lock)> guard(_lock);

	_source = src;
	_line = ln;
}

bool Workspace::SourcePosition::getAndClear(std::string &src, int &ln) {
	src.clear();
	ln = -1;

	LockGuard<decltype(_lock)> guard(_lock);

	if (_source.empty() || _line < 0)
		return false;

	src = _source;
	ln = _line;

	_source.clear();
	_line = -1;

	return true;
}

Workspace::Workspace() {
	consoleTextBox(new ImGui::CodeEditor());
}

Workspace::~Workspace() {
	delete consoleTextBox();
	consoleTextBox(nullptr);
}

bool Workspace::open(class Window* wnd, class Renderer* rnd, const class Project* project, Executable* exec, class Primitives* primitives, const Text::Dictionary &options) {
	// Prepare.
	Platform::threadName("BITTY");

	LockGuard<RecursiveMutex>::UniquePtr acquired;
	Project* prj = project->acquire(acquired);

	ImGuiStyle &style = ImGui::GetStyle();
	style.ScrollbarRounding = 0;
	style.TabRounding = 0;

	// Initialize properties.
	busy(false);

	activeFrameRate(BITTY_ACTIVE_FRAME_RATE);

	currentState(Executable::READY);

	pluginsEnabled(options.find(WORKSPACE_OPTION_PLUGIN_DISABLED_KEY) == options.end());
	pluginsMenuProjectItemCount(0);
	pluginsMenuPluginsItemCount(0);
	pluginsMenuHelpItemCount(0);

	splashCustomized(false);

	effectCustomized(false);

	menuHeight(0.0f);
	bannerHeight(0.0f);
	bannerVisible(&settings()->bannerVisible);
	headVisible(false);

	assetsWidth(0.0f);
	assetsVisible(&settings()->assetsVisible);
	assetsResizing(false);
	assetsFocused(false);
	assetsSelectedIndex(-1);
	assetsEditingIndex(-1);
	assetsFiltering(false);
	assetsFilteringInitialized(false);

	bodyArea(Rect(0.0f, 0.0f, 0.0f, 0.0f));

	Text::Dictionary::const_iterator sizeOpt = options.find(WORKSPACE_OPTION_WINDOW_SIZE_KEY);
	if (sizeOpt != options.end()) {
		const std::string sizeStr = sizeOpt->second;
		const Text::Array sizeArr = Text::split(sizeStr, "x");

		do {
			if (sizeArr.size() != 2)
				break;
			int w = 0, h = 0;
			if (!Text::fromString(sizeArr[0], w) || !Text::fromString(sizeArr[1], h))
				break;
			if (w < WINDOW_MIN_WIDTH || h < WINDOW_MIN_HEIGHT)
				break;

			const Math::Vec2i size(w, h);
			settings()->applicationWindowSize = size;
#if !defined BITTY_OS_HTML
			wnd->size(settings()->applicationWindowSize);
			resizeApplication(
				Math::Vec2i(
					settings()->applicationWindowSize.x / rnd->scale(),
					settings()->applicationWindowSize.y / rnd->scale()
				)
			);
#endif /* BITTY_OS_HTML */
			wnd->displayIndex(settings()->applicationWindowDisplayIndex);
			settings()->applicationWindowFullscreen = false;
			settings()->applicationWindowMaximized = false;
		} while (false);
	}

	beginSplash(wnd, rnd, project);

	if (prj) {
		prj->preference(settings()->projectPreference);
		prj->ignoreDotFiles(settings()->projectIgnoreDotFiles);
	}

	editingClosing(false);

	canvasState(&settings()->canvasState);
	canvasFixRatio(&settings()->canvasFixRatio);
	canvasValidation(Math::Vec2i(0, 0));
	canvasSize_(Math::Vec2i(BITTY_CANVAS_DEFAULT_WIDTH, BITTY_CANVAS_DEFAULT_HEIGHT));
	canvasHovering(false);
	canvasFull(false);
	canvasInitialized(false);
	canvasFocused(false);

	documentInitialized(false);

	debugWidth(0.0f);
	debugVisible(&settings()->debugVisible);
	debugShown(false);
	debugResizing(false);
	debugActiveFrameIndex(0);
	debugStopping() = false;

	consoleHeight(0.0f);
	consoleVisible(&settings()->consoleVisible);
	consoleResizing(false);
	consoleFocused(false);

	// Initialize the console.
	consoleTextBox()->SetLanguageDefinition(ImGui::CodeEditor::LanguageDefinition::Text());
	consoleTextBox()->DisableShortcut(ImGui::CodeEditor::UndoRedo);
	consoleTextBox()->SetReadOnly(true);
	consoleTextBox()->SetShowLineNumbers(false);
	consoleTextBox()->SetShowWhiteSpaces(false);
	consoleTextBox()->SetTooltipEnabled(false);
	consoleEnabled(true);

	// Config the primitives module.
	primitives->input()->config(settings()->inputGamepads, INPUT_GAMEPAD_COUNT);

	// Config the recorder.
	recorder(
		Recorder::create(
			[&] (void) -> promise::Defer {
				return Operations::popupWait(
					rnd, this,
					theme()->dialogPrompt_Writing().c_str()
				);
			}
		)
	);

	// Load an initial project.
	loadProject(rnd, project, exec);

	// Load examples.
	loadExamples(rnd, project);

	// Load plugins.
	loadPlugins(rnd, project);

	// Load documents.
	loadDocuments();

	// Finish.
	fprintf(stdout, "Workspace opened.\n");

	return true;
}

bool Workspace::close(class Window* /* wnd */, class Renderer* /* rnd */, const class Project* project, Executable* exec) {
	// Dispose promise.
	popupPromiseType(NONE);
	popupPromise(nullptr);
	popupPromiseHandler(nullptr);
	popupPromiseContent().clear();
	popupPromiseDefault().clear();
	popupPromiseConfirmText().clear();
	popupPromiseDenyText().clear();
	popupPromiseCancelText().clear();

	// Unload documents.
	unloadDocuments();

	// Unload plugins.
	unloadPlugins();

	// Unload examples.
	unloadExamples();

	// Unload the initial project.
	unloadProject(project, exec);

	// Dispose the recorder.
	Recorder::destroy(recorder());
	recorder(nullptr);

	// Dispose properties.
	if (document()) {
		Document::destroy(document());
		document(nullptr);
	}

	// Finish.
	fprintf(stdout, "Workspace closed.\n");

	return true;
}

bool Workspace::canUseShortcuts(void) const {
	if (canvasFull())
		return false;

	return !popupBox();
}

bool Workspace::canSaveTo(const char* path) const {
#if defined BITTY_DEBUG
	ImGuiIO &io = ImGui::GetIO();

	if (io.KeyShift)
		return true;
#endif /* BITTY_DEBUG */

	const std::string abspath = Path::absoluteOf(WORKSPACE_EXAMPLE_PROJECT_DIR);
	if (Path::isParentOf(abspath.c_str(), path))
		return false;

	return true;
}

void Workspace::touchedFile(const char*) {
	// Do nothing.
}

void Workspace::touchedDirectory(const char*) {
	// Do nothing.
}

void Workspace::touchedExample(const char*) {
	// Do nothing.
}

void Workspace::clear(void) {
	LockGuard<decltype(consoleLock())> guard(consoleLock());

#if defined BITTY_OS_HTML
	const bool withConsole = false;
#else /* BITTY_OS_HTML */
	const bool withConsole = consoleEnabled();
#endif /* BITTY_OS_HTML */
	if (withConsole) {
		consoleTextBox()->SetText("");
	}
}

bool Workspace::print(const char* msg) {
	LockGuard<decltype(consoleLock())> guard(consoleLock());

#if defined BITTY_OS_HTML
	const bool withConsole = false;
#else /* BITTY_OS_HTML */
	const bool withConsole = consoleEnabled();
#endif /* BITTY_OS_HTML */
	if (withConsole) {
		consoleTextBox()->AppendText(msg, theme()->style()->messageColor);
		consoleTextBox()->AppendText("\n", theme()->style()->messageColor);
		consoleTextBox()->MoveBottom();
	}

	const std::string osstr = Unicode::toOs(msg);
	fprintf(stdout, "%s\n", osstr.c_str());

	return true;
}

bool Workspace::warn(const char* msg) {
	LockGuard<decltype(consoleLock())> guard(consoleLock());

#if defined BITTY_OS_HTML
	const bool withConsole = false;
#else /* BITTY_OS_HTML */
	const bool withConsole = consoleEnabled();
#endif /* BITTY_OS_HTML */
	if (withConsole) {
		consoleTextBox()->AppendText(msg, theme()->style()->warningColor);
		consoleTextBox()->AppendText("\n", theme()->style()->warningColor);
		consoleTextBox()->MoveBottom();
	}

	const std::string osstr = Unicode::toOs(msg);
	fprintf(stderr, "%s\n", osstr.c_str());

	return true;
}

bool Workspace::error(const char* msg) {
	LockGuard<decltype(consoleLock())> guard(consoleLock());

#if defined BITTY_OS_HTML
	const bool withConsole = false;
#else /* BITTY_OS_HTML */
	const bool withConsole = consoleEnabled();
#endif /* BITTY_OS_HTML */
	if (withConsole) {
		consoleTextBox()->AppendText(msg, theme()->style()->errorColor);
		consoleTextBox()->AppendText("\n", theme()->style()->errorColor);
		consoleTextBox()->MoveBottom();
	}

	const std::string osstr = Unicode::toOs(msg);
	fprintf(stderr, "%s\n", osstr.c_str());

	return true;
}

bool Workspace::promising(void) {
	LockGuard<decltype(popupPromiseLock())> guard(popupPromiseLock());

	if (popupPromise())
		return true;

	return false;
}

void Workspace::promise(Promise::Ptr &promise, Executable::PromiseHandler handler) {
	LockGuard<decltype(popupPromiseLock())> guard(popupPromiseLock());

	if (promise && handler) {
		popupPromiseType(FUNCTION);
		popupPromise(promise);
		popupPromiseHandler(handler);
		popupPromiseContent().clear();
		popupPromiseDefault().clear();
		popupPromiseConfirmText().clear();
		popupPromiseDenyText().clear();
		popupPromiseCancelText().clear();
	} else {
		popupPromiseType(NONE);
		popupPromise(nullptr);
		popupPromiseHandler(nullptr);
		popupPromiseContent().clear();
		popupPromiseDefault().clear();
		popupPromiseConfirmText().clear();
		popupPromiseDenyText().clear();
		popupPromiseCancelText().clear();
	}

	popupPromiseInit().reset();
}

void Workspace::waitbox(Promise::Ptr &promise, const char* content) {
	LockGuard<decltype(popupPromiseLock())> guard(popupPromiseLock());

	if (promise && content) {
		popupPromiseType(WAIT);
		popupPromise(promise);
		popupPromiseHandler(nullptr);
		popupPromiseContent(content);
		popupPromiseDefault().clear();
		popupPromiseConfirmText().clear();
		popupPromiseDenyText().clear();
		popupPromiseCancelText().clear();
	} else {
		popupPromiseType(NONE);
		popupPromise(nullptr);
		popupPromiseHandler(nullptr);
		popupPromiseContent().clear();
		popupPromiseDefault().clear();
		popupPromiseConfirmText().clear();
		popupPromiseDenyText().clear();
		popupPromiseCancelText().clear();
	}

	popupPromiseInit().reset();
}

void Workspace::msgbox(Promise::Ptr &promise, const char* msg, const char* confirmTxt, const char* denyTxt, const char* cancelTxt) {
	LockGuard<decltype(popupPromiseLock())> guard(popupPromiseLock());

	if (promise && msg) {
		popupPromiseType(MSGBOX);
		popupPromise(promise);
		popupPromiseHandler(nullptr);
		popupPromiseContent(msg);
		popupPromiseDefault().clear();
		if (confirmTxt)
			popupPromiseConfirmText(confirmTxt);
		else
			popupPromiseConfirmText().clear();
		if (denyTxt)
			popupPromiseDenyText(confirmTxt);
		else
			popupPromiseDenyText().clear();
		if (cancelTxt)
			popupPromiseCancelText(confirmTxt);
		else
			popupPromiseCancelText().clear();
	} else {
		popupPromiseType(NONE);
		popupPromise(nullptr);
		popupPromiseHandler(nullptr);
		popupPromiseContent().clear();
		popupPromiseDefault().clear();
		popupPromiseConfirmText().clear();
		popupPromiseDenyText().clear();
		popupPromiseCancelText().clear();
	}

	popupPromiseInit().reset();
}

void Workspace::input(Promise::Ptr &promise, const char* prompt, const char* default_) {
	LockGuard<decltype(popupPromiseLock())> guard(popupPromiseLock());

	if (promise && prompt) {
		popupPromiseType(INPUT);
		popupPromise(promise);
		popupPromiseHandler(nullptr);
		popupPromiseContent(prompt);
		if (default_)
			popupPromiseDefault(default_);
		else
			popupPromiseDefault().clear();
		popupPromiseConfirmText().clear();
		popupPromiseDenyText().clear();
		popupPromiseCancelText().clear();
	} else {
		popupPromiseType(NONE);
		popupPromise(nullptr);
		popupPromiseHandler(nullptr);
		popupPromiseContent().clear();
		popupPromiseDefault().clear();
		popupPromiseConfirmText().clear();
		popupPromiseDenyText().clear();
		popupPromiseCancelText().clear();
	}

	popupPromiseInit().reset();
}

bool Workspace::focus(const char* src, int ln) {
	debugProgramPointer().set(src, ln); // 1-based.

	return true;
}

void Workspace::require(Executable*) {
	assert(false && "Not implemented.");
}

void Workspace::stop(void) {
	debugStopping() = true;
}

Math::Vec2i Workspace::applicationSize(void) {
	LockGuard<decltype(applicationSizeLock())> guard(applicationSizeLock());

	return Math::Vec2i(std::abs(applicationSize_().x), std::abs(applicationSize_().y));
}

bool Workspace::resizeApplication(const Math::Vec2i &size) {
	LockGuard<decltype(applicationSizeLock())> guard(applicationSizeLock());

	if (size.x == applicationSize_().x && size.y == applicationSize_().y)
		return true;

	applicationSize_(size);

	return true;
}

Math::Vec2i Workspace::canvasSize(void) {
	LockGuard<decltype(canvasSizeLock())> guard(canvasSizeLock());

	return Math::Vec2i(std::abs(canvasSize_().x), std::abs(canvasSize_().y));
}

bool Workspace::resizeCanvas(const Math::Vec2i &size) {
	if (size.x > BITTY_CANVAS_MAX_WIDTH || size.y > BITTY_CANVAS_MAX_HEIGHT)
		return false;

	LockGuard<decltype(canvasSizeLock())> guard(canvasSizeLock());

	if (size.x == canvasSize_().x && size.y == canvasSize_().y)
		return true;

	canvasSize_(size);

	return true;
}

void Workspace::effect(const char* material) {
	if (material) {
		effectCustomized(true);
		effectConfig(material);
	} else {
		effectCustomized(true);
		effectConfig().clear();
	}
}

void Workspace::focusGained(class Window* /* wnd */, class Renderer* /* rnd */, const class Project* /* project */, Executable* /* exec */, class Primitives* /* primitives */) {
	// Do nothing.
}

void Workspace::focusLost(class Window* wnd, class Renderer* rnd, const class Project* project, Executable* /* exec */, class Primitives* primitives) {
	save(wnd, rnd, project, primitives);
}

void Workspace::renderTargetsReset(class Window* /* wnd */, class Renderer* /* rnd */, const class Project* /* project */, Executable* /* exec */, class Primitives* /* primitives */) {
	// Do nothing.
}

void Workspace::resized(class Window* wnd, class Renderer* rnd, const class Project* project, const Math::Vec2i &size) {
	if (!wnd->maximized() && !wnd->fullscreen())
		settings()->applicationWindowSize = size;

	withEditingAsset(
		project,
		[&] (Asset*, Editable* editor) -> void {
			editor->resized(rnd, project);
		}
	);
}

void Workspace::maximized(class Window*, class Renderer*) {
	settings()->applicationWindowFullscreen = false;
	settings()->applicationWindowMaximized = true;
}

void Workspace::restored(class Window* wnd, class Renderer*) {
	settings()->applicationWindowFullscreen = wnd->fullscreen();
	settings()->applicationWindowMaximized = false;
}

bool Workspace::quit(class Window*, class Renderer* rnd, const class Project* project, Executable* exec, class Primitives* primitives) {
	do {
		LockGuard<RecursiveMutex>::UniquePtr acquired;
		Project* prj = project->acquire(acquired);
		if (!prj)
			break;

		if (!prj->dirty())
			break;

		busy(true);

		Operations::projectStop(rnd, this, project, exec, primitives);

#if BITTY_TRIAL_ENABLED
		Operations::fileClose(rnd, this, project, exec)
			.then(
				[this] (bool saved) -> void {
					busy(false);

					if (!saved) {
						SDL_Event evt;
						evt.type = SDL_QUIT;
						SDL_PushEvent(&evt);
					}
				}
			)
			.fail(
				[this] (void) -> void {
					busy(false);
				}
			);
#else /* BITTY_TRIAL_ENABLED */
		Operations::fileClose(rnd, this, project, exec)
			.then(
				[this] (void) -> void {
					busy(false);

					SDL_Event evt;
					evt.type = SDL_QUIT;
					SDL_PushEvent(&evt);
				}
			)
			.fail(
				[this] (void) -> void {
					busy(false);
				}
			);
#endif /* BITTY_TRIAL_ENABLED */
	} while (false);

	if (busy())
		return false;

	return true;
}

Variant Workspace::post(unsigned msg, int argc, const Variant* argv) {
	switch (msg) {
	case Editable::ON_TOGGLE_BREAKPOINT: {
			if (popupBox())
				break;

			const std::string name = unpack<std::string>(argc, argv, 0, "");
			const Variant::Int ln = unpack<Variant::Int>(argc, argv, 1, -1);
			const Project* project = (const Project*)unpack<void*>(argc, argv, 2, nullptr);
			Executable* exec = (Executable*)unpack<void*>(argc, argv, 3, nullptr);

			Operations::debugToggleBreakpoint(this, project, exec, name.c_str(), ln);
		}

		break;
	}

	return Variant();
}

bool Workspace::load(class Window* wnd, class Renderer* rnd, const class Project* project, class Primitives* primitives, const rapidjson::Document &doc) {
	LockGuard<RecursiveMutex>::UniquePtr acquired;
	Project* prj = project->acquire(acquired);

	Jpath::get(doc, settings()->applicationWindowDisplayIndex, "application", "window", "display_index");
	Jpath::get(doc, settings()->applicationWindowFullscreen, "application", "window", "fullscreen");
	Jpath::get(doc, settings()->applicationWindowMaximized, "application", "window", "maximized");
	Jpath::get(doc, settings()->applicationWindowSize.x, "application", "window", "size", 0);
	Jpath::get(doc, settings()->applicationWindowSize.y, "application", "window", "size", 1);
	Jpath::get(doc, settings()->applicationPauseOnFocusLost, "application", "pause_on_focus_lost");
	Jpath::get(doc, settings()->applicationPauseOnEsc, "application", "pause_on_esc");

	Jpath::get(doc, settings()->bannerVisible, "banner", "visible");
	Jpath::get(doc, settings()->assetsVisible, "assets", "visible");

	Jpath::get(doc, settings()->projectPreference, "project", "preference");
	Jpath::get(doc, settings()->projectIgnoreDotFiles, "project", "ignore_dot_files");
	Jpath::get(doc, settings()->projectAutoBackup, "project", "auto_backup");

	Jpath::get(doc, settings()->editorShowWhiteSpaces, "editor", "show_white_spaces");
	Jpath::get(doc, settings()->editorCaseSensitive, "editor", "case_sensitive");
	Jpath::get(doc, settings()->editorMatchWholeWord, "editor", "match_whole_word");

	Jpath::get(doc, settings()->canvasState, "canvas", "state");
	Jpath::get(doc, settings()->canvasFixRatio, "canvas", "fix_ratio");

	Jpath::get(doc, settings()->debugVisible, "debug", "visible");

	Jpath::get(doc, settings()->consoleVisible, "console", "visible");
	Jpath::get(doc, settings()->consoleClearOnStart, "console", "clear_on_start");

	for (int i = 0; i < INPUT_GAMEPAD_COUNT; ++i) {
		Input::Gamepad &pad = settings()->inputGamepads[i];
		for (int j = 0; j < Input::BUTTON_COUNT; ++j) {
			unsigned dev = pad.buttons[j].device;
			Jpath::get(doc, dev, "input", "gamepad", i, j, "device");
			Jpath::get(doc, pad.buttons[j].index, "input", "gamepad", i, j, "index");
			unsigned type = pad.buttons[j].type;
			Jpath::get(doc, type, "input", "gamepad", i, j, "type");
			pad.buttons[j].type = (Input::Types)type;
			switch (pad.buttons[j].type) {
			case Input::VALUE:
				Jpath::get(doc, pad.buttons[j].value, "input", "gamepad", i, j, "value");

				break;
			case Input::HAT: {
					Jpath::get(doc, pad.buttons[j].hat.index, "input", "gamepad", i, j, "sub");
					unsigned short subType = pad.buttons[j].hat.value;
					Jpath::get(doc, subType, "input", "gamepad", i, j, "value");
					pad.buttons[j].hat.value = (Input::Hat::Types)subType;
				}

				break;
			case Input::AXIS:
				Jpath::get(doc, pad.buttons[j].axis.index, "input", "gamepad", i, j, "sub");
				Jpath::get(doc, pad.buttons[j].axis.value, "input", "gamepad", i, j, "value");

				break;
			}

			pad.buttons[j].device = (Input::Devices)dev;
		}
	}
	Jpath::get(doc, settings()->inputOnscreenGamepadEnabled, "input", "onscreen_gamepad", "enabled");
	Jpath::get(doc, settings()->inputOnscreenGamepadSwapAB, "input", "onscreen_gamepad", "swap_ab");
	Jpath::get(doc, settings()->inputOnscreenGamepadScale, "input", "onscreen_gamepad", "scale");
	Jpath::get(doc, settings()->inputOnscreenGamepadPadding.x, "input", "onscreen_gamepad", "padding", 0);
	Jpath::get(doc, settings()->inputOnscreenGamepadPadding.y, "input", "onscreen_gamepad", "padding", 1);

	const Math::Vec2i size = wnd->size();
	if (settings()->applicationWindowSize == Math::Vec2i(0, 0))
		settings()->applicationWindowSize = size;
#if !defined BITTY_OS_HTML
	if (size != settings()->applicationWindowSize)
		wnd->size(settings()->applicationWindowSize);
	resizeApplication(
		Math::Vec2i(
			settings()->applicationWindowSize.x / rnd->scale(),
			settings()->applicationWindowSize.y / rnd->scale()
		)
	);
#endif /* BITTY_OS_HTML */
	wnd->displayIndex(settings()->applicationWindowDisplayIndex);
	if (settings()->applicationWindowFullscreen)
		wnd->fullscreen(true);
	else if (settings()->applicationWindowMaximized)
		wnd->maximize();

	if (prj) {
		prj->preference(settings()->projectPreference);
		prj->ignoreDotFiles(settings()->projectIgnoreDotFiles);
	}

	primitives->input()->config(settings()->inputGamepads, INPUT_GAMEPAD_COUNT);

	return true;
}

bool Workspace::save(class Window* wnd, class Renderer*, const class Project*, class Primitives*, rapidjson::Document &doc) {
	settings()->applicationWindowDisplayIndex = wnd->displayIndex();

	Jpath::set(doc, doc, settings()->applicationWindowDisplayIndex, "application", "window", "display_index");
	Jpath::set(doc, doc, settings()->applicationWindowFullscreen, "application", "window", "fullscreen");
	Jpath::set(doc, doc, settings()->applicationWindowMaximized, "application", "window", "maximized");
	Jpath::set(doc, doc, settings()->applicationWindowSize.x, "application", "window", "size", 0);
	Jpath::set(doc, doc, settings()->applicationWindowSize.y, "application", "window", "size", 1);
	Jpath::set(doc, doc, settings()->applicationPauseOnFocusLost, "application", "pause_on_focus_lost");
	Jpath::set(doc, doc, settings()->applicationPauseOnEsc, "application", "pause_on_esc");

	Jpath::set(doc, doc, settings()->bannerVisible, "banner", "visible");
	Jpath::set(doc, doc, settings()->assetsVisible, "assets", "visible");

	Jpath::set(doc, doc, settings()->projectPreference, "project", "preference");
	Jpath::set(doc, doc, settings()->projectIgnoreDotFiles, "project", "ignore_dot_files");
	Jpath::set(doc, doc, settings()->projectAutoBackup, "project", "auto_backup");

	Jpath::set(doc, doc, settings()->editorShowWhiteSpaces, "editor", "show_white_spaces");
	Jpath::set(doc, doc, settings()->editorCaseSensitive, "editor", "case_sensitive");
	Jpath::set(doc, doc, settings()->editorMatchWholeWord, "editor", "match_whole_word");

	Jpath::set(doc, doc, settings()->canvasState, "canvas", "state");
	Jpath::set(doc, doc, settings()->canvasFixRatio, "canvas", "fix_ratio");

	Jpath::set(doc, doc, settings()->debugVisible, "debug", "visible");

	Jpath::set(doc, doc, settings()->consoleVisible, "console", "visible");
	Jpath::set(doc, doc, settings()->consoleClearOnStart, "console", "clear_on_start");

	for (int i = 0; i < INPUT_GAMEPAD_COUNT; ++i) {
		const Input::Gamepad &pad = settings()->inputGamepads[i];
		for (int j = 0; j < Input::BUTTON_COUNT; ++j) {
			const unsigned dev = pad.buttons[j].device;
			Jpath::set(doc, doc, dev, "input", "gamepad", i, j, "device");
			Jpath::set(doc, doc, pad.buttons[j].index, "input", "gamepad", i, j, "index");
			Jpath::set(doc, doc, (unsigned short)pad.buttons[j].type, "input", "gamepad", i, j, "type");
			switch (pad.buttons[j].type) {
			case Input::VALUE:
				Jpath::set(doc, doc, pad.buttons[j].value, "input", "gamepad", i, j, "value");

				break;
			case Input::HAT:
				Jpath::set(doc, doc, pad.buttons[j].hat.index, "input", "gamepad", i, j, "sub");
				Jpath::set(doc, doc, (unsigned short)pad.buttons[j].hat.value, "input", "gamepad", i, j, "value");

				break;
			case Input::AXIS:
				Jpath::set(doc, doc, pad.buttons[j].axis.index, "input", "gamepad", i, j, "sub");
				Jpath::set(doc, doc, pad.buttons[j].axis.value, "input", "gamepad", i, j, "value");

				break;
			}
		}
	}
	Jpath::set(doc, doc, settings()->inputOnscreenGamepadEnabled, "input", "onscreen_gamepad", "enabled");
	Jpath::set(doc, doc, settings()->inputOnscreenGamepadSwapAB, "input", "onscreen_gamepad", "swap_ab");
	Jpath::set(doc, doc, settings()->inputOnscreenGamepadScale, "input", "onscreen_gamepad", "scale");
	Jpath::set(doc, doc, settings()->inputOnscreenGamepadPadding.x, "input", "onscreen_gamepad", "padding", 0);
	Jpath::set(doc, doc, settings()->inputOnscreenGamepadPadding.y, "input", "onscreen_gamepad", "padding", 1);

	return true;
}

void Workspace::loadProject(class Renderer* rnd, const class Project* project, Executable* exec) {
	Operations::fileNew(rnd, this, project, exec);
}

void Workspace::unloadProject(const class Project* project, Executable* exec) {
	canvasFull(false);

	exec->clearBreakpoints(nullptr);

	LockGuard<RecursiveMutex>::UniquePtr acquired;
	Project* prj = project->acquire(acquired);
	if (!prj)
		return;

	prj->unload();
	prj->readonly(false);
}

void Workspace::loadExamples(class Renderer* rnd, const class Project* project) {
	LockGuard<RecursiveMutex>::UniquePtr acquired;
	Project* prj = project->acquire(acquired);

	DirectoryInfo::Ptr dirInfo = DirectoryInfo::make(WORKSPACE_EXAMPLE_PROJECT_DIR);
	FileInfos::Ptr fileInfos = dirInfo->getFiles("*." BITTY_PROJECT_EXT, true);
	for (int i = 0; i < fileInfos->count(); ++i) {
		FileInfo::Ptr fileInfo = fileInfos->get(i);

		const std::string path = fileInfo->fullPath();

		std::shared_ptr<Project> newPrj(new Project());
		if (prj)
			newPrj->loader(prj->loader());
		newPrj->factory(prj->factory());
		newPrj->open(rnd);
		if (newPrj->load(path.c_str())) {
			const Entry entry = newPrj->title();
			examples()[entry] = path;

			newPrj->unload();
		}
		newPrj->close();
		newPrj->loader(nullptr);

		Platform::idle();
	}
}

void Workspace::unloadExamples(void) {
	examples().clear();
}

void Workspace::loadPlugins(class Renderer* rnd, const class Project* project) {
	if (!pluginsEnabled())
		return;

	auto load = [this] (Renderer* rnd, const Project* project, const DirectoryInfo::Ptr &dirInfo, const FileInfo::Ptr &fileInfo) -> bool {
		const std::string package = dirInfo->fullPath();
		const std::string &entry = fileInfo->fullPath();

		Plugin* plugin = new Plugin(
			rnd,
			this,
			project,
			entry.c_str()
		);
		if (plugin->open()) {
			if (plugin->instant())
				plugin->close();

			Plugin::Array::const_iterator exists = std::find_if(
				plugins().begin(), plugins().end(),
				[&] (const Plugin *val) -> bool {
					return Entry::compare(val->entry(), plugin->entry()) == 0;
				}
			);
			if (exists != plugins().end()) {
				const std::string msg = Text::cformat("Ignored duplicate plugin: \"%s\".\n", (*exists)->entry().c_str());
				warn(msg.c_str());

				plugin->close();
				delete plugin;

				return false;
			}

			plugins().push_back(plugin);

			const Text::Array parts = plugin->entry().parts();
			if (!parts.empty() && parts.front() == PLUGIN_MENU_PROJECT_NAME)
				pluginsMenuProjectItemCount(pluginsMenuProjectItemCount() + 1);
			if (!parts.empty() && parts.front() == PLUGIN_MENU_PLUGIN_NAME)
				pluginsMenuPluginsItemCount(pluginsMenuPluginsItemCount() + 1);
			if (!parts.empty() && parts.front() == PLUGIN_MENU_HELP_NAME)
				pluginsMenuHelpItemCount(pluginsMenuHelpItemCount() + 1);
		} else {
			plugin->close();
			delete plugin;

			return false;
		}

		return true;
	};

	DirectoryInfo::Ptr dirInfo = DirectoryInfo::make(PLUGIN_BUILTIN_DIR);
	FileInfos::Ptr fileInfos = dirInfo->getFiles("*." BITTY_PROJECT_EXT, true);
	for (int i = 0; i < fileInfos->count(); ++i) {
		FileInfo::Ptr fileInfo = fileInfos->get(i);

		load(rnd, project, dirInfo, fileInfo);

		Platform::idle();
	}

	const std::string customDir = Path::combine(Path::writableDirectory().c_str(), PLUGIN_CUSTOM_DIR);
	dirInfo = DirectoryInfo::make(customDir.c_str());
	fileInfos = dirInfo->getFiles("*." BITTY_PROJECT_EXT, true);
	for (int i = 0; i < fileInfos->count(); ++i) {
		FileInfo::Ptr fileInfo = fileInfos->get(i);

		load(rnd, project, dirInfo, fileInfo);

		Platform::idle();
	}

	std::sort(
		plugins().begin(), plugins().end(),
		[] (const Plugin* left, const Plugin* right) -> bool {
			if (left->order() != right->order())
				return left->order() < right->order();

			return Entry::compare(left->entry(), right->entry()) < 0;
		}
	);
}

void Workspace::unloadPlugins(void) {
	pluginsMenuProjectItemCount(0);
	pluginsMenuPluginsItemCount(0);
	pluginsMenuHelpItemCount(0);

	for (Plugin* plugin : plugins()) {
		plugin->close();
		delete plugin;
	}
	plugins().clear();
}

void Workspace::loadDocuments(void) {
	DirectoryInfo::Ptr dirInfo = DirectoryInfo::make(DOCUMENT_MARKDOWN_DIR);
	FileInfos::Ptr fileInfos = dirInfo->getFiles("*." DOCUMENT_MARKDOWN_EXT, true);
	for (int i = 0; i < fileInfos->count(); ++i) {
		FileInfo::Ptr fileInfo = fileInfos->get(i);

		const std::string package = dirInfo->fullPath();
		const std::string path = fileInfo->fullPath();
		std::string entry = path.substr(package.length());
		entry = entry.substr(0, entry.length() - strlen("." DOCUMENT_MARKDOWN_EXT));

		documents()[Entry(entry)] = path;

		Platform::idle();
	}
}

void Workspace::unloadDocuments(void) {
	documents().clear();
}

void Workspace::execute(class Window* /* wnd */, class Renderer* rnd, const class Project* /* project */, Executable* exec, class Primitives* primitives, double delta, bool alive) {
	currentState(exec ? exec->current() : Executable::READY);

	if (!exec)
		return;

	if (executing() && !canvasTexture()) {
		canvasTexture(Texture::Ptr(Texture::create()));
		canvasTexture()->scale(canvasScaleMode());
		canvasTexture()->blend(Texture::BLEND);

		primitives->canvas(canvasTexture());
	}

	if (!alive || halting()) {
		primitives->forbid();

		exec->stop();

		primitives->reset();
	}

#if BITTY_MULTITHREAD_ENABLED
	(void)rnd;

	exec->update(delta);
#else /* BITTY_MULTITHREAD_ENABLED */
	if (executing()) {
		BITTY_RENDER_TARGET(rnd, canvasTexture().get())
		BITTY_RENDER_SCALE(rnd, 1)
		exec->update(delta);

		const Executable::States current = exec ? exec->current() : Executable::READY;
		if (current != Executable::RUNNING && current != Executable::PAUSED)
			currentState(Executable::HALTING);
	}
#endif /* BITTY_MULTITHREAD_ENABLED */
}

void Workspace::prepare(class Window* /* wnd */, class Renderer* rnd, const class Project* project, Executable* exec, class Primitives* primitives) {
	if (debugStopping()) {
		Operations::projectStop(rnd, this, project, exec, primitives);
	}
}

void Workspace::dialog(class Window* /* wnd */, class Renderer* /* rnd */, const class Project*) {
	if (!init().end())
		return;

	if (popupBox())
		popupBox()->update();
}

void Workspace::banner(class Window* /* wnd */, class Renderer* rnd, const class Project* project, Executable* exec, class Primitives* primitives) {
	if (!*bannerVisible()) {
		bannerHeight(0);

		return;
	}

	if (immersive())
		return;

	ImGuiIO &io = ImGui::GetIO();
	ImGuiStyle &style = ImGui::GetStyle();

	VariableGuard<decltype(style.WindowBorderSize)> guardBorderSize(&style.WindowBorderSize, style.WindowBorderSize, 0.0f);

	ImGui::SetNextWindowPos(ImVec2(0.0f, menuHeight()), ImGuiCond_Always);
	ImGui::SetNextWindowSize(
		ImVec2((float)rnd->width(), bannerHeight()),
		ImGuiCond_Always
	);
	if (ImGui::Begin("@Bnr", nullptr, WORKSPACE_WND_FLAGS_DOCK_NO_TITLE)) {
		const ImVec2 buttonSize(32 * io.FontGlobalScale, 32 * io.FontGlobalScale);

		bool any = false;
		unsigned type = 0;
		bool pastable = false;
		const char* undoable = nullptr;
		const char* redoable = nullptr;
		editingAssetStates(project, &any, &type, nullptr, nullptr, &pastable, nullptr, &undoable, &redoable);

		switch (currentState()) {
		case Executable::READY:
			if (!executing() || paused()) {
				if (ImGui::ImageButton(theme()->iconPlay()->pointer(rnd), buttonSize)) {
					Operations::projectRun(rnd, this, project, exec, primitives);
				}
			} else {
				ImGui::ImageButton(theme()->iconPlay_Gray()->pointer(rnd), buttonSize);
			}

			break;
		case Executable::RUNNING: // Fall through.
		case Executable::PAUSED:
			if (ImGui::ImageButton(theme()->iconStop()->pointer(rnd), buttonSize)) {
				Operations::projectStop(rnd, this, project, exec, primitives);
			}

			break;
		case Executable::HALTING:
			ImGui::ImageButton(theme()->iconPlay()->pointer(rnd), buttonSize);

			break;
		}
		ImGui::SameLine();

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
		if (any) {
			if (ImGui::ImageButton(theme()->iconCopy()->pointer(rnd), buttonSize)) {
				withEditingAsset(
					project,
					[] (Asset*, Editable* editor) -> void {
						editor->copy();
					}
				);
			}
		} else {
			ImGui::ImageButton(theme()->iconCopy_Gray()->pointer(rnd), buttonSize);
		}
		ImGui::SameLine();
		if (any) {
			if (ImGui::ImageButton(theme()->iconCut()->pointer(rnd), buttonSize)) {
				withEditingAsset(
					project,
					[] (Asset*, Editable* editor) -> void {
						editor->cut();
					}
				);
			}
		} else {
			ImGui::ImageButton(theme()->iconCut_Gray()->pointer(rnd), buttonSize);
		}
		ImGui::SameLine();
		if (any && pastable) {
			if (ImGui::ImageButton(theme()->iconPaste()->pointer(rnd), buttonSize)) {
				withEditingAsset(
					project,
					[] (Asset*, Editable* editor) -> void {
						editor->paste();
					}
				);
			}
		} else {
			ImGui::ImageButton(theme()->iconPaste_Gray()->pointer(rnd), buttonSize);
		}
		ImGui::PopStyleVar();
		ImGui::SameLine();

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
		if (undoable) {
			if (ImGui::ImageButton(theme()->iconUndo()->pointer(rnd), buttonSize)) {
				withEditingAsset(
					project,
					[] (Asset* asset, Editable* editor) -> void {
						editor->undo(asset);
					}
				);
			}
			if (*undoable && ImGui::IsItemHovered())
				ImGui::SetTooltip(undoable);
		} else {
			ImGui::ImageButton(theme()->iconUndo_Gray()->pointer(rnd), buttonSize);
		}
		ImGui::SameLine();
		if (redoable) {
			if (ImGui::ImageButton(theme()->iconRedo()->pointer(rnd), buttonSize)) {
				withEditingAsset(
					project,
					[] (Asset* asset, Editable* editor) -> void {
						editor->redo(asset);
					}
				);
			}
			if (*redoable && ImGui::IsItemHovered())
				ImGui::SetTooltip(redoable);
		} else {
			ImGui::ImageButton(theme()->iconRedo_Gray()->pointer(rnd), buttonSize);
		}
		ImGui::PopStyleVar();

		bannerHeight(ImGui::GetItemRectSize().y + style.WindowPadding.y * 2);

		ImGui::End();
	}
}

void Workspace::assets(class Window* wnd, class Renderer* rnd, const class Project* project, Executable* exec, class Primitives* primitives) {
	assetsFocused(false);

	if (!*assetsVisible())
		return;

	if (immersive())
		return;

	ImGuiIO &io = ImGui::GetIO();
	ImGuiStyle &style = ImGui::GetStyle();

	VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2());

	const float minWidth = std::min(rnd->width() * 0.257f, 256.0f * io.FontGlobalScale);
	ImGuiWindowFlags flags = WORKSPACE_WND_FLAGS_DOCK;
	if (assetsWidth() <= 0.0f) {
		assetsWidth(minWidth);
	}

	const float gripMarginX = ImGui::WindowResizingPadding().x;
	const float gripPaddingY = 16.0f;
	if (assetsResizing() && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
		assetsResizing(false);

		withEditingAsset(
			project,
			[&] (Asset*, Editable* editor) -> void {
				editor->resized(rnd, project);
			}
		);
	}
	if (
		ImGui::IsMouseHoveringRect(
			ImVec2(assetsWidth() - gripMarginX, bodyArea().yMin() + gripPaddingY),
			ImVec2(assetsWidth(), bodyArea().yMax() - gripPaddingY),
			false
		) &&
		!popupBox() && !headVisible() && !canvasHovering()
	) {
		assetsResizing(true);

		ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
	} else if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
		assetsResizing(false);
	}
	if (assetsResizing()) {
		flags &= ~ImGuiWindowFlags_NoResize;
	}

	ImGui::SetNextWindowPos(ImVec2(0.0f, bodyArea().yMin()), ImGuiCond_Always);
	ImGui::SetNextWindowSize(
		ImVec2(assetsWidth(), bodyArea().height()),
		ImGuiCond_Always
	);
	ImGui::SetNextWindowSizeConstraints(ImVec2(minWidth, -1), ImVec2(rnd->width() * 0.7f, -1));
	if (ImGui::Begin(theme()->windowAssets(), assetsVisible(), flags)) {
		assetsSelectedIndex(-1);
		Asset::List::Index assetsContextIndex(-1);

		filterAssets(wnd, rnd, project, exec);

		ImGui::BeginChild("@Asts", ImVec2(), false, ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar | ImGuiWindowFlags_NoNav);

		auto sel = [] (Project* prj, Asset* asset) -> void {
			prj->foreach(
				[&] (Asset* &asset_, Asset::List::Index) -> void {
					Asset::States* states = asset_->states();

					if (asset_ == asset) {
						states->select();

						if (states->activity() == Asset::States::CLOSED)
							states->activate(Asset::States::INSPECTABLE);
					} else {
						states->deselect();
					}
				}
			);
		};

		ImGui::Hierarchy hierarchy(
			[&] (const std::string &dir) -> bool {
				return ImGui::TreeNode(
					theme()->sliceDirectory()->pointer(rnd), theme()->sliceDirectory_Open()->pointer(rnd),
					dir,
					ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_SpanFullWidth, ImGuiButtonFlags_None,
					theme()->style()->iconColor
				);
			},
			[] (void) -> void {
				ImGui::TreePop();
			}
		);
		hierarchy.prepare();

		do {
			LockGuard<RecursiveMutex>::UniquePtr acquired;
			Project* prj = project->acquire(acquired);
			if (!prj)
				break;

			Asset* infoAsset = prj->info();
			Asset* mainAsset = prj->main();

			VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2());

			prj->foreach(
				[&] (Asset* &asset, Asset::List::Index index) -> void {
					const Entry &entry = asset->entry();
					Asset::States* states = asset->states();

					if (assetsFiltering() && !assetsFilterPatterns().empty()) {
						bool show = false;
						for (const std::string &pattern : assetsFilterPatterns()) {
							if (Text::matchWildcard(entry.name(), pattern.c_str(), true)) {
								show = true;

								break;
							}
						}

						if (!show)
							return;
					}

					Text::Array::const_iterator begin = entry.parts().begin();
					Text::Array::const_iterator end = entry.parts().end() - 1;
					if (entry.parts().size() == 1) {
						begin = entry.parts().end();
						end = entry.parts().end();
					}

					if (hierarchy.with(begin, end)) {
						const std::string &file = entry.parts().back();
						ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_SpanFullWidth;
						if (states->selected()) {
							flags |= ImGuiTreeNodeFlags_Selected;
							if (asset != infoAsset && asset != mainAsset)
								assetsSelectedIndex(index);
						}
						const ImGuiButtonFlags buttonFlags = ImGuiButtonFlags_None;
						if (ImGui::TreeNode(theme()->sliceFile()->pointer(rnd), theme()->sliceFile()->pointer(rnd), file, flags, buttonFlags, theme()->style()->iconColor)) {
							ImGui::TreePop();
						}
						const bool rmb = ImGui::IsItemClicked(ImGuiMouseButton_Right);
						if (rmb || ImGui::IsItemClicked()) {
							sel(prj, asset);
							if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
								states->activate(Asset::States::EDITABLE);

							if (rmb)
								assetsContextIndex = index;
						}
					}
				}
			);
		} while (false);

		if (assetsContextIndex >= 0 || (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)))
			ImGui::OpenPopup("@Asts/Ctx");

		showAssetContextMenu(wnd, rnd, project, exec, primitives);

		hierarchy.finish();

		ImGui::EndChild();

		assetsWidth(ImGui::GetWindowSize().x);

		ImVec2 customBtnPos = ImGui::CustomButtonAutoPosition();
		const bool rem = assetsSelectedIndex() >= 0 && ImGui::TitleBarCustomButton("#Rm", &customBtnPos, ImGui::CustomRemoveButton, theme()->tooltipAssets_Remove().c_str());
		const bool add = ImGui::TitleBarCustomButton("#Add", &customBtnPos, ImGui::CustomAddButton, theme()->tooltipAssets_New().c_str());

		assetsFocused(ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows));

		ImGui::End();

		if (rem) {
			Operations::projectRemoveAsset(rnd, this, project, exec, assetsSelectedIndex());
		}
		if (add) {
			Operations::projectAddAsset(rnd, this, project, assetsSelectedIndex());
		}
	}
}

void Workspace::editing(class Window* wnd, class Renderer* rnd, const class Project* project, Executable* exec, class Primitives* primitives, double delta, bool* indicated) {
	const Asset::List::Index lastEditing = assetsEditingIndex();
	assetsEditingIndex(-1);

	if (immersive())
		return;

	ImGuiStyle &style = ImGui::GetStyle();

	VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2());
	VariableGuard<decltype(style.FramePadding)> guardFramePadding(&style.FramePadding, style.FramePadding, ImVec2());
	VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2());

	const bool debugS = *debugVisible() && executing();
	const float debugW = debugS ? debugWidth() : 0.0f;
	const float consoleH = *consoleVisible() ? consoleHeight() : 0.0f;

	ImGui::SetNextWindowPos(ImVec2(bodyArea().xMin(), bodyArea().yMin()), ImGuiCond_Always);
	ImGui::SetNextWindowSize(
		ImVec2(bodyArea().width() - debugW, bodyArea().height() - consoleH),
		ImGuiCond_Always
	);
	if (ImGui::Begin("@Ed", nullptr, WORKSPACE_WND_FLAGS_DOCK_NO_TITLE)) {
		const ImVec2 pos = ImGui::GetWindowPos();
		const ImVec2 size = ImGui::GetWindowSize();

		if (!init().end())
			ImGui::SetNextWindowFocus();

		VariableGuard<decltype(style.FramePadding)> guardFramePadding_(&style.FramePadding, style.FramePadding, guardFramePadding.previous());

		const ImGuiTabBarFlags tabBarFlags = ImGuiTabBarFlags_Reorderable;
		if (ImGui::BeginTabBar("@Asts", tabBarFlags)) {
			do {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));
				VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing_(&style.ItemSpacing, style.ItemSpacing, guardItemSpacing.previous());

				LockGuard<RecursiveMutex>::UniquePtr acquired;
				Project* prj = project->acquire(acquired);
				if (!prj || prj->empty()) {
					ImGui::TabBarTabListPopupButton(
						[&] (void) -> void {
							ImGui::MenuItem(theme()->generic_None(), nullptr, nullptr, false);
						}
					);

					break;
				}

				ImGui::TabBarTabListPopupButton(
					[&] (void) -> void {
						std::string selected;
						if (ImGui::AssetMenu(project, selected)) {
							if (!prj)
								return;

							Asset* asset = prj->get(selected.c_str());
							if (!asset)
								return;

							asset->prepare(Asset::EDITING, false);

							Asset::States* states = asset->states();
							states->activate(Asset::States::INSPECTABLE);
							states->focus();
						}
					}
				);
			} while (false);

			do {
				LockGuard<RecursiveMutex>::UniquePtr acquired;
				Project* prj = project->acquire(acquired);
				if (!prj)
					break;

				bool switched = false;
				Asset* frontAsset = nullptr;
				prj->foreach( // On the secondary orders.
					[&] (Asset* &asset, Asset::List::Index index) -> void {
						Asset::States* states = asset->states();
						if (states->activity() == Asset::States::CLOSED)
							return;

						const bool unsaved = asset->dirty();
						const bool pending = states->activity() == Asset::States::INSPECTABLE;

						ImGuiTabItemFlags tabItemFlags = ImGuiTabItemFlags_NoTooltip;
						if (states->focusing())
							tabItemFlags |= ImGuiTabItemFlags_SetSelected;
						if (unsaved)
							tabItemFlags |= ImGuiTabItemFlags_UnsavedDocument;

						if (pending) {
							ImGui::PushStyleColor(ImGuiCol_Tab, theme()->style()->tabPendingColor);
							ImGui::PushStyleColor(ImGuiCol_TabHovered, theme()->style()->tabPendingHoveredColor);
							ImGui::PushStyleColor(ImGuiCol_TabActive, theme()->style()->tabPendingColor);
						}

						bool opened = true;
						ImGui::PushStyleColor(ImGuiCol_Text, pending ? theme()->style()->tabTextPendingColor : theme()->style()->tabTextColor);
						if (ImGui::BeginTabItem(asset->entry().name(), &opened, tabItemFlags)) {
							ImGui::PopStyleColor();

							assetsEditingIndex(index);

							frontAsset = asset;

							Editable* editor = asset->editor();

							if (lastEditing != index) {
								switched = true;

								do {
									if (lastEditing == -1)
										break;

									Asset* lastEditingAsset = prj->get(lastEditing);
									if (!lastEditingAsset)
										break;

									Editable* lastEditingEditor = lastEditingAsset->editor();
									if (!lastEditingEditor)
										break;

									lastEditingEditor->lostFocus(rnd, project);
								} while (false);

								if (editor)
									editor->gainFocus(rnd, project);
							}

							if (!editor) {
								if (!asset->object(Asset::EDITING)) {
									const unsigned type = asset->type();
									const std::string ext = asset->extName();
									const unsigned finalType = Operations::projectGetCustomAssetType(rnd, this, project, ext, type);
									if (finalType != type)
										asset->custom(true);
								}

								asset->prepare(Asset::EDITING, false);

								if (!asset->object(Asset::EDITING))
									resolveAssetRef(wnd, rnd, project, asset->entry().c_str());

								editor = asset->editor();

								if (editor) {
									if (executing())
										editor->readonly(true);

									fillAssetEditorSettings(editor);
								}
							}
							if (editor) {
								editor->update(
									wnd, rnd,
									this, project, exec,
									"@Edtr",
									pos.x, pos.y, size.x, size.y - ImGui::TabBarHeight(),
									1.0f, 1.0f,
									pending,
									delta
								);

								if (editor->hasUnsavedChanges())
									states->activate(Asset::States::EDITABLE);
							}

							ImGui::EndTabItem();

							if (editingClosing()) {
								editingClosing(false);
								opened = false;
							}
						} else {
							ImGui::PopStyleColor();
						}
						if (!opened) {
							Operations::fileCloseAsset(rnd, this, project, index);

							assetsEditingIndex(-1);

							frontAsset = nullptr;
						}

						if (pending) {
							ImGui::PopStyleColor(3);
						}
					},
					true
				);

				if (switched) {
					int inspectible = 0;
					prj->foreach( // On the secondary orders.
						[&] (Asset* &asset, Asset::List::Index index) -> void {
							if (assetsEditingIndex() == index)
								return;

							Asset::States* states = asset->states();
							if (states->activity() != Asset::States::INSPECTABLE)
								return;

							if (++inspectible == 1) {
								if (frontAsset->states()->activity() != Asset::States::INSPECTABLE)
									return;
							}

							states->deactivate();
							states->deselect();

							asset->finish(Asset::EDITING, false);
						},
						true
					);
					prj->cleanup(Asset::EDITING);

					prj->bringToFront(frontAsset);
					const Asset::List::Index index = prj->indexOf(frontAsset, true);
					assetsEditingIndex(index);
				}
			} while (false);

			do {
				if (!executing())
					break;
				if (*canvasState() != FRAME)
					break;

				ImGuiTabItemFlags tabItemFlags = ImGuiTabItemFlags_NoTooltip;
				if (!canvasInitialized()) {
					canvasInitialized(true);
					tabItemFlags |= ImGuiTabItemFlags_SetSelected;
				}

				bool opened = true;
				if (ImGui::BeginTabItem(theme()->tabCanvas(), &opened, tabItemFlags)) {
					scene(wnd, rnd, project, exec, primitives, delta, indicated);

					ImGui::EndTabItem();

					if (editingClosing()) {
						editingClosing(false);
						opened = false;
					}
				} else {
					canvasHovering(false);
				}
				if (!opened) {
					Operations::projectStop(rnd, this, project, exec, primitives);
				}
			} while (false);

			do {
				if (!document())
					break;

				ImGuiTabItemFlags tabItemFlags = ImGuiTabItemFlags_NoTooltip;
				if (!documentInitialized()) {
					documentInitialized(true);
					tabItemFlags |= ImGuiTabItemFlags_SetSelected;
				}

				bool opened = true;
				if (ImGui::BeginTabItem(documentTitle(), &opened, tabItemFlags)) {
					document(wnd, rnd);

					ImGui::EndTabItem();

					if (editingClosing()) {
						editingClosing(false);
						opened = false;
					}
				}
				if (!opened) {
					documentInitialized(false);

					Document::destroy(document());
					document(nullptr);
				}
			} while (false);

			ImGui::EndTabBar();
		}

		ImGui::End();
	}
}

bool Workspace::canvas(class Window* wnd, class Renderer* rnd, const class Project* project, Executable* exec, class Primitives* primitives, double delta, bool* indicated) {
	canvasFocused(false);

	if (!executing())
		return false;

	if (paused() && *canvasState() == MAXIMIZED)
		return true;

	if (*canvasState() == FRAME && !canvasFull())
		return true;

	ImGuiIO &io = ImGui::GetIO();
	ImGuiStyle &style = ImGui::GetStyle();

	VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2());
	VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2());

	ImGuiWindowFlags flags = ImGuiWindowFlags_None;
	if (*canvasState() == POPUP && !canvasFull()) {
		const bool num1 = ImGui::IsKeyPressed(SDL_SCANCODE_1);
		const bool num2 = ImGui::IsKeyPressed(SDL_SCANCODE_2);
		const bool num3 = ImGui::IsKeyPressed(SDL_SCANCODE_3);
		const bool num4 = ImGui::IsKeyPressed(SDL_SCANCODE_4);
#if WORKSPACE_MODIFIER_KEY == WORKSPACE_MODIFIER_KEY_CTRL
		const bool modifier = io.KeyCtrl;
#elif WORKSPACE_MODIFIER_KEY == WORKSPACE_MODIFIER_KEY_CMD
		const bool modifier = io.KeySuper;
#endif /* WORKSPACE_MODIFIER_KEY */

		flags = WORKSPACE_WND_FLAGS_FLOAT;
		if (canvasHovering())
			flags |= ImGuiWindowFlags_NoMove;

		Math::Vec2i cvsSize;
		do {
			LockGuard<decltype(canvasSizeLock())> guard(canvasSizeLock());

			cvsSize = canvasSize_();
		} while (false);
		float canvasRatio = BITTY_CANVAS_DEFAULT_WIDTH / BITTY_CANVAS_DEFAULT_HEIGHT;
		if (cvsSize.x > 0 && cvsSize.y > 0) {
			canvasRatio = (float)cvsSize.x / cvsSize.y;
		}
		float times = 1;
		ImGuiCond cond = ImGuiCond_Once;
		if (num1 && modifier) {
			times = 1;
			cond = ImGuiCond_Always;
		} else if (num2 && modifier) {
			times = 2;
			cond = ImGuiCond_Always;
		} else if (num3 && modifier) {
			times = 3;
			cond = ImGuiCond_Always;
		} else if (num4 && modifier) {
			times = 4;
			cond = ImGuiCond_Always;
		} else if (cvsSize.y > 0) {
			times = std::max((float)std::floor(rnd->height() / cvsSize.y) - 1, 1.0f);
		} else {
			times = std::max((float)std::floor(rnd->height() / BITTY_CANVAS_DEFAULT_HEIGHT) - 1, 1.0f);
		}
		const bool fixedSize = cvsSize.x > 0 && cvsSize.y > 0;
		const bool justStopped = canvasValidation() == Math::Vec2i(-1, -1);
		const bool diffSize = canvasValidation() != cvsSize;
		const bool scaledSize = (num1 || num2 || num3 || num4) && modifier;
		const bool tobeValidated = fixedSize && ((!justStopped && diffSize) || scaledSize);
		if (tobeValidated) {
			cond = ImGuiCond_Always;

			canvasValidation(cvsSize);
		} else if (diffSize) {
			canvasValidation(Math::Vec2i(std::abs(cvsSize.x), std::abs(cvsSize.y)));
		}
		ImVec2 wndSize;
		ImVec2 wndMinSize;
		if (cvsSize.x > 0) {
			wndSize.x = cvsSize.x * times + style.WindowBorderSize * 4 + 1;
			wndMinSize = ImVec2(
				cvsSize.x * 0.5f,
				cvsSize.x * 0.5f / canvasRatio + ImGui::TitleBarHeight()
			);
		} else {
			wndSize.x = BITTY_CANVAS_DEFAULT_WIDTH * times + style.WindowBorderSize * 4 + 1;
			wndMinSize = ImVec2(
				BITTY_CANVAS_DEFAULT_WIDTH * 0.5f,
				BITTY_CANVAS_DEFAULT_WIDTH * 0.5f / canvasRatio + ImGui::TitleBarHeight()
			);
		}
		if (cvsSize.y > 0) {
			wndSize.y = cvsSize.y * times + style.WindowBorderSize * 4 + ImGui::TitleBarHeight();
		} else {
			wndSize.y = BITTY_CANVAS_DEFAULT_HEIGHT * times + style.WindowBorderSize * 4 + ImGui::TitleBarHeight();
		}
		ImGui::SetNextWindowPos(ImVec2((rnd->width() - wndSize.x) * 0.5f, (rnd->height() - wndSize.y) * 0.5f), cond);
		ImGui::SetNextWindowSize(wndSize, cond);
		ImGui::SetNextWindowSizeConstraints(wndMinSize, ImVec2((float)rnd->width(), (float)rnd->height()));
	} else if (*canvasState() == MAXIMIZED || canvasFull()) {
		flags = WORKSPACE_WND_FLAGS_DOCK_NO_TITLE;
		flags |= ImGuiWindowFlags_NoScrollWithMouse;

		canvasValidation(Math::Vec2i(rnd->width(), rnd->height()));

		const ImVec2 wndSize((float)rnd->width(), (float)rnd->height());
		ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
		ImGui::SetNextWindowSize(wndSize, ImGuiCond_Always);
	} else {
		assert(false && "Impossible.");
	}

	bool opened = true;
	if (ImGui::Begin(theme()->windowCanvas(), &opened, flags)) {
		if (*canvasState() == POPUP && !canvasFull()) {
			ImVec2 customBtnPos = ImGui::CustomButtonAutoPosition();
			if (ImGui::TitleBarCustomButton("#Max", &customBtnPos, ImGui::CustomMaxButton))
				*canvasState() = MAXIMIZED;
			if (ImGui::TitleBarCustomButton("#Min", &customBtnPos, ImGui::CustomMinButton))
				*canvasState() = FRAME;
		}

		scene(wnd, rnd, project, exec, primitives, delta, indicated);

		canvasFocused(ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows));

		ImGui::EnsureWindowVisible();

		ImGui::End();
	}
	if (!opened) {
		Operations::projectStop(rnd, this, project, exec, primitives);
	}

	return true;
}

void Workspace::debug(class Window* /* wnd */, class Renderer* rnd, const class Project* project, Executable* exec, class Primitives* primitives, unsigned fps) {
	debugShown(false);

	if (!*debugVisible() || !executing())
		return;

	if (immersive())
		return;

	debugShown(true);

	ImGuiIO &io = ImGui::GetIO();
	ImGuiStyle &style = ImGui::GetStyle();

	VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2());

	const float minWidth = std::min(rnd->width() * 0.257f, 256.0f * io.FontGlobalScale);
	ImGuiWindowFlags flags = WORKSPACE_WND_FLAGS_DOCK;
	if (debugWidth() <= 0.0f) {
		debugWidth(minWidth);
	}

	const float consoleH = *consoleVisible() ? consoleHeight() : 0.0f;

	const float gripMarginX = ImGui::WindowResizingPadding().x;
	const float gripPaddingY = 16.0f;
	if (debugResizing() && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
		debugResizing(false);
	}
	if (
		ImGui::IsMouseHoveringRect(
			ImVec2(bodyArea().xMax() - debugWidth(), bodyArea().yMin() + gripPaddingY),
			ImVec2(bodyArea().xMax() - debugWidth() + gripMarginX, bodyArea().yMax() - consoleH - gripPaddingY),
			false
		) &&
		!popupBox() && !headVisible() && !canvasHovering()
	) {
		debugResizing(true);

		ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
	} else if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
		debugResizing(false);
	}
	if (debugResizing()) {
		flags &= ~ImGuiWindowFlags_NoResize;
	}

	ImGui::SetNextWindowPos(ImVec2(bodyArea().xMax() - debugWidth(), bodyArea().yMin()), ImGuiCond_Always);
	ImGui::SetNextWindowSize(
		ImVec2(debugWidth(), bodyArea().height() - consoleH),
		ImGuiCond_Always
	);
	ImGui::SetNextWindowSizeConstraints(ImVec2(minWidth, -1), ImVec2(bodyArea().width() * 0.7f, -1));
	if (ImGui::Begin(theme()->windowDebug(), debugVisible(), flags)) {
		const ImVec2 buttonSize(13 * io.FontGlobalScale, 13 * io.FontGlobalScale);
		if (paused()) {
			if (ImGui::ImageButton(theme()->slicePlay()->pointer(rnd), buttonSize, ImGui::ColorConvertU32ToFloat4(theme()->style()->iconColor))) {
				Operations::debugContinue(this, project, exec);
			}
			if (ImGui::IsItemHovered()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				ImGui::SetTooltip(theme()->tooltipDebug_Continue());
			}
			ImGui::SameLine();
		} else {
			if (ImGui::ImageButton(theme()->slicePause()->pointer(rnd), buttonSize, ImGui::ColorConvertU32ToFloat4(theme()->style()->iconColor))) {
				Operations::debugBreak(this, project, exec);
			}
			if (ImGui::IsItemHovered()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				ImGui::SetTooltip(theme()->tooltipDebug_Break());
			}
			ImGui::SameLine();
		}
		if (paused()) {
			if (ImGui::ImageButton(theme()->sliceStep()->pointer(rnd), buttonSize, ImGui::ColorConvertU32ToFloat4(theme()->style()->iconColor)) || ImGui::IsKeyReleased(SDL_SCANCODE_F10)) {
				Operations::debugStepOver(this, project, exec);
			}
			if (ImGui::IsItemHovered()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				ImGui::SetTooltip(theme()->tooltipDebug_Step());
			}
			ImGui::SameLine(0, 0);
			if (ImGui::ImageButton(theme()->sliceStep_Into()->pointer(rnd), buttonSize, ImGui::ColorConvertU32ToFloat4(theme()->style()->iconColor)) || ImGui::IsKeyReleased(SDL_SCANCODE_F11)) {
				Operations::debugStepInto(this, project, exec);
			}
			if (ImGui::IsItemHovered()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				ImGui::SetTooltip(theme()->tooltipDebug_StepInto());
			}
			ImGui::SameLine(0, 0);
			if (ImGui::ImageButton(theme()->sliceStep_Out()->pointer(rnd), buttonSize, ImGui::ColorConvertU32ToFloat4(theme()->style()->iconColor)) || (ImGui::IsKeyReleased(SDL_SCANCODE_F11) && io.KeyShift)) {
				Operations::debugStepOut(this, project, exec);
			}
			if (ImGui::IsItemHovered()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				ImGui::SetTooltip(theme()->tooltipDebug_StepOut());
			}
			ImGui::SameLine();
		} else {
			const ImVec4 col = ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, col);

			ImGui::ImageButton(theme()->sliceStep()->pointer(rnd), buttonSize, ImGui::ColorConvertU32ToFloat4(theme()->style()->iconDisabledColor));
			if (ImGui::IsItemHovered()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				ImGui::SetTooltip(theme()->tooltipDebug_Step());
			}
			ImGui::SameLine(0, 0);
			ImGui::ImageButton(theme()->sliceStep_Into()->pointer(rnd), buttonSize, ImGui::ColorConvertU32ToFloat4(theme()->style()->iconDisabledColor));
			if (ImGui::IsItemHovered()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				ImGui::SetTooltip(theme()->tooltipDebug_StepInto());
			}
			ImGui::SameLine(0, 0);
			ImGui::ImageButton(theme()->sliceStep_Out()->pointer(rnd), buttonSize, ImGui::ColorConvertU32ToFloat4(theme()->style()->iconDisabledColor));
			if (ImGui::IsItemHovered()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				ImGui::SetTooltip(theme()->tooltipDebug_StepOut());
			}
			ImGui::SameLine();

			ImGui::PopStyleColor();
		}
		if (ImGui::ImageButton(theme()->sliceBreakpoints_Disable()->pointer(rnd), buttonSize, ImGui::ColorConvertU32ToFloat4(theme()->style()->iconColor))) {
			Operations::debugDisableBreakpoints(this, project, exec, nullptr);
		}
		if (ImGui::IsItemHovered()) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			ImGui::SetTooltip(theme()->tooltipDebug_Disable());
		}
		ImGui::SameLine(0, 0);
		if (ImGui::ImageButton(theme()->sliceBreakpoints_Enable()->pointer(rnd), buttonSize, ImGui::ColorConvertU32ToFloat4(theme()->style()->iconColor))) {
			Operations::debugEnableBreakpoints(this, project, exec, nullptr);
		}
		if (ImGui::IsItemHovered()) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			ImGui::SetTooltip(theme()->tooltipDebug_Enable());
		}
		ImGui::SameLine(0, 0);
		if (ImGui::ImageButton(theme()->sliceBreakpoints_Clear()->pointer(rnd), buttonSize, ImGui::ColorConvertU32ToFloat4(theme()->style()->iconColor))) {
			Operations::debugClearBreakpoints(this, project, exec, nullptr);
		}
		if (ImGui::IsItemHovered()) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding_(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			ImGui::SetTooltip(theme()->tooltipDebug_Clear());
		}

		ImGui::BeginChild("@Dbg/Dtl", ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_NoNav);
		{
			if (paused()) {
				Operations::debugSetProgramPointer(this, project, exec);

				typedef std::tuple<std::string, std::string, Variant> Variable;
				typedef std::list<Variable> Variables;

				int i = 0;
				Variables vars;
				Variables upvs;
				Executable::RecordGetter record = [&] (const char* src, int ln, int /* lnDef */, const char* name, const char* /* what */, Executable::VariableGetter getVars) -> void {
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					if (i == debugActiveFrameIndex())
						ImGui::TextUnformatted(">");
					bool sel = false;
					ImGui::PushID(i);
					{
						ImGui::TableSetColumnIndex(1);
						std::string srcLn = Text::toString(ln);
						srcLn += ", ";
						srcLn += src;
						sel = ImGui::Selectable(srcLn);
					}
					ImGui::PopID();
					ImGui::TableSetColumnIndex(2);
					ImGui::TextUnformatted(name);

					if (sel && debugActiveFrameIndex() != i) {
						debugActiveFrameIndex(i);

						focus(src, ln); // 1-based.

						vars.clear();
						upvs.clear();
					}

					bool nextVar = i == debugActiveFrameIndex();
					while (nextVar) {
						const char* varName = nullptr;
						const char* varType = nullptr;
						const Variant* varData = nullptr;
						bool isUpvalue = false;
						nextVar = getVars(varName, varType, varData, isUpvalue);
						if (nextVar && varName && varType && varData) {
							if (isUpvalue)
								upvs.push_back(std::make_tuple(varName, varType, *varData));
							else
								vars.push_back(std::make_tuple(varName, varType, *varData));
						}
					}

					++i;
				};

				ImGui::TextUnformatted(theme()->windowDebug_CallStack());
				const ImGuiTableFlags rcdFlags = ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit;
				if (ImGui::BeginTable("@Records", 3, rcdFlags)) {
					const float width = ImGui::GetFontSize() * 1.6f;
					ImGui::TableSetupColumn(" ", ImGuiTableColumnFlags_WidthFixed, width);
					ImGui::TableSetupColumn(theme()->windowDebug_Source(), ImGuiTableColumnFlags_WidthStretch, (debugWidth() - width) * 0.8f);
					ImGui::TableSetupColumn(theme()->windowDebug_Name(), ImGuiTableColumnFlags_WidthStretch, (debugWidth() - width) * 0.2f);
					ImGui::TableHeadersRow();
					exec->getRecords(record);
					ImGui::EndTable();
				}

				if (!vars.empty()) {
					ImGui::TextUnformatted(theme()->windowDebug_Local());
					const ImGuiTableFlags lclFlags = ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit;
					if (ImGui::BeginTable("@Locals", 3, lclFlags)) {
						const float width = ImGui::GetFontSize() * 5.0f;
						ImGui::TableSetupColumn(theme()->windowDebug_VariableName(), ImGuiTableColumnFlags_WidthFixed, width);
						ImGui::TableSetupColumn(theme()->windowDebug_VariableType(), ImGuiTableColumnFlags_WidthFixed, width);
						ImGui::TableSetupColumn(theme()->windowDebug_VariableValue(), ImGuiTableColumnFlags_WidthStretch);
						ImGui::TableHeadersRow();
						int j = 0;
						for (const Variable &var : vars) {
							const std::string &id = std::get<0>(var);
							const std::string &type = std::get<1>(var);
							const Variant &val = std::get<2>(var);
							ImGui::TableNextRow();
							ImGui::PushID(j);
							{
								ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted(id);
								ImGui::TableSetColumnIndex(1); ImGui::TextUnformatted(type);
								ImGui::TableSetColumnIndex(2); ImGui::DebugVariable(val);
							}
							ImGui::PopID();
							++j;
						}

						ImGui::EndTable();
					}
				}
				if (!upvs.empty()) {
					ImGui::TextUnformatted(theme()->windowDebug_Upvalue());
					const ImGuiTableFlags upFlags = ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit;
					if (ImGui::BeginTable("@Upvalues", 3, upFlags)) {
						const float width = ImGui::GetFontSize() * 5.0f;
						ImGui::TableSetupColumn(theme()->windowDebug_VariableName(), ImGuiTableColumnFlags_WidthFixed, width);
						ImGui::TableSetupColumn(theme()->windowDebug_VariableType(), ImGuiTableColumnFlags_WidthFixed, width);
						ImGui::TableSetupColumn(theme()->windowDebug_VariableValue(), ImGuiTableColumnFlags_WidthStretch);
						ImGui::TableHeadersRow();
						int j = 0;
						for (const Variable &var : upvs) {
							const std::string &id = std::get<0>(var);
							const std::string &type = std::get<1>(var);
							const Variant &val = std::get<2>(var);
							ImGui::TableNextRow();
							ImGui::PushID(j);
							{
								ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted(id);
								ImGui::TableSetColumnIndex(1); ImGui::TextUnformatted(type);
								ImGui::TableSetColumnIndex(2); ImGui::DebugVariable(val);
							}
							ImGui::PopID();
							++j;
						}

						ImGui::EndTable();
					}
				}
			} else {
				ImGui::TextUnformatted(theme()->windowDebug_Running());
			}

			ImGui::TextUnformatted("Stat:");
			ImGui::Text("   CPU FPS: %u", exec->fps());
			ImGui::Text("   GPU FPS: %u", fps);
			ImGui::Text("  COMMANDS: %u", primitives->commands());

			debugWidth(ImGui::GetWindowSize().x);
		}
		ImGui::EndChild();

		ImGui::End();
	}
}

void Workspace::console(class Window* /* wnd */, class Renderer* rnd, const class Project* project) {
	consoleFocused(false);

	if (!*consoleVisible())
		return;

	if (immersive())
		return;

	ImGuiIO &io = ImGui::GetIO();
	ImGuiStyle &style = ImGui::GetStyle();

	VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2());

	const float minHeight = std::min(bodyArea().height() * 0.3f, 256.0f * io.FontGlobalScale);
	ImGuiWindowFlags flags = WORKSPACE_WND_FLAGS_DOCK;
	if (consoleHeight() <= 0.0f) {
		consoleHeight(minHeight);
	}
	const float consoleY = rnd->height() - consoleHeight();

	const float gripPaddingX = 16.0f;
	const float gripMarginY = ImGui::WindowResizingPadding().y;
	if (consoleResizing() && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
		consoleResizing(false);

		withEditingAsset(
			project,
			[&] (Asset*, Editable* editor) -> void {
				editor->resized(rnd, project);
			}
		);
	}
	if (
		ImGui::IsMouseHoveringRect(
			ImVec2(
				bodyArea().xMin() + gripPaddingX,
				consoleY
			),
			ImVec2(
				bodyArea().xMax() - gripPaddingX * 2.0f,
				consoleY + gripMarginY
			),
			false
		) &&
		!popupBox() && !headVisible() && !canvasHovering()
	) {
		consoleResizing(true);

		ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
	} else if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
		consoleResizing(false);
	}
	if (consoleResizing()) {
		flags &= ~ImGuiWindowFlags_NoResize;
	}

	ImGui::SetNextWindowPos(ImVec2(bodyArea().xMin(), consoleY), ImGuiCond_Always);
	ImGui::SetNextWindowSize(
		ImVec2(bodyArea().width(), consoleHeight()),
		ImGuiCond_Always
	);
	ImGui::SetNextWindowSizeConstraints(ImVec2(-1, minHeight), ImVec2(-1, bodyArea().height() * 0.7f));
	if (ImGui::Begin(theme()->windowConsole(), consoleVisible(), flags)) {
		const bool clr = ImGui::TitleBarCustomButton("#Clr", nullptr, ImGui::CustomClearButton, theme()->generic_Clear().c_str());

		const ImVec2 content = ImGui::GetContentRegionAvail();

		do {
			LockGuard<decltype(consoleLock())> guard(consoleLock());

			if (clr)
				consoleTextBox()->SetText("");

			consoleTextBox()->Render("@Cnsl", content);
		} while (false);

		consoleHeight(ImGui::GetWindowSize().y);

		consoleFocused(ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows));

		ImGui::End();
	}
}

void Workspace::promise(class Window*, class Renderer*, const class Project*) {
	LockGuard<decltype(popupPromiseLock())> guard(popupPromiseLock());

	popupPromiseInit().update();
	if (!popupPromiseInit().end()) // To avoid instantly closing on single thread.
		return;

	switch (popupPromiseType()) {
	case FUNCTION: {
			Variant ret = nullptr;
			const bool resolved = popupPromiseHandler()(&ret);

			if (resolved)
				popupPromise()->resolve(ret);
			else
				popupPromise()->reject(nullptr);

			popupPromise(nullptr);

			popupPromiseType(NONE);
			popupPromiseHandler(nullptr);
			popupPromiseContent().clear();
			popupPromiseDefault().clear();
			popupPromiseConfirmText().clear();
			popupPromiseDenyText().clear();
			popupPromiseCancelText().clear();
		}

		break;
	case WAIT: {
			ImGui::WaitingPopupBox::TimeoutHandler timeout = nullptr;

			timeout = ImGui::WaitingPopupBox::TimeoutHandler(
				[&] (void) -> void {
					ImGui::PopupBox::Ptr popup = popupBox();

					popupPromise()->resolve(true);

					popupPromise(nullptr);

					if (popup == popupBox())
						popupBox(nullptr);
				},
				nullptr
			);

			waitingPopupBox(
				popupPromiseContent(),
				timeout
			);

			popupPromiseType(NONE);
			popupPromiseHandler(nullptr);
			popupPromiseContent().clear();
			popupPromiseDefault().clear();
			popupPromiseConfirmText().clear();
			popupPromiseDenyText().clear();
			popupPromiseCancelText().clear();
		}

		break;
	case MSGBOX: {
			bool withConfirm = !popupPromiseConfirmText().empty() && popupPromiseConfirmText() != EXECUTABLE_ANY_NAME;
			bool withDeny = !popupPromiseDenyText().empty() && popupPromiseDenyText() != EXECUTABLE_ANY_NAME;
			bool withCancel = !popupPromiseCancelText().empty() && popupPromiseCancelText() != EXECUTABLE_ANY_NAME;

			ImGui::MessagePopupBox::ConfirmHandler confirm = nullptr;
			ImGui::MessagePopupBox::DenyHandler deny = nullptr;
			ImGui::MessagePopupBox::CancelHandler cancel = nullptr;

			confirm = ImGui::MessagePopupBox::ConfirmHandler(
				[&] (void) -> void {
					ImGui::PopupBox::Ptr popup = popupBox();

					popupPromise()->resolve(true);

					popupPromise(nullptr);

					if (popup == popupBox())
						popupBox(nullptr);
				},
				nullptr
			);
			if (!popupPromiseDenyText().empty()) {
				deny = ImGui::MessagePopupBox::DenyHandler(
					[&] (void) -> void {
						ImGui::PopupBox::Ptr popup = popupBox();

						popupPromise()->resolve(false);

						popupPromise(nullptr);

						if (popup == popupBox())
							popupBox(nullptr);
					},
					nullptr
				);
			}
			if (!popupPromiseCancelText().empty()) {
				cancel = ImGui::MessagePopupBox::CancelHandler(
					[&] (void) -> void {
						ImGui::PopupBox::Ptr popup = popupBox();

						popupPromise()->reject(nullptr);

						popupPromise(nullptr);

						if (popup == popupBox())
							popupBox(nullptr);
					},
					nullptr
				);
			}

			messagePopupBox(
				popupPromiseContent(),
				confirm, deny, cancel,
				withConfirm ? popupPromiseConfirmText().c_str() : nullptr,
				withDeny ? popupPromiseDenyText().c_str() : nullptr,
				withCancel ? popupPromiseCancelText().c_str() : nullptr
			);

			popupPromiseType(NONE);
			popupPromiseHandler(nullptr);
			popupPromiseContent().clear();
			popupPromiseDefault().clear();
			popupPromiseConfirmText().clear();
			popupPromiseDenyText().clear();
			popupPromiseCancelText().clear();
		}

		break;
	case INPUT: {
			ImGui::InputPopupBox::ConfirmHandler confirm = nullptr;
			ImGui::InputPopupBox::CancelHandler cancel = nullptr;

			confirm = ImGui::InputPopupBox::ConfirmHandler(
				[&] (const char* input) -> void {
					ImGui::PopupBox::Ptr popup = popupBox();

					popupPromise()->resolve(input);

					popupPromise(nullptr);

					if (popup == popupBox())
						popupBox(nullptr);
				},
				nullptr
			);
			cancel = ImGui::InputPopupBox::CancelHandler(
				[&] (void) -> void {
					ImGui::PopupBox::Ptr popup = popupBox();

					popupPromise()->reject(nullptr);

					popupPromise(nullptr);

					if (popup == popupBox())
						popupBox(nullptr);
				},
				nullptr
			);

			inputPopupBox(
				popupPromiseContent(),
				popupPromiseDefault().c_str(), ImGuiInputTextFlags_None,
				confirm,
				cancel
			);

			popupPromiseType(NONE);
			popupPromiseHandler(nullptr);
			popupPromiseContent().clear();
			popupPromiseDefault().clear();
			popupPromiseConfirmText().clear();
			popupPromiseDenyText().clear();
			popupPromiseCancelText().clear();
		}

		break;
	default: // Do nothing.
		break;
	}
}

void Workspace::plugins(class Window*, class Renderer*, const class Project*, double delta) {
	if (!pluginsEnabled())
		return;

	for (Plugin* plugin : plugins()) {
		plugin->update(delta);
	}
}

void Workspace::finish(class Window* /* wnd */, class Renderer* /* rnd */, const class Project*) {
	if (init().begin()) {
		ImGuiWindow* wnd = ImGui::FindWindowByName("@Ed");
		if (wnd)
			ImGui::FocusWindow(wnd);
	}

	init().update();
}

void Workspace::waitingPopupBox(
	const std::string &content,
	const ImGui::WaitingPopupBox::TimeoutHandler &timeout_
) {
	ImGui::WaitingPopupBox::TimeoutHandler timeout = timeout_;

	if (timeout.empty()) {
		timeout = ImGui::WaitingPopupBox::TimeoutHandler([&] (void) -> void { popupBox(nullptr); }, nullptr);
	}

	popupBox(
		ImGui::PopupBox::Ptr(
			new ImGui::WaitingPopupBox(
				content,
				timeout
			)
		)
	);
}

void Workspace::messagePopupBox(
	const std::string &content,
	const ImGui::MessagePopupBox::ConfirmHandler &confirm_,
	const ImGui::MessagePopupBox::DenyHandler &deny,
	const ImGui::MessagePopupBox::CancelHandler &cancel,
	const char* confirmTxt,
	const char* denyTxt,
	const char* cancelTxt
) {
	const char* btnConfirm = confirmTxt;
	const char* btnDeny = denyTxt;
	const char* btnCancel = cancelTxt;

	ImGui::MessagePopupBox::ConfirmHandler confirm = confirm_;

	if (confirm_.empty() && deny.empty() && cancel.empty()) {
		if (!btnConfirm)
			btnConfirm = theme()->generic_Ok().c_str();
	} else if (!confirm_.empty() && deny.empty() && cancel.empty()) {
		if (!btnConfirm)
			btnConfirm = theme()->generic_Ok().c_str();
	} else if (!confirm_.empty() && !deny.empty() && cancel.empty()) {
		if (!btnConfirm)
			btnConfirm = theme()->generic_Yes().c_str();
		if (!btnDeny)
			btnDeny = theme()->generic_No().c_str();
	} else if (!confirm_.empty() && !deny.empty() && !cancel.empty()) {
		if (!btnConfirm)
			btnConfirm = theme()->generic_Yes().c_str();
		if (!btnDeny)
			btnDeny = theme()->generic_No().c_str();
		if (!btnCancel)
			btnCancel = theme()->generic_Cancel().c_str();
	}
	if (confirm_.empty()) {
		confirm = ImGui::MessagePopupBox::ConfirmHandler([&] (void) -> void { popupBox(nullptr); }, nullptr);
	}

	popupBox(
		ImGui::PopupBox::Ptr(
			new ImGui::MessagePopupBox(
				BITTY_NAME,
				content,
				confirm, deny, cancel,
				btnConfirm, btnDeny, btnCancel
			)
		)
	);
}

void Workspace::inputPopupBox(
	const std::string &content,
	const std::string &default_, unsigned flags,
	const ImGui::InputPopupBox::ConfirmHandler &confirm_,
	const ImGui::InputPopupBox::CancelHandler &cancel
) {
	const char* btnConfirm = nullptr;
	const char* btnCancel = nullptr;

	ImGui::InputPopupBox::ConfirmHandler confirm = confirm_;

	btnConfirm = theme()->generic_Ok().c_str();
	btnCancel = theme()->generic_Cancel().c_str();
	if (confirm_.empty()) {
		confirm = ImGui::InputPopupBox::ConfirmHandler([&] (const char*) -> void { popupBox(nullptr); }, nullptr);
	}

	popupBox(
		ImGui::PopupBox::Ptr(
			new ImGui::InputPopupBox(
				BITTY_NAME,
				content, default_, flags,
				confirm, cancel,
				btnConfirm, btnCancel
			)
		)
	);
}

void Workspace::scene(class Window* wnd, class Renderer* rnd, const class Project* /* project */, Executable* /* exec */, class Primitives* primitives, double delta, bool* indicated) {
	// Prepare.
	ImGuiStyle &style = ImGui::GetStyle();

	if (canvasFull())
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
	const float borderSize = style.WindowBorderSize;

	const ImVec2 regMin = ImGui::GetWindowContentRegionMin();
	const ImVec2 regMax = ImGui::GetWindowContentRegionMax();
	const ImVec2 regSize(
		regMax.x - regMin.x - borderSize * 2,
		regMax.y - regMin.y - borderSize * (*canvasState() == MAXIMIZED ? 2 : 1)
	);

	// Calculate a proper size and resize the canvas image.
	Math::Vec2i srcSize;
	do {
		LockGuard<decltype(canvasSizeLock())> guard(canvasSizeLock());

		if (canvasSize_().x <= 0 && canvasSize_().y <= 0) {
			srcSize = Math::Vec2i((Int)regSize.x, (Int)regSize.y);
			canvasSize_().x = -srcSize.x;
			canvasSize_().y = -srcSize.y;
		} else if (canvasSize_().x <= 0) {
			srcSize = Math::Vec2i(
				(Int)(canvasSize_().y * regSize.x / regSize.y),
				canvasSize_().y
			);
			canvasSize_().x = -srcSize.x;
		} else if (canvasSize_().y <= 0) {
			srcSize = Math::Vec2i(
				canvasSize_().x,
				(Int)(canvasSize_().x * regSize.y / regSize.x)
			);
			canvasSize_().y = -srcSize.y;
		} else {
			srcSize = Math::Vec2i(canvasSize_().x, canvasSize_().y);
		}
	} while (false);

	if (canvasTexture()->width() == 0 || canvasTexture()->height() == 0) {
		canvasTexture()->fromBytes(rnd, Texture::TARGET, nullptr, srcSize.x, srcSize.y, 0, canvasScaleMode());

		BITTY_RENDER_TARGET(rnd, canvasTexture().get())
		BITTY_RENDER_SCALE(rnd, 1)
		const Color col(30, 30, 30, 255);
		rnd->clear(&col);
	}
	if (canvasTexture()->width() != srcSize.x || canvasTexture()->height() != srcSize.y) {
		canvasTexture()->resize(rnd, srcSize.x, srcSize.y);
	}

	// Calculate the widget area.
	ImVec2 dstPos = ImGui::GetCursorPos();
	ImVec2 dstSize = regSize;
	if (*canvasState() == FRAME && !canvasFull())
		dstSize.y -= ImGui::TabBarHeight();
	bool horizontalPadded = false;
	bool verticalPadded = false;
	if (*canvasFixRatio()) {
		const float srcRatio = (float)srcSize.x / (float)srcSize.y;
		const float dstRatio = dstSize.x / dstSize.y;
		if (srcRatio < dstRatio) {
			const float w = dstSize.x;
			dstSize.x = dstSize.y * srcRatio;
			dstSize.x -= borderSize * 2;
			dstSize.y -= borderSize * 2;
			dstPos.x += (w - dstSize.x) * 0.5f;
			if (*canvasState() == MAXIMIZED)
				dstPos.y += borderSize;

			horizontalPadded = true;
		} else if (srcRatio > dstRatio) {
			const float h = dstSize.y;
			dstSize.y = dstSize.x / srcRatio;
			dstSize.x -= borderSize * 2;
			dstSize.y -= borderSize * 2;
			dstPos.x += borderSize;
			dstPos.y += (h - dstSize.y) * 0.5f;

			verticalPadded = true;
		} else {
			dstSize.x -= borderSize * 2;
			dstSize.y -= borderSize * 2;
			dstPos.x += borderSize;
		}
	} else {
		dstSize.x -= borderSize * 2;
		dstSize.y -= borderSize * 2;
		dstPos.x += borderSize;
		dstPos.y += borderSize;
	}

	// Commit to the canvas image.
	do {
		const ImVec2 wndPos = ImGui::GetWindowPos();
		const Math::Rectf clientArea = Math::Rectf::byXYWH(
			wndPos.x + dstPos.x, wndPos.y + dstPos.y,
			std::ceil(dstSize.x), std::ceil(dstSize.y)
		);
		const Math::Vec2i canvasSz = srcSize;

		const int scale = rnd->scale() / wnd->scale();
		BITTY_RENDER_TARGET(rnd, canvasTexture().get())
		BITTY_RENDER_SCALE(rnd, 1)
		primitives->update(&clientArea, &canvasSz, scale, delta, canvasHovering(), indicated);
	} while (false);

	// Render the border pads.
	if (canvasFull()) {
		const ImVec2 texSize((float)theme()->imagePadLandscapeLeft()->width(), (float)theme()->imagePadLandscapeLeft()->height());
		if (horizontalPadded && dstPos.x > texSize.x * 0.5f) {
			ImGui::SetCursorPos(regMin);
			ImGui::NineGridsImage(
				theme()->imagePadLandscapeLeft()->pointer(rnd),
				texSize,
				ImVec2(dstPos.x, regSize.y + 1),
				false, true
			);
			ImGui::SetCursorPos(regMin + ImVec2(dstPos.x + dstSize.x + 1, 0));
			ImGui::NineGridsImage(
				theme()->imagePadLandscapeRight()->pointer(rnd),
				texSize,
				ImVec2(regSize.x - (dstPos.x + dstSize.x) + 1, regSize.y + 1),
				false, false
			);
		}
		if (verticalPadded && dstPos.y > texSize.y * 0.5f) {
			ImGui::SetCursorPos(regMin);
			ImGui::NineGridsImage(
				theme()->imagePadPortraitTop()->pointer(rnd),
				texSize,
				ImVec2(regSize.x + 1, dstPos.y),
				true, false
			);
			ImGui::SetCursorPos(regMin + ImVec2(0, dstPos.y + dstSize.y + 1));
			ImGui::NineGridsImage(
				theme()->imagePadPortraitBottom()->pointer(rnd),
				texSize,
				ImVec2(regSize.x + 1, regSize.y - (dstPos.y + dstSize.y) + 1),
				true, true
			);
		}
	}

	// Render the canvas image.
	ImGui::SetCursorPos(dstPos);
	ImGui::Image(
		canvasTexture()->pointer(rnd),
		dstSize,
		ImVec2(0, 0), ImVec2(1, 1),
		ImVec4(1, 1, 1, 1), ImVec4(1, 1, 1, 0.5f)
	);

	if (*canvasState() == FRAME) {
		canvasHovering(ImGui::IsItemHovered());
	} else {
		const ImRect wndRect(
			ImGui::GetWindowPos() + ImGui::GetWindowContentRegionMin(),
			ImGui::GetWindowPos() + ImGui::GetWindowContentRegionMax()
		);
		const ImVec2 mousePos = ImGui::GetMousePos();
		canvasHovering(wndRect.Contains(mousePos));
	}

	// Render the onscreen gamepad.
	if (settings()->inputOnscreenGamepadEnabled) {
		const int pressed = primitives->input()->updateOnscreenGamepad(
			wnd, rnd,
			theme()->fontBlock(),
			settings()->inputOnscreenGamepadSwapAB,
			settings()->inputOnscreenGamepadScale,
			settings()->inputOnscreenGamepadPadding.x, settings()->inputOnscreenGamepadPadding.y
		);
		(void)pressed;
	}

	// Process frame recording.
	if (recorder()->recording()) {
		recorder()->update(wnd, rnd, canvasTexture().get());

		ImGui::Indicator("REC", dstPos + ImVec2(4, 4));
	}

	// Finish.
	if (canvasFull())
		ImGui::PopStyleVar();
}

void Workspace::document(class Window* wnd, class Renderer* rnd) {
	if (!document())
		return;

	ImGui::Dummy(ImVec2(8, 0));
	ImGui::SameLine();

	const ImVec2 size = ImGui::GetContentRegionAvail();
	ImGui::BeginChild("@Doc", size, false, ImGuiWindowFlags_NoNav);
	{
		document()->update(wnd, rnd, theme(), false);
	}
	ImGui::EndChild();
}

void Workspace::toggleManual(const char* path) {
	const char* shown = document() ? document()->shown() : nullptr;
	const bool close = !path || (shown ? strcmp(shown, path) == 0 : false);

	if (document() && close) {
		Document::destroy(document());
		document(nullptr);

		documentTitle().clear();

		documentInitialized(false);
	} else {
		if (document()) {
			Document::destroy(document());
			document(nullptr);

			documentInitialized(false);
		}

		if (!path)
			path = DOCUMENT_MARKDOWN_DIR "Manual." DOCUMENT_MARKDOWN_EXT;

		FileInfo::Ptr fileInfo = FileInfo::make(path);
		documentTitle("[" + fileInfo->fileName() + "]");

		document(Document::create());
		document()->show(path);
	}
}

void Workspace::toggleFullscreen(class Window* wnd) {
	settings()->applicationWindowMaximized = false;

	settings()->applicationWindowFullscreen = !settings()->applicationWindowFullscreen;
	wnd->fullscreen(settings()->applicationWindowFullscreen);
}

void Workspace::toggleMaximized(class Window* wnd) {
	if (settings()->applicationWindowFullscreen) {
		settings()->applicationWindowFullscreen = false;
		wnd->fullscreen(false);
	}

	settings()->applicationWindowMaximized = !settings()->applicationWindowMaximized;
	if (settings()->applicationWindowMaximized)
		wnd->maximize();
	else
		wnd->restore();
}

bool Workspace::immersive(void) const {
	if (canvasFull())
		return true;

	return currentState() == Executable::RUNNING && // Is running, and
		*canvasState() == MAXIMIZED;                // is maximized.
}

bool Workspace::executing(void) const {
	return currentState() == Executable::RUNNING || // Is running, or
		currentState() == Executable::PAUSED;       // is paused.
}

bool Workspace::paused(void) const {
	return currentState() == Executable::PAUSED;    // Is paused.
}

bool Workspace::halting(void) const {
	return currentState() == Executable::HALTING;   // Is halting.
}

void Workspace::projectStates(
	const Project* project,
	bool* dirty,
	bool* persisted,
	bool* archived,
	const char** url
) const {
	if (dirty)
		*dirty = false;
	if (persisted)
		*persisted = false;
	if (archived)
		*archived = false;
	if (url)
		*url = nullptr;

	LockGuard<RecursiveMutex>::UniquePtr acquired;
	Project* prj = project->acquire(acquired);
	if (!prj)
		return;

	if (dirty)
		*dirty = prj->dirty();
	if (persisted)
		*persisted = !prj->path().empty();
	if (archived)
		*archived = prj->archived();
	if (url && !prj->url().empty())
		*url = prj->url().c_str();
}

int Workspace::editingAssetStates(
	const Project* project,
	bool* any,
	unsigned* type,
	unsigned* referencing,
	bool* dirty,
	bool* pastable, bool* selectable,
	const char** undoable, const char** redoable
) const {
	if (any)
		*any = false;
	if (type)
		*type = 0;
	if (referencing)
		*referencing = 0;
	if (dirty)
		*dirty = false;
	if (pastable)
		*pastable = false;
	if (selectable)
		*selectable = false;
	if (undoable)
		*undoable = nullptr;
	if (redoable)
		*redoable = nullptr;

	return withEditingAsset(
		project,
		[&] (Asset* asset, Editable* editor) -> void {
			if (any)
				*any = true;

			if (type)
				*type = asset->type();

			if (referencing)
				*referencing = asset->referencing();

			if (dirty)
				*dirty = asset->dirty();

			if (pastable)
				*pastable = editor->pastable();
			if (selectable)
				*selectable = editor->selectable();

			if (undoable)
				*undoable = editor->undoable();
			if (redoable)
				*redoable = editor->redoable();
		}
	);
}

int Workspace::withEditingAsset(const class Project* project, EditorHandler handler) const {
	LockGuard<RecursiveMutex>::UniquePtr acquired;
	Project* prj = project->acquire(acquired);
	if (!prj)
		return 0;

	if (assetsEditingIndex() == -1)
		return 0;

	Asset* asset = prj->get(assetsEditingIndex());
	if (!asset)
		return 0;

	Editable* editor = asset->editor();
	if (!editor)
		return 0;

	if (handler)
		handler(asset, editor);

	return 1;
}

void Workspace::fillAssetEditorSettings(Editable* editor) const {
	editor->post(Editable::SET_THEME_STYLE, (Variant::Int)theme()->styleIndex());

	editor->post(Editable::SET_SHOW_SPACES, settings()->editorShowWhiteSpaces);
}

void Workspace::showAssetContextMenu(class Window*, class Renderer* rnd, const class Project* project, Executable* exec, class Primitives* primitives) {
	ImGuiIO &io = ImGui::GetIO();
	ImGuiStyle &style = ImGui::GetStyle();

	VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(8, 8));
	VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(8, 4));

	if (ImGui::BeginPopup("@Asts/Ctx")) {
		bool prjPersisted = false;
		projectStates(project, nullptr, &prjPersisted, nullptr, nullptr);

		if (ImGui::MenuItem(theme()->menuProject_NewAsset())) {
			Operations::projectAddAsset(rnd, this, project, assetsSelectedIndex());
		}
		if (assetsSelectedIndex() >= 0) {
			if (io.KeyShift) {
				if (ImGui::MenuItem(theme()->menuProject_RemoveAsset())) {
					Operations::projectRemoveAsset(rnd, this, project, exec, assetsSelectedIndex());
				}
			}
			if (ImGui::MenuItem(theme()->menuProject_RenameAsset())) {
				Operations::projectRenameAsset(rnd, this, project, assetsSelectedIndex());
			}
		}
		bool filtering = assetsFiltering();
		if (ImGui::MenuItem(theme()->menuProject_FilterAssets(), nullptr, &filtering)) {
			assetsFiltering(filtering);
			assetsFilteringInitialized(false);
		}
		ImGui::Separator();
		if (ImGui::MenuItem(theme()->menuProject_AddFile())) {
			Operations::projectAddFile(rnd, this, project, assetsSelectedIndex());
		}
#if !BITTY_TRIAL_ENABLED
		if (ImGui::MenuItem(theme()->menuProject_Import())) {
			Operations::projectImport(rnd, this, project);
		}
		if (ImGui::MenuItem(theme()->menuProject_Export())) {
			Operations::projectExport(rnd, this, project);
		}
#endif /* BITTY_TRIAL_ENABLED */
		if (prjPersisted) {
			ImGui::Separator();
			if (ImGui::MenuItem(theme()->menuProject_Reload())) {
				Operations::projectStop(rnd, this, project, exec, primitives);

				Operations::projectReload(rnd, this, project, exec);
			}
		}

		ImGui::EndPopup();
	}
}

void Workspace::filterAssets(class Window*, class Renderer* rnd, const class Project*, Executable*) {
	ImGuiIO &io = ImGui::GetIO();
	ImGuiStyle &style = ImGui::GetStyle();

	bool openMenu = false;

	auto filter = [&] (const char* what) -> void {
		assetsFilterInput(what);
		assetsFilterPatterns().clear();
		Text::Array patterns = Text::split(assetsFilterInput(), ",");
		for (std::string &pattern : patterns) {
			pattern = Text::trim(pattern);
			if (!pattern.empty()) {
				if (Text::indexOf(pattern, "*") == std::string::npos && Text::indexOf(pattern, "?") == std::string::npos)
					pattern = "*" + pattern + "*";
				assetsFilterPatterns().push_back(pattern);
			}
		}
	};

	if (assetsFiltering()) {
		const ImVec2 buttonSize(13 * io.FontGlobalScale, 13 * io.FontGlobalScale);

		ImGui::PushID("@Asts/Fltr");
		do {
			if (ImGui::IsWindowFocused() && ImGui::IsKeyPressed(SDL_SCANCODE_ESCAPE))
				assetsFiltering(false);

			VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(0, 0));
			VariableGuard<decltype(style.FramePadding)> guardFramePadding(&style.FramePadding, style.FramePadding, ImVec2(2, 2));

			if (!assetsFilteringInitialized()) {
				ImGui::SetKeyboardFocusHere();
				assetsFilteringInitialized(true);
			}
			ImGui::SetNextItemWidth(ImGui::GetWindowWidth() - (buttonSize.x + style.FramePadding.x * 2) * 2);
			char buf[256]; // Fixed size.
			const size_t n = std::min(BITTY_COUNTOF(buf) - 1, assetsFilterInput().length());
			if (n > 0)
				memcpy(buf, assetsFilterInput().c_str(), n);
			buf[n] = '\0';
			const bool changed = ImGui::InputText("", buf, sizeof(buf), ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_NoUndoRedo);
			if (changed) {
				filter(buf);
			}

			ImGui::SameLine();
			if (ImGui::ImageButton(theme()->sliceFilter()->pointer(rnd), buttonSize, ImGui::ColorConvertU32ToFloat4(theme()->style()->iconColor))) {
				openMenu = true;
			}
			ImGui::SameLine();
			if (ImGui::ImageButton(theme()->sliceRecycle()->pointer(rnd), buttonSize, ImGui::ColorConvertU32ToFloat4(theme()->style()->iconColor))) {
				assetsFilterInput().clear();
				assetsFilterPatterns().clear();
			}
		} while (false);
		ImGui::PopID();
	}

	if (openMenu)
		ImGui::OpenPopup("@Asts/Fltr/Opt");

	VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(8, 8));
	VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(8, 4));

	if (ImGui::BeginPopup("@Asts/Fltr/Opt")) {
		if (ImGui::MenuItem(theme()->menuAsset_Code()))
			filter("*." BITTY_LUA_EXT);
		if (ImGui::MenuItem(theme()->menuAsset_Sprites()))
			filter("*." BITTY_SPRITE_EXT);
		if (ImGui::MenuItem(theme()->menuAsset_Maps()))
			filter("*." BITTY_MAP_EXT);
		if (ImGui::MenuItem(theme()->menuAsset_Images()))
			filter("*." BITTY_IMAGE_EXT ",*.png,*.jpg,*.bmp,*.tga");
		if (ImGui::MenuItem(theme()->menuAsset_Palettes()))
			filter("*." BITTY_PALETTE_EXT);
		if (ImGui::MenuItem(theme()->menuAsset_Fonts()))
			filter("*." BITTY_FONT_EXT);
		if (ImGui::MenuItem(theme()->menuAsset_Audio()))
			filter("*.mp3,*.ogg,*.wav,*.mid,*.aiff,*.voc,*.mod,*.xm,*.s3m,*.669,*.it,*.med,*.opus,*.flac");
		if (ImGui::MenuItem(theme()->menuAsset_Json()))
			filter("*." BITTY_JSON_EXT);
		if (ImGui::MenuItem(theme()->menuAsset_Text()))
			filter("*." BITTY_TEXT_EXT);

		ImGui::EndPopup();
	}
}

void Workspace::resizeAsset(class Window* /* wnd */, class Renderer* rnd, const class Project* project, Asset::List::Index index) {
	LockGuard<RecursiveMutex>::UniquePtr acquired;
	Project* prj = project->acquire(acquired);
	if (!prj)
		return;

	Asset* asset = prj->get(index);
	if (!asset)
		return;

	const std::string entry = asset->entry().name();

	switch (asset->type()) {
	case Image::TYPE():
		Operations::editResizeImage(rnd, this, project, entry.c_str());

		break;
	case Map::TYPE():
		Operations::editResizeMap(rnd, this, project, entry.c_str());

		break;
	default: // Do nothing.
		break;
	}
}

void Workspace::resizeAssetGrid(class Window* /* wnd */, class Renderer* rnd, const class Project* project, Asset::List::Index index) {
	LockGuard<RecursiveMutex>::UniquePtr acquired;
	Project* prj = project->acquire(acquired);
	if (!prj)
		return;

	Asset* asset = prj->get(index);
	if (!asset)
		return;

	const std::string entry = asset->entry().name();

	switch (asset->type()) {
	case Image::TYPE():
		Operations::editResizeImageGrid(rnd, this, project, entry.c_str());

		break;
	default: // Do nothing.
		break;
	}
}

void Workspace::resizeAssetTile(class Window* /* wnd */, class Renderer* rnd, const class Project* project, Asset::List::Index index) {
	auto next = [rnd, this, project, index] (void) -> void {
		LockGuard<RecursiveMutex>::UniquePtr acquired;
		Project* prj = project->acquire(acquired);
		if (!prj)
			return;

		Asset* asset = prj->get(index);
		if (!asset)
			return;

		const std::string entry = asset->entry().name();

		const std::string msg = Text::cformat("Resize tile size of asset \"%s\".\n", entry.c_str());
		print(msg.c_str());

		Operations::editResizeTile(rnd, this, project, entry.c_str())
			.then(
				[this, project, entry] (bool, const Math::Vec2i* newSize) -> void {
					LockGuard<RecursiveMutex>::UniquePtr acquired;
					Project* prj = project->acquire(acquired);
					if (!prj)
						return;

					Asset* asset = prj->get(entry.c_str());
					if (!asset)
						return;

					Asset::States* states = asset->states();
					states->activate(Asset::States::EDITABLE);

					Editable* editor = asset->editor();
					if (editor)
						editor->post(Editable::RECALCULATE);

					const std::string newSizeStr = Text::toString(newSize->x) + "x" + Text::toString(newSize->y);
					const std::string msg = Text::cformat("Resized tile size to \"%s\" of asset \"%s\".\n", newSizeStr.c_str(), entry.c_str());
					print(msg.c_str());
				}
			)
			.fail(
				[this, entry] (void) -> void {
					const std::string msg = Text::cformat("Canceled to resize tile size of asset \"%s\".\n", entry.c_str());
					print(msg.c_str());
				}
			);
	};

	Operations::fileSaveAsset(rnd, this, project, index)
		.then(next);
}

void Workspace::rebindAssetRef(class Window* /* wnd */, class Renderer* rnd, const class Project* project, Asset::List::Index index) {
	auto next = [rnd, this, project, index] (void) -> void {
		LockGuard<RecursiveMutex>::UniquePtr acquired;
		Project* prj = project->acquire(acquired);
		if (!prj)
			return;

		Asset* asset = prj->get(index);
		if (!asset)
			return;

		const std::string entry = asset->entry().name();
		const std::string ref = asset->ref();

		const std::string msg = Text::cformat("Rebind ref: \"%s\" of asset \"%s\".\n", ref.c_str(), entry.c_str());
		print(msg.c_str());

		Asset::States* states = asset->states();
		states->deactivate();
		states->deselect();

		asset->finish(Asset::EDITING, false);

		Operations::editResolveRef(rnd, this, project, entry.c_str())
			.then(
				[this, project, entry, ref] (bool, const std::string &newRef) -> void {
					LockGuard<RecursiveMutex>::UniquePtr acquired;
					Project* prj = project->acquire(acquired);
					if (!prj)
						return;

					Asset* asset = prj->get(entry.c_str());
					if (!asset)
						return;

					Asset::States* states = asset->states();
					states->activate(Asset::States::EDITABLE);
					states->focus();

					const std::string msg = Text::cformat("Rebinded ref: \"%s\" to \"%s\" of asset \"%s\".\n", ref.c_str(), newRef.c_str(), entry.c_str());
					print(msg.c_str());
				}
			)
			.fail(
				[this, project, entry, ref] (void) -> void {
					LockGuard<RecursiveMutex>::UniquePtr acquired;
					Project* prj = project->acquire(acquired);
					if (!prj)
						return;

					Asset* asset = prj->get(entry.c_str());
					if (!asset)
						return;

					Asset::States* states = asset->states();
					states->activate(Asset::States::EDITABLE);
					states->focus();

					const std::string msg = Text::cformat("Canceled to rebind ref: \"%s\" of asset \"%s\".\n", ref.c_str(), entry.c_str());
					print(msg.c_str());
				}
			);
	};

	Operations::fileCloseAsset(rnd, this, project, index)
		.then(next);
}

void Workspace::resolveAssetRef(class Window* /* wnd */, class Renderer* rnd, const class Project* project, const char* asset_) {
	LockGuard<RecursiveMutex>::UniquePtr acquired;
	Project* prj = project->acquire(acquired);
	if (!prj)
		return;

	Asset* asset = prj->get(asset_);
	if (!asset)
		return;

	const std::string entry = asset->entry().name();
	const std::string ref = asset->ref();

	const std::string msg = Text::cformat("Missing ref: \"%s\" of asset \"%s\".\n", ref.c_str(), entry.c_str());
	error(msg.c_str());

	Asset::States* states = asset->states();
	states->deactivate();

	asset->finish(Asset::EDITING, false);

	Operations::editResolveRef(rnd, this, project, entry.c_str())
		.then(
			[this, project, entry, ref] (bool, const std::string &newRef) -> void {
				LockGuard<RecursiveMutex>::UniquePtr acquired;
				Project* prj = project->acquire(acquired);
				if (!prj)
					return;

				Asset* asset = prj->get(entry.c_str());
				if (!asset)
					return;

				Asset::States* states = asset->states();
				states->activate(Asset::States::EDITABLE);
				states->focus();

				const std::string msg = Text::cformat("Resolved missing ref: \"%s\" to \"%s\" of asset \"%s\".\n", ref.c_str(), newRef.c_str(), entry.c_str());
				print(msg.c_str());
			}
		)
		.fail(
			[this, entry, ref] (void) -> void {
				const std::string msg_ = theme()->dialogItem_UnsolveAssetRefFor() + " " + entry;
				messagePopupBox(
					msg_,
					nullptr,
					nullptr,
					nullptr
				);

				const std::string msg = Text::cformat("Failed or canceled to resolve missing ref: \"%s\" of asset \"%s\".\n", ref.c_str(), entry.c_str());
				warn(msg.c_str());
			}
		);
}

void Workspace::beginSplash(class Window* wnd, class Renderer* rnd, const class Project* project) {
#if BITTY_SPLASH_ENABLED
	if (Path::existsFile(WORKSPACE_SPLASH_FILE)) {
		splashCustomized(true);

		workspaceCreateSplash(wnd, rnd, this);
		workspaceRenderSplash(wnd, rnd, this, nullptr);
	} else {
		workspaceCreateSplash(wnd, rnd, this, 0);
		workspaceRenderSplash(wnd, rnd, this, nullptr);
	}

	workspaceWaitSplash(wnd, rnd, this, project);
#else /* BITTY_SPLASH_ENABLED */
	(void)wnd;
	(void)project;

	const Color color(0x00, 0x00, 0x00, 0x00);
	rnd->clear(&color);
#endif /* BITTY_SPLASH_ENABLED */
}

void Workspace::endSplash(class Window* wnd, class Renderer* rnd) {
#if BITTY_SPLASH_ENABLED
	if (splashCustomized()) {
		if (splashBitty()) {
			theme()->destroyTexture(rnd, splashBitty());
			splashBitty(nullptr);
		}

		if (splashEngine()) {
			theme()->destroyTexture(rnd, splashEngine());
			splashEngine(nullptr);
		}
	} else {
		constexpr const int INDICES[] = {
			1, 2, 3, 4, 5,
			0, 0, 6, 6, 6,
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0
		};

		Sfx::Ptr sfx(Sfx::create());
		sfx->fromBytes(RES_SOUND_SPLASH, BITTY_COUNTOF(RES_SOUND_SPLASH));
		sfx->play(false, nullptr, -1);

		for (int i = 0; i < BITTY_COUNTOF(INDICES); ++i) {
			const long long begin = DateTime::ticks();
			const long long end = begin + DateTime::fromSeconds(0.05);
			while (DateTime::ticks() < end) {
				constexpr const int STEP = 20;
				workspaceSleep(STEP);
				Platform::idle();
			}

			workspaceCreateSplash(wnd, rnd, this, INDICES[i]);
			workspaceRenderSplash(wnd, rnd, this, nullptr);
		}

		if (splashBitty()) {
			theme()->destroyTexture(rnd, splashBitty());
			splashBitty(nullptr);
		}

		if (splashEngine()) {
			theme()->destroyTexture(rnd, splashEngine());
			splashEngine(nullptr);
		}
	}
#else /* BITTY_SPLASH_ENABLED */
	(void)wnd;

	const Color color(0x00, 0x00, 0x00, 0x00);
	rnd->clear(&color);
#endif /* BITTY_SPLASH_ENABLED */
}

/* ===========================================================================} */
