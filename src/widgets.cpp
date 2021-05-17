/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2021 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "code.h"
#include "datetime.h"
#include "encoding.h"
#include "filesystem.h"
#include "image.h"
#include "map.h"
#include "platform.h"
#include "project.h"
#include "sprite.h"
#include "widgets.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "../lib/imgui/imgui_internal.h"
#include <SDL.h>

/*
** {===========================================================================
** Utilities
*/

bool operator == (const ImVec2 &left, const ImVec2 &right) {
	return left.x == right.x && left.y == right.y;
}

bool operator != (const ImVec2 &left, const ImVec2 &right) {
	return left.x != right.x || left.y != right.y;
}

/* ===========================================================================} */

/*
** {===========================================================================
** ImGui widgets
*/

namespace ImGui {

bool Initializer::begin(void) const {
	return _ticks == 0;
}

bool Initializer::end(void) const {
	return _ticks >= 2;
}

void Initializer::update(void) {
	if (_ticks < 2)
		++_ticks;
}

void Initializer::reset(void) {
	_ticks = 0;
}

Hierarchy::Hierarchy(BeginHandler begin, EndHandler end) : _begin(begin), _end(end) {
}

void Hierarchy::prepare(void) {
	_opened.push(true);
}

void Hierarchy::finish(void) {
	while (_opened.size() > 1) {
		if (_opened.top() && _end)
			_end();
		_opened.pop();
	}
}

bool Hierarchy::with(Text::Array::const_iterator begin, Text::Array::const_iterator end) {
	Compare::diff(begin, end, _path.begin(), _path.end(), &_dec, &_inc); // Calculate difference between the current entry and the last `path`.
	_path.assign(begin, end); // Assign for calculation during the next loop step.

	for (int i = 0; i < _dec; ++i) {
		if (_opened.top() && _end)
			_end();
		_opened.pop();
	}
	for (const std::string &dir : _inc) {
		if (_opened.top()) {
			if (_begin)
				_opened.push(_begin(dir));
			else
				_opened.push(false);
		} else {
			_opened.push(false);
		}
	}

	return _opened.top();
}

PopupBox::PopupBox() {
}

PopupBox::~PopupBox() {
}

WaitingPopupBox::WaitingPopupBox(
	const std::string &content,
	const TimeoutHandler &timeout
) : _content(content),
	_timeoutHandler(timeout) {
}

WaitingPopupBox::~WaitingPopupBox() {
}

void WaitingPopupBox::update(void) {
	ImGuiIO &io = GetIO();

	bool isOpen = true;
	bool toClose = false;

	const unsigned long long now = DateTime::ticks();

	if (_init.begin()) {
		OpenPopup("#Wait");

		_timeoutTime = now + DateTime::fromSeconds(0.25);
	}

	const ImVec2 pos = io.DisplaySize/* * io.DisplayFramebufferScale*/ * 0.5f;
	SetNextWindowPos(pos, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
	const ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize;
	if (BeginPopupModal("#Wait", &isOpen, flags)) {
		TextUnformatted(_content);

		if (now >= _timeoutTime) {
			toClose = true;

			CloseCurrentPopup();
		}

		EndPopup();
	}

	if (isOpen)
		_init.update();

	if (!isOpen)
		toClose = true;

	if (toClose) {
		_init.reset();

		if (!_timeoutHandler.empty())
			_timeoutHandler();
	}
}

MessagePopupBox::MessagePopupBox(
	const std::string &title,
	const std::string &content,
	const ConfirmHandler &confirm, const DenyHandler &deny, const CancelHandler &cancel,
	const char* confirmTxt, const char* denyTxt, const char* cancelTxt
) : _title(title),
	_content(content),
	_confirmHandler(confirm), _denyHandler(deny), _cancelHandler(cancel)
{
	if (confirmTxt)
		_confirmText = confirmTxt;
	if (denyTxt)
		_denyText = denyTxt;
	if (cancelTxt)
		_cancelText = cancelTxt;
}

MessagePopupBox::~MessagePopupBox() {
}

void MessagePopupBox::update(void) {
	bool isOpen = true;
	bool toConfirm = false;
	bool toDeny = false;
	bool toCancel = false;

	if (_init.begin())
		OpenPopup(_title);

	if (BeginPopupModal(_title, _cancelHandler.empty() ? nullptr : &isOpen, ImGuiWindowFlags_AlwaysAutoResize)) {
		TextUnformatted(_content);

		const char* confirm = _confirmText.c_str();
		const char* deny = _denyText.empty() ? "No" : _denyText.c_str();
		const char* cancel = _cancelText.empty() ? "Cancel" : _cancelText.c_str();

		if (_confirmText.empty()) {
			if (_denyHandler.empty())
				confirm = "Ok";
			else
				confirm = "Yes";
		}

		int count = 1;
		if (!_denyHandler.empty())
			++count;
		if (!_cancelHandler.empty())
			++count;
		CentralizeButton(count);

		if (Button(confirm, ImVec2(WIDGETS_BUTTON_WIDTH, 0)) || IsKeyReleased(SDL_SCANCODE_RETURN) || IsKeyReleased(SDL_SCANCODE_Y)) {
			toConfirm = true;

			CloseCurrentPopup();
		}

		if (!_denyHandler.empty()) {
			SameLine();
			if (Button(deny, ImVec2(WIDGETS_BUTTON_WIDTH, 0))) {
				toDeny = true;

				CloseCurrentPopup();
			}
		}

		if (!_cancelHandler.empty()) {
			SameLine();
			if (Button(cancel, ImVec2(WIDGETS_BUTTON_WIDTH, 0)) || IsKeyReleased(SDL_SCANCODE_ESCAPE)) {
				toCancel = true;

				CloseCurrentPopup();
			}
		}

		if (!_init.begin() && !_init.end())
			CentralizeWindow();

		EnsureWindowVisible();

		EndPopup();
	}

	if (isOpen)
		_init.update();

	if (!isOpen)
		toCancel = true;

	if (toConfirm) {
		_init.reset();

		if (!_confirmHandler.empty())
			_confirmHandler();
	}
	if (toDeny) {
		_init.reset();

		if (!_denyHandler.empty())
			_denyHandler();
	}
	if (toCancel) {
		_init.reset();

		if (!_cancelHandler.empty())
			_cancelHandler();
	}
}

InputPopupBox::InputPopupBox(
	const std::string &title,
	const std::string &content, const std::string &default_, unsigned flags,
	const ConfirmHandler &confirm, const CancelHandler &cancel,
	const char* confirmTxt, const char* cancelTxt
) : _title(title),
	_content(content), _default(default_), _flags(flags),
	_confirmHandler(confirm), _cancelHandler(cancel)
{
	memset(_buffer, 0, sizeof(_buffer));
	memcpy(_buffer, _default.c_str(), std::min(sizeof(_buffer), _default.length()));

	if (confirmTxt)
		_confirmText = confirmTxt;
	if (cancelTxt)
		_cancelText = cancelTxt;
}

InputPopupBox::~InputPopupBox() {
}

void InputPopupBox::update(void) {
	bool isOpen = true;
	bool toConfirm = false;
	bool toCancel = false;

	if (_init.begin())
		OpenPopup(_title);

	if (BeginPopupModal(_title, _cancelHandler.empty() ? nullptr : &isOpen, ImGuiWindowFlags_AlwaysAutoResize)) {
		TextUnformatted(_content);

		if (!_init.end())
			SetKeyboardFocusHere();
		InputText("", _buffer, sizeof(_buffer), _flags | ImGuiInputTextFlags_AutoSelectAll);

		const char* confirm = _confirmText.empty() ? "Ok" : _confirmText.c_str();
		const char* cancel = _cancelText.empty() ? "Cancel" : _cancelText.c_str();

		CentralizeButton(2);

		if (Button(confirm, ImVec2(WIDGETS_BUTTON_WIDTH, 0)) || IsKeyReleased(SDL_SCANCODE_RETURN)) {
			toConfirm = true;

			CloseCurrentPopup();
		}

		SameLine();
		if (Button(cancel, ImVec2(WIDGETS_BUTTON_WIDTH, 0)) || IsKeyReleased(SDL_SCANCODE_ESCAPE)) {
			toCancel = true;

			CloseCurrentPopup();
		}

		if (!_init.begin() && !_init.end())
			CentralizeWindow();

		EnsureWindowVisible();

		EndPopup();
	}

	if (isOpen)
		_init.update();

	if (!isOpen)
		toCancel = true;

	if (toConfirm) {
		_init.reset();

		if (!_confirmHandler.empty())
			_confirmHandler(_buffer);
	}
	if (toCancel) {
		_init.reset();

		if (!_cancelHandler.empty())
			_cancelHandler();
	}
}

AddAssetPopupBox::AddAssetPopupBox(
	const class Project* project,
	const std::string &title,
	const std::string &type, const Types &types, const TypeNames &typeNames, const TypeExtensions &typeExtensions, int typeIndex,
	const std::string &size, const Vec2s &defaultSizes, const Vec2s &maxSizes,
	const std::string &size2, const Vec2s &defaultSizes2, const Vec2s &maxSizes2,
	const std::string &content, const std::string &default_,
	const std::string &tooltipRefPalette, const std::string &tooltipRefImage, const std::string &tooltipSize, const std::string &tooltipPath,
	const std::string &none, const std::string &reference, const std::string &palette,
	const ConfirmHandler &confirm, const CancelHandler &cancel,
	const char* confirmTxt, const char* cancelTxt
) : _project(project),
	_title(title),
	_type(type), _types(types), _typeNames(typeNames), _typeExtensions(typeExtensions), _typeIndex(typeIndex),
	_size(size), _defaultSizes(defaultSizes), _maxSizes(maxSizes),
	_size2(size2), _defaultSizes2(defaultSizes2), _maxSizes2(maxSizes2),
	_none(none), _reference(reference), _palette(palette),
	_content(content), _default(default_),
	_tooltipRefPalette(tooltipRefPalette), _tooltipRefImage(tooltipRefImage), _tooltipSize(tooltipSize), _tooltipPath(tooltipPath),
	_confirmHandler(confirm), _cancelHandler(cancel)
{
	if (_typeIndex < 0 || _typeIndex >= (int)_defaultSizes.size()) {
		_typeIndex = 0;
		_sizeVec = Math::Vec2i(0, 0);
		_sizeVec2 = Math::Vec2i(0, 0);
	} else {
		_sizeVec = _defaultSizes[_typeIndex];
		_sizeVec2 = _defaultSizes2[_typeIndex];
	}

	memset(_buffer, 0, sizeof(_buffer));
	memcpy(_buffer, _default.c_str(), std::min(sizeof(_buffer), _default.length()));

	if (confirmTxt)
		_confirmText = confirmTxt;
	if (cancelTxt)
		_cancelText = cancelTxt;

	_language = BITTY_LUA_EXT;
	do {
		LockGuard<RecursiveMutex>::UniquePtr acquired;
		Project* prj = project->acquire(acquired);
		if (!prj)
			break;

		_language = prj->language();
	} while (false);
}

AddAssetPopupBox::~AddAssetPopupBox() {
}

void AddAssetPopupBox::update(void) {
	ImGuiStyle &style = GetStyle();

	const Math::Vec2i* szPtr = nullptr;
	const Math::Vec2i* szPtr2 = nullptr;

	bool isOpen = true;
	bool toConfirm = false;
	bool toCancel = false;

	if (_init.begin())
		OpenPopup(_title);

	auto typeChanged = [&] (void) -> void {
		_refs.clear();
		_refIndex = -1;

		_sizeVec = _defaultSizes[_typeIndex];
		_sizeVec2 = _defaultSizes2[_typeIndex];
	};

	if (BeginPopupModal(_title, _cancelHandler.empty() ? nullptr : &isOpen, ImGuiWindowFlags_AlwaysAutoResize)) {
		PushID("@Type");
		{
			SetNextItemWidth(60);
			TextUnformatted(_type);
			const bool changed = Combo(
				"",
				&_typeIndex,
				[] (void* data, int idx, const char** outText) -> bool {
					TypeNames* names = (TypeNames*)data;
					const std::string &name = names->at(idx);
					*outText = name.c_str();

					return true;
				},
				&_typeNames,
				(int)_types.size()
			);
			if (changed)
				typeChanged();
		}
		PopID();

		switch (_types[_typeIndex]) {
		case Palette::TYPE(): // Do nothing.
			break;
		case Image::TYPE():
			PushID("@Ref");
			{
				RefSelector(_project, _refs, &_refIndex, Image::TYPE(), _none.c_str(), _palette.c_str(), _reference.c_str());

				SameLine();
				SetHelpTooltip(_tooltipRefPalette);
			}
			PopID();

			PushID("@Sz");
			{
				PushItemWidth((CalcItemWidth() - style.ItemSpacing.x) * 0.5f);

				TextUnformatted(_size);
				int v[2] = { _sizeVec.x, _sizeVec.y };
				PushID("@X");
				if (DragInt("", &v[0], 1, 1, _maxSizes[_typeIndex].x))
					_sizeVec.x = v[0];
				PopID();
				SameLine();
				PushID("@Y");
				if (DragInt("", &v[1], 1, 1, _maxSizes[_typeIndex].y))
					_sizeVec.y = v[1];
				PopID();
				szPtr = &_sizeVec;

				SameLine();
				SetHelpTooltip(_tooltipSize);

				PopItemWidth();
			}
			PopID();

			break;
		case Sprite::TYPE():
			PushID("@Ref");
			{
				RefSelector(_project, _refs, &_refIndex, Sprite::TYPE(), _none.c_str(), _palette.c_str(), _reference.c_str());

				SameLine();
				SetHelpTooltip(_tooltipRefImage);
			}
			PopID();

			PushID("@Sz");
			{
				PushItemWidth((CalcItemWidth() - style.ItemSpacing.x) * 0.5f);

				TextUnformatted(_size);
				int v[2] = { _sizeVec.x, _sizeVec.y };
				PushID("@X");
				if (DragInt("", &v[0], 1, 1, _maxSizes[_typeIndex].x))
					_sizeVec.x = v[0];
				PopID();
				SameLine();
				PushID("@Y");
				if (DragInt("", &v[1], 1, 1, _maxSizes[_typeIndex].y))
					_sizeVec.y = v[1];
				PopID();
				szPtr = &_sizeVec;

				SameLine();
				SetHelpTooltip(_tooltipSize);

				PopItemWidth();
			}
			PopID();

			break;
		case Map::TYPE():
			PushID("@Ref");
			{
				RefSelector(_project, _refs, &_refIndex, Map::TYPE(), _none.c_str(), _palette.c_str(), _reference.c_str());

				SameLine();
				SetHelpTooltip(_tooltipRefImage);
			}
			PopID();

			PushID("@Tile/Sz");
			{
				PushItemWidth((CalcItemWidth() - style.ItemSpacing.x) * 0.5f);

				TextUnformatted(_size2);
				int v[2] = { _sizeVec2.x, _sizeVec2.y };
				PushID("@X");
				if (DragInt("", &v[0], 1, 1, _maxSizes2[_typeIndex].x))
					_sizeVec2.x = v[0];
				PopID();
				SameLine();
				PushID("@Y");
				if (DragInt("", &v[1], 1, 1, _maxSizes2[_typeIndex].y))
					_sizeVec2.y = v[1];
				PopID();
				szPtr2 = &_sizeVec2;

				SameLine();
				SetHelpTooltip(_tooltipSize);

				PopItemWidth();
			}
			PopID();

			PushID("@Sz");
			{
				PushItemWidth((CalcItemWidth() - style.ItemSpacing.x) * 0.5f);

				TextUnformatted(_size);
				int v[2] = { _sizeVec.x, _sizeVec.y };
				PushID("@X");
				if (DragInt("", &v[0], 1, 1, _maxSizes[_typeIndex].x))
					_sizeVec.x = v[0];
				PopID();
				SameLine();
				PushID("@Y");
				if (DragInt("", &v[1], 1, 1, _maxSizes[_typeIndex].y))
					_sizeVec.y = v[1];
				PopID();
				szPtr = &_sizeVec;

				SameLine();
				SetHelpTooltip(_tooltipSize);

				PopItemWidth();
			}
			PopID();

			break;
		case Code::TYPE(): // Fall through.
		case Json::TYPE(): // Fall through.
		case Text::TYPE(): // Do nothing.
			break;
		default: // Do nothing.
			break;
		}

		PushID("@Input");
		{
			TextUnformatted(_content);

			if (!_init.end())
				SetKeyboardFocusHere();
			if (InputText("", _buffer, sizeof(_buffer), ImGuiInputTextFlags_AutoSelectAll)) {
				const std::string ext = Asset::extOf(_buffer);
				const unsigned y = Asset::typeOf(ext, false);
				if (y) {
					const int old = _typeIndex;
					const unsigned* offset = std::find(&_types[0], &_types[_types.size() - 1], y);
					const ptrdiff_t idx = offset - &_types[0];
					_typeIndex = Math::clamp((int)idx, 0, (int)(_types.size() - 1));
					if (_typeIndex != old)
						typeChanged();
				}
			}

			SameLine();
			SetHelpTooltip(_tooltipPath);
		}
		PopID();

		const char* confirm = _confirmText.empty() ? "Ok" : _confirmText.c_str();
		const char* cancel = _cancelText.empty() ? "Cancel" : _cancelText.c_str();

		CentralizeButton(2);

		if (Button(confirm, ImVec2(WIDGETS_BUTTON_WIDTH, 0)) || IsKeyReleased(SDL_SCANCODE_RETURN)) {
			toConfirm = true;

			CloseCurrentPopup();
		}

		SameLine();
		if (Button(cancel, ImVec2(WIDGETS_BUTTON_WIDTH, 0)) || IsKeyReleased(SDL_SCANCODE_ESCAPE)) {
			toCancel = true;

			CloseCurrentPopup();
		}

		if (!_init.begin() && !_init.end())
			CentralizeWindow();

		EnsureWindowVisible();

		EndPopup();
	}

	if (isOpen)
		_init.update();

	if (!isOpen)
		toCancel = true;

	if (toConfirm) {
		_init.reset();

		if (!_confirmHandler.empty()) {
			const char* ref = nullptr;
			if (_refIndex > 0 && _refIndex <= (int)_refs.size())
				ref = _refs[_refIndex].c_str();

			std::string buffer = _buffer;
			const std::string ext = Asset::extOf(_buffer);
			const unsigned y = Asset::typeOf(ext, false);
			if (!buffer.empty() && y != _types[_typeIndex]) {
				switch (_types[_typeIndex]) {
				case Image::TYPE():
					buffer += ref ? "." BITTY_IMAGE_EXT : ".png";

					break;
				case Code::TYPE():
					buffer += "." + _language;

					break;
				default:
					buffer += "." + _typeExtensions[_typeIndex];

					break;
				}
			}
			Path::uniform(buffer);

			_confirmHandler(_types[_typeIndex], ref, szPtr, szPtr2, buffer.c_str());
		}
	}
	if (toCancel) {
		_init.reset();

		if (!_cancelHandler.empty())
			_cancelHandler();
	}
}

AddFilePopupBox::AddFilePopupBox(
	const std::string &title,
	const std::string &path, const std::string &defaultPath,
	const std::string &browse, Browser browser,
	const std::string &content, const std::string &default_, const std::string &tooltip,
	const ConfirmHandler &confirm, const CancelHandler &cancel,
	const char* confirmTxt, const char* cancelTxt
) : _title(title),
	_path(path), _defaultPath(defaultPath),
	_browse(browse), _browser(browser),
	_content(content), _default(default_), _tooltip(tooltip),
	_confirmHandler(confirm), _cancelHandler(cancel)
{
	memset(_buffer, 0, sizeof(_buffer));
	memcpy(_buffer, _default.c_str(), std::min(sizeof(_buffer), _default.length()));

	if (confirmTxt)
		_confirmText = confirmTxt;
	if (cancelTxt)
		_cancelText = cancelTxt;
}

AddFilePopupBox::~AddFilePopupBox() {
}

void AddFilePopupBox::update(void) {
	bool isOpen = true;
	bool toConfirm = false;
	bool toCancel = false;

	if (_init.begin())
		OpenPopup(_title);

	if (BeginPopupModal(_title, _cancelHandler.empty() ? nullptr : &isOpen, ImGuiWindowFlags_AlwaysAutoResize)) {
		PushID("@Path");
		{
			TextUnformatted(_path);

			const std::string buf = _defaultPath;
			InputText("", (char*)buf.c_str(), buf.length(), ImGuiInputTextFlags_ReadOnly);
			SameLine();
			if (Button(_browse, ImVec2(WIDGETS_BUTTON_WIDTH, 0))) {
				const std::string newPath = _browser(_defaultPath);
				if (!newPath.empty()) {
					_defaultPath = newPath;
					Path::split(_defaultPath, &_default, nullptr, nullptr);
					memset(_buffer, 0, sizeof(_buffer));
					memcpy(_buffer, _default.c_str(), std::min(sizeof(_buffer), _default.length()));
				}
			}
		}
		PopID();

		PushID("@Input");
		{
			TextUnformatted(_content);

			if (!_init.end())
				SetKeyboardFocusHere();
			InputText("", _buffer, sizeof(_buffer), ImGuiInputTextFlags_AutoSelectAll);

			SameLine();
			SetHelpTooltip(_tooltip);
		}
		PopID();

		const char* confirm = _confirmText.empty() ? "Ok" : _confirmText.c_str();
		const char* cancel = _cancelText.empty() ? "Cancel" : _cancelText.c_str();

		CentralizeButton(2);

		if (Button(confirm, ImVec2(WIDGETS_BUTTON_WIDTH, 0)) || IsKeyReleased(SDL_SCANCODE_RETURN)) {
			toConfirm = true;

			CloseCurrentPopup();
		}

		SameLine();
		if (Button(cancel, ImVec2(WIDGETS_BUTTON_WIDTH, 0)) || IsKeyReleased(SDL_SCANCODE_ESCAPE)) {
			toCancel = true;

			CloseCurrentPopup();
		}

		if (!_init.begin() && !_init.end())
			CentralizeWindow();

		EnsureWindowVisible();

		EndPopup();
	}

	if (isOpen)
		_init.update();

	if (!isOpen)
		toCancel = true;

	if (toConfirm) {
		_init.reset();

		if (!_confirmHandler.empty()) {
			std::string buffer = _buffer;
			Path::uniform(buffer);

			_confirmHandler(_defaultPath.c_str(), buffer.c_str());
		}
	}
	if (toCancel) {
		_init.reset();

		if (!_cancelHandler.empty())
			_cancelHandler();
	}
}

ResizePopupBox::ResizePopupBox(
	const std::string &title,
	const std::string &size, const Math::Vec2i &defaultSize, const Math::Vec2i &maxSize,
	const ConfirmHandler &confirm, const CancelHandler &cancel,
	const char* confirmTxt, const char* cancelTxt
) : _title(title),
	_size(size), _defaultSize(defaultSize), _maxSize(maxSize),
	_confirmHandler(confirm), _cancelHandler(cancel)
{
	_sizeVec = defaultSize;

	if (confirmTxt)
		_confirmText = confirmTxt;
	if (cancelTxt)
		_cancelText = cancelTxt;
}

ResizePopupBox::~ResizePopupBox() {
}

void ResizePopupBox::update(void) {
	ImGuiStyle &style = GetStyle();

	const Math::Vec2i* szPtr = nullptr;

	bool isOpen = true;
	bool toConfirm = false;
	bool toCancel = false;

	if (_init.begin())
		OpenPopup(_title);

	if (BeginPopupModal(_title, _cancelHandler.empty() ? nullptr : &isOpen, ImGuiWindowFlags_AlwaysAutoResize)) {
		PushID("@Asset/Sz");
		{
			PushItemWidth((CalcItemWidth() - style.ItemSpacing.x) * 0.5f);

			TextUnformatted(_size);
			int v[2] = { _sizeVec.x, _sizeVec.y };
			PushID("@X");
			if (DragInt("", &v[0], 1, 1, _maxSize.x))
				_sizeVec.x = v[0];
			PopID();
			SameLine();
			PushID("@Y");
			if (DragInt("", &v[1], 1, 1, _maxSize.y))
				_sizeVec.y = v[1];
			PopID();
			szPtr = &_sizeVec;

			PopItemWidth();
		}
		PopID();

		const char* confirm = _confirmText.empty() ? "Ok" : _confirmText.c_str();
		const char* cancel = _cancelText.empty() ? "Cancel" : _cancelText.c_str();

		CentralizeButton(2);

		if (Button(confirm, ImVec2(WIDGETS_BUTTON_WIDTH, 0)) || IsKeyReleased(SDL_SCANCODE_RETURN)) {
			toConfirm = true;

			CloseCurrentPopup();
		}

		SameLine();
		if (Button(cancel, ImVec2(WIDGETS_BUTTON_WIDTH, 0)) || IsKeyReleased(SDL_SCANCODE_ESCAPE)) {
			toCancel = true;

			CloseCurrentPopup();
		}

		if (!_init.begin() && !_init.end())
			CentralizeWindow();

		EnsureWindowVisible();

		EndPopup();
	}

	if (isOpen)
		_init.update();

	if (!isOpen)
		toCancel = true;

	if (toConfirm) {
		_init.reset();

		if (_sizeVec == _defaultSize) {
			if (!_cancelHandler.empty())
				_cancelHandler();
		} else {
			if (!_confirmHandler.empty())
				_confirmHandler(szPtr);
		}
	}
	if (toCancel) {
		_init.reset();

		if (!_cancelHandler.empty())
			_cancelHandler();
	}
}

SelectAssetPopupBox::SelectAssetPopupBox(
	const class Project* project,
	const std::string &title,
	const std::string &content,
	const std::string &default_,
	const std::string &extra,
	ImTextureID texId, ImTextureID openTexId, ImTextureID fileTexId, ImU32 col,
	AssetFilter filter,
	const ConfirmHandlerForSingleSelection &confirm, const CancelHandler &cancel,
	const char* confirmTxt, const char* cancelTxt
) : _project(project),
	_title(title),
	_content(content),
	_singleSelection(default_),
	_extra(extra),
	_texId(texId), _openTexId(openTexId), _fileTexId(fileTexId), _color(col),
	_filter(filter),
	_confirmHandlerForSingleSelection(confirm), _cancelHandler(cancel),
	_confirmText(confirmTxt), _cancelText(cancelTxt)
{
}

SelectAssetPopupBox::SelectAssetPopupBox(
	const class Project* project,
	const std::string &title,
	const std::string &content,
	const Text::Set &default_,
	const std::string &extra,
	const std::string &all,
	ImTextureID texId, ImTextureID openTexId, ImU32 col,
	AssetFilter filter,
	const ConfirmHandlerForMultipleSelection &confirm, const CancelHandler &cancel,
	const char* confirmTxt, const char* cancelTxt
) : _project(project),
	_title(title),
	_content(content),
	_multipleSelection(default_),
	_extra(extra),
	_all(all),
	_texId(texId), _openTexId(openTexId), _color(col),
	_filter(filter),
	_confirmHandlerForMultipleSelection(confirm), _cancelHandler(cancel),
	_confirmText(confirmTxt), _cancelText(cancelTxt)
{
}

SelectAssetPopupBox::~SelectAssetPopupBox() {
}

void SelectAssetPopupBox::update(void) {
	ImGuiIO &io = GetIO();

	bool isOpen = true;
	bool toConfirm = false;
	bool toCancel = false;

	const bool hasHandlerForSingleSelection = !_confirmHandlerForSingleSelection.empty();
	const bool hasHandlerForMultipleSelection = !_confirmHandlerForMultipleSelection.empty();

	if (_init.begin())
		OpenPopup(_title);

	if (BeginPopupModal(_title, _cancelHandler.empty() ? nullptr : &isOpen, ImGuiWindowFlags_AlwaysAutoResize)) {
		TextUnformatted(_content);

		BeginChild("@Sel", ImVec2(256.0f * io.FontGlobalScale, 256.0f * io.FontGlobalScale), true, ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar);
		int total = 0;
		if (!_confirmHandlerForSingleSelection.empty())
			AssetSelector(_project, _singleSelection, _texId, _openTexId, _fileTexId, _color, _filter);
		else if (!_confirmHandlerForMultipleSelection.empty())
			AssetSelector(_project, _multipleSelection, _texId, _openTexId, _color, _filter, &total);
		EndChild();

		if (!_confirmHandlerForMultipleSelection.empty()) {
			const bool partial = total != (int)_multipleSelection.size();
			bool any = !partial && !_multipleSelection.empty();
			if (Checkbox(_all, &any)) {
				if (any)
					AssetSelectAll(_project, _multipleSelection, _filter);
				else
					_multipleSelection.clear();
			}
		}

		if (!_extra.empty())
			TextUnformatted(_extra);

		const char* confirm = _confirmText.empty() ? "Ok" : _confirmText.c_str();
		const char* cancel = _cancelText.empty() ? "Cancel" : _cancelText.c_str();

		CentralizeButton(2);

		const bool confirmable = (hasHandlerForSingleSelection && !_singleSelection.empty()) || (hasHandlerForMultipleSelection && !_multipleSelection.empty());
		if (confirmable) {
			if (Button(confirm, ImVec2(WIDGETS_BUTTON_WIDTH, 0)) || IsKeyReleased(SDL_SCANCODE_RETURN)) {
				toConfirm = true;

				CloseCurrentPopup();
			}
		} else {
			PushStyleColor(ImGuiCol_Text, GetStyleColorVec4(ImGuiCol_TextDisabled));
			Button(confirm, ImVec2(WIDGETS_BUTTON_WIDTH, 0));
			PopStyleColor();
		}

		SameLine();
		if (Button(cancel, ImVec2(WIDGETS_BUTTON_WIDTH, 0)) || IsKeyReleased(SDL_SCANCODE_ESCAPE)) {
			toCancel = true;

			CloseCurrentPopup();
		}

		if (!_init.begin() && !_init.end())
			CentralizeWindow();

		EnsureWindowVisible();

		EndPopup();
	}

	if (isOpen)
		_init.update();

	if (!isOpen)
		toCancel = true;

	if (toConfirm) {
		_init.reset();

		if (hasHandlerForSingleSelection)
			_confirmHandlerForSingleSelection(_singleSelection);

		if (hasHandlerForMultipleSelection)
			_confirmHandlerForMultipleSelection(_multipleSelection);
	}
	if (toCancel) {
		_init.reset();

		if (!_cancelHandler.empty())
			_cancelHandler();
	}
}

SwitchAssetPopupBox::SwitchAssetPopupBox(
	const class Project* project,
	const std::string &title,
	const ConfirmHandler &confirm, const CancelHandler &cancel
) : _title(title),
	_confirmHandler(confirm), _cancelHandler(cancel)
{
	do {
		LockGuard<RecursiveMutex>::UniquePtr acquired;
		Project* prj = project->acquire(acquired);
		if (!prj)
			break;

		prj->foreach(
			[&] (Asset* &asset, Asset::List::Index) -> void {
				Asset::States* states = asset->states();
				const Asset::States::Activity activity = states->activity();
				if (activity == Asset::States::CLOSED)
					return;

				const Entry &entry = asset->entry();
				_assets.push_back(entry.name());
			},
			true
		);

		_selection = _assets.front();
	} while (false);
}

SwitchAssetPopupBox::~SwitchAssetPopupBox() {
}

void SwitchAssetPopupBox::update(void) {
	ImGuiIO &io = GetIO();
	ImGuiStyle &style = GetStyle();

	bool isOpen = true;
	bool toConfirm = false;
	bool toCancel = false;

	if (_init.begin())
		OpenPopup(_title);

	if (BeginPopupModal(_title, _cancelHandler.empty() ? nullptr : &isOpen, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize)) {
		BeginChild("@Sel", ImVec2(256.0f * io.FontGlobalScale, 256.0f * io.FontGlobalScale), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);

		for (int i = 0; i < (int)_assets.size(); ++i) {
			const std::string asset = _assets[i];
			const bool active = _selection == asset;
			if (active) {
				PushStyleColor(ImGuiCol_Button, style.Colors[ImGuiCol_TitleBgActive]);
				PushStyleColor(ImGuiCol_ButtonHovered, style.Colors[ImGuiCol_TitleBgActive]);
				PushStyleColor(ImGuiCol_ButtonActive, style.Colors[ImGuiCol_TitleBgActive]);
			}
			if (Button(asset.c_str(), ImVec2(GetContentRegionAvail().x, 0))) {
				toConfirm = true;

				_selection = asset;

				CloseCurrentPopup();
			}
			if (active) {
				PopStyleColor(3);
			}
		}

		EndChild();

		const bool tab = IsKeyPressed(SDL_SCANCODE_TAB);
		if (tab) {
			Text::Array::iterator it = std::find(_assets.begin(), _assets.end(), _selection);
			if (it != _assets.end()) {
				int next = (int)(it - _assets.begin());
				if (io.KeyShift) {
					--next;
					if (next < 0)
						next = (int)_assets.size() - 1;
				} else {
					++next;
					if (next >= (int)_assets.size())
						next = 0;
				}
				_selection = _assets[next];
			}
		}

		if (!io.KeyCtrl) {
			toConfirm = true;

			CloseCurrentPopup();
		}

		if (IsKeyReleased(SDL_SCANCODE_ESCAPE)) {
			toCancel = true;

			CloseCurrentPopup();
		}

		if (!_init.begin() && !_init.end())
			CentralizeWindow();

		EnsureWindowVisible();

		EndPopup();
	}

	if (isOpen)
		_init.update();

	if (!isOpen)
		toCancel = true;

	if (toConfirm) {
		_init.reset();

		if (!_confirmHandler.empty())
			_confirmHandler(_selection.c_str());
	}
	if (toCancel) {
		_init.reset();

		if (!_cancelHandler.empty())
			_cancelHandler();
	}
}

ImVec2 GetMousePosOnCurrentItem(const ImVec2* ref_pos) {
	const ImVec2 ref = ref_pos ? *ref_pos : GetCursorScreenPos();

	ImVec2 pos = GetMousePos();
	pos -= ref;

	return pos;
}

void PushID(const std::string &str_id) {
	PushID(str_id.c_str(), str_id.c_str() + str_id.length());
}

Rect LastItemRect(void) {
	ImGuiWindow* window = GetCurrentWindow();

	return std::make_pair(window->DC.LastItemRect.Min, window->DC.LastItemRect.Max);
}

void Dummy(const ImVec2 &size, ImU32 col) {
	ImGuiWindow* window = GetCurrentWindow();
	ImDrawList* drawList = GetWindowDrawList();

	if (window->SkipItems)
		return;

	const ImVec2 pos = window->DC.CursorPos;
	const ImRect bb(pos, pos + size);
	ItemSize(size);
	ItemAdd(bb, 0);

	drawList->AddRectFilled(pos, pos + size, col);
}

void Dummy(const ImVec2 &size, const ImVec4 &col) {
	Dummy(size, ColorConvertFloat4ToU32(col));
}

ImVec2 WindowResizingPadding(void) {
	constexpr const float WINDOWS_RESIZE_FROM_EDGES_HALF_THICKNESS = 4.0f;

	ImGuiIO &io = GetIO();
	ImGuiStyle &style = GetStyle();

	ImVec2 paddingRegular = style.TouchExtraPadding;
	ImVec2 paddingForResizeFromEdges = io.ConfigWindowsResizeFromEdges ? ImMax(style.TouchExtraPadding, ImVec2(WINDOWS_RESIZE_FROM_EDGES_HALF_THICKNESS, WINDOWS_RESIZE_FROM_EDGES_HALF_THICKNESS)) : paddingRegular;

	return paddingForResizeFromEdges;
}

bool Begin(const std::string &name, bool* p_open, ImGuiWindowFlags flags) {
	return Begin(name.c_str(), p_open, flags);
}

void CentralizeWindow(void) {
	ImGuiIO &io = GetIO();

	const float maxWidth = io.DisplaySize.x/* * io.DisplayFramebufferScale.x*/;
	const float maxHeight = io.DisplaySize.y/* * io.DisplayFramebufferScale.y*/;

	const float width = GetWindowWidth();
	const float height = GetWindowHeight();

	const ImVec2 pos((maxWidth - width) * 0.5f, (maxHeight - height) * 0.5f);
	SetWindowPos(pos);
}

void EnsureWindowVisible(void) {
	ImGuiIO &io = GetIO();

	const float maxWidth = io.DisplaySize.x/* * io.DisplayFramebufferScale.x*/;
	const float maxHeight = io.DisplaySize.y/* * io.DisplayFramebufferScale.y*/;

	float currWidth = GetWindowWidth();
	float currHeight = GetWindowHeight();
	if (currWidth > maxWidth)
		currWidth = maxWidth;
	if (currHeight > maxHeight)
		currHeight = maxHeight;
	if (currWidth != maxWidth || currHeight != maxHeight)
		SetWindowSize(ImVec2(currWidth, currHeight));
	const float width = currWidth;
	const float height = currHeight;

	ImVec2 pos = GetWindowPos();
	if (pos.x < 0)
		pos.x = 0;
	if (pos.y < 0)
		pos.y = 0;
	if (pos.x + width > maxWidth)
		pos.x = maxWidth - width;
	if (pos.y + height > maxHeight)
		pos.y = maxHeight - height;
	if (pos.x < 0)
		pos.x = (maxWidth - width) * 0.5f;
	if (pos.y < 0)
		pos.y = (maxHeight - height) * 0.5f;
	if (pos != GetWindowPos())
		SetWindowPos(pos);
}

void OpenPopup(const std::string &str_id, ImGuiPopupFlags popup_flags) {
	OpenPopup(str_id.c_str(), popup_flags);
}

bool BeginPopupModal(const std::string &name, bool* p_open, ImGuiWindowFlags flags) {
	return BeginPopupModal(name.c_str(), p_open, flags);
}

float TitleBarHeight(void) {
	ImGuiStyle &style = GetStyle();

	return GetFontSize() + style.FramePadding.y * 2.0f;
}

bool TitleBarCustomButton(const char* label, ImVec2* pos, ButtonDrawer draw, const char* tooltip) {
	ImGuiStyle &style = GetStyle();
	ImGuiWindow* window = GetCurrentWindow();

	const ImGuiID id = GetID(label);

	const ImRect titleBarRect = window->TitleBarRect();
	const float padR = style.FramePadding.x;
	const float buttonSz = GetFontSize();

	ImVec2 position;
	if (pos && pos->x > 0 && pos->y > 0) {
		position = *pos;
	} else {
		position = ImVec2(
			titleBarRect.Max.x - (padR + buttonSz) * 2 - style.FramePadding.x/* - padR*/,
			titleBarRect.Min.y
		);
		if (pos)
			*pos = position;
	}
	if (pos)
		pos->x -= padR + buttonSz;

	PushClipRect(titleBarRect.Min, titleBarRect.Max, false);

	const ImRect bb(position, position + ImVec2(GetFontSize(), GetFontSize()) + style.FramePadding * 2.0f);
	const bool isClipped = !ItemAdd(bb, id);

	bool hovered = false, held = false;
	const bool pressed = ButtonBehavior(bb, id, &hovered, &held);

	if (!isClipped) {
		const ImVec2 center = bb.GetCenter();

		if (draw)
			draw(center, held, hovered, tooltip);
	}

	PopClipRect();

	return pressed;
}

ImVec2 CustomButtonAutoPosition(void) {
	return ImVec2(-1, -1);
}

bool CustomButton(const char* label, ImVec2* pos, ButtonDrawer draw, const char* tooltip) {
	ImGuiStyle &style = GetStyle();

	const ImGuiID id = GetID(label);

	const float padR = style.FramePadding.x;
	const float buttonSz = GetFontSize();

	SameLine();

	ImVec2 position = GetCursorScreenPos();
	position.x = GetWindowPos().x + GetWindowWidth() - (padR + buttonSz) - style.FramePadding.x;
	if (pos && pos->x > 0 && pos->y > 0) {
		position = *pos;
	} else {
		if (pos)
			*pos = position;
	}
	if (pos)
		pos->x -= padR + buttonSz;

	const ImRect bb(position, position + ImVec2(GetFontSize(), GetFontSize()) + style.FramePadding * 2.0f);
	const bool isClipped = !ItemAdd(bb, id);

	bool hovered = false, held = false;
	ButtonBehavior(bb, id, &hovered, &held);
	if (!hovered) {
		if (IsItemHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
			hovered = true;
	}

	bool pressed = false;
	if (hovered && bb.Contains(GetMousePos())) {
		ClearActiveID();
		SetHoveredID(id);
		pressed = IsMouseClicked(ImGuiMouseButton_Left);
	}

	if (!isClipped) {
		const ImVec2 center = bb.GetCenter();

		if (draw)
			draw(center, held, hovered, tooltip);
	}

	NewLine();

	return pressed;
}

void CustomAddButton(const ImVec2 &center, bool held, bool hovered, const char* tooltip) {
	ImGuiStyle &style = GetStyle();
	ImDrawList* drawList = GetWindowDrawList();

	const ImU32 bgCol = GetColorU32(held ? ImGuiCol_ButtonActive : ImGuiCol_ButtonHovered);
	if (hovered)
		drawList->AddRectFilled(center - ImVec2(GetFontSize() * 0.5f, GetFontSize() * 0.5f), center + ImVec2(GetFontSize() * 0.5f, GetFontSize() * 0.5f), bgCol);

	const ImU32 lnCol = GetColorU32(ImGuiCol_Text);
	const float lnExtent = GetFontSize() * 0.5f - 1;
	drawList->AddLine(center + ImVec2(-lnExtent, 0), center + ImVec2(lnExtent, 0), lnCol, 1);
	drawList->AddLine(center + ImVec2(0, -lnExtent), center + ImVec2(0, lnExtent), lnCol, 1);

	if (tooltip && IsItemHovered()) {
		VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

		SetTooltip(tooltip);
	}
}

void CustomRemoveButton(const ImVec2 &center, bool held, bool hovered, const char* tooltip) {
	ImGuiStyle &style = GetStyle();
	ImDrawList* drawList = GetWindowDrawList();

	const ImU32 bgCol = GetColorU32(held ? ImGuiCol_ButtonActive : ImGuiCol_ButtonHovered);
	if (hovered)
		drawList->AddRectFilled(center - ImVec2(GetFontSize() * 0.5f, GetFontSize() * 0.5f), center + ImVec2(GetFontSize() * 0.5f, GetFontSize() * 0.5f), bgCol);

	const ImU32 lnCol = GetColorU32(ImGuiCol_Text);
	const float lnExtent = GetFontSize() * 0.5f - 1;
	drawList->AddLine(center + ImVec2(-lnExtent, 0), center + ImVec2(lnExtent, 0), lnCol, 1);

	if (tooltip && IsItemHovered()) {
		VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

		SetTooltip(tooltip);
	}
}

void CustomRenameButton(const ImVec2 &center, bool held, bool hovered, const char* tooltip) {
	ImGuiStyle &style = GetStyle();
	ImDrawList* drawList = GetWindowDrawList();

	const ImU32 bgCol = GetColorU32(held ? ImGuiCol_ButtonActive : ImGuiCol_ButtonHovered);
	if (hovered)
		drawList->AddRectFilled(center - ImVec2(GetFontSize() * 0.5f, GetFontSize() * 0.5f), center + ImVec2(GetFontSize() * 0.5f, GetFontSize() * 0.5f), bgCol);

	const ImU32 lnCol = GetColorU32(ImGuiCol_Text);
	const float lnExtent = GetFontSize() * 0.5f - 1;
	drawList->AddRect(center + ImVec2(-lnExtent, -lnExtent * 0.5f), center + ImVec2(lnExtent, lnExtent * 0.5f), lnCol);
	drawList->AddLine(center + ImVec2(lnExtent * 0.25f, -lnExtent), center + ImVec2(lnExtent * 0.25f, lnExtent), lnCol, 1);
	drawList->AddLine(center + ImVec2(lnExtent * -0.1f, -lnExtent), center + ImVec2(lnExtent * 0.6f, -lnExtent), lnCol, 1);
	drawList->AddLine(center + ImVec2(lnExtent * -0.1f, lnExtent), center + ImVec2(lnExtent * 0.6f, lnExtent), lnCol, 1);

	if (tooltip && IsItemHovered()) {
		VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

		SetTooltip(tooltip);
	}
}

void CustomClearButton(const ImVec2 &center, bool held, bool hovered, const char* tooltip) {
	ImGuiStyle &style = GetStyle();
	ImDrawList* drawList = GetWindowDrawList();

	const ImU32 bgCol = GetColorU32(held ? ImGuiCol_ButtonActive : ImGuiCol_ButtonHovered);
	if (hovered)
		drawList->AddRectFilled(center - ImVec2(GetFontSize() * 0.5f, GetFontSize() * 0.5f), center + ImVec2(GetFontSize() * 0.5f, GetFontSize() * 0.5f), bgCol);

	const ImU32 lnCol = GetColorU32(ImGuiCol_Text);
	const float lnExtent = GetFontSize() * 0.5f - 1;
	const int lnSeg = 4;
	const int lnStep = (int)(lnExtent * 2 / (lnSeg - 1));
	const int lnHeight = lnStep * (lnSeg - 1);
	float yOff = -lnExtent + (lnExtent * 2 - lnHeight) / 2;
	for (int i = 0; i < lnSeg; ++i) {
		drawList->AddLine(center + ImVec2(-lnExtent, yOff), center + ImVec2(lnExtent, yOff), lnCol, 1);
		yOff += lnStep;
	}

	const ImU32 xCol = GetColorU32(ImVec4(0.93f, 0.27f, 0.27f, 1));
	const float xExtent = lnExtent;
	drawList->AddLine(
		center + ImVec2(-lnExtent, -lnExtent),
		center + ImVec2(-lnExtent, -lnExtent) + ImVec2(xExtent, xExtent),
		xCol, 1
	);
	drawList->AddLine(
		center + ImVec2(-lnExtent, -lnExtent) + ImVec2(0, xExtent),
		center + ImVec2(-lnExtent, -lnExtent) + ImVec2(xExtent, 0),
		xCol, 1
	);

	if (tooltip && IsItemHovered()) {
		VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

		SetTooltip(tooltip);
	}
}

void CustomMinButton(const ImVec2 &center, bool held, bool hovered, const char* tooltip) {
	ImGuiStyle &style = GetStyle();
	ImDrawList* drawList = GetWindowDrawList();

	const ImU32 bgCol = GetColorU32(held ? ImGuiCol_ButtonActive : ImGuiCol_ButtonHovered);
	if (hovered)
		drawList->AddRectFilled(center - ImVec2(GetFontSize() * 0.5f, GetFontSize() * 0.5f), center + ImVec2(GetFontSize() * 0.5f, GetFontSize() * 0.5f), bgCol);

	const ImU32 lnCol = GetColorU32(ImGuiCol_Text);
	const float lnExtent = GetFontSize() * 0.5f - 1;
	drawList->AddLine(center + ImVec2(-lnExtent, lnExtent - 1), center + ImVec2(lnExtent, lnExtent - 1), lnCol, 1);

	if (tooltip && IsItemHovered()) {
		VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

		SetTooltip(tooltip);
	}
}

void CustomMaxButton(const ImVec2 &center, bool held, bool hovered, const char* tooltip) {
	ImGuiStyle &style = GetStyle();
	ImDrawList* drawList = GetWindowDrawList();

	const ImU32 bgCol = GetColorU32(held ? ImGuiCol_ButtonActive : ImGuiCol_ButtonHovered);
	if (hovered)
		drawList->AddRectFilled(center - ImVec2(GetFontSize() * 0.5f, GetFontSize() * 0.5f), center + ImVec2(GetFontSize() * 0.5f, GetFontSize() * 0.5f), bgCol);

	const ImU32 lnCol = GetColorU32(ImGuiCol_Text);
	const float lnExtent = GetFontSize() * 0.5f - 1;
	drawList->AddRect(center + ImVec2(-lnExtent, -lnExtent), center + ImVec2(lnExtent, lnExtent), lnCol);

	if (tooltip && IsItemHovered()) {
		VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

		SetTooltip(tooltip);
	}
}

void CustomCloseButton(const ImVec2 &center, bool held, bool hovered, const char* tooltip) {
	ImGuiStyle &style = GetStyle();
	ImDrawList* drawList = GetWindowDrawList();

	const ImU32 bgCol = GetColorU32(held ? ImGuiCol_ButtonActive : ImGuiCol_ButtonHovered);
	if (hovered)
		drawList->AddRectFilled(center - ImVec2(GetFontSize() * 0.5f, GetFontSize() * 0.5f), center + ImVec2(GetFontSize() * 0.5f, GetFontSize() * 0.5f), bgCol);

	const ImU32 lnCol = GetColorU32(ImGuiCol_Text);
	const float lnExtent = GetFontSize() * 0.5f - 1;
	drawList->AddLine(center + ImVec2(-lnExtent, -lnExtent), center + ImVec2(lnExtent, lnExtent), lnCol);
	drawList->AddLine(center + ImVec2(lnExtent, -lnExtent), center + ImVec2(-lnExtent, lnExtent), lnCol);

	if (tooltip && IsItemHovered()) {
		VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

		SetTooltip(tooltip);
	}
}

void CustomPlayButton(const ImVec2 &center, bool held, bool hovered, const char* tooltip) {
	ImGuiStyle &style = GetStyle();
	ImDrawList* drawList = GetWindowDrawList();

	const ImU32 bgCol = GetColorU32(held ? ImGuiCol_ButtonActive : ImGuiCol_ButtonHovered);
	if (hovered)
		drawList->AddRectFilled(center - ImVec2(GetFontSize() * 0.5f, GetFontSize() * 0.5f), center + ImVec2(GetFontSize() * 0.5f, GetFontSize() * 0.5f), bgCol);

	const ImU32 lnCol = GetColorU32(ImGuiCol_Text);
	const float lnExtent = GetFontSize() * 0.5f - 1;
	drawList->AddTriangleFilled(
		center + ImVec2(-lnExtent, -lnExtent),
		center + ImVec2(lnExtent, 0),
		center + ImVec2(-lnExtent, lnExtent),
		lnCol
	);

	if (tooltip && IsItemHovered()) {
		VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

		SetTooltip(tooltip);
	}
}

void CustomStopButton(const ImVec2 &center, bool held, bool hovered, const char* tooltip) {
	ImGuiStyle &style = GetStyle();
	ImDrawList* drawList = GetWindowDrawList();

	const ImU32 bgCol = GetColorU32(held ? ImGuiCol_ButtonActive : ImGuiCol_ButtonHovered);
	if (hovered)
		drawList->AddRectFilled(center - ImVec2(GetFontSize() * 0.5f, GetFontSize() * 0.5f), center + ImVec2(GetFontSize() * 0.5f, GetFontSize() * 0.5f), bgCol);

	const ImU32 lnCol = GetColorU32(ImGuiCol_Text);
	const float lnExtent = GetFontSize() * 0.5f - 1;
	drawList->AddRectFilled(center + ImVec2(-lnExtent, -lnExtent), center + ImVec2(lnExtent, lnExtent), lnCol);

	if (tooltip && IsItemHovered()) {
		VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

		SetTooltip(tooltip);
	}
}

void TextUnformatted(const std::string &text) {
	TextUnformatted(text.c_str(), text.c_str() + text.length());
}

bool Url(const char* label, const char* link, bool adj) {
	ImVec2 pos = GetCursorPos();
	TextColored(ImColor(41, 148, 255, 255), label);
	if (IsItemHovered()) {
		std::string ul = label;
		std::transform(ul.begin(), ul.end(), ul.begin(), [] (char) -> char { return '_'; });
		SetCursorPos(pos);
		if (adj)
			AlignTextToFramePadding();
		TextColored(ImColor(41, 148, 255, 255), ul.c_str());
	}
	if (IsItemHovered() && IsMouseReleased(ImGuiMouseButton_Left)) { // Used `IsItemHovered()` instead of `IsItemClicked()` to avoid a clicking issue.
		if (link) {
			const std::string osstr = Unicode::toOs(link);

			Platform::surf(osstr.c_str());
		}

		return true;
	}

	return false;
}

void SetTooltip(const std::string &text) {
	SetTooltip(text.c_str());
}

void SetHelpTooltip(const std::string &text) {
	ImGuiStyle &style = GetStyle();

	TextUnformatted("[?]");
	if (!text.empty() && IsItemHovered()) {
		VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

		SetTooltip(text);
	}
}

bool Checkbox(const std::string &label, bool* v) {
	return Checkbox(label.c_str(), v);
}

void Indicator(const ImVec2 &min, const ImVec2 &max, float thickness) {
	ImDrawList* drawList = GetWindowDrawList();

	const bool tick = !!(((int)(DateTime::toSeconds(DateTime::ticks()) * 1)) % 2);
	drawList->AddRect(
		min, max,
		tick ? IM_COL32_WHITE : IM_COL32_BLACK,
		0, ImDrawFlags_RoundCornersNone,
		thickness
	);
}

void Indicator(const char* label, const ImVec2 &pos) {
	const bool tick = !!(((int)(DateTime::toSeconds(DateTime::ticks()) * 1)) % 2);
	const ImVec2 old = GetCursorPos();
	SetCursorPos(pos);
	TextColored(ColorConvertU32ToFloat4(tick ? IM_COL32(255, 0, 0, 255) : IM_COL32_BLACK_TRANS), label);
	SetCursorPos(old);
}

static bool ProgressBar(const char* label, void* p_data, const void* p_min, const void* p_max, const char* format, bool readonly) {
	// Prepare.
	ImGuiWindow* window = GetCurrentWindow();

	if (window->SkipItems)
		return false;

	ImGuiContext &g = *GetCurrentContext();
	const ImGuiStyle &style = g.Style;
	const ImGuiID id = window->GetID(label);
	const float w = CalcItemWidth();

	const ImVec2 label_size = CalcTextSize(label, nullptr, true);
	const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w, label_size.y + style.FramePadding.y * 2.0f));
	const ImRect total_bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));

	// Add an item.
	ItemSize(total_bb, style.FramePadding.y);
	if (!ItemAdd(total_bb, id, &frame_bb))
		return false;

	// Default format string when passing `nullptr`.
	if (!format)
		format = DataTypeGetInfo(ImGuiDataType_Float)->PrintFmt;

	// Tabbing or Ctrl+LMB on progress turns it into an input box.
	const bool hovered = ItemHoverable(frame_bb, id);
	bool temp_input_is_active = !readonly && TempInputIsActive(id);
	bool temp_input_start = false;
	if (!readonly && !temp_input_is_active) {
		const bool focus_requested = FocusableItemRegister(window, id);
		const bool clicked = (hovered && g.IO.MouseClicked[0]);
		if (focus_requested || clicked || g.NavActivateId == id || g.NavInputId == id) {
			SetActiveID(id, window);
			SetFocusID(id, window);
			FocusWindow(window);
			g.ActiveIdUsingNavDirMask |= (1 << ImGuiDir_Left) | (1 << ImGuiDir_Right);
			if (focus_requested || (clicked && g.IO.KeyCtrl) || g.NavInputId == id) {
				temp_input_start = true;
				FocusableItemUnregister(window);
			}
		}
	}

	// Our current specs do NOT clamp when using Ctrl+LMB manual input, but we should eventually add a flag for that.
	if (temp_input_is_active || temp_input_start)
		return TempInputScalar(frame_bb, id, label, ImGuiDataType_Float, p_data, format); // , p_min, p_max);

	// Draw frame.
	const ImU32 frame_col = GetColorU32(g.ActiveId == id ? ImGuiCol_FrameBgActive : g.HoveredId == id ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
	RenderNavHighlight(frame_bb, id);
	RenderFrame(frame_bb.Min, frame_bb.Max, frame_col, true, style.FrameRounding);

	// Slider behaviour.
	const float grab_sz = std::min(style.GrabMinSize, frame_bb.GetWidth());
	const ImRect slider_bb(frame_bb.Min.x - grab_sz, frame_bb.Min.y, frame_bb.Max.x + grab_sz, frame_bb.Max.y);
	ImRect grab_bb;
	const bool value_changed = SliderBehavior(slider_bb, id, ImGuiDataType_Float, p_data, p_min, p_max, format, ImGuiSliderFlags_None, &grab_bb);
	if (value_changed)
		MarkItemEdited(id);

	// Render grab.
	if (grab_bb.Max.x > grab_bb.Min.x) {
		window->DrawList->PushClipRect(frame_bb.Min, frame_bb.Max);
		window->DrawList->AddRectFilled(
			frame_bb.Min,
			ImVec2(grab_bb.GetCenter().x, grab_bb.Max.y),
			GetColorU32(!readonly && g.ActiveId == id ? ImGuiCol_SliderGrabActive : ImGuiCol_SliderGrab),
			style.GrabRounding
		);
		window->DrawList->PopClipRect();
	}

	// Display value using user-provided display format so user can add prefix/suffix/decorations to the value.
	char value_buf[64];
	const char* value_buf_end = value_buf + DataTypeFormatString(value_buf, IM_ARRAYSIZE(value_buf), ImGuiDataType_Float, p_data, format);
	RenderTextClipped(frame_bb.Min, frame_bb.Max, value_buf, value_buf_end, nullptr, ImVec2(0.5f, 0.5f));

	if (label_size.x > 0.0f)
		RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y), label);

	// Finish.
	return value_changed;
}

