/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2021 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __OPERATIONS_H__
#define __OPERATIONS_H__

#include "bitty.h"
#include "workspace.h"
#if defined BITTY_CP_VC
#	pragma warning(push)
#	pragma warning(disable : 4100)
#endif /* BITTY_CP_VC */
#include "../lib/promise_cpp/promise.hpp"
#if defined BITTY_CP_VC
#	pragma warning(pop)
#endif /* BITTY_CP_VC */

/*
** {===========================================================================
** Operations
*/

/**
 * @brief Asynchronized and synchronized operations for workspace.
 */
class Operations {
public:
	static promise::Defer popupMessage(class Renderer* rnd, Workspace* ws, const char* content, bool deniable = false, bool cancelable = false);
	static promise::Defer popupInput(class Renderer* rnd, Workspace* ws, const char* content, const char* default_ = nullptr, unsigned flags = 0);
	static promise::Defer popupWait(class Renderer* rnd, Workspace* ws, const char* content);

	static promise::Defer fileBackup(class Renderer* rnd, Workspace* ws, const Project* project);
	static promise::Defer fileRestore(class Renderer* rnd, Workspace* ws, const Project* project);
	static promise::Defer fileClean(class Renderer* rnd, Workspace* ws, const Project* project);
	static promise::Defer fileNew(class Renderer* rnd, Workspace* ws, const class Project* project, Executable* exec);
	static promise::Defer fileOpenFile(class Renderer* rnd, Workspace* ws, const class Project* project, Executable* exec, const char* path = nullptr);
	static promise::Defer fileOpenDirectory(class Renderer* rnd, Workspace* ws, const class Project* project, Executable* exec, const char* path = nullptr);
	static promise::Defer fileOpenExample(class Renderer* rnd, Workspace* ws, const class Project* project, Executable* exec, const char* path = nullptr);
	static promise::Defer fileCloseAsset(class Renderer* rnd, Workspace* ws, const class Project* project, Asset::List::Index index);
	static promise::Defer fileClose(class Renderer* rnd, Workspace* ws, const class Project* project, Executable* exec);
	static promise::Defer fileAskSave(class Renderer* rnd, Workspace* ws, const class Project* project, Executable* exec);
	static promise::Defer fileSaveAsset(class Renderer* rnd, Workspace* ws, const class Project* project, Asset::List::Index index);
	static promise::Defer fileSaveFile(class Renderer* rnd, Workspace* ws, const class Project* project, bool saveAs);
	static promise::Defer fileSaveDirectory(class Renderer* rnd, Workspace* ws, const class Project* project, bool saveAs);

	static promise::Defer editResizeImage(class Renderer* rnd, Workspace* ws, const class Project* project, const char* asset);
	static promise::Defer editResizeMap(class Renderer* rnd, Workspace* ws, const class Project* project, const char* asset);
	static promise::Defer editResizeTile(class Renderer* rnd, Workspace* ws, const class Project* project, const char* asset);
	static promise::Defer editResolveRef(class Renderer* rnd, Workspace* ws, const class Project* project, const char* asset);
	static promise::Defer editResolveRefs(class Renderer* rnd, Workspace* ws, const class Project* project, const char* asset);
	static promise::Defer editSwitchAsset(class Renderer* rnd, Workspace* ws, const class Project* project);

	static promise::Defer projectAddAsset(class Renderer* rnd, Workspace* ws, const class Project* project, Asset::List::Index index);
	static promise::Defer projectRemoveAsset(class Renderer* rnd, Workspace* ws, const class Project* project, Executable* exec, Asset::List::Index index);
	static promise::Defer projectRenameAsset(class Renderer* rnd, Workspace* ws, const class Project* project, Asset::List::Index index);
	static promise::Defer projectAddFile(class Renderer* rnd, Workspace* ws, const class Project* project, Asset::List::Index index);
	static promise::Defer projectImport(class Renderer* rnd, Workspace* ws, const class Project* project, const char* path = nullptr, bool excludeInfoAndMain = false);
	static promise::Defer projectExport(class Renderer* rnd, Workspace* ws, const class Project* project);
	static promise::Defer projectBrowse(class Renderer* rnd, Workspace* ws, const class Project* project);

	static unsigned projectGetCustomAssetType(class Renderer* rnd, Workspace* ws, const class Project* project, const std::string &ext, unsigned type);

	static void projectRun(class Renderer* rnd, Workspace* ws, const class Project* project, Executable* exec, class Primitives* primitives);
	static void projectStop(class Renderer* rnd, Workspace* ws, const class Project* project, Executable* exec, class Primitives* primitives);

	static void debugBreak(Workspace* ws, const class Project* project, Executable* exec);
	static void debugContinue(Workspace* ws, const class Project* project, Executable* exec);
	static void debugStepOver(Workspace* ws, const class Project* project, Executable* exec);
	static void debugStepInto(Workspace* ws, const class Project* project, Executable* exec);
	static void debugStepOut(Workspace* ws, const class Project* project, Executable* exec);
	static bool debugSetProgramPointer(Workspace* ws, const class Project* project, Executable* exec);
	static void debugToggleBreakpoint(Workspace* ws, const class Project* project, Executable* exec, const char* src = nullptr, int ln = -1);
	static void debugEnableBreakpoints(Workspace* ws, const class Project* project, Executable* exec, const char* src /* nullable */);
	static void debugDisableBreakpoints(Workspace* ws, const class Project* project, Executable* exec, const char* src /* nullable */);
	static void debugClearBreakpoints(Workspace* ws, const class Project* project, Executable* exec, const char* src /* nullable */);

	static promise::Defer pluginRunMenuItem(class Renderer* rnd, Workspace* ws, const class Project* project, Plugin* plugin);
};

/* ===========================================================================} */

#endif /* __OPERATIONS_H__ */
