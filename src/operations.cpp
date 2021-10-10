/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2021 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "archive.h"
#include "bytes.h"
#include "code.h"
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
#include "theme.h"
#include "../lib/jpath/jpath.hpp"
#if defined BITTY_CP_VC
#	pragma warning(push)
#	pragma warning(disable : 4800)
#	pragma warning(disable : 4819)
#endif /* BITTY_CP_VC */
#if defined BITTY_OS_WIN || defined BITTY_OS_MAC || defined BITTY_OS_LINUX
#	include "../lib/portable_file_dialogs/portable-file-dialogs.h"
#elif defined BITTY_OS_HTML
#	include "../lib/portable_file_dialogs_polyfill/portable-file-dialogs.h"
#else /* Platform macro. */
#	include "../lib/portable_file_dialogs_polyfill/portable-file-dialogs.h"
#endif /* Platform macro. */
#if defined BITTY_CP_VC
#	pragma warning(pop)
#endif /* BITTY_CP_VC */

/*
** {===========================================================================
** Macros and constants
*/

#ifndef OPERATIONS_BACKUP_DIR
#	define OPERATIONS_BACKUP_DIR "backup"
#endif /* OPERATIONS_BACKUP_DIR */
#ifndef OPERATIONS_BACKUP_NAME
#	define OPERATIONS_BACKUP_NAME "latest"
#endif /* OPERATIONS_BACKUP_NAME */
#ifndef OPERATIONS_EDITING_NAME
#	define OPERATIONS_EDITING_NAME "editing"
#endif /* OPERATIONS_EDITING_NAME */

#ifndef OPERATIONS_CODE_PLACEHOLDER
#	define OPERATIONS_CODE_PLACEHOLDER "{CODE}"
#endif /* OPERATIONS_CODE_PLACEHOLDER */
#ifndef OPERATIONS_COUNT_PLACEHOLDER
#	define OPERATIONS_COUNT_PLACEHOLDER "{COUNT}"
#endif /* OPERATIONS_COUNT_PLACEHOLDER */

#ifndef OPERATIONS_BITTY_FILE_FILTER
#	define OPERATIONS_BITTY_FILE_FILTER { \
			"Bitty project files (*." BITTY_PROJECT_EXT ")", "*." BITTY_PROJECT_EXT, \
			"All files (*.*)", "*" \
		}
#endif /* OPERATIONS_BITTY_FILE_FILTER */
#ifndef OPERATIONS_BITTY_FULL_FILE_FILTER
#	define OPERATIONS_BITTY_FULL_FILE_FILTER { \
			"Bitty project files (*." BITTY_PROJECT_EXT ", *." BITTY_TEXT_EXT ", *." BITTY_ZIP_EXT ")", "*." BITTY_PROJECT_EXT " *." BITTY_TEXT_EXT " *." BITTY_ZIP_EXT, \
			"All files (*.*)", "*" \
		}
#endif /* OPERATIONS_BITTY_FULL_FILE_FILTER */
#ifndef OPERATIONS_ASSET_FILE_FILTER
#	define OPERATIONS_ASSET_FILE_FILTER { \
			"All assets", \
				"*." OPERATIONS_CODE_PLACEHOLDER " " \
				"*." BITTY_PALETTE_EXT " " \
				"*." BITTY_IMAGE_EXT " *.png *.jpg *.bmp *.tga" " " \
				"*." BITTY_SPRITE_EXT " " \
				"*." BITTY_MAP_EXT " " \
				"*." BITTY_FONT_EXT " " \
				"*.mp3 *.ogg *.wav *.mid *.aiff *.voc *.mod *.opus *.flac" " " \
				"*." BITTY_TEXT_EXT " *." BITTY_JSON_EXT, \
			"Code files (*." OPERATIONS_CODE_PLACEHOLDER ")", "*." OPERATIONS_CODE_PLACEHOLDER, \
			"Palette files (*." BITTY_PALETTE_EXT ")", "*." BITTY_PALETTE_EXT, \
			"Image files (*." BITTY_IMAGE_EXT ", *.png, *.jpg, *.bmp, *.tga" ")", "*." BITTY_IMAGE_EXT " *.png *.jpg *.bmp *.tga", \
			"Sprite files (*." BITTY_SPRITE_EXT ")", "*." BITTY_SPRITE_EXT, \
			"Map files (*." BITTY_MAP_EXT ")", "*." BITTY_MAP_EXT, \
			"Font files (*." BITTY_FONT_EXT ")", "*." BITTY_FONT_EXT, \
			"Audio files (*.mp3, *.ogg, *.wav, etc.)", "*.mp3 *.ogg *.wav *.mid *.aiff *.voc *.mod *.opus *.flac", \
			"Data files (*." BITTY_TEXT_EXT ", *." BITTY_JSON_EXT ")", "*." BITTY_TEXT_EXT " *." BITTY_JSON_EXT, \
			"All files (*.*)", "*" \
		}
#endif /* OPERATIONS_ASSET_FILE_FILTER */

#ifndef OPERATIONS_ASSET_DEFAULT_NAME
#	define OPERATIONS_ASSET_DEFAULT_NAME "noname"
#endif /* OPERATIONS_ASSET_DEFAULT_NAME */

static constexpr const char OPERATIONS_ASSET_DEFAULT_CODE[] =
	"function setup()\n"
	"end\n"
	"\n"
	"function update(delta)\n"
	"end\n";

#ifndef OPERATIONS_AUTO_CLOSE_POPUP
#	define OPERATIONS_AUTO_CLOSE_POPUP(W) \
		ProcedureGuard<void> __CLOSE##__LINE__( \
			std::bind( \
				[] (Workspace* ws) -> void* { \
					ImGui::PopupBox::Ptr popup = ws->popupBox(); \
					return (void*)(uintptr_t)popup.get(); \
				}, \
				W \
			), \
			std::bind( \
				[] (Workspace* ws, void* ptr) -> void { \
					ImGui::PopupBox* popup = (ImGui::PopupBox*)(uintptr_t)ptr; \
					if (popup == ws->popupBox().get()) \
						ws->popupBox(nullptr); \
				}, \
				W, std::placeholders::_1 \
			) \
		);
#endif /* OPERATIONS_AUTO_CLOSE_POPUP */

/* ===========================================================================} */

/*
** {===========================================================================
** Utilities
*/

static void operationsHandleError(Workspace* ws, const char* msg) {
	ws->error(msg);
}

static void operationsAppendCustomAssetType(
	class Renderer*, Workspace* ws, const class Project*,
	ImGui::AddAssetPopupBox::Types &types, ImGui::AddAssetPopupBox::TypeNames &typeNames, ImGui::AddAssetPopupBox::TypeExtensions &typeExtensions,
	ImGui::AddAssetPopupBox::Vec2s &defaultSizes, ImGui::AddAssetPopupBox::Vec2s &maxSizes,
	ImGui::AddAssetPopupBox::Vec2s &defaultSizes2, ImGui::AddAssetPopupBox::Vec2s &maxSizes2
) {
	for (Plugin* plugin : ws->plugins()) {
		if (!plugin->is(Plugin::Usages::COMPILER))
			continue;

		const Plugin::Schema &schema = plugin->schema();
		if (std::find(types.begin(), types.end(), schema.type()) != types.end()) {
			fprintf(stderr, "Asset type already exists: \"%s\".\n", schema.name.c_str());

			continue;
		}

		types.push_back(schema.type());
		typeNames.push_back(schema.name);
		typeExtensions.push_back(schema.extension);
		defaultSizes.push_back(Math::Vec2i());
		maxSizes.push_back(Math::Vec2i());
		defaultSizes2.push_back(Math::Vec2i());
		maxSizes2.push_back(Math::Vec2i());
	}
}

/* ===========================================================================} */

/*
** {===========================================================================
** Operations
*/

promise::Defer Operations::popupMessage(class Renderer*, Workspace* ws, const char* content, bool deniable, bool cancelable) {
	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			ImGui::MessagePopupBox::ConfirmHandler confirm = nullptr;
			ImGui::MessagePopupBox::DenyHandler deny = nullptr;
			ImGui::MessagePopupBox::CancelHandler cancel = nullptr;

			confirm = ImGui::MessagePopupBox::ConfirmHandler(
				[ws, df] (void) -> void {
					OPERATIONS_AUTO_CLOSE_POPUP(ws)

					df.resolve(true);
				},
				nullptr
			);
			if (deniable) {
				deny = ImGui::MessagePopupBox::DenyHandler(
					[ws, df] (void) -> void {
						OPERATIONS_AUTO_CLOSE_POPUP(ws)

						df.resolve(false);
					},
					nullptr
				);
			}
			if (cancelable) {
				cancel = ImGui::MessagePopupBox::CancelHandler(
					[ws, df] (void) -> void {
						OPERATIONS_AUTO_CLOSE_POPUP(ws)

						df.reject();
					},
					nullptr
				);
			}
			ws->messagePopupBox(
				content,
				confirm,
				deny,
				cancel
			);
		}
	);
}

promise::Defer Operations::popupInput(class Renderer*, Workspace* ws, const char* content, const char* default_, unsigned flags) {
	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			ImGui::InputPopupBox::ConfirmHandler confirm(
				[ws, df] (const char* name) -> void {
					OPERATIONS_AUTO_CLOSE_POPUP(ws)

					df.resolve(name);
				},
				nullptr
			);
			ImGui::InputPopupBox::CancelHandler cancel(
				[ws, df] (void) -> void {
					OPERATIONS_AUTO_CLOSE_POPUP(ws)

					df.reject();
				},
				nullptr
			);
			ws->inputPopupBox(
				content ? content : ws->theme()->dialogItem_Input().c_str(),
				default_ ? default_ : "", flags,
				confirm,
				cancel
			);
		}
	);
}

promise::Defer Operations::popupWait(class Renderer*, Workspace* ws, const char* content) {
	const ImGui::PopupBox::Ptr reserved = ws->popupBox();

	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			ImGui::WaitingPopupBox::TimeoutHandler timeout(
				[ws, df] (void) -> void {
					OPERATIONS_AUTO_CLOSE_POPUP(ws)

					df.resolve(true);
				},
				nullptr
			);
			ws->waitingPopupBox(
				content,
				timeout
			);
		}
	)
	.then(
		[reserved] (void) -> void {
			// Do nothing.
		}
	);
}

promise::Defer Operations::fileBackup(class Renderer*, Workspace*, const Project* project) {
	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			LockGuard<RecursiveMutex>::UniquePtr acquired;
			Project* prj = project->acquire(acquired);
			if (!prj) {
				df.reject();

				return;
			}

			const std::string path = prj->path();
			if (path.empty()) {
				df.reject();

				return;
			}

			const std::string backupDir = Path::combine(Path::writableDirectory().c_str(), OPERATIONS_BACKUP_DIR);
			if (!Path::existsDirectory(backupDir.c_str()))
				Path::touchDirectory(backupDir.c_str());

			const std::string dstDir = Path::combine(Path::writableDirectory().c_str(), OPERATIONS_BACKUP_DIR, OPERATIONS_BACKUP_NAME);
			const std::string dstFile = Path::combine(Path::writableDirectory().c_str(), OPERATIONS_BACKUP_DIR, OPERATIONS_BACKUP_NAME "." BITTY_PROJECT_EXT);
			if (Path::existsDirectory(dstDir.c_str()))
				Path::removeDirectory(dstDir.c_str(), false);
			if (Path::existsFile(dstFile.c_str()))
				Path::removeFile(dstFile.c_str(), false);

			if (Path::existsDirectory(path.c_str()))
				Path::copyDirectory(path.c_str(), dstDir.c_str());
			else if (Path::existsFile(path.c_str()))
				Path::copyFile(path.c_str(), dstFile.c_str());

			const std::string editPath = Path::combine(Path::writableDirectory().c_str(), OPERATIONS_EDITING_NAME "." BITTY_TEXT_EXT);
			File::Ptr editFile(File::create());
			if (editFile->open(editPath.c_str(), Stream::WRITE)) {
				std::string time;
				DateTime::now(time);
				std::string editInfo;
				editInfo += "Path: " + prj->path();
				editInfo += '\n';
				editInfo += "Time: " + time;
				editInfo += '\n';
				editFile->writeString(editInfo);
				editFile->close();
			}

			df.resolve(true);
		}
	);
}