bool ProgressBar(const char* label, float* v, float v_min, float v_max, const char* format, bool readonly) {
	return ProgressBar(label, v, &v_min, &v_max, format, readonly);
}

bool Button(const std::string &label, const ImVec2 &size) {
	return Button(label.c_str(), size);
}

void CentralizeButton(int count, float width) {
	ImGuiStyle &style = GetStyle();

	const float xAdv = (GetWindowWidth() - width * count - style.ItemSpacing.x * (count - 1)) * 0.5f;
	SetCursorPosX(std::max(xAdv, 0.0f));
}

bool ColorButton(const char* desc_id, const ImVec4 &col, ImGuiColorEditFlags flags, const ImVec2 &size, const char* tooltip) {
	ImGuiStyle &style = GetStyle();

	const bool result = ColorButton(desc_id, col, flags, size);
	if (tooltip && IsItemHovered()) {
		VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

		SetTooltip(tooltip);
	}

	return result;
}

bool ImageButton(ImTextureID user_texture_id, const ImVec2 &size, const ImVec4& tint_col, bool selected, const char* tooltip) {
	ImGuiStyle &style = GetStyle();

	if (selected) {
		const ImVec4 btnCol = GetStyleColorVec4(ImGuiCol_CheckMark);
		PushStyleColor(ImGuiCol_Button, btnCol);
		PushStyleColor(ImGuiCol_ButtonHovered, btnCol);
		PushStyleColor(ImGuiCol_ButtonActive, btnCol);
	}

	const bool result = ImageButton(user_texture_id, size, ImVec2(0, 0), ImVec2(1, 1), -1, ImVec4(0, 0, 0, 0), tint_col);

	if (selected) {
		PopStyleColor(3);
	}

	if (tooltip && IsItemHovered()) {
		VariableGuard<decltype(style.WindowPadding)> guardWindowPadding(&style.WindowPadding, style.WindowPadding, ImVec2(WIDGETS_TOOLTIP_PADDING, WIDGETS_TOOLTIP_PADDING));

		SetTooltip(tooltip);
	}

	return result;
}

