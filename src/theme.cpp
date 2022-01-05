/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2022 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#include "bytes.h"
#include "encoding.h"
#include "file_handle.h"
#include "filesystem.h"
#include "hacks.h"
#include "image.h"
#include "texture.h"
#include "theme.h"
#include "resource/inline_resource.h"
#include "../lib/imgui/imgui_internal.h"
#include "../lib/jpath/jpath.hpp"

/*
** {===========================================================================
** Macros and constants
*/

#ifndef THEME_FONT_RANGES_DEFAULT_NAME
#	define THEME_FONT_RANGES_DEFAULT_NAME "default"
#endif /* THEME_FONT_RANGES_DEFAULT_NAME */
#ifndef THEME_FONT_RANGES_CHINESE_NAME
#	define THEME_FONT_RANGES_CHINESE_NAME "chinese"
#endif /* THEME_FONT_RANGES_CHINESE_NAME */
#ifndef THEME_FONT_RANGES_JAPANESE_NAME
#	define THEME_FONT_RANGES_JAPANESE_NAME "japanese"
#endif /* THEME_FONT_RANGES_JAPANESE_NAME */
#ifndef THEME_FONT_RANGES_KOREAN_NAME
#	define THEME_FONT_RANGES_KOREAN_NAME "korean"
#endif /* THEME_FONT_RANGES_KOREAN_NAME */
#ifndef THEME_FONT_RANGES_CYRILLIC_NAME
#	define THEME_FONT_RANGES_CYRILLIC_NAME "cyrillic"
#endif /* THEME_FONT_RANGES_CYRILLIC_NAME */
#ifndef THEME_FONT_RANGES_THAI_NAME
#	define THEME_FONT_RANGES_THAI_NAME "thai"
#endif /* THEME_FONT_RANGES_THAI_NAME */
#ifndef THEME_FONT_RANGES_VIETNAMESE_NAME
#	define THEME_FONT_RANGES_VIETNAMESE_NAME "vietnamese"
#endif /* THEME_FONT_RANGES_VIETNAMESE_NAME */
#ifndef THEME_FONT_RANGES_POLISH_NAME
#	define THEME_FONT_RANGES_POLISH_NAME "polish"
#endif /* THEME_FONT_RANGES_POLISH_NAME */

/* ===========================================================================} */

/*
** {===========================================================================
** Theme
*/

Theme::Theme() {
}

Theme::~Theme() {
}

