/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2025 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "theme_studio.h"
#include "../lib/imgui/imgui_internal.h"

/*
** {===========================================================================
** Studio theme
*/

ThemeStudio::ThemeStudio() {
}

ThemeStudio::~ThemeStudio() {
}

Theme::Styles ThemeStudio::styleIndex(void) const {
	return _styleType;
}

void ThemeStudio::styleIndex(Styles idx) {
	_styleType = idx;

	switch (_styleType) {
	case DARK:
		style(&styleDark());

		break;
	case CLASSIC:
		style(&styleClassic());

		break;
	case LIGHT:
		style(&styleLight());

		break;
	}

	memcpy(ImGui::GetStyle().Colors, style()->builtin, sizeof(ImGui::GetStyle().Colors));
}

bool ThemeStudio::open(class Renderer* rnd) {
	if (!Theme::open(rnd))
		return false;

	styleDark().builtin[ImGuiCol_Text]                   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	styleDark().builtin[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	styleDark().builtin[ImGuiCol_WindowBg]               = ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
	styleDark().builtin[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	styleDark().builtin[ImGuiCol_PopupBg]                = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
	styleDark().builtin[ImGuiCol_Border]                 = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
	styleDark().builtin[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	styleDark().builtin[ImGuiCol_FrameBg]                = ImVec4(0.16f, 0.19f, 0.18f, 0.54f);
	styleDark().builtin[ImGuiCol_FrameBgHovered]         = ImVec4(0.16f, 0.29f, 0.38f, 0.40f);
	styleDark().builtin[ImGuiCol_FrameBgActive]          = ImVec4(0.16f, 0.29f, 0.38f, 0.67f);
	styleDark().builtin[ImGuiCol_TitleBg]                = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
	styleDark().builtin[ImGuiCol_TitleBgActive]          = ImVec4(0.16f, 0.29f, 0.48f, 1.00f);
	styleDark().builtin[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
	styleDark().builtin[ImGuiCol_MenuBarBg]              = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
	styleDark().builtin[ImGuiCol_ScrollbarBg]            = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
	styleDark().builtin[ImGuiCol_ScrollbarGrab]          = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
	styleDark().builtin[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
	styleDark().builtin[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
	styleDark().builtin[ImGuiCol_CheckMark]              = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	styleDark().builtin[ImGuiCol_SliderGrab]             = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
	styleDark().builtin[ImGuiCol_SliderGrabActive]       = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	styleDark().builtin[ImGuiCol_Button]                 = ImVec4(0.36f, 0.36f, 0.36f, 0.40f);
	styleDark().builtin[ImGuiCol_ButtonHovered]          = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
	styleDark().builtin[ImGuiCol_ButtonActive]           = ImVec4(0.46f, 0.46f, 0.46f, 1.00f);
	styleDark().builtin[ImGuiCol_Header]                 = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
	styleDark().builtin[ImGuiCol_HeaderHovered]          = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
	styleDark().builtin[ImGuiCol_HeaderActive]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	styleDark().builtin[ImGuiCol_Separator]              = styleDark().builtin[ImGuiCol_Border];
	styleDark().builtin[ImGuiCol_SeparatorHovered]       = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
	styleDark().builtin[ImGuiCol_SeparatorActive]        = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
	styleDark().builtin[ImGuiCol_ResizeGrip]             = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
	styleDark().builtin[ImGuiCol_ResizeGripHovered]      = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
	styleDark().builtin[ImGuiCol_ResizeGripActive]       = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
	styleDark().builtin[ImGuiCol_Tab]                    = ImLerp(styleDark().builtin[ImGuiCol_Header],       styleDark().builtin[ImGuiCol_TitleBgActive], 0.80f);
	styleDark().builtin[ImGuiCol_TabHovered]             = styleDark().builtin[ImGuiCol_HeaderHovered];
	styleDark().builtin[ImGuiCol_TabActive]              = ImLerp(styleDark().builtin[ImGuiCol_HeaderActive], styleDark().builtin[ImGuiCol_TitleBgActive], 0.60f);
	styleDark().builtin[ImGuiCol_TabUnfocused]           = ImLerp(styleDark().builtin[ImGuiCol_Tab],          styleDark().builtin[ImGuiCol_TitleBg], 0.80f);
	styleDark().builtin[ImGuiCol_TabUnfocusedActive]     = ImLerp(styleDark().builtin[ImGuiCol_TabActive],    styleDark().builtin[ImGuiCol_TitleBg], 0.40f);
	styleDark().builtin[ImGuiCol_PlotLines]              = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
	styleDark().builtin[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	styleDark().builtin[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	styleDark().builtin[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	styleDark().builtin[ImGuiCol_TableHeaderBg]          = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
	styleDark().builtin[ImGuiCol_TableBorderStrong]      = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
	styleDark().builtin[ImGuiCol_TableBorderLight]       = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
	styleDark().builtin[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	styleDark().builtin[ImGuiCol_TableRowBgAlt]          = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
	styleDark().builtin[ImGuiCol_TextSelectedBg]         = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
	styleDark().builtin[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
	styleDark().builtin[ImGuiCol_NavHighlight]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	styleDark().builtin[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
	styleDark().builtin[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
	styleDark().builtin[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
	styleDark().tabTextColor                             = ImGui::GetColorU32(ImVec4(1.00f, 1.00f, 1.00f, 1.00f));
	styleDark().tabTextPendingColor                      = ImGui::GetColorU32(ImVec4(1.00f, 1.00f, 1.00f, 1.00f));
	styleDark().tabPendingColor                          = ImGui::GetColorU32(ImVec4(0.40f, 0.13f, 0.47f, 1.00f));
	styleDark().tabPendingHoveredColor                   = ImGui::GetColorU32(ImVec4(0.50f, 0.23f, 0.57f, 1.00f));
	styleDark().iconColor                                = ImGui::GetColorU32(ImVec4(1.00f, 1.00f, 1.00f, 1.00f));
	styleDark().iconDisabledColor                        = ImGui::GetColorU32(ImVec4(0.50f, 0.50f, 0.50f, 1.00f));
	styleDark().messageColor                             = ImGui::GetColorU32(ImVec4(1.00f, 1.00f, 1.00f, 1.00f));
	styleDark().warningColor                             = ImGui::GetColorU32(ImVec4(0.95f, 0.93f, 0.10f, 1.00f));
	styleDark().errorColor                               = ImGui::GetColorU32(ImVec4(0.93f, 0.00f, 0.00f, 1.00f));

	styleClassic().builtin[ImGuiCol_Text]                   = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
	styleClassic().builtin[ImGuiCol_TextDisabled]           = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
	styleClassic().builtin[ImGuiCol_WindowBg]               = ImVec4(0.01f, 0.08f, 0.20f, 0.80f);
	styleClassic().builtin[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	styleClassic().builtin[ImGuiCol_PopupBg]                = ImVec4(0.11f, 0.11f, 0.14f, 0.92f);
	styleClassic().builtin[ImGuiCol_Border]                 = ImVec4(0.50f, 0.50f, 0.50f, 0.50f);
	styleClassic().builtin[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	styleClassic().builtin[ImGuiCol_FrameBg]                = ImVec4(0.43f, 0.43f, 0.43f, 0.39f);
	styleClassic().builtin[ImGuiCol_FrameBgHovered]         = ImVec4(0.47f, 0.47f, 0.69f, 0.40f);
	styleClassic().builtin[ImGuiCol_FrameBgActive]          = ImVec4(0.42f, 0.41f, 0.64f, 0.69f);
	styleClassic().builtin[ImGuiCol_TitleBg]                = ImVec4(0.06f, 0.12f, 0.23f, 1.00f);
	styleClassic().builtin[ImGuiCol_TitleBgActive]          = ImVec4(0.16f, 0.22f, 0.33f, 1.00f);
	styleClassic().builtin[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.16f, 0.22f, 0.33f, 0.20f);
	styleClassic().builtin[ImGuiCol_MenuBarBg]              = ImVec4(0.01f, 0.08f, 0.20f, 1.00f);
	styleClassic().builtin[ImGuiCol_ScrollbarBg]            = ImVec4(0.10f, 0.15f, 0.30f, 0.60f);
	styleClassic().builtin[ImGuiCol_ScrollbarGrab]          = ImVec4(0.26f, 0.32f, 0.43f, 0.50f);
	styleClassic().builtin[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.26f, 0.32f, 0.43f, 0.70f);
	styleClassic().builtin[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.26f, 0.32f, 0.43f, 0.90f);
	styleClassic().builtin[ImGuiCol_CheckMark]              = ImVec4(0.90f, 0.90f, 0.90f, 0.50f);
	styleClassic().builtin[ImGuiCol_SliderGrab]             = ImVec4(0.36f, 0.42f, 0.48f, 0.80f);
	styleClassic().builtin[ImGuiCol_SliderGrabActive]       = ImVec4(0.36f, 0.42f, 0.48f, 0.90f);
	styleClassic().builtin[ImGuiCol_Button]                 = ImVec4(0.35f, 0.40f, 0.51f, 0.62f);
	styleClassic().builtin[ImGuiCol_ButtonHovered]          = ImVec4(0.40f, 0.48f, 0.61f, 0.79f);
	styleClassic().builtin[ImGuiCol_ButtonActive]           = ImVec4(0.46f, 0.54f, 0.70f, 1.00f);
	styleClassic().builtin[ImGuiCol_Header]                 = ImVec4(0.36f, 0.42f, 0.48f, 0.45f);
	styleClassic().builtin[ImGuiCol_HeaderHovered]          = ImVec4(0.46f, 0.62f, 0.68f, 0.90f);
	styleClassic().builtin[ImGuiCol_HeaderActive]           = ImVec4(0.46f, 0.52f, 0.58f, 0.90f);
	styleClassic().builtin[ImGuiCol_Separator]              = ImVec4(0.50f, 0.50f, 0.50f, 0.60f);
	styleClassic().builtin[ImGuiCol_SeparatorHovered]       = ImVec4(0.60f, 0.60f, 0.70f, 1.00f);
	styleClassic().builtin[ImGuiCol_SeparatorActive]        = ImVec4(0.70f, 0.70f, 0.90f, 1.00f);
	styleClassic().builtin[ImGuiCol_ResizeGrip]             = ImVec4(1.00f, 1.00f, 1.00f, 0.16f);
	styleClassic().builtin[ImGuiCol_ResizeGripHovered]      = ImVec4(0.78f, 0.82f, 1.00f, 0.60f);
	styleClassic().builtin[ImGuiCol_ResizeGripActive]       = ImVec4(0.78f, 0.82f, 1.00f, 0.90f);
	styleClassic().builtin[ImGuiCol_Tab]                    = ImVec4(0.21f, 0.30f, 0.43f, 1.00f);
	styleClassic().builtin[ImGuiCol_TabHovered]             = ImVec4(1.00f, 0.98f, 0.71f, 1.00f);
	styleClassic().builtin[ImGuiCol_TabActive]              = ImVec4(1.00f, 0.95f, 0.61f, 1.00f);
	styleClassic().builtin[ImGuiCol_TabUnfocused]           = ImLerp(styleClassic().builtin[ImGuiCol_Tab],          styleClassic().builtin[ImGuiCol_TitleBg], 0.80f);
	styleClassic().builtin[ImGuiCol_TabUnfocusedActive]     = ImLerp(styleClassic().builtin[ImGuiCol_TabActive],    styleClassic().builtin[ImGuiCol_TitleBg], 0.40f);
	styleClassic().builtin[ImGuiCol_PlotLines]              = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	styleClassic().builtin[ImGuiCol_PlotLinesHovered]       = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	styleClassic().builtin[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	styleClassic().builtin[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	styleClassic().builtin[ImGuiCol_TableHeaderBg]          = ImVec4(0.27f, 0.27f, 0.38f, 1.00f);
	styleClassic().builtin[ImGuiCol_TableBorderStrong]      = ImVec4(0.31f, 0.31f, 0.45f, 1.00f);
	styleClassic().builtin[ImGuiCol_TableBorderLight]       = ImVec4(0.26f, 0.26f, 0.28f, 1.00f);
	styleClassic().builtin[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	styleClassic().builtin[ImGuiCol_TableRowBgAlt]          = ImVec4(1.00f, 1.00f, 1.00f, 0.07f);
	styleClassic().builtin[ImGuiCol_TextSelectedBg]         = ImVec4(0.00f, 0.00f, 1.00f, 0.35f);
	styleClassic().builtin[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
	styleClassic().builtin[ImGuiCol_NavHighlight]           = styleClassic().builtin[ImGuiCol_HeaderHovered];
	styleClassic().builtin[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
	styleClassic().builtin[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
	styleClassic().builtin[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
	styleClassic().tabTextColor                             = ImGui::GetColorU32(ImVec4(0.60f, 0.60f, 0.60f, 1.00f));
	styleClassic().tabTextPendingColor                      = ImGui::GetColorU32(ImVec4(0.60f, 0.60f, 0.60f, 1.00f));
	styleClassic().tabPendingColor                          = ImGui::GetColorU32(ImVec4(0.26f, 0.59f, 0.98f, 1.00f));
	styleClassic().tabPendingHoveredColor                   = ImGui::GetColorU32(ImVec4(0.36f, 0.69f, 0.98f, 1.00f));
	styleClassic().iconColor                                = ImGui::GetColorU32(ImVec4(1.00f, 1.00f, 1.00f, 1.00f));
	styleClassic().iconDisabledColor                        = ImGui::GetColorU32(ImVec4(0.50f, 0.50f, 0.50f, 1.00f));
	styleClassic().messageColor                             = ImGui::GetColorU32(ImVec4(1.00f, 1.00f, 1.00f, 1.00f));
	styleClassic().warningColor                             = ImGui::GetColorU32(ImVec4(0.95f, 0.93f, 0.10f, 1.00f));
	styleClassic().errorColor                               = ImGui::GetColorU32(ImVec4(0.93f, 0.00f, 0.00f, 1.00f));

	styleLight().builtin[ImGuiCol_Text]                   = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
	styleLight().builtin[ImGuiCol_TextDisabled]           = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
	styleLight().builtin[ImGuiCol_WindowBg]               = ImVec4(0.94f, 0.94f, 0.94f, 1.00f);
	styleLight().builtin[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	styleLight().builtin[ImGuiCol_PopupBg]                = ImVec4(1.00f, 1.00f, 1.00f, 0.98f);
	styleLight().builtin[ImGuiCol_Border]                 = ImVec4(0.00f, 0.00f, 0.00f, 0.30f);
	styleLight().builtin[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	styleLight().builtin[ImGuiCol_FrameBg]                = ImVec4(0.90f, 0.90f, 0.90f, 1.00f);
	styleLight().builtin[ImGuiCol_FrameBgHovered]         = ImVec4(0.26f, 0.59f, 0.98f, 0.40f);
	styleLight().builtin[ImGuiCol_FrameBgActive]          = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
	styleLight().builtin[ImGuiCol_TitleBg]                = ImVec4(0.96f, 0.96f, 0.96f, 1.00f);
	styleLight().builtin[ImGuiCol_TitleBgActive]          = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
	styleLight().builtin[ImGuiCol_TitleBgCollapsed]       = ImVec4(1.00f, 1.00f, 1.00f, 0.51f);
	styleLight().builtin[ImGuiCol_MenuBarBg]              = ImVec4(0.86f, 0.86f, 0.86f, 1.00f);
	styleLight().builtin[ImGuiCol_ScrollbarBg]            = ImVec4(0.98f, 0.98f, 0.98f, 0.53f);
	styleLight().builtin[ImGuiCol_ScrollbarGrab]          = ImVec4(0.69f, 0.69f, 0.69f, 0.80f);
	styleLight().builtin[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.49f, 0.49f, 0.49f, 0.80f);
	styleLight().builtin[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.49f, 0.49f, 0.49f, 1.00f);
	styleLight().builtin[ImGuiCol_CheckMark]              = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	styleLight().builtin[ImGuiCol_SliderGrab]             = ImVec4(0.26f, 0.59f, 0.98f, 0.78f);
	styleLight().builtin[ImGuiCol_SliderGrabActive]       = ImVec4(0.46f, 0.54f, 0.80f, 0.60f);
	styleLight().builtin[ImGuiCol_Button]                 = ImVec4(0.82f, 0.82f, 0.82f, 1.00f);
	styleLight().builtin[ImGuiCol_ButtonHovered]          = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	styleLight().builtin[ImGuiCol_ButtonActive]           = ImVec4(0.06f, 0.53f, 0.98f, 1.00f);
	styleLight().builtin[ImGuiCol_Header]                 = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
	styleLight().builtin[ImGuiCol_HeaderHovered]          = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
	styleLight().builtin[ImGuiCol_HeaderActive]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	styleLight().builtin[ImGuiCol_Separator]              = ImVec4(0.39f, 0.39f, 0.39f, 0.62f);
	styleLight().builtin[ImGuiCol_SeparatorHovered]       = ImVec4(0.14f, 0.44f, 0.80f, 0.78f);
	styleLight().builtin[ImGuiCol_SeparatorActive]        = ImVec4(0.14f, 0.44f, 0.80f, 1.00f);
	styleLight().builtin[ImGuiCol_ResizeGrip]             = ImVec4(0.80f, 0.80f, 0.80f, 0.56f);
	styleLight().builtin[ImGuiCol_ResizeGripHovered]      = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
	styleLight().builtin[ImGuiCol_ResizeGripActive]       = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
	styleLight().builtin[ImGuiCol_Tab]                    = ImLerp(styleLight().builtin[ImGuiCol_Header],       styleLight().builtin[ImGuiCol_TitleBgActive], 0.90f);
	styleLight().builtin[ImGuiCol_TabHovered]             = ImVec4(0.00f, 0.48f, 0.80f, 1.00f);
	styleLight().builtin[ImGuiCol_TabActive]              = ImVec4(0.00f, 0.42f, 0.74f, 1.00f);
	styleLight().builtin[ImGuiCol_TabUnfocused]           = ImLerp(styleLight().builtin[ImGuiCol_Tab],          styleLight().builtin[ImGuiCol_TitleBg], 0.80f);
	styleLight().builtin[ImGuiCol_TabUnfocusedActive]     = ImLerp(styleLight().builtin[ImGuiCol_TabActive],    styleLight().builtin[ImGuiCol_TitleBg], 0.40f);
	styleLight().builtin[ImGuiCol_PlotLines]              = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
	styleLight().builtin[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	styleLight().builtin[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	styleLight().builtin[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.45f, 0.00f, 1.00f);
	styleLight().builtin[ImGuiCol_TableHeaderBg]          = ImVec4(0.78f, 0.87f, 0.98f, 1.00f);
	styleLight().builtin[ImGuiCol_TableBorderStrong]      = ImVec4(0.57f, 0.57f, 0.64f, 1.00f);
	styleLight().builtin[ImGuiCol_TableBorderLight]       = ImVec4(0.68f, 0.68f, 0.74f, 1.00f);
	styleLight().builtin[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	styleLight().builtin[ImGuiCol_TableRowBgAlt]          = ImVec4(0.30f, 0.30f, 0.30f, 0.09f);
	styleLight().builtin[ImGuiCol_TextSelectedBg]         = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
	styleLight().builtin[ImGuiCol_DragDropTarget]         = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
	styleLight().builtin[ImGuiCol_NavHighlight]           = styleLight().builtin[ImGuiCol_HeaderHovered];
	styleLight().builtin[ImGuiCol_NavWindowingHighlight]  = ImVec4(0.70f, 0.70f, 0.70f, 0.70f);
	styleLight().builtin[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.20f, 0.20f, 0.20f, 0.20f);
	styleLight().builtin[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.20f, 0.20f, 0.20f, 0.35f);
	styleLight().tabTextColor                             = ImGui::GetColorU32(ImVec4(1.00f, 1.00f, 1.00f, 1.00f));
	styleLight().tabTextPendingColor                      = ImGui::GetColorU32(ImVec4(1.00f, 1.00f, 1.00f, 1.00f));
	styleLight().tabPendingColor                          = ImGui::GetColorU32(ImVec4(0.40f, 0.13f, 0.47f, 1.00f));
	styleLight().tabPendingHoveredColor                   = ImGui::GetColorU32(ImVec4(0.50f, 0.23f, 0.57f, 1.00f));
	styleLight().iconColor                                = ImGui::GetColorU32(ImVec4(0.00f, 0.00f, 0.00f, 1.00f));
	styleLight().iconDisabledColor                        = ImGui::GetColorU32(ImVec4(0.50f, 0.50f, 0.50f, 1.00f));
	styleLight().messageColor                             = ImGui::GetColorU32(ImVec4(0.00f, 0.00f, 0.00f, 1.00f));
	styleLight().warningColor                             = ImGui::GetColorU32(ImVec4(1.00f, 0.69f, 0.06f, 1.00f));
	styleLight().errorColor                               = ImGui::GetColorU32(ImVec4(0.93f, 0.00f, 0.00f, 1.00f));

	styleIndex(DARK);

	windowPreferences_Theme("Theme:");
	windowPreferences_Style("Style         ");

	fprintf(stdout, "Theme opened.\n");

	return true;
}

bool ThemeStudio::close(class Renderer* rnd) {
	if (!Theme::close(rnd))
		return false;

	fprintf(stdout, "Theme closed.\n");

	return true;
}

bool ThemeStudio::load(class Renderer* rnd) {
	bool result = true;

	if (!Theme::load(rnd))
		result = false;

	return result;
}

bool ThemeStudio::save(void) const {
	bool result = true;

	if (!Theme::save())
		result = false;

	return result;
}

void ThemeStudio::setColor(const std::string &key, ImGuiCol idx, const ImColor &col) {
	if (idx < 0 || idx >= ImGuiCol_COUNT)
		return;

	const bool dark = key == "dark" || key == "all" || key == "default";
	const bool classic = key == "classic" || key == "all";
	const bool light = key == "light" || key == "all";

	if (dark) {
		styleDark().builtin[idx] = (ImVec4)col;
	}
	if (classic) {
		styleClassic().builtin[idx] = (ImVec4)col;
	}
	if (light) {
		styleLight().builtin[idx] = (ImVec4)col;
	}
}

void ThemeStudio::setColor(const std::string &key, const std::string &idx, const ImColor &col) {
	const bool dark = key == "dark" || key == "all" || key == "default";
	const bool classic = key == "classic" || key == "all";
	const bool light = key == "light" || key == "all";

	if (dark) {
		if (idx == "tab_text")
			styleDark().tabTextColor = ImGui::GetColorU32((ImVec4)col);
		else if (idx == "tab_text_pending")
			styleDark().tabTextPendingColor = ImGui::GetColorU32((ImVec4)col);
		else if (idx == "tab_pending")
			styleDark().tabPendingColor = ImGui::GetColorU32((ImVec4)col);
		else if (idx == "tab_pending_hovered")
			styleDark().tabPendingHoveredColor = ImGui::GetColorU32((ImVec4)col);
		else if (idx == "icon")
			styleDark().iconColor = ImGui::GetColorU32((ImVec4)col);
		else if (idx == "icon_disabled")
			styleDark().iconDisabledColor = ImGui::GetColorU32((ImVec4)col);
		else if (idx == "message")
			styleDark().messageColor = ImGui::GetColorU32((ImVec4)col);
		else if (idx == "warning")
			styleDark().warningColor = ImGui::GetColorU32((ImVec4)col);
		else if (idx == "error")
			styleDark().errorColor = ImGui::GetColorU32((ImVec4)col);
	}
	if (classic) {
		if (idx == "tab_text")
			styleClassic().tabTextColor = ImGui::GetColorU32((ImVec4)col);
		else if (idx == "tab_text_pending")
			styleClassic().tabTextPendingColor = ImGui::GetColorU32((ImVec4)col);
		else if (idx == "tab_pending")
			styleClassic().tabPendingColor = ImGui::GetColorU32((ImVec4)col);
		else if (idx == "tab_pending_hovered")
			styleClassic().tabPendingHoveredColor = ImGui::GetColorU32((ImVec4)col);
		else if (idx == "icon")
			styleClassic().iconColor = ImGui::GetColorU32((ImVec4)col);
		else if (idx == "icon_disabled")
			styleClassic().iconDisabledColor = ImGui::GetColorU32((ImVec4)col);
		else if (idx == "message")
			styleClassic().messageColor = ImGui::GetColorU32((ImVec4)col);
		else if (idx == "warning")
			styleClassic().warningColor = ImGui::GetColorU32((ImVec4)col);
		else if (idx == "error")
			styleClassic().errorColor = ImGui::GetColorU32((ImVec4)col);
	}
	if (light) {
		if (idx == "tab_text")
			styleLight().tabTextColor = ImGui::GetColorU32((ImVec4)col);
		else if (idx == "tab_text_pending")
			styleLight().tabTextPendingColor = ImGui::GetColorU32((ImVec4)col);
		else if (idx == "tab_pending")
			styleLight().tabPendingColor = ImGui::GetColorU32((ImVec4)col);
		else if (idx == "tab_pending_hovered")
			styleLight().tabPendingHoveredColor = ImGui::GetColorU32((ImVec4)col);
		else if (idx == "icon")
			styleLight().iconColor = ImGui::GetColorU32((ImVec4)col);
		else if (idx == "icon_disabled")
			styleLight().iconDisabledColor = ImGui::GetColorU32((ImVec4)col);
		else if (idx == "message")
			styleLight().messageColor = ImGui::GetColorU32((ImVec4)col);
		else if (idx == "warning")
			styleLight().warningColor = ImGui::GetColorU32((ImVec4)col);
		else if (idx == "error")
			styleLight().errorColor = ImGui::GetColorU32((ImVec4)col);
	}
}

/* ===========================================================================} */
