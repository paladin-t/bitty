/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2022 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "document.h"
#include "encoding.h"
#include "file_handle.h"
#include "filesystem.h"
#include "platform.h"
#include "renderer.h"
#include "text.h"
#include "texture.h"
#include "theme.h"
#include "../lib/md4c/src/md4c.h"
#include <stack>
#include <SDL.h>

/*
** {===========================================================================
** Utilities
*/

static bool documentEscape(void) {
	return ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Escape), false);
}

static int documentSameLineIfPossible(float /* scale */, bool sameLine, const char* begin, size_t size, float endX) {
	if (!sameLine)
		return 0;

	const ImVec2 textSz = ImGui::CalcTextSize(begin, begin + size);
	const float width = textSz.x;
	const float posX = 0.0f;
	const float spacingW = -1.0f;
	ImGui::SameLine(posX, spacingW);
	const float curX = ImGui::GetCursorPosX();
	float wndR = ImGui::GetContentRegionAvail().x;
	if (ImGui::TableGetColumnCount() > 1)
		wndR = endX;
	wndR -= 5.0f; // Move to right a bit.
	if (curX + width > wndR) {
		ImGui::NewLine();
	}
	if (width > 1e-5f) {
		ImGui::PushItemWidth(width);

		return 1;
	}

	return 0;
}

/* ===========================================================================} */

/*
** {===========================================================================
** Document
*/

class DocumentImpl : public Document {
private:
	struct Context {
		Window* window = nullptr;
		Renderer* renderer = nullptr;
		DocumentImpl* self = nullptr;
		const Theme* theme = nullptr;

		float scale = 1.0f;

		bool newBlock = true;

		int indent = 0;

		int codeSeed = 1;
		int tableSeed = 0;

		char href[BITTY_MAX_PATH] = { '\0' };
		size_t hrefSize = 0;

		Context(Window* wnd, Renderer* rnd, DocumentImpl* self_, const Theme* theme_, float scale_) : window(wnd), renderer(rnd), self(self_), theme(theme_), scale(scale_) {
		}
	};

	typedef std::stack<MD_BLOCKTYPE> BlockStack;

	typedef std::stack<MD_SPANTYPE> SpanStack;

	typedef std::stack<float> ScaleStack;

	struct CodeHeight {
		typedef std::vector<CodeHeight> Array;

		float top = 0.0f;
		float bottom = 0.0f;
	};

	struct TableColumn {
		typedef std::vector<TableColumn> Array;
		typedef std::vector<float> Borders;

		bool initialized = false;
		int columnCount = 0;
		Borders borders;

		TableColumn() {
		}

		void setBorder(int idx, float val) {
			if (idx >= (int)borders.size())
				borders.resize(idx + 1);

			borders[idx] = val;
		}
		float getBorder(int idx) const {
			float first = 5.0f;
			if (!borders.empty())
				first = borders.front() - 1;
			if (idx < 0 || idx >= (int)borders.size())
				return ImGui::GetWindowContentRegionMax().x + first;

			return borders[idx];
		}
	};

	typedef std::map<const char*, Texture*> ImageDictionary;

private:
	std::string _document;
	int _shown = 0;
	bool _bringToFront = false;

	MD_PARSER _renderer;

	std::string _docTarget;
	std::string _scrollTarget;
	unsigned _scrollTargetDelay = 0;
	std::string _title;
	std::string _tableOfContent;
	std::string _content;

	BlockStack _blockStack;
	SpanStack _spanStack;
	ScaleStack _scaleStack;

	CodeHeight::Array _codeHeights;
	TableColumn::Array _tableColumns;
	int _tableCount = 0;
	int _tableRowIndex = 0;
	int _tableColumnIndex = 0;

