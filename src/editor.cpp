/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2023 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "editor.h"
#include "mathematics.h"
#include "../lib/imgui/imgui.h"

/*
** {===========================================================================
** Editor
*/

float Editor::Ref::windowWidth(float exp) {
	const float w = ImGui::GetContentRegionAvail().x;
	_verticalScrollBarVisible = w < std::floor(exp);

	return w;
}

int Editor::Ref::windowFlags(void) const {
	return (_verticalScrollBarVisible ? ImGuiWindowFlags_AlwaysVerticalScrollbar : ImGuiWindowFlags_None) | ImGuiWindowFlags_NoNav;
}

void Editor::Ref::windowResized(void) {
	_verticalScrollBarVisible = false;
}

Editor::Splitter Editor::split(void) {
	ImGuiIO &io = ImGui::GetIO();

	const ImVec2 content = ImGui::GetContentRegionAvail();
	const float toolsWidth = Math::clamp(content.x * 0.25f, 175.0f * io.FontGlobalScale, 256.0f * io.FontGlobalScale);
	const float paintingWidth = content.x - toolsWidth;

	return std::make_pair(paintingWidth, toolsWidth);
}

/* ===========================================================================} */
