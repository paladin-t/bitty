#ifndef __IMGUI_CODE_EDIT__
#define __IMGUI_CODE_EDIT__

#include "../imgui/imgui.h"
#include <array>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <regex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#ifndef ICE_UTF_CHAR_WIDTH
#	define ICE_UTF_CHAR_WIDTH 2
#endif /* ICE_UTF_CHAR_WIDTH */

#ifndef ICE_MERGE_UNDO_REDO
#	define ICE_MERGE_UNDO_REDO 1
#endif /* ICE_MERGE_UNDO_REDO */

#ifndef ICE_CASE_FUNC
#	define ICE_CASE_FUNC ::tolower
#endif /* ICE_CASE_FUNC */

#ifndef ICE_NEW_ARRAY
#	define ICE_NEW_ARRAY(T, S) (new T[S])
#endif /* ICE_NEW_ARRAY */
#ifndef ICE_DELETE_ARRAY
#	define ICE_DELETE_ARRAY(A) (delete [] (A))
#endif /* ICE_DELETE_ARRAY */

#ifndef countof
#	define countof(A) (sizeof(A) / sizeof(*(A)))
#endif /* countof */

namespace ImGui {

class CodeEditor {

public:
	enum class PaletteIndex : uint8_t {
		Default,
		Keyword,
		Number,
		String,
		CharLiteral,
		Punctuation,
		Preprocessor,
		Symbol,
		Identifier,
		KnownIdentifier,
		PreprocIdentifier,
		Comment,
		MultiLineComment,
		Space,
		Background,
		Cursor,
		Selection,
		ErrorMarker,
		WarningMarker,
		Breakpoint,
		ProgramPointer,
		LineNumber,
		CurrentLineFill,
		CurrentLineFillInactive,
		CurrentLineEdge,
		LineEdited,
		LineEditedSaved,
		LineEditedReverted,
		Max
	};

	enum ShortcutType {
		UndoRedo            = 1 << 0,
		CopyCutPasteDelete  = 1 << 1,
		BackSpaceOrWordwise = 1 << 2,
		IndentUnindent      = 1 << 3,
		ToUpperOrLowerCase  = 1 << 4,
		All                 = UndoRedo | CopyCutPasteDelete | BackSpaceOrWordwise | IndentUnindent | ToUpperOrLowerCase
	};

	struct Coordinates {
		int Line = 0, Column = 0;

		Coordinates() {
		}
		Coordinates(int aLine, int aColumn) : Line(aLine), Column(aColumn) {
			assert(aLine >= 0);
			assert(aColumn >= 0);
		}
		static Coordinates Invalid(void) {
			static Coordinates invalid(-1, -1);
			
			return invalid;
		}

		bool operator == (const Coordinates &o) const {
			return Line == o.Line && Column == o.Column;
		}
		bool operator != (const Coordinates &o) const {
			return Line != o.Line || Column != o.Column;
		}
		bool operator < (const Coordinates &o) const {
			if (Line != o.Line)
				return Line < o.Line;

			return Column < o.Column;
		}
		bool operator > (const Coordinates &o) const {
			if (Line != o.Line)
				return Line > o.Line;

			return Column > o.Column;
		}
		bool operator <= (const Coordinates &o) const {
			if (Line != o.Line)
				return Line < o.Line;

			return Column <= o.Column;
		}
		bool operator >= (const Coordinates &o) const {
			if (Line != o.Line)
				return Line > o.Line;

			return Column >= o.Column;
		}
	};

	struct Identifier {
		Coordinates Location;
		std::string Declaration;
	};
	typedef std::unordered_map<std::string, Identifier> Identifiers;
	typedef std::unordered_set<std::string> Keywords;

	struct Error {
		std::string message;
		bool isWarning = false;
		bool withLineNumber = false;

		Error();
		Error(const std::string &msg, bool isWarning_, bool withLineNumber_);
	};
	typedef std::map<int, Error> ErrorMarkers;
	typedef std::unordered_map<int, bool> Breakpoints;

	typedef std::array<ImU32, (size_t)PaletteIndex::Max> Palette;

	typedef unsigned Char; // UTF-8.
	struct Glyph {
		ImWchar Codepoint = 0;
		Char Character = 0;
		ImU32 ColorIndex = (ImU32)PaletteIndex::Default; // Either a palette index or a 32bit color value.
		bool MultiLineComment;
		int Width = 0;

		Glyph(Char aChar, ImU32 aColorIndex);
		Glyph(Char aChar, PaletteIndex aColorIndex);
	};

