/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2024 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "editor_polyfill.h"
#include "theme.h"
#include "workspace.h"
#include "../lib/imgui/imgui.h"

/*
** {===========================================================================
** Macros and constants
*/

#ifndef EDITOR_POLYFILL_NONE
#	define EDITOR_POLYFILL_NONE(CLASS) \
	public: \
		CLASS() { \
		} \
		virtual ~CLASS() override { \
		} \
		virtual unsigned type(void) const override { \
			return TYPE(); \
		} \
		virtual void open(const class Project*, const char*, Object::Ptr, const char*) override { \
		} \
		virtual void close(const class Project*) override { \
		} \
		virtual void flush(void) const override { \
		} \
		virtual bool readonly(void) const override { \
			return true; \
		} \
		virtual void readonly(bool) override { \
		} \
		virtual bool hasUnsavedChanges(void) const override { \
			return false; \
		} \
		virtual void markChangesSaved(const class Project*) override { \
		} \
		virtual void copy(void) override { \
		} \
		virtual void cut(void) override { \
		} \
		virtual bool pastable(void) const override { \
			return false; \
		} \
		virtual void paste(void) override { \
		} \
		virtual void del(void) override { \
		} \
		virtual bool selectable(void) const override { \
			return false; \
		} \
		virtual const char* redoable(void) const override { \
			return nullptr; \
		} \
		virtual const char* undoable(void) const override { \
			return nullptr; \
		} \
		virtual void redo(class Asset*) override { \
		} \
		virtual void undo(class Asset*) override { \
		} \
		virtual Variant post(unsigned, int, const Variant*) override { \
			return Variant(false); \
		} \
		virtual void update( \
			class Window* wnd, class Renderer* rnd, \
			class Workspace* ws, const class Project*, class Executable*, \
			const char*, \
			float, float, float width, float height, \
			float, float, \
			bool pending, \
			double \
		) override { \
			ImGuiStyle &style = ImGui::GetStyle(); \
			const float statusBarHeight = ImGui::GetTextLineHeightWithSpacing() + style.FramePadding.y * 2; \
			bool statusBarActived = ImGui::IsWindowFocused(); \
			ImGui::BeginChild("@Plyfl/Vu", ImVec2(width, height - statusBarHeight), false, ImGuiWindowFlags_AlwaysHorizontalScrollbar | ImGuiWindowFlags_NoNav); \
			{ \
				ImGui::Dummy(ImVec2(8, 0)); \
				ImGui::SameLine(); \
				ImGui::AlignTextToFramePadding(); \
				ImGui::TextUnformatted("Get pro version to view or edit"); \
				ImGui::Dummy(ImVec2(8, 0)); \
				ImGui::SameLine(); \
				ImGui::Url("Homepage", "https://paladin-t.github.io/bitty/#buy-bitty-engine"); \
				statusBarActived |= ImGui::IsWindowFocused(); \
			} \
			ImGui::EndChild(); \
			renderStatus(wnd, rnd, ws, width, statusBarHeight, pending, statusBarActived); \
		} \
		virtual void played(class Renderer*, const class Project*) override { \
		} \
		virtual void stopped(class Renderer*, const class Project*) override { \
		} \
		virtual void resized(class Renderer*, const class Project*) override { \
		} \
		virtual void lostFocus(class Renderer*, const class Project*) override { \
		} \
		virtual void gainFocus(class Renderer*, const class Project*) override { \
		} \
	private: \
		void renderStatus(Window* /* wnd */, Renderer* /* rnd */, Workspace* ws, float width, float height, bool pending, bool actived) const { \
			ImGuiStyle &style = ImGui::GetStyle(); \
			if (actived) { \
				const ImVec2 pos = ImGui::GetCursorPos(); \
				ImGui::Dummy( \
					ImVec2(width - style.ChildBorderSize, height - style.ChildBorderSize), \
					ImGui::GetStyleColorVec4(ImGuiCol_TabActive) \
				); \
				ImGui::SetCursorPos(pos); \
			} \
			if (actived) \
				ImGui::PushStyleColor(ImGuiCol_Text, pending ? ws->theme()->style()->tabTextPendingColor : ws->theme()->style()->tabTextColor); \
			ImGui::Dummy(ImVec2(8, 0)); \
			ImGui::SameLine(); \
			ImGui::AlignTextToFramePadding(); \
			ImGui::TextUnformatted(ws->theme()->generic_Unknown()); \
			if (actived) \
				ImGui::PopStyleColor(); \
		}