	ImageDictionary _images;

public:
	DocumentImpl() {
		_renderer = {
			0,

			MD_FLAG_TABLES,

			[] (MD_BLOCKTYPE type, void* detail, void* userdata) -> int {
				Context* context = (Context*)userdata;
				DocumentImpl* impl = context->self;

				return impl->enterBlock(type, detail, context);
			},
			[] (MD_BLOCKTYPE type, void* detail, void* userdata) -> int {
				Context* context = (Context*)userdata;
				DocumentImpl* impl = context->self;

				return impl->leaveBlock(type, detail, context);
			},

			[] (MD_SPANTYPE type, void* detail, void* userdata) -> int {
				Context* context = (Context*)userdata;
				DocumentImpl* impl = context->self;

				return impl->enterSpan(type, detail, context);
			},
			[] (MD_SPANTYPE type, void* detail, void* userdata) -> int {
				Context* context = (Context*)userdata;
				DocumentImpl* impl = context->self;

				return impl->leaveSpan(type, detail, context);
			},

			[] (MD_TEXTTYPE type, const MD_CHAR* text, MD_SIZE size, void* userdata) -> int {
				Context* context = (Context*)userdata;
				DocumentImpl* impl = context->self;

				return impl->text(type, text, size, context);
			},

			[] (const char* msg, void* userdata) -> void {
				Context* context = (Context*)userdata;
				DocumentImpl* impl = context->self;

				impl->debugLog(msg, context);
			},

			nullptr
		};
	}
	virtual ~DocumentImpl() override {
		hide();
	}

	virtual const char* title(void) override {
		if (_title.empty())
			return nullptr;

		return _title.c_str();
	}

	virtual const char* shown(void) override {
		if (!_shown)
			return nullptr;

		return _document.c_str();
	}
	virtual void show(const char* doc) override {
		// Prepare.
		if (_shown)
			return;

		_document.clear();
		_shown = 1;

		_codeHeights.clear();
		_tableColumns.clear();
		_tableCount = 0;
		_tableRowIndex = 0;
		_tableColumnIndex = 0;

		if (!_content.empty())
			return;

		// Load document.
		auto load = [&] (const char* doc_) -> bool {
			// Prepare.
			bool result = false;
			File* file = File::create();

			// Open the file.
			if (file->open(doc_, Stream::READ)) {
				// Read from file.
				size_t l = file->count();
				Byte* buf = new Byte[l + 1];
				file->readBytes(buf, (size_t)l);
				buf[l] = '\0';
				_content = (const char*)buf;
				delete [] buf;
				file->close();

				// Parse table of content.
				do {
					constexpr char HEAD[] = "## Table of Content";
					constexpr char TAIL[] = "<!-- End Table of Content -->";
					size_t b = 0, e = 0;
					b = Text::indexOf(_content, HEAD);
					if (b == std::string::npos)
						break;
					e = Text::indexOf(_content, TAIL, b);
					if (e == std::string::npos)
						break;
					_tableOfContent = _content.substr(b + 19, e - (b + 19));
				} while (false);
				do {
					if (_tableOfContent.empty())
						break;
					constexpr char HEAD[] = "<!--";
					constexpr char TAIL[] = "-->";
					size_t b, e;
					b = Text::indexOf(_tableOfContent, HEAD);
					if (b == std::string::npos)
						break;
					e = Text::indexOf(_tableOfContent, TAIL, b);
					if (e == std::string::npos)
						break;
					_tableOfContent.erase(b, e - b + 3);
				} while (true);

				// Remove comments.
				do {
					constexpr char HEAD[] = "<!--";
					constexpr char TAIL[] = "-->";
					size_t b = 0, e = 0;
					b = Text::indexOf(_content, HEAD);
					if (b == std::string::npos)
						break;
					e = Text::indexOf(_content, TAIL, b);
					if (e == std::string::npos)
						break;
					_content.erase(b, e - b + 3);
				} while (true);

				// Get meta information.
				FileInfo::Ptr fileInfo = FileInfo::make(doc_);
				_title = fileInfo->fileName().c_str();
				if (!_title.empty())
					_title[0] = (char)::toupper(_title[0]);

				_document = doc_;

				result = true;
			}

			// Finish.
			File::destroy(file);

			return result;
		};
		if (!load(doc)) {
			std::string base, ext, parent;
			Path::split(doc, &base, &ext, &parent);
			if (!base.empty())
				base[0] = (char)::toupper(base[0]);
			const std::string doc_ = Path::combine(parent.c_str(), base.c_str()) + "." + ext;
			load(doc_.c_str());
		}
	}
	virtual void hide(void) override {
		if (!_shown)
			return;

		_shown = 0;

		_title.clear();
		_tableOfContent.clear();
		_content.clear();

		_codeHeights.clear();
		_tableColumns.clear();
		_tableCount = 0;
		_tableRowIndex = 0;
		_tableColumnIndex = 0;

		for (ImageDictionary::value_type kv : _images) {
			Texture* tex = kv.second;
			Texture::destroy(tex);
		}
		_images.clear();
	}