static void NineGridsImageHorizontally(ImTextureID texture_id, const ImVec2 &src_size, const ImVec2 &dst_size, bool top_down) {
	ImGuiWindow* window = GetCurrentWindow();
	ImDrawList* drawList = GetWindowDrawList();

	if (window->SkipItems)
		return;

	const ImVec2 pos = window->DC.CursorPos;
	const ImRect bb(pos, pos + dst_size);
	ItemSize(dst_size);
	ItemAdd(bb, 0);

	const float width = src_size.x / 3.0f;
	const float height = src_size.y / 3.0f;

	auto middle = [&] (void) -> void {
		drawList->AddImage( // Middle-middle.
			texture_id,
			ImVec2(bb.Min.x, bb.Min.y), ImVec2(bb.Max.x, bb.Max.y),
			ImVec2(1.0f / 3.0f, 1.0f / 3.0f), ImVec2(2.0f / 3.0f, 2.0f / 3.0f)
		);
		drawList->AddImage( // Middle-left.
			texture_id,
			ImVec2(bb.Min.x, bb.Min.y), ImVec2(bb.Min.x + width, bb.Max.y),
			ImVec2(0.0f, 1.0f / 3.0f), ImVec2(1.0f / 3.0f, 2.0f / 3.0f)
		);
		drawList->AddImage( // Middle-right.
			texture_id,
			ImVec2(bb.Max.x - width, bb.Min.y), ImVec2(bb.Max.x, bb.Max.y),
			ImVec2(2.0f / 3.0f, 1.0f / 3.0f), ImVec2(1.0f, 2.0f / 3.0f)
		);
	};
	auto top = [&] (void) -> void {
		drawList->AddImage( // Top-middle.
			texture_id,
			ImVec2(bb.Min.x, bb.Min.y), ImVec2(bb.Max.x, bb.Min.y + height),
			ImVec2(1.0f / 3.0f, 0.0f), ImVec2(2.0f / 3.0f, 1.0f / 3.0f)
		);
		drawList->AddImage( // Top-left.
			texture_id,
			ImVec2(bb.Min.x, bb.Min.y), ImVec2(bb.Min.x + width, bb.Min.y + height),
			ImVec2(0.0f, 0.0f), ImVec2(1.0f / 3.0f, 1.0f / 3.0f)
		);
		drawList->AddImage( // Top-right.
			texture_id,
			ImVec2(bb.Max.x - width, bb.Min.y), ImVec2(bb.Max.x, bb.Min.y + height),
			ImVec2(2.0f / 3.0f, 0.0f), ImVec2(1.0f, 1.0f / 3.0f)
		);
	};
	auto bottom = [&] (void) -> void {
		drawList->AddImage( // Bottom-middle.
			texture_id,
			ImVec2(bb.Min.x, bb.Max.y - height), ImVec2(bb.Max.x, bb.Max.y),
			ImVec2(1.0f / 3.0f, 2.0f / 3.0f), ImVec2(2.0f / 3.0f, 1.0f)
		);
		drawList->AddImage( // Bottom-left.
			texture_id,
			ImVec2(bb.Min.x, bb.Max.y - height), ImVec2(bb.Min.x + width, bb.Max.y),
			ImVec2(0.0f, 2.0f / 3.0f), ImVec2(1.0f / 3.0f, 1.0f)
		);
		drawList->AddImage( // Bottom-right.
			texture_id,
			ImVec2(bb.Max.x - width, bb.Max.y - height), ImVec2(bb.Max.x, bb.Max.y),
			ImVec2(2.0f / 3.0f, 2.0f / 3.0f), ImVec2(1.0f, 1.0f)
		);
	};

	if (top_down) {
		middle();
		top();
		bottom();
	} else {
		middle();
		bottom();
		top();
	}
}

