/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2025 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "audio.h"
#include "editing.h"
#include "encoding.h"
#include "image.h"
#include "map.h"
#include "project.h"
#include "sprite.h"
#include "theme.h"
#include "workspace.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "../lib/imgui/imgui_internal.h"
#include "../lib/lz4/lib/lz4.h"
#include <SDL.h>

/*
** {===========================================================================
** Utilities
*/

static const char* editingTextOffset(const char* code, int ln, int col) {
	// To line.
	if (ln > 0) {
		while (*code) {
			const char ch = *code;
			if (ch == '\n') {
				if (--ln == 0) {
					++code;

					break;
				}
			}
			++code;
		}
	}

	// To column.
	for (int i = 0; i < col; ++i) {
		if (*code == '\0')
			break;

		const int n = Unicode::expectUtf8(code);
		code += n;
	}

	return code;
}
static const char* editingTextFindForward(const char* txt, const char* const what, int ln, int col, int* lnoff, int* coloff) {
	if (lnoff)
		*lnoff = 0;
	if (coloff)
		*coloff = 0;
	const char* off = editingTextOffset(txt, ln, col);

	const char* mat = strstr(off, what);
	if (mat) { // Found.
		const char* str = off;
		while (str < mat) {
			if (*str == '\0')
				break;
			if (*str == '\n') {
				++(*lnoff);
				*coloff = 0;
				++str;
			} else {
				const int n = Unicode::expectUtf8(str);
				++(*coloff);
				str += n;
			}
		}
	}

	return mat;
}
static const char* editingTextFindBackward(const char* txt, const char* const what, int ln, int col, int* lnoff, int* coloff) {
	if (lnoff)
		*lnoff = 0;
	if (coloff)
		*coloff = 0;
	const char* off = editingTextOffset(txt, ln, col);

	std::string stdcode;
	stdcode.assign(txt, off);
	size_t mat = stdcode.rfind(what, stdcode.length() - 1);

	if (mat != std::string::npos) { // Found.
		const char* str = txt;
		while (str < txt + mat) {
			if (*str == '\0')
				break;
			if (*str == '\n') {
				++(*lnoff);
				*coloff = 0;
				++str;
			} else {
				const int n = Unicode::expectUtf8(str);
				++(*coloff);
				str += n;
			}
		}

		return txt + mat;
	}

	return nullptr;
}

/* ===========================================================================} */

/*
** {===========================================================================
** Editing
*/

namespace Editing {

Dots::Dots(Color* col) : colored(col) {
}

Dots::Dots(int* idx) : indexed(idx) {
}

Dot::Dot() {
	colored = Color(0, 0, 0, 0);
}

Dot::Dot(const Dot &other) {
	memcpy(this, &other, sizeof(Dot));
}

Dot &Dot::operator = (const Dot &other) {
	memcpy(this, &other, sizeof(Dot));

	return *this;
}

Point::Point() {
	dot.colored = Color(0, 0, 0, 0);
}

Point::Point(const Math::Vec2i &pos) : position(pos) {
}

Point::Point(const Math::Vec2i &pos, const Color &col) : position(pos) {
	dot.colored = col;
}

Point::Point(const Math::Vec2i &pos, int idx) : position(pos) {
	dot.indexed = idx;
}

bool Point::operator < (const Point &other) const {
	return position < other.position;
}

Painting::Painting() {
}

Painting::operator bool (void) const {
	return _value;
}

Painting &Painting::set(bool val) {
	_lastValue = _value;
	_value = val;

	_lastPosition = _position;
	const ImVec2 pos = ImGui::GetMousePos();
	_position = Math::Vec2f(pos.x, pos.y);

	return *this;
}

bool Painting::moved(void) const {
	return _lastPosition != _position;
}

bool Painting::down(void) const {
	return !_lastValue && _value;
}

bool Painting::up(void) const {
	return _lastValue && !_value;
}

void Painting::clear(void) {
	_lastValue = false;
	_value = false;
	_lastPosition = Math::Vec2f(-1, -1);
	_position = Math::Vec2f(-1, -1);
}

Brush::Brush() {
}

bool Brush::operator == (const Brush &other) const {
	return position == other.position &&
		size == other.size;
}

bool Brush::operator != (const Brush &other) const {
	return position != other.position ||
		size != other.size;
}

bool Brush::operator == (const Math::Recti &other) const {
	return position.x == other.xMin() &&
		position.y == other.yMin() &&
		size.x == other.width() &&
		size.y == other.height();
}

bool Brush::operator != (const Math::Recti &other) const {
	return position.x != other.xMin() ||
		position.y != other.yMin() ||
		size.x != other.width() ||
		size.y != other.height();
}

bool Brush::empty(void) const {
	return position == Math::Vec2i(-1, -1);
}

Math::Vec2i Brush::unit(void) const {
	if (size.x <= 0 || size.y <= 0)
		return position;

	return Math::Vec2i(position.x / size.x, position.y / size.y);
}

Math::Recti Brush::toRect(void) const {
	return Math::Recti::byXYWH(position.x, position.y, size.x, size.y);
}

Brush &Brush::fromRect(const Math::Recti &other) {
	position = Math::Vec2i(other.xMin(), other.yMin());
	size = Math::Vec2i(other.width(), other.height());

	return *this;
}

Frame::Frame() {
}

Frame::Frame(const std::string &anim, int idx) : animation(anim), index(idx) {
}

bool Frame::operator == (const Frame &other) const {
	return animation == other.animation &&
		index == other.index;
}

bool Frame::operator != (const Frame &other) const {
	return animation != other.animation ||
		index != other.index;
}

bool Frame::empty(void) const {
	return animation.empty() &&
		index == -1;
}

Tiler::Tiler() {
}

bool Tiler::operator == (const Tiler &other) const {
	return position == other.position &&
		size == other.size;
}

bool Tiler::operator != (const Tiler &other) const {
	return position != other.position ||
		size != other.size;
}

bool Tiler::empty(void) const {
	return position == Math::Vec2i(-1, -1);
}

Tuner::Tuner() {
}

bool Tuner::operator == (const Tuner &other) const {
	return position == other.position &&
		length == other.length;
}

bool Tuner::operator != (const Tuner &other) const {
	return position != other.position ||
		length != other.length;
}

bool Tuner::empty(void) const {
	return position == 0 && length == 0;
}

Shortcut::Shortcut(
	int key_,
	bool ctrl_, bool shift_, bool alt_,
	bool numLock_, bool capsLock_,
	bool super_
) : key(key_),
	ctrl(ctrl_), shift(shift_), alt(alt_),
	numLock(numLock_), capsLock(capsLock_),
	super(super_)
{
}

bool Shortcut::pressed(bool repeat) const {
	ImGuiIO &io = ImGui::GetIO();

	const SDL_Keymod mod = SDL_GetModState();
	const bool num = !!(mod & KMOD_NUM);
	const bool caps = !!(mod & KMOD_CAPS);

	if (!key && !ctrl && !shift && !alt && !numLock && !capsLock && !super)
		return false;

	if (key && !ImGui::IsKeyPressed(key, repeat))
		return false;

	if (ctrl && !io.KeyCtrl)
		return false;
	else if (!ctrl && io.KeyCtrl)
		return false;

	if (shift && !io.KeyShift)
		return false;
	else if (!shift && io.KeyShift)
		return false;

	if (alt && !io.KeyAlt)
		return false;
	else if (!alt && io.KeyAlt)
		return false;

	if (numLock && !num)
		return false;

	if (capsLock && !caps)
		return false;

	if (super && !io.KeySuper)
		return false;
	else if (!super && io.KeySuper)
		return false;

	return true;
}

bool Shortcut::released(void) const {
	ImGuiIO &io = ImGui::GetIO();

	const SDL_Keymod mod = SDL_GetModState();
	const bool num = !!(mod & KMOD_NUM);
	const bool caps = !!(mod & KMOD_CAPS);

	if (!key && !ctrl && !shift && !alt && !numLock && !capsLock && !super)
		return false;

	if (key && ImGui::IsKeyReleased(key))
		return true;

	if (ctrl && !io.KeyCtrl)
		return true;
	else if (!ctrl && io.KeyCtrl)
		return true;

	if (shift && !io.KeyShift)
		return true;
	else if (!shift && io.KeyShift)
		return true;

	if (alt && !io.KeyAlt)
		return true;
	else if (!alt && io.KeyAlt)
		return true;

	if (numLock && !num)
		return true;

	if (capsLock && !caps)
		return true;

	if (super && !io.KeySuper)
		return true;
	else if (!super && io.KeySuper)
		return true;

	return false;
}

bool palette(
	Renderer* rnd,
	Workspace*,
	const Palette* ptr,
	float width,
	Color* col,
	int* cursor, bool showCursor,
	Texture* transparent
) {
	bool result = false;

	if (width <= 0)
		width = ImGui::GetContentRegionAvail().x;
	const int X_COUNT = (int)std::sqrt(IMAGE_PALETTE_COLOR_COUNT);
	const float size = std::min(width / X_COUNT, ImGui::GetFontSize() * 4);

	const int count = ptr->count();
	assert(count == IMAGE_PALETTE_COLOR_COUNT);
	for (int i = 0; i < count; ++i) {
		Color color;
		ptr->get(i, color);
		const ImVec4 col4v(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f);

		ImGui::PushID(i);

		const ImVec2 pos = ImGui::GetCursorScreenPos();
		bool pressed = false;
		if (color.a < 255 && transparent) {
			ImGui::Image(transparent->pointer(rnd), ImVec2(size, size));
			ImGui::SetCursorScreenPos(pos);
		}
		pressed = ImGui::ColorButton("", col4v, ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_AlphaPreview, ImVec2(size, size));
		if (pressed) {
			result = true;

			if (cursor)
				*cursor = i;
			if (col)
				*col = color;
		}
		if (showCursor) {
			const bool active = cursor && *cursor == i;
			if (active)
				ImGui::Indicator(pos, pos + ImVec2(size, size));
		}
		if ((i + 1) % X_COUNT != 0)
			ImGui::SameLine();

		ImGui::PopID();
	}

	return result;
}

bool image(
	Renderer* rnd,
	Workspace* ws,
	const Image* ptr, Texture* tex,
	float width,
	Brush* cursor, bool showCursor,
	const Brush* selection,
	Texture* overlay,
	const Math::Vec2i* gridSize, bool showGrids,
	bool showTransparentBackbround,
	int mouseActionButton
) {
	return image(
		rnd,
		ws,
		ptr, tex,
		width,
		cursor, showCursor, Brush(),
		selection,
		overlay,
		gridSize, showGrids,
		showTransparentBackbround,
		mouseActionButton
	);
}

bool image(
	Renderer* rnd,
	Workspace* ws,
	const Image* ptr, Texture* tex,
	float width,
	Brush* cursor, bool showCursor, Brush &&cursorStamp,
	const Brush* selection,
	Texture* overlay,
	const Math::Vec2i* gridSize, bool showGrids,
	bool showTransparentBackbround,
	int mouseActionButton
) {
	ImGuiStyle &style = ImGui::GetStyle();
	ImDrawList* drawList = ImGui::GetWindowDrawList();

	bool result = false;

	float widgetWidth = (float)ptr->width();
	float widgetHeight = (float)ptr->height();
	float scale = 1;
	if (width > 0 && std::abs(widgetWidth - width) >= Math::EPSILON<float>()) {
		scale = width / widgetWidth;
		widgetHeight = widgetHeight / widgetWidth * width;
		widgetWidth = width;
	}
	const ImVec2 widgetSize(widgetWidth, widgetHeight);

	if (showTransparentBackbround) {
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2());
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0);
		ImGui::BeginChildFrame(
			ImGui::GetID("@Img/Bg"),
			widgetSize + ImVec2(style.WindowBorderSize * 2 + 1, style.WindowBorderSize * 2),
			ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoNav
		);
	}

	ImVec2 curPos = ImGui::GetCursorScreenPos();
	curPos.x += 1;
	if (showTransparentBackbround) {
		float transparentWidth = 0;
		float transparentHeight = 0;
		if (gridSize && (gridSize->x > BITTY_GRID_DEFAULT_SIZE || gridSize->y > BITTY_GRID_DEFAULT_SIZE)) {
			transparentWidth = (float)gridSize->x * 2 * scale;
			transparentHeight = (float)gridSize->y * 2 * scale;
		} else {
			transparentWidth = (float)ws->theme()->iconTransparent()->width() * scale;
			transparentHeight = (float)ws->theme()->iconTransparent()->height() * scale;
		}
		for (float j = 0; j < widgetHeight; j += transparentHeight) {
			for (float i = 0; i < widgetWidth; i += transparentWidth) {
				const ImVec2 pos = curPos + ImVec2(i, j) + ImVec2(0, 1);
				ImGui::SetCursorScreenPos(pos);
				ImGui::Image(
					(void*)ws->theme()->iconTransparent()->pointer(rnd),
					ImVec2(transparentWidth, transparentHeight),
					ImVec2(0, 0), ImVec2(1, 1),
					ImVec4(1, 1, 1, 1), ImVec4(1, 1, 1, 0.5f)
				);
			}
		}
	}
	ImGui::SetCursorScreenPos(curPos);
	ImGui::Image(
		(void*)tex->pointer(rnd),
		widgetSize,
		ImVec2(0, 0), ImVec2(1, 1),
		ImVec4(1, 1, 1, 1), ImVec4(1, 1, 1, 0.5f)
	);
	if (overlay) {
		ImGui::SetCursorScreenPos(curPos);
		ImGui::Image(
			(void*)overlay->pointer(rnd),
			widgetSize,
			ImVec2(0, 0), ImVec2(1, 1),
			ImVec4(1, 1, 1, 1), ImVec4(1, 1, 1, 0.5f)
		);
	}

	if (showGrids && gridSize) {
		for (float i = 0; i <= widgetWidth; i += gridSize->x * scale) {
			drawList->AddLine(
				curPos + ImVec2(i, 0),
				curPos + ImVec2(i, widgetHeight),
				ImGui::GetColorU32(ImVec4(1, 1, 1, 0.75f))
			);
		}
		for (float j = 0; j <= widgetHeight; j += gridSize->y * scale) {
			drawList->AddLine(
				curPos + ImVec2(0, j),
				curPos + ImVec2(widgetWidth, j),
				ImGui::GetColorU32(ImVec4(1, 1, 1, 0.75f))
			);
		}
	}

	if (showCursor && cursor) {
		const Math::Recti rect = Math::Recti::byXYWH(cursor->position.x, cursor->position.y, cursor->size.x, cursor->size.y);
		const Int cursorX = rect.xMin();
		const Int cursorY = rect.yMin();
		const bool active = Math::intersects(Math::Recti(0, 0, ptr->width(), ptr->height()), rect);
		if (active) {
			ImVec2 blinkPos((float)cursorX, (float)cursorY);
			blinkPos *= scale;
			blinkPos += curPos;
			ImVec2 blinkSize((float)rect.width(), (float)rect.height());
			blinkSize *= scale;
			float blinkThick = 3;
			if (blinkSize.x <= 3) {
				blinkPos.x -= 1;
				blinkSize.x += 2;
				blinkThick = 1;
			}
			if (blinkSize.y <= 3) {
				blinkPos.y -= 1;
				blinkSize.y += 2;
				blinkThick = 1;
			}
			ImGui::Indicator(blinkPos, blinkPos + blinkSize, blinkThick);
		}
	}

	if (selection) {
		const Math::Recti rect = Math::Recti::byXYWH(selection->position.x, selection->position.y, selection->size.x, selection->size.y);
		const Int selX = selection->position.x;
		const Int selY = selection->position.y;
		const bool active = Math::intersects(Math::Recti(0, 0, ptr->width(), ptr->height()), rect);
		if (active) {
			ImVec2 blinkPos((float)selX, (float)selY);
			blinkPos *= scale;
			blinkPos += curPos;
			ImVec2 blinkSize((float)selection->size.x, (float)selection->size.y);
			blinkSize *= scale;
			float blinkThick = 3;
			if (blinkSize.x <= 3) {
				blinkPos.x -= 1;
				blinkSize.x += 2;
				blinkThick = 1;
			}
			if (blinkSize.y <= 3) {
				blinkPos.y -= 1;
				blinkSize.y += 2;
				blinkThick = 1;
			}
			ImGui::Indicator(blinkPos, blinkPos + blinkSize, blinkThick);
		}
	}

	if (ImGui::IsItemHovered()) {
		ImVec2 imgPos = ImGui::GetMousePosOnCurrentItem(&curPos);
		imgPos /= scale;

		const Int x = Math::clamp((int)imgPos.x, 0, ptr->width() - 1);
		const Int y = Math::clamp((int)imgPos.y, 0, ptr->height() - 1);

		if (cursorStamp.empty()) {
			if (ImGui::IsMouseDown(mouseActionButton))
				result = true;

			if (cursor) {
				cursor->position = Math::Vec2i(
					x / cursor->size.x * cursor->size.x,
					y / cursor->size.y * cursor->size.y
				);
			}
		} else if (cursor) {
			if (ImGui::IsMouseClicked(mouseActionButton)) {
				cursorStamp.position = Math::Vec2i(
					x / cursorStamp.size.x,
					y / cursorStamp.size.y
				);
				cursor->position = Math::Vec2i(
					x / cursorStamp.size.x * cursorStamp.size.x,
					y / cursorStamp.size.y * cursorStamp.size.y
				);
				cursor->size = cursorStamp.size;
			} else if (ImGui::IsMouseDown(mouseActionButton)) {
				const Math::Vec2i diff = Math::Vec2i(x / cursorStamp.size.x, y / cursorStamp.size.y) -
					cursorStamp.position +
					Math::Vec2i(1, 1);
				const Math::Recti rect = Math::Recti::byXYWH(cursorStamp.position.x, cursorStamp.position.y, diff.x, diff.y);
				cursor->position = Math::Vec2i(rect.xMin(), rect.yMin()) * cursorStamp.size;
				cursor->size = Math::Vec2i(rect.width(), rect.height()) * cursorStamp.size;
			} else if (ImGui::IsMouseReleased(mouseActionButton)) {
				result = true;
			}
		}
	}

	if (showTransparentBackbround) {
		ImGui::PopStyleVar(2);
		ImGui::EndChildFrame();
	}

	return result;
}