promise::Defer Operations::fileRestore(class Renderer*, Workspace* ws, const Project* project) {
	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			LockGuard<RecursiveMutex>::UniquePtr acquired;
			Project* prj = project->acquire(acquired);
			if (!prj) {
				df.reject();

				return;
			}

			const std::string editPath = Path::combine(Path::writableDirectory().c_str(), OPERATIONS_EDITING_NAME "." BITTY_TEXT_EXT);
			if (!Path::existsFile(editPath.c_str())) {
				Path::touchFile(editPath.c_str());

				df.resolve(true);

				return;
			}

			const std::string dstDir = Path::combine(Path::writableDirectory().c_str(), OPERATIONS_BACKUP_DIR, OPERATIONS_BACKUP_NAME);
			const std::string dstFile = Path::combine(Path::writableDirectory().c_str(), OPERATIONS_BACKUP_DIR, OPERATIONS_BACKUP_NAME "." BITTY_PROJECT_EXT);
			if (!Path::existsDirectory(dstDir.c_str()) && !Path::existsFile(dstFile.c_str())) {
				df.resolve(false);

				return;
			}

			ImGui::MessagePopupBox::ConfirmHandler confirm(
				[ws, df] (void) -> void {
					OPERATIONS_AUTO_CLOSE_POPUP(ws)

					const std::string path = Unicode::toOs(Path::writableDirectory());
					Platform::browse(path.c_str());

					df.reject(false);
				},
				nullptr
			);
			ImGui::MessagePopupBox::DenyHandler deny(
				[ws, df] (void) -> void {
					OPERATIONS_AUTO_CLOSE_POPUP(ws)

					df.reject(false);
				},
				nullptr
			);
			ws->messagePopupBox(
				ws->theme()->dialogAsk_DidntQuitCleanlyBrowseTheLatestBackup(),
				confirm,
				deny,
				nullptr
			);
		}
	);
}

promise::Defer Operations::fileClean(class Renderer*, Workspace*, const Project* project) {
	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			LockGuard<RecursiveMutex>::UniquePtr acquired;
			Project* prj = project->acquire(acquired);
			if (!prj) {
				df.reject();

				return;
			}

			const std::string editPath = Path::combine(Path::writableDirectory().c_str(), OPERATIONS_EDITING_NAME "." BITTY_TEXT_EXT);
			if (!Path::existsFile(editPath.c_str())) {
				df.resolve(false);

				return;
			}

			Path::removeFile(editPath.c_str(), false);

			df.resolve(true);
		}
	);
}

promise::Defer Operations::fileNew(class Renderer* rnd, Workspace* ws, const class Project* project, Executable* exec) {
	auto next = [project] (promise::Defer df) -> void {
		LockGuard<RecursiveMutex>::UniquePtr acquired;
		Project* prj = project->acquire(acquired);
		if (!prj)
			return;

		if (prj->empty()) {
			prj->title("Noname");
			prj->version("1.0");

			Asset* asset = prj->factory().create(prj);
			Json::Ptr json(Json::create());
			rapidjson::Document doc;
			Jpath::set(doc, doc, prj->id(), "id");
			Jpath::set(doc, doc, prj->title(), "title");
			Jpath::set(doc, doc, prj->description(), "description");
			Jpath::set(doc, doc, prj->author(), "author");
			Jpath::set(doc, doc, prj->version(), "version");
			Jpath::set(doc, doc, prj->genre(), "genre");
			Jpath::set(doc, doc, prj->url(), "url");
			json->fromJson(doc);
			asset->link(json, PROJECT_INFO_NAME "." BITTY_JSON_EXT);
			prj->add(asset);

			asset = prj->factory().create(prj);
			Code::Ptr code(Code::create());
			code->text(OPERATIONS_ASSET_DEFAULT_CODE, BITTY_COUNTOF(OPERATIONS_ASSET_DEFAULT_CODE) - 1);
			asset->link(code, prj->entry().c_str());

			Asset::States* states = asset->states();
			states->activate(Asset::States::EDITABLE);

			prj->add(asset);
		}

		df.resolve(true);
	};

	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			fileClose(rnd, ws, project, exec)
				.then(
					[next, df] (void) -> promise::Defer {
						return promise::newPromise(std::bind(next, df));
					}
				)
				.fail(
					[df] (void) -> void {
						df.reject();
					}
				);
		}
	);
}

promise::Defer Operations::fileOpenFile(class Renderer* rnd, Workspace* ws, const class Project* project, Executable* exec, const char* path_) {
	const std::string path = path_ ? path_ : "";
	auto next = [ws, project, exec, path] (promise::Defer df) -> void {
		std::string path_ = path;
		if (path_.empty()) {
			pfd::open_file open(
				ws->theme()->generic_Open(),
				"",
				OPERATIONS_BITTY_FULL_FILE_FILTER
			);
			if (open.result().empty() || open.result().front().empty()) {
				df.reject();

				return;
			}
			path_ = open.result().front();
		}
		Path::uniform(path_);

#if defined BITTY_DEBUG
		const long long start = DateTime::ticks();
#endif /* BITTY_DEBUG */

		LockGuard<RecursiveMutex>::UniquePtr acquired;
		Project* prj = project->acquire(acquired);
		if (!prj) {
			df.reject();

			return;
		}

		exec->clearBreakpoints(nullptr);

		prj->unload();
		prj->readonly(false);

		if (!prj->load(path_.c_str())) {
			df.reject();

			ws->messagePopupBox(
				ws->theme()->dialogPrompt_CannotLoadProject(),
				nullptr,
				nullptr,
				nullptr
			);

			return;
		}
		prj->dirty(false);

		Asset* asset = prj->main();
		if (asset) {
			Asset::States* states = asset->states();
			states->activate(Asset::States::EDITABLE);
		}

		df.resolve(true);

#if defined BITTY_DEBUG
		const long long end = DateTime::ticks();
		const long long diff = end - start;
		const double secs = DateTime::toSeconds(diff);
		fprintf(stdout, "Project opened in %gs.\n", secs);
#endif /* BITTY_DEBUG */
	};

	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			fileAskSave(rnd, ws, project, exec)
				.then(
					[next, df] (bool /* arg */) -> promise::Defer {
						return promise::newPromise(std::bind(next, df));
					}
				)
				.fail(
					[df] (void) -> void {
						df.reject();
					}
				);
		}
	);
}

promise::Defer Operations::fileOpenDirectory(class Renderer* rnd, Workspace* ws, const class Project* project, Executable* exec, const char* path_) {
	const std::string path = path_ ? path_ : "";
	auto next = [ws, project, exec, path] (promise::Defer df) -> void {
		OPERATIONS_AUTO_CLOSE_POPUP(ws)

		std::string path_ = path;
		if (path_.empty()) {
			pfd::select_folder open(
				ws->theme()->generic_Open(),
				""
			);
			if (open.result().empty()) {
				df.reject();

				return;
			}
			path_ = open.result();
		}
		Path::uniform(path_);

#if defined BITTY_DEBUG
		const long long start = DateTime::ticks();
#endif /* BITTY_DEBUG */

		LockGuard<RecursiveMutex>::UniquePtr acquired;
		Project* prj = project->acquire(acquired);
		if (!prj) {
			df.reject();

			return;
		}

		exec->clearBreakpoints(nullptr);

		prj->unload();
		prj->readonly(false);

		const DirectoryInfo::Ptr dirInfo(DirectoryInfo::make(path_.c_str()));
		if (!dirInfo->exists()) {
			df.reject();

			return;
		}
		FileInfos::Ptr fileInfos = dirInfo->getFiles(PROJECT_INFO_NAME "." BITTY_JSON_EXT, false);
		if (fileInfos->count() == 0) {
			df.reject();

			ws->messagePopupBox(
				ws->theme()->dialogPrompt_InvalidProject(),
				nullptr,
				nullptr,
				nullptr
			);

			return;
		}
		fileInfos = dirInfo->getFiles(PROJECT_ENTRY_NAME "." BITTY_LUA_EXT, false);
		if (fileInfos->count() == 0) {
			df.reject();

			ws->messagePopupBox(
				ws->theme()->dialogPrompt_InvalidProject(),
				nullptr,
				nullptr,
				nullptr
			);

			return;
		}

		if (!prj->load(path_.c_str())) {
			df.reject();

			ws->messagePopupBox(
				ws->theme()->dialogPrompt_CannotLoadProject(),
				nullptr,
				nullptr,
				nullptr
			);

			return;
		}
		prj->dirty(false);

		Asset* asset = prj->main();
		if (asset) {
			Asset::States* states = asset->states();
			states->activate(Asset::States::EDITABLE);
		}

		df.resolve(true);

#if defined BITTY_DEBUG
		const long long end = DateTime::ticks();
		const long long diff = end - start;
		const double secs = DateTime::toSeconds(diff);
		fprintf(stdout, "Project opened in %gs.\n", secs);
#endif /* BITTY_DEBUG */
	};

	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			fileAskSave(rnd, ws, project, exec)
				.then(
					[next, df] (bool /* arg */) -> promise::Defer {
						return promise::newPromise(std::bind(next, df));
					}
				)
				.fail(
					[df] (void) -> void {
						df.reject();
					}
				);
		}
	);
}

promise::Defer Operations::fileOpenExample(class Renderer* rnd, Workspace* ws, const class Project* project, Executable* exec, const char* path_) {
	const std::string path = path_ ? path_ : "";
	auto next = [project, path] (promise::Defer df) -> void {
		std::string path_ = path;
		Path::uniform(path_);

		LockGuard<RecursiveMutex>::UniquePtr acquired;
		Project* prj = project->acquire(acquired);
		if (!prj) {
			df.reject();

			return;
		}

		if (!prj->load(path_.c_str())) {
			df.reject();

			return;
		}
		prj->readonly(true);
		prj->dirty(false);

		Asset* asset = prj->main();
		if (asset) {
			Asset::States* states = asset->states();
			states->activate(Asset::States::EDITABLE);
		}

		df.resolve(true);
	};

	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			fileClose(rnd, ws, project, exec)
				.then(
					[next, df] (void) -> promise::Defer {
						return promise::newPromise(std::bind(next, df));
					}
				)
				.fail(
					[df] (void) -> void {
						df.reject();
					}
				);
		}
	);
}

promise::Defer Operations::fileCloseAsset(class Renderer* rnd, Workspace* ws, const class Project* project, Asset::List::Index index) {
	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			std::string entry;
			bool revertible = true;
			std::string msg = ws->theme()->dialogAsk_SaveAsset();
			do {
				LockGuard<RecursiveMutex>::UniquePtr acquired;
				Project* prj = project->acquire(acquired);
				if (!prj) {
					df.reject();

					return;
				}

				Asset* asset = prj->get(index);
				if (!asset) {
					df.reject();

					return;
				}

				if (!asset->dirty()) {
					Asset::States* states = asset->states();
					states->deactivate();
					states->deselect();

					asset->finish(Asset::EDITING, false);

					prj->cleanup(Asset::EDITING);

					df.resolve(false);

					return;
				}

				entry = asset->entry().name();
				revertible = asset->revertible();

				std::string inner;
				inner += ":\n  \"";
				inner += asset->entry().name();
				inner += '"';
				inner += '?';
				msg.pop_back();
				msg += inner;
			} while (false);

			ImGui::MessagePopupBox::ConfirmHandler confirm(
				[rnd, ws, project, index, entry, df] (void) -> void {
					OPERATIONS_AUTO_CLOSE_POPUP(ws)

#if BITTY_TRIAL_ENABLED
					df.reject();

					ws->messagePopupBox(
						ws->theme()->dialogPrompt_GetFullVersionToSave(),
						nullptr,
						nullptr,
						nullptr
					);
#else /* BITTY_TRIAL_ENABLED */
					fileSaveAsset(rnd, ws, project, index)
						.then(
							[project, index, entry, df] (bool arg) -> void {
								LockGuard<RecursiveMutex>::UniquePtr acquired;
								Project* prj = project->acquire(acquired);
								if (!prj) {
									df.reject();

									return;
								}

								Asset* asset = prj->get(entry.c_str());
								if (!asset) {
									df.reject();

									return;
								}

								Asset::States* states = asset->states();
								states->deactivate();
								states->deselect();

								asset->finish(Asset::EDITING, false);

								prj->cleanup(Asset::EDITING);

								df.resolve(arg);
							}
						)
						.fail(
							[df] (void) -> void {
								df.reject();
							}
						);
#endif /* BITTY_TRIAL_ENABLED */
				},
				nullptr
			);
			ImGui::MessagePopupBox::DenyHandler deny(
				[ws, project, index, entry, df] (void) -> void {
					OPERATIONS_AUTO_CLOSE_POPUP(ws)

					do {
						LockGuard<RecursiveMutex>::UniquePtr acquired;
						Project* prj = project->acquire(acquired);
						if (!prj)
							break;

						Asset* asset = prj->get(entry.c_str());
						if (!asset)
							break;

						Asset::States* states = asset->states();
						states->deactivate();
						states->deselect();

						if (asset->finish(Asset::EDITING, false))
							asset->dirty(false);

						prj->cleanup(Asset::EDITING);
					} while (false);

					df.resolve(false);
				},
				nullptr
			);
			ImGui::MessagePopupBox::CancelHandler cancel(
				[ws, df] (void) -> void {
					OPERATIONS_AUTO_CLOSE_POPUP(ws)

					df.reject();
				},
				nullptr
			);
			ws->messagePopupBox(
				msg,
				confirm,
				revertible ? deny : nullptr,
				cancel
			);
		}
	);
}