static void NineGridsImageVertically(ImTextureID texture_id, const ImVec2 &src_size, const ImVec2 &dst_size, bool left_to_right) {
	ImGuiWindow* window = GetCurrentWindow();
	ImDrawList* drawList = GetWindowDrawList();

	if (window->SkipItems)
		return;

	const ImVec2 pos = window->DC.CursorPos;
	const ImRect bb(pos, pos + dst_size);
	ItemSize(dst_size);
	ItemAdd(bb, 0);

	const float width = src_size.x / 3.0f;
	const float height = src_size.y / 3.0f;

	auto middle = [&] (void) -> void {
		drawList->AddImage( // Middle-middle.
			texture_id,
			ImVec2(bb.Min.x, bb.Min.y), ImVec2(bb.Max.x, bb.Max.y),
			ImVec2(1.0f / 3.0f, 1.0f / 3.0f), ImVec2(2.0f / 3.0f, 2.0f / 3.0f)
		);
		drawList->AddImage( // Top-middle.
			texture_id,
			ImVec2(bb.Min.x, bb.Min.y), ImVec2(bb.Max.x, bb.Min.y + height),
			ImVec2(1.0f / 3.0f, 0.0f), ImVec2(2.0f / 3.0f, 1.0f / 3.0f)
		);
		drawList->AddImage( // Bottom-middle.
			texture_id,
			ImVec2(bb.Min.x, bb.Max.y - height), ImVec2(bb.Max.x, bb.Max.y),
			ImVec2(1.0f / 3.0f, 2.0f / 3.0f), ImVec2(2.0f / 3.0f, 1.0f)
		);
	};
	auto left = [&] (void) -> void {
		drawList->AddImage( // Middle-left.
			texture_id,
			ImVec2(bb.Min.x, bb.Min.y), ImVec2(bb.Min.x + width, bb.Max.y),
			ImVec2(0.0f, 1.0f / 3.0f), ImVec2(1.0f / 3.0f, 2.0f / 3.0f)
		);
		drawList->AddImage( // Top-left.
			texture_id,
			ImVec2(bb.Min.x, bb.Min.y), ImVec2(bb.Min.x + width, bb.Min.y + height),
			ImVec2(0.0f, 0.0f), ImVec2(1.0f / 3.0f, 1.0f / 3.0f)
		);
		drawList->AddImage( // Bottom-left.
			texture_id,
			ImVec2(bb.Min.x, bb.Max.y - height), ImVec2(bb.Min.x + width, bb.Max.y),
			ImVec2(0.0f, 2.0f / 3.0f), ImVec2(1.0f / 3.0f, 1.0f)
		);
	};
	auto right = [&] (void) -> void {
		drawList->AddImage( // Middle-right.
			texture_id,
			ImVec2(bb.Max.x - width, bb.Min.y), ImVec2(bb.Max.x, bb.Max.y),
			ImVec2(2.0f / 3.0f, 1.0f / 3.0f), ImVec2(1.0f, 2.0f / 3.0f)
		);
		drawList->AddImage( // Top-right.
			texture_id,
			ImVec2(bb.Max.x - width, bb.Min.y), ImVec2(bb.Max.x, bb.Min.y + height),
			ImVec2(2.0f / 3.0f, 0.0f), ImVec2(1.0f, 1.0f / 3.0f)
		);
		drawList->AddImage( // Bottom-right.
			texture_id,
			ImVec2(bb.Max.x - width, bb.Max.y - height), ImVec2(bb.Max.x, bb.Max.y),
			ImVec2(2.0f / 3.0f, 2.0f / 3.0f), ImVec2(1.0f, 1.0f)
		);
	};

	if (left_to_right) {
		middle();
		right();
		left();
	} else {
		middle();
		left();
		right();
	}
}