bool sprite(
	Renderer* rnd,
	Workspace* ws,
	const Sprite* ptr,
	float width, float scale,
	Frame* cursor, bool showCursor,
	SpriteAnimationAddingHandler addAnim, SpriteAnimationRemovingHandler removeAnim, SpriteAnimationRenamingHandler renameAnim,
	SpriteFrameInsertingHandler insertFrame, SpriteFrameRemovingHandler removeFrame,
	SpriteFrameIntervalChangingHandler changeInterval
) {
	ImGuiStyle &style = ImGui::GetStyle();

	bool result = false;
	const bool focusingWnd = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

	const float minSize = ImGui::GetFontSize() * 3 * scale;
	float itemWidth = ImGui::GetFontSize() * 4 * scale;
	float itemHeight = itemWidth * ((float)ptr->height() / ptr->width());
	if (itemWidth < minSize) {
		itemWidth = minSize;
		itemHeight = minSize * ((float)ptr->height() / ptr->width());
	}
	if (itemHeight < minSize) {
		itemWidth = minSize * ((float)ptr->width() / ptr->height());
		itemHeight = minSize;
	}
	const ImVec2 itemSize(itemWidth, itemHeight);

	auto addToTail = [ws] (const Frame &tail, const ImVec2 &sz, Frame &inserter) -> void {
		ImGuiStyle &style = ImGui::GetStyle();

		if (!tail.animation.empty()) {
			ImGui::SameLine();
			if (ImGui::Button("+", sz + style.FramePadding * 2)) {
				if (inserter.empty())
					inserter = tail;

				ImGui::SetScrollX(ImGui::GetScrollMaxX());
			}

			if (ImGui::IsItemHovered()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				ImGui::SetTooltip(ws->theme()->tooltipEditing_AddFrame());
			}

			ImGui::EndChildFrame();
		}
	};

	Frame tail;
	Frame inserter;
	bool extended = false;
	const char* tobeRemoved = nullptr;
	const char* tobeRenamed = nullptr;

	for (int i = 0; i < ptr->count(); ++i) {
		Math::Recti area;
		double interval = 0;
		const char* key = nullptr;
		Texture::Ptr tex = nullptr;
		ptr->get(i, &tex, &area, &interval, &key);

		const char* anim = key;
		if ((!anim || !(*anim)) && i == 0)
			anim = EDITING_ANONYMOUS;

		if (anim && *anim) { // First frame of an animation.
			addToTail(tail, itemSize, inserter);

			extended = ImGui::CollapsingHeader(anim, ImGuiTreeNodeFlags_DefaultOpen);

			const float padR = style.FramePadding.x;
			const float buttonSz = ImGui::GetFontSize();
			ImGui::SameLine();
			ImVec2 customBtnPos = ImGui::GetCursorScreenPos();
			ImGui::NewLine();
			customBtnPos.x = ImGui::GetWindowPos().x + width - (padR + buttonSz) - style.FramePadding.x;
			if (ImGui::CustomButton("#Dl", &customBtnPos, ImGui::CustomCloseButton, ws->theme()->tooltipEditing_DeleteAnimation().c_str()) && focusingWnd)
				tobeRemoved = anim;
			if (ImGui::CustomButton("#Rn", &customBtnPos, ImGui::CustomRenameButton, ws->theme()->tooltipEditing_RenameAnimation().c_str()) && focusingWnd)
				tobeRenamed = anim;

			if (extended) {
				ImGui::BeginChildFrame(
					ImGui::GetID(anim),
					ImVec2(0, itemHeight + (style.FramePadding.y + style.ItemInnerSpacing.y) * 2 + style.ScrollbarSize),
					ImGuiWindowFlags_AlwaysHorizontalScrollbar | ImGuiWindowFlags_NoNav
				);
			}

			if (extended)
				tail.animation = anim;
			else
				tail.animation.clear();
		}
		tail.index = i + 1; // Advance for inserting.

		if (extended) {
			const ImVec2 uv0((float)area.xMin() / tex->width(), (float)(area.yMin()) / tex->height());
			const ImVec2 uv1((float)(area.xMax() + 1) / tex->width(), (float)(area.yMax() + 1) / tex->height());

			ImGui::PushID(i);

			const bool hasCursor = showCursor && cursor && cursor->animation == anim && cursor->index == i;
			if (!anim || !(*anim)) // Not first frame.
				ImGui::SameLine();
			if (!hasCursor) {
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 1));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 1));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 1));
			}
			if (ImGui::ImageButton(tex->pointer(rnd), itemSize, uv0, uv1)) {
				result = true;

				if (cursor) {
					cursor->animation = anim;
					cursor->index = i;
				}
			}
			if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
				if (changeInterval)
					changeInterval(Frame(anim, i));
			}
			if (!hasCursor) {
				ImGui::PopStyleColor(3);
			}

			ImGui::PopID();
		}
	}

	if (tobeRemoved) {
		if (removeAnim)
			removeAnim(tobeRemoved);
	}
	if (tobeRenamed) {
		if (renameAnim)
			renameAnim(tobeRenamed);
	}
	addToTail(tail, itemSize, inserter);
	if (!inserter.empty() && insertFrame)
		insertFrame(inserter);
	if (ImGui::Button(" + ", ImVec2(width, 0))) {
		if (addAnim)
			addAnim();
	}
	if (ImGui::IsItemHovered()) {
		VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

		ImGui::SetTooltip(ws->theme()->tooltipEditing_AddAnimation());
	}

	return result;
}

