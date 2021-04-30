/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2021 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __WORKSPACE_H__
#define __WORKSPACE_H__

#include "bitty.h"
#include "asset.h"
#include "dispatchable.h"
#include "widgets.h"

/*
** {===========================================================================
** Macros and constants
*/

#ifndef WORKSPACE_OPTION_APPLICATION_DEFAULT_KEY
#	define WORKSPACE_OPTION_APPLICATION_DEFAULT_KEY ""
#endif /* WORKSPACE_OPTION_APPLICATION_DEFAULT_KEY */
#ifndef WORKSPACE_OPTION_APPLICATION_CWD_KEY
#	define WORKSPACE_OPTION_APPLICATION_CWD_KEY "w"
#endif /* WORKSPACE_OPTION_APPLICATION_CWD_KEY */
#ifndef WORKSPACE_OPTION_WINDOW_BORDERLESS_KEY
#	define WORKSPACE_OPTION_WINDOW_BORDERLESS_KEY "b"
#endif /* WORKSPACE_OPTION_WINDOW_BORDERLESS_KEY */
#ifndef WORKSPACE_OPTION_WINDOW_SIZE_KEY
#	define WORKSPACE_OPTION_WINDOW_SIZE_KEY "s"
#endif /* WORKSPACE_OPTION_WINDOW_SIZE_KEY */
#ifndef WORKSPACE_OPTION_WINDOW_HIGH_DPI_DISABLED_KEY
#	define WORKSPACE_OPTION_WINDOW_HIGH_DPI_DISABLED_KEY "d"
#endif /* WORKSPACE_OPTION_WINDOW_HIGH_DPI_DISABLED_KEY */
#ifndef WORKSPACE_OPTION_RENDERER_X2_KEY
#	define WORKSPACE_OPTION_RENDERER_X2_KEY "x2"
#endif /* WORKSPACE_OPTION_RENDERER_X2_KEY */
#ifndef WORKSPACE_OPTION_RENDERER_X3_KEY
#	define WORKSPACE_OPTION_RENDERER_X3_KEY "x3"
#endif /* WORKSPACE_OPTION_RENDERER_X3_KEY */
#ifndef WORKSPACE_OPTION_PLUGIN_DISABLED_KEY
#	define WORKSPACE_OPTION_PLUGIN_DISABLED_KEY "p"
#endif /* WORKSPACE_OPTION_PLUGIN_DISABLED_KEY */
#ifndef WORKSPACE_OPTION_EXECUTABLE_TIMEOUT_DISABLED_KEY
#	define WORKSPACE_OPTION_EXECUTABLE_TIMEOUT_DISABLED_KEY "t"
#endif /* WORKSPACE_OPTION_EXECUTABLE_TIMEOUT_DISABLED_KEY */

#ifndef WORKSPACE_AUTORUN_PROJECT_DIR
#	define WORKSPACE_AUTORUN_PROJECT_DIR "../" /* Relative path. */
#endif /* WORKSPACE_AUTORUN_PROJECT_DIR */
#ifndef WORKSPACE_AUTORUN_PROJECT_NAME
#	define WORKSPACE_AUTORUN_PROJECT_NAME "data"
#endif /* WORKSPACE_AUTORUN_PROJECT_NAME */

#ifndef WORKSPACE_CONFIG_NAME
#	define WORKSPACE_CONFIG_NAME "config"
#endif /* WORKSPACE_CONFIG_NAME */

#ifndef WORKSPACE_EXAMPLE_PROJECT_DIR
#	define WORKSPACE_EXAMPLE_PROJECT_DIR "../examples/" /* Relative path. */
#endif /* WORKSPACE_EXAMPLE_PROJECT_DIR */