void NineGridsImage(ImTextureID texture_id, const ImVec2 &src_size, const ImVec2 &dst_size, bool horizontal, bool normal) {
	if (horizontal)
		NineGridsImageHorizontally(texture_id, src_size, dst_size, normal);
	else
		NineGridsImageVertically(texture_id, src_size, dst_size, normal);
}

bool BeginMenu(const std::string &label, bool enabled) {
	return BeginMenu(label.c_str(), enabled);
}

bool MenuItem(const std::string &label, const char* shortcut, bool selected, bool enabled) {
	return MenuItem(label.c_str(), shortcut, selected, enabled);
}

bool MenuItem(const std::string &label, const char* shortcut, bool* selected, bool enabled) {
	return MenuItem(label.c_str(), shortcut, selected, enabled);
}

float ColorPickerMinWidthForInput(void) {
	return 186;
}

float TabBarHeight(void) {
	ImGuiStyle &style = GetStyle();

	return GetFontSize() + style.FramePadding.y * 2.0f;
}

bool BeginTabItem(const std::string &str_id, const std::string &label, bool* p_open, ImGuiTabItemFlags flags) {
	PushID(str_id);
	const bool result = BeginTabItem(label.c_str(), p_open, flags);
	PopID();

	return result;
}

bool BeginTabItem(const std::string &label, bool* p_open, ImGuiTabItemFlags flags) {
	return BeginTabItem(label.c_str(), p_open, flags);
}