bool sprite(
	Renderer* rnd,
	Workspace* ws,
	const Sprite* ptr,
	float /* width */, float scale,
	Frame* cursor, bool showCursor,
	const Text::Array &anims,
	std::string* focus,
	const Sprite* player,
	SpriteAnimationAddingHandler addAnim, SpriteAnimationRemovingHandler removeAnim, SpriteAnimationRenamingHandler renameAnim,
	SpriteFrameInsertingHandler insertFrame, SpriteFrameRemovingHandler removeFrame,
	SpriteFrameIntervalChangingHandler changeInterval
) {
	ImGuiStyle &style = ImGui::GetStyle();

	bool result = false;
	const bool focusingWnd = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

	const ImVec2 contentSize =  ImGui::GetContentRegionAvail();

	const float minSize = ImGui::GetFontSize() * 3 * scale;
	float itemWidth = ImGui::GetFontSize() * 4 * scale;
	float itemHeight = itemWidth * ((float)ptr->height() / ptr->width());
	if (itemWidth < minSize) {
		itemWidth = minSize;
		itemHeight = minSize * ((float)ptr->height() / ptr->width());
	}
	if (itemHeight < minSize) {
		itemWidth = minSize * ((float)ptr->width() / ptr->height());
		itemHeight = minSize;
	}
	const ImVec2 itemSize(itemWidth, itemHeight);

	const float previewHeight = contentSize.y - ImGui::TabBarHeight() - (itemHeight + (style.FramePadding.y + style.ItemInnerSpacing.y) * 2 + style.ScrollbarSize);

	const ImVec2 cursorPos = ImGui::GetCursorPos();

	do {
		if (!cursor)
			break;

		if (previewHeight <= 0)
			break;

		Math::Recti area;
		Texture::Ptr tex = nullptr;
		if (!player || !player->get(cursor->index, &tex, &area, nullptr, nullptr)) {
			if (!ptr->get(cursor->index, &tex, &area, nullptr, nullptr))
				break;
		}

		ImVec2 dstPos = cursorPos;
		ImVec2 dstSize(contentSize.x - style.WindowBorderSize, previewHeight);
		const float srcRatio = (float)area.width() / area.height();
		const float dstRatio = dstSize.x / dstSize.y;
		if (srcRatio < dstRatio) {
			const float w = dstSize.x;
			dstSize.x = dstSize.y * srcRatio;
			dstSize.x -= style.WindowBorderSize * 2;
			dstSize.y -= style.WindowBorderSize * 2;
			dstPos.x += (w - dstSize.x) * 0.5f;
		} else if (srcRatio > dstRatio) {
			const float h = dstSize.y;
			dstSize.y = dstSize.x / srcRatio;
			dstSize.x -= style.WindowBorderSize * 2;
			dstSize.y -= style.WindowBorderSize * 2;
			dstPos.x += style.WindowBorderSize;
			dstPos.y += (h - dstSize.y) * 0.5f;
		} else {
			dstSize.x -= style.WindowBorderSize * 2;
			dstSize.y -= style.WindowBorderSize * 2;
			dstPos.x += style.WindowBorderSize;
		}

		const ImVec2 uv0((float)area.xMin() / tex->width(), (float)(area.yMin()) / tex->height());
		const ImVec2 uv1((float)(area.xMax() + 1) / tex->width(), (float)(area.yMax() + 1) / tex->height());

		ImGui::PushID(cursor);

		ImGui::SetCursorPos(dstPos);
		ImGui::Image(tex->pointer(rnd), dstSize, uv0, uv1, ImVec4(1, 1, 1, 1), ImVec4(0, 0, 0, 1));
		ImGui::SetCursorPos(cursorPos);

		ImGui::PopID();
	} while (false);

	Frame inserter;
	const char* tobeRemoved = nullptr;
	const char* tobeRenamed = nullptr;

	ImGui::PushStyleColor(ImGuiCol_Tab, ws->theme()->style()->builtin[ImGuiCol_Tab]);
	ImGui::PushStyleColor(ImGuiCol_TabHovered, ws->theme()->style()->builtin[ImGuiCol_TabHovered]);
	ImGui::PushStyleColor(ImGuiCol_TabActive, ws->theme()->style()->builtin[ImGuiCol_TabActive]);

	ImGui::SetCursorPos(cursorPos + ImVec2(0, previewHeight));
	const ImGuiTabBarFlags tabBarFlags = ImGuiTabBarFlags_NoTooltip;
	if (ImGui::BeginTabBar("@Spr/Tls/Vu/Tab", tabBarFlags)) {
		const char* selected = nullptr;

		const ImGuiTabItemFlags tabItemFlags = ImGuiTabItemFlags_None;
		for (const std::string &key : anims) {
			const char* anim = key.c_str();
			const Sprite::Range range = ptr->rangeOf(anim);
			const int beginIdx = std::get<0>(range);
			const int endIdx = std::get<1>(range);
			if ((!anim || !(*anim)) && beginIdx == 0)
				anim = EDITING_ANONYMOUS;

			if (focus && focus->empty())
				*focus = anim;

			std::string tab = anim;
			tab += "     ";
			if (ImGui::BeginTabItem(tab, nullptr, tabItemFlags | (focus && *focus == anim ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None), ws->theme()->style()->tabTextColor)) {
				const ImGui::Rect lastRect = ImGui::LastItemRect();
				if (ImGui::IsMouseHoveringRect(lastRect.first, lastRect.second)) {
					const float padR = style.FramePadding.x;
					const float buttonSz = ImGui::GetFontSize();
					ImVec2 customBtnPos(
						lastRect.second.x,
						lastRect.first.y
					);
					customBtnPos.x = customBtnPos.x - (padR + buttonSz) - style.FramePadding.x;
					if (ImGui::CustomButton("#Dl", &customBtnPos, ImGui::CustomCloseButton, ws->theme()->tooltipEditing_DeleteAnimation().c_str()) && focusingWnd) {
						tobeRemoved = anim;
					}
					if (ImGui::CustomButton("#Rn", &customBtnPos, ImGui::CustomRenameButton, ws->theme()->tooltipEditing_RenameAnimation().c_str()) && focusingWnd) {
						tobeRenamed = anim;
					}
				}

				selected = anim;

				ImGui::BeginChildFrame(
					ImGui::GetID(anim),
					ImVec2(0, 0),
					ImGuiWindowFlags_AlwaysHorizontalScrollbar | ImGuiWindowFlags_NoNav
				);

				for (int i = beginIdx; i <= endIdx; ++i) {
					Math::Recti area;
					double interval = 0;
					Texture::Ptr tex = nullptr;
					ptr->get(i, &tex, &area, &interval, nullptr);

					const ImVec2 uv0((float)area.xMin() / tex->width(), (float)(area.yMin()) / tex->height());
					const ImVec2 uv1((float)(area.xMax() + 1) / tex->width(), (float)(area.yMax() + 1) / tex->height());

					ImGui::PushID(i);

					const bool hasCursor = showCursor && cursor && cursor->animation == anim && cursor->index == i;
					if (i != beginIdx) // Not first frame.
						ImGui::SameLine();
					if (!hasCursor) {
						ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 1));
						ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0, 0, 0, 1));
						ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0, 0, 0, 1));
					}
					if (ImGui::ImageButton(tex->pointer(rnd), itemSize, uv0, uv1)) {
						result = true;

						if (cursor) {
							cursor->animation = anim;
							cursor->index = i;
						}
					}
					if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
						if (changeInterval)
							changeInterval(Frame(anim, i));
					}
					if (!hasCursor) {
						ImGui::PopStyleColor(3);
					}

					ImGui::PopID();
				}

				ImGui::SameLine();
				if (ImGui::Button("+", itemSize + style.FramePadding * 2)) {
					inserter.animation = anim;
					inserter.index = endIdx + 1; // Advance for inserting.

					ImGui::SetScrollX(ImGui::GetScrollMaxX());
				}

				if (ImGui::IsItemHovered()) {
					VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

					ImGui::SetTooltip(ws->theme()->tooltipEditing_AddFrame());
				}

				ImGui::EndChildFrame();

				ImGui::EndTabItem();
			}

			const ImGui::Rect lastRect = ImGui::LastItemRect();
			if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsMouseHoveringRect(lastRect.first, lastRect.second)/* && focusingWnd */) {
				if (focus)
					*focus = anim;
			}
		}

		if (focus) {
			if (ImGui::BeginTabItem(" + ", nullptr, tabItemFlags | ImGuiTabItemFlags_NoCloseWithMiddleMouseButton))
				ImGui::EndTabItem();

			if (ImGui::IsItemHovered()) {
				VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

				ImGui::SetTooltip(ws->theme()->tooltipEditing_AddAnimation());
			}
		}
		const ImGui::Rect lastRect = ImGui::LastItemRect();
		if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && ImGui::IsMouseHoveringRect(lastRect.first, lastRect.second) && focusingWnd) {
			if (focus) {
				if (selected)
					*focus = selected;
				else
					focus->clear();
			}

			if (addAnim)
				addAnim();
		}

		ImGui::EndTabBar();
	}

	ImGui::PopStyleColor(3);

	if (tobeRemoved) {
		if (removeAnim)
			removeAnim(tobeRemoved);
	}
	if (tobeRenamed) {
		if (renameAnim)
			renameAnim(tobeRenamed);
	}
	if (!inserter.empty() && insertFrame)
		insertFrame(inserter);

	return result;
}