	enum class LineState : uint8_t {
		None,
		Edited,
		EditedSaved,
		EditedReverted
	};

	struct Line {
		std::vector<Glyph> Glyphs;
		LineState Changed = LineState::None;

		void Clear(void);
		void Change(void);
		void Save(void);
		void Revert(void);
	};
	typedef std::vector<Line> Lines;

	struct LanguageDefinition {
		typedef std::pair<std::string, PaletteIndex> TokenRegexString;
		typedef std::vector<TokenRegexString> TokenRegexStrings;
		typedef std::vector<std::pair<Char, Char> > RangedCharPairs;

		typedef std::function<bool(const char*, const char*, const char* &, const char* &, PaletteIndex &)> TokenizeCallback;

		std::string Name;
		Keywords Keys;
		Keywords Symbols;
		Identifiers Ids;
		Identifiers PreprocIds;
		std::string CommentStart, CommentEnd;
		Char CommentException = '\0';
		std::string SimpleCommentHead;

		TokenizeCallback Tokenize;

		TokenRegexStrings TokenRegexPatterns;

		RangedCharPairs RangedCharPatterns;

		bool CaseSensitive;

		static LanguageDefinition Text(void);
		static LanguageDefinition Json(void);
		static LanguageDefinition AngelScript(void);
		static LanguageDefinition C(void);
		static LanguageDefinition CPlusPlus(void);
		static LanguageDefinition GLSL(void);
		static LanguageDefinition HLSL(void);
		static LanguageDefinition Lua(void);
		static LanguageDefinition SQL(void);
	};

	typedef std::function<bool(ImGuiKey)> KeyPressed;

	typedef std::function<void(bool)> Colorized;
	typedef std::function<void(bool)> Modified;
	typedef std::function<void(int, bool)> HeadClicked;
	typedef std::function<void(int, bool)> LineClicked;

	CodeEditor();
	virtual ~CodeEditor();

	LanguageDefinition &SetLanguageDefinition(const LanguageDefinition &aLanguageDef);
	const LanguageDefinition &GetLanguageDefinition(void) const;
	LanguageDefinition &GetLanguageDefinition(void);

	void SetFont(const ImFont* font);
	const ImFont* GetFont(void) const;

	void SetPalette(const Palette &aValue);
	const Palette &GetPalette(void) const;

	void SetErrorMarkers(const ErrorMarkers &aMarkers);
	void ClearErrorMarkers(void);
	void SetBreakpoints(const Breakpoints &aMarkers);
	void ClearBreakpoints(void);
	void SetProgramPointer(int aPointer);
	int GetProgramPointer(void);

	void Render(const char* aTitle, const ImVec2 &aSize = ImVec2(), bool aBorder = false);

	void SetKeyPressedHandler(const KeyPressed &aHandler);
	void SetColorizedHandler(const Colorized &aHandler);
	void SetModifiedHandler(const Modified &aHandler);
	void SetHeadClickedHandler(const HeadClicked &aHandler);
	void SetLineClickedHandler(const LineClicked &aHandler);
	bool IsChangesSaved(void) const;
	void SetChangesCleared(void);
	void SetChangesSaved(void);

	void SetText(const std::string &aText);
	std::string GetText(const char* aNewline = "\n") const;
	std::vector<std::string> GetTextLines(bool aIncludeComment, bool aIncludeString) const;
	std::string GetTextLine(int aLine) const;
	std::vector<std::pair<std::string, PaletteIndex> > GetWordsAtLine(int aLine, bool aIncludeComment, bool aIncludeString, bool aIncludeSpace) const;

	void InsertText(const char* aValue);
	void AppendText(const char* aText, ImU32 aColor = (ImU32)PaletteIndex::Default);

	int GetTotalLines(void) const;
	int GetColumnsAt(int aLine) const;
	int GetTotalTokens(void) const;

	void SetCursorPosition(const Coordinates &aPosition);
	Coordinates GetCursorPosition(void) const;
	void EnsureCursorVisible(bool aForceAbove = false, bool aSlowMode = false);
	float GetScrollPositionY(void);
	void SetScrollPositionY(float val);

	void SetIndentWithTab(bool aValue);
	bool GetIndentWithTab(void) const;

	void SetTabSize(int aValue);
	int GetTabSize(void) const;

	void SetHeadSize(float aValue);
	float GetHeadSize(void) const;

	void SetOverwrite(bool aValue);
	bool IsOverwrite(void) const;

	void SetReadOnly(bool aValue);
	bool IsReadOnly(void) const;

	void SetShowLineNumbers(bool aValue);
	bool IsShowLineNumbers(void) const;

	void SetStickyLineNumbers(bool aValue);
	bool IsStickyLineNumbers(void) const;

	void SetHeadClickEnabled(bool aValue);
	bool IsHeadClickEnabled(void) const;

	void EnableShortcut(ShortcutType aType);
	void DisableShortcut(ShortcutType aType);
	bool IsShortcutsEnabled(ShortcutType aType) const;

	void SetErrorTipEnabled(bool aValue);
	bool IsErrorTipEnabled(void) const;

	void SetTooltipEnabled(bool aValue);
	bool IsTooltipEnabled(void) const;

	void SetShowWhiteSpaces(bool aValue);
	bool IsShowWhiteSpaces(void) const;

	void SetSafeColumnIndicatorOffset(int aValue);
	int GetSafeColumnIndicatorOffset(void) const;

	bool IsEditorFocused(void) const;

	void MoveUp(int aAmount = 1, bool aSelect = false);
	void MoveDown(int aAmount = 1, bool aSelect = false);
	void MoveLeft(int aAmount = 1, bool aSelect = false, bool aWordMode = false);
	void MoveRight(int aAmount = 1, bool aSelect = false, bool aWordMode = false);
	void MoveTop(bool aSelect = false);
	void MoveBottom(bool aSelect = false);
	void MoveHome(bool aSelect = false);
	void MoveEnd(bool aSelect = false);

	std::string GetWordUnderCursor(Coordinates* aStart = nullptr, Coordinates* aEnd = nullptr) const;
	void SetSelectionStart(const Coordinates &aPosition);
	void SetSelectionEnd(const Coordinates &aPosition);
	void SetSelection(const Coordinates &aStart, const Coordinates &aEnd, bool aWordMode = false);
	void SelectWordUnderCursor(void);
	void SelectWordUnderMouse(void);
	void SelectAll(void);
	bool HasSelection(void) const;
	void ClearSelection(void);
	void GetSelection(Coordinates &aStart, Coordinates &aEnd);
	std::string GetSelectionText(const char* aNewline = "\n") const;
	int GetSelectionLines(void) const;
	int GetNonEmptySelectionLines(void) const;
	int GetCommentLines(void) const;

	void Copy(void);
	void Cut(void);
	void Paste(void);
	void Paste(const char* aTxt);
	void Delete(void);
	void DeleteLine(void);
	void Indent(bool aByKey = true);
	void Unindent(bool aByKey = true);
	void ToLowerCase(void);
	void ToUpperCase(void);
	void Comment(void);
	void Uncomment(void);
	void MoveLineUp(void);
	void MoveLineDown(void);

	void ClearUndoRedoStack(void);
	bool CanUndo(void) const;
	bool CanRedo(void) const;
	void Undo(int aSteps = 1);
	void Redo(int aSteps = 1);

	void SyncTo(CodeEditor* aOther);

	static const Palette &GetDarkPalette(void);
	static const Palette &GetLightPalette(void);
	static const Palette &GetRetroBluePalette(void);

protected:
	typedef std::vector<std::pair<std::regex, PaletteIndex> > RegexList;

	struct EditorState {
		Coordinates SelectionStart;
		Coordinates SelectionEnd;
		Coordinates CursorPosition;
	};

	enum class UndoType : uint8_t {
		Add,
		Remove,
		Indent,
		Unindent,
		ToLowerCase,
		ToUpperCase,
		Comment,
		Uncomment,
		MoveLineUp,
		MoveLineDown
	};

	struct UndoRecord {
		UndoRecord() {
		}
		~UndoRecord() {
		}

		bool Similar(const UndoRecord* o) const;

		void Undo(CodeEditor* aEditor);
		void Redo(CodeEditor* aEditor);

		UndoType Type;

		std::string Overwritten;
		std::string Content;
		Coordinates Start;
		Coordinates End;

		EditorState Before;
		EditorState After;
	};

	typedef std::vector<UndoRecord> UndoBuffer;

	void RenderText(int &aOffset, const ImVec2 &aPosition, ImU32 aPalette, ImU32 aColor, const char* aText, const std::list<Glyph> &aGlyphs, int aWidth);
	void Colorize(int aFromLine = 0, int aCount = -1);
	void ColorizeRange(int aFromLine = 0, int aToLine = 0);
	bool ColorizeMultilineComments(void);
	void ColorizeInternal(void);
	int TextDistanceToLineStart(const Coordinates &aFrom) const;
	int GetPageSize(void) const;
	Coordinates GetActualCursorCoordinates(void) const;
	Coordinates SanitizeCoordinates(const Coordinates &aValue) const;
	void Advance(Coordinates &aCoordinates) const;
	int GetCharacterWidth(const Glyph &aGlyph) const;
	Coordinates ScreenPosToCoordinates(const ImVec2 &aPosition) const;
	bool IsOnWordBoundary(const Coordinates &aAt) const;
	void AddUndo(UndoRecord &aValue);
	std::string GetText(const Coordinates &aStart, const Coordinates &aEnd, const char* aNewline) const;
	int AppendBuffer(std::string &aBuffer, std::list<Glyph> &aGlyphs, Glyph &aGlyph, int aIndex, int &aWidth);
	int InsertTextAt(Coordinates &aWhere, const char* aValue);
	void DeleteRange(const Coordinates &aStart, const Coordinates &aEnd);
	void DeleteSelection(void);
	Line &InsertLine(int aIndex);
	void RemoveLine(int aStart, int aEnd);
	void RemoveLine(int aIndex);
	void BackSpace(void);
	void BackSpaceWordwise(void);
	void EnterCharacter(Char aChar);
	Coordinates FindWordStart(const Coordinates &aFrom) const;
	Coordinates FindWordEnd(const Coordinates &aFrom) const;
	std::string GetWordAt(const Coordinates &aCoords, Coordinates* aStart = nullptr, Coordinates* aEnd = nullptr) const;
	Char GetCharAt(const Coordinates &aCoords) const;
	Char GetCharUnderCursor(void) const;
	const LanguageDefinition::RangedCharPairs::value_type* FindRangedCharPairStart(Char aChar) const;
	const LanguageDefinition::RangedCharPairs::value_type* FindRangedCharPairEnd(Char aChar) const;
	void OnChanged(const Coordinates &aStart, const Coordinates &aEnd, int aOffset, bool aClearInputtedRangedPair);
	bool OnKeyPressed(ImGuiKey aKey);
	void OnColorized(bool aMultilineComment) const;
	void OnModified(bool aNewLine, bool aClearAutoIndent) const;
	void OnHeadClicked(int aLine, bool aDoubleClicked) const;
	void OnLineClicked(int aLine, bool aDoubleClicked) const;

	Lines CodeLines;
	float LineSpacing;
	EditorState State;
	UndoBuffer UndoBuf;
	int UndoIndex;
	int SavedIndex;
	mutable struct AutoIndentRecord {
		bool hasValue = false;
		UndoRecord record;

		AutoIndentRecord() {
		}
		AutoIndentRecord(const UndoRecord &aRecord) : record(aRecord) {
			hasValue = true;
		}

		void clear(void) {
			hasValue = false;
			record = UndoRecord();
		}
	} LastAutoIndent;
	KeyPressed KeyPressedHandler;
	Colorized ColorizedHandler;
	Modified ModifiedHandler;
	HeadClicked HeadClickedHandler;
	LineClicked LineClickedHandler;

	const ImFont* Font;
	ImVector<ImWchar> InputBuffer;
	ImVec2 CharAdv;
	bool IndentWithTab;
	int TabSize;
	int TextStart;
	float HeadSize;
	bool Overwrite;
	bool ReadOnly;
	bool ShowLineNumbers;
	bool StickyLineNumbers;
	bool HeadClickEnabled;
	ShortcutType ShortcutsEnabled;
	bool WithinRender;
	int ScrollToCursor;
	float ScrollY;
	bool ToSetScrollY;
	bool WordSelectionMode;
	int ColorRangeMin, ColorRangeMax;
	std::string LastSymbol;
	PaletteIndex LastSymbolPalette;
	bool InputtedRangedPair;
	int CheckMultilineComments;
	bool ErrorTipEnabled;
	bool TooltipEnabled;
	bool ShowWhiteSpaces;
	int SafeColumnIndicatorOffset;
	ImVec2 CursorScreenPos;
	bool EditorFocused;

	Breakpoints Brks;
	ErrorMarkers Errs;
	int ProgramPointer;
	Coordinates InteractiveStart, InteractiveEnd;

	Palette Plt;
	LanguageDefinition LangDef;
	RegexList Regexes;

};

}

#endif /* __IMGUI_CODE_EDIT__ */
