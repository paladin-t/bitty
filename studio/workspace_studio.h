/*
** Bitty
**
** An itty bitty 2D game engine.
**
** Copyright (C) 2020 - 2026 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __WORKSPACE_STUDIO_H__
#define __WORKSPACE_STUDIO_H__

#include "../src/workspace.h"

/*
** {===========================================================================
** Studio workspace
**
** @note Specialized workspace for Pro version.
*/

namespace Bitty {

class WorkspaceStudio : public Workspace {
public:
	struct StudioSettings : public Settings {
		struct RecentTouched {
			enum Types {
				EXAMPLE,
				FILE,
				DIRECTORY
			};

			typedef std::vector<RecentTouched> Array;

			Types type = EXAMPLE;
			std::string path;

			RecentTouched();
			RecentTouched(Types type, const std::string &path);

			bool operator == (const RecentTouched &other) const;
		};

		int themeStyle = 0;

		bool recentListEnabled = true;
		RecentTouched::Array recentTouched;

		StudioSettings();
		StudioSettings(const StudioSettings &other) = delete;

		StudioSettings &operator = (const StudioSettings &other);

		bool operator != (const StudioSettings &other) const;
	};

protected:
	BITTY_PROPERTY_READONLY_PTR(const char, autorun)

protected:
	bool _opened = false;

	StudioSettings _settings;

	class ThemeStudio* _theme = nullptr;

	class Loader* _loader = nullptr;

	Text::Array _droppedFiles;

public:
	WorkspaceStudio();
	virtual ~WorkspaceStudio() override;

	virtual bool open(class Window* wnd, class Renderer* rnd, const class Project* project, Executable* exec, class Primitives* primitives, unsigned fps, const Text::Dictionary &options) override;
	virtual bool close(class Window* wnd, class Renderer* rnd, const class Project* project, Executable* exec) override;

	virtual const Settings* settings(void) const override;
	virtual Settings* settings(void) override;
	virtual class Theme* theme(void) const override;

	virtual bool prefer2XScaleForBigDisplay(void) const override;

	virtual void touchedFile(const char* path) override;
	virtual void touchedDirectory(const char* path) override;
	virtual void touchedExample(const char* path) override;

	virtual bool load(class Window* wnd, class Renderer* rnd, const class Project* project, class Primitives* primitives) override;
	virtual bool save(class Window* wnd, class Renderer* rnd, const class Project* project, class Primitives* primitives) override;

	virtual unsigned update(class Window* wnd, class Renderer* rnd, const class Project* project, Executable* exec, class Primitives* primitives, double delta, unsigned fps, bool alive, bool* indicated) override;

	virtual void require(Executable* exec) override;

	virtual void focusGained(class Window* wnd, class Renderer* rnd, const class Project* project, Executable* exec, class Primitives* primitives) override;
	virtual void focusLost(class Window* wnd, class Renderer* rnd, const class Project* project, Executable* exec, class Primitives* primitives) override;

	virtual void renderTargetsReset(class Window* wnd, class Renderer* rnd, const class Project* project, Executable* exec, class Primitives* primitives) override;

	virtual void fileDropped(class Window* wnd, class Renderer* rnd, const char* const path) override;
	virtual void dropBegan(class Window* wnd, class Renderer* rnd) override;
	virtual void dropEndded(class Window* wnd, class Renderer* rnd, const class Project* project, Executable* exec) override;

protected:
	virtual bool load(class Window* wnd, class Renderer* rnd, const class Project* project, class Primitives* primitives, const rapidjson::Document &doc) override;
	virtual bool save(class Window* wnd, class Renderer* rnd, const class Project* project, class Primitives* primitives, rapidjson::Document &doc) override;

	virtual void loadProject(class Window* wnd, class Renderer* rnd, const class Project* project, Executable* exec, class Primitives* primitives, const Text::Dictionary &options);
	virtual void unloadProject(const class Project* project, Executable* exec) override;

private:
	unsigned updateEditing(class Window* wnd, class Renderer* rnd, const class Project* project, Executable* exec, class Primitives* primitives, double delta, unsigned fps, bool alive, bool* indicated);
	unsigned updateBuilding(class Window* wnd, class Renderer* rnd, const class Project* project, Executable* exec, class Primitives* primitives, double delta, unsigned fps, bool alive, bool* indicated);

	void refresh(class Window* wnd, class Renderer* rnd, const class Project* project, Executable* exec, class Primitives* primitives);

	void addRecentTouched(StudioSettings::RecentTouched::Types type, const char* path);
	void openRecentTouched(class Window* wnd, class Renderer* rnd, const class Project* project, Executable* exec, class Primitives* primitives, StudioSettings::RecentTouched::Types type, int idx, const char* path);

	void shortcuts(class Window* wnd, class Renderer* rnd, const class Project* project, Executable* exec, class Primitives* primitives);
	void menu(class Window* wnd, class Renderer* rnd, const class Project* project, Executable* exec, class Primitives* primitives);

	void showPreferences(class Window* wnd, class Renderer* rnd, const class Project* project, class Primitives* primitives);
	void showAbout(class Window* wnd, class Renderer* rnd, class Primitives* primitives);
	void showPaused(class Window* wnd, class Renderer* rnd, const class Project* project, class Primitives* primitives);
};

}

/* ===========================================================================} */

#endif /* __WORKSPACE_STUDIO_H__ */