#ifndef WORKSPACE_MODIFIER_KEY_CTRL
#	define WORKSPACE_MODIFIER_KEY_CTRL 0
#endif /* WORKSPACE_MODIFIER_KEY_CTRL */
#ifndef WORKSPACE_MODIFIER_KEY_CMD
#	define WORKSPACE_MODIFIER_KEY_CMD 1
#endif /* WORKSPACE_MODIFIER_KEY_CMD */
#ifndef WORKSPACE_MODIFIER_KEY
#	if defined BITTY_OS_APPLE
#		define WORKSPACE_MODIFIER_KEY WORKSPACE_MODIFIER_KEY_CMD
#	else /* BITTY_OS_APPLE */
#		define WORKSPACE_MODIFIER_KEY WORKSPACE_MODIFIER_KEY_CTRL
#	endif /* BITTY_OS_APPLE */
#endif /* WORKSPACE_MODIFIER_KEY */
#ifndef WORKSPACE_MODIFIER_KEY_NAME
#	if WORKSPACE_MODIFIER_KEY == WORKSPACE_MODIFIER_KEY_CTRL
#		define WORKSPACE_MODIFIER_KEY_NAME "Ctrl"
#	elif WORKSPACE_MODIFIER_KEY == WORKSPACE_MODIFIER_KEY_CMD
#		define WORKSPACE_MODIFIER_KEY_NAME "Cmd"
#	endif /* WORKSPACE_MODIFIER_KEY */
#endif /* WORKSPACE_MODIFIER_KEY_NAME */

constexpr const ImGuiWindowFlags WORKSPACE_WND_FLAGS_DOCK =
	ImGuiWindowFlags_NoResize |
	ImGuiWindowFlags_NoMove |
	ImGuiWindowFlags_NoScrollbar |
	ImGuiWindowFlags_NoCollapse |
	ImGuiWindowFlags_NoSavedSettings |
	ImGuiWindowFlags_NoBringToFrontOnFocus;
constexpr const ImGuiWindowFlags WORKSPACE_WND_FLAGS_DOCK_NO_TITLE =
	WORKSPACE_WND_FLAGS_DOCK |
	ImGuiWindowFlags_NoTitleBar;
constexpr const ImGuiWindowFlags WORKSPACE_WND_FLAGS_FLOAT =
	ImGuiWindowFlags_NoScrollbar |
	ImGuiWindowFlags_NoCollapse |
	ImGuiWindowFlags_NoSavedSettings;

/* ===========================================================================} */

/*
** {===========================================================================
** Forward declaration
*/

namespace ImGui {

class CodeEditor;

}

/* ===========================================================================} */

/*
** {===========================================================================
** Workspace
*/

/**
 * @brief Workspace entity.
 */
class Workspace : public Executable::Observer, public Dispatchable {
public:
	struct Settings {
		int applicationWindowDisplayIndex = 0;
		bool applicationWindowFullscreen = false;
		bool applicationWindowMaximized = false;
		Math::Vec2i applicationWindowSize;
		bool applicationPauseOnFocusLost = true;

		unsigned projectPreference = 0;
		bool projectIgnoreDotFiles = true;

		bool bannerVisible = true;

		bool assetsVisible = true;

		bool editorShowWhiteSpaces = true;
		bool editorCaseSensitive = false;
		bool editorMatchWholeWord = false;

		unsigned canvasState = POPUP;
		bool canvasFixRatio = true;

		bool debugVisible = true;

		bool consoleVisible = true;
		bool consoleClearOnStart = true;

		Input::Gamepad inputGamepads[INPUT_GAMEPAD_COUNT];
		bool inputOnscreenGamepadEnabled = true;
		bool inputOnscreenGamepadSwapAB = false;
		float inputOnscreenGamepadScale = 1.0f;
		Math::Vec2<float> inputOnscreenGamepadPadding = Math::Vec2<float>(8.0f, 12.0f);

		Settings();
	};

protected:
	typedef Math::Rect<float, 0> Rect;

	enum PopupPromiseTypes {
		NONE,
		FUNCTION,
		WAIT,
		MSGBOX,
		INPUT
	};

	enum CanvasStates {
		POPUP,
		FRAME,
		MAXIMIZED
	};

	typedef std::function<void(Asset*, class Editable*)> EditorHandler;

private:
	struct SourcePosition {
	private:
		std::string _source;
		int _line = -1;

		Mutex _lock;

	public:
		SourcePosition();

