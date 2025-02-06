/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2025 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __WIDGETS_H__
#define __WIDGETS_H__

#include "bitty.h"
#include "entry.h"
#include "input.h"
#include "plugin.h"
#include "../lib/imgui/imgui.h"
#include <stack>

/*
** {===========================================================================
** Macros and constants
*/

#ifndef WIDGETS_BUTTON_WIDTH
#	define WIDGETS_BUTTON_WIDTH 64.0f
#endif /* WIDGETS_BUTTON_WIDTH */

#ifndef WIDGETS_TOOLTIP_PADDING
#	define WIDGETS_TOOLTIP_PADDING 8.0f
#endif /* WIDGETS_TOOLTIP_PADDING */

/* ===========================================================================} */

/*
** {===========================================================================
** Utilities
*/

bool operator == (const ImVec2 &left, const ImVec2 &right);
bool operator != (const ImVec2 &left, const ImVec2 &right);

/* ===========================================================================} */

/*
** {===========================================================================
** ImGui widgets
*/

/**
 * @brief Custom ImGui widgets.
 *   The functions follow the naming convention of the original ImGui library.
 */
namespace ImGui {

typedef std::pair<ImVec2, ImVec2> Rect;

typedef std::function<void(const ImVec2 &, bool, bool, const char*)> ButtonDrawer;

typedef std::function<void(void)> TabBarDropper;

typedef std::function<bool(const class Asset*)> AssetFilter;

class Initializer {
private:
	int _ticks = 0;

public:
	bool begin(void) const;
	bool end(void) const;
	void update(void);
	void reset(void);
};

class Hierarchy {
public:
	typedef std::function<bool(const std::string &)> BeginHandler;
	typedef std::function<void(void)> EndHandler;

private:
	BeginHandler _begin = nullptr;
	EndHandler _end = nullptr;

	std::stack<bool> _opened;
	int _dec = 0;
	Text::Array _inc;
	Text::Array _path;

public:
	Hierarchy(BeginHandler begin, EndHandler end);

	void prepare(void);
	void finish(void);

	bool with(Text::Array::const_iterator begin, Text::Array::const_iterator end);
};

class PopupBox {
public:
	typedef std::shared_ptr<PopupBox> Ptr;

public:
	PopupBox();
	virtual ~PopupBox();

	virtual void update(void) = 0;
};

class WaitingPopupBox : public PopupBox {
public:
	struct TimeoutHandler : public Handler<TimeoutHandler, void> {
		using Handler::Handler;
	};

private:
	std::string _content;

	TimeoutHandler _timeoutHandler = nullptr;

	unsigned long long _timeoutTime = 0;

	Initializer _init;

public:
	WaitingPopupBox(
		const std::string &content,
		const TimeoutHandler &confirm
	);
	virtual ~WaitingPopupBox() override;

	virtual void update(void) override;
};

class MessagePopupBox : public PopupBox {
public:
	struct ConfirmHandler : public Handler<ConfirmHandler, void> {
		using Handler::Handler;
	};
	struct DenyHandler : public Handler<DenyHandler, void> {
		using Handler::Handler;
	};
	struct CancelHandler : public Handler<CancelHandler, void> {
		using Handler::Handler;
	};

private:
	std::string _title;
	std::string _content;

	ConfirmHandler _confirmHandler = nullptr;
	std::string _confirmText;
	DenyHandler _denyHandler = nullptr;
	std::string _denyText;
	CancelHandler _cancelHandler = nullptr;
	std::string _cancelText;

	Initializer _init;

public:
	MessagePopupBox(
		const std::string &title,
		const std::string &content,
		const ConfirmHandler &confirm, const DenyHandler &deny, const CancelHandler &cancel,
		const char* confirmTxt /* nullable */, const char* denyTxt /* nullable */, const char* cancelTxt /* nullable */
	);
	virtual ~MessagePopupBox() override;