bool Theme::open(class Renderer* rnd) {
	if (_opened)
		return false;
	_opened = true;

	ImGuiStyle &style_ = ImGui::GetStyle();
	style_.DisabledAlpha = 0.45f;

	style(&styleDefault());

	static_assert(ImGuiCol_COUNT == 53, "The size of `ImGuiCol_COUNT` has been changed, consider modify corresponding array.");
	styleDefault().builtin[ImGuiCol_Text]                   = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
	styleDefault().builtin[ImGuiCol_TextDisabled]           = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	styleDefault().builtin[ImGuiCol_WindowBg]               = ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
	styleDefault().builtin[ImGuiCol_ChildBg]                = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	styleDefault().builtin[ImGuiCol_PopupBg]                = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
	styleDefault().builtin[ImGuiCol_Border]                 = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
	styleDefault().builtin[ImGuiCol_BorderShadow]           = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	styleDefault().builtin[ImGuiCol_FrameBg]                = ImVec4(0.16f, 0.19f, 0.18f, 0.54f);
	styleDefault().builtin[ImGuiCol_FrameBgHovered]         = ImVec4(0.16f, 0.29f, 0.38f, 0.40f);
	styleDefault().builtin[ImGuiCol_FrameBgActive]          = ImVec4(0.16f, 0.29f, 0.38f, 0.67f);
	styleDefault().builtin[ImGuiCol_TitleBg]                = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
	styleDefault().builtin[ImGuiCol_TitleBgActive]          = ImVec4(0.16f, 0.29f, 0.48f, 1.00f);
	styleDefault().builtin[ImGuiCol_TitleBgCollapsed]       = ImVec4(0.00f, 0.00f, 0.00f, 0.51f);
	styleDefault().builtin[ImGuiCol_MenuBarBg]              = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
	styleDefault().builtin[ImGuiCol_ScrollbarBg]            = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
	styleDefault().builtin[ImGuiCol_ScrollbarGrab]          = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
	styleDefault().builtin[ImGuiCol_ScrollbarGrabHovered]   = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
	styleDefault().builtin[ImGuiCol_ScrollbarGrabActive]    = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
	styleDefault().builtin[ImGuiCol_CheckMark]              = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	styleDefault().builtin[ImGuiCol_SliderGrab]             = ImVec4(0.24f, 0.52f, 0.88f, 1.00f);
	styleDefault().builtin[ImGuiCol_SliderGrabActive]       = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	styleDefault().builtin[ImGuiCol_Button]                 = ImVec4(0.36f, 0.36f, 0.36f, 0.40f);
	styleDefault().builtin[ImGuiCol_ButtonHovered]          = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
	styleDefault().builtin[ImGuiCol_ButtonActive]           = ImVec4(0.46f, 0.46f, 0.46f, 1.00f);
	styleDefault().builtin[ImGuiCol_Header]                 = ImVec4(0.26f, 0.59f, 0.98f, 0.31f);
	styleDefault().builtin[ImGuiCol_HeaderHovered]          = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);
	styleDefault().builtin[ImGuiCol_HeaderActive]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	styleDefault().builtin[ImGuiCol_Separator]              = styleDefault().builtin[ImGuiCol_Border];
	styleDefault().builtin[ImGuiCol_SeparatorHovered]       = ImVec4(0.10f, 0.40f, 0.75f, 0.78f);
	styleDefault().builtin[ImGuiCol_SeparatorActive]        = ImVec4(0.10f, 0.40f, 0.75f, 1.00f);
	styleDefault().builtin[ImGuiCol_ResizeGrip]             = ImVec4(0.26f, 0.59f, 0.98f, 0.25f);
	styleDefault().builtin[ImGuiCol_ResizeGripHovered]      = ImVec4(0.26f, 0.59f, 0.98f, 0.67f);
	styleDefault().builtin[ImGuiCol_ResizeGripActive]       = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);
	styleDefault().builtin[ImGuiCol_Tab]                    = ImLerp(styleDefault().builtin[ImGuiCol_Header],       styleDefault().builtin[ImGuiCol_TitleBgActive], 0.80f);
	styleDefault().builtin[ImGuiCol_TabHovered]             = styleDefault().builtin[ImGuiCol_HeaderHovered];
	styleDefault().builtin[ImGuiCol_TabActive]              = ImLerp(styleDefault().builtin[ImGuiCol_HeaderActive], styleDefault().builtin[ImGuiCol_TitleBgActive], 0.60f);
	styleDefault().builtin[ImGuiCol_TabUnfocused]           = ImLerp(styleDefault().builtin[ImGuiCol_Tab],          styleDefault().builtin[ImGuiCol_TitleBg], 0.80f);
	styleDefault().builtin[ImGuiCol_TabUnfocusedActive]     = ImLerp(styleDefault().builtin[ImGuiCol_TabActive],    styleDefault().builtin[ImGuiCol_TitleBg], 0.40f);
	styleDefault().builtin[ImGuiCol_PlotLines]              = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
	styleDefault().builtin[ImGuiCol_PlotLinesHovered]       = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	styleDefault().builtin[ImGuiCol_PlotHistogram]          = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	styleDefault().builtin[ImGuiCol_PlotHistogramHovered]   = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	styleDefault().builtin[ImGuiCol_TableHeaderBg]          = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
	styleDefault().builtin[ImGuiCol_TableBorderStrong]      = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
	styleDefault().builtin[ImGuiCol_TableBorderLight]       = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
	styleDefault().builtin[ImGuiCol_TableRowBg]             = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	styleDefault().builtin[ImGuiCol_TableRowBgAlt]          = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
	styleDefault().builtin[ImGuiCol_TextSelectedBg]         = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
	styleDefault().builtin[ImGuiCol_DragDropTarget]         = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
	styleDefault().builtin[ImGuiCol_NavHighlight]           = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
	styleDefault().builtin[ImGuiCol_NavWindowingHighlight]  = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
	styleDefault().builtin[ImGuiCol_NavWindowingDimBg]      = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
	styleDefault().builtin[ImGuiCol_ModalWindowDimBg]       = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
	styleDefault().tabTextColor                             = ImGui::GetColorU32(ImVec4(1.00f, 1.00f, 1.00f, 1.00f));
	styleDefault().tabTextPendingColor                      = ImGui::GetColorU32(ImVec4(1.00f, 1.00f, 1.00f, 1.00f));
	styleDefault().tabPendingColor                          = ImGui::GetColorU32(ImVec4(0.40f, 0.13f, 0.47f, 1.00f));
	styleDefault().tabPendingHoveredColor                   = ImGui::GetColorU32(ImVec4(0.50f, 0.23f, 0.57f, 1.00f));
	styleDefault().iconColor                                = ImGui::GetColorU32(ImVec4(1.00f, 1.00f, 1.00f, 1.00f));
	styleDefault().iconDisabledColor                        = ImGui::GetColorU32(ImVec4(0.50f, 0.50f, 0.50f, 1.00f));
	styleDefault().messageColor                             = ImGui::GetColorU32(ImVec4(1.00f, 1.00f, 1.00f, 1.00f));
	styleDefault().warningColor                             = ImGui::GetColorU32(ImVec4(0.95f, 0.93f, 0.10f, 1.00f));
	styleDefault().errorColor                               = ImGui::GetColorU32(ImVec4(0.93f, 0.00f, 0.00f, 1.00f));

	generic_AddFile("Add file");
	generic_All("All");
	generic_Apply("Apply");
	generic_Binary("Binary");
	generic_Browse("Browse");
	generic_Cancel("Cancel");
	generic_Clear("Clear");
	generic_Close("Close");
	generic_Export("Export");
	generic_Import("Import");
	generic_Install("Install");
	generic_List("List");
	generic_No("No");
	generic_None("<None>");
	generic_Ok("Ok");
	generic_Open("Open");
	generic_Ready("Ready");
	generic_Reinstall("Reinstall");
	generic_Revert("Revert");
	generic_SaveTo("Save to");
	generic_Tab("Tab");
	generic_Text("Text");
	generic_Unknown("<Unknown>");
	generic_Yes("Yes");

	menuFile("File");
	menuFile_New("New");
	menuFile_Open("Open...");
	menuFile_OpenDirectory("Open Directory...");
	menuFile_OpenExamples("Open Examples");
	menuFile_OpenRecent("Open Recent");
	menuFile_Clear("Clear");
	menuFile_Close("Close");
	menuFile_SaveAsset("Save Asset");
	menuFile_Save("Save");
	menuFile_SaveAs("Save as...");
	menuFile_SaveAsFile("Save as File...");
	menuFile_SaveAsDirectory("Save as Directory...");
	menuFile_SaveFile("Save File...");
	menuFile_SaveDirectory("Save Directory...");
	menuFile_Preferences("Preferences...");
	menuFile_Quit("Quit");

	menuEdit("Edit");
	menuEdit_Undo("Undo");
	menuEdit_Redo("Redo");
	menuEdit_Cut("Cut");
	menuEdit_Copy("Copy");
	menuEdit_Paste("Paste");
	menuEdit_Delete("Delete");
	menuEdit_SelectAll("Select All");
	menuEdit_IncreaseIndent("Increase Indent");
	menuEdit_DecreaseIndent("Decrease Indent");
	menuEdit_Find("Find...");
	menuEdit_FindNext("Find Next");
	menuEdit_FindPrevious("Find Previous");
	menuEdit_GotoLine("Goto Line...");
	menuEdit_ResizeGrid("Resize Grid...");
	menuEdit_ResizeImage("Resize Image...");
	menuEdit_ResizeTile("Resize Tile...");
	menuEdit_ResizeMap("Resize Map...");
	menuEdit_ResolveRef("Resolve Ref...");

	menuProject("Project");
	menuProject_Run("Run");
	menuProject_Stop("Stop");
	menuProject_NewAsset("New Asset...");
	menuProject_RemoveAsset("Remove Asset...");
	menuProject_RenameAsset("Rename Asset...");
	menuProject_FilterAssets("Filter Assets...");
	menuProject_AddFile("Add File...");
	menuProject_Import("Import...");
	menuProject_Export("Export...");
	menuProject_Reload("Reload");
	menuProject_Browse("Browse...");
	menuProject_Explore("Explore...");

	menuAsset_Code("Code");
	menuAsset_Sprites("Sprites");
	menuAsset_Maps("Maps");
	menuAsset_Images("Images");
	menuAsset_Palettes("Palettes");
	menuAsset_Fonts("Fonts");
	menuAsset_Audio("Audio");
	menuAsset_Json("JSON");
	menuAsset_Text("Text");

	menuDebug("Debug");
	menuDebug_Break("Break");
	menuDebug_Continue("Continue");
	menuDebug_Step("Step");
	menuDebug_StepInto("Step Into");
	menuDebug_StepOut("Step Out");
	menuDebug_ToggleBreakpoint("Toggle Breakpoint");

	menuPlugins("Plugins");

	menuWindow("Window");
	menuWindow_Screen("Screen");
	menuWindow_Screen_ShootCanvas("Shoot Canvas");
	menuWindow_Screen_RecordCanvas("Record Canvas");
	menuWindow_Screen_StopRecording("Stop Recording");
	menuWindow_Application("Application");
	menuWindow_Application_Fullscreen("Fullscreen");
	menuWindow_Application_Maximized("Maximized");
	menuWindow_Canvas("Canvas");
	menuWindow_Canvas_Popup("Popup");
	menuWindow_Canvas_Frame("Frame");
	menuWindow_Canvas_Maximized("Maximized");
	menuWindow_Buttons("Buttons");
	menuWindow_Assets("Assets");
	menuWindow_Debug("Debug");
	menuWindow_Console("Console");

	menuHelp("Help");
	menuHelp_Manual("Manual");
	menuHelp_About("About");

	dialogPrompt_AlreadyExists("Already exists.");
	dialogPrompt_CannotAddMoreFrame("Cannot add more frame.");
	dialogPrompt_CannotLoadProject("Cannot load project.");
	dialogPrompt_CannotReadFromCurrentProject("Cannot read from current project.");
	dialogPrompt_CannotSaveToReadonlyLocations("Cannot save to readonly locations.");
	dialogPrompt_CannotSaveToNonemptyDirectory("Cannot save to nonempty directory.");
	dialogPrompt_CannotWriteToCurrentProject("Cannot write to current project.");
	dialogPrompt_GetFullVersionToSave("Get full version to save.");
	dialogPrompt_InvalidArea("Invalid area.");
	dialogPrompt_InvalidAsset("Invalid asset.");
	dialogPrompt_InvalidName("Invalid name.");
	dialogPrompt_InvalidProject("Invalid project.");
	dialogPrompt_InvalidRef("Invalid ref.");
	dialogPrompt_NotUndoable("(Not undoable)");
	dialogPrompt_PathDoesntExistRemoveThisRecord("Path doesn't exist,\nremove this record?");
	dialogPrompt_Reading("Reading...");
	dialogPrompt_Running("Running...");
	dialogPrompt_Saving("Saving...");
	dialogPrompt_UnknownType("Unknown type.");
	dialogPrompt_Writing("Writing...");

	dialogAsk_DidntQuitCleanlyBrowseTheLatestBackup("Didn't quit cleanly,\nbrowse the latest backup?");
	dialogAsk_RemoveAsset("Remove asset?");
	dialogAsk_SaveAsset("Save asset?");
	dialogAsk_SaveTheCurrentWorkspace("Save the current workspace?");

	dialogItem_ConflictAssets("Conflict assets:");
	dialogItem_Find("Find:");
	dialogItem_Goto("Goto:");
	dialogItem_GridSize("Grid size:");
	dialogItem_ImageSize("Image size:");
	dialogItem_Input("Input:");
	dialogItem_InputAnimationName("Input animation name:");
	dialogItem_InputAssetName("Input asset name:");
	dialogItem_InputInterval("Input interval:");
	dialogItem_MapSize("Map size:");
	dialogItem_Palette("Palette:");
	dialogItem_Path("Path:");
	dialogItem_Ref("Ref:");
	dialogItem_ResolveAssetRefFor("Resolve asset ref for:");
	dialogItem_SelectAsset("Select asset:");
	dialogItem_SelectAssets("Select assets:");
	dialogItem_Size("Size:");
	dialogItem_TileSize("Tile size:");
	dialogItem_TotalCount("total {COUNT}...");
	dialogItem_Type("Type:");
	dialogItem_UnsolveAssetRefFor("Unsolve asset ref for:");
	dialogItem_View("View:");

	tabCanvas("[Canvas]");

	tabPreferences_Editor("Editor");
	tabPreferences_Graphics("Graphics");
	tabPreferences_Input("Input");
	tabPreferences_Misc("Misc.");
	tabPreferences_Onscreen("Onscreen");

	windowPreferences("Preferences");
	windowPreferences_Editor_AutoBackup("Auto backup");
	windowPreferences_Editor_ClearOnStart("Clear on start");
	windowPreferences_Editor_Console("Console:");
	windowPreferences_Editor_IgnoreDotFiles("Ignore dot files");
	windowPreferences_Editor_PackageFormat("Package format");
	windowPreferences_Editor_Project("Project:");
	windowPreferences_Editor_ShowWhiteSpaces("Show white spaces");
	windowPreferences_Editor_TextEditor("Text editor:");
	windowPreferences_Graphics_Application("Application:");
	windowPreferences_Graphics_Canvas("Canvas:");
	windowPreferences_Graphics_FixCanvasRatio("Fix canvas ratio");
	windowPreferences_Graphics_Fullscreen("Fullscreen");
	windowPreferences_Input_ClickAgainToCancelBackspaceToClear("(Click again to cancel, Backspace to clear)");
	windowPreferences_Input_ClickToChange("(Click to change)");
	windowPreferences_Input_Gamepads("Gamepads:");
	windowPreferences_Input_WaitingForInput("Waiting for input...");
	windowPreferences_Misc_Application("Application:");
	windowPreferences_Misc_PauseOnFocusLost("Pause on focus lost");
	windowPreferences_NeedToReopen("                (Need to reopen)");
	windowPreferences_Onscreen_Enabled("Enabled");
	windowPreferences_Onscreen_Gamepad("Gamepad:");
	windowPreferences_Onscreen_PaddingX("Padding X");
	windowPreferences_Onscreen_PaddingY("Padding Y");
	windowPreferences_Onscreen_Scale("    Scale");
	windowPreferences_Onscreen_SwapAB("Swap A/B");

	windowAssets("Assets");

	windowDebug("Debug");
	windowDebug_Running("Running...");
	windowDebug_CallStack("Call stack:");
	windowDebug_Source("Source");
	windowDebug_Name("Fn");
	windowDebug_Local("Local:");
	windowDebug_Upvalue("Upvalue:");
	windowDebug_VariableName("Name");
	windowDebug_VariableType("Type");
	windowDebug_VariableValue("Value");

	windowPaused_Resume("RESUME");
	windowPaused_Options("OPTIONS");
	windowPaused_About("ABOUT");

	windowCanvas("Canvas");

	windowDocument("Document");

	windowConsole("Console");

	windowAbout("About");

	tooltipAssets_New("New");
	tooltipAssets_Remove("Remove");

	tooltipEditing_AddAnimation("Add animation");
	tooltipEditing_AddFrame("Add frame");
	tooltipEditing_CaseSensitive("Case-sensitive");
	tooltipEditing_DeleteAnimation("Delete animation");
	tooltipEditing_DeleteFrame("Delete frame");
	tooltipEditing_InsertFrame("Insert frame");
	tooltipEditing_MatchWholeWords("Match whole words");
	tooltipEditing_RenameAnimation("Rename animation");

	tooltipProject_InputDirSubFileToCreateInDirectory("Eg. input \"dir/sub/.../file\" to create in directory");
	tooltipProject_DragOrDoubleClickToChange("Drag or double click to change");
	tooltipProject_OptionalSelectAPaletteNoneForTrueColor("Optional, select a palette; none for true-color");
	tooltipProject_SelectAnImage("Select an image");

	tooltipDebug_Break("Pause");
	tooltipDebug_Clear("Clear breakpoints");
	tooltipDebug_Continue("Resume (F5)");
	tooltipDebug_Disable("Disable breakpoints");
	tooltipDebug_Enable("Enable breakpoints");
	tooltipDebug_Step("Step (F10)");
	tooltipDebug_StepInto("Step into (F11)");
	tooltipDebug_StepOut("Step out (Shift+F11)");

	statusTip_Readonly("Readonly");

	statusItem_Area("Area:");
	statusItem_Col("Col:");
	statusItem_Index("Index:");
	statusItem_Ln("Ln:");
	statusItem_Pos("Pos:");

	statusItemSound_Title("     Title:");
	statusItemSound_Artist("    Artist:");
	statusItemSound_Album("     Album:");
	statusItemSound_Copyright(" Copyright:");

	iconPlay(createTexture(rnd, RES_ICON_PLAY, BITTY_COUNTOF(RES_ICON_PLAY)));
	iconPlay_Gray(createTexture(rnd, RES_ICON_PLAY_GRAY, BITTY_COUNTOF(RES_ICON_PLAY_GRAY)));
	iconPause(createTexture(rnd, RES_ICON_PAUSE, BITTY_COUNTOF(RES_ICON_PAUSE)));
	iconStop(createTexture(rnd, RES_ICON_STOP, BITTY_COUNTOF(RES_ICON_STOP)));

	iconCopy(createTexture(rnd, RES_ICON_COPY, BITTY_COUNTOF(RES_ICON_COPY)));
	iconCopy_Gray(createTexture(rnd, RES_ICON_COPY_GRAY, BITTY_COUNTOF(RES_ICON_COPY_GRAY)));
	iconCut(createTexture(rnd, RES_ICON_CUT, BITTY_COUNTOF(RES_ICON_CUT)));
	iconCut_Gray(createTexture(rnd, RES_ICON_CUT_GRAY, BITTY_COUNTOF(RES_ICON_CUT_GRAY)));
	iconPaste(createTexture(rnd, RES_ICON_PASTE, BITTY_COUNTOF(RES_ICON_PASTE)));
	iconPaste_Gray(createTexture(rnd, RES_ICON_PASTE_GRAY, BITTY_COUNTOF(RES_ICON_PASTE_GRAY)));

	iconUndo(createTexture(rnd, RES_ICON_UNDO, BITTY_COUNTOF(RES_ICON_UNDO)));
	iconUndo_Gray(createTexture(rnd, RES_ICON_UNDO_GRAY, BITTY_COUNTOF(RES_ICON_UNDO_GRAY)));
	iconRedo(createTexture(rnd, RES_ICON_REDO, BITTY_COUNTOF(RES_ICON_REDO)));
	iconRedo_Gray(createTexture(rnd, RES_ICON_REDO_GRAY, BITTY_COUNTOF(RES_ICON_REDO_GRAY)));

	iconMove(createTexture(rnd, RES_ICON_MOVE, BITTY_COUNTOF(RES_ICON_MOVE)));
	iconEyedropper(createTexture(rnd, RES_ICON_EYEDROPPER, BITTY_COUNTOF(RES_ICON_EYEDROPPER)));
	iconPencil(createTexture(rnd, RES_ICON_PENCIL, BITTY_COUNTOF(RES_ICON_PENCIL)));
	iconPaintbucket(createTexture(rnd, RES_ICON_PAINTBUCKET, BITTY_COUNTOF(RES_ICON_PAINTBUCKET)));
	iconLasso(createTexture(rnd, RES_ICON_LASSO, BITTY_COUNTOF(RES_ICON_LASSO)));
	iconLine(createTexture(rnd, RES_ICON_LINE, BITTY_COUNTOF(RES_ICON_LINE)));
	iconBox(createTexture(rnd, RES_ICON_BOX, BITTY_COUNTOF(RES_ICON_BOX)));
	iconBox_Fill(createTexture(rnd, RES_ICON_BOX_FILL, BITTY_COUNTOF(RES_ICON_BOX_FILL)));
	iconEllipse(createTexture(rnd, RES_ICON_ELLIPSE, BITTY_COUNTOF(RES_ICON_ELLIPSE)));
	iconEllipse_Fill(createTexture(rnd, RES_ICON_ELLIPSE_FILL, BITTY_COUNTOF(RES_ICON_ELLIPSE_FILL)));
	iconStamp(createTexture(rnd, RES_ICON_STAMP, BITTY_COUNTOF(RES_ICON_STAMP)));

	iconClock(createTexture(rnd, RES_ICON_CLOCK, BITTY_COUNTOF(RES_ICON_CLOCK)));

	iconMagnify(createTexture(rnd, RES_ICON_MAGNIFY, BITTY_COUNTOF(RES_ICON_MAGNIFY)));

	iconPencils(createTexture(rnd, RES_ICON_PENCILS, BITTY_COUNTOF(RES_ICON_PENCILS)));

	iconRotate_Clockwise(createTexture(rnd, RES_ICON_ROTATE_CLOCKWISE, BITTY_COUNTOF(RES_ICON_ROTATE_CLOCKWISE)));
	iconRotate_Anticlockwise(createTexture(rnd, RES_ICON_ROTATE_ANTICLOCKWISE, BITTY_COUNTOF(RES_ICON_ROTATE_ANTICLOCKWISE)));
	iconRotate_HalfTurn(createTexture(rnd, RES_ICON_ROTATE_HALF_TURN, BITTY_COUNTOF(RES_ICON_ROTATE_HALF_TURN)));
	iconFlip_Vertically(createTexture(rnd, RES_ICON_FLIP_VERTICALLY, BITTY_COUNTOF(RES_ICON_FLIP_VERTICALLY)));
	iconFlip_Horizontally(createTexture(rnd, RES_ICON_FLIP_HORIZONTALLY, BITTY_COUNTOF(RES_ICON_FLIP_HORIZONTALLY)));

	iconTransparent(createTexture(rnd, RES_ICON_TRANSPARENT, BITTY_COUNTOF(RES_ICON_TRANSPARENT)));

	sliceDirectory(createTexture(rnd, RES_SLICE_DIRECTORY, BITTY_COUNTOF(RES_SLICE_DIRECTORY)));
	sliceDirectory_Open(createTexture(rnd, RES_SLICE_DIRECTORY_OPEN, BITTY_COUNTOF(RES_SLICE_DIRECTORY_OPEN)));
	sliceFile(createTexture(rnd, RES_SLICE_FILE, BITTY_COUNTOF(RES_SLICE_FILE)));

	sliceFilter(createTexture(rnd, RES_SLICE_FILTER, BITTY_COUNTOF(RES_SLICE_FILTER)));
	sliceRecycle(createTexture(rnd, RES_SLICE_RECYCLE, BITTY_COUNTOF(RES_SLICE_RECYCLE)));

	slicePrevious(createTexture(rnd, RES_SLICE_PREVIOUS, BITTY_COUNTOF(RES_SLICE_PREVIOUS)));
	sliceNext(createTexture(rnd, RES_SLICE_NEXT, BITTY_COUNTOF(RES_SLICE_NEXT)));

	sliceCaseSensitive(createTexture(rnd, RES_SLICE_CASE_SENSITIVE, BITTY_COUNTOF(RES_SLICE_CASE_SENSITIVE)));
	sliceWholeWord(createTexture(rnd, RES_SLICE_WHOLE_WORD, BITTY_COUNTOF(RES_SLICE_WHOLE_WORD)));

	slicePlus(createTexture(rnd, RES_SLICE_PLUS, BITTY_COUNTOF(RES_SLICE_PLUS)));
	sliceMinus(createTexture(rnd, RES_SLICE_MINUS, BITTY_COUNTOF(RES_SLICE_MINUS)));

	slicePlay(createTexture(rnd, RES_SLICE_PLAY, BITTY_COUNTOF(RES_SLICE_PLAY)));
	sliceStop(createTexture(rnd, RES_SLICE_STOP, BITTY_COUNTOF(RES_SLICE_STOP)));
	slicePause(createTexture(rnd, RES_SLICE_PAUSE, BITTY_COUNTOF(RES_SLICE_PAUSE)));
	sliceBreakpoints_Disable(createTexture(rnd, RES_SLICE_BREAKPOINTS_DISABLE, BITTY_COUNTOF(RES_SLICE_BREAKPOINTS_DISABLE)));
	sliceBreakpoints_Enable(createTexture(rnd, RES_SLICE_BREAKPOINTS_ENABLE, BITTY_COUNTOF(RES_SLICE_BREAKPOINTS_ENABLE)));
	sliceBreakpoints_Clear(createTexture(rnd, RES_SLICE_BREAKPOINTS_CLEAR, BITTY_COUNTOF(RES_SLICE_BREAKPOINTS_CLEAR)));
	sliceStep(createTexture(rnd, RES_SLICE_STEP, BITTY_COUNTOF(RES_SLICE_STEP)));
	sliceStep_Into(createTexture(rnd, RES_SLICE_STEP_INTO, BITTY_COUNTOF(RES_SLICE_STEP_INTO)));
	sliceStep_Out(createTexture(rnd, RES_SLICE_STEP_OUT, BITTY_COUNTOF(RES_SLICE_STEP_OUT)));

	sliceNumber_1(createTexture(rnd, RES_SLICE_NUMBER_1, BITTY_COUNTOF(RES_SLICE_NUMBER_1)));
	sliceNumber_2(createTexture(rnd, RES_SLICE_NUMBER_2, BITTY_COUNTOF(RES_SLICE_NUMBER_2)));
	sliceNumber_3(createTexture(rnd, RES_SLICE_NUMBER_3, BITTY_COUNTOF(RES_SLICE_NUMBER_3)));
	sliceNumber_4(createTexture(rnd, RES_SLICE_NUMBER_4, BITTY_COUNTOF(RES_SLICE_NUMBER_4)));

	imagePadPortraitTop(createTexture(rnd, RES_IMAGE_PAD_PORTRAIT_TOP, BITTY_COUNTOF(RES_IMAGE_PAD_PORTRAIT_TOP)));
	imagePadPortraitBottom(createTexture(rnd, RES_IMAGE_PAD_PORTRAIT_BOTTOM, BITTY_COUNTOF(RES_IMAGE_PAD_PORTRAIT_BOTTOM)));
	imagePadLandscapeLeft(createTexture(rnd, RES_IMAGE_PAD_LANDSCAPE_LEFT, BITTY_COUNTOF(RES_IMAGE_PAD_LANDSCAPE_LEFT)));
	imagePadLandscapeRight(createTexture(rnd, RES_IMAGE_PAD_LANDSCAPE_RIGHT, BITTY_COUNTOF(RES_IMAGE_PAD_LANDSCAPE_RIGHT)));

	return true;
}