		void set(const std::string &src, int ln);
		bool getAndClear(std::string &src, int &ln);
	};

	friend class Operations;

protected:
	BITTY_PROPERTY(ImGui::Initializer, init)

	BITTY_PROPERTY_READONLY(bool, busy)

	BITTY_PROPERTY_READONLY(unsigned, activeFrameRate)

	BITTY_PROPERTY_READONLY(Executable::States, currentState)

	BITTY_PROPERTY_READONLY_PTR(class Recorder, recorder)

	BITTY_PROPERTY(Entry::Dictionary, examples)

	BITTY_PROPERTY_READONLY(bool, pluginsEnabled)
	BITTY_PROPERTY(Plugin::Array, plugins)
	BITTY_PROPERTY(int, pluginsMenuProjectItemCount)
	BITTY_PROPERTY(int, pluginsMenuPluginsItemCount)
	BITTY_PROPERTY(int, pluginsMenuHelpItemCount)

	BITTY_PROPERTY(Entry::Dictionary, documents)

	BITTY_PROPERTY(bool, splashCustomized)
	BITTY_PROPERTY_PTR(Texture, splashBitty)
	BITTY_PROPERTY_PTR(Texture, splashEngine)

	BITTY_PROPERTY(bool, effectCustomized)
	BITTY_PROPERTY(std::string, effectConfig)

	BITTY_PROPERTY_READONLY(ImGui::PopupBox::Ptr, popupBox)
	BITTY_PROPERTY(ImGui::Initializer, popupPromiseInit)
	BITTY_PROPERTY(PopupPromiseTypes, popupPromiseType)
	BITTY_PROPERTY(Promise::Ptr, popupPromise)
	BITTY_PROPERTY(Executable::PromiseHandler, popupPromiseHandler)
	BITTY_PROPERTY(std::string, popupPromiseContent)
	BITTY_PROPERTY(std::string, popupPromiseDefault)
	BITTY_PROPERTY(std::string, popupPromiseConfirmText)
	BITTY_PROPERTY(std::string, popupPromiseDenyText)
	BITTY_PROPERTY(std::string, popupPromiseCancelText)
	BITTY_FIELD(Mutex, popupPromiseLock)

	BITTY_PROPERTY_READONLY(float, menuHeight)
	BITTY_PROPERTY_READONLY(float, bannerHeight)
	BITTY_PROPERTY_READONLY_PTR(bool, bannerVisible)
	BITTY_PROPERTY_READONLY(bool, headVisible)

	BITTY_PROPERTY_READONLY(float, assetsWidth)
	BITTY_PROPERTY_READONLY_PTR(bool, assetsVisible)
	BITTY_PROPERTY_READONLY(bool, assetsResizing)
	BITTY_PROPERTY_READONLY(bool, assetsFocused)
	BITTY_PROPERTY_READONLY(Asset::List::Index, assetsSelectedIndex)
	BITTY_PROPERTY_READONLY(Asset::List::Index, assetsEditingIndex)
	BITTY_PROPERTY_READONLY(bool, assetsFiltering)
	BITTY_PROPERTY_READONLY(bool, assetsFilteringInitialized)
	BITTY_PROPERTY(std::string, assetsFilterInput)
	BITTY_PROPERTY(Text::Array, assetsFilterPatterns)

	BITTY_PROPERTY_READONLY(Rect, bodyArea)

	BITTY_PROPERTY_READONLY(bool, editingClosing)

	BITTY_PROPERTY_READONLY_PTR(unsigned, canvasState)
	BITTY_PROPERTY_READONLY_PTR(bool, canvasFixRatio)
	BITTY_PROPERTY(Math::Vec2i, canvasValidation)
	BITTY_PROPERTY(Math::Vec2i, canvasSize)
	BITTY_FIELD(Mutex, canvasSizeLock)
	BITTY_PROPERTY_READONLY(Texture::Ptr, canvasTexture)
	BITTY_PROPERTY_READONLY(bool, canvasHovering)
	BITTY_PROPERTY_READONLY(bool, canvasFull)
	BITTY_PROPERTY_READONLY(bool, canvasInitialized)
	BITTY_PROPERTY_READONLY(bool, canvasFocused)

