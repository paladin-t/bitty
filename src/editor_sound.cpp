/*
** Bitty
**
** An itty bitty 2D game engine.
**
** Copyright (C) 2020 - 2026 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "analyser.h"
#include "audio.h"
#include "editing.h"
#include "editor_sound.h"
#include "theme.h"
#include "workspace.h"
#include <SDL.h>

/*
** {===========================================================================
** Sound editor
*/

namespace Bitty {

class EditorSoundImpl : public EditorSound {
private:
	bool _opened = false;

	std::string _name;
	Sound::Ptr _object = nullptr;
	bool _dirty = false;

	Analyser::Spec _spec;
	Editing::Tuner _cursor;
	struct {
		std::string text;
		bool filled = false;

		void clear(void) {
			text.clear();
			filled = false;
		}
	} _status;
	struct {
		float previous[ANALYSER_COLUMN_COUNT];
		float spectrum[ANALYSER_COLUMN_COUNT];
		bool analysed = false;
		float smooth = 0.2f;

		bool empty(void) const {
			return !analysed;
		}
		void clear(void) {
			analysed = false;
		}
	} _overlay;

public:
	EditorSoundImpl() {
	}
	virtual ~EditorSoundImpl() override {
		close(nullptr);
	}

	virtual unsigned type(void) const override {
		return TYPE();
	}

	virtual void open(const class Project* /* project */, const char* name, Object::Ptr obj, const char* /* ref */) override {
		if (_opened)
			return;
		_opened = true;

		_name = name;

		_object = Object::as<Sound::Ptr>(obj);

		_spec = Analyser::deviceSpec();
		if (_object) {
			_cursor.length = _object->length();
		}

		fprintf(stdout, "Sound editor opened: \"%s\".\n", _name.c_str());
	}
	virtual void close(const class Project* /* project */) override {
		if (!_opened)
			return;
		_opened = false;

		fprintf(stdout, "Sound editor closed: \"%s\".\n", _name.c_str());

		_status.clear();

		if (_object) {
			_object->stop();
			_object = nullptr;
		}
		_name.clear();
	}

	virtual void flush(void) const override {
		// Do nothing.
	}

	virtual bool readonly(void) const override {
		return true;
	}
	virtual void readonly(bool) override {
		// Do nothing.
	}

	virtual bool hasUnsavedChanges(void) const override {
		return _dirty;
	}
	virtual void markChangesSaved(const class Project* /* project */) override {
		_dirty = false;
	}

	virtual void copy(void) override {
		// Do nothing.
	}
	virtual void cut(void) override {
		// Do nothing.
	}
	virtual bool pastable(void) const override {
		return false;
	}
	virtual void paste(void) override {
		// Do nothing.
	}
	virtual void del(void) override {
		// Do nothing.
	}
	virtual bool selectable(void) const override {
		return false;
	}

	virtual const char* redoable(void) const override {
		return nullptr;
	}
	virtual const char* undoable(void) const override {
		return nullptr;
	}

	virtual void redo(class Asset*) override {
		// Do nothing.
	}
	virtual void undo(class Asset*) override {
		// Do nothing.
	}

	virtual Variant post(unsigned, int, const Variant*) override {
		return Variant(false);
	}

	virtual void update(
		class Window* wnd, class Renderer* rnd,
		class Workspace* ws, const class Project* /* project */, class Executable* exec,
		const char* /* title */,
		float /* x */, float /* y */, float width, float height,
		int /* scale */,
		bool pending,
		double /* delta */
	) override {
		ImGuiStyle &style = ImGui::GetStyle();

		shortcuts(wnd, rnd, ws, exec);

		const float statusBarHeight = ImGui::GetTextLineHeightWithSpacing() + style.FramePadding.y * 2;
		bool statusBarActived = ImGui::IsWindowFocused();

		ImGui::BeginChild("@Snd/Ply", ImVec2(width, height - statusBarHeight), false, ImGuiWindowFlags_NoNav);
		{
			_object->update();

			Editing::Tuner::Commands cmd = Editing::Tuner::NONE;
			const float* spectrum = nullptr;
			size_t spectrumCount = 0;
			if (!_overlay.empty()) {
				spectrum = _overlay.spectrum;
				spectrumCount = ANALYSER_COLUMN_COUNT;
			}
			const bool sliding = Editing::sound(
				rnd,
				ws,
				_object.get(),
				-1.0f, height - statusBarHeight,
				&_cursor, true,
				&cmd,
				spectrum, spectrumCount
			);
			if (sliding)
				_object->position(_cursor.position);

			switch (cmd) {
			case Editing::Tuner::PLAY:
				if (exec->current() == Executable::READY) {
					_object->stop();

					_object->play(
						false,
						std::bind(&EditorSoundImpl::soundFed, this, std::placeholders::_1, std::placeholders::_2),
						std::bind(&EditorSoundImpl::soundStop, this)
					);
				}

				break;
			case Editing::Tuner::STOP:
				_object->stop();

				_overlay.clear();

				break;
			default: // Do nothing.
				break;
			}

			statusBarActived |= ImGui::IsWindowFocused();
		}
		ImGui::EndChild();

		renderStatus(wnd, rnd, ws, width, statusBarHeight, pending, statusBarActived);
	}