bool Theme::close(class Renderer* rnd) {
	if (!_opened)
		return false;
	_opened = false;

	style(nullptr);

	destroyTexture(rnd, iconPlay());
	destroyTexture(rnd, iconPlay_Gray());
	destroyTexture(rnd, iconPause());
	destroyTexture(rnd, iconStop());

	destroyTexture(rnd, iconCopy());
	destroyTexture(rnd, iconCopy_Gray());
	destroyTexture(rnd, iconCut());
	destroyTexture(rnd, iconCut_Gray());
	destroyTexture(rnd, iconPaste());
	destroyTexture(rnd, iconPaste_Gray());

	destroyTexture(rnd, iconUndo());
	destroyTexture(rnd, iconUndo_Gray());
	destroyTexture(rnd, iconRedo());
	destroyTexture(rnd, iconRedo_Gray());

	destroyTexture(rnd, iconMove());
	destroyTexture(rnd, iconEyedropper());
	destroyTexture(rnd, iconPencil());
	destroyTexture(rnd, iconPaintbucket());
	destroyTexture(rnd, iconLasso());
	destroyTexture(rnd, iconLine());
	destroyTexture(rnd, iconBox());
	destroyTexture(rnd, iconBox_Fill());
	destroyTexture(rnd, iconEllipse());
	destroyTexture(rnd, iconEllipse_Fill());
	destroyTexture(rnd, iconStamp());

	destroyTexture(rnd, iconClock());

	destroyTexture(rnd, iconMagnify());

	destroyTexture(rnd, iconPencils());

	destroyTexture(rnd, iconRotate_Clockwise());
	destroyTexture(rnd, iconRotate_Anticlockwise());
	destroyTexture(rnd, iconRotate_HalfTurn());
	destroyTexture(rnd, iconFlip_Vertically());
	destroyTexture(rnd, iconFlip_Horizontally());

	destroyTexture(rnd, iconTransparent());

	destroyTexture(rnd, sliceDirectory());
	destroyTexture(rnd, sliceDirectory_Open());
	destroyTexture(rnd, sliceFile());

	destroyTexture(rnd, sliceFilter());
	destroyTexture(rnd, sliceRecycle());

	destroyTexture(rnd, slicePrevious());
	destroyTexture(rnd, sliceNext());

	destroyTexture(rnd, sliceCaseSensitive());
	destroyTexture(rnd, sliceWholeWord());

	destroyTexture(rnd, slicePlus());
	destroyTexture(rnd, sliceMinus());

	destroyTexture(rnd, slicePlay());
	destroyTexture(rnd, sliceStop());
	destroyTexture(rnd, slicePause());
	destroyTexture(rnd, sliceBreakpoints_Disable());
	destroyTexture(rnd, sliceBreakpoints_Enable());
	destroyTexture(rnd, sliceBreakpoints_Clear());
	destroyTexture(rnd, sliceStep());
	destroyTexture(rnd, sliceStep_Into());
	destroyTexture(rnd, sliceStep_Out());

	destroyTexture(rnd, sliceNumber_1());
	destroyTexture(rnd, sliceNumber_2());
	destroyTexture(rnd, sliceNumber_3());
	destroyTexture(rnd, sliceNumber_4());

	destroyTexture(rnd, imagePadPortraitTop());
	destroyTexture(rnd, imagePadPortraitBottom());
	destroyTexture(rnd, imagePadLandscapeLeft());
	destroyTexture(rnd, imagePadLandscapeRight());

	return true;
}