promise::Defer Operations::fileClose(class Renderer* rnd, Workspace* ws, const class Project* project, Executable* exec) {
	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			fileAskSave(rnd, ws, project, exec)
				.then(
					[ws, project, exec, df] (bool arg) -> void {
						ws->assetsFiltering(false);
						ws->assetsFilteringInitialized(false);
						ws->assetsFilterInput().clear();
						ws->assetsFilterPatterns().clear();

						exec->clearBreakpoints(nullptr);

						do {
							LockGuard<RecursiveMutex>::UniquePtr acquired;
							Project* prj = project->acquire(acquired);
							if (!prj)
								break;

							prj->unload();
							prj->readonly(false);
						} while (false);

						df.resolve(arg);
					}
				)
				.fail(
					[df] (void) -> void {
						df.reject();
					}
				);
		}
	);
}

promise::Defer Operations::fileAskSave(class Renderer* rnd, Workspace* ws, const class Project* project, Executable* exec) {
	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			LockGuard<RecursiveMutex>::UniquePtr acquired;
			Project* prj = project->acquire(acquired);
			if (!prj) {
				df.reject();

				return;
			}

			if (!prj->dirty()) {
				df.resolve(false);

				return;
			}

			const bool archived = prj->archived();

			ImGui::MessagePopupBox::ConfirmHandler confirm(
				[rnd, ws, project, exec, archived, df] (void) -> void {
#if BITTY_TRIAL_ENABLED
					OPERATIONS_AUTO_CLOSE_POPUP(ws)

					df.reject();

					ws->messagePopupBox(
						ws->theme()->dialogPrompt_GetFullVersionToSave(),
						nullptr,
						nullptr,
						nullptr
					);
#else /* BITTY_TRIAL_ENABLED */
					(archived ? fileSaveFile(rnd, ws, project, false) : fileSaveDirectory(rnd, ws, project, false))
						.then(
							[ws, df] (bool arg) -> void {
								OPERATIONS_AUTO_CLOSE_POPUP(ws)

								df.resolve(arg);
							}
						)
						.fail(
							[ws, df] (void) -> void {
								OPERATIONS_AUTO_CLOSE_POPUP(ws)

								df.reject();
							}
						);
#endif /* BITTY_TRIAL_ENABLED */
				},
				nullptr
			);
			ImGui::MessagePopupBox::DenyHandler deny(
				[ws, df] (void) -> void {
					OPERATIONS_AUTO_CLOSE_POPUP(ws)

					df.resolve(false);
				},
				nullptr
			);
			ImGui::MessagePopupBox::CancelHandler cancel(
				[ws, df] (void) -> void {
					OPERATIONS_AUTO_CLOSE_POPUP(ws)

					df.reject();
				},
				nullptr
			);
			ws->messagePopupBox(
				ws->theme()->dialogAsk_SaveTheCurrentWorkspace(),
				confirm,
				deny,
				cancel
			);
		}
	);
}

promise::Defer Operations::fileSaveAsset(class Renderer* rnd, Workspace* ws, const class Project* project, Asset::List::Index index) {
#if BITTY_TRIAL_ENABLED
	(void)rnd;
	(void)project;
	(void)index;

	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			df.resolve(true);

			ws->messagePopupBox(
				ws->theme()->dialogPrompt_GetFullVersionToSave(),
				nullptr,
				nullptr,
				nullptr
			);
		}
	);
#else /* BITTY_TRIAL_ENABLED */
	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			LockGuard<RecursiveMutex>::UniquePtr acquired;
			Project* prj = project->acquire(acquired);
			if (!prj) {
				df.reject();

				return;
			}

			Asset* asset = prj->get(index);
			if (!asset) {
				df.reject();

				return;
			}

			if (!asset->dirty()) {
				df.resolve(true);

				return;
			}

			bool saveProject = prj->path().empty();
			if (!saveProject) {
				const Archive* archive = prj->archive(Stream::Accesses::READ);
				const bool archiveFormatChanged = archive && archive->format() != prj->preference();
				if (archive && archiveFormatChanged) {
					prj->archive(nullptr);

					saveProject = true;
				}
			}
			if (!saveProject) {
				if (prj->readonly())
					saveProject = true;
			}

			auto next = [rnd, ws, project, index, saveProject] (promise::Defer df) -> void {
				LockGuard<RecursiveMutex>::UniquePtr acquired;
				Project* prj = project->acquire(acquired);
				if (!prj) {
					df.reject();

					return;
				}

				Asset* asset = prj->get(index);
				if (!asset) {
					df.reject();

					return;
				}

				if (saveProject) {
					prj->dirty(true);

					(prj->archived() ? fileSaveFile(rnd, ws, project, false) : fileSaveDirectory(rnd, ws, project, false))
						.then(
							[df] (bool arg) -> void {
								df.resolve(arg);
							}
						)
						.fail(
							[df] (void) -> void {
								df.reject();
							}
						);
				} else {
					Editable* editor = asset->editor();
					if (!editor) {
						df.reject();

						return;
					}

					fileBackup(rnd, ws, project);
					editor->flush();
					if (prj->archived()) {
						if (asset->exists())
							asset->remove();
					}
					asset->save(Asset::EDITING);
					asset->dirty(false);

					if (asset == prj->info())
						prj->parse();

					df.resolve(true);
				}
			};

			popupWait(rnd, ws, ws->theme()->dialogPrompt_Saving().c_str())
				.then(
					[next, df] (void) -> promise::Defer {
						return promise::newPromise(std::bind(next, df));
					}
				);
		}
	);
#endif /* BITTY_TRIAL_ENABLED */
}

promise::Defer Operations::fileSaveFile(class Renderer* rnd, Workspace* ws, const class Project* project, bool saveAs) {
#if BITTY_TRIAL_ENABLED
	(void)rnd;
	(void)project;
	(void)saveAs;

	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			df.resolve(true);

			ws->messagePopupBox(
				ws->theme()->dialogPrompt_GetFullVersionToSave(),
				nullptr,
				nullptr,
				nullptr
			);
		}
	);
#else /* BITTY_TRIAL_ENABLED */
	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			LockGuard<RecursiveMutex>::UniquePtr acquired;
			Project* prj = project->acquire(acquired);
			if (!prj) {
				df.reject();

				return;
			}

			if (prj->archived() && !prj->dirty() && !saveAs) {
				df.resolve(false);

				return;
			}

			std::string path;
			if (prj->archived() && !saveAs)
				path = prj->path();
			if (path.empty() || prj->readonly()) {
				pfd::save_file save(
					ws->theme()->generic_SaveTo(),
					"",
					OPERATIONS_BITTY_FILE_FILTER
				);
				path = save.result();
				Path::uniform(path);
				if (path.empty()) {
					df.reject();

					return;
				}
				std::string ext;
				Path::split(path, nullptr, &ext, nullptr);
				if (ext.empty())
					path += "." BITTY_PROJECT_EXT;
				if (!ws->canSaveTo(path.c_str())) {
					df.reject();

					ws->messagePopupBox(
						ws->theme()->dialogPrompt_CannotSaveToReadonlyLocations(),
						nullptr,
						nullptr,
						nullptr
					);

					return;
				}
			}

			auto next = [rnd, ws, project, path] (promise::Defer df) -> void {
#if defined BITTY_DEBUG
				const long long start = DateTime::ticks();
#endif /* BITTY_DEBUG */

				LockGuard<RecursiveMutex>::UniquePtr acquired;
				Project* prj = project->acquire(acquired);
				if (!prj) {
					df.reject();

					return;
				}

				std::map<std::string, Asset::States::Activity> cache;
				prj->foreach(
					[&] (Asset* &asset, Asset::List::Index) -> void {
						Asset::States* states = asset->states();
						const std::string entry = asset->entry().name();
						const Asset::States::Activity activity = states->activity();
						cache.insert(std::make_pair(entry, activity));
					}
				);
				fileBackup(rnd, ws, project);
				if (!prj->save(path.c_str(), true, std::bind(operationsHandleError, ws, std::placeholders::_1))) {
					df.reject();

					return;
				}
				prj->foreach(
					[&] (Asset* &asset, Asset::List::Index) -> void {
						Asset::States* states = asset->states();
						const std::string entry = asset->entry().name();
						auto kv = cache.find(entry);
						if (kv == cache.end())
							return;

						states->activate(kv->second);
					}
				);
				prj->readonly(false);
				prj->dirty(false);

				df.resolve(true);

#if defined BITTY_DEBUG
				const long long end = DateTime::ticks();
				const long long diff = end - start;
				const double secs = DateTime::toSeconds(diff);
				fprintf(stdout, "Project saved in %gs.\n", secs);
#endif /* BITTY_DEBUG */
			};

			popupWait(rnd, ws, ws->theme()->dialogPrompt_Saving().c_str())
				.then(
					[next, df] (void) -> promise::Defer {
						return promise::newPromise(std::bind(next, df));
					}
				);
		}
	);
#endif /* BITTY_TRIAL_ENABLED */
}

promise::Defer Operations::fileSaveDirectory(class Renderer* rnd, Workspace* ws, const class Project* project, bool saveAs) {
#if BITTY_TRIAL_ENABLED
	(void)rnd;
	(void)project;
	(void)saveAs;

	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			df.resolve(true);

			ws->messagePopupBox(
				ws->theme()->dialogPrompt_GetFullVersionToSave(),
				nullptr,
				nullptr,
				nullptr
			);
		}
	);
#else /* BITTY_TRIAL_ENABLED */
	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			LockGuard<RecursiveMutex>::UniquePtr acquired;
			Project* prj = project->acquire(acquired);
			if (!prj) {
				df.reject();

				return;
			}

			if (!prj->archived() && !prj->dirty() && !saveAs) {
				df.resolve(false);

				return;
			}

			std::string path;
			if (!prj->archived() && !saveAs)
				path = prj->path();
			if (path.empty() || prj->readonly()) {
				pfd::select_folder save(
					ws->theme()->generic_SaveTo(),
					""
				);
				path = save.result();
				Path::uniform(path);
				if (path.empty()) {
					df.reject();

					return;
				}
				if (!ws->canSaveTo(path.c_str())) {
					df.reject();

					ws->messagePopupBox(
						ws->theme()->dialogPrompt_CannotSaveToReadonlyLocations(),
						nullptr,
						nullptr,
						nullptr
					);

					return;
				}

				DirectoryInfo::Ptr dirInfo = DirectoryInfo::make(path.c_str());
				if (!dirInfo->exists())
					dirInfo->make();
				FileInfos::Ptr fileInfos = dirInfo->getFiles("*;*.*", true);
				if (fileInfos->count() != 0) {
					df.reject();

					ws->messagePopupBox(
						ws->theme()->dialogPrompt_CannotSaveToNonemptyDirectory(),
						nullptr,
						nullptr,
						nullptr
					);

					return;
				}
			}

			auto next = [rnd, ws, project, path] (promise::Defer df) -> void {
#if defined BITTY_DEBUG
				const long long start = DateTime::ticks();
#endif /* BITTY_DEBUG */

				LockGuard<RecursiveMutex>::UniquePtr acquired;
				Project* prj = project->acquire(acquired);
				if (!prj) {
					df.reject();

					return;
				}

				std::map<std::string, Asset::States::Activity> cache;
				prj->foreach(
					[&] (Asset* &asset, Asset::List::Index) -> void {
						Asset::States* states = asset->states();
						const std::string entry = asset->entry().name();
						const Asset::States::Activity activity = states->activity();
						cache.insert(std::make_pair(entry, activity));
					}
				);
				fileBackup(rnd, ws, project);
				if (!prj->save(path.c_str(), true, std::bind(operationsHandleError, ws, std::placeholders::_1))) {
					df.reject();

					return;
				}
				prj->foreach(
					[&] (Asset* &asset, Asset::List::Index) -> void {
						Asset::States* states = asset->states();
						const std::string entry = asset->entry().name();
						auto kv = cache.find(entry);
						if (kv == cache.end())
							return;

						states->activate(kv->second);
					}
				);
				prj->readonly(false);
				prj->dirty(false);

				df.resolve(true);

#if defined BITTY_DEBUG
				const long long end = DateTime::ticks();
				const long long diff = end - start;
				const double secs = DateTime::toSeconds(diff);
				fprintf(stdout, "Project saved in %gs.\n", secs);
#endif /* BITTY_DEBUG */
			};

			popupWait(rnd, ws, ws->theme()->dialogPrompt_Saving().c_str())
				.then(
					[next, df] (void) -> promise::Defer {
						return promise::newPromise(std::bind(next, df));
					}
				);
		}
	);