bool BeginTabItem(const std::string &label, bool* p_open, ImGuiTabItemFlags flags, ImU32 col) {
	PushStyleColor(ImGuiCol_Text, col);
	const bool result = BeginTabItem(label.c_str(), p_open, flags);
	PopStyleColor();

	return result;
}

void TabBarTabListPopupButton(TabBarDropper dropper) {
	ImGuiStyle &style = GetStyle();

	ImGuiWindow* window = GetCurrentWindow();
	ImGuiTabBar* tab_bar = GImGui->CurrentTabBar;

	const float tab_list_popup_button_width = GetFontSize() + style.FramePadding.y;
	const ImVec2 backup_cursor_pos = window->DC.CursorPos;
	window->DC.CursorPos = ImVec2(tab_bar->BarRect.Min.x - style.FramePadding.y, tab_bar->BarRect.Min.y);
	tab_bar->BarRect.Min.x += tab_list_popup_button_width;

	ImVec4 arrow_col = GetStyleColorVec4(ImGuiCol_Text);
	arrow_col.w *= 0.5f;
	PushStyleColor(ImGuiCol_Text, arrow_col);
	PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
	const bool open = BeginCombo("##V", nullptr, ImGuiComboFlags_NoPreview);
	PopStyleColor(2);

	if (open) {
		if (dropper)
			dropper();

		EndCombo();
	}

	window->DC.CursorPos = backup_cursor_pos;
}

bool BeginTable(const std::string &str_id, int column, ImGuiTableFlags flags, const ImVec2 &outer_size, float inner_width) {
	return BeginTable(str_id.c_str(), column, flags, outer_size, inner_width);
}

void TableSetupColumn(const std::string &label, ImGuiTableColumnFlags flags, float init_width_or_weight, ImU32 user_id) {
	TableSetupColumn(label.c_str(), flags, init_width_or_weight, user_id);
}

static bool TreeNodeBehavior(ImGuiID id, ImTextureID texture_id, ImTextureID open_tex_id, bool* checked, ImGuiTreeNodeFlags flags, const char* label, const char* label_end, ImGuiButtonFlags button_flags, ImU32 col) {
	// See: `TreeNodeBehavior` in "./lib/imgui/imgui_widgets.cpp".

	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const bool display_frame = (flags & ImGuiTreeNodeFlags_Framed) != 0;
	const ImVec2 padding = (display_frame || (flags & ImGuiTreeNodeFlags_FramePadding)) ? style.FramePadding : ImVec2(style.FramePadding.x, ImMin(window->DC.CurrLineTextBaseOffset, style.FramePadding.y));

	if (!label_end)
		label_end = FindRenderedTextEnd(label);
	const ImVec2 label_size = CalcTextSize(label, label_end, false);

	// We vertically grow up to current line height up the typical widget height.
	const float frame_height = ImMax(ImMin(window->DC.CurrLineSize.y, g.FontSize + style.FramePadding.y * 2), label_size.y + padding.y * 2);
	ImRect frame_bb;
	frame_bb.Min.x = (flags & ImGuiTreeNodeFlags_SpanFullWidth) ? window->WorkRect.Min.x : window->DC.CursorPos.x;
	frame_bb.Min.y = window->DC.CursorPos.y;
	frame_bb.Max.x = window->WorkRect.Max.x;
	frame_bb.Max.y = window->DC.CursorPos.y + frame_height;
	if (display_frame)
	{
		// Framed header expand a little outside the default padding, to the edge of InnerClipRect
		// (FIXME: May remove this at some point and make InnerClipRect align with WindowPadding.x instead of WindowPadding.x*0.5f)
		frame_bb.Min.x -= IM_FLOOR(window->WindowPadding.x * 0.5f - 1.0f);
		frame_bb.Max.x += IM_FLOOR(window->WindowPadding.x * 0.5f);
	}

	const float text_offset_x = g.FontSize + (display_frame ? padding.x * 3 : padding.x * 2);           // Collapser arrow width + Spacing
	const float text_offset_y = ImMax(padding.y, window->DC.CurrLineTextBaseOffset);                    // Latch before ItemSize changes it
	const float text_width = g.FontSize + (label_size.x > 0.0f ? label_size.x + padding.x * 2 : 0.0f);  // Include collapser
	ImVec2 text_pos(window->DC.CursorPos.x + text_offset_x, window->DC.CursorPos.y + text_offset_y);
	ItemSize(ImVec2(text_width, frame_height), padding.y);

	// For regular tree nodes, we arbitrary allow to click past 2 worth of ItemSpacing
	ImRect interact_bb = frame_bb;
	if (!display_frame && (flags & (ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_SpanFullWidth)) == 0) {
		//interact_bb.Max.x = frame_bb.Min.x + text_width + style.ItemSpacing.x * 2.0f;
		interact_bb.Max.x = frame_bb.Min.x + frame_bb.GetWidth();
	}

	// Store a flag for the current depth to tell if we will allow closing this node when navigating one of its child.
	// For this purpose we essentially compare if g.NavIdIsAlive went from 0 to 1 between TreeNode() and TreePop().
	// This is currently only support 32 level deep and we are fine with (1 << Depth) overflowing into a zero.
	const bool is_leaf = (flags & ImGuiTreeNodeFlags_Leaf) != 0;
	bool is_open = TreeNodeBehaviorIsOpen(id, flags);
	if (is_open && !g.NavIdIsAlive && (flags & ImGuiTreeNodeFlags_NavLeftJumpsBackHere) && !(flags & ImGuiTreeNodeFlags_NoTreePushOnOpen))
		window->DC.TreeJumpToParentOnPopMask |= (1 << window->DC.TreeDepth);

	bool item_add = ItemAdd(interact_bb, id);
	window->DC.LastItemStatusFlags |= ImGuiItemStatusFlags_HasDisplayRect;
	window->DC.LastItemDisplayRect = frame_bb;

	if (!item_add)
	{
		if (is_open && !(flags & ImGuiTreeNodeFlags_NoTreePushOnOpen))
			TreePushOverrideID(id);
		IMGUI_TEST_ENGINE_ITEM_INFO(window->DC.LastItemId, label, window->DC.ItemFlags | (is_leaf ? 0 : ImGuiItemStatusFlags_Openable) | (is_open ? ImGuiItemStatusFlags_Opened : 0));
		return is_open;
	}

	if (flags & ImGuiTreeNodeFlags_AllowItemOverlap)
		button_flags |= ImGuiButtonFlags_AllowItemOverlap;
	if (!is_leaf)
		button_flags |= ImGuiButtonFlags_PressedOnDragDropHold;

	// We allow clicking on the arrow section with keyboard modifiers held, in order to easily
	// allow browsing a tree while preserving selection with code implementing multi-selection patterns.
	// When clicking on the rest of the tree node we always disallow keyboard modifiers.
	const float arrow_hit_x1 = (text_pos.x - text_offset_x) - style.TouchExtraPadding.x;
	const float arrow_hit_x2 = (text_pos.x - text_offset_x) + (g.FontSize + padding.x * 2.0f) + style.TouchExtraPadding.x;
	const bool is_mouse_x_over_arrow = (g.IO.MousePos.x >= arrow_hit_x1 && g.IO.MousePos.x < arrow_hit_x2);
	if (window != g.HoveredWindow || !is_mouse_x_over_arrow)
		button_flags |= ImGuiButtonFlags_NoKeyModifiers;

	// Open behaviors can be altered with the _OpenOnArrow and _OnOnDoubleClick flags.
	// Some alteration have subtle effects (e.g. toggle on MouseUp vs MouseDown events) due to requirements for multi-selection and drag and drop support.
	// - Single-click on label = Toggle on MouseUp (default, when _OpenOnArrow=0)
	// - Single-click on arrow = Toggle on MouseDown (when _OpenOnArrow=0)
	// - Single-click on arrow = Toggle on MouseDown (when _OpenOnArrow=1)
	// - Double-click on label = Toggle on MouseDoubleClick (when _OpenOnDoubleClick=1)
	// - Double-click on arrow = Toggle on MouseDoubleClick (when _OpenOnDoubleClick=1 and _OpenOnArrow=0)
	// It is rather standard that arrow click react on Down rather than Up.
	// We set ImGuiButtonFlags_PressedOnClickRelease on OpenOnDoubleClick because we want the item to be active on the initial MouseDown in order for drag and drop to work.
	if (is_mouse_x_over_arrow)
		button_flags |= ImGuiButtonFlags_PressedOnClick;
	else if (flags & ImGuiTreeNodeFlags_OpenOnDoubleClick)
		button_flags |= ImGuiButtonFlags_PressedOnClickRelease | ImGuiButtonFlags_PressedOnDoubleClick;
	else
		button_flags |= ImGuiButtonFlags_PressedOnClickRelease;

	bool selected = (flags & ImGuiTreeNodeFlags_Selected) != 0;
	const bool was_selected = selected;

	bool hovered = false, held = false;
	bool pressed = checked ? false : ButtonBehavior(interact_bb, id, &hovered, &held, button_flags);
	bool toggled = false;
	if (!is_leaf)
	{
		if (pressed && g.DragDropHoldJustPressedId != id)
		{
			if ((flags & (ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick)) == 0 || (g.NavActivateId == id))
				toggled = true;
			if (flags & ImGuiTreeNodeFlags_OpenOnArrow)
				toggled |= is_mouse_x_over_arrow && !g.NavDisableMouseHover; // Lightweight equivalent of IsMouseHoveringRect() since ButtonBehavior() already did the job
			if ((flags & ImGuiTreeNodeFlags_OpenOnDoubleClick) && g.IO.MouseDoubleClicked[0])
				toggled = true;
		}
		else if (pressed && g.DragDropHoldJustPressedId == id)
		{
			IM_ASSERT(button_flags & ImGuiButtonFlags_PressedOnDragDropHold);
			if (!is_open) // When using Drag and Drop "hold to open" we keep the node highlighted after opening, but never close it again.
				toggled = true;
		}

		if (g.NavId == id && g.NavMoveRequest && g.NavMoveDir == ImGuiDir_Left && is_open)
		{
			toggled = true;
			NavMoveRequestCancel();
		}
		if (g.NavId == id && g.NavMoveRequest && g.NavMoveDir == ImGuiDir_Right && !is_open) // If there's something upcoming on the line we may want to give it the priority?
		{
			toggled = true;
			NavMoveRequestCancel();
		}

		if (toggled)
		{
			is_open = !is_open;
			window->DC.StateStorage->SetInt(id, is_open);
			window->DC.LastItemStatusFlags |= ImGuiItemStatusFlags_ToggledOpen;
		}
	}
	if (flags & ImGuiTreeNodeFlags_AllowItemOverlap)
		SetItemAllowOverlap();

	// In this branch, TreeNodeBehavior() cannot toggle the selection so this will never trigger.
	if (selected != was_selected) //-V547
		window->DC.LastItemStatusFlags |= ImGuiItemStatusFlags_ToggledSelection;

	// Render
	const ImU32 text_col = GetColorU32(ImGuiCol_Text);
	ImGuiNavHighlightFlags nav_highlight_flags = ImGuiNavHighlightFlags_TypeThin;
	if (display_frame)
	{
		// Framed type
		const ImU32 bg_col = GetColorU32((held && hovered) ? ImGuiCol_HeaderActive : hovered ? ImGuiCol_HeaderHovered : ImGuiCol_Header);
		RenderFrame(frame_bb.Min, frame_bb.Max, bg_col, true, style.FrameRounding);
		RenderNavHighlight(frame_bb, id, nav_highlight_flags);
		if (flags & ImGuiTreeNodeFlags_Bullet)
		{
			RenderBullet(window->DrawList, ImVec2(text_pos.x - text_offset_x * 0.60f, text_pos.y + g.FontSize * 0.5f), text_col);
		}
		else if (!is_leaf)
		{
			//RenderArrow(window->DrawList, ImVec2(text_pos.x - text_offset_x + padding.x, text_pos.y), text_col, is_open ? ImGuiDir_Down : ImGuiDir_Right, 1.0f);
			const float offset_x = text_offset_x - 1;
			if (checked)
			{
				SetCursorScreenPos(ImVec2(text_pos.x - offset_x, text_pos.y));
				Checkbox(label, checked);
			}
			else
			{
				window->DrawList->AddImage(
					is_open ? open_tex_id : texture_id,
					ImVec2(text_pos.x - offset_x, text_pos.y), ImVec2(text_pos.x - offset_x, text_pos.y) + ImVec2(g.FontSize, g.FontSize),
					ImVec2(0, 0), ImVec2(1, 1),
					col
				);
			}
		}
		else // Leaf without bullet, left-adjusted text
			text_pos.x -= text_offset_x;
		if (flags & ImGuiTreeNodeFlags_ClipLabelForTrailingButton)
			frame_bb.Max.x -= g.FontSize + style.FramePadding.x;
		if (g.LogEnabled)
		{
			// NB: '##' is normally used to hide text (as a library-wide feature), so we need to specify the text range to make sure the ## aren't stripped out here.
			const char log_prefix[] = "\n##";
			const char log_suffix[] = "##";
			LogRenderedText(&text_pos, log_prefix, log_prefix + 3);
			RenderTextClipped(text_pos, frame_bb.Max, label, label_end, &label_size);
			LogRenderedText(&text_pos, log_suffix, log_suffix + 2);
		}
		else
		{
			RenderTextClipped(text_pos, frame_bb.Max, label, label_end, &label_size);
		}
	}
	else
	{
		// Unframed typed for tree nodes
		if (hovered || selected)
		{
			const ImU32 bg_col = GetColorU32((held && hovered) ? ImGuiCol_HeaderActive : hovered ? ImGuiCol_HeaderHovered : ImGuiCol_Header);
			RenderFrame(frame_bb.Min, frame_bb.Max, bg_col, false);
			RenderNavHighlight(frame_bb, id, nav_highlight_flags);
		}
		if (flags & ImGuiTreeNodeFlags_Bullet)
		{
			RenderBullet(window->DrawList, ImVec2(text_pos.x - text_offset_x * 0.5f, text_pos.y + g.FontSize * 0.5f), text_col);
		}
		else if (!is_leaf)
		{
			//RenderArrow(window->DrawList, ImVec2(text_pos.x - text_offset_x + padding.x, text_pos.y + g.FontSize * 0.15f), text_col, is_open ? ImGuiDir_Down : ImGuiDir_Right, 0.70f);
			const float offset_x = text_offset_x - 1;
			if (checked)
			{
				SetCursorScreenPos(ImVec2(text_pos.x - offset_x, text_pos.y));
				Checkbox(label, checked);
			}
			else
			{
				window->DrawList->AddImage(
					is_open ? open_tex_id : texture_id,
					ImVec2(text_pos.x - offset_x, text_pos.y), ImVec2(text_pos.x - offset_x, text_pos.y) + ImVec2(g.FontSize, g.FontSize),
					ImVec2(0, 0), ImVec2(1, 1),
					col
				);
			}
		}
		if (g.LogEnabled)
			LogRenderedText(&text_pos, ">");
		if (!checked)
			RenderText(text_pos, label, label_end, false);
	}

	if (is_open && !(flags & ImGuiTreeNodeFlags_NoTreePushOnOpen))
		TreePushOverrideID(id);
	IMGUI_TEST_ENGINE_ITEM_INFO(id, label, window->DC.ItemFlags | (is_leaf ? 0 : ImGuiItemStatusFlags_Openable) | (is_open ? ImGuiItemStatusFlags_Opened : 0));
	return is_open;
}