bool Theme::load(class Renderer* rnd) {
	// Prepare.
	ImGuiIO &io = ImGui::GetIO();

	// Load theme config.
	fromFile(THEME_CONFIG_DIR THEME_CONFIG_DEFAULT_NAME "." BITTY_JSON_EXT);

	DirectoryInfo::Ptr dirInfo = DirectoryInfo::make(THEME_CONFIG_DIR);
	FileInfos::Ptr fileInfos = dirInfo->getFiles("*." BITTY_JSON_EXT, false);
	for (int i = 0; i < fileInfos->count(); ++i) {
		FileInfo::Ptr fileInfo = fileInfos->get(i);
		if (fileInfo->fileName() == THEME_CONFIG_DEFAULT_NAME)
			continue;

		fromFile(fileInfo->fullPath().c_str());
	}

	// Load the builtin fonts.
	ImFontConfig fontCfg;
	fontCfg.FontDataOwnedByAtlas = false;
	fontBlock(
		io.Fonts->AddFontFromMemoryTTF(
			(void*)RES_FONT_BLOCK, sizeof(RES_FONT_BLOCK),
			28.0f,
			&fontCfg, io.Fonts->GetGlyphRangesDefault()
		)
	);
	fontBlock_Bold(
		io.Fonts->AddFontFromMemoryTTF(
			(void*)RES_FONT_BLOCK_BOLD, sizeof(RES_FONT_BLOCK_BOLD),
			28.0f,
			&fontCfg, io.Fonts->GetGlyphRangesDefault()
		)
	);
	fontBlock_Italic(
		io.Fonts->AddFontFromMemoryTTF(
			(void*)RES_FONT_BLOCK_ITALIC, sizeof(RES_FONT_BLOCK_ITALIC),
			28.0f,
			&fontCfg, io.Fonts->GetGlyphRangesDefault()
		)
	);
	fontBlock_BoldItalic(
		io.Fonts->AddFontFromMemoryTTF(
			(void*)RES_FONT_BLOCK_BOLD_ITALIC, sizeof(RES_FONT_BLOCK_BOLD_ITALIC),
			28.0f,
			&fontCfg, io.Fonts->GetGlyphRangesDefault()
		)
	);

	// Rebuild the font glyphs.
	if (io.Fonts->TexID) {
		ImGuiSDL::Texture* texture = static_cast<ImGuiSDL::Texture*>(io.Fonts->TexID);
		delete texture;
		io.Fonts->TexID = nullptr;
	}

	unsigned char* pixels = nullptr;
	int width = 0, height = 0;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
	ImGuiSDL::Texture* texture = new ImGuiSDL::Texture(rnd, pixels, width, height);
	io.Fonts->TexID = (void*)texture;

	// Finish.
	return true;
}