#endif /* BITTY_TRIAL_ENABLED */
}

promise::Defer Operations::editResizeImage(class Renderer*, Workspace* ws, const class Project* project, const char* asset_) {
	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			OPERATIONS_AUTO_CLOSE_POPUP(ws)

			LockGuard<RecursiveMutex>::UniquePtr acquired;
			Project* prj = project->acquire(acquired);
			if (!prj) {
				df.reject();

				return;
			}

			Asset* asset = prj->get(asset_);
			if (!asset) {
				df.reject();

				return;
			}

			if (asset->type() != Image::TYPE()) {
				df.reject();

				return;
			}

			Object::Ptr obj = asset->object(Asset::EDITING);
			Image::Ptr ptr = Object::as<Image::Ptr>(obj);
			if (!ptr) {
				df.reject();

				return;
			}
			const Math::Vec2i defaultSize(ptr->width(), ptr->height());
			const Math::Vec2i maxSize(BITTY_IMAGE_MAX_WIDTH, BITTY_IMAGE_MAX_HEIGHT);

			const std::string assetStr = asset_;
			ImGui::ResizePopupBox::ConfirmHandler confirm(
				[ws, project, assetStr, df] (const Math::Vec2i* size) -> void {
					OPERATIONS_AUTO_CLOSE_POPUP(ws)

					if (!size || size->x <= 0 || size->y <= 0) {
						df.reject();

						return;
					}

					LockGuard<RecursiveMutex>::UniquePtr acquired;
					Project* prj = project->acquire(acquired);
					if (!prj) {
						df.reject();

						return;
					}

					Asset* asset = prj->get(assetStr.c_str());
					if (!asset) {
						df.reject();

						return;
					}

					Editable* editor = asset->editor();
					if (!editor) {
						df.reject();

						return;
					}

					editor->post(Editable::RESIZE, size->x, size->y);

					df.resolve(true, size);
				},
				nullptr
			);
			ImGui::ResizePopupBox::CancelHandler cancel(
				[ws, df] (void) -> void {
					OPERATIONS_AUTO_CLOSE_POPUP(ws)

					df.reject();
				},
				nullptr
			);
			ws->popupBox(
				ImGui::PopupBox::Ptr(
					new ImGui::ResizePopupBox(
						BITTY_NAME,
						ws->theme()->dialogItem_ImageSize(),
						defaultSize,
						maxSize,
						confirm,
						cancel,
						ws->theme()->generic_Ok().c_str(), ws->theme()->generic_Cancel().c_str()
					)
				)
			);
		}
	);
}

promise::Defer Operations::editResizeImageGrid(class Renderer*, Workspace* ws, const class Project* project, const char* asset_) {
	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			OPERATIONS_AUTO_CLOSE_POPUP(ws)

			LockGuard<RecursiveMutex>::UniquePtr acquired;
			Project* prj = project->acquire(acquired);
			if (!prj) {
				df.reject();

				return;
			}

			Asset* asset = prj->get(asset_);
			if (!asset) {
				df.reject();

				return;
			}

			if (asset->type() != Image::TYPE()) {
				df.reject();

				return;
			}

			Object::Ptr obj = asset->object(Asset::EDITING);
			Image::Ptr ptr = Object::as<Image::Ptr>(obj);
			if (!ptr) {
				df.reject();

				return;
			}
			const Math::Vec2i defaultSize(BITTY_GRID_DEFAULT_SIZE, BITTY_GRID_DEFAULT_SIZE);
			const Math::Vec2i maxSize(ptr->width(), ptr->height());

			const std::string assetStr = asset_;
			ImGui::ResizePopupBox::ConfirmHandler confirm(
				[ws, project, assetStr, df] (const Math::Vec2i* size) -> void {
					OPERATIONS_AUTO_CLOSE_POPUP(ws)

					if (!size || size->x <= 0 || size->y <= 0) {
						df.reject();

						return;
					}

					LockGuard<RecursiveMutex>::UniquePtr acquired;
					Project* prj = project->acquire(acquired);
					if (!prj) {
						df.reject();

						return;
					}

					Asset* asset = prj->get(assetStr.c_str());
					if (!asset) {
						df.reject();

						return;
					}

					Editable* editor = asset->editor();
					if (!editor) {
						df.reject();

						return;
					}

					editor->post(Editable::RESIZE_GRID, size->x, size->y);

					df.resolve(true, size);
				},
				nullptr
			);
			ImGui::ResizePopupBox::CancelHandler cancel(
				[ws, df] (void) -> void {
					OPERATIONS_AUTO_CLOSE_POPUP(ws)

					df.reject();
				},
				nullptr
			);
			ws->popupBox(
				ImGui::PopupBox::Ptr(
					new ImGui::ResizePopupBox(
						BITTY_NAME,
						ws->theme()->dialogItem_GridSize(),
						defaultSize,
						maxSize,
						confirm,
						cancel,
						ws->theme()->generic_Ok().c_str(), ws->theme()->generic_Cancel().c_str()
					)
				)
			);
		}
	);
}

promise::Defer Operations::editResizeMap(class Renderer*, Workspace* ws, const class Project* project, const char* asset_) {
	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			OPERATIONS_AUTO_CLOSE_POPUP(ws)

			LockGuard<RecursiveMutex>::UniquePtr acquired;
			Project* prj = project->acquire(acquired);
			if (!prj) {
				df.reject();

				return;
			}

			Asset* asset = prj->get(asset_);
			if (!asset) {
				df.reject();

				return;
			}

			if (asset->type() != Map::TYPE()) {
				df.reject();

				return;
			}

			Object::Ptr obj = asset->object(Asset::EDITING);
			Map::Ptr ptr = Object::as<Map::Ptr>(obj);
			if (!ptr) {
				df.reject();

				return;
			}
			const Math::Vec2i defaultSize(ptr->width(), ptr->height());
			const Math::Vec2i maxSize(BITTY_MAP_MAX_WIDTH, BITTY_MAP_MAX_HEIGHT);

			const std::string assetStr = asset_;
			ImGui::ResizePopupBox::ConfirmHandler confirm(
				[ws, project, assetStr, df] (const Math::Vec2i* size) -> void {
					OPERATIONS_AUTO_CLOSE_POPUP(ws)

					if (!size || size->x <= 0 || size->y <= 0) {
						df.reject();

						return;
					}

					LockGuard<RecursiveMutex>::UniquePtr acquired;
					Project* prj = project->acquire(acquired);
					if (!prj) {
						df.reject();

						return;
					}

					Asset* asset = prj->get(assetStr.c_str());
					if (!asset) {
						df.reject();

						return;
					}

					Editable* editor = asset->editor();
					if (!editor) {
						df.reject();

						return;
					}

					editor->post(Editable::RESIZE, size->x, size->y);

					df.resolve(true, size);
				},
				nullptr
			);
			ImGui::ResizePopupBox::CancelHandler cancel(
				[ws, df] (void) -> void {
					OPERATIONS_AUTO_CLOSE_POPUP(ws)

					df.reject();
				},
				nullptr
			);
			ws->popupBox(
				ImGui::PopupBox::Ptr(
					new ImGui::ResizePopupBox(
						BITTY_NAME,
						ws->theme()->dialogItem_MapSize(),
						defaultSize,
						maxSize,
						confirm,
						cancel,
						ws->theme()->generic_Ok().c_str(), ws->theme()->generic_Cancel().c_str()
					)
				)
			);
		}
	);
}

promise::Defer Operations::editResizeTile(class Renderer*, Workspace* ws, const class Project* project, const char* asset_) {
	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			OPERATIONS_AUTO_CLOSE_POPUP(ws)

			LockGuard<RecursiveMutex>::UniquePtr acquired;
			Project* prj = project->acquire(acquired);
			if (!prj) {
				df.reject();

				return;
			}

			Asset* asset = prj->get(asset_);
			if (!asset) {
				df.reject();

				return;
			}

			if (asset->type() != Map::TYPE()) {
				df.reject();

				return;
			}

			Object::Ptr obj = asset->object(Asset::EDITING);
			Map::Ptr ptr = Object::as<Map::Ptr>(obj);
			if (!ptr) {
				df.reject();

				return;
			}
			Map::Tiles tiles;
			if (!ptr->tiles(tiles) || !tiles.texture) {
				df.reject();

				return;
			}
			Math::Vec2i defaultSize(BITTY_MAP_TILE_DEFAULT_SIZE, BITTY_MAP_TILE_DEFAULT_SIZE);
			if (tiles.count.x > 0 && tiles.count.y > 0)
				defaultSize = tiles.size();
			const Math::Vec2i maxSize(tiles.texture->width(), tiles.texture->height());

			const std::string assetStr = asset_;
			ImGui::ResizePopupBox::ConfirmHandler confirm(
				[ws, project, assetStr, df] (const Math::Vec2i* size) -> void {
					OPERATIONS_AUTO_CLOSE_POPUP(ws)

					if (!size || size->x <= 0 || size->y <= 0) {
						df.reject();

						return;
					}

					LockGuard<RecursiveMutex>::UniquePtr acquired;
					Project* prj = project->acquire(acquired);
					if (!prj) {
						df.reject();

						return;
					}

					Asset* asset = prj->get(assetStr.c_str());
					if (!asset) {
						df.reject();

						return;
					}

					Object::Ptr obj = asset->object(Asset::EDITING);
					Map::Ptr ptr = Object::as<Map::Ptr>(obj);
					if (!ptr) {
						df.reject();

						return;
					}
					Map::Tiles tiles;
					if (!ptr->tiles(tiles) || !tiles.texture) {
						df.reject();

						return;
					}
					tiles.count = Math::Vec2i(
						tiles.texture->width() / size->x,
						tiles.texture->height() / size->y
					);
					tiles.fit(Math::Vec2i(size->x, size->y));
					ptr->tiles(&tiles);
					asset->dirty(true);

					df.resolve(true, size);
				},
				nullptr
			);
			ImGui::ResizePopupBox::CancelHandler cancel(
				[ws, df] (void) -> void {
					OPERATIONS_AUTO_CLOSE_POPUP(ws)

					df.reject();
				},
				nullptr
			);
			ws->popupBox(
				ImGui::PopupBox::Ptr(
					new ImGui::ResizePopupBox(
						BITTY_NAME,
						ws->theme()->dialogItem_TileSize(),
						defaultSize,
						maxSize,
						confirm,
						cancel,
						ws->theme()->generic_Ok().c_str(), ws->theme()->generic_Cancel().c_str()
					)
				)
			);
		}
	);
}