	virtual void bringToFront(void) override {
		_bringToFront = true;
	}

	virtual void update(class Window* wnd, class Renderer* rnd, const class Theme* theme, bool windowed) override {
		if (!_shown)
			return;

		shortcuts();

		if (!windowed) {
			document(wnd, rnd, theme);

			return;
		}

		const float scale = ImGui::GetIO().FontGlobalScale;

		if (_bringToFront) {
			_bringToFront = false;
			ImGui::SetNextWindowFocus();
		}
		if (_shown <= 2) { // Tricky for making it located at center.
			++_shown;
			ImGui::SetNextWindowPos(ImVec2(rnd->width() * 0.5f, rnd->height() * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
		} else {
			ImGui::SetNextWindowPos(ImVec2(rnd->width() * 0.5f, rnd->height() * 0.5f), ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
		}
		float sc = scale;
		if (sc > 3.0f)
			sc = 3.0f;
		ImGui::SetNextWindowSizeConstraints(ImVec2(320.0f, 240.0f), ImVec2(1e10, 1e10));
		ImGui::SetNextWindowSize(ImVec2(600.0f * sc, 400.0f  *sc), ImGuiCond_Appearing);
		if (ImGui::Begin(_title.empty() ? theme->windowDocument().c_str() : _title.c_str(), nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoNav)) {
			ImGui::BeginChild("Content", ImVec2(0.0f, ImGui::GetContentRegionAvail().y - ImGui::GetFrameHeightWithSpacing()), false, ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_NoNav);

			document(wnd, rnd, theme);

			ImGui::EndChild();

			if (ImGui::Button(theme->generic_Close().c_str()) || documentEscape()) {
				hide();

				ImGui::CloseCurrentPopup();
			}
		}
		ImGui::End();
	}

	void document(class Window* wnd, class Renderer* rnd, const class Theme* theme) {
		// Prepare.
		const float scale = ImGui::GetIO().FontGlobalScale;

		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.5f, 0.5f, 0.5f, 0.62f));
		ImGui::PushStyleColor(ImGuiCol_BorderShadow, ImVec4(0, 0, 0, 0));

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 2.0f * scale));

		// Process document jumping.
		if (!_docTarget.empty()) {
			hide();
			show(_docTarget.c_str());
			_docTarget.clear();
		}
		// Render the content.
		float width = 0;
		const bool withTableOfContent = !_tableOfContent.empty() && ImGui::GetWindowWidth() > 800;
		if (withTableOfContent)
			width = ImGui::GetWindowWidth() - 266;
		ImGui::BeginChild("@Doc/Ctt", ImVec2(width, 0), false, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoNav);
		{
			// Render the content.
			Context context(wnd, rnd, this, theme, scale);
			md_parse(
				(const MD_CHAR*)_content.c_str(), (MD_SIZE)_content.length(),
				&_renderer, &context
			);
			assert(_blockStack.empty());
			assert(_spanStack.empty());
			assert(_scaleStack.empty());
			while (!_blockStack.empty())
				_blockStack.pop();
			while (!_spanStack.empty())
				_spanStack.pop();
			while (!_scaleStack.empty())
				_scaleStack.pop();

			// Process navigation.
			if (!_scrollTarget.empty()) {
				if (_scrollTargetDelay > 0)
					--_scrollTargetDelay;
				else
					_scrollTarget.clear();

				if (_scrollTarget == ">>TOP") {
					ImGui::SetScrollY(0);
				} else if (_scrollTarget == ">>BOTTOM") {
					const float y = ImGui::GetCursorPosY();
					ImGui::SetScrollY(y);
				}
				if (_scrollTarget == ">>UP") {
					ImGui::SetScrollY(ImGui::GetScrollY() - 16);
				} else if (_scrollTarget == ">>DOWN") {
					ImGui::SetScrollY(ImGui::GetScrollY() + 16);
				}
			}
		}
		ImGui::EndChild();