	virtual void update(void) override;
};

class InputPopupBox : public PopupBox {
public:
	struct ConfirmHandler : public Handler<ConfirmHandler, void, const char*> {
		using Handler::Handler;
	};
	struct CancelHandler : public Handler<CancelHandler, void> {
		using Handler::Handler;
	};

private:
	std::string _title;
	std::string _content;
	std::string _default;
	unsigned _flags = 0;
	char _buffer[256]; // Fixed size.

	ConfirmHandler _confirmHandler = nullptr;
	std::string _confirmText;
	CancelHandler _cancelHandler = nullptr;
	std::string _cancelText;

	Initializer _init;

public:
	InputPopupBox(
		const std::string &title,
		const std::string &content, const std::string &default_, unsigned flags,
		const ConfirmHandler &confirm, const CancelHandler &cancel,
		const char* confirmTxt /* nullable */, const char* cancelTxt /* nullable */
	);
	virtual ~InputPopupBox() override;

	virtual void update(void) override;
};

class AddAssetPopupBox : public PopupBox {
public:
	struct ConfirmHandler : public Handler<ConfirmHandler, void, unsigned, const char*, const Math::Vec2i*, const Math::Vec2i*, const char*> {
		using Handler::Handler;
	};
	struct CancelHandler : public Handler<CancelHandler, void> {
		using Handler::Handler;
	};

	typedef std::vector<unsigned> Types;
	typedef std::vector<std::string> TypeNames;
	typedef std::vector<std::string> TypeExtensions;
	typedef std::vector<Math::Vec2i> Vec2s;

private:
	const class Project* _project = nullptr;
	std::string _title;
	std::string _type;
	Types _types;
	TypeNames _typeNames;
	TypeExtensions _typeExtensions;
	int _typeIndex = 0;
	Text::Array _refs;
	int _refIndex = -1;
	std::string _size;
	Math::Vec2i _sizeVec;
	Vec2s _defaultSizes;
	Vec2s _maxSizes;
	std::string _size2;
	Math::Vec2i _sizeVec2;
	Vec2s _defaultSizes2;
	Vec2s _maxSizes2;
	std::string _reference;
	std::string _palette;
	std::string _none;
	std::string _content;
	std::string _default;
	char _buffer[BITTY_MAX_PATH];
	std::string _tooltipRefPalette;
	std::string _tooltipRefImage;
	std::string _tooltipSize;
	std::string _tooltipPath;

	ConfirmHandler _confirmHandler = nullptr;
	std::string _confirmText;
	CancelHandler _cancelHandler = nullptr;
	std::string _cancelText;

	Initializer _init;
	std::string _language;

public:
	AddAssetPopupBox(
		const class Project* project,
		const std::string &title,
		const std::string &type, const Types &types, const TypeNames &typeNames, const TypeExtensions &typeExtensions, int typeIndex,
		const std::string &size, const Vec2s &defaultSizes, const Vec2s &maxSizes,
		const std::string &size2, const Vec2s &defaultSizes2, const Vec2s &maxSizes2,
		const std::string &content, const std::string &default_,
		const std::string &tooltipRefPalette, const std::string &tooltipRefImage, const std::string &tooltipSize, const std::string &tooltipPath,
		const std::string &none, const std::string &reference, const std::string &palette,
		const ConfirmHandler &confirm, const CancelHandler &cancel,
		const char* confirmTxt /* nullable */, const char* cancelTxt /* nullable */
	);
	virtual ~AddAssetPopupBox() override;

	virtual void update(void) override;
};

class AddFilePopupBox : public PopupBox {
public:
	struct ConfirmHandler : public Handler<ConfirmHandler, void, const char*, const char*> {
		using Handler::Handler;
	};
	struct CancelHandler : public Handler<CancelHandler, void> {
		using Handler::Handler;
	};

	typedef std::function<std::string(const std::string &)> Browser;

private:
	std::string _title;
	std::string _path;
	std::string _defaultPath;
	std::string _browse;
	Browser _browser;
	std::string _content;
	std::string _default;
	char _buffer[BITTY_MAX_PATH];
	std::string _tooltip;

	ConfirmHandler _confirmHandler = nullptr;
	std::string _confirmText;
	CancelHandler _cancelHandler = nullptr;
	std::string _cancelText;