promise::Defer Operations::editResolveRef(class Renderer* rnd, Workspace* ws, const class Project* project, const char* asset_) {
	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			OPERATIONS_AUTO_CLOSE_POPUP(ws)

			LockGuard<RecursiveMutex>::UniquePtr acquired;
			Project* prj = project->acquire(acquired);
			if (!prj) {
				df.reject();

				return;
			}

			Asset* asset = prj->get(asset_);
			if (!asset) {
				df.reject();

				return;
			}

			std::string extra = ws->theme()->dialogItem_ResolveAssetRefFor();
			extra += "\n  \"";
			extra += asset->entry().name();
			extra += "\"";

			ImGui::AssetFilter filter = nullptr;
			switch (asset->type()) {
			case Image::TYPE():
				filter = [] (const class Asset* asset) -> bool {
					return asset->type() != Palette::TYPE();
				};

				break;
			case Sprite::TYPE():
				filter = [] (const class Asset* asset) -> bool {
					return asset->type() != Image::TYPE();
				};

				break;
			case Map::TYPE():
				filter = [] (const class Asset* asset) -> bool {
					return asset->type() != Image::TYPE();
				};

				break;
			}

			const std::string assetStr = asset_;
			ImGui::SelectAssetPopupBox::ConfirmHandlerForSingleSelection confirm(
				[ws, project, assetStr, df] (const std::string &selected) -> void {
					OPERATIONS_AUTO_CLOSE_POPUP(ws)

					if (selected.empty()) {
						df.reject();

						return;
					}

					LockGuard<RecursiveMutex>::UniquePtr acquired;
					Project* prj = project->acquire(acquired);
					if (!prj) {
						df.reject();

						return;
					}

					Asset* asset = prj->get(assetStr.c_str());
					if (!asset) {
						df.reject();

						return;
					}

					Asset* refAsset = prj->get(selected.c_str());
					if (!refAsset) {
						df.reject();

						return;
					}

					asset->ref(selected);
					asset->reload(Asset::EDITING);
					asset->dirty(true);

					df.resolve(true, selected);
				},
				nullptr
			);
			ImGui::SelectAssetPopupBox::CancelHandler cancel(
				[ws, df] (void) -> void {
					OPERATIONS_AUTO_CLOSE_POPUP(ws)

					df.reject();
				},
				nullptr
			);
			ws->popupBox(
				ImGui::PopupBox::Ptr(
					new ImGui::SelectAssetPopupBox(
						project,
						BITTY_NAME,
						ws->theme()->dialogItem_SelectAsset(),
						"",
						extra,
						ws->theme()->sliceDirectory()->pointer(rnd), ws->theme()->sliceDirectory_Open()->pointer(rnd), ws->theme()->sliceFile()->pointer(rnd), ws->theme()->style()->iconColor,
						filter,
						confirm, cancel,
						ws->theme()->generic_Ok().c_str(), ws->theme()->generic_Cancel().c_str()
					)
				)
			);
		}
	);
}

promise::Defer Operations::editResolveRefs(class Renderer*, Workspace* ws, const class Project* project, const char* asset_) {
	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			LockGuard<RecursiveMutex>::UniquePtr acquired;
			Project* prj = project->acquire(acquired);
			if (!prj) {
				df.reject();

				return;
			}

			std::list<Asset*> referencing;
			prj->foreach(
				[&] (Asset* &asset, Asset::List::Index) -> void {
					Asset::States* states = asset->states();
					if (states->activity() == Asset::States::CLOSED)
						return;

					if (asset->referencing() == 0)
						return;

					referencing.insert(
						std::upper_bound(
							referencing.begin(), referencing.end(),
							asset,
							[] (const Asset* left, const Asset* right) -> bool {
								return Asset::compare(left, right) > 0;
							}
						),
						asset
					);
				}
			);

			Text::Array referenced = { asset_ };
			std::list<Asset*> invalidated;
			for (Asset* asset : referencing) {
				const Entry &entry = asset->entry();

				if (std::find(referenced.begin(), referenced.end(), asset->ref()) != referenced.end()) {
					referenced.push_back(entry.name());
					invalidated.insert(
						std::upper_bound(
							invalidated.begin(), invalidated.end(),
							asset,
							[] (const Asset* left, const Asset* right) -> bool {
								return Asset::compare(left, right) > 0;
							}
						),
						asset
					);
				}
			}

			int count = 0;
			for (Asset* asset : invalidated) {
				Asset::States* states = asset->states();

				if (asset->dirty()) {
					const std::string msg = Text::cformat("Ignored validating: \"%s\".", asset->entry().c_str());
					ws->warn(msg.c_str());

					return;
				}

				const Asset::States::Activity activity = states->activity();
				states->deactivate();
				states->deselect();

				asset->finish((Asset::Usages)(Asset::RUNNING | Asset::EDITING), false);
				prj->cleanup((Asset::Usages)(Asset::RUNNING | Asset::EDITING));
				asset->prepare(Asset::RUNNING, false);
				if (!asset->object(Asset::RUNNING))
					continue;

				asset->prepare(Asset::EDITING, false);
				states->activate(activity);

				++count;
			}

			df.resolve(count);
		}
	);
}

promise::Defer Operations::editSwitchAsset(class Renderer*, Workspace* ws, const class Project* project) {
	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			OPERATIONS_AUTO_CLOSE_POPUP(ws)

			LockGuard<RecursiveMutex>::UniquePtr acquired;
			Project* prj = project->acquire(acquired);
			if (!prj) {
				df.reject();

				return;
			}

			int opened = 0;
			prj->foreach(
				[&] (Asset* &asset, Asset::List::Index) -> void {
					Asset::States* states = asset->states();
					const Asset::States::Activity activity = states->activity();
					if (activity == Asset::States::EDITABLE || activity == Asset::States::INSPECTABLE)
						++opened;
				}
			);
			if (opened <= 1) {
				df.reject();

				return;
			}

			ImGui::SwitchAssetPopupBox::ConfirmHandler confirm(
				[ws, project, df] (const char* selected) -> void {
					OPERATIONS_AUTO_CLOSE_POPUP(ws)

					if (!selected) {
						df.reject();

						return;
					}

					LockGuard<RecursiveMutex>::UniquePtr acquired;
					Project* prj = project->acquire(acquired);
					if (!prj) {
						df.reject();

						return;
					}

					Asset* asset = prj->get(selected);
					if (!asset) {
						df.reject();

						return;
					}

					Asset::States* states = asset->states();
					states->focus();

					df.resolve(true, selected);
				},
				nullptr
			);
			ImGui::SwitchAssetPopupBox::CancelHandler cancel(
				[ws, df] (void) -> void {
					OPERATIONS_AUTO_CLOSE_POPUP(ws)

					df.reject();
				},
				nullptr
			);
			ws->popupBox(
				ImGui::PopupBox::Ptr(
					new ImGui::SwitchAssetPopupBox(
						project,
						BITTY_NAME,
						confirm, cancel
					)
				)
			);
		}
	);
}

promise::Defer Operations::projectAddAsset(class Renderer* rnd, Workspace* ws, const class Project* project, Asset::List::Index index) {
	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			std::string path = OPERATIONS_ASSET_DEFAULT_NAME;
			unsigned type = 0;
			do {
				LockGuard<RecursiveMutex>::UniquePtr acquired;
				Project* prj = project->acquire(acquired);
				if (!prj)
					break;

				Asset* asset = prj->get(index);
				if (!asset)
					break;

				type = asset->type();
				path = asset->entry().name();
				Path::split(path, nullptr, nullptr, &path);
				path = Path::combine(path.c_str(), OPERATIONS_ASSET_DEFAULT_NAME);
			} while (false);

			ImGui::AddAssetPopupBox::ConfirmHandler confirm(
				[ws, project, df] (unsigned type, const char* ref, const Math::Vec2i* size, const Math::Vec2i* tileSize, const char* name) -> void {
					OPERATIONS_AUTO_CLOSE_POPUP(ws)

					if (type == 0) {
						df.reject();

						return;
					}

					bool valid = Path::isValid(name);
					if (valid) {
						const Text::Array parts = Text::split(name, "/");
						valid &= std::find_if(
							parts.begin(), parts.end(),
							[] (const std::string &str) -> bool {
								return std::find_if(
									str.begin(), str.end(),
									[] (char ch) -> bool {
										return ch != '.';
									}
								) == str.end();
							}
						) == parts.end();
					}
					if (!valid) {
						df.reject();

						ws->messagePopupBox(
							ws->theme()->dialogPrompt_InvalidName(),
							nullptr,
							nullptr,
							nullptr
						);

						return;
					}

					if (!ref && (type == Sprite::TYPE() || type == Map::TYPE())) {
						ImGui::PopupBox::Ptr prev = ws->popupBox();
						ImGui::MessagePopupBox::ConfirmHandler confirm(
							[ws, prev] (void) -> void {
								ws->popupBox(prev);
							},
							nullptr
						);
						ws->messagePopupBox(
							ws->theme()->dialogPrompt_InvalidRef(),
							confirm,
							nullptr,
							nullptr
						);

						return;
					}

					LockGuard<RecursiveMutex>::UniquePtr acquired;
					Project* prj = project->acquire(acquired);
					if (!prj) {
						df.reject();

						return;
					}

					Asset* exists = prj->get(name);
					if (exists) {
						df.reject();

						ws->messagePopupBox(
							ws->theme()->dialogPrompt_AlreadyExists(),
							nullptr,
							nullptr,
							nullptr
						);

						return;
					}

					unsigned finalType = type;
					IDictionary::Ptr options = nullptr;
					switch (type) {
					case Image::TYPE():
						if (size) {
							options = IDictionary::Ptr(Dictionary::create());
							options->set("width", size->x);
							options->set("height", size->y);
							options->set(ASSET_REF_NAME, ref ? ref : "");
						}

						break;
					case Sprite::TYPE():
						if (size) {
							options = IDictionary::Ptr(Dictionary::create());
							options->set("width", size->x);
							options->set("height", size->y);
						}

						break;
					case Map::TYPE():
						if (size && tileSize) {
							options = IDictionary::Ptr(Dictionary::create());
							options->set("width", size->x);
							options->set("height", size->y);
							options->set(ASSET_REF_NAME, ref ? ref : "");

							IList::Ptr lstCount(List::create());
							lstCount->add(tileSize->x);
							lstCount->add(tileSize->y);

							IDictionary::Ptr dictTiles(Dictionary::create());
							dictTiles->set("count", (Object::Ptr)lstCount);

							options->set("tiles", (Object::Ptr)dictTiles);
						}

						break;
					case Code::TYPE(): // Fall through.
					case Palette::TYPE(): // Fall through.
					case Json::TYPE(): // Fall through.
					case Text::TYPE(): // Do nothing.
						break;
					default:
						// Customized asset type.
						finalType = Bytes::TYPE(); // Use `Bytes`.

						break;
					}

					Asset* asset = prj->factory().create(prj);
					Object::Ptr obj = Asset::fromBlank(Asset::EDITING, project, finalType, options);
					asset->link(obj, name);
					if (ref)
						asset->ref(ref);

					Asset::States* states = asset->states();
					states->activate(Asset::States::EDITABLE);
					states->focus();
					asset->dirty(true);

					prj->add(asset);

					df.resolve(true);
				},
				nullptr
			);
			ImGui::AddAssetPopupBox::CancelHandler cancel(
				[ws, df] (void) -> void {
					OPERATIONS_AUTO_CLOSE_POPUP(ws)

					df.reject();
				},
				nullptr
			);
			ImGui::AddAssetPopupBox::Types types = {
				Code::TYPE(),
				Sprite::TYPE(),
				Map::TYPE(),
				Image::TYPE(),
				Palette::TYPE(),
				Json::TYPE(),
				Text::TYPE()
			};
			ImGui::AddAssetPopupBox::TypeNames typeNames = {
				"Code",
				"Sprite",
				"Map",
				"Image",
				"Palette",
				"JSON",
				"Text"
			};
			ImGui::AddAssetPopupBox::TypeExtensions typeExtensions = {
				"",
				BITTY_SPRITE_EXT,
				BITTY_MAP_EXT,
				"",
				BITTY_PALETTE_EXT,
				BITTY_JSON_EXT,
				BITTY_TEXT_EXT
			};
			ImGui::AddAssetPopupBox::Vec2s defaultSizes = { // Default sizes.
				Math::Vec2i(),
				Math::Vec2i(BITTY_SPRITE_DEFAULT_WIDTH, BITTY_SPRITE_DEFAULT_HEIGHT),
				Math::Vec2i(BITTY_MAP_DEFAULT_WIDTH, BITTY_MAP_DEFAULT_HEIGHT),
				Math::Vec2i(BITTY_IMAGE_DEFAULT_WIDTH, BITTY_IMAGE_DEFAULT_HEIGHT),
				Math::Vec2i(),
				Math::Vec2i(),
				Math::Vec2i()
			};
			ImGui::AddAssetPopupBox::Vec2s maxSizes = { // Max sizes.
				Math::Vec2i(),
				Math::Vec2i(BITTY_SPRITE_MAX_WIDTH, BITTY_SPRITE_MAX_HEIGHT),
				Math::Vec2i(BITTY_MAP_MAX_WIDTH, BITTY_MAP_MAX_HEIGHT),
				Math::Vec2i(BITTY_IMAGE_MAX_WIDTH, BITTY_IMAGE_MAX_HEIGHT),
				Math::Vec2i(),
				Math::Vec2i(),
				Math::Vec2i()
			};
			ImGui::AddAssetPopupBox::Vec2s defaultSizes2 = { // 2nd default sizes.
				Math::Vec2i(),
				Math::Vec2i(),
				Math::Vec2i(BITTY_MAP_TILE_DEFAULT_SIZE, BITTY_MAP_TILE_DEFAULT_SIZE),
				Math::Vec2i(),
				Math::Vec2i(),
				Math::Vec2i(),
				Math::Vec2i()
			};
			ImGui::AddAssetPopupBox::Vec2s maxSizes2 = { // 2nd max sizes.
				Math::Vec2i(),
				Math::Vec2i(),
				Math::Vec2i(BITTY_MAP_TILE_DEFAULT_SIZE * 4, BITTY_MAP_TILE_DEFAULT_SIZE * 4),
				Math::Vec2i(),
				Math::Vec2i(),
				Math::Vec2i(),
				Math::Vec2i()
			};
			operationsAppendCustomAssetType(rnd, ws, project, types, typeNames, typeExtensions, defaultSizes, maxSizes, defaultSizes2, maxSizes2);
			int typeIndex = 0;
			for (int i = 0; i < (int)types.size(); ++i) {
				if (types[i] == type) {
					typeIndex = i;

					break;
				}
			}
			ws->popupBox(
				ImGui::PopupBox::Ptr(
					new ImGui::AddAssetPopupBox(
						project,
						BITTY_NAME,
						ws->theme()->dialogItem_Type(), types, typeNames, typeExtensions, typeIndex,
						ws->theme()->dialogItem_Size(), defaultSizes, maxSizes,
						ws->theme()->dialogItem_TileSize(), defaultSizes2, maxSizes2,
						ws->theme()->dialogItem_InputAssetName(), path,
						ws->theme()->tooltipProject_OptionalSelectAPaletteNoneForTrueColor(), ws->theme()->tooltipProject_SelectAnImage(), ws->theme()->tooltipProject_DragOrDoubleClickToChange(), ws->theme()->tooltipProject_InputDirSubFileToCreateInDirectory(),
						ws->theme()->generic_None(), ws->theme()->dialogItem_Ref(), ws->theme()->dialogItem_Palette(),
						confirm,
						cancel,
						ws->theme()->generic_Ok().c_str(), ws->theme()->generic_Cancel().c_str()
					)
				)
			);
		}
	);
}