bool map(
	Renderer* rnd,
	Workspace* ws,
	const Map* ptr, const Math::Vec2i &tileSize,
	float width,
	Tiler* cursor, bool showCursor,
	const Tiler* selection,
	const Math::Vec2i* showGrids,
	bool showTransparentBackbround,
	MapCelGetter getCel,
	int mouseActionButton
) {
	ImDrawList* drawList = ImGui::GetWindowDrawList();

	bool result = false;
	const bool focusingWnd = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

	float widgetWidth = (float)ptr->width();
	float widgetHeight = (float)ptr->height();
	float scale = 1;
	if (width > 0 && std::abs(widgetWidth * tileSize.x - width) >= Math::EPSILON<float>()) {
		scale = width / (widgetWidth * tileSize.x);
		widgetHeight = widgetHeight / (widgetWidth * tileSize.x) * width;
		widgetWidth = width / tileSize.x;
	}
	const ImVec2 widgetSize(widgetWidth * tileSize.x, widgetHeight * tileSize.y);

	const float scrollX = ImGui::GetScrollX() / scale;
	const float scrollY = ImGui::GetScrollY() / scale;

	const ImVec2 content = ImGui::GetContentRegionAvail();
	const int beginX = Math::clamp((int)(scrollX / tileSize.x), 0, ptr->width());
	const int beginY = Math::clamp((int)(scrollY / tileSize.y), 0, ptr->height());
	const int endX = Math::clamp((int)std::ceil((scrollX + content.x) / tileSize.x), 0, ptr->width());
	const int endY = Math::clamp((int)std::ceil((scrollY + content.y) / tileSize.y), 0, ptr->height());

	ImGui::BeginChild("@Map/Pat/Cels", widgetSize + ImVec2(2, 1), false, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoNav);
	{
		ImVec2 curPos = ImGui::GetCursorScreenPos();
		curPos.x += 1;
		const bool hoveringWnd = ImGui::IsMouseHoveringRect(curPos, curPos + widgetSize);

		drawList->AddRect(curPos, curPos + widgetSize, ImGui::GetColorU32(ImVec4(1, 1, 1, 0.5f)));

		for (int j = beginY; j < endY; ++j) {
			for (int i = beginX; i < endX; ++i) {
				Math::Recti area;
				Texture::Ptr tex = nullptr;
				if (getCel) {
					const int idx = getCel(Math::Vec2i(i, j));
					if (idx != Map::INVALID())
						tex = ptr->at(idx, &area);
				}
				if (!tex)
					tex = ptr->at(i, j, &area);
				if (!tex)
					continue;

				const ImVec2 pos = ImVec2(i * (float)area.width() * scale, j * (float)area.height() * scale) + curPos;
				const ImVec2 sz = ImVec2((float)area.width(), (float)area.height()) * scale;
				const ImRect rect(pos, pos + sz);
				const ImVec2 uv0((float)area.xMin() / tex->width(), (float)(area.yMin()) / tex->height());
				const ImVec2 uv1((float)(area.xMax() + 1) / tex->width(), (float)(area.yMax() + 1) / tex->height());
				if (showTransparentBackbround)
					drawList->AddImage(ws->theme()->iconTransparent()->pointer(rnd), rect.Min, rect.Max, ImVec2(0, 0), ImVec2(1, 1));
				drawList->AddImage(tex->pointer(rnd), rect.Min, rect.Max, uv0, uv1);

				if (hoveringWnd && focusingWnd && !result) {
					const ImVec2 mousePos = ImGui::GetMousePos();
					if (rect.Contains(mousePos)) {
						if (ImGui::IsMouseDown(mouseActionButton))
							result = true;

						if (cursor)
							cursor->position = Math::Vec2i(i, j);
					}
				}
			}
		}

		if (showGrids) {
			for (int i = beginX; i <= endX; ++i) {
				drawList->AddLine(
					curPos + ImVec2(i * (float)tileSize.x * scale, 0),
					curPos + ImVec2(i * (float)tileSize.x * scale, widgetHeight * (float)tileSize.y),
					ImGui::GetColorU32(ImVec4(1, 1, 1, 0.75f))
				);
			}
			for (int j = beginY; j <= endY; ++j) {
				drawList->AddLine(
					curPos + ImVec2(0, j * (float)tileSize.y * scale),
					curPos + ImVec2(widgetWidth * (float)tileSize.x, j * (float)tileSize.y * scale),
					ImGui::GetColorU32(ImVec4(1, 1, 1, 0.75f))
				);
			}
		}

		if (showCursor && cursor) {
			const Math::Recti rect = Math::Recti::byXYWH(cursor->position.x, cursor->position.y, cursor->size.x, cursor->size.y);
			const Int cursorX = rect.xMin();
			const Int cursorY = rect.yMin();
			const bool active = Math::intersects(Math::Recti(0, 0, ptr->width(), ptr->height()), rect);
			if (active) {
				ImVec2 blinkPos((float)cursorX, (float)cursorY);
				blinkPos *= scale;
				blinkPos.x *= tileSize.x;
				blinkPos.y *= tileSize.y;
				blinkPos += curPos;
				ImVec2 blinkSize((float)rect.width(), (float)rect.height());
				blinkSize *= scale;
				blinkSize.x *= tileSize.x;
				blinkSize.y *= tileSize.y;
				float blinkThick = 3;
				if (blinkSize.x <= 3) {
					blinkPos.x -= 1;
					blinkSize.x += 2;
					blinkThick = 1;
				}
				if (blinkSize.y <= 3) {
					blinkPos.y -= 1;
					blinkSize.y += 2;
					blinkThick = 1;
				}
				ImGui::Indicator(blinkPos, blinkPos + blinkSize, blinkThick);
			}
		}

		if (selection) {
			const Math::Recti rect = Math::Recti::byXYWH(selection->position.x, selection->position.y, selection->size.x, selection->size.y);
			const Int selX = selection->position.x;
			const Int selY = selection->position.y;
			const bool active = Math::intersects(Math::Recti(0, 0, ptr->width(), ptr->height()), rect);
			if (active) {
				ImVec2 blinkPos((float)selX, (float)selY);
				blinkPos *= scale;
				blinkPos.x *= tileSize.x;
				blinkPos.y *= tileSize.y;
				blinkPos += curPos;
				ImVec2 blinkSize((float)selection->size.x, (float)selection->size.y);
				blinkSize *= scale;
				blinkSize.x *= tileSize.x;
				blinkSize.y *= tileSize.y;
				float blinkThick = 3;
				if (blinkSize.x <= 3) {
					blinkPos.x -= 1;
					blinkSize.x += 2;
					blinkThick = 1;
				}
				if (blinkSize.y <= 3) {
					blinkPos.y -= 1;
					blinkSize.y += 2;
					blinkThick = 1;
				}
				ImGui::Indicator(blinkPos, blinkPos + blinkSize, blinkThick);
			}
		}
	}
	ImGui::EndChild();

	return result;
}

bool sound(
	Renderer* rnd,
	Workspace* ws,
	Sound* ptr,
	float width, float height,
	Tuner* cursor, bool showCursor,
	Tuner::Commands* cmd,
	const float* columns, size_t columnCount
) {
	ImGuiIO &io = ImGui::GetIO();
	ImGuiStyle &style = ImGui::GetStyle();
	ImDrawList* drawList = ImGui::GetWindowDrawList();

	bool result = false;

	if (cmd)
		*cmd = Tuner::NONE;

	const ImVec2 contentSize =  ImGui::GetContentRegionAvail();
	if (width <= 0)
		width = contentSize.x;
	if (height <= 0)
		height = contentSize.y;

	const ImVec2 buttonSize(13 * io.FontGlobalScale, 13 * io.FontGlobalScale);
	const float columnsAreaHeight = height - (buttonSize.y + style.ItemInnerSpacing.y * 2);

	const ImVec4 col = ImGui::GetStyleColorVec4(ImGuiCol_TitleBgActive);
	const ImVec2 pos = ImGui::GetCursorScreenPos();
	drawList->AddLine(pos + ImVec2(0, columnsAreaHeight * 0.5f), pos + ImVec2(width, columnsAreaHeight * 0.5f), ImGui::GetColorU32(col));
	if (columns && columnCount > 0) {
		const float itemPadding = width / columnCount * 0.1f;
		const float itemWidth = (width - itemPadding * (columnCount - 1)) / columnCount;
		float x0 = 0;
		float x1 = x0 + itemWidth;
		for (size_t i = 0; i < columnCount; ++i) {
			const ImVec2 p0(x0, columnsAreaHeight * 0.5f - columnsAreaHeight * 0.5f * columns[i]);
			const ImVec2 p1(x1, columnsAreaHeight * 0.5f);
			drawList->AddRectFilled(pos + p0, pos + p1, ImGui::GetColorU32(col));

			x0 += itemWidth + itemPadding;
			x1 += itemWidth + itemPadding;
		}
	}

	ImGui::SetCursorScreenPos(pos);
	if (ptr->title()) {
		const char* txt = *ptr->title() ? ptr->title() : ws->theme()->generic_Unknown().c_str();
		ImGui::Text("%s %s", ws->theme()->statusItemSound_Title().c_str(), txt);
	}
	if (ptr->artist() && *ptr->artist()) {
		const char* txt = ptr->artist();
		ImGui::Text("%s %s", ws->theme()->statusItemSound_Artist().c_str(), txt);
	}
	if (ptr->album() && *ptr->album()) {
		const char* txt = ptr->album();
		ImGui::Text("%s %s", ws->theme()->statusItemSound_Album().c_str(), txt);
	}
	if (ptr->copyright() && *ptr->copyright()) {
		const char* txt = ptr->copyright();
		ImGui::Text("%s %s", ws->theme()->statusItemSound_Copyright().c_str(), txt);
	}

	ImGui::SetCursorScreenPos(pos + ImVec2(0, columnsAreaHeight));
	if (ImGui::ImageButton(ws->theme()->slicePlay()->pointer(rnd), buttonSize, ImGui::ColorConvertU32ToFloat4(ws->theme()->style()->iconColor))) {
		if (cmd)
			*cmd = Tuner::PLAY;
	}
	ImGui::SameLine();
	if (ImGui::ImageButton(ws->theme()->sliceStop()->pointer(rnd), buttonSize, ImGui::ColorConvertU32ToFloat4(ws->theme()->style()->iconColor))) {
		if (cmd)
			*cmd = Tuner::STOP;
	}

	if (cursor && showCursor) {
		ImGui::SameLine();
		ImGui::SetNextItemWidth(width - (buttonSize.x + style.ItemInnerSpacing.x * 2) * 2);

		ImGui::PushID(cursor);

		char fmt[32];
		snprintf(fmt, BITTY_COUNTOF(fmt), "%%.3f/%.3f", cursor->length);
		float curr = 0;
		if (ptr->playing()) {
			curr = (float)cursor->position;
			if (ImGui::ProgressBar("", &curr, 0, (float)cursor->length, fmt)) {
				result = true;

				cursor->position = curr;
			} else {
				cursor->position = ptr->position();
			}
		} else {
			ImGui::ProgressBar("", &curr, 0, (float)cursor->length, fmt, true);
		}

		ImGui::PopID();
	}

	return result;
}