	Initializer _init;

public:
	AddFilePopupBox(
		const std::string &title,
		const std::string &path, const std::string &defaultPath,
		const std::string &browse, Browser browser,
		const std::string &content, const std::string &default_, const std::string &tooltip,
		const ConfirmHandler &confirm, const CancelHandler &cancel,
		const char* confirmTxt /* nullable */, const char* cancelTxt /* nullable */
	);
	virtual ~AddFilePopupBox() override;

	virtual void update(void) override;
};

class ResizePopupBox : public PopupBox {
public:
	struct ConfirmHandler : public Handler<ConfirmHandler, void, const Math::Vec2i*> {
		using Handler::Handler;
	};
	struct CancelHandler : public Handler<CancelHandler, void> {
		using Handler::Handler;
	};

private:
	std::string _title;
	std::string _size;
	Math::Vec2i _sizeVec;
	Math::Vec2i _defaultSize;
	Math::Vec2i _maxSize;

	ConfirmHandler _confirmHandler = nullptr;
	std::string _confirmText;
	CancelHandler _cancelHandler = nullptr;
	std::string _cancelText;

	Initializer _init;

public:
	ResizePopupBox(
		const std::string &title,
		const std::string &size, const Math::Vec2i &defaultSize, const Math::Vec2i &maxSize,
		const ConfirmHandler &confirm, const CancelHandler &cancel,
		const char* confirmTxt /* nullable */, const char* cancelTxt /* nullable */
	);
	virtual ~ResizePopupBox() override;

	virtual void update(void) override;
};

class SelectAssetPopupBox : public PopupBox {
public:
	struct ConfirmHandlerForSingleSelection : public Handler<ConfirmHandlerForSingleSelection, void, const std::string &> {
		using Handler::Handler;
	};
	struct ConfirmHandlerForMultipleSelection : public Handler<ConfirmHandlerForMultipleSelection, void, const Text::Set &> {
		using Handler::Handler;
	};
	struct CancelHandler : public Handler<CancelHandler, void> {
		using Handler::Handler;
	};

private:
	const class Project* _project = nullptr;
	std::string _title;
	std::string _content;
	std::string _singleSelection;
	Text::Set _multipleSelection;
	std::string _extra;

	std::string _all;
	ImTextureID _texId = nullptr;
	ImTextureID _openTexId = nullptr;
	ImTextureID _fileTexId = nullptr;
	ImU32 _color = IM_COL32_WHITE;
	AssetFilter _filter = nullptr;

	ConfirmHandlerForSingleSelection _confirmHandlerForSingleSelection = nullptr;
	ConfirmHandlerForMultipleSelection _confirmHandlerForMultipleSelection = nullptr;
	std::string _confirmText;
	CancelHandler _cancelHandler = nullptr;
	std::string _cancelText;

	Initializer _init;

public:
	SelectAssetPopupBox(
		const class Project* project,
		const std::string &title,
		const std::string &content,
		const std::string &default_,
		const std::string &extra,
		ImTextureID texId, ImTextureID openTexId, ImTextureID fileTexId, ImU32 col,
		AssetFilter filter,
		const ConfirmHandlerForSingleSelection &confirm, const CancelHandler &cancel,
		const char* confirmTxt /* nullable */, const char* cancelTxt /* nullable */
	);
	SelectAssetPopupBox(
		const class Project* project,
		const std::string &title,
		const std::string &content,
		const Text::Set &default_,
		const std::string &extra,
		const std::string &all,
		ImTextureID texId, ImTextureID openTexId, ImU32 col,
		AssetFilter filter,
		const ConfirmHandlerForMultipleSelection &confirm, const CancelHandler &cancel,
		const char* confirmTxt /* nullable */, const char* cancelTxt /* nullable */
	);
	virtual ~SelectAssetPopupBox() override;

	virtual void update(void) override;
};

class SwitchAssetPopupBox : public PopupBox {
public:
	struct ConfirmHandler : public Handler<ConfirmHandler, void, const char*> {
		using Handler::Handler;
	};
	struct CancelHandler : public Handler<CancelHandler, void> {
		using Handler::Handler;
	};

private:
	std::string _title;
	Text::Array _assets;
	std::string _selection;