promise::Defer Operations::projectRemoveAsset(class Renderer* rnd, Workspace* ws, const class Project* project, Executable* exec, Asset::List::Index index) {
	std::string name;

	const promise::Defer proc = promise::newPromise(
		[&] (promise::Defer df) -> void {
			std::string msg = ws->theme()->dialogAsk_RemoveAsset();
			std::string tips;
			do {
				LockGuard<RecursiveMutex>::UniquePtr acquired;
				Project* prj = project->acquire(acquired);
				if (!prj)
					break;

				if (prj->archived())
					tips = ws->theme()->dialogPrompt_NotUndoable();

				Asset* asset = prj->get(index);
				if (!asset)
					break;

				name = asset->entry().name();

				std::string inner;
				inner += ":\n  \"";
				inner += asset->entry().name();
				inner += '"';
				inner += '?';
				msg.pop_back();
				msg += inner;
			} while (false);
			if (!tips.empty()) {
				msg += '\n';
				msg += tips;
			}

			ImGui::MessagePopupBox::ConfirmHandler confirm(
				[ws, project, exec, index, df] (void) -> void {
					OPERATIONS_AUTO_CLOSE_POPUP(ws)

					LockGuard<RecursiveMutex>::UniquePtr acquired;
					Project* prj = project->acquire(acquired);
					if (!prj) {
						df.reject();

						return;
					}

					Asset* asset = prj->get(index);
					if (asset)
						exec->clearBreakpoints(asset->entry().c_str());

					asset->remove();
					prj->remove(index);
					prj->dirty(true);

					df.resolve(true);
				},
				nullptr
			);
			ImGui::MessagePopupBox::DenyHandler cancel(
				[ws, df] (void) -> void {
					OPERATIONS_AUTO_CLOSE_POPUP(ws)

					df.reject();
				},
				nullptr
			);
			ws->messagePopupBox(
				msg,
				confirm,
				cancel,
				nullptr
			);
		}
	);

	return proc
		.then(
			[rnd, ws, project, name] (void) -> promise::Defer {
				return editResolveRefs(rnd, ws, project, name.c_str());
			}
		);
}

promise::Defer Operations::projectRenameAsset(class Renderer* rnd, Workspace* ws, const class Project* project, Asset::List::Index index) {
	std::string name;

	const promise::Defer proc = promise::newPromise(
		[&] (promise::Defer df) -> void {
			std::string default_;
			std::string ext;
			do {
				LockGuard<RecursiveMutex>::UniquePtr acquired;
				Project* prj = project->acquire(acquired);
				if (!prj)
					break;

				Asset* asset = prj->get(index);
				if (!asset)
					break;

				name = asset->entry().name();

				default_ = asset->entry().name();
				Path::split(default_, nullptr, &ext, nullptr);
				if (!ext.empty())
					default_ = default_.substr(0, default_.length() - ext.length() - 1);
			} while (false);

			ImGui::InputPopupBox::ConfirmHandler confirm(
				[ws, project, index, ext, df] (const char* name) -> void {
					OPERATIONS_AUTO_CLOSE_POPUP(ws)

					std::string nameExt = name;
					nameExt += '.';
					nameExt += ext;
					Path::uniform(nameExt);

					LockGuard<RecursiveMutex>::UniquePtr acquired;
					Project* prj = project->acquire(acquired);
					if (!prj) {
						df.reject();

						return;
					}

					Asset* exists = prj->get(nameExt.c_str());
					if (exists) {
						df.reject();

						ws->messagePopupBox(
							ws->theme()->dialogPrompt_AlreadyExists(),
							nullptr,
							nullptr,
							nullptr
						);

						return;
					}

					Asset* asset = prj->get(index);
					if (!asset) {
						df.reject();

						return;
					}

					if (!asset->rename(nameExt.c_str())) {
						df.reject();

						return;
					}

					Asset::States* states = asset->states();
					states->focus();

					prj->sort();
					prj->dirty(true);

					df.resolve(true);
				},
				nullptr
			);
			ImGui::InputPopupBox::CancelHandler cancel(
				[ws, df] (void) -> void {
					OPERATIONS_AUTO_CLOSE_POPUP(ws)

					df.reject();
				},
				nullptr
			);
			ws->inputPopupBox(
				ws->theme()->dialogItem_InputAssetName(),
				default_, ImGuiInputTextFlags_None,
				confirm,
				cancel
			);
		}
	);

	return proc
		.then(
			[rnd, ws, project, name] (void) -> promise::Defer {
				return editResolveRefs(rnd, ws, project, name.c_str());
			}
		);
}

promise::Defer Operations::projectAddFile(class Renderer* rnd, Workspace* ws, const class Project* project, Asset::List::Index index) {
	ImGui::AddFilePopupBox::Browser browser = [ws, project] (const std::string &old) -> std::string {
		std::string defaultPath;
		if (!old.empty()) {
			FileInfo::Ptr fileInfo = FileInfo::make(old.c_str());
			DirectoryInfo::Ptr dirInfo = fileInfo->parent();
			if (dirInfo->exists())
				defaultPath = dirInfo->fullPath();
		}

		std::string lang = BITTY_LUA_EXT;
		do {
			LockGuard<RecursiveMutex>::UniquePtr acquired;
			Project* prj = project->acquire(acquired);
			if (!prj)
				break;

			lang = prj->language();
		} while (false);
		Text::Array filters = OPERATIONS_ASSET_FILE_FILTER;
		for (std::string &filter : filters)
			filter = Text::replace(filter, OPERATIONS_CODE_PLACEHOLDER, lang);
		pfd::open_file open(
			ws->theme()->generic_AddFile(),
			defaultPath.c_str(),
			filters
		);
		if (open.result().empty() || open.result().front().empty())
			return "";

		std::string path = open.result().front();
		Path::uniform(path);

		return path;
	};

	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			const std::string path = browser("");
			if (path.empty()) {
				df.reject();

				return;
			}
			std::string name;
			Path::split(path, &name, nullptr, nullptr);

			do {
				LockGuard<RecursiveMutex>::UniquePtr acquired;
				Project* prj = project->acquire(acquired);
				if (!prj)
					break;

				Asset* asset = prj->get(index);
				if (!asset)
					break;

				std::string parent;
				Path::split(asset->entry().name(), nullptr, nullptr, &parent);
				name = Path::combine(parent.c_str(), name.c_str());
			} while (false);

			ImGui::AddFilePopupBox::ConfirmHandler confirm(
				[rnd, ws, project, df] (const char* newPath, const char* newName) -> void {
					OPERATIONS_AUTO_CLOSE_POPUP(ws)

					std::string ext;
					Path::split(newPath, nullptr, &ext, nullptr);
					const std::string dotExt = "." + ext;
					std::string nameExt = newName;
					if (!Text::endsWith(nameExt, dotExt, true))
						nameExt += dotExt;
					const unsigned type = Asset::typeOf(ext, true);
					switch (type) {
					case Palette::TYPE(): // Fall through.
					case Image::TYPE(): // Fall through.
					case Sprite::TYPE(): // Fall through.
					case Map::TYPE(): // Fall through.
					case Sound::TYPE(): // Fall through.
					case Font::TYPE(): // Fall through.
					case Code::TYPE(): // Fall through.
					case Json::TYPE(): // Fall through.
					case Text::TYPE(): // Fall through.
					case Bytes::TYPE(): // Do nothing.
						break;
					default:
						df.reject();

						ws->messagePopupBox(
							ws->theme()->dialogPrompt_UnknownType(),
							nullptr,
							nullptr,
							nullptr
						);

						return;
					}

					LockGuard<RecursiveMutex>::UniquePtr acquired;
					Project* prj = project->acquire(acquired);
					if (!prj) {
						df.reject();

						return;
					}

					if (Path::isParentOf(prj->path().c_str(), newPath)) {
						df.reject();

						ws->messagePopupBox(
							ws->theme()->dialogPrompt_CannotReadFromCurrentProject(),
							nullptr,
							nullptr,
							nullptr
						);

						return;
					}

					Asset* exists = prj->get(nameExt.c_str());
					if (exists) {
						df.reject();

						ws->messagePopupBox(
							ws->theme()->dialogPrompt_AlreadyExists(),
							nullptr,
							nullptr,
							nullptr
						);

						return;
					}

					Asset* asset = prj->factory().create(prj);

					bool ok = false;
					File::Ptr file(File::create());
					if (file->open(newPath, Stream::READ)) {
						Bytes::Ptr buf(Bytes::create());
						file->readBytes(buf.get());
						buf->poke(0);
						ok = asset->link(type, buf.get(), nameExt.c_str(), nullptr);
						file->close();
					}

					if (!ok) {
						prj->factory().destroy(asset);

						df.reject();

						ws->messagePopupBox(
							ws->theme()->dialogPrompt_InvalidAsset(),
							nullptr,
							nullptr,
							nullptr
						);

						return;
					}

					Asset::States* states = asset->states();
					states->activate(Asset::States::EDITABLE);
					states->focus();

					prj->add(asset);
					prj->dirty(true);

					df.resolve(true);
				},
				nullptr
			);
			ImGui::AddFilePopupBox::CancelHandler cancel(
				[ws, df] (void) -> void {
					OPERATIONS_AUTO_CLOSE_POPUP(ws)

					df.reject();
				},
				nullptr
			);
			ws->popupBox(
				ImGui::PopupBox::Ptr(
					new ImGui::AddFilePopupBox(
						BITTY_NAME,
						ws->theme()->dialogItem_Path(), path,
						ws->theme()->generic_Browse(), browser,
						ws->theme()->dialogItem_InputAssetName(), name, ws->theme()->tooltipProject_InputDirSubFileToCreateInDirectory(),
						confirm, cancel,
						ws->theme()->generic_Ok().c_str(), ws->theme()->generic_Cancel().c_str()
					)
				)
			);
		}
	);
}