namespace Tools {

Marker::Coordinates::Coordinates() {
}

Marker::Coordinates::Coordinates(int ln, int col) : line(ln), column(col) {
}

Marker::Coordinates::Coordinates(int idx, int ln, int col) : index(idx), line(ln), column(col) {
}

bool Marker::Coordinates::operator == (const Coordinates &other) const {
	return compare(other) == 0;
}

bool Marker::Coordinates::operator < (const Coordinates &other) const {
	return compare(other) < 0;
}

bool Marker::Coordinates::operator > (const Coordinates &other) const {
	return compare(other) > 0;
}

int Marker::Coordinates::compare(const Coordinates &other) const {
	if (index < other.index)
		return -1;
	else if (index > other.index)
		return 1;

	if (line < other.line)
		return -1;
	else if (line > other.line)
		return 1;

	if (column < other.column)
		return -1;
	else if (column > other.column)
		return 1;

	return 0;
}

bool Marker::Coordinates::empty(void) const {
	return index == -1 && line == -1 && column == -1;
}

void Marker::Coordinates::clear(void) {
	index = -1;
	line = -1;
	column = -1;
}

Marker::Marker() {
}

Marker::Marker(const Coordinates &begin_, const Coordinates &end_) : begin(begin_), end(end_) {
}

const Marker::Coordinates &Marker::min(void) const {
	if (begin < end)
		return begin;

	return end;
}

const Marker::Coordinates &Marker::max(void) const {
	if (begin > end)
		return begin;

	return end;
}

bool Marker::empty(void) const {
	return begin.empty() || end.empty();
}

void Marker::clear(void) {
	begin.clear();
	end.clear();
}

bool colorable(
	Renderer*,
	Workspace* ws,
	Color colors[EDITING_ITEM_COUNT_PER_LINE * 2],
	int* cursor,
	float width,
	bool allowShortcuts
) {
	ImGuiStyle &style = ImGui::GetStyle();

	Theme* theme = ws->theme();

	bool result = false;
	const bool focusingWnd = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

	if (width <= 0)
		width = ImGui::GetContentRegionAvail().x;
	constexpr const int X_COUNT = EDITING_ITEM_COUNT_PER_LINE;
	float xOffset = 0;
	float size = width / X_COUNT;
	const float iconSize = (float)theme->iconMove()->width(); // Borrowed.
	if (size > iconSize) {
		size = iconSize * (int)(size / iconSize);
		xOffset = (width - size * X_COUNT) * 0.5f;
	}

	constexpr const char* const TOOLTIPS[] = {
		"(1)",
		"(2)",
		"(3)",
		"(4)",
		"(5)",
		"(6)",
		"(7)",
		"(8)",
		"(9)",
		"(0)"
	};
	const Shortcut shortcuts[] = {
		Shortcut(SDL_SCANCODE_1),
		Shortcut(SDL_SCANCODE_2),
		Shortcut(SDL_SCANCODE_3),
		Shortcut(SDL_SCANCODE_4),
		Shortcut(SDL_SCANCODE_5),
		Shortcut(SDL_SCANCODE_6),
		Shortcut(SDL_SCANCODE_7),
		Shortcut(SDL_SCANCODE_8),
		Shortcut(SDL_SCANCODE_9),
		Shortcut(SDL_SCANCODE_0)
	};

	VariableGuard<decltype(style.FramePadding)> guardFramePadding(&style.FramePadding, style.FramePadding, ImVec2());
	VariableGuard<decltype(style.ItemInnerSpacing)> guardItemInnerSpacing(&style.ItemInnerSpacing, style.ItemInnerSpacing, ImVec2());

	for (int i = 0; i < EDITING_ITEM_COUNT_PER_LINE * 2; ++i) {
		if (xOffset > 0 && (i % X_COUNT) == 0) {
			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
		}

		const Color &color = colors[i];
		const ImVec4 col4v(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f);

		ImGui::PushID(i);

		const ImVec2 pos = ImGui::GetCursorScreenPos();

		if (ImGui::ColorButton("", col4v, ImGuiColorEditFlags_NoDragDrop | ImGuiColorEditFlags_AlphaPreview, ImVec2(size, size), TOOLTIPS[i]) || (allowShortcuts && focusingWnd && shortcuts[i].pressed())) {
			result = true;

			if (cursor)
				*cursor = i;
		}

		if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
			ImGui::OpenPopup("@Col/Pikr");
		}

		VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(8, 8));

		if (ImGui::BeginPopup("@Col/Pikr")) {
			VariableGuard<decltype(style.ItemSpacing)> guardItemSpacing(&style.ItemSpacing, style.ItemSpacing, ImVec2(8, 4));

			const Color col = colors[i];
			float col4f[] = {
				Math::clamp(col.r / 255.0f, 0.0f, 1.0f),
				Math::clamp(col.g / 255.0f, 0.0f, 1.0f),
				Math::clamp(col.b / 255.0f, 0.0f, 1.0f),
				Math::clamp(col.a / 255.0f, 0.0f, 1.0f)
			};

			if (ImGui::ColorPicker4("", col4f, ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_NoSidePreview)) {
				const Color value(
					(Byte)(col4f[0] * 255),
					(Byte)(col4f[1] * 255),
					(Byte)(col4f[2] * 255),
					(Byte)(col4f[3] * 255)
				);
				colors[i] = value;
			}

			ImGui::EndPopup();
		}

		if (cursor && *cursor == i)
			ImGui::Indicator(pos, pos + ImVec2(size, size));

		if (((i + 1) % X_COUNT) != 0 && i != EDITING_ITEM_COUNT_PER_LINE * 2 - 1)
			ImGui::SameLine();

		ImGui::PopID();
	}

	return result;
}

bool paintable(
	Renderer* rnd,
	Workspace* ws,
	Tools* cursor,
	float width,
	bool allowShortcuts,
	unsigned mask
) {
	ImGuiStyle &style = ImGui::GetStyle();

	Theme* theme = ws->theme();

	bool result = false;
	const bool focusingWnd = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

	if (width <= 0)
		width = ImGui::GetContentRegionAvail().x;
	constexpr const int X_COUNT = EDITING_ITEM_COUNT_PER_LINE;
	float xOffset = 0;
	float size = width / X_COUNT;
	const float iconSize = (float)theme->iconMove()->width();
	if (size > iconSize) {
		size = iconSize * (int)(size / iconSize);
		xOffset = (width - size * X_COUNT) * 0.5f;
	}

	const ImTextureID icons[] = {
		theme->iconMove()->pointer(rnd),
		theme->iconEyedropper()->pointer(rnd),
		theme->iconPencil()->pointer(rnd),
		theme->iconPaintbucket()->pointer(rnd),
		theme->iconLasso()->pointer(rnd),
		theme->iconLine()->pointer(rnd),
		theme->iconBox()->pointer(rnd),
		theme->iconBox_Fill()->pointer(rnd),
		theme->iconEllipse()->pointer(rnd),
		theme->iconEllipse_Fill()->pointer(rnd),
		theme->iconStamp()->pointer(rnd)
	};
	constexpr const char* const TOOLTIPS[] = {
		"(H)",
		"(I)",
		"(B)",
		"(G)",
		"(M)",
		"(L)",
		"(X)",
		"(Shift+X)",
		"(E)",
		"(Shift+E)",
		"(S)"
	};
	const Shortcut shortcuts[] = {
		Shortcut(SDL_SCANCODE_H),
		Shortcut(SDL_SCANCODE_I),
		Shortcut(SDL_SCANCODE_B),
		Shortcut(SDL_SCANCODE_G),
		Shortcut(SDL_SCANCODE_M),
		Shortcut(SDL_SCANCODE_L),
		Shortcut(SDL_SCANCODE_X),
		Shortcut(SDL_SCANCODE_X, false, true),
		Shortcut(SDL_SCANCODE_E),
		Shortcut(SDL_SCANCODE_E, false, true),
		Shortcut(SDL_SCANCODE_S)
	};
	const ImU32 colors[] = {
		IM_COL32_WHITE,
		IM_COL32_WHITE,
		IM_COL32_WHITE,
		IM_COL32_WHITE,
		IM_COL32_WHITE,
		ws->theme()->style()->iconColor,
		ws->theme()->style()->iconColor,
		ws->theme()->style()->iconColor,
		ws->theme()->style()->iconColor,
		ws->theme()->style()->iconColor,
		IM_COL32_WHITE
	};

	VariableGuard<decltype(style.FramePadding)> guardFramePadding(&style.FramePadding, style.FramePadding, ImVec2());

	for (int i = 0, j = 0; i < BITTY_COUNTOF(icons); ++i) {
		if ((mask & (1 << i)) == 0)
			continue;

		if (xOffset > 0 && (j % X_COUNT) == 0) {
			ImGui::Dummy(ImVec2(xOffset, 0));
			ImGui::SameLine();
		}

		const bool selected = cursor && *cursor == i;
		if (ImGui::ImageButton(icons[i], ImVec2(size, size), ImGui::ColorConvertU32ToFloat4(colors[i]), selected, TOOLTIPS[i]) || (allowShortcuts && focusingWnd && shortcuts[i].pressed())) {
			result = true;

			if (cursor)
				*cursor = (Tools)i;
		}

		if (((j + 1) % X_COUNT) != 0 && i != BITTY_COUNTOF(icons) - 1)
			ImGui::SameLine();

		++j;
	}

	return result;
}

bool magnifiable(
	Renderer* rnd,
	Workspace* ws,
	int* cursor,
	float width,
	bool allowShortcuts
) {
	ImGuiIO &io = ImGui::GetIO();
	ImGuiStyle &style = ImGui::GetStyle();
	ImDrawList* drawList = ImGui::GetWindowDrawList();

	Theme* theme = ws->theme();

	bool result = false;
	const bool focusingWnd = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

	if (width <= 0)
		width = ImGui::GetContentRegionAvail().x;
	constexpr const int X_COUNT = EDITING_ITEM_COUNT_PER_LINE;
	float xOffset = 0;
	float size = width / X_COUNT;
	const float iconSize = (float)theme->iconMagnify()->width();
	if (size > iconSize) {
		size = iconSize * (int)(size / iconSize);
		xOffset = (width - size * X_COUNT) * 0.5f;
	}

	const char* const TOOLTIP = theme->tooltipEditing_Magnifiable().c_str();
	const Shortcut shortcuts[] = {
		Shortcut(SDL_SCANCODE_MINUS),
		Shortcut(SDL_SCANCODE_EQUALS)
	};

	VariableGuard<decltype(style.FramePadding)> guardFramePadding(&style.FramePadding, style.FramePadding, ImVec2());

	ImGui::Dummy(ImVec2(xOffset, 0));
	ImGui::SameLine();

	const ImVec4 btnCol = ImGui::GetStyleColorVec4(ImGuiCol_ChildBg);
	ImGui::PushStyleColor(ImGuiCol_Button, btnCol);
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, btnCol);
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, btnCol);

	if (ImGui::ImageButton(theme->iconMagnify()->pointer(rnd), ImVec2(size, size), ImColor(IM_COL32_WHITE), false, TOOLTIP) && focusingWnd) {
		result = true;

		if (cursor) {
			if (++*cursor >= 4)
				*cursor = 0;
		}
	}
	constexpr const int MIN_ZOOM = 0;
	constexpr const int MAX_ZOOM = 3;
#if WORKSPACE_MODIFIER_KEY == WORKSPACE_MODIFIER_KEY_CTRL
	const bool modifier = io.KeyCtrl;
#elif WORKSPACE_MODIFIER_KEY == WORKSPACE_MODIFIER_KEY_CMD
	const bool modifier = io.KeySuper;