bool TreeNode(ImTextureID texture_id, ImTextureID open_tex_id, const std::string &label, ImGuiTreeNodeFlags flags, ImGuiButtonFlags button_flags, ImU32 col) {
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	return TreeNodeBehavior(GetID(label.c_str()), texture_id, open_tex_id, nullptr, flags, label.c_str(), nullptr, button_flags, col);
}

bool TreeNode(bool* checked, const std::string &label, ImGuiTreeNodeFlags flags, ImGuiButtonFlags button_flags) {
	ImGuiWindow* window = GetCurrentWindow();
	if (window->SkipItems)
		return false;

	return TreeNodeBehavior(GetID(label.c_str()), nullptr, nullptr, checked, flags, label.c_str(), nullptr, button_flags, IM_COL32_WHITE);
}

bool Selectable(const std::string &label, bool selected, ImGuiSelectableFlags flags, const ImVec2 &size) {
	return Selectable(label.c_str(), selected, flags, size);
}

bool Selectable(const std::string &label, bool* p_selected, ImGuiSelectableFlags flags, const ImVec2 &size) {
	return Selectable(label.c_str(), p_selected, flags, size);
}

void RefSelector(
	const class Project* project,
	Text::Array &refs, int* ref_index,
	unsigned type,
	const char* none, const char* palette, const char* reference
) {
	int refIdx = ref_index ? *ref_index : 0;
	const char* refStr = reference ? reference : "Reference:";
	unsigned expType = 0;
	switch (type) {
	case Image::TYPE():
		refStr = palette ? palette : "Palette:";
		expType = Palette::TYPE();

		break;
	case Sprite::TYPE():
		expType = Image::TYPE();

		break;
	case Map::TYPE():
		expType = Image::TYPE();

		break;
	default:
		assert(false && "Impossible.");

		return;
	}

	if (refIdx == -1 && refs.empty()) {
		refs.push_back(none ? none : "<None>");
		do {
			LockGuard<RecursiveMutex>::UniquePtr acquired;
			Project* prj = project->acquire(acquired);
			if (!prj)
				break;

			prj->foreach(
				[&] (Asset* &asset, Asset::List::Index) -> void {
					if (asset->type() != expType)
						return;

					const Entry &entry = asset->entry();
					refs.push_back(entry.name());
				}
			);
		} while (false);

		refIdx = refs.size() == 1 ? 0 : 1;
	}

	TextUnformatted(refStr);
	Combo(
		"",
		&refIdx,
		[] (void* data, int idx, const char** outText) -> bool {
			Text::Array* refs = (Text::Array*)data;
			*outText = refs->at(idx).c_str();

			return true;
		},
		&refs,
		(int)refs.size()
	);

	if (ref_index)
		*ref_index = refIdx;
}

void AssetSelectAll(
	const class Project* project,
	Text::Set &selected,
	AssetFilter filter
) {
	selected.clear();

	Hierarchy hierarchy(
		[] (const std::string &) -> bool {
			return true;
		},
		[] (void) -> void {
		}
	);
	hierarchy.prepare();

	do {
		LockGuard<RecursiveMutex>::UniquePtr acquired;
		Project* prj = project->acquire(acquired);
		if (!prj)
			break;

		prj->foreach(
			[&] (Asset* &asset, Asset::List::Index) -> void {
				if (filter && filter(asset))
					return;

				const Entry &entry = asset->entry();

				Text::Array::const_iterator begin = entry.parts().begin();
				Text::Array::const_iterator end = entry.parts().end() - 1;
				if (entry.parts().size() == 1) {
					begin = entry.parts().end();
					end = entry.parts().end();
				}

				if (hierarchy.with(begin, end)) {
					const std::string &full = entry.name();
					selected.insert(full);
				}
			}
		);
	} while (false);

	hierarchy.finish();
}

bool AssetSelector(
	const class Project* project,
	Text::Set &selected,
	ImTextureID dir_tex_id, ImTextureID open_dir_tex_id,
	ImU32 col,
	AssetFilter filter,
	int* total
) {
	bool result = false;

	if (total)
		*total = 0;

	Hierarchy hierarchy(
		[&] (const std::string &dir) -> bool {
			return TreeNode(dir_tex_id, open_dir_tex_id, dir, ImGuiTreeNodeFlags_None, ImGuiButtonFlags_None, col);
		},
		[] (void) -> void {
			TreePop();
		}
	);
	hierarchy.prepare();

	do {
		LockGuard<RecursiveMutex>::UniquePtr acquired;
		Project* prj = project->acquire(acquired);
		if (!prj)
			break;

		prj->foreach(
			[&] (Asset* &asset, Asset::List::Index) -> void {
				if (filter && filter(asset))
					return;

				if (total)
					++*total;

				const Entry &entry = asset->entry();

				Text::Array::const_iterator begin = entry.parts().begin();
				Text::Array::const_iterator end = entry.parts().end() - 1;
				if (entry.parts().size() == 1) {
					begin = entry.parts().end();
					end = entry.parts().end();
				}

				if (hierarchy.with(begin, end)) {
					const std::string &file = entry.parts().back();
					const std::string &full = entry.name();
					const Text::Set::iterator it = selected.find(full);
					const bool wasChecked = it != selected.end();
					bool checked = wasChecked;
					if (TreeNode(&checked, file, ImGuiTreeNodeFlags_None, ImGuiButtonFlags_None)) {
						TreePop();
					}
					if (wasChecked && !checked) {
						selected.erase(it);
						result = true;
					} else if (!wasChecked && checked) {
						selected.insert(full);
						result = true;
					}
				}
			}
		);
	} while (false);

	hierarchy.finish();

	return result;
}