promise::Defer Operations::projectImport(class Renderer* rnd, Workspace* ws, const class Project* project, const char* path_, bool excludeInfoAndMain) {
	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			std::string path;
			if (path_) {
				path = path_;
			} else {
				pfd::open_file open(
					ws->theme()->generic_Open(),
					"",
					OPERATIONS_BITTY_FULL_FILE_FILTER
				);
				if (open.result().empty() || open.result().front().empty()) {
					df.reject();

					return;
				}
				path = open.result().front();
				Path::uniform(path);
			}

			LockGuard<RecursiveMutex>::UniquePtr acquired;
			Project* prj = project->acquire(acquired);
			if (!prj) {
				df.reject();

				return;
			}

			if (Path::isParentOf(prj->path().c_str(), path.c_str())) {
				df.reject();

				ws->messagePopupBox(
					ws->theme()->dialogPrompt_CannotReadFromCurrentProject(),
					nullptr,
					nullptr,
					nullptr
				);

				return;
			}

			std::shared_ptr<Project> newPrj(new Project());
			newPrj->loader(prj->loader());
			newPrj->factory(prj->factory());
			newPrj->open(rnd);
			if (!newPrj->load(path.c_str())) {
				newPrj->close();
				newPrj->loader(nullptr);

				df.reject();

				return;
			}
			if (excludeInfoAndMain) {
				Asset* infoAsset = newPrj->info();
				if (infoAsset)
					newPrj->remove(infoAsset);
				Asset* mainAsset = newPrj->main();
				if (mainAsset)
					newPrj->remove(mainAsset);
			}

			Text::Set entries;
			newPrj->foreach(
				[&] (Asset* &asset, Asset::List::Index) -> void {
					entries.insert(asset->entry().name());
				}
			);

			Text::Set conflictions;
			for (const std::string &ent : entries) {
				Asset* exists = prj->get(ent.c_str());
				if (exists)
					conflictions.insert(ent);
			}
			std::string extra;
			if (!conflictions.empty()) {
				extra += ws->theme()->dialogItem_ConflictAssets();
				extra += "\n";
				size_t i = 0;
				for (Text::Set::iterator it = conflictions.begin(); it != conflictions.end(); ++it, ++i) {
					const std::string &con = *it;
					extra += "  \"";
					extra += con;
					extra += "\"";
					if (i >= 3) {
						const std::string count = Text::toString((int)conflictions.size());
						const std::string total = Text::replace(ws->theme()->dialogItem_TotalCount(), OPERATIONS_COUNT_PLACEHOLDER, count);
						extra += "\n  ";
						extra += total;
						extra += "\n";

						break;
					}
					if (i < conflictions.size() - 1)
						extra += "\n";
				}
			}

			ImGui::SelectAssetPopupBox::ConfirmHandlerForMultipleSelection confirm(
				[rnd, ws, project, newPrj, path, df] (const Text::Set &selected) -> void {
					OPERATIONS_AUTO_CLOSE_POPUP(ws)

					LockGuard<RecursiveMutex>::UniquePtr acquired;
					Project* prj = project->acquire(acquired);
					if (!prj) {
						newPrj->close();
						newPrj->loader(nullptr);

						df.reject();

						return;
					}

					auto next = [ws, project, newPrj, path, selected] (promise::Defer df) -> void {
						LockGuard<RecursiveMutex>::UniquePtr acquired;
						Project* prj = project->acquire(acquired);
						if (!prj) {
							df.reject();

							return;
						}

						Asset* infoAsset = prj->info();
						Asset* mainAsset = prj->main();
						for (const std::string &ent : selected) {
							if (infoAsset && infoAsset->entry().name() == ent) {
								const std::string msg = "Ignored meta info: \"" + ent + "\".";
								ws->warn(msg.c_str());

								continue; // Ignore meta info.
							}
							if (mainAsset && mainAsset->entry().name() == ent) {
								const std::string msg = "Ignored entry code: \"" + ent + "\".";
								ws->warn(msg.c_str());

								continue; // Ignore entry code.
							}

							Asset* newAsset = newPrj->get(ent.c_str());
							if (!newAsset)
								continue;
							Bytes::Ptr buf(Bytes::create());
							if (!newAsset->load(Asset::EDITING))
								continue;
							if (!newAsset->save(Asset::EDITING, buf.get()))
								continue;
							buf->poke(0);

							Asset* exists = prj->get(ent.c_str());
							if (exists) {
								const std::string msg = "Ignored confliction: \"" + ent + "\".";
								ws->warn(msg.c_str());

								continue; // Ignore confliction.
							}

							Asset* asset = prj->factory().create(prj);
							asset->link(newAsset->type(), buf.get(), ent.c_str(), nullptr);
							prj->add(asset);
						}

						newPrj->close();
						newPrj->loader(nullptr);

						prj->dirty(true);

						std::string msg = "Imported from: \"";
						msg += path;
						msg += "\".";
						ws->print(msg.c_str());

						df.resolve(true);
					};

					popupWait(rnd, ws, ws->theme()->dialogPrompt_Reading().c_str())
						.then(
							[next, df] (void) -> promise::Defer {
								return promise::newPromise(std::bind(next, df));
							}
						);
				},
				nullptr
			);
			ImGui::SelectAssetPopupBox::CancelHandler cancel(
				[ws, newPrj, df] (void) -> void {
					OPERATIONS_AUTO_CLOSE_POPUP(ws)

					newPrj->close();
					newPrj->loader(nullptr);

					df.reject();
				},
				nullptr
			);
			ws->popupBox(
				ImGui::PopupBox::Ptr(
					new ImGui::SelectAssetPopupBox(
						newPrj.get(),
						BITTY_NAME,
						ws->theme()->dialogItem_SelectAssets(),
						entries,
						extra,
						ws->theme()->generic_All(),
						ws->theme()->sliceDirectory()->pointer(rnd), ws->theme()->sliceDirectory_Open()->pointer(rnd), ws->theme()->style()->iconColor,
						nullptr,
						confirm, cancel,
						ws->theme()->generic_Ok().c_str(), ws->theme()->generic_Cancel().c_str()
					)
				)
			);
		}
	);
}

promise::Defer Operations::projectExport(class Renderer* rnd, Workspace* ws, const class Project* project) {
	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			LockGuard<RecursiveMutex>::UniquePtr acquired;
			Project* prj = project->acquire(acquired);
			if (!prj) {
				df.reject();

				return;
			}

			Text::Set entries;
			prj->foreach(
				[&] (Asset* &asset, Asset::List::Index) -> void {
					entries.insert(asset->entry().name());
				}
			);

			ImGui::SelectAssetPopupBox::ConfirmHandlerForMultipleSelection confirm(
				[rnd, ws, project, df] (const Text::Set &selected) -> void {
					OPERATIONS_AUTO_CLOSE_POPUP(ws)

					LockGuard<RecursiveMutex>::UniquePtr acquired;
					Project* prj = project->acquire(acquired);
					if (!prj) {
						df.reject();

						return;
					}

					unsigned preference = prj->preference();
					std::string path;
					if (path.empty()) {
						pfd::save_file save(
							ws->theme()->generic_Export(),
							"",
							OPERATIONS_BITTY_FULL_FILE_FILTER
						);
						path = save.result();
						Path::uniform(path);
						if (path.empty()) {
							df.reject();

							return;
						}
						std::string ext;
						Path::split(path, nullptr, &ext, nullptr);
						if (ext.empty())
							path += "." BITTY_PROJECT_EXT;
						else if (ext == BITTY_TEXT_EXT)
							preference = Archive::TXT;
						else if (ext == BITTY_ZIP_EXT)
							preference = Archive::ZIP;
					}
					if (Path::isParentOf(prj->path().c_str(), path.c_str())) {
						df.reject();

						ws->messagePopupBox(
							ws->theme()->dialogPrompt_CannotWriteToCurrentProject(),
							nullptr,
							nullptr,
							nullptr
						);

						return;
					}
					if (!ws->canSaveTo(path.c_str())) {
						df.reject();

						ws->messagePopupBox(
							ws->theme()->dialogPrompt_CannotSaveToReadonlyLocations(),
							nullptr,
							nullptr,
							nullptr
						);

						return;
					}

					auto next = [rnd, ws, project, preference, path, selected] (promise::Defer df) -> void {
						LockGuard<RecursiveMutex>::UniquePtr acquired;
						Project* prj = project->acquire(acquired);
						if (!prj) {
							df.reject();

							return;
						}

						const bool overwrite = Path::existsFile(path.c_str());
						if (overwrite)
							Path::removeFile(path.c_str(), true);

						std::shared_ptr<Project> newPrj(new Project());
						newPrj->loader(prj->loader());
						newPrj->factory(prj->factory());
						newPrj->preference(preference);
						newPrj->open(rnd);
						newPrj->path(path);
						for (const std::string &ent : selected) {
							Asset* asset = prj->get(ent.c_str());
							if (!asset)
								continue;
							Editable* editor = asset->editor();
							if (editor)
								editor->flush();
							Bytes::Ptr buf(Bytes::create());
							bool saved = asset->toBytes(buf.get());
							if (!saved)
								saved = asset->object(Asset::EDITING) && asset->save(Asset::EDITING, buf.get());
							if (!saved)
								saved = asset->object(Asset::RUNNING) && asset->save(Asset::RUNNING, buf.get());
							if (!saved)
								continue;
							buf->poke(0);

							Asset* newAsset = newPrj->factory().create(newPrj.get());
							newAsset->link(asset->type(), buf.get(), ent.c_str(), nullptr);
							newPrj->add(newAsset);
						}
						newPrj->dirty(true);
						if (!newPrj->save(path.c_str(), false, std::bind(operationsHandleError, ws, std::placeholders::_1))) {
							newPrj->close();
							newPrj->loader(nullptr);

							df.reject();

							return;
						}

						newPrj->close();
						newPrj->loader(nullptr);

						std::string msg = "Exported to: \"";
						msg += path;
						if (overwrite)
							msg += "\" by overwriting.";
						else
							msg += "\".";
						ws->print(msg.c_str());

						df.resolve(true);
					};

					popupWait(rnd, ws, ws->theme()->dialogPrompt_Writing().c_str())
						.then(
							[next, df] (void) -> promise::Defer {
								return promise::newPromise(std::bind(next, df));
							}
						);
				},
				nullptr
			);
			ImGui::SelectAssetPopupBox::CancelHandler cancel(
				[ws, df] (void) -> void {
					OPERATIONS_AUTO_CLOSE_POPUP(ws)

					df.reject();
				},
				nullptr
			);
			ws->popupBox(
				ImGui::PopupBox::Ptr(
					new ImGui::SelectAssetPopupBox(
						project,
						BITTY_NAME,
						ws->theme()->dialogItem_SelectAssets(),
						entries,
						"",
						ws->theme()->generic_All(),
						ws->theme()->sliceDirectory()->pointer(rnd), ws->theme()->sliceDirectory_Open()->pointer(rnd), ws->theme()->style()->iconColor,
						nullptr,
						confirm, cancel,
						ws->theme()->generic_Ok().c_str(), ws->theme()->generic_Cancel().c_str()
					)
				)
			);
		}
	);
}

promise::Defer Operations::projectReload(class Renderer* rnd, Workspace* ws, const class Project* project, Executable* exec) {
	std::string path;

	LockGuard<RecursiveMutex>::UniquePtr acquired;
	Project* prj = project->acquire(acquired);
	if (!prj) {
		return promise::newPromise(
			[&] (promise::Defer df) -> void {
				df.reject();
			}
		);
	}

	path = prj->path();

	auto next = [rnd, ws, project, exec, path] (promise::Defer df) -> void {
		if (Path::existsDirectory(path.c_str())) {
			fileOpenDirectory(rnd, ws, project, exec, path.c_str())
				.then(
					[ws, df, path] (bool arg) -> void {
						std::string msg = "Reloaded project directory: \"";
						msg += path;
						msg += "\".";
						ws->print(msg.c_str());

						df.resolve(arg);
					}
				)
				.fail(
					[df] (void) -> void {
						df.reject();
					}
				);
		} else {
			fileOpenFile(rnd, ws, project, exec, path.c_str())
				.then(
					[ws, df, path] (bool arg) -> void {
						std::string msg = "Reloaded project file: \"";
						msg += path;
						msg += "\".";
						ws->print(msg.c_str());

						df.resolve(arg);
					}
				)
				.fail(
					[df] (void) -> void {
						df.reject();
					}
				);
		}
	};

	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			fileClose(rnd, ws, project, exec)
				.then(
					[next, df] (void) -> promise::Defer {
						return promise::newPromise(std::bind(next, df));
					}
				)
				.fail(
					[df] (void) -> void {
						df.reject();
					}
				);
		}
	);
}