		// Render table of content.
		if (withTableOfContent) {
			ImGui::SameLine();
			ImGui::BeginChild("@Doc/ToC", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_NoNav);
			{
				Context context(wnd, rnd, this, theme, scale);
				md_parse(
					(const MD_CHAR*)_tableOfContent.c_str(), (MD_SIZE)_tableOfContent.length(),
					&_renderer, &context
				);
				assert(_blockStack.empty());
				assert(_spanStack.empty());
				assert(_scaleStack.empty());
				while (!_blockStack.empty())
					_blockStack.pop();
				while (!_spanStack.empty())
					_spanStack.pop();
				while (!_scaleStack.empty())
					_scaleStack.pop();
			}
			ImGui::EndChild();
		}

		// Finish.
		ImGui::PopStyleVar();

		ImGui::PopStyleColor(2);
	}

private:
	void shortcuts(void) {
		const bool home = ImGui::IsKeyPressed(SDL_SCANCODE_HOME);
		const bool end = ImGui::IsKeyPressed(SDL_SCANCODE_END);
		const bool up = ImGui::IsKeyPressed(SDL_SCANCODE_UP);
		const bool down = ImGui::IsKeyPressed(SDL_SCANCODE_DOWN);

		if (home) {
			_scrollTarget = ">>TOP";
			_scrollTargetDelay = 1;
		} else if (end) {
			_scrollTarget = ">>BOTTOM";
			_scrollTargetDelay = 1;
		}
		if (up) {
			_scrollTarget = ">>UP";
			_scrollTargetDelay = 1;
		} else if (down) {
			_scrollTarget = ">>DOWN";
			_scrollTargetDelay = 1;
		}
	}

	int enterBlock(MD_BLOCKTYPE type, void* detail, Context* context) {
		_blockStack.push(type);

		switch (type) {
		case MD_BLOCK_UL:
			if (++context->indent > 1)
				ImGui::Indent();

			break;
		case MD_BLOCK_OL:
			break;
		case MD_BLOCK_LI:
			_scaleStack.push(1.0f);

			ImGui::SetWindowFontScale(1.0f);

			ImGui::Bullet();

			break;
		case MD_BLOCK_H: {
				ImFont* font = (ImFont*)context->theme->fontBlock();
				ImGui::PushFont(font);

				MD_BLOCK_H_DETAIL* h = (MD_BLOCK_H_DETAIL*)detail;
				const float scale = 1.0f - (h->level - 1) * 0.1f;

				_scaleStack.push(scale);

				ImGui::SetWindowFontScale(scale);

				ImGui::NewLine();
			}

			break;
		case MD_BLOCK_CODE: {
				ImGui::PushID(context->codeSeed);
				if (context->codeSeed < (int)_codeHeights.size())
					ImGui::BeginChild((ImGuiID)context->codeSeed, ImVec2(0.0f, _codeHeights[context->codeSeed].bottom + ImGui::GetFrameHeightWithSpacing()), true, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoNav);
				else
					ImGui::BeginChild((ImGuiID)context->codeSeed, ImVec2(0.0f, 0.0f), true, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoNav);
				++context->codeSeed;
				if ((int)_codeHeights.size() < context->codeSeed) {
					_codeHeights.resize(context->codeSeed);
					_codeHeights[context->codeSeed - 1].top = ImGui::GetCursorPosY();
				}
			}

			break;
		case MD_BLOCK_P:
			break;
		case MD_BLOCK_TABLE:
			if (context->tableSeed + 1 > (int)_tableColumns.size()) {
				_tableColumns.resize(context->tableSeed + 1);
				_tableColumns[context->tableSeed] = TableColumn();
			}
			if (_tableColumns[context->tableSeed].columnCount && _tableColumns[context->tableSeed].initialized) {
				const std::string id = "@Tbl" + Text::toString(context->tableSeed);
				const ImGuiTableFlags tblFlags = ImGuiTableFlags_Resizable | ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit;
				if (ImGui::BeginTable(id.c_str(), _tableColumns[context->tableSeed].columnCount, tblFlags))
					++_tableCount;
				for (int i = 0; i < _tableColumns[context->tableSeed].columnCount; ++i)
					ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);
			}

			break;
		case MD_BLOCK_THEAD:
			break;
		case MD_BLOCK_TBODY:
			break;
		case MD_BLOCK_TR:
			if (_tableCount) {
				_tableColumnIndex = 0;
				if (++_tableRowIndex == 1)
					ImGui::TableHeadersRow();
				else
					ImGui::TableNextRow();
			}

			break;
		case MD_BLOCK_TH:
			if (!_tableColumns[context->tableSeed].initialized)
				++_tableColumns[context->tableSeed].columnCount;

			if (_tableCount) {
				ImGui::TableSetColumnIndex(_tableColumnIndex);
				_tableColumns[context->tableSeed].setBorder(_tableColumnIndex, ImGui::GetCursorPos().x);
				++_tableColumnIndex;
			}

			break;
		case MD_BLOCK_TD:
			if (_tableCount) {
				ImGui::TableSetColumnIndex(_tableColumnIndex);
				_tableColumns[context->tableSeed].setBorder(_tableColumnIndex, ImGui::GetCursorPos().x);
				++_tableColumnIndex;
			}

			break;
		default:
			_scaleStack.push(1.0f);

			ImGui::SetWindowFontScale(1.0f);

			break;
		}

		context->newBlock = true;

		return 0;
	}
	int leaveBlock(MD_BLOCKTYPE type, void* /* detail */, Context* context) {
		_blockStack.pop();

		switch (type) {
		case MD_BLOCK_UL:
			if (--context->indent > 0)
				ImGui::Unindent();

			break;
		case MD_BLOCK_OL:
			break;
		case MD_BLOCK_LI:
			ImGui::SetWindowFontScale(1.0f);

			_scaleStack.pop();

			break;
		case MD_BLOCK_H:
			ImGui::SetWindowFontScale(1.0f);

			_scaleStack.pop();

			ImGui::PopFont();

			ImGui::NewLine();

			break;
		case MD_BLOCK_CODE:
			if (_codeHeights[context->codeSeed - 1].bottom == 0) {
				_codeHeights[context->codeSeed - 1].bottom = ImGui::GetCursorPosY() - _codeHeights[context->codeSeed - 1].top;
			}

			ImGui::EndChild();
			ImGui::PopID();

			break;
		case MD_BLOCK_P:
			if (_blockStack.top() == MD_BLOCK_LI)
				break;

			ImGui::NewLine();

			break;
		case MD_BLOCK_TABLE:
			++context->tableSeed;

			if (_tableCount) {
				--_tableCount;
				_tableRowIndex = 0;
				_tableColumnIndex = 0;
				ImGui::EndTable();
			}

			ImGui::NewLine();

			break;
		case MD_BLOCK_THEAD:
			_tableColumns[context->tableSeed].initialized = true;

			break;
		case MD_BLOCK_TBODY:
			break;
		case MD_BLOCK_TR:
			break;
		case MD_BLOCK_TH:
			break;
		case MD_BLOCK_TD:
			break;
		default:
			ImGui::SetWindowFontScale(1.0f);

			_scaleStack.pop();

			break;
		}

		return 0;
	}

	int enterSpan(MD_SPANTYPE type, void* detail, Context* context) {
		_spanStack.push(type);

		switch (type) {
		case MD_SPAN_STRONG: {
				ImFont* font = (ImFont*)context->theme->fontBlock_Bold();
				ImGui::PushFont(font);

				_scaleStack.push(0.5f);

				ImGui::SetWindowFontScale(0.5f);
			}

			break;
		case MD_SPAN_A: {
				MD_SPAN_A_DETAIL* a = (MD_SPAN_A_DETAIL*)detail;
				context->hrefSize = std::min((size_t)a->href.size, BITTY_COUNTOF(context->href));
				assert(a->href.size <= BITTY_COUNTOF(context->href));
				if (context->hrefSize == BITTY_COUNTOF(context->href)) {
					memcpy(context->href, a->href.text, context->hrefSize - 3);
					memcpy(context->href + context->hrefSize - 3, "...", 3);
				} else {
					memcpy(context->href, a->href.text, context->hrefSize);
				}

				_scaleStack.push(1.0f);

				ImGui::SetWindowFontScale(1.0f);
			}

			break;
		case MD_SPAN_IMG: {
				MD_SPAN_IMG_DETAIL* img = (MD_SPAN_IMG_DETAIL*)detail;

				Texture* tex = nullptr;
				ImageDictionary::iterator it = _images.find(img->src.text);
				if (it == _images.end()) {
					std::string str;
					str.assign(img->src.text, img->src.size);
					str = Path::combine(DOCUMENT_MARKDOWN_DIR, str.c_str());
					tex = Theme::createTexture(context->renderer, str.c_str());
					_images[img->src.text] = tex;
				} else {
					tex = it->second;
				}
				if (tex) {
					ImVec2 size((float)tex->width(), (float)tex->height());
					if (ImGui::GetContentRegionAvail().x < size.x) {
						size.y = ImGui::GetContentRegionAvail().x * size.y / size.x;
						size.x = ImGui::GetContentRegionAvail().x;
					}
					ImGui::Image(tex->pointer(context->renderer), size);
				}
			}

			break;
		case MD_SPAN_CODE:
			_scaleStack.push(1.0f);

			ImGui::SetWindowFontScale(1.0f);

			break;
		default: // Do nothing.
			break;
		}

		return 0;
	}
	int leaveSpan(MD_SPANTYPE type, void* /* detail */, Context* context) {
		switch (type) {
		case MD_SPAN_STRONG:
			ImGui::SetWindowFontScale(1.0f);

			_scaleStack.pop();

			ImGui::PopFont();

			break;
		case MD_SPAN_A:
			ImGui::SetWindowFontScale(1.0f);

			_scaleStack.pop();

			context->href[0] = '\0';
			context->hrefSize = 0;

			break;
		case MD_SPAN_IMG:
			break;
		case MD_SPAN_CODE:
			ImGui::SetWindowFontScale(1.0f);

			_scaleStack.pop();

			break;
		default: // Do nothing.
			break;
		}

		_spanStack.pop();

		return 0;
	}

	void url(const char* text, size_t tl, const char* link, size_t ll, Context* /* context */) {
		const ImU32 col = ImGui::GetColorU32(ImGuiCol_Text);
		ImGui::AlignTextToFramePadding();
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(41 / 255.0f, 148 / 255.0f, 1, 1));
		ImGui::TextUnformatted(text, text + tl);
		if (ImGui::IsItemHovered()) {
			ImGui::PushStyleColor(ImGuiCol_Text, col);
			const float scale = _scaleStack.top();
			ImGui::PushFont(nullptr);
			_scaleStack.push(0.5f);
			ImGui::SetWindowFontScale(0.5f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
			{
				ImGui::BeginTooltip();
				ImGui::TextUnformatted(link, link + ll);
				ImGui::EndTooltip();
			}
			ImGui::PopStyleVar();
			_scaleStack.pop();
			ImGui::SetWindowFontScale(scale);
			ImGui::PopFont();
			ImGui::PopStyleColor();
		}
		if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) { // Used `ImGui::IsItemHovered()` instead of `ImGui::IsItemClicked()` to avoid a clicking issue.
			bool pag = false;
			bool doc = false;
			bool web = false;
			std::string str;
			str.assign(link, ll);

			if (Text::startsWith(str, "#", false)) {
				pag = true;
				_scrollTarget = str;
				_scrollTargetDelay = 1;
			}

			if (!pag) {
				if (Text::startsWith(str, "http://", true) || Text::startsWith(str, "https://", true))
					web = true;
				else
					str = Path::combine(DOCUMENT_MARKDOWN_DIR, str.c_str());
			}

			if (!pag && !web) {
				std::string base, ext;
				Path::split(str, &base, &ext, nullptr);
				if (ext.empty()) {
					const size_t sep = Text::indexOf(str, '#');
					if (sep != std::string::npos) {
						pag = true;
						_scrollTarget = str.substr(sep);
						_scrollTargetDelay = 1;
						str = str.substr(0, sep);
					}
					str += "." DOCUMENT_MARKDOWN_EXT;
					doc = true;
				}
			}

			if (doc) {
				_docTarget = str;
			} else if (!pag) {
				const std::string osstr = Unicode::toOs(str);

				Platform::surf(osstr.c_str());
			}
		}
		ImGui::PopStyleColor();
	}

	void text(float scale, bool sameLine, const char* begin, size_t l, Context* context) {
		bool spanCode = !_spanStack.empty() && _spanStack.top() == MD_SPAN_CODE;
		int pushed = 0;

		float endX = 0.0f;
		if (context->tableSeed >= 0 && context->tableSeed < (int)_tableColumns.size())
			endX = _tableColumns[context->tableSeed].getBorder(_tableColumnIndex);
		if (spanCode) {
			pushed += documentSameLineIfPossible(scale, sameLine, begin, l, endX);
			std::string str;
			str.assign(begin, l);

			ImColor col(80, 80, 80, 180);
			ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)col);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)col);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)col);

			ImGui::Button(str.c_str());

			ImGui::PopStyleColor(3);
		} else if (context->hrefSize) {
			pushed += documentSameLineIfPossible(scale, sameLine, begin, l, endX);
			url(begin, l, context->href, context->hrefSize, context);
		} else {
			pushed += documentSameLineIfPossible(scale, sameLine, begin, l, endX);
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted(begin, begin + l);
		}

		for (int i = 0; i < pushed; ++i)
			ImGui::PopItemWidth();
	}
	int text(MD_TEXTTYPE type, const MD_CHAR* txt, MD_SIZE size, Context* context) {
		bool ignore = false;
		bool sameLine = true;
		if (context->newBlock) {
			context->newBlock = false;
			sameLine = false;
		}
		if (*txt == '\n') {
			context->newBlock = true;
			ignore = true;
			sameLine = false;
		}

		float scale = 1.0f;
		if (!_scaleStack.empty())
			scale = _scaleStack.top();

		if (!ignore) {
			switch (type) {
			case MD_TEXT_NORMAL: {
					const char* begin = txt;
					const char* end = txt;
					while (end && end <= txt + size) {
						bool sl = begin == txt ? sameLine : true;
						const char* endding = end;
						end = strchr(end, ' ');
						if (end) {
							if (++end > txt + size) {
								end = txt + size;
								size_t l = end - begin;
								text(scale, sl, begin, l, context);

								break; // Break looping.
							}
							size_t l = end - begin;
							text(scale, sl, begin, l, context);
							begin = end;
						} else if (endding && endding <= txt + size) {
							size_t l = txt + size - endding;
							text(scale, sl, begin, l, context);
						}
					}
				}

				break;
			case MD_TEXT_CODE: {
					text(scale, sameLine, txt, size, context);

					const char* end = _content.c_str() + _content.size();
					const char* tail = txt + size;
					if (tail + 1 < end) {
						if (*(tail + 0) == '\r' && *(tail + 1) == '\n') {
							if (tail + 3 < end) {
								if (*(tail + 2) == '\r' && *(tail + 3) == '\n')
									ImGui::NewLine();
							}
						} else if (*(tail + 0) == '\n') {
							if (*(tail + 1) == '\n')
								ImGui::NewLine();
						} else if (*(tail + 0) == '\r') {
							if (*(tail + 1) == '\r')
								ImGui::NewLine();
						}
					}
				}

				break;
			default:
				text(scale, sameLine, txt, size, context);

				break;
			}

			if (_blockStack.top() == MD_BLOCK_H && !_scrollTarget.empty()) {
				std::string str;
				str.assign(txt, size);
				size_t pos = 0;
				do {
					pos = Text::indexOf(str, '/');
					if (pos != std::string::npos) {
						str = str.erase(pos, 1);
					}
				} while (pos != std::string::npos);
				std::transform(
					str.begin(), str.end(), str.begin(),
					[] (char ch) -> char {
						if (ch == ' ')
							return '-';
						else
							return ch;
					}
				);
				if (_scrollTarget.size() - 1 == str.size() && Text::startsWith(str, _scrollTarget.c_str() + 1, true)) {
					if (_scrollTargetDelay > 0) {
						--_scrollTargetDelay;
						ImGui::SetScrollHereY(0.0f);
					} else {
						_scrollTarget.clear();
						ImGui::SetScrollHereY(0.0f);
					}
				}
			}
		}

		return 0;
	}

	void debugLog(const char* msg, Context* /* context */) {
		fprintf(stderr, "%s", msg);

		assert(!msg);
	}
};

Document::~Document() {
}

Document* Document::create(void) {
	DocumentImpl* result = new DocumentImpl();

	return result;
}

void Document::destroy(Document* ptr) {
	DocumentImpl* impl = static_cast<DocumentImpl*>(ptr);
	delete impl;
}

/* ===========================================================================} */