	BITTY_PROPERTY_PTR(class Document, document)
	BITTY_PROPERTY(std::string, documentTitle)
	BITTY_PROPERTY_READONLY(bool, documentInitialized)

	BITTY_PROPERTY_READONLY(float, debugWidth)
	BITTY_PROPERTY_READONLY_PTR(bool, debugVisible)
	BITTY_PROPERTY_READONLY(bool, debugShown)
	BITTY_PROPERTY_READONLY(bool, debugResizing)
	BITTY_PROPERTY_READONLY(int, debugActiveFrameIndex)
	BITTY_FIELD(SourcePosition, debugProgramPointer)
	BITTY_FIELD(Atomic<bool>, debugStopping)

	BITTY_PROPERTY_READONLY(float, consoleHeight)
	BITTY_PROPERTY_READONLY_PTR(bool, consoleVisible)
	BITTY_PROPERTY_READONLY(bool, consoleResizing)
	BITTY_PROPERTY_READONLY(bool, consoleFocused)
	BITTY_PROPERTY_READONLY_PTR(ImGui::CodeEditor, consoleTextBox)
	BITTY_FIELD(Mutex, consoleLock)

public:
	Workspace();
	virtual ~Workspace();

	/**
	 * @brief Opens the workspace for further operation.
	 */
	virtual bool open(class Window* wnd, class Renderer* rnd, const class Project* project, Executable* exec, class Primitives* primitives, const Text::Dictionary &options);
	/**
	 * @brief Closes the workspace after all operations.
	 */
	virtual bool close(class Window* wnd, class Renderer* rnd, const class Project* project, Executable* exec);

	/**
	 * @brief Gets the settings pointer for read.
	 */
	virtual const Settings* settings(void) const = 0;
	/**
	 * @brief Gets the settings pointer for read and write.
	 */
	virtual Settings* settings(void) = 0;
	/**
	 * @brief Gets the theme pointer.
	 */
	virtual class Theme* theme(void) const = 0;

	/**
	 * @brief Gets whether shortcuts are allowed.
	 */
	bool canUseShortcuts(void) const;
	/**
	 * @brief Gets whether it's possible to save to a specific path.
	 */
	bool canSaveTo(const char* path) const;

	/**
	 * @brief Loads workspace data.
	 */
	virtual bool load(class Window* wnd, class Renderer* rnd, const class Project* project, class Primitives* primitives) = 0;
	/**
	 * @brief Saves workspace data.
	 */
	virtual bool save(class Window* wnd, class Renderer* rnd, const class Project* project, class Primitives* primitives) = 0;

	/**
	 * @brief Updates the workspace for one frame.
	 */
	virtual unsigned update(class Window* wnd, class Renderer* rnd, const class Project* project, Executable* exec, class Primitives* primitives, double delta, unsigned fps, bool alive, bool* indicated) = 0;