#endif /* WORKSPACE_MODIFIER_KEY */
	const bool zoomOut = shortcuts[0].pressed() || (*cursor > MIN_ZOOM && modifier && io.MouseWheel < 0);
	const bool zoomIn = shortcuts[1].pressed() || (*cursor < MAX_ZOOM && modifier && io.MouseWheel > 0);
	if (allowShortcuts && focusingWnd && zoomOut) {
		result = true;

		if (cursor) {
			if (--*cursor < 0)
				*cursor = 3;
		}
	} else if (allowShortcuts && focusingWnd && zoomIn) {
		result = true;

		if (cursor) {
			if (++*cursor >= 4)
				*cursor = 0;
		}
	}

	ImGui::PopStyleColor(3);

	ImGui::SameLine();

	const ImU32 col = ImGui::GetColorU32(ImGuiCol_Text);
	const ImVec2 pos = ImGui::GetCursorScreenPos();
	drawList->PathLineTo(pos + ImVec2(1, 1));
	drawList->PathLineTo(pos + ImVec2(size * 4 - 1, 1));
	drawList->PathLineTo(pos + ImVec2(size * 4 - 1, size - 1));
	drawList->PathLineTo(pos + ImVec2(1, size - 1));
	drawList->PathStroke(col, true);

	if (cursor && *cursor >= 0 && *cursor <= 3) {
		drawList->PathLineTo(pos + ImVec2(size * *cursor, 1));
		drawList->PathLineTo(pos + ImVec2(size * (*cursor + 1), 1));
		drawList->PathLineTo(pos + ImVec2(size * (*cursor + 1), size - 1));
		drawList->PathLineTo(pos + ImVec2(size * *cursor, size - 1));
		drawList->PathFillConvex(col);

		Texture* texs[] = {
			theme->sliceNumber_1(),
			theme->sliceNumber_2(),
			theme->sliceNumber_3(),
			theme->sliceNumber_4()
		};
		Texture* tex = texs[*cursor];
		drawList->AddImage(
			tex->pointer(rnd),
			ImVec2(pos + ImVec2(size * *cursor, 1)),
			ImVec2(pos + ImVec2(size * (*cursor + 1), size - 1)),
			ImVec2(0, 0), ImVec2(1, 1),
			ImGui::GetColorU32(ImGuiCol_FrameBg)
		);

		const ImRect rects[] = {
			ImRect(
				ImVec2(pos + ImVec2(0, 1)),
				ImVec2(pos + ImVec2(size, size - 1))
			),
			ImRect(
				ImVec2(pos + ImVec2(size, 1)),
				ImVec2(pos + ImVec2(size * 2, size - 1))
			),
			ImRect(
				ImVec2(pos + ImVec2(size * 2, 1)),
				ImVec2(pos + ImVec2(size * 3, size - 1))
			),
			ImRect(
				ImVec2(pos + ImVec2(size * 3, 1)),
				ImVec2(pos + ImVec2(size * 4, size - 1))
			)
		};
		for (int i = 0; i < BITTY_COUNTOF(rects); ++i) {
			if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && ImGui::IsMouseHoveringRect(rects[i].Min, rects[i].Max, false) && focusingWnd) {
				result = true;

				*cursor = i;

				break;
			}
		}
	}

	ImGui::NewLine();

	return result;
}

bool weighable(
	Renderer* rnd,
	Workspace* ws,
	int* cursor,
	float width,
	bool allowShortcuts
) {
	ImGuiStyle &style = ImGui::GetStyle();
	ImDrawList* drawList = ImGui::GetWindowDrawList();

	Theme* theme = ws->theme();

	bool result = false;
	const bool focusingWnd = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

	if (width <= 0)
		width = ImGui::GetContentRegionAvail().x;
	constexpr const int X_COUNT = EDITING_ITEM_COUNT_PER_LINE;
	float xOffset = 0;
	float size = width / X_COUNT;
	const float iconSize = (float)theme->iconPencils()->width();
	if (size > iconSize) {
		size = iconSize * (int)(size / iconSize);
		xOffset = (width - size * X_COUNT) * 0.5f;
	}

	constexpr const char* const TOOLTIP = "([/])";
	const Shortcut shortcuts[] = {
		Shortcut(SDL_SCANCODE_LEFTBRACKET),
		Shortcut(SDL_SCANCODE_RIGHTBRACKET)
	};

	VariableGuard<decltype(style.FramePadding)> guardFramePadding(&style.FramePadding, style.FramePadding, ImVec2());

	ImGui::Dummy(ImVec2(xOffset, 0));
	ImGui::SameLine();

	const ImVec4 btnCol = ImGui::GetStyleColorVec4(ImGuiCol_ChildBg);
	ImGui::PushStyleColor(ImGuiCol_Button, btnCol);
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, btnCol);
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, btnCol);

	if (ImGui::ImageButton(theme->iconPencils()->pointer(rnd), ImVec2(size, size), ImColor(IM_COL32_WHITE), false, TOOLTIP) && focusingWnd) {
		result = true;

		if (cursor) {
			if (++*cursor >= 4)
				*cursor = 0;
		}
	}
	if (allowShortcuts && focusingWnd && shortcuts[0].pressed()) {
		result = true;

		if (cursor) {
			if (--*cursor < 0)
				*cursor = 3;
		}
	} else if (allowShortcuts && focusingWnd && shortcuts[1].pressed()) {
		result = true;

		if (cursor) {
			if (++*cursor >= 4)
				*cursor = 0;
		}
	}

	ImGui::PopStyleColor(3);

	ImGui::SameLine();

	const ImU32 col = ImGui::GetColorU32(ImGuiCol_Text);
	const ImVec2 pos = ImGui::GetCursorScreenPos();
	const float xOffsets[] = { 1, size, size * 2, size * 3, size * 4 };
	const float yOffsets[] = { size * (3.0f / 4.0f), size * (2.0f / 4.0f), size * (1.0f / 4.0f), 1, 1 };
	{
		drawList->PathLineTo(pos + ImVec2(xOffsets[0], yOffsets[0]));
		drawList->PathLineTo(pos + ImVec2(xOffsets[1], yOffsets[0]));
		drawList->PathLineTo(pos + ImVec2(xOffsets[1], yOffsets[1]));
		drawList->PathLineTo(pos + ImVec2(xOffsets[2], yOffsets[1]));
		drawList->PathLineTo(pos + ImVec2(xOffsets[2], yOffsets[2]));
		drawList->PathLineTo(pos + ImVec2(xOffsets[3], yOffsets[2]));
		drawList->PathLineTo(pos + ImVec2(xOffsets[3], yOffsets[3]));
	}
	drawList->PathLineTo(pos + ImVec2(size * 4 - 1, 1));
	drawList->PathLineTo(pos + ImVec2(size * 4 - 1, size - 1));
	drawList->PathLineTo(pos + ImVec2(1, size - 1));
	drawList->PathStroke(col, true);

	if (cursor && *cursor >= 0 && *cursor <= 3) {
		drawList->PathLineTo(pos + ImVec2(xOffsets[*cursor] - 1, yOffsets[*cursor]));
		drawList->PathLineTo(pos + ImVec2(xOffsets[*cursor + 1], yOffsets[*cursor]));
		drawList->PathLineTo(pos + ImVec2(xOffsets[*cursor + 1], size - 1));
		drawList->PathLineTo(pos + ImVec2(xOffsets[*cursor] - 1, size - 1));
		drawList->PathFillConvex(col);

		const ImRect rects[] = {
			ImRect(
				ImVec2(pos + ImVec2(0, 1)),
				ImVec2(pos + ImVec2(size, size - 1))
			),
			ImRect(
				ImVec2(pos + ImVec2(size, 1)),
				ImVec2(pos + ImVec2(size * 2, size - 1))
			),
			ImRect(
				ImVec2(pos + ImVec2(size * 2, 1)),
				ImVec2(pos + ImVec2(size * 3, size - 1))
			),
			ImRect(
				ImVec2(pos + ImVec2(size * 3, 1)),
				ImVec2(pos + ImVec2(size * 4, size - 1))
			)
		};
		for (int i = 0; i < BITTY_COUNTOF(rects); ++i) {
			if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && ImGui::IsMouseHoveringRect(rects[i].Min, rects[i].Max, false) && focusingWnd) {
				result = true;

				*cursor = i;

				break;
			}
		}
	}

	ImGui::NewLine();

	return result;
}

bool flippable(
	Renderer* rnd,
	Workspace* ws,
	RotationsAndFlippings* cursor,
	float width,
	bool allowShortcuts,
	unsigned mask
) {
	ImGuiStyle &style = ImGui::GetStyle();

	Theme* theme = ws->theme();

	bool result = false;
	const bool focusingWnd = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

	if (cursor)
		*cursor = INVALID;

	if (width <= 0)
		width = ImGui::GetContentRegionAvail().x;
	constexpr const int X_COUNT = EDITING_ITEM_COUNT_PER_LINE;
	float xOffset = 0;
	float size = width / X_COUNT;
	const float iconSize = (float)theme->iconRotate_Clockwise()->width();
	if (size > iconSize) {
		size = iconSize * (int)(size / iconSize);
		xOffset = (width - size * X_COUNT) * 0.5f;
	}

	const ImTextureID icons[] = {
		theme->iconRotate_Clockwise()->pointer(rnd),
		theme->iconRotate_Anticlockwise()->pointer(rnd),
		theme->iconRotate_HalfTurn()->pointer(rnd),
		theme->iconFlip_Vertically()->pointer(rnd),
		theme->iconFlip_Horizontally()->pointer(rnd)
	};
	constexpr const char* const TOOLTIPS[] = {
		"(,)",
		"(.)",
		"(/)",
		"(;)",
		"(')"
	};
	const Shortcut shortcuts[] = {
		Shortcut(SDL_SCANCODE_COMMA),
		Shortcut(SDL_SCANCODE_PERIOD),
		Shortcut(SDL_SCANCODE_SLASH),
		Shortcut(SDL_SCANCODE_SEMICOLON),
		Shortcut(SDL_SCANCODE_APOSTROPHE)
	};

	VariableGuard<decltype(style.FramePadding)> guardFramePadding(&style.FramePadding, style.FramePadding, ImVec2());

	ImGui::Dummy(ImVec2(xOffset, 0));
	ImGui::SameLine();

	for (int i = 0; i < BITTY_COUNTOF(icons); ++i) {
		if ((mask & (1 << i)) == 0) {
			const ImVec4 btnCol = ImGui::GetStyleColorVec4(ImGuiCol_ChildBg);
			ImGui::PushStyleColor(ImGuiCol_Button, btnCol);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, btnCol);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, btnCol);

			ImGui::ImageButton(icons[i], ImVec2(size, size), ImColor(IM_COL32(128, 128, 128, 255)), false, TOOLTIPS[i]);

			ImGui::PopStyleColor(3);
		} else {
			if (ImGui::ImageButton(icons[i], ImVec2(size, size), ImColor(IM_COL32_WHITE), false, TOOLTIPS[i]) || (allowShortcuts && focusingWnd && shortcuts[i].pressed())) {
				result = true;

				if (cursor)
					*cursor = (RotationsAndFlippings)i;
			}
		}

		if (i != BITTY_COUNTOF(icons) - 1)
			ImGui::SameLine();
	}

	return result;
}

bool tickable(
	Renderer* rnd,
	Workspace* ws,
	double* cursor,
	float width
) {
	ImGuiStyle &style = ImGui::GetStyle();

	Theme* theme = ws->theme();

	bool result = false;

	if (width <= 0)
		width = ImGui::GetContentRegionAvail().x;
	constexpr const int X_COUNT = EDITING_ITEM_COUNT_PER_LINE;
	float xOffset = 0;
	float size = width / X_COUNT;
	const float iconSize = (float)theme->iconClock()->width();
	if (size > iconSize) {
		size = iconSize * (int)(size / iconSize);
		xOffset = (width - size * X_COUNT) * 0.5f;
	}

	VariableGuard<decltype(style.FramePadding)> guardFramePadding(&style.FramePadding, style.FramePadding, ImVec2());

	ImGui::Dummy(ImVec2(xOffset, 0));
	ImGui::SameLine();

	ImGui::Image(theme->iconClock()->pointer(rnd), ImVec2(size, size));

	ImGui::SameLine();

	ImGui::PushID("@Itvl/Drag");
	{
		VariableGuard<decltype(style.FramePadding)> guardFramePadding_(&style.FramePadding, style.FramePadding, ImVec2(0, (size - ImGui::GetFontSize()) * 0.5f));

		ImGui::SetNextItemWidth(size * 4);
		if (cursor) {
			float data = (float)*cursor;
			if (ImGui::DragFloat("", &data, 0.05f, 0, 10)) {
				result = true;

				*cursor = data;
			}
		} else {
			float data = (float)*cursor;
			if (ImGui::DragFloat("", &data, 0.05f, 0, 10)) {
				result = true;
			}
		}
	}
	ImGui::PopID();

	return result;
}