promise::Defer Operations::projectBrowse(class Renderer*, Workspace*, const class Project* project) {
	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			LockGuard<RecursiveMutex>::UniquePtr acquired;
			Project* prj = project->acquire(acquired);
			if (!prj) {
				df.reject();

				return;
			}

			std::string path = prj->path();
			if (path.empty()) {
				df.reject();

				return;
			}

			if (Path::existsFile(path.c_str())) {
				FileInfo::Ptr fileInfo = FileInfo::make(path.c_str());
				path = Unicode::toOs(fileInfo->parentPath());
				Platform::browse(path.c_str());

				df.resolve(true);
			} else if (Path::existsDirectory(path.c_str())) {
				path = Unicode::toOs(path);
				Platform::browse(path.c_str());

				df.resolve(true);
			} else {
				df.reject();
			}
		}
	);
}

unsigned Operations::projectGetCustomAssetType(class Renderer*, Workspace* ws, const class Project*, const std::string &ext, unsigned type) {
	if (type != Bytes::TYPE())
		return type;

	for (Plugin* plugin : ws->plugins()) {
		if (!plugin->is(Plugin::Usages::COMPILER))
			continue;

		const Plugin::Schema &schema = plugin->schema();
		if (ext == schema.extension)
			return schema.type();
	}

	return type;
}

void Operations::projectRun(class Renderer* rnd, Workspace* ws, const class Project* project, Executable* exec, class Primitives*) {
	if (ws->settings()->consoleClearOnStart)
		ws->clear();

	ws->debugActiveFrameIndex(0);

	do {
		LockGuard<RecursiveMutex>::UniquePtr acquired;
		Project* prj = project->acquire(acquired);
		if (!prj)
			break;

		prj->cleanup(Asset::RUNNING);

		prj->foreach(
			[rnd, project] (Asset* &asset, Asset::List::Index) -> void {
				Editable* editor = asset->editor();
				if (!editor)
					return;

				editor->readonly(true);

				editor->played(rnd, project);

				editor->post(Editable::SET_PROGRAM_POINTER, (Variant::Int)-1);
			}
		);
	} while (false);

	if (!ws->executing())
		debugEnableBreakpoints(ws, project, exec, nullptr);

	if (exec)
		exec->run();
}

void Operations::projectStop(class Renderer* rnd, Workspace* ws, const class Project* project, Executable* exec, class Primitives* primitives) {
	if (ws->recorder()->recording())
		ws->recorder()->stop();

	ws->debugActiveFrameIndex(0);

	primitives->forbid();

	if (exec)
		exec->stop();

	primitives->reset();
	primitives->canvas(nullptr);

	do {
		LockGuard<RecursiveMutex>::UniquePtr acquired;
		Project* prj = project->acquire(acquired);
		if (!prj)
			break;

		prj->cleanup(Asset::RUNNING);

		prj->foreach(
			[rnd, project] (Asset* &asset, Asset::List::Index) -> void {
				Editable* editor = asset->editor();
				if (!editor)
					return;

				editor->stopped(rnd, project);

				editor->readonly(false);

				editor->post(Editable::SET_PROGRAM_POINTER, (Variant::Int)-1);
			}
		);
	} while (false);

	ws->debugStopping() = false;

	if (ws->canvasValidation() != Math::Vec2i(0, 0))
		ws->canvasValidation(Math::Vec2i(-1, -1));
	ws->canvasSize_(Math::Vec2i(BITTY_CANVAS_DEFAULT_WIDTH, BITTY_CANVAS_DEFAULT_HEIGHT));
	if (ws->canvasTexture())
		ws->canvasTexture(nullptr);
	ws->canvasHovering(false);
	ws->canvasInitialized(false);

	ws->currentState(exec ? exec->current() : Executable::READY);
}

void Operations::debugBreak(Workspace* ws, const class Project*, Executable* exec) {
	ws->debugActiveFrameIndex(0);

	if (exec)
		exec->pause();
}

void Operations::debugContinue(Workspace* ws, const class Project* project, Executable* exec) {
	ws->debugActiveFrameIndex(0);

	if (exec)
		exec->resume();

	do {
		LockGuard<RecursiveMutex>::UniquePtr acquired;
		Project* prj = project->acquire(acquired);
		if (!prj)
			break;

		prj->foreach(
			[] (Asset* &asset, Asset::List::Index) -> void {
				Editable* editor = asset->editor();
				if (!editor)
					return;

				editor->post(Editable::SET_PROGRAM_POINTER, (Variant::Int)-1);
			}
		);
	} while (false);
}

void Operations::debugStepOver(Workspace* ws, const class Project* project, Executable* exec) {
	ws->debugActiveFrameIndex(0);

	if (exec)
		exec->stepOver();

	do {
		LockGuard<RecursiveMutex>::UniquePtr acquired;
		Project* prj = project->acquire(acquired);
		if (!prj)
			break;

		prj->foreach(
			[] (Asset* &asset, Asset::List::Index) -> void {
				Editable* editor = asset->editor();
				if (!editor)
					return;

				editor->post(Editable::SET_PROGRAM_POINTER, (Variant::Int)-1);
			}
		);
	} while (false);
}

void Operations::debugStepInto(Workspace* ws, const class Project* project, Executable* exec) {
	ws->debugActiveFrameIndex(0);

	if (exec)
		exec->stepInto();

	do {
		LockGuard<RecursiveMutex>::UniquePtr acquired;
		Project* prj = project->acquire(acquired);
		if (!prj)
			break;

		prj->foreach(
			[] (Asset* &asset, Asset::List::Index) -> void {
				Editable* editor = asset->editor();
				if (!editor)
					return;

				editor->post(Editable::SET_PROGRAM_POINTER, (Variant::Int)-1);
			}
		);
	} while (false);
}

void Operations::debugStepOut(Workspace* ws, const class Project* project, Executable* exec) {
	ws->debugActiveFrameIndex(0);

	if (exec)
		exec->stepOut();

	do {
		LockGuard<RecursiveMutex>::UniquePtr acquired;
		Project* prj = project->acquire(acquired);
		if (!prj)
			break;

		prj->foreach(
			[] (Asset* &asset, Asset::List::Index) -> void {
				Editable* editor = asset->editor();
				if (!editor)
					return;

				editor->post(Editable::SET_PROGRAM_POINTER, (Variant::Int)-1);
			}
		);
	} while (false);
}

bool Operations::debugSetProgramPointer(Workspace* ws, const class Project* project, Executable*) {
	std::string src;
	int ln = -1;
	if (!ws->debugProgramPointer().getAndClear(src, ln)) // 1-based.
		return false;
	if (src.empty() || ln < 0)
		return false;

	LockGuard<RecursiveMutex>::UniquePtr acquired;
	Project* prj = project->acquire(acquired);
	if (!prj)
		return false;

	Asset* asset = prj->get(src.c_str());
	if (!asset)
		return false;

	asset->prepare(Asset::EDITING, false);

	Asset::States* states = asset->states();
	states->activate(Asset::States::INSPECTABLE);
	states->focus();

	Editable* editor = asset->editor();
	if (!editor)
		return false;

	editor->post(Editable::SET_CURSOR, (Variant::Int)ln - 1);

	editor->post(Editable::SET_PROGRAM_POINTER, (Variant::Int)ln - 1);

	return true;
}

void Operations::debugToggleBreakpoint(Workspace* ws, const class Project* project, Executable* exec, const char* src, int ln) {
	if (!exec)
		return;

	if (!src && ws->assetsEditingIndex() < 0)
		return;

	LockGuard<RecursiveMutex>::UniquePtr acquired;
	Project* prj = project->acquire(acquired);
	if (!prj)
		return;

	Asset* asset = nullptr;
	if (src)
		asset = prj->get(src);
	else
		asset = prj->get(ws->assetsEditingIndex());
	if (!asset)
		return;

	Editable* editor = asset->editor();
	if (!editor)
		return;

	if (ln < 0)
		ln = (int)(Variant::Int)editor->post(Editable::GET_CURSOR);
	const bool brk = !(bool)editor->post(Editable::GET_BREAKPOINT, (Variant::Int)ln);
	editor->post(Editable::SET_BREAKPOINT, (Variant::Int)ln, brk);

	exec->setBreakpoint(asset->entry().c_str(), ln + 1, brk); // 1-based.
}

void Operations::debugEnableBreakpoints(Workspace*, const class Project* project, Executable* exec, const char* src) {
	if (!exec)
		return;

	LockGuard<RecursiveMutex>::UniquePtr acquired;
	Project* prj = project->acquire(acquired);
	if (!prj)
		return;

	prj->foreach(
		[&] (Asset* &asset, Asset::List::Index) -> void {
			if (asset->type() != Code::TYPE())
				return;

			if (src && strcmp(asset->entry().c_str(), src) != 0)
				return;

			Editable* editor = asset->editor();
			if (!editor)
				return;

			Object::Ptr obj = (Object::Ptr)editor->post(Editable::GET_BREAKPOINTS);
			IList::Ptr lst = Object::as<IList::Ptr>(obj);

			exec->clearBreakpoints(asset->entry().c_str());
			const int n = lst->count();
			for (int i = 0; i < n; ++i) {
				const Variant::Int line = (Variant::Int)lst->at(i);
				exec->setBreakpoint(asset->entry().c_str(), line + 1, true); // 1-based.
				editor->post(Editable::SET_BREAKPOINT, line, true, true);
			}
		}
	);
}

void Operations::debugDisableBreakpoints(Workspace*, const class Project* project, Executable* exec, const char* src) {
	if (!exec)
		return;

	LockGuard<RecursiveMutex>::UniquePtr acquired;
	Project* prj = project->acquire(acquired);
	if (!prj)
		return;

	prj->foreach(
		[&] (Asset* &asset, Asset::List::Index) -> void {
			if (asset->type() != Code::TYPE())
				return;

			if (src && strcmp(asset->entry().c_str(), src) != 0)
				return;

			Editable* editor = asset->editor();
			if (!editor)
				return;

			Object::Ptr obj = (Object::Ptr)editor->post(Editable::GET_BREAKPOINTS);
			IList::Ptr lst = Object::as<IList::Ptr>(obj);

			exec->clearBreakpoints(asset->entry().c_str());
			const int n = lst->count();
			for (int i = 0; i < n; ++i) {
				const Variant::Int line = (Variant::Int)lst->at(i);
				editor->post(Editable::SET_BREAKPOINT, line, true, false);
			}
		}
	);
}

void Operations::debugClearBreakpoints(Workspace*, const class Project* project, Executable* exec, const char* src) {
	if (!exec)
		return;

	LockGuard<RecursiveMutex>::UniquePtr acquired;
	Project* prj = project->acquire(acquired);
	if (!prj)
		return;

	prj->foreach(
		[&] (Asset* &asset, Asset::List::Index) -> void {
			if (asset->type() != Code::TYPE())
				return;

			if (src && strcmp(asset->entry().c_str(), src) != 0)
				return;

			Editable* editor = asset->editor();
			if (!editor)
				return;

			editor->post(Editable::CLEAR_BREAKPOINTS);

			exec->clearBreakpoints(asset->entry().c_str());
		}
	);
}

promise::Defer Operations::pluginRunMenuItem(class Renderer*, Workspace* ws, const class Project*, Plugin* plugin) {
	return promise::newPromise(
		[&] (promise::Defer df) -> void {
			ImGui::WaitingPopupBox::TimeoutHandler timeout(
				[ws, df, plugin] (void) -> void {
					OPERATIONS_AUTO_CLOSE_POPUP(ws)

					plugin->run(Plugin::Functions::MENU);

					df.resolve(true);
				},
				nullptr
			);
			ws->waitingPopupBox(
				ws->theme()->dialogPrompt_Running(),
				timeout
			);
		}
	);
}

/* ===========================================================================} */