	/**
	 * @brief Implements `Executable::Observer`. Clears output in the console window.
	 */
	virtual void clear(void) override;
	/**
	 * @brief Implements `Executable::Observer`. Outputs a specific message to the console window.
	 */
	virtual bool print(const char* msg) override;
	/**
	 * @brief Implements `Executable::Observer`. Outputs a specific warning to the console window.
	 */
	virtual bool warn(const char* msg) override;
	/**
	 * @brief Implements `Executable::Observer`. Outputs a specific error to the console window.
	 */
	virtual bool error(const char* msg) override;
	/**
	 * @brief Implements `Executable::Observer`. Gets whether there is pending promise.
	 */
	virtual bool promising(void) override;
	/**
	 * @brief Implements `Executable::Observer`. Promises for custom handler.
	 */
	virtual void promise(Promise::Ptr &promise, Executable::PromiseHandler handler) override;
	/**
	 * @brief Implements `Executable::Observer`. Promises for wait box.
	 */
	virtual void waitbox(Promise::Ptr &promise, const char* content) override;
	/**
	 * @brief Implements `Executable::Observer`. Promises for message box.
	 */
	virtual void msgbox(Promise::Ptr &promise, const char* msg, const char* confirmTxt, const char* denyTxt, const char* cancelTxt) override;
	/**
	 * @brief Implements `Executable::Observer`. Promises for input box.
	 */
	virtual void input(Promise::Ptr &promise, const char* prompt, const char* default_) override;
	/**
	 * @brief Implements `Executable::Observer`. Sets focus to a specific source file and line.
	 */
	virtual bool focus(const char* src, int ln) override;
	/**
	 * @brief Implements `Executable::Observer`. Requires libraries.
	 */
	virtual void require(Executable* exec) override;
	/**
	 * @brief Implements `Executable::Observer`. Stops execution.
	 */
	virtual void stop(void) override;
	/**
	 * @brief Implements `Executable::Observer`. Gets the size of the rendering canvas.
	 */
	virtual Math::Vec2i size(void) override;
	/**
	 * @brief Implements `Executable::Observer`. Sets the size of the rendering canvas.
	 */
	virtual bool resize(const Math::Vec2i &size) override;
	/**
	 * @brief Implements `Executable::Observer`. Sets fullscreen effect.
	 */
	virtual void effect(const char* material) override;

	/**
	 * @brief Callback for focus gained.
	 */
	virtual void focusGained(class Window* wnd, class Renderer* rnd, const class Project* project, Executable* exec, class Primitives* primitives);
	/**
	 * @brief Callback for focus lost.
	 */
	virtual void focusLost(class Window* wnd, class Renderer* rnd, const class Project* project, Executable* exec, class Primitives* primitives);
	/**
	 * @brief Callback for render targets reset.
	 */
	virtual void renderTargetsReset(class Window* wnd, class Renderer* rnd, const class Project* project, Executable* exec, class Primitives* primitives);
	/**
	 * @brief Callback when the application window resized.
	 */
	virtual void resized(class Window* wnd, class Renderer* rnd, const class Project* project, const Math::Vec2i &size);
	/**
	 * @brief Callback when the application window maximized.
	 */
	virtual void maximized(class Window* wnd, class Renderer* rnd);
	/**
	 * @brief Callback when the application window restored.
	 */
	virtual void restored(class Window* wnd, class Renderer* rnd);
	/**
	 * @brief Callback when the application is going to quit.
	 */
	virtual bool quit(class Window* wnd, class Renderer* rnd, const class Project* project, Executable* exec, class Primitives* primitives);

	/**
	 * @brief Implements `Dispatchable`.
	 */
	virtual Variant post(unsigned msg, int argc, const Variant* argv) override;
	/**
	 * @brief Implements `Dispatchable`.
	 */
	using Dispatchable::post;

protected:
	virtual bool load(class Window* wnd, class Renderer* rnd, const class Project* project, class Primitives* primitives, const rapidjson::Document &doc);
	/**
	 * @param[out] doc
	 */
	virtual bool save(class Window* wnd, class Renderer* rnd, const class Project* project, class Primitives* primitives, rapidjson::Document &doc);

	void execute(class Window* wnd, class Renderer* rnd, const class Project* project, Executable* exec, class Primitives* primitives, double delta, bool alive);
	void prepare(class Window* wnd, class Renderer* rnd, const class Project* project, Executable* exec, class Primitives* primitives);
	void dialog(class Window* wnd, class Renderer* rnd, const class Project* project);
	void banner(class Window* wnd, class Renderer* rnd, const class Project* project, Executable* exec, class Primitives* primitives);
	void assets(class Window* wnd, class Renderer* rnd, const class Project* project, Executable* exec, class Primitives* primitives);
	void editing(class Window* wnd, class Renderer* rnd, const class Project* project, Executable* exec, class Primitives* primitives, double delta, bool* indicated);
	bool canvas(class Window* wnd, class Renderer* rnd, const class Project* project, Executable* exec, class Primitives* primitives, double delta, bool* indicated);
	void debug(class Window* wnd, class Renderer* rnd, const class Project* project, Executable* exec, class Primitives* primitives, unsigned fps);
	void console(class Window* wnd, class Renderer* rnd, const class Project* project);
	void promise(class Window* wnd, class Renderer* rnd, const class Project* project);
	void plugins(class Window* wnd, class Renderer* rnd, const class Project* project, double delta);
	void finish(class Window* wnd, class Renderer* rnd, const class Project* project);

