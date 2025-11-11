/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2025 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "bytes.h"
#include "editor_font.h"
#include "font.h"
#include "image.h"
#include "theme.h"
#include "workspace.h"

/*
** {===========================================================================
** Font editor
*/

class EditorFontImpl : public EditorFont {
private:
	struct Glyph {
		typedef std::list<Glyph> List;

		Bytes::Ptr pixels = nullptr;
		int width = 0;
		int height = 0;

		Glyph() {
		}
		Glyph(Bytes::Ptr pix, int w, int h) : pixels(pix), width(w), height(h) {
		}
	};

private:
	bool _opened = false;

	std::string _name;
	Bytes::Ptr _object = nullptr;
	Texture::Ptr _texture = nullptr;

	int _size = 24;
	struct {
		std::string text;
		bool filled = false;

		void clear(void) {
			text.clear();
			filled = false;
		}
	} _status;

public:
	EditorFontImpl() {
	}
	virtual ~EditorFontImpl() override {
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

		_object = Object::as<Bytes::Ptr>(obj);

		fprintf(stdout, "Font editor opened: \"%s\".\n", _name.c_str());
	}
	virtual void close(const class Project* /* project */) override {
		if (!_opened)
			return;
		_opened = false;

		fprintf(stdout, "Font editor closed: \"%s\".\n", _name.c_str());

		_status.clear();

		_texture = nullptr;
		_object = nullptr;
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
		return false;
	}
	virtual void markChangesSaved(const class Project*) override {
		// Do nothing.
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
		class Workspace* ws, const class Project* /* project */, class Executable* /* exec */,
		const char* /* title */,
		float /* x */, float /* y */, float width, float height,
		float /* scaleX */, float /* scaleY */,
		bool pending,
		double /* delta */
	) override {
		ImGuiStyle &style = ImGui::GetStyle();

		const float statusBarHeight = ImGui::GetTextLineHeightWithSpacing() + style.FramePadding.y * 2;
		bool statusBarActived = ImGui::IsWindowFocused();

		const float posX = ImGui::GetCursorPosX();
		const float posY = ImGui::GetCursorPosY();
		ImGui::Dummy(ImVec2(8, 0));
		ImGui::SameLine();
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(ws->theme()->dialogItem_Size());
		ImGui::PushID("@Fnt/Ctrl");
		{
			ImGui::SameLine();
			ImGui::SetNextItemWidth(width - (ImGui::GetCursorPosX() - posX));
			if (ImGui::SliderInt("", &_size, 8, 128)) {
				_texture = nullptr;
			}
		}
		ImGui::PopID();
		const float toolBarHeight = ImGui::GetCursorPosY() - posY;

		ImGui::BeginChild("@Fnt/Vu", ImVec2(width, height - statusBarHeight - toolBarHeight), false, ImGuiWindowFlags_AlwaysHorizontalScrollbar | ImGuiWindowFlags_NoNav);
		{
			if (!_texture) {
				const std::string LINES[] = {
					"The Quick Brown Fox Jumps Over the Lazy Dog",
					"ABCDEFGHIJKLMNOPQRSTUVWXYZ",
					"abcdefghijklmnopqrstuvwxyz",
					"1234567890!@#$%^&*()",
					"`-=[]\\;',./~_+{}|:\"<>?"
				};
				const Color WHITE(255, 255, 255, 255);
				constexpr const int PADDING = 4;

				Font::Ptr font(Font::create());
				if (font->fromBytes(_object->pointer(), _object->count(), _size, 0)) {
					int width_ = PADDING, height_ = PADDING;
					Glyph::List lines[BITTY_COUNTOF(LINES)];
					for (int k = 0; k < BITTY_COUNTOF(LINES); ++k) {
						const std::string &str = LINES[k];
						Glyph::List &ln = lines[k];
						int w = PADDING, h = PADDING;
						for (char ch : str) {
							Bytes::Ptr glyph(Bytes::create());
							int w_ = 0, h_ = 0;
							font->render(ch, glyph.get(), &WHITE, &w_, &h_);
							ln.push_back(Glyph(glyph, w_, h_));

							w += w_ + PADDING;
							if (h_ > h)
								h = h_;
						}

						if (w > width_)
							width_ = w;
						height_ += h + PADDING;
					}

					int x = PADDING, y = PADDING;
					Image::Ptr img(Image::create(nullptr));
					img->fromBlank(width_, height_, 0);
					for (int k = 0; k < BITTY_COUNTOF(LINES); ++k) {
						const Glyph::List &ln = lines[k];
						int h = PADDING;
						for (const Glyph &ch : ln) {
							for (int j = 0; j < ch.height; ++j) {
								for (int i = 0; i < ch.width; ++i) {
									const Color col = ((Color*)ch.pixels->pointer())[i + j * ch.width];
									img->set(x + i, y + j, col);
								}
							}

							x += ch.width + PADDING;
							if (ch.height > h)
								h = ch.height;
						}

						x = PADDING;
						y += h + PADDING;
					}

					_texture = Texture::Ptr(Texture::create());
					_texture->fromImage(rnd, Texture::STATIC, img.get(), Texture::NEAREST);
					_texture->blend(Texture::BLEND);
				}
			}

			if (_texture) {
				ImGui::Image(
					(void*)_texture->pointer(rnd),
					ImVec2((float)_texture->width(), (float)_texture->height()),
					ImVec2(0, 0), ImVec2(1, 1),
					ImVec4(1, 1, 1, 1), ImVec4(1, 1, 1, 0.5f)
				);
			} else {
				ImGui::TextUnformatted(ws->theme()->dialogPrompt_InvalidAsset());
			}

			statusBarActived |= ImGui::IsWindowFocused();
		}
		ImGui::EndChild();

		renderStatus(wnd, rnd, ws, width, statusBarHeight, pending, statusBarActived);
	}

	virtual void played(class Renderer* /* rnd */, const class Project* /* project */) override {
		// Do nothing.
	}
	virtual void stopped(class Renderer* /* rnd */, const class Project* /* project */) override {
		// Do nothing.
	}

	virtual void resized(class Renderer* /* rnd */, const class Project* /* project */) override {
		// Do nothing.
	}

	virtual void lostFocus(class Renderer* /* rnd */, const class Project* /* project */) override {
		_texture = nullptr;
	}
	virtual void gainFocus(class Renderer* /* rnd */, const class Project* /* project */) override {
		// Do nothing.
	}

private:
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
			"%s",
			_status.text.c_str()
		);
		if (actived)
			ImGui::PopStyleColor();
	}
};

EditorFont* EditorFont::create(void) {
	EditorFontImpl* result = new EditorFontImpl();

	return result;
}

void EditorFont::destroy(EditorFont* ptr) {
	EditorFontImpl* impl = static_cast<EditorFontImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */
