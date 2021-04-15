/*
** Bitty
**
** An itty bitty game engine.
**
** Copyright (C) 2020 - 2021 Tony Wang, all rights reserved
**
** For the latest info, see https://github.com/paladin-t/bitty/
*/

#ifndef __THEME_H__
#define __THEME_H__

#include "bitty.h"
#include "mathematics.h"
#include "../lib/imgui/imgui.h"
#include <string>

/*
** {===========================================================================
** Macros and constants
*/

#ifndef THEME_CONFIG_DIR
#	define THEME_CONFIG_DIR "../themes/" /* Relative path. */
#endif /* THEME_CONFIG_DIR */
#ifndef THEME_CONFIG_DEFAULT_NAME
#	define THEME_CONFIG_DEFAULT_NAME "default"
#endif /* THEME_CONFIG_DEFAULT_NAME */

/* ===========================================================================} */

/*
** {===========================================================================
** Theme
*/

/**
 * @brief Theme entity.
 */
class Theme {
public:
	enum Styles {
		DARK,
		CLASSIC,
		LIGHT
	};

	struct Style {
		ImVec4 builtin[ImGuiCol_COUNT];

		ImU32 tabTextColor;
		ImU32 tabTextPendingColor;
		ImU32 tabPendingColor;
		ImU32 tabPendingHoveredColor;

		ImU32 iconColor;
		ImU32 iconDisabledColor;

		ImU32 messageColor;
		ImU32 warningColor;
		ImU32 errorColor;
	};

public:
	BITTY_PROPERTY_PTR(Style, style)

	BITTY_PROPERTY(Style, styleDefault)

	BITTY_PROPERTY_READONLY(std::string, generic_AddFile)
	BITTY_PROPERTY_READONLY(std::string, generic_All)
	BITTY_PROPERTY_READONLY(std::string, generic_Apply)
	BITTY_PROPERTY_READONLY(std::string, generic_Binary)
	BITTY_PROPERTY_READONLY(std::string, generic_Browse)
	BITTY_PROPERTY_READONLY(std::string, generic_Cancel)
	BITTY_PROPERTY_READONLY(std::string, generic_Clear)
	BITTY_PROPERTY_READONLY(std::string, generic_Close)
	BITTY_PROPERTY_READONLY(std::string, generic_Export)
	BITTY_PROPERTY_READONLY(std::string, generic_Import)
	BITTY_PROPERTY_READONLY(std::string, generic_Install)
	BITTY_PROPERTY_READONLY(std::string, generic_List)
	BITTY_PROPERTY_READONLY(std::string, generic_No)
	BITTY_PROPERTY_READONLY(std::string, generic_None)
	BITTY_PROPERTY_READONLY(std::string, generic_Ok)
	BITTY_PROPERTY_READONLY(std::string, generic_Open)
	BITTY_PROPERTY_READONLY(std::string, generic_Ready)
	BITTY_PROPERTY_READONLY(std::string, generic_Reinstall)
	BITTY_PROPERTY_READONLY(std::string, generic_Revert)
	BITTY_PROPERTY_READONLY(std::string, generic_SaveTo)
	BITTY_PROPERTY_READONLY(std::string, generic_Tab)
	BITTY_PROPERTY_READONLY(std::string, generic_Text)
	BITTY_PROPERTY_READONLY(std::string, generic_Unknown)
	BITTY_PROPERTY_READONLY(std::string, generic_Yes)

	BITTY_PROPERTY_READONLY(std::string, menuFile)
	BITTY_PROPERTY_READONLY(std::string, menuFile_New)
	BITTY_PROPERTY_READONLY(std::string, menuFile_Open)
	BITTY_PROPERTY_READONLY(std::string, menuFile_OpenDirectory)
	BITTY_PROPERTY_READONLY(std::string, menuFile_OpenExamples)
	BITTY_PROPERTY_READONLY(std::string, menuFile_Close)
	BITTY_PROPERTY_READONLY(std::string, menuFile_SaveAsset)
	BITTY_PROPERTY_READONLY(std::string, menuFile_Save)
	BITTY_PROPERTY_READONLY(std::string, menuFile_SaveAs)
	BITTY_PROPERTY_READONLY(std::string, menuFile_SaveAsFile)
	BITTY_PROPERTY_READONLY(std::string, menuFile_SaveAsDirectory)
	BITTY_PROPERTY_READONLY(std::string, menuFile_SaveFile)
	BITTY_PROPERTY_READONLY(std::string, menuFile_SaveDirectory)
	BITTY_PROPERTY_READONLY(std::string, menuFile_Preferences)
	BITTY_PROPERTY_READONLY(std::string, menuFile_Quit)

	BITTY_PROPERTY_READONLY(std::string, menuEdit)
	BITTY_PROPERTY_READONLY(std::string, menuEdit_Undo)
	BITTY_PROPERTY_READONLY(std::string, menuEdit_Redo)
	BITTY_PROPERTY_READONLY(std::string, menuEdit_Cut)
	BITTY_PROPERTY_READONLY(std::string, menuEdit_Copy)
	BITTY_PROPERTY_READONLY(std::string, menuEdit_Paste)
	BITTY_PROPERTY_READONLY(std::string, menuEdit_Delete)
	BITTY_PROPERTY_READONLY(std::string, menuEdit_SelectAll)
	BITTY_PROPERTY_READONLY(std::string, menuEdit_IncreaseIndent)
	BITTY_PROPERTY_READONLY(std::string, menuEdit_DecreaseIndent)
	BITTY_PROPERTY_READONLY(std::string, menuEdit_Find)
	BITTY_PROPERTY_READONLY(std::string, menuEdit_FindNext)
	BITTY_PROPERTY_READONLY(std::string, menuEdit_FindPrevious)
	BITTY_PROPERTY_READONLY(std::string, menuEdit_GotoLine)
	BITTY_PROPERTY_READONLY(std::string, menuEdit_ResizeImage)
	BITTY_PROPERTY_READONLY(std::string, menuEdit_ResizeTile)
	BITTY_PROPERTY_READONLY(std::string, menuEdit_ResizeMap)
	BITTY_PROPERTY_READONLY(std::string, menuEdit_ResolveRef)

	BITTY_PROPERTY_READONLY(std::string, menuProject)
	BITTY_PROPERTY_READONLY(std::string, menuProject_Run)
	BITTY_PROPERTY_READONLY(std::string, menuProject_Stop)
	BITTY_PROPERTY_READONLY(std::string, menuProject_NewAsset)
	BITTY_PROPERTY_READONLY(std::string, menuProject_RemoveAsset)
	BITTY_PROPERTY_READONLY(std::string, menuProject_RenameAsset)
	BITTY_PROPERTY_READONLY(std::string, menuProject_FilterAssets)
	BITTY_PROPERTY_READONLY(std::string, menuProject_AddFile)
	BITTY_PROPERTY_READONLY(std::string, menuProject_Import)
	BITTY_PROPERTY_READONLY(std::string, menuProject_Export)
	BITTY_PROPERTY_READONLY(std::string, menuProject_Reload)
	BITTY_PROPERTY_READONLY(std::string, menuProject_Browse)
	BITTY_PROPERTY_READONLY(std::string, menuProject_Explore)

	BITTY_PROPERTY_READONLY(std::string, menuAsset_Code)
	BITTY_PROPERTY_READONLY(std::string, menuAsset_Sprites)
	BITTY_PROPERTY_READONLY(std::string, menuAsset_Maps)
	BITTY_PROPERTY_READONLY(std::string, menuAsset_Images)
	BITTY_PROPERTY_READONLY(std::string, menuAsset_Palettes)
	BITTY_PROPERTY_READONLY(std::string, menuAsset_Fonts)
	BITTY_PROPERTY_READONLY(std::string, menuAsset_Audio)
	BITTY_PROPERTY_READONLY(std::string, menuAsset_Json)
	BITTY_PROPERTY_READONLY(std::string, menuAsset_Text)