	ConfirmHandler _confirmHandler = nullptr;
	CancelHandler _cancelHandler = nullptr;

	Initializer _init;

public:
	SwitchAssetPopupBox(
		const class Project* project,
		const std::string &title,
		const ConfirmHandler &confirm, const CancelHandler &cancel
	);
	virtual ~SwitchAssetPopupBox() override;

	virtual void update(void) override;
};

ImVec2 GetMousePosOnCurrentItem(const ImVec2* ref_pos = nullptr);

void PushID(const std::string &str_id);

Rect LastItemRect(void);

void Dummy(const ImVec2 &size, ImU32 col);
void Dummy(const ImVec2 &size, const ImVec4 &col);

ImVec2 WindowResizingPadding(void);
bool Begin(const std::string &name, bool* p_open = nullptr, ImGuiWindowFlags flags = ImGuiWindowFlags_None);
void CentralizeWindow(void);
void EnsureWindowVisible(void);

void OpenPopup(const std::string &str_id, ImGuiPopupFlags popup_flags = ImGuiPopupFlags_None);
bool BeginPopupModal(const std::string &name, bool* p_open = nullptr, ImGuiWindowFlags flags = ImGuiWindowFlags_None);

float TitleBarHeight(void);
/**
 * @brief Adds custom buttons aside the close button, layouts from right to left.
 */
bool TitleBarCustomButton(const char* label, ImVec2* pos, ButtonDrawer draw, const char* tooltip = nullptr);

ImVec2 CustomButtonAutoPosition(void);

bool CustomButton(const char* label, ImVec2* pos, ButtonDrawer draw, const char* tooltip = nullptr);

void CustomAddButton(const ImVec2 &center, bool held, bool hovered, const char* tooltip);
void CustomRemoveButton(const ImVec2 &center, bool held, bool hovered, const char* tooltip);
void CustomRenameButton(const ImVec2 &center, bool held, bool hovered, const char* tooltip);
void CustomClearButton(const ImVec2 &center, bool held, bool hovered, const char* tooltip);
void CustomMinButton(const ImVec2 &center, bool held, bool hovered, const char* tooltip);
void CustomMaxButton(const ImVec2 &center, bool held, bool hovered, const char* tooltip);
void CustomCloseButton(const ImVec2 &center, bool held, bool hovered, const char* tooltip);
void CustomPlayButton(const ImVec2 &center, bool held, bool hovered, const char* tooltip);
void CustomStopButton(const ImVec2 &center, bool held, bool hovered, const char* tooltip);

void TextUnformatted(const std::string &text);

bool Url(const char* label, const char* link, bool adj = false);

void SetTooltip(const std::string &text);
void SetHelpTooltip(const std::string &text);

bool Checkbox(const std::string &label, bool* v);

void Indicator(const ImVec2 &min, const ImVec2 &max, float thickness = 3);
void Indicator(const char* label, const ImVec2 &pos);

bool ProgressBar(const char* label, float* v, float v_min, float v_max, const char* format = "%.3f", bool readonly = false);

bool Button(const std::string &label, const ImVec2 &size = ImVec2(0, 0));
void CentralizeButton(int count = 1, float width = WIDGETS_BUTTON_WIDTH);

bool ColorButton(const char* desc_id, const ImVec4 &col, ImGuiColorEditFlags flags, const ImVec2 &size, const char* tooltip /* nullable */);

bool ImageButton(ImTextureID user_texture_id, const ImVec2 &size, const ImVec4 &tint_col, bool selected = false, const char* tooltip = nullptr);

void NineGridsImage(ImTextureID texture_id, const ImVec2 &src_size, const ImVec2 &dst_size, bool horizontal = true, bool normal = true);

bool BeginMenu(const std::string &label, bool enabled = true);
bool MenuItem(const std::string &label, const char* shortcut = nullptr, bool selected = false, bool enabled = true);
bool MenuItem(const std::string &label, const char* shortcut, bool* selected, bool enabled = true);

float ColorPickerMinWidthForInput(void);

float TabBarHeight(void);
bool BeginTabItem(const std::string &str_id, const std::string &label, bool* p_open = nullptr, ImGuiTabItemFlags flags = ImGuiTabItemFlags_None);
bool BeginTabItem(const std::string &label, bool* p_open = nullptr, ImGuiTabItemFlags flags = ImGuiTabItemFlags_None);
bool BeginTabItem(const std::string &label, bool* p_open, ImGuiTabItemFlags flags, ImU32 col);
void TabBarTabListPopupButton(TabBarDropper dropper);

bool BeginTable(const std::string &str_id, int column, ImGuiTableFlags flags = ImGuiTableFlags_None, const ImVec2 &outer_size = ImVec2(0.0f, 0.0f), float inner_width = 0.0f);
void TableSetupColumn(const std::string &label, ImGuiTableColumnFlags flags = ImGuiTableFlags_None, float init_width_or_weight = 0.0f, ImU32 user_id = 0);

/**
 * @brief Uses specific textures instead of the default arrow or bullet for
 *   node head.
 */
bool TreeNode(ImTextureID texture_id, ImTextureID open_tex_id, const std::string &label, ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None, ImGuiButtonFlags button_flags = ImGuiButtonFlags_None, ImU32 col = IM_COL32_WHITE);
/**
 * @brief Uses checkbox instead of the default arrow or bullet for node head.
 */
bool TreeNode(bool* checked, const std::string &label, ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None, ImGuiButtonFlags button_flags = ImGuiButtonFlags_None);

bool Selectable(const std::string &label, bool selected = false, ImGuiSelectableFlags flags = ImGuiSelectableFlags_None, const ImVec2 &size = ImVec2(0, 0));
bool Selectable(const std::string &label, bool* p_selected, ImGuiSelectableFlags flags = ImGuiSelectableFlags_None, const ImVec2 &size = ImVec2(0, 0));

void RefSelector(
	const class Project* project,
	Text::Array &refs, int* ref_index,
	unsigned type,
	const char* none /* nullable */, const char* palette /* nullable */, const char* reference /* nullable */
);

void AssetSelectAll(
	const class Project* project,
	Text::Set &selected,
	AssetFilter filter = nullptr
);
bool AssetSelector(
	const class Project* project,
	Text::Set &selected,
	ImTextureID dir_tex_id = nullptr, ImTextureID open_dir_tex_id = nullptr,
	ImU32 col = IM_COL32_WHITE,
	AssetFilter filter = nullptr,
	int* total = nullptr
);
bool AssetSelector(
	const class Project* project,
	std::string &selected,
	ImTextureID dir_tex_id = nullptr, ImTextureID open_dir_tex_id = nullptr, ImTextureID file_tex_id = nullptr,
	ImU32 col = IM_COL32_WHITE,
	AssetFilter filter = nullptr,
	int* total = nullptr
);
bool AssetMenu(
	const class Project* project,
	std::string &selected,
	AssetFilter filter = nullptr
);

bool ExampleMenu(
	const class Project* project,
	const Entry::Dictionary &examples,
	std::string &selected
);

bool PluginMenu(
	const class Project* project,
	const Plugin::Array &plugins,
	const char* menu,
	Plugin* &selected
);

bool DocumentMenu(
	const class Project* project,
	const Entry::Dictionary &documents,
	std::string &selected
);

void DebugVariable(const Variant &val);

void ConfigGamepads(
	Input* input,
	Input::Gamepad* pads, int pad_count,
	int* active_pad_index /* nullable */, int* active_btn_index /* nullable */,
	const char* label_wait /* nullable */
);
void ConfigOnscreenGamepad(
	bool* enabled,
	bool* swap_ab,
	float* scale, float* padding_x, float* padding_y,
	const char* label_enabled /* nullable */,
	const char* label_swap_ab /* nullable */,
	const char* label_scale /* nullable */, const char* label_padding_x /* nullable */, const char* label_padding_y /* nullable */
);

}

/* ===========================================================================} */

#endif /* __WIDGETS_H__ */