bool AssetSelector(
	const class Project* project,
	std::string &selected,
	ImTextureID dir_tex_id, ImTextureID open_dir_tex_id, ImTextureID file_tex_id,
	ImU32 col,
	AssetFilter filter,
	int* total
) {
	bool result = false;

	if (total)
		*total = 0;

	Hierarchy hierarchy(
		[&] (const std::string &dir) -> bool {
			return TreeNode(dir_tex_id, open_dir_tex_id, dir, ImGuiTreeNodeFlags_None, ImGuiButtonFlags_None, col);
		},
		[] (void) -> void {
			TreePop();
		}
	);
	hierarchy.prepare();

	do {
		LockGuard<RecursiveMutex>::UniquePtr acquired;
		Project* prj = project->acquire(acquired);
		if (!prj)
			break;

		prj->foreach(
			[&] (Asset* &asset, Asset::List::Index) -> void {
				if (filter && filter(asset))
					return;

				if (total)
					++*total;

				const Entry &entry = asset->entry();

				Text::Array::const_iterator begin = entry.parts().begin();
				Text::Array::const_iterator end = entry.parts().end() - 1;
				if (entry.parts().size() == 1) {
					begin = entry.parts().end();
					end = entry.parts().end();
				}

				if (hierarchy.with(begin, end)) {
					const std::string &file = entry.parts().back();
					ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None;
					if (selected == entry.name()) {
						flags |= ImGuiTreeNodeFlags_Selected;
					}
					if (TreeNode(file_tex_id, file_tex_id, file, flags, ImGuiButtonFlags_None, col)) {
						TreePop();
					}
					if (IsItemClicked()) {
						selected = entry.name();

						result = true;
					}
				}
			}
		);
	} while (false);

	hierarchy.finish();

	return result;
}

bool AssetMenu(
	const class Project* project,
	std::string &selected,
	AssetFilter filter
) {
	bool result = false;

	Hierarchy hierarchy(
		[] (const std::string &dir) -> bool {
			return BeginMenu(dir);
		},
		[] (void) -> void {
			EndMenu();
		}
	);
	hierarchy.prepare();

	do {
		LockGuard<RecursiveMutex>::UniquePtr acquired;
		Project* prj = project->acquire(acquired);
		if (!prj)
			break;

		prj->foreach(
			[&] (Asset* &asset, Asset::List::Index) -> void {
				if (filter && filter(asset))
					return;

				const Entry &entry = asset->entry();

				Text::Array::const_iterator begin = entry.parts().begin();
				Text::Array::const_iterator end = entry.parts().end() - 1;
				if (entry.parts().size() == 1) {
					begin = entry.parts().end();
					end = entry.parts().end();
				}

				if (hierarchy.with(begin, end)) {
					const std::string &file = entry.parts().back();
					ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None;
					if (selected == entry.name()) {
						flags |= ImGuiTreeNodeFlags_Selected;
					}
					if (MenuItem(file)) {
						selected = entry.name();

						result = true;
					}
				}
			}
		);
	} while (false);

	hierarchy.finish();

	return result;
}

bool ExampleMenu(
	const class Project*,
	const Entry::Dictionary &examples,
	std::string &selected
) {
	bool result = false;

	selected.clear();

	Hierarchy hierarchy(
		[] (const std::string &dir) -> bool {
			return BeginMenu(dir);
		},
		[] (void) -> void {
			EndMenu();
		}
	);
	hierarchy.prepare();

	for (Entry::Dictionary::value_type kv : examples) {
		const Entry &entry = kv.first;
		const std::string path = kv.second;
		Text::Array::const_iterator begin = entry.parts().begin();
		Text::Array::const_iterator end = entry.parts().end() - 1;
		if (entry.parts().size() == 1) {
			begin = entry.parts().end();
			end = entry.parts().end();
		}

		if (hierarchy.with(begin, end)) {
			std::string file = entry.parts().back();
			const std::string dotBit = "." BITTY_PROJECT_EXT;
			if (Text::endsWith(file, dotBit, true))
				file = file.substr(0, file.length() - dotBit.length());
			if (MenuItem(file)) {
				selected = path;

				result = true;
			}
		}
	}

	hierarchy.finish();

	return result;
}

bool PluginMenu(
	const class Project*,
	const Plugin::Array &plugins,
	const char* menu,
	Plugin* &selected
) {
	bool result = false;

	selected = nullptr;

	Hierarchy hierarchy(
		[] (const std::string &dir) -> bool {
			return BeginMenu(dir);
		},
		[] (void) -> void {
			EndMenu();
		}
	);
	hierarchy.prepare();

	for (Plugin* plugin : plugins) {
		if (!plugin->is(Plugin::Usages::MENU))
			continue;

		const Entry &entry = plugin->entry();

		if (entry.parts().empty())
			continue;
		if (entry.parts().front() != menu)
			continue;

		Text::Array::const_iterator begin = entry.parts().begin() + 1; // Ignore the menu head.
		Text::Array::const_iterator end = entry.parts().end() - 1;
		if (entry.parts().size() == 1) {
			begin = entry.parts().end();
			end = entry.parts().end();
		}

		if (hierarchy.with(begin, end)) {
			std::string file = entry.parts().back();
			const std::string dotBit = "." BITTY_PROJECT_EXT;
			if (Text::endsWith(file, dotBit, true))
				file = file.substr(0, file.length() - dotBit.length());
			if (MenuItem(file)) {
				selected = plugin;

				result = true;
			}
		}
	}

	hierarchy.finish();

	return result;
}

bool DocumentMenu(
	const class Project*,
	const Entry::Dictionary &documents,
	std::string &selected
) {
	bool result = false;

	selected.clear();

	Hierarchy hierarchy(
		[] (const std::string &dir) -> bool {
			return BeginMenu(dir);
		},
		[] (void) -> void {
			EndMenu();
		}
	);
	hierarchy.prepare();

	for (Entry::Dictionary::value_type kv : documents) {
		const Entry &entry = kv.first;
		const std::string path = kv.second;
		Text::Array::const_iterator begin = entry.parts().begin();
		Text::Array::const_iterator end = entry.parts().end() - 1;
		if (entry.parts().size() == 1) {
			begin = entry.parts().end();
			end = entry.parts().end();
		}

		if (hierarchy.with(begin, end)) {
			std::string file = entry.parts().back();
			const std::string dotBit = "." BITTY_PROJECT_EXT;
			if (Text::endsWith(file, dotBit, true))
				file = file.substr(0, file.length() - dotBit.length());
			if (MenuItem(file)) {
				selected = path;

				result = true;
			}
		}
	}

	hierarchy.finish();

	return result;
}

static void DebugVariable(const Variant &val, int level) {
	if (val.type() == Variant::STRING) {
		std::string val_ = val.toString();
		val_ = Text::replace(val_, "\r", "\\r");
		val_ = Text::replace(val_, "\n", "\\n");
		val_ = Text::replace(val_, "\t", "\\t");
		TextUnformatted(val_);

		return;
	}
	if (val.type() == Variant::POINTER) {
		const std::string val_ = (const char*)(void*)val;
		TextUnformatted(val_);

		return;
	}
	if (val.type() != Variant::OBJECT) {
		TextUnformatted(val.toString());

		return;
	}

	const Object::Ptr obj = (Object::Ptr)val;
	if (obj && Object::is<IList::Ptr>(obj)) {
		IList::Ptr lst = Object::as<IList::Ptr>(obj);
		PushID(level);
		if (lst->count() == 0) {
			TextUnformatted("[...]");
		} else if (TreeNode("[...]")) {
			if (level <= BITTY_DEBUG_TABLE_LEVEL_MAX_COUNT) {
				for (int i = 0; i < lst->count(); ++i) {
					if (i >= BITTY_DEBUG_TABLE_ITEM_MAX_COUNT) {
						const std::string more = Text::toString(lst->count() - i) + " more...";
						TextUnformatted(more);

						break;
					}

					PushID(i);
					const std::string idx = Text::toString(i + 1) + ":";
					TextUnformatted(idx); SameLine();
					const Variant item = lst->at(i);
					DebugVariable(item, level + 1);
					PopID();
				}
			} else {
				TextUnformatted("...");
			}

			TreePop();
		}
		PopID();
	} else if (obj && Object::is<IDictionary::Ptr>(obj)) {
		IDictionary::Ptr dict = Object::as<IDictionary::Ptr>(obj);
		PushID(level);
		if (dict->count() == 0) {
			TextUnformatted("{...}");
		} else if (TreeNode("{...}")) {
			if (level <= BITTY_DEBUG_TABLE_LEVEL_MAX_COUNT) {
				const IDictionary::Keys keys = dict->keys();
				int i = 0;
				for (IDictionary::Keys::const_iterator it = keys.begin(); it != keys.end() && i < (int)keys.size(); ++it, ++i) {
					if (i >= BITTY_DEBUG_TABLE_ITEM_MAX_COUNT) {
						const std::string more = Text::toString((int)keys.size() - i) + " more...";
						TextUnformatted(more);

						break;
					}

					PushID(i);
					const std::string key = *it + ":";
					TextUnformatted(key); SameLine();
					const Variant val_ = dict->get(*it);
					DebugVariable(val_, level + 1);
					PopID();
				}
			} else {
				TextUnformatted("...");
			}

			TreePop();
		}
		PopID();
	} else {
		TextUnformatted(val.toString());
	}
}

void DebugVariable(const Variant &val) {
	DebugVariable(val, 1);
}

void ConfigGamepads(
	Input* input,
	Input::Gamepad* pads, int pad_count,
	int* active_pad_index, int* active_btn_index,
	const char* label_wait
) {
	constexpr const char* const KEY_NAMES[] = {
		" Left",
		"Right",
		"   Up",
		" Down",
		"    A",
		"    B"
	};

	for (int i = 0; i < pad_count; ++i) {
		Input::Gamepad &pad = pads[i];

		constexpr const int PLACEHOLDER_INDEX = 6;
		char buf[] = {
			'P', 'l', 'a', 'y', 'e', 'r', '?', '\t', ' ', ' ', ' ', ' ', ' ', '\0'
		};
		buf[PLACEHOLDER_INDEX] = (char)(i + '1'); // Put pad index at the first placeholder.
		if (CollapsingHeader(buf, i == 0 ? ImGuiTreeNodeFlags_DefaultOpen : ImGuiTreeNodeFlags_None)) {
			for (int b = 0; b < Input::BUTTON_COUNT; ++b) {
				buf[PLACEHOLDER_INDEX + 1] = (char)(b + '1'); // Put button index at the second placeholder.

				PushID(buf);

				if (b == Input::A)
					Separator();

				const std::string key = input->nameOf(pad.buttons[b]);

				AlignTextToFramePadding();
				Text(KEY_NAMES[b]);

				SameLine();
				if (active_pad_index && *active_pad_index == i && active_btn_index && *active_btn_index == b) {
					if (Button(label_wait ? label_wait : "Waiting for input...", ImVec2(-1, 0))) {
						*active_pad_index = -1;
						*active_btn_index = -1;
					}
					Input::Button btn;
					if (input->pressed(btn)) {
						if (btn.device == Input::KEYBOARD && btn.value == SDL_SCANCODE_BACKSPACE) {
							btn.device = Input::INVALID;
							btn.index = 0;
							btn.value = 0;
						}
						pad.buttons[b] = btn;
						*active_pad_index = -1;
						*active_btn_index = -1;
					}
				} else {
					if (Button(key.c_str(), ImVec2(-1, 0))) {
						*active_pad_index = i;
						*active_btn_index = b;
					}
				}

				PopID();
			}
		}
	}
}

void ConfigOnscreenGamepad(
	bool* enabled,
	bool* swap_ab,
	float* scale, float* padding_x, float* padding_y,
	const char* label_enabled,
	const char* label_swap_ab,
	const char* label_scale, const char* label_padding_x, const char* label_padding_y
) {
	Checkbox(label_enabled ? label_enabled : "Enabled", enabled);

	Checkbox(label_swap_ab ? label_swap_ab : "Swap A/B", swap_ab);

	PushID("@Scl");
	{
		TextUnformatted(label_scale ? label_scale : "    Scale");
		SameLine();
		PushItemWidth(-1.0f);
		if (DragFloat("", scale, 0.005f, 1.0f, INPUT_GAMEPAD_MAX_SCALE, "%.1f"))
			*scale = Math::clamp(*scale, 1.0f, INPUT_GAMEPAD_MAX_SCALE);
		PopItemWidth();
	}
	PopID();

	PushID("@PadX");
	{
		TextUnformatted(label_padding_x ? label_padding_x : "Padding X");
		SameLine();
		PushItemWidth(-1.0f);
		if (DragFloat("", padding_x, 0.05f, 0.0f, INPUT_GAMEPAD_MAX_X_PADDING, "%.1f%%"))
			*padding_x = Math::clamp(*padding_x, 0.0f, INPUT_GAMEPAD_MAX_X_PADDING);
		PopItemWidth();
	}
	PopID();

	PushID("@PadY");
	{
		TextUnformatted(label_padding_y ? label_padding_y : "Padding Y");
		SameLine();
		PushItemWidth(-1.0f);
		if (DragFloat("", padding_y, 0.05f, 0.0f, INPUT_GAMEPAD_MAX_Y_PADDING, "%.1f%%"))
			*padding_y = Math::clamp(*padding_y, 0.0f, INPUT_GAMEPAD_MAX_Y_PADDING);
		PopItemWidth();
	}
	PopID();
}

}

/* ===========================================================================} */