	BITTY_PROPERTY_READONLY(std::string, menuDebug)
	BITTY_PROPERTY_READONLY(std::string, menuDebug_Break)
	BITTY_PROPERTY_READONLY(std::string, menuDebug_Continue)
	BITTY_PROPERTY_READONLY(std::string, menuDebug_Step)
	BITTY_PROPERTY_READONLY(std::string, menuDebug_StepInto)
	BITTY_PROPERTY_READONLY(std::string, menuDebug_StepOut)
	BITTY_PROPERTY_READONLY(std::string, menuDebug_ToggleBreakpoint)

	BITTY_PROPERTY_READONLY(std::string, menuPlugins)

	BITTY_PROPERTY_READONLY(std::string, menuWindow)
	BITTY_PROPERTY_READONLY(std::string, menuWindow_Screen)
	BITTY_PROPERTY_READONLY(std::string, menuWindow_Screen_ShootCanvas)
	BITTY_PROPERTY_READONLY(std::string, menuWindow_Screen_RecordCanvas)
	BITTY_PROPERTY_READONLY(std::string, menuWindow_Screen_StopRecording)
	BITTY_PROPERTY_READONLY(std::string, menuWindow_Application)
	BITTY_PROPERTY_READONLY(std::string, menuWindow_Application_Fullscreen)
	BITTY_PROPERTY_READONLY(std::string, menuWindow_Application_Maximized)
	BITTY_PROPERTY_READONLY(std::string, menuWindow_Canvas)
	BITTY_PROPERTY_READONLY(std::string, menuWindow_Canvas_Popup)
	BITTY_PROPERTY_READONLY(std::string, menuWindow_Canvas_Frame)
	BITTY_PROPERTY_READONLY(std::string, menuWindow_Canvas_Maximized)
	BITTY_PROPERTY_READONLY(std::string, menuWindow_Buttons)
	BITTY_PROPERTY_READONLY(std::string, menuWindow_Assets)
	BITTY_PROPERTY_READONLY(std::string, menuWindow_Debug)
	BITTY_PROPERTY_READONLY(std::string, menuWindow_Console)

	BITTY_PROPERTY_READONLY(std::string, menuHelp)
	BITTY_PROPERTY_READONLY(std::string, menuHelp_Manual)
	BITTY_PROPERTY_READONLY(std::string, menuHelp_About)

	BITTY_PROPERTY_READONLY(std::string, dialogPrompt_AlreadyExists)
	BITTY_PROPERTY_READONLY(std::string, dialogPrompt_CannotAddMoreFrame)
	BITTY_PROPERTY_READONLY(std::string, dialogPrompt_CannotLoadProject)
	BITTY_PROPERTY_READONLY(std::string, dialogPrompt_CannotReadFromCurrentProject)
	BITTY_PROPERTY_READONLY(std::string, dialogPrompt_CannotSaveToReadonlyLocations)
	BITTY_PROPERTY_READONLY(std::string, dialogPrompt_CannotSaveToNonemptyDirectory)
	BITTY_PROPERTY_READONLY(std::string, dialogPrompt_CannotWriteToCurrentProject)
	BITTY_PROPERTY_READONLY(std::string, dialogPrompt_GetFullVersionToSave)
	BITTY_PROPERTY_READONLY(std::string, dialogPrompt_InvalidArea)
	BITTY_PROPERTY_READONLY(std::string, dialogPrompt_InvalidName)
	BITTY_PROPERTY_READONLY(std::string, dialogPrompt_InvalidProject)
	BITTY_PROPERTY_READONLY(std::string, dialogPrompt_InvalidRef)
	BITTY_PROPERTY_READONLY(std::string, dialogPrompt_NotUndoable)
	BITTY_PROPERTY_READONLY(std::string, dialogPrompt_Reading)
	BITTY_PROPERTY_READONLY(std::string, dialogPrompt_Running)
	BITTY_PROPERTY_READONLY(std::string, dialogPrompt_Saving)
	BITTY_PROPERTY_READONLY(std::string, dialogPrompt_UnknownType)
	BITTY_PROPERTY_READONLY(std::string, dialogPrompt_Writing)

	BITTY_PROPERTY_READONLY(std::string, dialogAsk_DidntQuitCleanlyBrowseTheLatestBackup)
	BITTY_PROPERTY_READONLY(std::string, dialogAsk_RemoveAsset)
	BITTY_PROPERTY_READONLY(std::string, dialogAsk_SaveAsset)
	BITTY_PROPERTY_READONLY(std::string, dialogAsk_SaveTheCurrentWorkspace)

	BITTY_PROPERTY_READONLY(std::string, dialogItem_ConflictAssets)
	BITTY_PROPERTY_READONLY(std::string, dialogItem_Find)
	BITTY_PROPERTY_READONLY(std::string, dialogItem_Goto)
	BITTY_PROPERTY_READONLY(std::string, dialogItem_ImageSize)
	BITTY_PROPERTY_READONLY(std::string, dialogItem_Input)
	BITTY_PROPERTY_READONLY(std::string, dialogItem_InputAnimationName)
	BITTY_PROPERTY_READONLY(std::string, dialogItem_InputAssetName)
	BITTY_PROPERTY_READONLY(std::string, dialogItem_InputInterval)
	BITTY_PROPERTY_READONLY(std::string, dialogItem_MapSize)
	BITTY_PROPERTY_READONLY(std::string, dialogItem_Palette)
	BITTY_PROPERTY_READONLY(std::string, dialogItem_Path)
	BITTY_PROPERTY_READONLY(std::string, dialogItem_Ref)
	BITTY_PROPERTY_READONLY(std::string, dialogItem_ResolveAssetRefFor)
	BITTY_PROPERTY_READONLY(std::string, dialogItem_SelectAsset)
	BITTY_PROPERTY_READONLY(std::string, dialogItem_SelectAssets)
	BITTY_PROPERTY_READONLY(std::string, dialogItem_Size)
	BITTY_PROPERTY_READONLY(std::string, dialogItem_TileSize)
	BITTY_PROPERTY_READONLY(std::string, dialogItem_TotalCount)
	BITTY_PROPERTY_READONLY(std::string, dialogItem_Type)
	BITTY_PROPERTY_READONLY(std::string, dialogItem_UnsolveAssetRefFor)
	BITTY_PROPERTY_READONLY(std::string, dialogItem_View)

	BITTY_PROPERTY_READONLY(std::string, tabCanvas)

	BITTY_PROPERTY_READONLY(std::string, tabPreferences_Editor)
	BITTY_PROPERTY_READONLY(std::string, tabPreferences_Graphics)
	BITTY_PROPERTY_READONLY(std::string, tabPreferences_Input)
	BITTY_PROPERTY_READONLY(std::string, tabPreferences_Misc)
	BITTY_PROPERTY_READONLY(std::string, tabPreferences_Onscreen)

	BITTY_PROPERTY_READONLY(std::string, windowPreferences)
	BITTY_PROPERTY_READONLY(std::string, windowPreferences_Editor_ClearOnStart)
	BITTY_PROPERTY_READONLY(std::string, windowPreferences_Editor_Console)
	BITTY_PROPERTY_READONLY(std::string, windowPreferences_Editor_IgnoreDotFiles)
	BITTY_PROPERTY_READONLY(std::string, windowPreferences_Editor_PackageFormat)
	BITTY_PROPERTY_READONLY(std::string, windowPreferences_Editor_Project)
	BITTY_PROPERTY_READONLY(std::string, windowPreferences_Editor_ShowWhiteSpaces)
	BITTY_PROPERTY_READONLY(std::string, windowPreferences_Editor_TextEditor)
	BITTY_PROPERTY_READONLY(std::string, windowPreferences_Input_ClickAgainToCancelBackspaceToClear)
	BITTY_PROPERTY_READONLY(std::string, windowPreferences_Input_ClickToChange)
	BITTY_PROPERTY_READONLY(std::string, windowPreferences_Input_Gamepads)
	BITTY_PROPERTY_READONLY(std::string, windowPreferences_Input_WaitingForInput)
	BITTY_PROPERTY_READONLY(std::string, windowPreferences_Misc_Application)
	BITTY_PROPERTY_READONLY(std::string, windowPreferences_Graphics_Application)
	BITTY_PROPERTY_READONLY(std::string, windowPreferences_Graphics_Canvas)
	BITTY_PROPERTY_READONLY(std::string, windowPreferences_Graphics_FixCanvasRatio)
	BITTY_PROPERTY_READONLY(std::string, windowPreferences_Graphics_Fullscreen)
	BITTY_PROPERTY_READONLY(std::string, windowPreferences_Misc_PauseOnFocusLost)
	BITTY_PROPERTY_READONLY(std::string, windowPreferences_NeedToReopen)
	BITTY_PROPERTY_READONLY(std::string, windowPreferences_Onscreen_Enabled)
	BITTY_PROPERTY_READONLY(std::string, windowPreferences_Onscreen_Gamepad)
	BITTY_PROPERTY_READONLY(std::string, windowPreferences_Onscreen_PaddingX)
	BITTY_PROPERTY_READONLY(std::string, windowPreferences_Onscreen_PaddingY)
	BITTY_PROPERTY_READONLY(std::string, windowPreferences_Onscreen_Scale)
	BITTY_PROPERTY_READONLY(std::string, windowPreferences_Onscreen_SwapAB)

	BITTY_PROPERTY_READONLY(std::string, windowAssets)

	BITTY_PROPERTY_READONLY(std::string, windowDebug)
	BITTY_PROPERTY_READONLY(std::string, windowDebug_Running)
	BITTY_PROPERTY_READONLY(std::string, windowDebug_CallStack)
	BITTY_PROPERTY_READONLY(std::string, windowDebug_Source)
	BITTY_PROPERTY_READONLY(std::string, windowDebug_Name)
	BITTY_PROPERTY_READONLY(std::string, windowDebug_Local)
	BITTY_PROPERTY_READONLY(std::string, windowDebug_Upvalue)
	BITTY_PROPERTY_READONLY(std::string, windowDebug_VariableName)
	BITTY_PROPERTY_READONLY(std::string, windowDebug_VariableType)
	BITTY_PROPERTY_READONLY(std::string, windowDebug_VariableValue)

	BITTY_PROPERTY_READONLY(std::string, windowPaused_Resume)
	BITTY_PROPERTY_READONLY(std::string, windowPaused_Options)
	BITTY_PROPERTY_READONLY(std::string, windowPaused_About)

	BITTY_PROPERTY_READONLY(std::string, windowCanvas)

	BITTY_PROPERTY_READONLY(std::string, windowDocument)

	BITTY_PROPERTY_READONLY(std::string, windowConsole)

	BITTY_PROPERTY_READONLY(std::string, windowAbout)

	BITTY_PROPERTY_READONLY(std::string, tooltipAssets_New)
	BITTY_PROPERTY_READONLY(std::string, tooltipAssets_Remove)

	BITTY_PROPERTY_READONLY(std::string, tooltipEditing_AddAnimation)
	BITTY_PROPERTY_READONLY(std::string, tooltipEditing_AddFrame)
	BITTY_PROPERTY_READONLY(std::string, tooltipEditing_CaseSensitive)
	BITTY_PROPERTY_READONLY(std::string, tooltipEditing_DeleteAnimation)
	BITTY_PROPERTY_READONLY(std::string, tooltipEditing_DeleteFrame)
	BITTY_PROPERTY_READONLY(std::string, tooltipEditing_InsertFrame)
	BITTY_PROPERTY_READONLY(std::string, tooltipEditing_MatchWholeWords)
	BITTY_PROPERTY_READONLY(std::string, tooltipEditing_RenameAnimation)

	BITTY_PROPERTY_READONLY(std::string, tooltipProject_InputDirSubFileToCreateInDirectory)
	BITTY_PROPERTY_READONLY(std::string, tooltipProject_DragOrDoubleClickToChange)
	BITTY_PROPERTY_READONLY(std::string, tooltipProject_OptionalSelectAPaletteNoneForTrueColor)
	BITTY_PROPERTY_READONLY(std::string, tooltipProject_SelectAnImage)

	BITTY_PROPERTY_READONLY(std::string, tooltipDebug_Break)
	BITTY_PROPERTY_READONLY(std::string, tooltipDebug_Clear)
	BITTY_PROPERTY_READONLY(std::string, tooltipDebug_Continue)
	BITTY_PROPERTY_READONLY(std::string, tooltipDebug_Disable)
	BITTY_PROPERTY_READONLY(std::string, tooltipDebug_Enable)
	BITTY_PROPERTY_READONLY(std::string, tooltipDebug_Step)
	BITTY_PROPERTY_READONLY(std::string, tooltipDebug_StepInto)
	BITTY_PROPERTY_READONLY(std::string, tooltipDebug_StepOut)

	BITTY_PROPERTY_READONLY(std::string, statusTip_Readonly)

	BITTY_PROPERTY_READONLY(std::string, statusItem_Area)
	BITTY_PROPERTY_READONLY(std::string, statusItem_Col)
	BITTY_PROPERTY_READONLY(std::string, statusItem_Ln)
	BITTY_PROPERTY_READONLY(std::string, statusItem_Index)
	BITTY_PROPERTY_READONLY(std::string, statusItem_Pos)

	BITTY_PROPERTY_READONLY(std::string, statusItemSound_Title)
	BITTY_PROPERTY_READONLY(std::string, statusItemSound_Artist)
	BITTY_PROPERTY_READONLY(std::string, statusItemSound_Album)
	BITTY_PROPERTY_READONLY(std::string, statusItemSound_Copyright)

	BITTY_PROPERTY_PTR(class Texture, iconPlay)
	BITTY_PROPERTY_PTR(class Texture, iconPlay_Gray)
	BITTY_PROPERTY_PTR(class Texture, iconPause)
	BITTY_PROPERTY_PTR(class Texture, iconStop)

	BITTY_PROPERTY_PTR(class Texture, iconCopy)
	BITTY_PROPERTY_PTR(class Texture, iconCopy_Gray)
	BITTY_PROPERTY_PTR(class Texture, iconCut)
	BITTY_PROPERTY_PTR(class Texture, iconCut_Gray)
	BITTY_PROPERTY_PTR(class Texture, iconPaste)
	BITTY_PROPERTY_PTR(class Texture, iconPaste_Gray)

	BITTY_PROPERTY_PTR(class Texture, iconUndo)
	BITTY_PROPERTY_PTR(class Texture, iconUndo_Gray)
	BITTY_PROPERTY_PTR(class Texture, iconRedo)
	BITTY_PROPERTY_PTR(class Texture, iconRedo_Gray)

	BITTY_PROPERTY_PTR(class Texture, iconMove)
	BITTY_PROPERTY_PTR(class Texture, iconEyedropper)
	BITTY_PROPERTY_PTR(class Texture, iconPencil)
	BITTY_PROPERTY_PTR(class Texture, iconPaintbucket)
	BITTY_PROPERTY_PTR(class Texture, iconLasso)
	BITTY_PROPERTY_PTR(class Texture, iconLine)
	BITTY_PROPERTY_PTR(class Texture, iconBox)
	BITTY_PROPERTY_PTR(class Texture, iconBox_Fill)
	BITTY_PROPERTY_PTR(class Texture, iconEllipse)
	BITTY_PROPERTY_PTR(class Texture, iconEllipse_Fill)
	BITTY_PROPERTY_PTR(class Texture, iconStamp)

	BITTY_PROPERTY_PTR(class Texture, iconClock)

	BITTY_PROPERTY_PTR(class Texture, iconMagnify)

	BITTY_PROPERTY_PTR(class Texture, iconPencils)

	BITTY_PROPERTY_PTR(class Texture, iconRotate_Clockwise)
	BITTY_PROPERTY_PTR(class Texture, iconRotate_Anticlockwise)
	BITTY_PROPERTY_PTR(class Texture, iconRotate_HalfTurn)
	BITTY_PROPERTY_PTR(class Texture, iconFlip_Vertically)
	BITTY_PROPERTY_PTR(class Texture, iconFlip_Horizontally)

	BITTY_PROPERTY_PTR(class Texture, iconTransparent)

	BITTY_PROPERTY_PTR(class Texture, sliceDirectory)
	BITTY_PROPERTY_PTR(class Texture, sliceDirectory_Open)
	BITTY_PROPERTY_PTR(class Texture, sliceFile)

	BITTY_PROPERTY_PTR(class Texture, sliceFilter)
	BITTY_PROPERTY_PTR(class Texture, sliceRecycle)

	BITTY_PROPERTY_PTR(class Texture, slicePrevious)
	BITTY_PROPERTY_PTR(class Texture, sliceNext)

	BITTY_PROPERTY_PTR(class Texture, sliceCaseSensitive)
	BITTY_PROPERTY_PTR(class Texture, sliceWholeWord)

	BITTY_PROPERTY_PTR(class Texture, slicePlus)
	BITTY_PROPERTY_PTR(class Texture, sliceMinus)

	BITTY_PROPERTY_PTR(class Texture, slicePlay)
	BITTY_PROPERTY_PTR(class Texture, sliceStop)
	BITTY_PROPERTY_PTR(class Texture, slicePause)
	BITTY_PROPERTY_PTR(class Texture, sliceBreakpoints_Disable)
	BITTY_PROPERTY_PTR(class Texture, sliceBreakpoints_Enable)
	BITTY_PROPERTY_PTR(class Texture, sliceBreakpoints_Clear)
	BITTY_PROPERTY_PTR(class Texture, sliceStep)
	BITTY_PROPERTY_PTR(class Texture, sliceStep_Into)
	BITTY_PROPERTY_PTR(class Texture, sliceStep_Out)

	BITTY_PROPERTY_PTR(class Texture, sliceNumber_1)
	BITTY_PROPERTY_PTR(class Texture, sliceNumber_2)
	BITTY_PROPERTY_PTR(class Texture, sliceNumber_3)
	BITTY_PROPERTY_PTR(class Texture, sliceNumber_4)

	BITTY_PROPERTY_PTR(class Texture, imagePadPortraitTop)
	BITTY_PROPERTY_PTR(class Texture, imagePadPortraitBottom)
	BITTY_PROPERTY_PTR(class Texture, imagePadLandscapeLeft)
	BITTY_PROPERTY_PTR(class Texture, imagePadLandscapeRight)

	BITTY_PROPERTY_PTR(struct ImFont, fontCode)
	BITTY_PROPERTY_PTR(struct ImFont, fontBlock)
	BITTY_PROPERTY_PTR(struct ImFont, fontBlock_Bold)
	BITTY_PROPERTY_PTR(struct ImFont, fontBlock_Italic)
	BITTY_PROPERTY_PTR(struct ImFont, fontBlock_BoldItalic)

private:
	bool _opened = false;

public:
	Theme();
	virtual ~Theme();

	virtual Styles styleIndex(void) const = 0;
	virtual void styleIndex(Styles idx) = 0;

	virtual bool open(class Renderer* rnd);
	virtual bool close(class Renderer* rnd);

	virtual bool load(class Renderer* rnd);
	virtual bool save(void) const;

	static class Texture* createTexture(class Renderer* rnd, const Byte* buf, size_t len);
	static class Texture* createTexture(class Renderer* rnd, const char* path);
	static void destroyTexture(class Renderer* rnd, class Texture* &tex);

protected:
	virtual void setColor(const std::string &key, ImGuiCol idx, const ImColor &col);

private:
	void fromFile(const char* path);
};

/* ===========================================================================} */

#endif /* __THEME_H__ */