bool Theme::save(void) const {
	return true;
}

class Texture* Theme::createTexture(class Renderer* rnd, const Byte* buf, size_t len) {
	Texture* texture = Texture::create();
	Image::Ptr image(Image::create(nullptr));
	image->fromBytes(buf, len);
	texture->fromImage(rnd, Texture::STATIC, image.get(), Texture::NEAREST);

	return texture;
}

class Texture* Theme::createTexture(class Renderer* rnd, const char* path) {
	Texture* texture = nullptr;
	File::Ptr file(File::create());
	if (file->open(path, Stream::READ)) {
		Bytes::Ptr buf(Bytes::create());
		file->readBytes(buf.get());
		texture = createTexture(rnd, buf->pointer(), buf->count());
		file->close();
	}

	return texture;
}

void Theme::destroyTexture(class Renderer* /* rnd */, class Texture* &tex) {
	Texture::destroy(tex);
	tex = nullptr;
}

void Theme::setColor(const std::string &key, ImGuiCol idx, const ImColor &col) {
	if (idx < 0 || idx >= ImGuiCol_COUNT)
		return;

	const bool light = key == "light" || key == "all" || key == "default";

	if (light)
		styleDefault().builtin[idx] = (ImVec4)col;
}

void Theme::fromFile(const char* path_) {
	ImGuiIO &io = ImGui::GetIO();

	const std::string cfgPath = path_;

	rapidjson::Document doc;
	File::Ptr file(File::create());
	if (file->open(cfgPath.c_str(), Stream::READ)) {
		std::string buf;
		file->readString(buf);
		file->close();
		if (!Json::fromString(doc, buf.c_str()))
			doc.SetNull();
	}
	file.reset();

	const rapidjson::Value* colors = nullptr;
	Jpath::get(doc, colors, "colors");
	if (colors && colors->IsObject()) {
		for (rapidjson::Value::ConstMemberIterator itcol = colors->MemberBegin(); itcol != colors->MemberEnd(); ++itcol) {
			const rapidjson::Value &jkcol = itcol->name;
			const rapidjson::Value &jvcol = itcol->value;

			if (!jkcol.IsString() || !jvcol.IsObject())
				continue;

			const std::string key = jkcol.GetString();
			for (rapidjson::Value::ConstMemberIterator ittheme = jvcol.MemberBegin(); ittheme != jvcol.MemberEnd(); ++ittheme) {
				const rapidjson::Value &jktheme = ittheme->name;
				const rapidjson::Value &jvtheme = ittheme->value;

				const std::string theme = jktheme.GetString();
				Color col;
				if (!Jpath::get(jvtheme, col.r, 0) || !Jpath::get(jvtheme, col.g, 1) || !Jpath::get(jvtheme, col.b, 2) || !Jpath::get(jvtheme, col.a, 3))
					continue;

				if (theme == "window_mask_background")
					setColor(key, ImGuiCol_ModalWindowDimBg, ImColor(col.r, col.g, col.b, col.a));
			}
		}
	}

	const rapidjson::Value* fonts = nullptr;
	Jpath::get(doc, fonts, "fonts");
	if (fonts && fonts->IsArray()) {
		for (int i = 0; i < (int)fonts->Capacity(); ++i) {
			std::string operation = "merge";
			std::string usage = "generic";
			std::string path;
			float size = 0;
			std::string ranges;
			Math::Vec2i oversample(0, 0);
			Math::Vec2f glyphOffset(0, 0);

			Jpath::get(*fonts, operation, i, "operation");
			Jpath::get(*fonts, usage, i, "usage");
			Jpath::get(*fonts, path, i, "path");
			Jpath::get(*fonts, size, i, "size");
			Jpath::get(*fonts, ranges, i, "ranges");
			Jpath::get(*fonts, oversample.x, i, "oversample", 0);
			Jpath::get(*fonts, oversample.y, i, "oversample", 1);
			Jpath::get(*fonts, glyphOffset.x, i, "glyph_offset", 0);
			Jpath::get(*fonts, glyphOffset.y, i, "glyph_offset", 1);

			size = Math::clamp(size, 4.0f, 96.0f);
			oversample.x = Math::clamp(oversample.x, (Int)1, (Int)8);
			oversample.y = Math::clamp(oversample.y, (Int)1, (Int)8);
			glyphOffset.x = Math::clamp(glyphOffset.x, (Real)-96, (Real)96);
			glyphOffset.y = Math::clamp(glyphOffset.y, (Real)-96, (Real)96);

			const ImWchar* glyphRanges = io.Fonts->GetGlyphRangesDefault();
			ImVector<ImWchar> rangesVector;
			if (ranges == THEME_FONT_RANGES_DEFAULT_NAME) {
				glyphRanges = io.Fonts->GetGlyphRangesDefault();
			} else if (ranges == THEME_FONT_RANGES_CHINESE_NAME) {
				glyphRanges = io.Fonts->GetGlyphRangesChineseSimplifiedCommon();
			} else if (ranges == THEME_FONT_RANGES_JAPANESE_NAME) {
				glyphRanges = io.Fonts->GetGlyphRangesJapanese();
			} else if (ranges == THEME_FONT_RANGES_KOREAN_NAME) {
				glyphRanges = io.Fonts->GetGlyphRangesKorean();
			} else if (ranges == THEME_FONT_RANGES_CYRILLIC_NAME) {
				glyphRanges = io.Fonts->GetGlyphRangesCyrillic();
			} else if (ranges == THEME_FONT_RANGES_THAI_NAME) {
				glyphRanges = io.Fonts->GetGlyphRangesThai();
			} else if (ranges == THEME_FONT_RANGES_VIETNAMESE_NAME) {
				glyphRanges = io.Fonts->GetGlyphRangesVietnamese();
			} else if (ranges == THEME_FONT_RANGES_POLISH_NAME) {
				static constexpr const ImWchar RANGES_POLISH[] = {
					0x0020, 0x00ff, // Basic Latin + Latin supplement.
					0x0100, 0x017f, // Polish alphabet.
					0
				};
				glyphRanges = RANGES_POLISH;
			} else if (!ranges.empty()) {
				const std::wstring wstr = Unicode::toWide(ranges);
				if (!wstr.empty() && wstr.size() % 2 == 0) {
					for (int j = 0; j < (int)wstr.length(); j += 2) {
						const wchar_t ch0 = wstr[j];
						const wchar_t ch1 = wstr[j + 1];
						if (ch0 > ch1) {
							rangesVector.clear();

							break;
						}
						rangesVector.push_back(ch0);
						rangesVector.push_back(ch1);
					}
					if (!rangesVector.empty()) {
						if (rangesVector.back() != 0)
							rangesVector.push_back(0);
						glyphRanges = &rangesVector.front();
					}
				}
			}

			ImFontConfig fontCfg;
			fontCfg.OversampleH = (int)oversample.x; fontCfg.OversampleV = (int)oversample.y;
			fontCfg.GlyphOffset = ImVec2((float)glyphOffset.x, (float)glyphOffset.y);
			fontCfg.MergeMode = operation == "merge";
			if (operation == "set" || operation == "merge") {
				const bool setDefault = operation == "set" && usage == "generic" && glyphRanges == io.Fonts->GetGlyphRangesDefault();
				if (setDefault)
					io.Fonts->Clear();
				ImFont* font = io.Fonts->AddFontFromFileTTF(
					path.c_str(),
					size,
					&fontCfg, glyphRanges
				);
				if (setDefault && !font)
					io.Fonts->AddFontDefault();
				if (usage == "code")
					fontCode(font);
			} else if (operation == "clear") {
				io.Fonts->Clear();
				io.Fonts->AddFontDefault();
			}
		}
	}
}

/* ===========================================================================} */