bool jump(
	Renderer* rnd,
	Workspace* ws,
	int* cursor,
	float width,
	bool* initialized, bool* focused,
	int min, int max
) {
	ImGuiIO &io = ImGui::GetIO();
	ImGuiStyle &style = ImGui::GetStyle();

	bool result = false;

	if (focused)
		*focused = false;

	if (!cursor)
		return result;

	const ImVec2 buttonSize(13 * io.FontGlobalScale, 13 * io.FontGlobalScale);

	const float x = ImGui::GetCursorPosX();
	ImGui::Dummy(ImVec2(8, 0));
	ImGui::SameLine();
	ImGui::AlignTextToFramePadding();
	ImGui::TextUnformatted(ws->theme()->dialogItem_Goto());

	ImGui::PushID("@Jmp");
	do {
		ImGui::SameLine();
		if (!*initialized) {
			ImGui::SetKeyboardFocusHere();
			*initialized = true;
		}
		ImGui::SetNextItemWidth(width - (ImGui::GetCursorPosX() - x) - (buttonSize.x + style.FramePadding.x * 2) * 2);
		char buf[16];
		snprintf(buf, BITTY_COUNTOF(buf), "%d", *cursor + 1);
		const bool edited = ImGui::InputText("", buf, sizeof(buf), ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_AutoSelectAll);
		if (ImGui::GetActiveID() == ImGui::GetID("")) {
			if (focused)
				*focused = true;
		}
		if (!edited)
			break;
		int ln = 0;
		if (!Text::fromString(buf, ln))
			break;
		--ln;
		if (min >= 0 && max >= 0 && (ln < min || ln > max))
			break;

		result = true;

		*cursor = ln;
	} while (false);
	ImGui::PopID();

	ImGui::SameLine();
	if (ImGui::ImageButton(ws->theme()->slicePrevious()->pointer(rnd), buttonSize, ImGui::ColorConvertU32ToFloat4(ws->theme()->style()->iconColor))) {
		result = true;

		--*cursor;
		if (min >= 0 && *cursor < min)
			*cursor = min;
	}

	ImGui::SameLine();
	if (ImGui::ImageButton(ws->theme()->sliceNext()->pointer(rnd), buttonSize, ImGui::ColorConvertU32ToFloat4(ws->theme()->style()->iconColor))) {
		result = true;

		++*cursor;
		if (max >= 0 && *cursor > max)
			*cursor = max;
	}

	return result;
}

bool find(
	Renderer* rnd,
	Workspace* ws,
	Marker* cursor,
	float width,
	bool* initialized, bool* focused,
	const char* text, std::string* what,
	const Marker::Coordinates &max,
	int* direction,
	bool* caseSensitive, bool* wholeWord,
	bool visible,
	TextWordGetter getWord
) {
	ImGuiIO &io = ImGui::GetIO();
	ImGuiStyle &style = ImGui::GetStyle();

	bool result = false;

	if (focused)
		*focused = false;

	if (!cursor || !what)
		return result;

	int step = 0;
	if (direction) {
		step = *direction;
		*direction = 0;
	}

	if (visible) {
		const ImVec2 buttonSize(13 * io.FontGlobalScale, 13 * io.FontGlobalScale);

		const float x = ImGui::GetCursorPosX();
		ImGui::Dummy(ImVec2(8, 0));
		ImGui::SameLine();
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(ws->theme()->dialogItem_Find());

		ImGui::PushID("@Fnd");
		do {
			ImGui::SameLine();
			if (!*initialized) {
				ImGui::SetKeyboardFocusHere();
				*initialized = true;
			}
			ImGui::SetNextItemWidth(width - (ImGui::GetCursorPosX() - x) - (buttonSize.x + style.FramePadding.x * 2) * 4);
			char buf[256]; // Fixed size.
			const size_t n = std::min(BITTY_COUNTOF(buf) - 1, what->length());
			if (n > 0)
				memcpy(buf, what->c_str(), n);
			buf[n] = '\0';
			const ImGuiInputTextFlags flags = ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackAlways | ImGuiInputTextFlags_AllowTabInput;
			const bool edited = ImGui::InputText(
				"", buf, sizeof(buf),
				flags,
				[] (ImGuiInputTextCallbackData* data) -> int {
					std::string* what = (std::string*)data->UserData;
					what->assign(data->Buf, data->BufTextLen);

					return 0;
				},
				what
			);
			if (ImGui::GetActiveID() == ImGui::GetID("")) {
				if (focused)
					*focused = true;
			}
			if (!edited)
				break;

			step = 1;

			*what = buf;
		} while (false);
		ImGui::PopID();

		ImGui::SameLine();
		if (ImGui::ImageButton(ws->theme()->sliceCaseSensitive()->pointer(rnd), buttonSize, ImGui::ColorConvertU32ToFloat4(ws->theme()->style()->iconColor), caseSensitive ? *caseSensitive : false)) {
			if (caseSensitive)
				*caseSensitive = !*caseSensitive;
		}
		if (ImGui::IsItemHovered()) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			ImGui::SetTooltip(ws->theme()->tooltipEditing_CaseSensitive());
		}

		ImGui::SameLine();
		if (ImGui::ImageButton(ws->theme()->sliceWholeWord()->pointer(rnd), buttonSize, ImGui::ColorConvertU32ToFloat4(ws->theme()->style()->iconColor), wholeWord ? *wholeWord : false)) {
			if (wholeWord)
				*wholeWord = !*wholeWord;
		}
		if (ImGui::IsItemHovered()) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			ImGui::SetTooltip(ws->theme()->tooltipEditing_MatchWholeWords());
		}

		ImGui::SameLine();
		if (ImGui::ImageButton(ws->theme()->slicePrevious()->pointer(rnd), buttonSize, ImGui::ColorConvertU32ToFloat4(ws->theme()->style()->iconColor))) {
			if (!what->empty())
				step = -1;
		}

		ImGui::SameLine();
		if (ImGui::ImageButton(ws->theme()->sliceNext()->pointer(rnd), buttonSize, ImGui::ColorConvertU32ToFloat4(ws->theme()->style()->iconColor))) {
			if (!what->empty())
				step = 1;
		}
	}

	auto fill = [] (Marker* cursor, const Marker::Coordinates &nbegin, const Marker::Coordinates &nend, TextWordGetter getWord) -> bool {
		if (getWord) {
			Marker src;
			getWord(nbegin, src);
			if ((src.begin == nbegin && src.end == nend) || (src.begin == nend && src.end == nbegin)) {
				cursor->begin = nbegin;
				cursor->end = nend;

				return true;
			}
		} else {
			cursor->begin = nbegin;
			cursor->end = nend;

			return true;
		}

		return false;
	};

	if (step && what->empty())
		step = 0;
	if (step == 1) {
		std::string pat = *what;
		std::string tmp;
		if (!caseSensitive || !*caseSensitive) {
			Text::toLowerCase(pat);

			tmp = text;
			Text::toLowerCase(tmp);
			text = tmp.c_str();
		}
		const std::wstring wide = Unicode::toWide(what->c_str());
		int lnoff = 0, coloff = 0;
		const char* mat = editingTextFindForward(text, pat.c_str(), cursor->max().line, cursor->max().column, &lnoff, &coloff);
		if (mat) { // Found.
			Marker::Coordinates nbegin(cursor->begin.line + lnoff, coloff);
			Marker::Coordinates nend(cursor->begin.line + lnoff, coloff + (int)wide.length());
			if (lnoff == 0) {
				nbegin.column += cursor->max().column;
				nend.column += cursor->max().column;
			}

			result = fill(cursor, nbegin, nend, wholeWord && *wholeWord ? getWord : nullptr);
		} else { // Not found.
			// Find again from the beginning.
			mat = editingTextFindForward(text, pat.c_str(), 0, 0, &lnoff, &coloff);
			if (mat) { // Found.
				const Marker::Coordinates nbegin(lnoff, coloff);
				const Marker::Coordinates nend(lnoff, coloff + (int)wide.length());

				result = fill(cursor, nbegin, nend, wholeWord && *wholeWord ? getWord : nullptr);
			}
		}
	} else if (step == -1) {
		Marker::Coordinates pos = cursor->min();
		std::string pat = *what;
		std::string tmp;
		if (!caseSensitive || !*caseSensitive) {
			Text::toLowerCase(pat);

			tmp = text;
			Text::toLowerCase(tmp);
			text = tmp.c_str();
		}
		std::wstring wide = Unicode::toWide(what->c_str());
		int lnoff = 0, coloff = 0;
		const char* mat = editingTextFindBackward(text, pat.c_str(), pos.line, pos.column, &lnoff, &coloff);
		if (mat) { // Found.
			const Marker::Coordinates nbegin(lnoff, coloff);
			const Marker::Coordinates nend(lnoff, coloff + (int)wide.length());

			result = fill(cursor, nbegin, nend, wholeWord && *wholeWord ? getWord : nullptr);
		} else if (!max.empty()) { // Not found.
			// Find again from the end.
			pos = max;
			mat = editingTextFindBackward(text, pat.c_str(), pos.line, pos.column, &lnoff, &coloff);
			if (mat) { // Found.
				const Marker::Coordinates nbegin(lnoff, coloff);
				const Marker::Coordinates nend(lnoff, coloff + (int)wide.length());

				result = fill(cursor, nbegin, nend, wholeWord && *wholeWord ? getWord : nullptr);
			}
		}
	}

	return result;
}