#endif /* EDITOR_POLYFILL_NONE */

/* ===========================================================================} */

/*
** {===========================================================================
** Polyfills
*/

class EditorBytesImpl : public EditorBytes {
	EDITOR_POLYFILL_NONE(EditorBytesImpl)
};

EditorBytes* EditorBytes::create(void) {
	EditorBytesImpl* result = new EditorBytesImpl();

	return result;
}

void EditorBytes::destroy(EditorBytes* ptr) {
	EditorBytesImpl* impl = static_cast<EditorBytesImpl*>(ptr);
	delete impl;
}

class EditorFontImpl : public EditorFont {
	EDITOR_POLYFILL_NONE(EditorFontImpl)
};

EditorFont* EditorFont::create(void) {
	EditorFontImpl* result = new EditorFontImpl();

	return result;
}

void EditorFont::destroy(EditorFont* ptr) {
	EditorFontImpl* impl = static_cast<EditorFontImpl*>(ptr);
	delete impl;
}

class EditorImageImpl : public EditorImage {
	EDITOR_POLYFILL_NONE(EditorImageImpl)
};

EditorImage* EditorImage::create(void) {
	EditorImageImpl* result = new EditorImageImpl();

	return result;
}

void EditorImage::destroy(EditorImage* ptr) {
	EditorImageImpl* impl = static_cast<EditorImageImpl*>(ptr);
	delete impl;
}

class EditorMapImpl : public EditorMap {
	EDITOR_POLYFILL_NONE(EditorMapImpl)
};

EditorMap* EditorMap::create(void) {
	EditorMapImpl* result = new EditorMapImpl();

	return result;
}

void EditorMap::destroy(EditorMap* ptr) {
	EditorMapImpl* impl = static_cast<EditorMapImpl*>(ptr);
	delete impl;
}

class EditorPaletteImpl : public EditorPalette {
	EDITOR_POLYFILL_NONE(EditorPaletteImpl)
};

EditorPalette* EditorPalette::create(void) {
	EditorPaletteImpl* result = new EditorPaletteImpl();

	return result;
}

void EditorPalette::destroy(EditorPalette* ptr) {
	EditorPaletteImpl* impl = static_cast<EditorPaletteImpl*>(ptr);
	delete impl;
}

class EditorPluginImpl : public EditorPlugin {
	EDITOR_POLYFILL_NONE(EditorPluginImpl)
};

EditorPlugin* EditorPlugin::create(void) {
	EditorPluginImpl* result = new EditorPluginImpl();

	return result;
}

void EditorPlugin::destroy(EditorPlugin* ptr) {
	EditorPluginImpl* impl = static_cast<EditorPluginImpl*>(ptr);
	delete impl;
}

class EditorSoundImpl : public EditorSound {
	EDITOR_POLYFILL_NONE(EditorSoundImpl)
};

EditorSound* EditorSound::create(void) {
	EditorSoundImpl* result = new EditorSoundImpl();

	return result;
}

void EditorSound::destroy(EditorSound* ptr) {
	EditorSoundImpl* impl = static_cast<EditorSoundImpl*>(ptr);
	delete impl;
}

class EditorSpriteImpl : public EditorSprite {
	EDITOR_POLYFILL_NONE(EditorSpriteImpl)
};

EditorSprite* EditorSprite::create(void) {
	EditorSpriteImpl* result = new EditorSpriteImpl();

	return result;
}

void EditorSprite::destroy(EditorSprite* ptr) {
	EditorSpriteImpl* impl = static_cast<EditorSpriteImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */
