/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2021 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "bitty.h"
#include "application.h"
#include "workspace_sketchbook.h"
#if defined BITTY_OS_HTML
#	include <emscripten.h>
#elif defined BITTY_OS_WIN
#	include <fcntl.h>
#	include <io.h>
#	include <Windows.h>
#elif defined BITTY_OS_MAC
// Do nothing.
#elif defined BITTY_OS_ANDROID
#	include "platform.h"
#	include <SDL.h>
#elif defined BITTY_OS_LINUX
// Do nothing.
#endif /* Platform macro. */

#if defined BITTY_OS_HTML
EM_JS(
	bool, fssynced, (), {
		return !!Module.syncdone;
	}
);
#endif /* BITTY_OS_HTML */

static int entry(int argc, const char* argv[]) {
#if defined BITTY_OS_HTML
	emscripten_set_main_loop([] (void) -> void { }, 0, 0);
	while (!fssynced()) {
		constexpr const int STEP = 10;
		emscripten_sleep(STEP);
	}
	Application* app = createApplication(new WorkspaceSketchbook(), argc, argv);
	emscripten_cancel_main_loop();
	emscripten_set_main_loop_arg(
		[] (void* arg) -> void {
			Application* app = (Application*)arg;
			updateApplication(app);
		},
		app,
		BITTY_ACTIVE_FRAME_RATE, 1
	);
	destroyApplication(app);
#else /* BITTY_OS_HTML */
	Application* app = createApplication(new WorkspaceSketchbook(), argc, argv);
	while (updateApplication(app)) { }
	destroyApplication(app);
#endif /* BITTY_OS_HTML */

	return 0;
}

#if defined BITTY_OS_HTML /* Platform macro. */
#if defined __EMSCRIPTEN_PTHREADS__
#	pragma message("Using Emscripten thread.")
#else /* __EMSCRIPTEN_PTHREADS__ */
#	pragma message("Not using Emscripten thread.")
#endif /* __EMSCRIPTEN_PTHREADS__ */

extern std::string platformBinPath;

extern void platformSetDocumentPathResolver(std::string(*)(void));

static std::string htmlDocumentPathResolve(void) {
	return "/Documents";
}

EM_JS(
	void, initHtml, (), {
		FS.mkdir('/Documents');
		FS.mount(MEMFS, { }, '/Documents');

		Module.syncdone = 0;
		FS.mkdir('/libsdl');
		FS.mkdir('/libsdl/bitty');
		FS.mkdir('/libsdl/bitty/engine');
		FS.mount(IDBFS, { }, '/libsdl/bitty/engine');

		FS.syncfs(
			true,
			function (err) {
				assert(!err);

				Module.print("Filesystem loaded.");
				Module.syncdone = 1;
			}
		);
	}
);

int main(int argc, const char* argv[]) {
	std::vector<const char*> args;
	initHtml();

	platformBinPath = "/html/bitty.js";

	platformSetDocumentPathResolver(htmlDocumentPathResolve);

	for (int i = 1; i < argc; ++i)
		args.push_back(argv[i]);

	if (args.empty())
		return entry(0, nullptr);
	else
		return entry((int)args.size(), &args.front());
}
#elif defined BITTY_OS_WIN /* Platform macro. */
#if defined BITTY_DEBUG
static void openTerminal(void) {
	long hConHandle;
	HANDLE lStdHandle;
	CONSOLE_SCREEN_BUFFER_INFO coninfo;
	FILE* fp = nullptr;

	::AllocConsole();

	::GetConsoleScreenBufferInfo(::GetStdHandle(STD_OUTPUT_HANDLE), &coninfo);
	coninfo.dwSize.Y = 500;
	::SetConsoleScreenBufferSize(::GetStdHandle(STD_OUTPUT_HANDLE), coninfo.dwSize);

	lStdHandle = ::GetStdHandle(STD_OUTPUT_HANDLE);
	hConHandle = _open_osfhandle((intptr_t)lStdHandle, _O_TEXT);
	fp = _fdopen((intptr_t)hConHandle, "w");
	*stdout = *fp;
	setvbuf(stdout, nullptr, _IONBF, 0);

	lStdHandle = ::GetStdHandle(STD_INPUT_HANDLE);
	hConHandle = _open_osfhandle((intptr_t)lStdHandle, _O_TEXT);
	fp = _fdopen((intptr_t)hConHandle, "r");
	*stdin = *fp;
	setvbuf(stdin, nullptr, _IONBF, 0);

	lStdHandle = ::GetStdHandle(STD_ERROR_HANDLE);
	hConHandle = _open_osfhandle((intptr_t)lStdHandle, _O_TEXT);
	fp = _fdopen((intptr_t)hConHandle, "w");
	*stderr = *fp;
	setvbuf(stderr, nullptr, _IONBF, 0);

	std::ios::sync_with_stdio();

	freopen("CON", "w", stdout);
	freopen("CON", "r", stdin);
	freopen("CON", "w", stderr);
}
#endif /* BITTY_DEBUG */

static std::vector<const char*> splitArgs(const char* ln, Text::Array &args) {
	std::vector<const char*> ret;
	args = Text::split(ln, " ");
	for (std::string &a : args) {
		a = Text::trim(a);
		ret.push_back(a.c_str());
	}

	return ret;
}

int CALLBACK WinMain(_In_ HINSTANCE /* hInstance */, _In_ HINSTANCE /* hPrevInstance */, _In_ LPSTR lpCmdLine, _In_ int /* nCmdShow */) {
#if defined BITTY_DEBUG
	_CrtSetBreakAlloc(0);
	atexit(
		[] (void) -> void {
			if (!!_CrtDumpMemoryLeaks()) {
				fprintf(stderr, "Memory leak!\n");

				_CrtDbgBreak();
			}
		}
	);

	openTerminal();
#endif /* BITTY_DEBUG */

	Text::Array argbuf;
	std::vector<const char*> args = splitArgs(lpCmdLine, argbuf);

	if (args.empty())
		return entry(0, nullptr);
	else
		return entry((int)args.size(), &args.front());
}
#elif defined BITTY_OS_MAC /* Platform macro. */
int main(int argc, const char* argv[]) {
	std::vector<const char*> args;

	for (int i = 1; i < argc; ++i)
		args.push_back(argv[i]);

	if (args.empty())
		return entry(0, nullptr);
	else
		return entry((int)args.size(), &args.front());
}
#elif defined BITTY_OS_ANDROID /* Platform macro. */
#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

FILE* fopen64(char const* fn, char const* m) {
	return fopen(fn, m);
}

long ftello64(FILE* fp) {
	return ftell(fp);
}

int fseeko64(FILE* fp, long off, int ori) {
	return fseek(fp, off, ori);
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

extern std::string platformBinPath;

extern void platformSetDocumentPathResolver(std::string(*)(void));

static std::string androidDocumentPathResolve(void) {
	std::string dir;
	const char* cstr = SDL_AndroidGetInternalStoragePath();
	if (!cstr) {
		int state = SDL_AndroidGetExternalStorageState();
		if (state & SDL_ANDROID_EXTERNAL_STORAGE_WRITE)
			cstr = SDL_AndroidGetExternalStoragePath();
	}
	if (!cstr) {
		cstr = SDL_GetPrefPath(BASIC8_DOMAIN, BASIC8_TITLE);
		dir = cstr;
		SDL_free((void*)cstr);
	}
	if (cstr)
		dir = cstr;

	return dir;
}

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef main
#	undef main
#endif /* main */

int main(int argc, const char* argv[]) {
	std::vector<const char*> args;

	platformBinPath = "/";

	platformSetDocumentPathResolver(androidDocumentPathResolve);

	if (argc >= 2)
		Platform::currentDirectory(argv[1]);

	for (int i = 2; i < argc; ++i)
		args.push_back(argv[i]);

	if (args.empty())
		return entry(0, nullptr);
	else
		return entry((int)args.size(), &args.front());
}

#ifdef __cplusplus
}
#endif /* __cplusplus */
#elif defined BITTY_OS_LINUX /* Platform macro. */
extern std::string platformBinPath;

int main(int argc, const char* argv[]) {
	char buf[BITTY_MAX_PATH + 1];
	buf[BITTY_MAX_PATH] = '\0';
	std::vector<const char*> args;

	realpath(argv[0], buf);
	if (argc >= 1)
		platformBinPath = buf;

	for (int i = 1; i < argc; ++i)
		args.push_back(argv[i]);

	if (args.empty())
		return entry(0, nullptr);
	else
		return entry((int)args.size(), &args.front());
}
#endif /* Platform macro. */