bool find(
	Renderer* rnd,
	Workspace* ws,
	Marker* cursor,
	float width,
	bool* initialized, bool* focused,
	const TextPages* textPages, std::string* what,
	const Marker::Coordinates &max,
	int* direction,
	bool* caseSensitive, bool* wholeWord, bool* globalSearch,
	bool visible,
	TextWordGetter getWord
) {
	typedef std::function<void(int &)> FindViewHandler;

	auto findOne = [] (
		Renderer* rnd,
		Workspace* ws,
		Marker* cursor,
		bool* focused,
		const TextPages* textPages, std::string* what,
		const Marker::Coordinates &max,
		int* direction,
		bool* caseSensitive, bool* wholeWord, bool* globalSearch,
		FindViewHandler findView,
		TextWordGetter getWord
	) -> bool {
		(void)rnd;
		(void)ws;

		bool result = false;

		if (focused)
			*focused = false;

		if (!cursor || !what)
			return result;

		int step = 0;
		if (direction) {
			step = *direction;
			*direction = 0;
		}

		findView(step);

		auto at = [textPages, caseSensitive] (int page, std::string &cache) -> const std::string* {
			const std::string* txt = (*textPages)[page];
			if (!txt)
				return nullptr;

			if (!caseSensitive || !*caseSensitive) {
				cache = *txt;
				Text::toLowerCase(cache);
				txt = &cache;
			}

			return txt;
		};
		auto fill = [] (Marker* cursor, const Marker::Coordinates &nbegin, const Marker::Coordinates &nend, TextWordGetter getWord) -> bool {
			if (getWord) {
				Marker src;
				getWord(nbegin, src);
				if ((src.begin == nbegin && src.end == nend) || (src.begin == nend && src.end == nbegin)) {
					cursor->begin = nbegin;
					cursor->end = nend;

					return true;
				}
			} else {
				cursor->begin = nbegin;
				cursor->end = nend;

				return true;
			}

			return false;
		};

		if (step && what->empty())
			step = 0;
		if (step == 1) { // Forward.
			std::string pat = *what;
			if (!caseSensitive || !*caseSensitive) {
				Text::toLowerCase(pat);
			}
			const std::wstring widepat = Unicode::toWide(what->c_str());
			int page = cursor->begin.index;
			std::string tmp;
			const std::string* text = at(page, tmp);
			int lnoff = 0, coloff = 0;
			const char* mat = editingTextFindForward(text->c_str(), pat.c_str(), cursor->max().line, cursor->max().column, &lnoff, &coloff);
			if (mat) { // Found.
				Marker::Coordinates nbegin(page, cursor->begin.line + lnoff, coloff);
				Marker::Coordinates nend(page, cursor->begin.line + lnoff, coloff + (int)widepat.length());
				if (lnoff == 0) {
					nbegin.column += cursor->max().column;
					nend.column += cursor->max().column;
				}

				result = fill(cursor, nbegin, nend, wholeWord && *wholeWord ? getWord : nullptr);
			} else { // Not found.
				// Open next page for global search.
				if (*globalSearch && textPages->size() > 1) {
					if (++page >= (int)textPages->size())
						page = 0;
					text = at(page, tmp);
					while (text == nullptr) {
						if (++page >= (int)textPages->size())
							page = 0;
						text = at(page, tmp);
					}
				}

				// Find again from the beginning.
			_forwardAgain:
				mat = editingTextFindForward(text->c_str(), pat.c_str(), 0, 0, &lnoff, &coloff);
				if (mat) { // Found.
					const Marker::Coordinates nbegin(page, lnoff, coloff);
					const Marker::Coordinates nend(page, lnoff, coloff + (int)widepat.length());

					result = fill(cursor, nbegin, nend, wholeWord && *wholeWord ? getWord : nullptr);
				} else { // Not found.
					if (page != cursor->begin.index) {
						page = cursor->begin.index;
						text = at(page, tmp);

						goto _forwardAgain;
					}
				}
			}
		} else if (step == -1) { // Backward.
			Marker::Coordinates pos = cursor->min();
			std::string pat = *what;
			if (!caseSensitive || !*caseSensitive) {
				Text::toLowerCase(pat);
			}
			std::wstring widepat = Unicode::toWide(what->c_str());
			int page = cursor->begin.index;
			std::string tmp;
			const std::string* text = at(page, tmp);
			int lnoff = 0, coloff = 0;
			const char* mat = editingTextFindBackward(text->c_str(), pat.c_str(), pos.line, pos.column, &lnoff, &coloff);
			if (mat) { // Found.
				const Marker::Coordinates nbegin(page, lnoff, coloff);
				const Marker::Coordinates nend(page, lnoff, coloff + (int)widepat.length());

				result = fill(cursor, nbegin, nend, wholeWord && *wholeWord ? getWord : nullptr);
			} else if (!max.empty()) { // Not found.
				// Open previous page for global search.
				if (*globalSearch && textPages->size() > 1) {
					if (--page < 0)
						page = (int)textPages->size() - 1;
					text = at(page, tmp);
					while (text == nullptr) {
						if (--page < 0)
							page = (int)textPages->size() - 1;;
						text = at(page, tmp);
					}
				}

				// Find again from the end.
				pos = max;
			_backwardAgain:
				mat = editingTextFindBackward(text->c_str(), pat.c_str(), pos.line, pos.column, &lnoff, &coloff);
				if (mat) { // Found.
					const Marker::Coordinates nbegin(page, lnoff, coloff);
					const Marker::Coordinates nend(page, lnoff, coloff + (int)widepat.length());

					result = fill(cursor, nbegin, nend, wholeWord && *wholeWord ? getWord : nullptr);
				} else { // Not found.
					if (page != cursor->begin.index) {
						page = cursor->begin.index;
						text = at(page, tmp);

						goto _backwardAgain;
					}
				}
			}
		} else /* if (step == 0) */ { // Error.
			// Do nothing.
		}

		return result;
	};

	ImGuiIO &io = ImGui::GetIO();
	ImGuiStyle &style = ImGui::GetStyle();

	auto findView = [&] (int &step) -> void {
		if (!visible)
			return;

		const ImVec2 buttonSize(13 * io.FontGlobalScale, 13 * io.FontGlobalScale);

		const float x = ImGui::GetCursorPosX();
		ImGui::Dummy(ImVec2(8, 0));
		ImGui::SameLine();
		ImGui::AlignTextToFramePadding();
		ImGui::TextUnformatted(ws->theme()->dialogItem_Find());

		ImGui::PushID("@Fnd");
		do {
			ImGui::SameLine();
			if (!*initialized) {
				ImGui::SetKeyboardFocusHere();
				*initialized = true;
			}
			ImGui::SetNextItemWidth(width - (ImGui::GetCursorPosX() - x) - (buttonSize.x + style.FramePadding.x * 2) * 5);
			char buf[256]; // Fixed size.
			const size_t n = std::min(BITTY_COUNTOF(buf) - 1, what->length());
			if (n > 0)
				memcpy(buf, what->c_str(), n);
			buf[n] = '\0';
			const ImGuiInputTextFlags flags = ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackAlways | ImGuiInputTextFlags_AllowTabInput;
			const bool edited = ImGui::InputText(
				"", buf, sizeof(buf),
				flags,
				[] (ImGuiInputTextCallbackData* data) -> int {
					std::string* what = (std::string*)data->UserData;
					what->assign(data->Buf, data->BufTextLen);

					return 0;
				},
				what
			);
			if (ImGui::GetActiveID() == ImGui::GetID("")) {
				if (focused)
					*focused = true;
			}
			if (!edited)
				break;

			step = 1;

			*what = buf;
		} while (false);
		ImGui::PopID();

		ImGui::SameLine();
		if (ImGui::ImageButton(ws->theme()->sliceCaseSensitive()->pointer(rnd), buttonSize, ImGui::ColorConvertU32ToFloat4(ws->theme()->style()->iconColor), caseSensitive ? *caseSensitive : false)) {
			if (caseSensitive)
				*caseSensitive = !*caseSensitive;
		}
		if (ImGui::IsItemHovered()) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			ImGui::SetTooltip(ws->theme()->tooltipEditing_CaseSensitive());
		}

		ImGui::SameLine();
		if (ImGui::ImageButton(ws->theme()->sliceWholeWord()->pointer(rnd), buttonSize, ImGui::ColorConvertU32ToFloat4(ws->theme()->style()->iconColor), wholeWord ? *wholeWord : false)) {
			if (wholeWord)
				*wholeWord = !*wholeWord;
		}
		if (ImGui::IsItemHovered()) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			ImGui::SetTooltip(ws->theme()->tooltipEditing_MatchWholeWords());
		}

		ImGui::SameLine();
		if (ImGui::ImageButton(ws->theme()->sliceGlobal()->pointer(rnd), buttonSize, ImGui::ColorConvertU32ToFloat4(ws->theme()->style()->iconColor), globalSearch ? *globalSearch : false)) {
			if (globalSearch)
				*globalSearch = !*globalSearch;
		}
		if (ImGui::IsItemHovered()) {
			VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

			ImGui::SetTooltip(ws->theme()->tooltipEditing_GlobalSearchForCode());
		}

		ImGui::SameLine();
		if (ImGui::ImageButton(ws->theme()->slicePrevious()->pointer(rnd), buttonSize, ImGui::ColorConvertU32ToFloat4(ws->theme()->style()->iconColor))) {
			if (!what->empty())
				step = -1;
		}

		ImGui::SameLine();
		if (ImGui::ImageButton(ws->theme()->sliceNext()->pointer(rnd), buttonSize, ImGui::ColorConvertU32ToFloat4(ws->theme()->style()->iconColor))) {
			if (!what->empty())
				step = 1;
		}
	};

	const bool result = findOne(
		rnd,
		ws,
		cursor,
		focused,
		textPages, what,
		max,
		direction,
		caseSensitive, wholeWord, globalSearch,
		findView,
		getWord
	);

	return result;
}

}

namespace Data {

Checkpoint::Checkpoint() {
}

bool Checkpoint::empty(void) const {
	return !bytes;
}

void Checkpoint::fill(void) {
	bytes = Bytes::Ptr(Bytes::create());
}

void Checkpoint::clear(void) {
	bytes = nullptr;
}

bool toCheckpoint(const Project* project, const char* name, Checkpoint &checkpoint) {
	bool result = false;

	Bytes::Ptr bytes = checkpoint.bytes;

	bytes->clear();

	LockGuard<RecursiveMutex>::UniquePtr acquired;
	Project* prj = project->acquire(acquired);
	if (!prj)
		return result;

	Asset* asset = prj->get(name);
	if (!asset)
		return result;

	checkpoint.compressed = asset->type() != Image::TYPE(); // Do not compress encoded image.
	if (!checkpoint.compressed) {
		std::string ext = asset->extName();
		Text::toLowerCase(ext);
		if (ext == BITTY_IMAGE_EXT)
			checkpoint.compressed = true;
	}
	if (checkpoint.compressed) {
		Bytes::Ptr cache(Bytes::create());
		result = asset->save(Asset::EDITING, cache.get());
		int n = LZ4_compressBound((int)cache->count());
		bytes->resize((size_t)n);
		n = LZ4_compress_default(
			(const char*)cache->pointer(), (char*)bytes->pointer(),
			(int)cache->count(), (int)bytes->count()
		);
		assert(n);
		if (n)
			bytes->resize((size_t)n);

		checkpoint.originalSize = cache->count();

		fprintf(stdout, "Saved checkpoint in %d bytes, compressed to %d bytes.\n", (int)cache->count(), (int)bytes->count());
	} else {
		result = asset->save(Asset::EDITING, bytes.get());

		checkpoint.originalSize = bytes->count();

		fprintf(stdout, "Saved checkpoint in %d bytes.\n", (int)bytes->count());
	}

	bytes->poke(0);

	return result;
}

bool fromCheckpoint(const Project* project, const char* name, Checkpoint &checkpoint) {
	bool result = false;

	Bytes::Ptr bytes = checkpoint.bytes;

	const size_t pos = bytes->peek();
	bytes->poke(0);

	LockGuard<RecursiveMutex>::UniquePtr acquired;
	Project* prj = project->acquire(acquired);
	if (!prj)
		return result;

	Asset* asset = prj->get(name);
	if (!asset)
		return result;

	if (checkpoint.compressed) {
		Bytes::Ptr cache(Bytes::create());
		cache->resize(checkpoint.originalSize);
		const int n = LZ4_decompress_safe(
			(const char*)bytes->pointer(), (char*)cache->pointer(),
			(int)bytes->count(), (int)cache->count()
		);
		(void)n;
		assert(n == (int)checkpoint.originalSize);
		result = asset->reload(Asset::EDITING, cache.get(), nullptr, false);

		fprintf(stdout, "Restored checkpoint from %d bytes, decompressed from %d bytes.\n", (int)cache->count(), (int)bytes->count());
	} else {
		result = asset->reload(Asset::EDITING, bytes.get(), nullptr, false);

		fprintf(stdout, "Restored checkpoint from %d bytes.\n", (int)bytes->count());
	}

	bytes->poke(pos);

	return result;
}

}

}

/* ===========================================================================} */