	void waitingPopupBox(
		const std::string &content,
		const ImGui::WaitingPopupBox::TimeoutHandler &timeout /* nullable */
	);
	void messagePopupBox(
		const std::string &content,
		const ImGui::MessagePopupBox::ConfirmHandler &confirm /* nullable */,
		const ImGui::MessagePopupBox::DenyHandler &deny /* nullable */,
		const ImGui::MessagePopupBox::CancelHandler &cancel /* nullable */,
		const char* confirmTxt = nullptr,
		const char* denyTxt = nullptr,
		const char* cancelTxt = nullptr
	);
	void inputPopupBox(
		const std::string &content,
		const std::string &default_, unsigned flags,
		const ImGui::InputPopupBox::ConfirmHandler &confirm /* nullable */,
		const ImGui::InputPopupBox::CancelHandler &cancel /* nullable */
	);
	void scene(class Window* wnd, class Renderer* rnd, const class Project* project, Executable* exec, class Primitives* primitives, double delta, bool* indicated);
	void document(class Window* wnd, class Renderer* rnd);
	void toggleManual(const char* path /* nullable */);
	void toggleFullscreen(class Window* wnd);
	void toggleMaximized(class Window* wnd);

	bool immersive(void) const;
	bool executing(void) const;
	bool paused(void) const;

	/**
	 * @param[out] dirty
	 * @param[out] persisted
	 * @param[out] archived
	 */
	void projectStates(
		const Project* project,
		bool* dirty /* nullable */,
		bool* persisted /* nullable */,
		bool* archived /* nullable */,
		const char** url /* nullable */
	) const;
	/**
	 * @param[out] any
	 * @param[out] type
	 * @param[out] referencing
	 * @param[out] dirty
	 * @param[out] pastable
	 * @param[out] undoable
	 * @param[out] redoable
	 */
	int editingAssetStates(
		const Project* project,
		bool* any /* nullable */,
		unsigned* type /* nullable */,
		unsigned* referencing /* nullable */,
		bool* dirty /* nullable */,
		bool* pastable /* nullable */,
		const char** undoable /* nullable */, const char** redoable /* nullable */
	) const;
	int withEditingAsset(const class Project* project, EditorHandler handler) const;
	void fillAssetEditorSettings(Editable* editor) const;
	void showAssetContextMenu(class Window* wnd, class Renderer* rnd, const class Project* project, Executable* exec, class Primitives* primitives);
	void filterAssets(class Window* wnd, class Renderer* rnd, const class Project* project, Executable* exec);
	void resizeAsset(class Window* wnd, class Renderer* rnd, const class Project* project, Asset::List::Index index);
	void resizeAssetGrid(class Window* wnd, class Renderer* rnd, const class Project* project, Asset::List::Index index);
	void resizeAssetTile(class Window* wnd, class Renderer* rnd, const class Project* project, Asset::List::Index index);
	void rebindAssetRef(class Window* wnd, class Renderer* rnd, const class Project* project, Asset::List::Index index);
	void resolveAssetRef(class Window* wnd, class Renderer* rnd, const class Project* project, const char* asset);

	void beginSplash(class Window* wnd, class Renderer* rnd, const class Project* project);
	void endSplash(class Window* wnd, class Renderer* rnd);

private:
	void loadProject(class Renderer* rnd, const class Project* project, Executable* exec);
	void unloadProject(const class Project* project, Executable* exec);

	void loadExamples(class Renderer* rnd, const class Project* project);
	void unloadExamples(void);

	void loadPlugins(class Renderer* rnd, const class Project* project);
	void unloadPlugins(void);

	void loadDocuments(void);
	void unloadDocuments(void);
};

/* ===========================================================================} */

#endif /* __WORKSPACE_H__ */