	virtual void played(class Renderer* /* rnd */, const class Project* /* project */) override {
		_object->stop();

		_overlay.clear();
	}
	virtual void stopped(class Renderer* /* rnd */, const class Project* /* project */) override {
		// Do nothing.
	}

	virtual void resized(class Renderer* /* rnd */, const class Project* /* project */) override {
		// Do nothing.
	}

	virtual void lostFocus(class Renderer* /* rnd */, const class Project* /* project */) override {
		_object->stop();

		_overlay.clear();
	}
	virtual void gainFocus(class Renderer* /* rnd */, const class Project* /* project */) override {
		// Do nothing.
	}

private:
	void shortcuts(Window* /* wnd */, Renderer* /* rnd */, Workspace* ws, Executable* exec) {
		if (!ws->canUseShortcuts())
			return;

		if (exec->current() != Executable::READY)
			return;

		const Editing::Shortcut enter(SDL_SCANCODE_RETURN);
		const Editing::Shortcut esc(SDL_SCANCODE_ESCAPE);
		if (enter.pressed()) {
			_object->stop();

			_object->play(
				false,
				std::bind(&EditorSoundImpl::soundFed, this, std::placeholders::_1, std::placeholders::_2),
				std::bind(&EditorSoundImpl::soundStop, this)
			);
		} else if (esc.pressed()) {
			_object->stop();

			_overlay.clear();
		}
	}

	void refreshStatus(Window* /* wnd */, Renderer* /* rnd */, Workspace* ws) {
		if (_status.filled)
			return;

		_status.filled = true;

		if (readonly()) {
			_status.text += ws->theme()->statusTip_Readonly();
		}
	}
	void renderStatus(Window* wnd, Renderer* rnd, Workspace* ws, float width, float height, bool pending, bool actived) {
		refreshStatus(wnd, rnd, ws);

		ImGuiStyle &style = ImGui::GetStyle();

		if (actived) {
			const ImVec2 pos = ImGui::GetCursorPos();
			ImGui::Dummy(
				ImVec2(width - style.ChildBorderSize, height - style.ChildBorderSize),
				ImGui::GetStyleColorVec4(ImGuiCol_TabActive)
			);
			ImGui::SetCursorPos(pos);
		}

		if (actived)
			ImGui::PushStyleColor(ImGuiCol_Text, pending ? ws->theme()->style()->tabTextPendingColor : ws->theme()->style()->tabTextColor);
		ImGui::Dummy(ImVec2(8, 0));
		ImGui::SameLine();
		ImGui::AlignTextToFramePadding();
		ImGui::Text(
			"%s %.3f/%.3f    %s",
			ws->theme()->statusItem_Pos().c_str(),
			_object->playing() ? _object->position() : 0.0, _object->length(),
			_status.text.c_str()
		);
		if (actived)
			ImGui::PopStyleColor();
	}

	void soundFed(void* stream, int len) {
		_overlay.clear();

		float current[ANALYSER_COLUMN_COUNT];
		Analyser::analyse(_spec, stream, len, current);
		memcpy(_overlay.previous, _overlay.spectrum, sizeof(_overlay.spectrum));
		for (int i = 0; i < BITTY_COUNTOF(_overlay.spectrum); ++i) {
			const float factor = ((1.0f - _overlay.smooth) * current[i]) + (_overlay.smooth * _overlay.previous[i]);
			_overlay.spectrum[i] = factor;
		}

		_overlay.analysed = true;
	}
	void soundStop(void) {
		_overlay.clear();
	}
};

EditorSound* EditorSound::create(void) {
	EditorSoundImpl* result = new EditorSoundImpl();

	return result;
}

void EditorSound::destroy(EditorSound* ptr) {
	EditorSoundImpl* impl = static_cast<EditorSoundImpl*>(ptr);
	delete impl;
}

}

/* ===========================================================================} */
