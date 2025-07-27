#include "imgui_code_editor.h"
#include "../imgui/imgui_internal.h"
#include <SDL.h>
#include <algorithm>
#include <chrono>
#include <regex>

namespace ImGui {

static const int COLORIZE_DELAY_FRAME_COUNT = 60;

static std::string ImTextStrToUtf8StdStr(const ImWchar* inText, const ImWchar* inTextEnd) {
	int sz = ImTextCountUtf8BytesFromStr(inText, inTextEnd);
	std::string result;
	result.resize((size_t)(sz + 1));
	ImTextStrToUtf8(&(*result.begin()), (int)result.length(), inText, inTextEnd);

	return result;
}

static int ImTextExpectUtf8Char(const char* ch) {
#define _TAKE(__ch, __c, __r) do { __c = *__ch++; __r++; } while(0)
#define _COPY(__ch, __c, __r, __cp) do { _TAKE(__ch, __c, __r); __cp = (__cp << 6) | ((unsigned char)__c & 0x3fu); } while(0)
#define _TRANS(__m, __cp, __g) do { __cp &= ((__g[(unsigned char)c] & __m) != 0); } while(0)
#define _TAIL(__ch, __c, __r, __cp, __g) do { _COPY(__ch, __c, __r, __cp); _TRANS(0x70, __cp, __g); } while(0)

	static constexpr const unsigned char RANGE[] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
		0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40,
		0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
		0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20,
		8, 8, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
		10, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 3, 3, 11, 6, 6, 6, 5, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8
	};

	int result = 0;
	unsigned codepoint = 0;
	unsigned char type = 0;
	char c = 0;

	if (!ch)
		return 0;

	_TAKE(ch, c, result);
	if (!(c & 0x80)) {
		codepoint = (unsigned char)c;

		return 1;
	}

	type = RANGE[(unsigned char)c];
	codepoint = (0xff >> type) & (unsigned char)c;

	switch (type) {
	case 2: _TAIL(ch, c, result, codepoint, RANGE); return result;
	case 3: _TAIL(ch, c, result, codepoint, RANGE); _TAIL(ch, c, result, codepoint, RANGE); return result;
	case 4: _COPY(ch, c, result, codepoint); _TRANS(0x50, codepoint, RANGE); _TAIL(ch, c, result, codepoint, RANGE); return result;
	case 5: _COPY(ch, c, result, codepoint); _TRANS(0x10, codepoint, RANGE); _TAIL(ch, c, result, codepoint, RANGE); _TAIL(ch, c, result, codepoint, RANGE); return result;
	case 6: _TAIL(ch, c, result, codepoint, RANGE); _TAIL(ch, c, result, codepoint, RANGE); _TAIL(ch, c, result, codepoint, RANGE); return result;
	case 10: _COPY(ch, c, result, codepoint); _TRANS(0x20, codepoint, RANGE); _TAIL(ch, c, result, codepoint, RANGE); return result;
	case 11: _COPY(ch, c, result, codepoint); _TRANS(0x60, codepoint, RANGE); _TAIL(ch, c, result, codepoint, RANGE); _TAIL(ch, c, result, codepoint, RANGE); return result;
	default: return 0;
	}

#undef _TAKE
#undef _COPY
#undef _TRANS
#undef _TAIL
}

static CodeEditor::Char ImTextTakeUtf8Bytes(const char* str, int n) {
	union { CodeEditor::Char ui; char ch[4]; } u;
	u.ui = 0;
	for (int i = 0; i < n; ++i)
		u.ch[i] = str[i];
	for (int i = n; i < 4; ++i)
		u.ch[i] = '\0';

	return u.ui;
}

static int ImTextCountUtf8Bytes(CodeEditor::Char chr) {
	int ret = 0;
	union { CodeEditor::Char ui; char ch[4]; } u;
	u.ui = chr;
	for (int i = 0; i < 4; ++i) {
		if (u.ch[i])
			ret = i + 1;
		else
			break;
	}

	return ret;
}

static int ImTextAppendUtf8ToStdStr(std::string &buf, CodeEditor::Char chr) {
	int ret = 0;
	union { CodeEditor::Char ui; char ch[4]; } u;
	u.ui = chr;
	for (int i = 0; i < 4; ++i) {
		if (u.ch[i]) {
			buf.push_back(u.ch[i]);

			ret = i + 1;
		} else {
			break;
		}
	}

	return ret;
}

static size_t ImTextToLowerCase(std::string &str) {
	size_t i = 0;
	while (i < str.length()) {
		int n = ImTextExpectUtf8Char(str.c_str() + i);
		if (n == 0)
			break;
		else if (n == 1)
			str[i] = (char)::tolower(str[i]);
		i += n;
	}

	return i;
}

static size_t ImTextToUpperCase(std::string &str) {
	size_t i = 0;
	while (i < str.length()) {
		int n = ImTextExpectUtf8Char(str.c_str() + i);
		if (n == 0)
			break;
		else if (n == 1)
			str[i] = (char)::toupper(str[i]);
		i += n;
	}

	return i;
}

CodeEditor::LanguageDefinition CodeEditor::LanguageDefinition::Text(void) {
	static bool inited = false;
	static LanguageDefinition langDef;
	if (!inited) {
		langDef.CaseSensitive = true;

		langDef.Name = "Text";

		inited = true;
	}

	return langDef;
}

CodeEditor::LanguageDefinition CodeEditor::LanguageDefinition::Json(void) {
	static bool inited = false;
	static LanguageDefinition langDef;
	if (!inited) {
		static const char* const keywords[] = {
			"false", "true", "null"
		};
		for (const char* const k : keywords)
			langDef.Keys.insert(k);

		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("L?\\\"(\\\\.|[^\\\"])*\\\"", PaletteIndex::String));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][+-]?[0-9]+)?[fF]?", PaletteIndex::Number));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("[+-]?[0-9]+[Uu]?[lL]?[lL]?", PaletteIndex::Number));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("0[0-7]+[Uu]?[lL]?[lL]?", PaletteIndex::Number));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("0[xX][0-9a-fA-F]+[uU]?[lL]?[lL]?", PaletteIndex::Number));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("[a-zA-Z_][a-zA-Z0-9_]*", PaletteIndex::Identifier));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("[\\[\\]\\{\\}\\-\\+\\:\\,]", PaletteIndex::Punctuation));

		langDef.CaseSensitive = true;

		langDef.Name = "JSON";

		inited = true;
	}

	return langDef;
}

CodeEditor::LanguageDefinition CodeEditor::LanguageDefinition::AngelScript(void) {
	static bool inited = false;
	static LanguageDefinition langDef;
	if (!inited) {
		static const char* const keywords[] = {
			"and", "abstract", "auto", "bool", "break", "case", "cast", "class", "const", "continue", "default", "do", "double", "else", "enum", "false", "final", "float", "for",
			"from", "funcdef", "function", "get", "if", "import", "in", "inout", "int", "interface", "int8", "int16", "int32", "int64", "is", "mixin", "namespace", "not",
			"null", "or", "out", "override", "private", "protected", "return", "set", "shared", "super", "switch", "this ", "true", "typedef", "uint", "uint8", "uint16", "uint32",
			"uint64", "void", "while", "xor"
		};

		for (const char* const k : keywords)
			langDef.Keys.insert(k);

		static const char* const identifiers[] = {
			"cos", "sin", "tab", "acos", "asin", "atan", "atan2", "cosh", "sinh", "tanh", "log", "log10", "pow", "sqrt", "abs", "ceil", "floor", "fraction", "closeTo", "fpFromIEEE", "fpToIEEE",
			"complex", "opEquals", "opAddAssign", "opSubAssign", "opMulAssign", "opDivAssign", "opAdd", "opSub", "opMul", "opDiv"
		};
		for (const char* const k : identifiers) {
			Identifier id;
			id.Declaration = "Built-in function";
			langDef.Ids.insert(std::make_pair(std::string(k), id));
		}

		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("//.*", PaletteIndex::Comment));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("L?\\\"(\\\\.|[^\\\"])*\\\"", PaletteIndex::String));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("\\'\\\\?[^\\']\\'", PaletteIndex::String));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][+-]?[0-9]+)?[fF]?", PaletteIndex::Number));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("[+-]?[0-9]+[Uu]?[lL]?[lL]?", PaletteIndex::Number));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("0[0-7]+[Uu]?[lL]?[lL]?", PaletteIndex::Number));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("0[xX][0-9a-fA-F]+[uU]?[lL]?[lL]?", PaletteIndex::Number));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("[a-zA-Z_][a-zA-Z0-9_]*", PaletteIndex::Identifier));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("[\\[\\]\\{\\}\\!\\%\\^\\&\\*\\(\\)\\-\\+\\=\\~\\|\\<\\>\\?\\/\\;\\,\\.]", PaletteIndex::Punctuation));

		langDef.CommentStart = "/*";
		langDef.CommentEnd = "*/";
		langDef.SimpleCommentHead = "//";

		langDef.CaseSensitive = true;

		langDef.Name = "AngelScript";

		inited = true;
	}

	return langDef;
}

CodeEditor::LanguageDefinition CodeEditor::LanguageDefinition::C(void) {
	static bool inited = false;
	static LanguageDefinition langDef;
	if (!inited) {
		static const char* const keywords[] = {
			"auto", "break", "case", "char", "const", "continue", "default", "do", "double", "else", "enum", "extern", "float", "for", "goto", "if", "inline", "int", "long", "register", "restrict", "return", "short",
			"signed", "sizeof", "static", "struct", "switch", "typedef", "union", "unsigned", "void", "volatile", "while", "_Alignas", "_Alignof", "_Atomic", "_Bool", "_Complex", "_Generic", "_Imaginary",
			"_Noreturn", "_Static_assert", "_Thread_local"
		};
		for (const char* const k : keywords)
			langDef.Keys.insert(k);

		static const char* const identifiers[] = {
			"abort", "abs", "acos", "asin", "atan", "atexit", "atof", "atoi", "atol", "ceil", "clock", "cosh", "ctime", "div", "exit", "fabs", "floor", "fmod", "getchar", "getenv", "isalnum", "isalpha", "isdigit", "isgraph",
			"ispunct", "isspace", "isupper", "kbhit", "log10", "log2", "log", "memcmp", "modf", "pow", "putchar", "putenv", "puts", "rand", "remove", "rename", "sinh", "sqrt", "srand", "strcat", "strcmp", "strerror", "time", "tolower", "toupper"
		};
		for (const char* const k : identifiers) {
			Identifier id;
			id.Declaration = "Built-in function";
			langDef.Ids.insert(std::make_pair(std::string(k), id));
		}

		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("//.*", PaletteIndex::Comment));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("[ \t]*#[ \\t]*[a-zA-Z_]+", PaletteIndex::Preprocessor));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("L?\\\"(\\\\.|[^\\\"])*\\\"", PaletteIndex::String));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("\\'\\\\?[^\\']\\'", PaletteIndex::CharLiteral));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][+-]?[0-9]+)?[fF]?", PaletteIndex::Number));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("[+-]?[0-9]+[Uu]?[lL]?[lL]?", PaletteIndex::Number));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("0[0-7]+[Uu]?[lL]?[lL]?", PaletteIndex::Number));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("0[xX][0-9a-fA-F]+[uU]?[lL]?[lL]?", PaletteIndex::Number));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("[a-zA-Z_][a-zA-Z0-9_]*", PaletteIndex::Identifier));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("[\\[\\]\\{\\}\\!\\%\\^\\&\\*\\(\\)\\-\\+\\=\\~\\|\\<\\>\\?\\/\\;\\,\\.]", PaletteIndex::Punctuation));

		langDef.CommentStart = "/*";
		langDef.CommentEnd = "*/";
		langDef.SimpleCommentHead = "//";

		langDef.CaseSensitive = true;

		langDef.Name = "C";

		inited = true;
	}

	return langDef;
}

CodeEditor::LanguageDefinition CodeEditor::LanguageDefinition::CPlusPlus(void) {
	static bool inited = false;
	static LanguageDefinition langDef;
	if (!inited) {
		static const char* const cppKeywords[] = {
			"alignas", "alignof", "and", "and_eq", "asm", "atomic_cancel", "atomic_commit", "atomic_noexcept", "auto", "bitand", "bitor", "bool", "break", "case", "catch", "char", "char16_t", "char32_t", "class",
			"compl", "concept", "const", "constexpr", "const_cast", "continue", "decltype", "default", "delete", "do", "double", "dynamic_cast", "else", "enum", "explicit", "export", "extern", "false", "float",
			"for", "friend", "goto", "if", "import", "inline", "int", "long", "module", "mutable", "namespace", "new", "noexcept", "not", "not_eq", "nullptr", "operator", "or", "or_eq", "private", "protected", "public",
			"register", "reinterpret_cast", "requires", "return", "short", "signed", "sizeof", "static", "static_assert", "static_cast", "struct", "switch", "synchronized", "template", "this", "thread_local",
			"throw", "true", "try", "typedef", "typeid", "typename", "union", "unsigned", "using", "virtual", "void", "volatile", "wchar_t", "while", "xor", "xor_eq"
		};
		for (const char* const k : cppKeywords)
			langDef.Keys.insert(k);

		static const char* const identifiers[] = {
			"abort", "abs", "acos", "asin", "atan", "atexit", "atof", "atoi", "atol", "ceil", "clock", "cosh", "ctime", "div", "exit", "fabs", "floor", "fmod", "getchar", "getenv", "isalnum", "isalpha", "isdigit", "isgraph",
			"ispunct", "isspace", "isupper", "kbhit", "log10", "log2", "log", "memcmp", "modf", "pow", "printf", "sprintf", "snprintf", "putchar", "putenv", "puts", "rand", "remove", "rename", "sinh", "sqrt", "srand", "strcat", "strcmp", "strerror", "time", "tolower", "toupper",
			"std", "string", "vector", "map", "unordered_map", "set", "unordered_set", "min", "max"
		};
		for (const char* const k : identifiers) {
			Identifier id;
			id.Declaration = "Built-in function";
			langDef.Ids.insert(std::make_pair(std::string(k), id));
		}

		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("//.*", PaletteIndex::Comment));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("[ \t]*#[ \\t]*[a-zA-Z_]+", PaletteIndex::Preprocessor));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("L?\\\"(\\\\.|[^\\\"])*\\\"", PaletteIndex::String));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("\\'\\\\?[^\\']\\'", PaletteIndex::CharLiteral));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("0[xX][0-9a-fA-F]+[uU]?[lL]?[lL]?", PaletteIndex::Number));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][+-]?[0-9]+)?[fF]?", PaletteIndex::Number));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("0[0-7]+[Uu]?[lL]?[lL]?", PaletteIndex::Number));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("[+-]?[0-9]+[Uu]?[lL]?[lL]?", PaletteIndex::Number));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("[a-zA-Z_][a-zA-Z0-9_]*", PaletteIndex::Identifier));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("[\\[\\]\\{\\}\\!\\%\\^\\&\\*\\(\\)\\-\\+\\=\\~\\|\\<\\>\\?\\/\\;\\,\\.]", PaletteIndex::Punctuation));

		langDef.CommentStart = "/*";
		langDef.CommentEnd = "*/";
		langDef.SimpleCommentHead = "//";

		langDef.CaseSensitive = true;

		langDef.Name = "C++";

		inited = true;
	}

	return langDef;
}

CodeEditor::LanguageDefinition CodeEditor::LanguageDefinition::GLSL(void) {
	static bool inited = false;
	static LanguageDefinition langDef;
	if (!inited) {
		static const char* const keywords[] = {
			"auto", "break", "case", "char", "const", "continue", "default", "do", "double", "else", "enum", "extern", "float", "for", "goto", "if", "inline", "int", "long", "register", "restrict", "return", "short",
			"signed", "sizeof", "static", "struct", "switch", "typedef", "union", "unsigned", "void", "volatile", "while", "_Alignas", "_Alignof", "_Atomic", "_Bool", "_Complex", "_Generic", "_Imaginary",
			"_Noreturn", "_Static_assert", "_Thread_local"
		};
		for (const char* const k : keywords)
			langDef.Keys.insert(k);

		static const char* const identifiers[] = {
			"abort", "abs", "acos", "asin", "atan", "atexit", "atof", "atoi", "atol", "ceil", "clock", "cosh", "ctime", "div", "exit", "fabs", "floor", "fmod", "getchar", "getenv", "isalnum", "isalpha", "isdigit", "isgraph",
			"ispunct", "isspace", "isupper", "kbhit", "log10", "log2", "log", "memcmp", "modf", "pow", "putchar", "putenv", "puts", "rand", "remove", "rename", "sinh", "sqrt", "srand", "strcat", "strcmp", "strerror", "time", "tolower", "toupper"
		};
		for (const char* const k : identifiers) {
			Identifier id;
			id.Declaration = "Built-in function";
			langDef.Ids.insert(std::make_pair(std::string(k), id));
		}

		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("//.*", PaletteIndex::Comment));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("[ \t]*#[ \\t]*[a-zA-Z_]+", PaletteIndex::Preprocessor));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("L?\\\"(\\\\.|[^\\\"])*\\\"", PaletteIndex::String));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("\\'\\\\?[^\\']\\'", PaletteIndex::CharLiteral));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][+-]?[0-9]+)?[fF]?", PaletteIndex::Number));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("[+-]?[0-9]+[Uu]?[lL]?[lL]?", PaletteIndex::Number));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("0[0-7]+[Uu]?[lL]?[lL]?", PaletteIndex::Number));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("0[xX][0-9a-fA-F]+[uU]?[lL]?[lL]?", PaletteIndex::Number));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("[a-zA-Z_][a-zA-Z0-9_]*", PaletteIndex::Identifier));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("[\\[\\]\\{\\}\\!\\%\\^\\&\\*\\(\\)\\-\\+\\=\\~\\|\\<\\>\\?\\/\\;\\,\\.]", PaletteIndex::Punctuation));

		langDef.CommentStart = "/*";
		langDef.CommentEnd = "*/";
		langDef.SimpleCommentHead = "//";

		langDef.CaseSensitive = true;

		langDef.Name = "GLSL";

		inited = true;
	}

	return langDef;
}

CodeEditor::LanguageDefinition CodeEditor::LanguageDefinition::HLSL(void) {
	static bool inited = false;
	static LanguageDefinition langDef;
	if (!inited) {
		static const char* const keywords[] = {
			"AppendStructuredBuffer", "asm", "asm_fragment", "BlendState", "bool", "break", "Buffer", "ByteAddressBuffer", "case", "cbuffer", "centroid", "class", "column_major", "compile", "compile_fragment",
			"CompileShader", "const", "continue", "ComputeShader", "ConsumeStructuredBuffer", "default", "DepthStencilState", "DepthStencilView", "discard", "do", "double", "DomainShader", "dword", "else",
			"export", "extern", "false", "float", "for", "fxgroup", "GeometryShader", "groupshared", "half", "Hullshader", "if", "in", "inline", "inout", "InputPatch", "int", "interface", "line", "lineadj",
			"linear", "LineStream", "matrix", "min16float", "min10float", "min16int", "min12int", "min16uint", "namespace", "nointerpolation", "noperspective", "NULL", "out", "OutputPatch", "packoffset",
			"pass", "pixelfragment", "PixelShader", "point", "PointStream", "precise", "RasterizerState", "RenderTargetView", "return", "register", "row_major", "RWBuffer", "RWByteAddressBuffer", "RWStructuredBuffer",
			"RWTexture1D", "RWTexture1DArray", "RWTexture2D", "RWTexture2DArray", "RWTexture3D", "sample", "sampler", "SamplerState", "SamplerComparisonState", "shared", "snorm", "stateblock", "stateblock_state",
			"static", "string", "struct", "switch", "StructuredBuffer", "tbuffer", "technique", "technique10", "technique11", "texture", "Texture1D", "Texture1DArray", "Texture2D", "Texture2DArray", "Texture2DMS",
			"Texture2DMSArray", "Texture3D", "TextureCube", "TextureCubeArray", "true", "typedef", "triangle", "triangleadj", "TriangleStream", "uint", "uniform", "unorm", "unsigned", "vector", "vertexfragment",
			"VertexShader", "void", "volatile", "while",
			"bool1","bool2","bool3","bool4","double1","double2","double3","double4", "float1", "float2", "float3", "float4", "int1", "int2", "int3", "int4", "in", "out", "inout",
			"uint1", "uint2", "uint3", "uint4", "dword1", "dword2", "dword3", "dword4", "half1", "half2", "half3", "half4",
			"float1x1","float2x1","float3x1","float4x1","float1x2","float2x2","float3x2","float4x2",
			"float1x3","float2x3","float3x3","float4x3","float1x4","float2x4","float3x4","float4x4",
			"half1x1","half2x1","half3x1","half4x1","half1x2","half2x2","half3x2","half4x2",
			"half1x3","half2x3","half3x3","half4x3","half1x4","half2x4","half3x4","half4x4",
		};
		for (const char* const k : keywords)
			langDef.Keys.insert(k);

		static const char* const identifiers[] = {
			"abort", "abs", "acos", "all", "AllMemoryBarrier", "AllMemoryBarrierWithGroupSync", "any", "asdouble", "asfloat", "asin", "asint", "asint", "asuint",
			"asuint", "atan", "atan2", "ceil", "CheckAccessFullyMapped", "clamp", "clip", "cos", "cosh", "countbits", "cross", "D3DCOLORtoUBYTE4", "ddx",
			"ddx_coarse", "ddx_fine", "ddy", "ddy_coarse", "ddy_fine", "degrees", "determinant", "DeviceMemoryBarrier", "DeviceMemoryBarrierWithGroupSync",
			"distance", "dot", "dst", "errorf", "EvaluateAttributeAtCentroid", "EvaluateAttributeAtSample", "EvaluateAttributeSnapped", "exp", "exp2",
			"f16tof32", "f32tof16", "faceforward", "firstbithigh", "firstbitlow", "floor", "fma", "fmod", "frac", "frexp", "fwidth", "GetRenderTargetSampleCount",
			"GetRenderTargetSamplePosition", "GroupMemoryBarrier", "GroupMemoryBarrierWithGroupSync", "InterlockedAdd", "InterlockedAnd", "InterlockedCompareExchange",
			"InterlockedCompareStore", "InterlockedExchange", "InterlockedMax", "InterlockedMin", "InterlockedOr", "InterlockedXor", "isfinite", "isinf", "isnan",
			"ldexp", "length", "lerp", "lit", "log", "log10", "log2", "mad", "max", "min", "modf", "msad4", "mul", "noise", "normalize", "pow", "printf",
			"Process2DQuadTessFactorsAvg", "Process2DQuadTessFactorsMax", "Process2DQuadTessFactorsMin", "ProcessIsolineTessFactors", "ProcessQuadTessFactorsAvg",
			"ProcessQuadTessFactorsMax", "ProcessQuadTessFactorsMin", "ProcessTriTessFactorsAvg", "ProcessTriTessFactorsMax", "ProcessTriTessFactorsMin",
			"radians", "rcp", "reflect", "refract", "reversebits", "round", "rsqrt", "saturate", "sign", "sin", "sincos", "sinh", "smoothstep", "sqrt", "step",
			"tan", "tanh", "tex1D", "tex1D", "tex1Dbias", "tex1Dgrad", "tex1Dlod", "tex1Dproj", "tex2D", "tex2D", "tex2Dbias", "tex2Dgrad", "tex2Dlod", "tex2Dproj",
			"tex3D", "tex3D", "tex3Dbias", "tex3Dgrad", "tex3Dlod", "tex3Dproj", "texCUBE", "texCUBE", "texCUBEbias", "texCUBEgrad", "texCUBElod", "texCUBEproj", "transpose", "trunc"
		};
		for (const char* const k : identifiers) {
			Identifier id;
			id.Declaration = "Built-in function";
			langDef.Ids.insert(std::make_pair(std::string(k), id));
		}

		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("//.*", PaletteIndex::Comment));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("[ \t]*#[ \\t]*[a-zA-Z_]+", PaletteIndex::Preprocessor));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("L?\\\"(\\\\.|[^\\\"])*\\\"", PaletteIndex::String));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("\\'\\\\?[^\\']\\'", PaletteIndex::CharLiteral));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][+-]?[0-9]+)?[fF]?", PaletteIndex::Number));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("[+-]?[0-9]+[Uu]?[lL]?[lL]?", PaletteIndex::Number));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("0[0-7]+[Uu]?[lL]?[lL]?", PaletteIndex::Number));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("0[xX][0-9a-fA-F]+[uU]?[lL]?[lL]?", PaletteIndex::Number));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("[a-zA-Z_][a-zA-Z0-9_]*", PaletteIndex::Identifier));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("[\\[\\]\\{\\}\\!\\%\\^\\&\\*\\(\\)\\-\\+\\=\\~\\|\\<\\>\\?\\/\\;\\,\\.]", PaletteIndex::Punctuation));

		langDef.CommentStart = "/*";
		langDef.CommentEnd = "*/";
		langDef.SimpleCommentHead = "//";

		langDef.CaseSensitive = true;

		langDef.Name = "HLSL";

		inited = true;
	}

	return langDef;
}

CodeEditor::LanguageDefinition CodeEditor::LanguageDefinition::Lua(void) {
	static bool inited = false;
	static LanguageDefinition langDef;
	if (!inited) {
		static const char* const keywords[] = {
			"and", "break", "do", "else", "elseif", "end",
			"false", "for", "function", "goto", "if", "in",
			"local", "nil", "not", "or", "repeat", "return",
			"then", "true", "until", "while"
		};

		for (const char* const k : keywords)
			langDef.Keys.insert(k);

		static const char* const identifiers[] = {
			"__add", "__sub", "__mul", "__div",
			"__mod", "__pow", "__unm", "__idiv",
			"__band", "__bor", "__bxor", "__bnot",
			"__shl", "__shr",
			"__concat", "__len",
			"__eq", "__lt", "__le",
			"__index", "__newindex", "__call",
			"__gc", "__close", "__mode", "__name", "__tostring",

			"char", "close", "flush", "getmetatable", "len", "lines", "pack", "read", "remove", "setmetatable", "type", "unpack", "write",

			"_G", "assert", "collectgarbage", "dofile", "error", "ipairs", "load", "loadfile", "next", "pairs", "pcall", "print", "rawequal", "rawget", "rawlen", "rawset", "select", "tonumber", "tostring", "xpcall",
			"coroutine", "create", "isyieldable", "resume", "running", "status", "wrap", "yield",
			"require", "package", "config", "cpath", "loaded", "loadlib", "path", "preload", "searchers", "searchpath",
			"string", "byte", "dump", "find", "format", "gmatch", "gsub", "lower", "match", "packsize", "rep", "reverse", "sub", "upper",
			"utf8", "charpattern", "codes", "codepoint", "offset",
			"table", "concat", "insert", "move", "sort",
			"math", "abs", "acos", "asin", "atan", "ceil", "cos", "deg", "exp", "floor", "fmod", "huge", "log", "max", "maxinteger", "min", "mininteger", "modf", "pi", "rad", "random", "randomseed", "sin", "sqrt", "tan", "tointeger", "ult",
			"io", "input", "open", "output", "popen", "tmpfile",
			"file", "seek", "setvbuf",
			"os", "clock", "date", "difftime", "execute", "exit", "getenv", "rename", "setlocale", "time", "tmpname",
			"debug", "gethook", "getinfo", "getlocal", "getregistry", "getupvalue", "getuservalue", "sethook", "setlocal", "setupvalue", "setuservalue", "traceback", "upvalueid", "upvaluejoin",
			"const", "self"
		};
		for (const char* const k : identifiers) {
			Identifier id;
			id.Declaration = "Built-in function";
			auto it = langDef.Ids.insert(std::make_pair(std::string(k), id));
			if (!it.second)
				fprintf(stderr, "Duplicated identifier: \"%s\"\n", k);
		}

		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("\\-\\-.*", PaletteIndex::Comment));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("L?\\\"(\\\\.|[^\\\"])*\\\"", PaletteIndex::String));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("\\\'[^\\\']*\\\'", PaletteIndex::String));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("0[xX][0-9a-fA-F]+[uU]?[lL]?[lL]?", PaletteIndex::Number));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][+-]?[0-9]+)?[fF]?", PaletteIndex::Number));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("[+-]?[0-9]+[Uu]?[lL]?[lL]?", PaletteIndex::Number));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("[a-zA-Z_][a-zA-Z0-9_]*", PaletteIndex::Identifier));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("[\\[\\]\\{\\}\\!\\#\\%\\^\\&\\*\\(\\)\\-\\+\\=\\~\\|\\:\\<\\>\\?\\/\\;\\,\\.]", PaletteIndex::Punctuation));

		langDef.CommentStart = "--[[";
		langDef.CommentEnd = "]]";
		langDef.SimpleCommentHead = "--";

		langDef.CaseSensitive = true;

		langDef.Name = "Lua";

		inited = true;
	}

	return langDef;
}

CodeEditor::LanguageDefinition CodeEditor::LanguageDefinition::SQL(void) {
	static bool inited = false;
	static LanguageDefinition langDef;
	if (!inited) {
		static const char* const keywords[] = {
			"ADD", "EXCEPT", "PERCENT", "ALL", "EXEC", "PLAN", "ALTER", "EXECUTE", "PRECISION", "AND", "EXISTS", "PRIMARY", "ANY", "EXIT", "PRINT", "AS", "FETCH", "PROC", "ASC", "FILE", "PROCEDURE",
			"AUTHORIZATION", "FILLFACTOR", "PUBLIC", "BACKUP", "FOR", "RAISERROR", "BEGIN", "FOREIGN", "READ", "BETWEEN", "FREETEXT", "READTEXT", "BREAK", "FREETEXTTABLE", "RECONFIGURE",
			"BROWSE", "FROM", "REFERENCES", "BULK", "FULL", "REPLICATION", "BY", "FUNCTION", "RESTORE", "CASCADE", "GOTO", "RESTRICT", "CASE", "GRANT", "RETURN", "CHECK", "GROUP", "REVOKE",
			"CHECKPOINT", "HAVING", "RIGHT", "CLOSE", "HOLDLOCK", "ROLLBACK", "CLUSTERED", "IDENTITY", "ROWCOUNT", "COALESCE", "IDENTITY_INSERT", "ROWGUIDCOL", "COLLATE", "IDENTITYCOL", "RULE",
			"COLUMN", "IF", "SAVE", "COMMIT", "IN", "SCHEMA", "COMPUTE", "INDEX", "SELECT", "CONSTRAINT", "INNER", "SESSION_USER", "CONTAINS", "INSERT", "SET", "CONTAINSTABLE", "INTERSECT", "SETUSER",
			"CONTINUE", "INTO", "SHUTDOWN", "CONVERT", "IS", "SOME", "CREATE", "JOIN", "STATISTICS", "CROSS", "KEY", "SYSTEM_USER", "CURRENT", "KILL", "TABLE", "CURRENT_DATE", "LEFT", "TEXTSIZE",
			"CURRENT_TIME", "LIKE", "THEN", "CURRENT_TIMESTAMP", "LINENO", "TO", "CURRENT_USER", "LOAD", "TOP", "CURSOR", "NATIONAL", "TRAN", "DATABASE", "NOCHECK", "TRANSACTION",
			"DBCC", "NONCLUSTERED", "TRIGGER", "DEALLOCATE", "NOT", "TRUNCATE", "DECLARE", "NULL", "TSEQUAL", "DEFAULT", "NULLIF", "UNION", "DELETE", "OF", "UNIQUE", "DENY", "OFF", "UPDATE",
			"DESC", "OFFSETS", "UPDATETEXT", "DISK", "ON", "USE", "DISTINCT", "OPEN", "USER", "DISTRIBUTED", "OPENDATASOURCE", "VALUES", "DOUBLE", "OPENQUERY", "VARYING","DROP", "OPENROWSET", "VIEW",
			"DUMMY", "OPENXML", "WAITFOR", "DUMP", "OPTION", "WHEN", "ELSE", "OR", "WHERE", "END", "ORDER", "WHILE", "ERRLVL", "OUTER", "WITH", "ESCAPE", "OVER", "WRITETEXT"
		};

		for (const char* const k : keywords)
			langDef.Keys.insert(k);

		static const char* const identifiers[] = {
			"ABS",  "ACOS",  "ADD_MONTHS",  "ASCII",  "ASCIISTR",  "ASIN",  "ATAN",  "ATAN2",  "AVG",  "BFILENAME",  "BIN_TO_NUM",  "BITAND",  "CARDINALITY",  "CASE",  "CAST",  "CEIL",
			"CHARTOROWID",  "CHR",  "COALESCE",  "COMPOSE",  "CONCAT",  "CONVERT",  "CORR",  "COS",  "COSH",  "COUNT",  "COVAR_POP",  "COVAR_SAMP",  "CUME_DIST",  "CURRENT_DATE",
			"CURRENT_TIMESTAMP",  "DBTIMEZONE",  "DECODE",  "DECOMPOSE",  "DENSE_RANK",  "DUMP",  "EMPTY_BLOB",  "EMPTY_CLOB",  "EXP",  "EXTRACT",  "FIRST_VALUE",  "FLOOR",  "FROM_TZ",  "GREATEST",
			"GROUP_ID",  "HEXTORAW",  "INITCAP",  "INSTR",  "INSTR2",  "INSTR4",  "INSTRB",  "INSTRC",  "LAG",  "LAST_DAY",  "LAST_VALUE",  "LEAD",  "LEAST",  "LENGTH",  "LENGTH2",  "LENGTH4",
			"LENGTHB",  "LENGTHC",  "LISTAGG",  "LN",  "LNNVL",  "LOCALTIMESTAMP",  "LOG",  "LOWER",  "LPAD",  "LTRIM",  "MAX",  "MEDIAN",  "MIN",  "MOD",  "MONTHS_BETWEEN",  "NANVL",  "NCHR",
			"NEW_TIME",  "NEXT_DAY",  "NTH_VALUE",  "NULLIF",  "NUMTODSINTERVAL",  "NUMTOYMINTERVAL",  "NVL",  "NVL2",  "POWER",  "RANK",  "RAWTOHEX",  "REGEXP_COUNT",  "REGEXP_INSTR",
			"REGEXP_REPLACE",  "REGEXP_SUBSTR",  "REMAINDER",  "REPLACE",  "ROUND",  "ROWNUM",  "RPAD",  "RTRIM",  "SESSIONTIMEZONE",  "SIGN",  "SIN",  "SINH",
			"SOUNDEX",  "SQRT",  "STDDEV",  "SUBSTR",  "SUM",  "SYS_CONTEXT",  "SYSDATE",  "SYSTIMESTAMP",  "TAN",  "TANH",  "TO_CHAR",  "TO_CLOB",  "TO_DATE",  "TO_DSINTERVAL",  "TO_LOB",
			"TO_MULTI_BYTE",  "TO_NCLOB",  "TO_NUMBER",  "TO_SINGLE_BYTE",  "TO_TIMESTAMP",  "TO_TIMESTAMP_TZ",  "TO_YMINTERVAL",  "TRANSLATE",  "TRIM",  "TRUNC", "TZ_OFFSET",  "UID",  "UPPER",
			"USER",  "USERENV",  "VAR_POP",  "VAR_SAMP",  "VARIANCE",  "VSIZE "
		};
		for (const char* const k : identifiers) {
			Identifier id;
			id.Declaration = "Built-in function";
			langDef.Ids.insert(std::make_pair(std::string(k), id));
		}

		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("\\-\\-.*", PaletteIndex::Comment));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("L?\\\"(\\\\.|[^\\\"])*\\\"", PaletteIndex::String));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("\\\'[^\\\']*\\\'", PaletteIndex::String));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][+-]?[0-9]+)?[fF]?", PaletteIndex::Number));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("[+-]?[0-9]+[Uu]?[lL]?[lL]?", PaletteIndex::Number));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("0[0-7]+[Uu]?[lL]?[lL]?", PaletteIndex::Number));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("0[xX][0-9a-fA-F]+[uU]?[lL]?[lL]?", PaletteIndex::Number));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("[a-zA-Z_][a-zA-Z0-9_]*", PaletteIndex::Identifier));
		langDef.TokenRegexPatterns.push_back(std::make_pair<std::string, PaletteIndex>("[\\[\\]\\{\\}\\!\\%\\^\\&\\*\\(\\)\\-\\+\\=\\~\\|\\<\\>\\?\\/\\;\\,\\.]", PaletteIndex::Punctuation));

		langDef.CommentStart = "/*";
		langDef.CommentEnd = "*/";

		langDef.CaseSensitive = false;

		langDef.Name = "SQL";

		inited = true;
	}

	return langDef;
}

bool CodeEditor::UndoRecord::Similar(const UndoRecord* o) const {
	if (o == nullptr)
		return false;
	if (Type != o->Type)
		return false;
	if (Start.Line != o->Start.Line || End.Line != o->End.Line)
		return false;

	auto isalpha = [] (char ch) {
		return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z');
	};
	auto isnum = [] (char ch) {
		return ch >= '0' && ch <= '9';
	};
	auto isblank = [] (char ch) {
		return ch == ' ' || ch == '\t';
	};
	if ((Content.length() == 1 && isalpha(*Content.begin())) &&
		(o->Content.length() == 1 && isalpha(*o->Content.begin()))) {
		return true;
	}
	if ((Content.length() == 1 && isnum(*Content.begin())) &&
		(o->Content.length() == 1 && isnum(*o->Content.begin()))) {
		return true;
	}
	if ((Content.length() == 1 && isblank(*Content.begin())) &&
		(o->Content.length() == 1 && isblank(*o->Content.begin()))) {
		return true;
	}
	if (Content.length() > 1 && Content.length() <= 4 && o->Content.length() > 1 && o->Content.length() <= 4) {
		int l = ImTextExpectUtf8Char(Content.c_str());
		int r = ImTextExpectUtf8Char(o->Content.c_str());
		if ((int)Content.length() == l && (int)o->Content.length() == r)
			return true;
	}

	return false;
}

void CodeEditor::UndoRecord::Undo(CodeEditor* aEditor) {
	if (!Content.empty()) {
		switch (Type) {
		case UndoType::Add: // Fall through.
		case UndoType::ToLowerCase: // Fall through.
		case UndoType::ToUpperCase: {
				aEditor->State = After;

				aEditor->DeleteRange(Start, End);
				aEditor->Colorize(Start.Line - 1, End.Line - Start.Line + 2);

				int lines = 0;
				if (!Overwritten.empty()) {
					lines = (int)aEditor->CodeLines.size();
					Coordinates start = Start;
					aEditor->InsertTextAt(start, Overwritten.c_str());
					lines = (int)aEditor->CodeLines.size() - lines + 1;
					aEditor->Colorize(Start.Line - 1, std::max(End.Line - Start.Line + 1, lines + 1));
				}

				Coordinates end(std::max(Start.Line + lines - 1, 0), Start.Column);
				if (Start > end)
					end = Start;
				aEditor->OnChanged(Start, end, -1, true);
			}

			break;
		case UndoType::Remove: {
				Coordinates start = Start;
				aEditor->InsertTextAt(start, Content.c_str());
				aEditor->Colorize(Start.Line - 1, End.Line - Start.Line + 2);

				aEditor->OnChanged(Start, End, -1, true);
			}

			break;
		case UndoType::Indent: {
				for (int i = Start.Line; i <= End.Line; ++i) {
					Line &line = aEditor->CodeLines[i];
					for (int j = 0; j < (int)Content.size(); ++j) {
						if (line.Glyphs.empty())
							break;

						line.Glyphs.erase(line.Glyphs.begin());
					}

					Coordinates pos(i, 0);
					aEditor->OnChanged(pos, pos, -1, true);
				}
			}

			break;
		case UndoType::Unindent: {
				if (Content.front() == '\0')
					break;

				for (int i = Start.Line; i <= End.Line; ++i) {
					Line &line = aEditor->CodeLines[i];
					for (int j = 0; j < (int)Content.size(); ++j) {
						line.Glyphs.insert(line.Glyphs.begin(), Glyph(Content[j], PaletteIndex::Default));

						Coordinates pos(i, 0);
						aEditor->OnChanged(pos, pos, -1, true);
					}
				}
			}

			break;
		case UndoType::Comment: {
				assert(End.Line - Start.Line + 1 == (int)Content.length());

				for (int i = Start.Line; i <= End.Line; ++i) {
					Line &line = aEditor->CodeLines[i];
					const char op = Content[i - Start.Line];
					if (op == 0) {
						// Do nothing.
					} else if (op == 1 || op == 2) {
						bool match = true;
						if (line.Glyphs.size() >= aEditor->LangDef.SimpleCommentHead.size()) {
							for (int j = 0; j < (int)aEditor->LangDef.SimpleCommentHead.length(); ++j) {
								Char cch = aEditor->LangDef.SimpleCommentHead[j];
								const Glyph &g = line.Glyphs[j];
								if (cch != g.Character) {
									match = false;

									break;
								}
							}
						} else {
							match = false;
						}
						if (match) {
							int n = 0;
							for (int j = 0; j < (int)aEditor->LangDef.SimpleCommentHead.length(); ++j)
								line.Glyphs.erase(line.Glyphs.begin());
							++n;

							if (!line.Glyphs.empty() && op == 2) {
								const Glyph &g_ = *line.Glyphs.begin();
								if (g_.Character == ' ') {
									line.Glyphs.erase(line.Glyphs.begin());
									++n;
								}
							}
						}

						Coordinates pos(i, 0);
						aEditor->OnChanged(pos, pos, -1, true);
					} else {
						assert(false);
					}
				}

				aEditor->Colorize(Start.Line, End.Line - Start.Line + 1);
			}

			break;
		case UndoType::Uncomment: {
				assert(End.Line - Start.Line + 1 == (int)Content.length());

				for (int i = Start.Line; i <= End.Line; ++i) {
					Line &line = aEditor->CodeLines[i];
					char op = Content[i - Start.Line];
					if (op == 0) {
						// Do nothing.
					} else if (op == 1 || op == 2) {
						if (op == 2)
							line.Glyphs.insert(line.Glyphs.begin(), Glyph(' ', PaletteIndex::Default));
						for (int j = (int)aEditor->LangDef.SimpleCommentHead.length() - 1; j >= 0; --j)
							line.Glyphs.insert(line.Glyphs.begin(), Glyph(aEditor->LangDef.SimpleCommentHead[j], PaletteIndex::Comment));

						Coordinates pos(i, 0);
						aEditor->OnChanged(pos, pos, -1, true);
					} else {
						assert(false);
					}
				}

				aEditor->Colorize(Start.Line, End.Line - Start.Line + 1);
			}

			break;
		case UndoType::MoveLineUp: {
				for (int i = End.Line - 1; i >= Start.Line - 1; --i) {
					std::swap(aEditor->CodeLines[i], aEditor->CodeLines[i + 1]);
				}
				Coordinates pos(Start.Line - 1, 0);
				Coordinates pos_(End.Line - 1 + 1, 0);
				aEditor->OnChanged(pos, pos_, -1, true);
			}

			break;
		case UndoType::MoveLineDown: {
				for (int i = Start.Line + 1; i <= End.Line + 1; ++i) {
					std::swap(aEditor->CodeLines[i], aEditor->CodeLines[i - 1]);
				}
				Coordinates pos(Start.Line + 1 - 1, 0);
				Coordinates pos_(End.Line + 1, 0);
				aEditor->OnChanged(pos, pos_, -1, true);
			}

			break;
		}
	}

	aEditor->State = Before;
	aEditor->EnsureCursorVisible();

	aEditor->OnModified(false, true);
}

void CodeEditor::UndoRecord::Redo(CodeEditor* aEditor) {
	if (!Content.empty()) {
		switch (Type) {
		case UndoType::Add: // Fall through.
		case UndoType::ToLowerCase: // Fall through.
		case UndoType::ToUpperCase: {
				aEditor->State = Before;

				aEditor->DeleteSelection();

				Coordinates start = Start;
				aEditor->InsertTextAt(start, Content.c_str());
				aEditor->Colorize(Start.Line - 1, End.Line - Start.Line + 1 + 1);

				aEditor->OnChanged(Start, End, 1, true);
			}

			break;
		case UndoType::Remove: {
				aEditor->DeleteRange(Start, End);
				aEditor->Colorize(Start.Line - 1, End.Line - Start.Line + 1);

				aEditor->OnChanged(Start, Start, 1, true);
			}

			break;
		case UndoType::Indent: {
				for (int i = Start.Line; i <= End.Line; ++i) {
					Line &line = aEditor->CodeLines[i];
					for (int j = 0; j < (int)Content.size(); ++j) {
						line.Glyphs.insert(line.Glyphs.begin(), Glyph(Content[j], PaletteIndex::Default));

						Coordinates pos(i, 0);
						aEditor->OnChanged(pos, pos, 1, true);
					}
				}
			}

			break;
		case UndoType::Unindent: {
				if (Content.front() == '\0')
					break;

				for (int i = Start.Line; i <= End.Line; ++i) {
					Line &line = aEditor->CodeLines[i];
					for (int j = 0; j < (int)Content.size(); ++j) {
						if (line.Glyphs.empty())
							break;

						line.Glyphs.erase(line.Glyphs.begin());
					}

					Coordinates pos(i, 0);
					aEditor->OnChanged(pos, pos, 1, true);
				}
			}

			break;
		case UndoType::Comment: {
				assert(End.Line - Start.Line + 1 == (int)Content.length());

				for (int i = Start.Line; i <= End.Line; ++i) {
					Line &line = aEditor->CodeLines[i];
					char op = Content[i - Start.Line];
					if (op == 0) {
						// Do nothing.
					} else if (op == 1 || op == 2) {
						if (op == 2)
							line.Glyphs.insert(line.Glyphs.begin(), Glyph(' ', PaletteIndex::Default));
						for (int j = (int)aEditor->LangDef.SimpleCommentHead.length() - 1; j >= 0; --j)
							line.Glyphs.insert(line.Glyphs.begin(), Glyph(aEditor->LangDef.SimpleCommentHead[j], PaletteIndex::Comment));

						Coordinates pos(i, 0);
						aEditor->OnChanged(pos, pos, -1, true);
					} else {
						assert(false);
					}
				}

				aEditor->Colorize(Start.Line, End.Line - Start.Line + 1);
			}

			break;
		case UndoType::Uncomment: {
				assert(End.Line - Start.Line + 1 == (int)Content.length());

				for (int i = Start.Line; i <= End.Line; ++i) {
					Line &line = aEditor->CodeLines[i];
					const char op = Content[i - Start.Line];
					if (op == 0) {
						// Do nothing.
					} else if (op == 1 || op == 2) {
						bool match = true;
						if (line.Glyphs.size() >= aEditor->LangDef.SimpleCommentHead.size()) {
							for (int j = 0; j < (int)aEditor->LangDef.SimpleCommentHead.length(); ++j) {
								Char cch = aEditor->LangDef.SimpleCommentHead[j];
								const Glyph &g = line.Glyphs[j];
								if (cch != g.Character) {
									match = false;

									break;
								}
							}
						} else {
							match = false;
						}
						if (match) {
							int n = 0;
							for (int j = 0; j < (int)aEditor->LangDef.SimpleCommentHead.length(); ++j)
								line.Glyphs.erase(line.Glyphs.begin());
							++n;

							if (!line.Glyphs.empty() && op == 2) {
								const Glyph &g_ = *line.Glyphs.begin();
								if (g_.Character == ' ') {
									line.Glyphs.erase(line.Glyphs.begin());
									++n;
								}
							}
						}

						Coordinates pos(i, 0);
						aEditor->OnChanged(pos, pos, -1, true);
					} else {
						assert(false);
					}
				}

				aEditor->Colorize(Start.Line, End.Line - Start.Line + 1);
			}

			break;
		case UndoType::MoveLineUp: {
				for (int i = Start.Line; i <= End.Line; ++i) {
					std::swap(aEditor->CodeLines[i], aEditor->CodeLines[i - 1]);
				}
				Coordinates pos(Start.Line - 1, 0);
				Coordinates pos_(End.Line, 0);
				aEditor->OnChanged(pos, pos_, -1, true);
			}

			break;
		case UndoType::MoveLineDown: {
				for (int i = End.Line; i >= Start.Line; --i) {
					std::swap(aEditor->CodeLines[i], aEditor->CodeLines[i + 1]);
				}
				Coordinates pos(Start.Line, 0);
				Coordinates pos_(End.Line + 1, 0);
				aEditor->OnChanged(pos, pos_, -1, true);
			}

			break;
		}
	}

	aEditor->State = After;
	aEditor->EnsureCursorVisible();

	aEditor->OnModified(false, true);
}

CodeEditor::Error::Error() {
}

CodeEditor::Error::Error(const std::string &msg, bool isWarning_, bool withLineNumber_) :
	message(msg), isWarning(isWarning_), withLineNumber(withLineNumber_) {
}

CodeEditor::Glyph::Glyph(Char aChar, ImU32 aColorIndex) : Character(aChar), ColorIndex(aColorIndex), MultiLineComment(false) {
	if (aChar <= 255) {
		Codepoint = (ImWchar)aChar;
	} else {
		const char* txt = (const char*)(&aChar);
		const char* tend = txt + sizeof(Char);
		unsigned int codepoint = 0;
		ImTextCharFromUtf8(&codepoint, txt, tend);
		Codepoint = (ImWchar)codepoint;
	}
}

CodeEditor::Glyph::Glyph(Char aChar, PaletteIndex aColorIndex) : Character(aChar), ColorIndex((ImU32)aColorIndex), MultiLineComment(false) {
	if (aChar <= 255) {
		Codepoint = (ImWchar)aChar;
	} else {
		const char* txt = (const char*)(&aChar);
		const char* tend = txt + sizeof(Char);
		unsigned int codepoint = 0;
		ImTextCharFromUtf8(&codepoint, txt, tend);
		Codepoint = (ImWchar)codepoint;
	}
}

void CodeEditor::Line::Clear(void) {
	Changed = LineState::None;
}

void CodeEditor::Line::Change(void) {
	Changed = LineState::Edited;
}

void CodeEditor::Line::Save(void) {
	Changed = LineState::EditedSaved;
}

void CodeEditor::Line::Revert(void) {
	Changed = LineState::EditedReverted;
}

CodeEditor::CodeEditor() :
	LineSpacing(1.0f),
	UndoIndex(0),
	SavedIndex(0),
	Font(nullptr),
	IndentWithTab(false),
	TabSize(4),
	TextStart(7),
	HeadSize(0),
	Overwrite(false),
	ReadOnly(false),
	ShowLineNumbers(true),
	StickyLineNumbers(false),
	HeadClickEnabled(false),
	ShortcutsEnabled(ShortcutType::All),
	WithinRender(false),
	ScrollToCursor(0),
	ScrollY(0.0f),
	ToSetScrollY(false),
	WordSelectionMode(false),
	ColorRangeMin(0),
	ColorRangeMax(0),
	InputtedRangedPair(false),
	CheckMultilineComments(0),
	ErrorTipEnabled(true),
	TooltipEnabled(true),
	ShowWhiteSpaces(true),
	SafeColumnIndicatorOffset(0),
	EditorFocused(false),
	ProgramPointer(-1)
{
	SetPalette(GetDarkPalette());
	SetLanguageDefinition(LanguageDefinition::Lua());
	CodeLines.push_back(Line());
}

CodeEditor::~CodeEditor() {
}

CodeEditor::LanguageDefinition &CodeEditor::SetLanguageDefinition(const LanguageDefinition &aLanguageDef) {
	LangDef = aLanguageDef;
	Regexes.clear();

	std::regex_constants::syntax_option_type opt = std::regex_constants::optimize;
	if (!LangDef.CaseSensitive)
		opt |= std::regex_constants::icase;
	for (const LanguageDefinition::TokenRegexString &r : LangDef.TokenRegexPatterns) {
		try {
			auto re = std::make_pair(std::regex(r.first, opt), r.second);
			Regexes.push_back(re);
		} catch (const std::regex_error &err) {
			if (err.what() && r.first.c_str())
				fprintf(stderr, "Regex error: %s of \"%s\".\n", err.what(), r.first.c_str());
			else
				fprintf(stderr, "Regex error: %s.\n", "unknown");
		}
	}

	return LangDef;
}

const CodeEditor::LanguageDefinition &CodeEditor::GetLanguageDefinition(void) const {
	return LangDef;
}

CodeEditor::LanguageDefinition &CodeEditor::GetLanguageDefinition(void) {
	return LangDef;
}

void CodeEditor::SetFont(const ImFont* font) {
	Font = font;
}

const ImFont* CodeEditor::GetFont(void) const {
	return Font;
}

void CodeEditor::SetPalette(const Palette &aValue) {
	Plt = aValue;
}

const CodeEditor::Palette &CodeEditor::GetPalette(void) const {
	return Plt;
}

void CodeEditor::SetErrorMarkers(const ErrorMarkers &aMarkers) {
	Errs = aMarkers;
}

void CodeEditor::ClearErrorMarkers(void) {
	if (!Errs.empty())
		Errs.clear();
}

void CodeEditor::SetBreakpoints(const Breakpoints &aMarkers) {
	Brks = aMarkers;
}

void CodeEditor::ClearBreakpoints(void) {
	if (!Brks.empty())
		Brks.clear();
}
void CodeEditor::SetProgramPointer(int aPointer) {
	ProgramPointer = aPointer;
}

int CodeEditor::GetProgramPointer(void) {
	return ProgramPointer;
}

void CodeEditor::Render(const char* aTitle, const ImVec2 &aSize, bool aBorder) {
	// Prepare.
	WithinRender = true;

	ImGuiContext &g = *GImGui;
	ImGuiIO &io = GetIO();
	const ImFont* font = GetFont();
	if (!font)
		font = io.Fonts->Fonts[0];
	const float xadv = font->IndexAdvanceX['X'];
	CharAdv = ImVec2(xadv, font->FontSize + LineSpacing);
	if (io.FontGlobalScale != 1.0f) {
		CharAdv.x *= io.FontGlobalScale;
		CharAdv.y *= io.FontGlobalScale;
	}
	if (IsShowLineNumbers()) {
		if (CodeLines.size() >= 10000)
			TextStart = 7;
		else if (CodeLines.size() >= 1000)
			TextStart = 6;
		else
			TextStart = 5;
		++TextStart; // For edited states.
	} else {
		TextStart = 1;
	}

	PushStyleColor(ImGuiCol_ChildBg, ColorConvertU32ToFloat4(Plt[(int)PaletteIndex::Background]));
	PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
	BeginChild(
		aTitle, aSize, aBorder,
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar |
		ImGuiWindowFlags_NoNav
	);

	PushAllowKeyboardFocus(true);

	const bool ctrl = io.KeyCtrl;
	const bool shift = io.KeyShift;
	const bool alt = io.KeyAlt;

	CursorScreenPos = GetCursorScreenPos();
	EditorFocused = IsWindowFocused();

	// Process shortcuts.
	if (IsEditorFocused()) {
		do {
			const float margin = 12.0f;
			const ImVec2 start(GetWindowPos().x + margin, GetWindowPos().y);
			const ImVec2 end(start.x + GetWindowWidth() - margin * 2, start.y + GetWindowHeight());
			if (IsMouseHoveringRect(start, end))
				SetMouseCursor(ImGuiMouseCursor_TextInput);
			CaptureKeyboardFromApp(true);
		} while (false);

		io.WantCaptureKeyboard = true;
		io.WantTextInput = true;

		if (IsShortcutsEnabled(ShortcutType::UndoRedo)) {
			if (!IsReadOnly()) {
				if (ctrl && !shift && !alt && IsKeyPressed(GetKeyIndex(ImGuiKey_Z))) {
					Undo();
				} else if (ctrl && !shift && !alt && IsKeyPressed(GetKeyIndex(ImGuiKey_Y))) {
					Redo();
				}
			}
		}

		if (IsShortcutsEnabled(ShortcutType::CopyCutPasteDelete)) {
			if (ctrl && !shift && !alt && IsKeyPressed(GetKeyIndex(ImGuiKey_A)))
				SelectAll();
			else if (ctrl && !shift && !alt && IsKeyPressed(GetKeyIndex(ImGuiKey_C)))
				Copy();
			else if (ctrl && !shift && !alt && IsKeyPressed(GetKeyIndex(ImGuiKey_X)))
				Cut();
			else if (!IsReadOnly() && ctrl && !shift && !alt && IsKeyPressed(GetKeyIndex(ImGuiKey_V)))
				Paste();
			else if (!IsReadOnly() && !shift && !alt && IsKeyPressed(GetKeyIndex(ImGuiKey_Delete)))
				Delete();
			else if (!IsReadOnly() && shift && !alt && IsKeyPressed(GetKeyIndex(ImGuiKey_Delete)))
				DeleteLine();
		}

		if (!ctrl && !alt && IsKeyPressed(GetKeyIndex(ImGuiKey_UpArrow)))
			MoveUp(1, shift);
		else if (!ctrl && !alt && IsKeyPressed(GetKeyIndex(ImGuiKey_DownArrow)))
			MoveDown(1, shift);
		else if (!alt && IsKeyPressed(GetKeyIndex(ImGuiKey_LeftArrow)))
			MoveLeft(1, shift, ctrl);
		else if (!alt && IsKeyPressed(GetKeyIndex(ImGuiKey_RightArrow)))
			MoveRight(1, shift, ctrl);
		else if (!ctrl && !alt && IsKeyPressed(GetKeyIndex(ImGuiKey_PageUp)))
			MoveUp(GetPageSize() - 2, shift);
		else if (!ctrl && !alt && IsKeyPressed(GetKeyIndex(ImGuiKey_PageDown)))
			MoveDown(GetPageSize() - 2, shift);
		else if (ctrl && !alt && IsKeyPressed(GetKeyIndex(ImGuiKey_Home)))
			MoveTop(shift);
		else if (ctrl && !alt && IsKeyPressed(GetKeyIndex(ImGuiKey_End)))
			MoveBottom(shift);
		else if (!ctrl && !alt && IsKeyPressed(GetKeyIndex(ImGuiKey_Home)))
			MoveHome(shift);
		else if (!ctrl && !alt && IsKeyPressed(GetKeyIndex(ImGuiKey_End)))
			MoveEnd(shift);

		if (!IsReadOnly()) {
			if (IsKeyPressed(GetKeyIndex(ImGuiKey_Enter)) || OnKeyPressed(ImGuiKey_Enter)) {
				if (!alt) {
					unsigned int c = '\n'; // Insert new line.
					io.AddInputCharacter((ImWchar)c);
				}
			} else if (IsKeyPressed(GetKeyIndex(ImGuiKey_Tab))) {
				if (HasSelection() && GetSelectionLines() > 1) {
					if (IsShortcutsEnabled(ShortcutType::IndentUnindent)) {
						if (!ctrl && !shift && !alt) // Indent multi-lines.
							Indent();
						else if (!ctrl && shift && !alt) // Unindent multi-lines.
							Unindent();
						else if (ctrl && shift && !alt) // Unindent multi-lines.
							Unindent();
					}
				} else {
					if (!ctrl && !shift && !alt) { // Insert tab.
						unsigned int c = '\t'; // Insert tab.
						io.AddInputCharacter((ImWchar)c);
					}
					if (IsShortcutsEnabled(ShortcutType::IndentUnindent)) {
						if (!ctrl && shift && !alt) // Unindent single line.
							Unindent();
						else if (ctrl && shift && !alt) // Unindent single line.
							Unindent();
					}
				}
			}
			if (IsShortcutsEnabled(ShortcutType::BackSpaceOrWordwise)) {
				if (!ctrl && !alt && IsKeyPressed(GetKeyIndex(ImGuiKey_Backspace))) {
					BackSpace();
				} else if (ctrl && !alt && IsKeyPressed(GetKeyIndex(ImGuiKey_Backspace))) {
					BackSpaceWordwise();
				}
			}
			if (IsShortcutsEnabled(ShortcutType::ToUpperOrLowerCase)) {
				if (ctrl && !alt && IsKeyPressed(SDL_SCANCODE_U)) {
					if (shift) {
						ToUpperCase();
					} else {
						ToLowerCase();
					}
				}
			}
		}

		InputBuffer = io.InputQueueCharacters;
		if (!IsReadOnly() && !InputBuffer.empty()) {
			InputBuffer.push_back('\0');
			std::string tmp = ImTextStrToUtf8StdStr(InputBuffer.Data, nullptr);
			char* str = (char*)tmp.c_str();
			while (*str && str < (char*)tmp.c_str() + tmp.length()) {
				int n = ImTextExpectUtf8Char(str);
				Char c = ImTextTakeUtf8Bytes(str, n);
				if (c != 0) {
					if (c == '\r')
						c = '\n';
					if (c == '\t') {
						if (IndentWithTab) {
							EnterCharacter(c);
						} else {
							for (int i = 0; i < TabSize; ++i)
								EnterCharacter(' ');
						}
					} else {
						EnterCharacter(c);
					}
				}
				str += n;
			}
		}
	}

	// Process colorization.
	ColorizeInternal();

	// Process the code lines.
	static std::string buffer; // Shared.
	static std::list<Glyph> glyphs; // Shared.
	ImVec2 contentSize = GetWindowContentRegionMax();
	ImDrawList* drawList = GetWindowDrawList();
	int appendIndex = 0;
	int longest = TextStart;

	SetHeadSize(CharAdv.x * TextStart);
	ImVec2 cursorScreenPos = GetCursorScreenPos();
	if (ToSetScrollY) {
		ToSetScrollY = false;
		SetScrollY(ScrollY);
	}
	const float scrollX = GetScrollX();
	const float scrollY = GetScrollY();
	ScrollY = scrollY;

	int dblClkLineNo = -1;
	int lineNo = (int)floor(scrollY / CharAdv.y);
	const int lineMax = std::max(0, std::min((int)CodeLines.size() - 1, lineNo + (int)ceil((scrollY + contentSize.y) / CharAdv.y)));
	if (!CodeLines.empty()) {
		// Render the safe column indicator.
		if (IsStickyLineNumbers()) {
			PushClipRect(
				ImVec2(cursorScreenPos.x + scrollX + GetHeadSize(), cursorScreenPos.y + scrollY),
				ImVec2(cursorScreenPos.x + scrollX + GetHeadSize() + aSize.x, cursorScreenPos.y + scrollY + aSize.y),
				true
			);
		}
		if (GetSafeColumnIndicatorOffset() > 0) {
			const ImVec2 istart(
				cursorScreenPos.x + CharAdv.x * TextStart + CharAdv.x * GetSafeColumnIndicatorOffset(),
				cursorScreenPos.y + CharAdv.y * lineNo
			);
			const ImVec2 iend(istart.x, istart.y + std::max(CharAdv.y * (lineMax + 1), contentSize.y));
			drawList->AddLine(istart, iend, Plt[(int)PaletteIndex::CurrentLineEdge]);
		}
		if (IsStickyLineNumbers()) {
			PopClipRect();
		}

		// Render the background of the head.
		if (IsStickyLineNumbers()) {
			const float factor = std::min(std::max(scrollX, 0.0f), CharAdv.x * 6) / (CharAdv.x * 6) * 0.15f;
			const ImVec4 col1 = ColorConvertU32ToFloat4(Plt[(int)PaletteIndex::CurrentLineFill]);
			const ImVec4 col(col1.x, col1.y, col1.z, factor);
			const ImU32 headColor = ColorConvertFloat4ToU32(col);
			drawList->AddRectFilled(
				ImVec2(cursorScreenPos.x + scrollX, cursorScreenPos.y + scrollY),
				ImVec2(cursorScreenPos.x + scrollX + GetHeadSize(), cursorScreenPos.y + scrollY + aSize.y),
				headColor
			);
		}

		// Render the code lines.
		while (lineNo <= lineMax) {
			// Prepare.
			ImVec2 lineStartScreenPos = ImVec2(cursorScreenPos.x, cursorScreenPos.y + CharAdv.y * lineNo);
			ImVec2 textScreenPos = ImVec2(lineStartScreenPos.x + CharAdv.x * TextStart, lineStartScreenPos.y);

			Line &line = CodeLines[lineNo];
			longest = std::max(TextStart + TextDistanceToLineStart(Coordinates(lineNo, (int)line.Glyphs.size())), longest);
			int columnNo = 0;
			const Coordinates lineStartCoord(lineNo, 0);
			const Coordinates lineEndCoord(lineNo, (int)line.Glyphs.size());

			int sstart = -1;
			int ssend = -1;

			assert(State.SelectionStart <= State.SelectionEnd);
			if (State.SelectionStart <= lineEndCoord)
				sstart = State.SelectionStart > lineStartCoord ? TextDistanceToLineStart(State.SelectionStart) : 0;
			if (State.SelectionEnd > lineStartCoord)
				ssend = TextDistanceToLineStart(State.SelectionEnd < lineEndCoord ? State.SelectionEnd : lineEndCoord);

			if (State.SelectionEnd.Line > lineNo)
				++ssend;

			const ImVec2 start(lineStartScreenPos.x + scrollX, lineStartScreenPos.y);
			const float lineNumberWidth = CharAdv.x * (TextStart - 1);
			const float lineStartX = IsStickyLineNumbers() ? start.x : lineStartScreenPos.x;

			// Process head clicking.
			if (dblClkLineNo == -1 && IsHeadClickEnabled() && IsEditorFocused()) {
				const float margin = 4.0f;
				const ImVec2 start_(start.x + margin, start.y);
				const ImVec2 end(lineStartX + CharAdv.x * std::min((TextStart - 1), 5), lineStartScreenPos.y + CharAdv.y);
				if (IsMouseHoveringRect(start_, end)) {
					SetMouseCursor(ImGuiMouseCursor_Hand);
					if (IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
						dblClkLineNo = lineNo;
						OnHeadClicked(lineNo, true);
					} else if (IsMouseClicked(ImGuiMouseButton_Left)) {
						OnHeadClicked(lineNo, false);
					}
				}
			}

			// Render the break points.
			auto brk = Brks.find(lineNo);
			if (brk != Brks.end()) {
				//ImVec2 end(lineStartScreenPos.x + contentSize.x + 2.0f * scrollX, lineStartScreenPos.y + CharAdv.y);
				//drawList->AddRectFilled(start, end, Plt[(int)PaletteIndex::Breakpoint]);

				const float offsetX = 2;
				const ImVec2 end(lineStartX + lineNumberWidth, lineStartScreenPos.y + CharAdv.y);
				const ImVec2 points[5] = {
					ImVec2(offsetX + start.x, start.y),
					ImVec2(end.x - CharAdv.y * 0.5f, start.y),
					ImVec2(end.x, start.y + CharAdv.y * 0.5f - 1),
					ImVec2(end.x - CharAdv.y * 0.5f, end.y),
					ImVec2(offsetX + start.x, end.y)
				};
				if (brk->second)
					drawList->AddConvexPolyFilled(points, countof(points), Plt[(int)PaletteIndex::Breakpoint]);
				else
					drawList->AddPolyline(points, countof(points), Plt[(int)PaletteIndex::Breakpoint], true, 1);
			}

			// Render the program pointer.
			if (GetProgramPointer() >= 0 && GetProgramPointer() == lineNo) {
				const float margin = brk != Brks.end() ? 1.0f : 0.0f;
				const float offsetX = 2;
				const ImVec2 end(lineStartX + lineNumberWidth, lineStartScreenPos.y + CharAdv.y);
				const ImVec2 points[5] = {
					ImVec2(offsetX + start.x + margin, start.y + margin),
					ImVec2(end.x - CharAdv.y * 0.5f - margin, start.y + margin),
					ImVec2(end.x - margin, start.y + CharAdv.y * 0.5f - 1),
					ImVec2(end.x - CharAdv.y * 0.5f - margin, end.y - margin),
					ImVec2(offsetX + start.x + margin, end.y - margin)
				};
				drawList->AddConvexPolyFilled(points, countof(points), Plt[(int)PaletteIndex::ProgramPointer]);
			}

			// Push the clip rectangle.
			if (IsStickyLineNumbers()) {
				PushClipRect(
					ImVec2(start.x + GetHeadSize(), start.y),
					ImVec2(start.x + GetHeadSize() + aSize.x, start.y + aSize.y),
					true
				);
			}

			// Render the selection.
			if (sstart != -1 && ssend != -1 && sstart < ssend) {
				const ImVec2 vstart(lineStartScreenPos.x + CharAdv.x * (sstart + TextStart), lineStartScreenPos.y);
				const ImVec2 vend(lineStartScreenPos.x + CharAdv.x * (ssend + TextStart), lineStartScreenPos.y + CharAdv.y);
				drawList->AddRectFilled(vstart, vend, Plt[(int)PaletteIndex::Selection]);
			}

			// Render the error/warning mark.
			ErrorMarkers::iterator errorIt = Errs.find(lineNo);
			if (errorIt != Errs.end()) {
				const int ln = errorIt->first;
				const Error &err = errorIt->second;
				const ImVec2 end(lineStartScreenPos.x + contentSize.x + 2.0f * scrollX, lineStartScreenPos.y + CharAdv.y);
				if (err.isWarning)
					drawList->AddRectFilled(start, end, Plt[(int)PaletteIndex::WarningMarker]);
				else
					drawList->AddRectFilled(start, end, Plt[(int)PaletteIndex::ErrorMarker]);

				if (IsErrorTipEnabled()) {
					if (IsMouseHoveringRect(lineStartScreenPos, end)) {
						PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
						BeginTooltip();
						if (err.withLineNumber) {
							if (err.isWarning) {
								Text("Warning at line %d:", ln);
							} else {
								Text("Error at line %d:", ln);
							}
							Separator();
							Text("%s", err.message.c_str());
						} else {
							if (err.isWarning) {
								Text("Warning: %s", err.message.c_str());
							} else {
								Text("Error: %s", err.message.c_str());
							}
						}
						EndTooltip();
						PopStyleVar();
					}
				}
			}

			// Render the line indicator.
			if (State.CursorPosition.Line == lineNo) {
				bool focused = IsEditorFocused();

				if (!IsReadOnly() && !HasSelection()) {
					const ImVec2 start_ = IsStickyLineNumbers() ?
						ImVec2(start.x + GetHeadSize(), start.y) :
						start;
					const ImVec2 end(start.x + contentSize.x + scrollX, start.y + CharAdv.y);
					drawList->AddRectFilled(start_, end, Plt[(int)(focused ? PaletteIndex::CurrentLineFill : PaletteIndex::CurrentLineFillInactive)]);
					drawList->AddRect(start_, end, Plt[(int)PaletteIndex::CurrentLineEdge]);
				}
			}

			// Render the code text.
			appendIndex = 0;
			unsigned prevColor = line.Glyphs.empty() ? (ImU32)PaletteIndex::Default : (line.Glyphs[0].MultiLineComment ? (ImU32)PaletteIndex::MultiLineComment : line.Glyphs[0].ColorIndex);
			const Glyph* prevGlyph = nullptr;

			int width = 0;
			int offset = 0;
			for (Glyph &glyph : line.Glyphs) {
				unsigned color = glyph.MultiLineComment ? (ImU32)PaletteIndex::MultiLineComment : glyph.ColorIndex;

				const bool sameColor = color == prevColor ||
					(glyph.Codepoint > 255 &&
						color == (unsigned)PaletteIndex::Default && prevColor == (unsigned)PaletteIndex::Identifier) ||
						((prevGlyph && prevGlyph->Codepoint > 255) &&
							color == (unsigned)PaletteIndex::Identifier && prevColor == (unsigned)PaletteIndex::Default);
				if (!sameColor && !buffer.empty()) {
					const ImU32 targetColor = prevColor >= (ImU32)PaletteIndex::Max ? prevColor : Plt[(int)prevColor];
					RenderText(offset, textScreenPos, prevColor, targetColor, buffer.c_str(), glyphs, width);
					textScreenPos.x += CharAdv.x * width;
					buffer.clear();
					glyphs.clear();
					prevColor = color;
					width = 0;
				}
				appendIndex = AppendBuffer(buffer, glyphs, glyph, appendIndex, width);
				++columnNo;
				prevGlyph = &glyph;
			}

			if (!buffer.empty()) {
				const ImU32 targetColor = prevColor >= (ImU32)PaletteIndex::Max ? prevColor : Plt[(int)prevColor];
				RenderText(offset, textScreenPos, prevColor, targetColor, buffer.c_str(), glyphs, width);
				buffer.clear();
				glyphs.clear();
			}

			// Render the line cursor.
			if (State.CursorPosition.Line == lineNo) {
				bool focused = IsEditorFocused();

				int cx = TextDistanceToLineStart(State.CursorPosition);

				if (focused) {
					static auto timeStart = std::chrono::system_clock::now(); // Shared.
					auto timeEnd = std::chrono::system_clock::now();
					auto diff = timeEnd - timeStart;
					auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(diff).count();
					const ImVec2 cstart(
						lineStartScreenPos.x + CharAdv.x * (cx + TextStart),
						lineStartScreenPos.y
					);
					if (elapsed > 400) {
						const ImVec2 cend(
							lineStartScreenPos.x + CharAdv.x * (cx + TextStart) + (Overwrite ? CharAdv.x : 1.0f),
							lineStartScreenPos.y + CharAdv.y
						);
						drawList->AddRectFilled(cstart, cend, Plt[(int)PaletteIndex::Cursor]);
						if (elapsed > 800)
							timeStart = timeEnd;
					}
					g.PlatformImePos = ImVec2(cstart.x, cstart.y + CharAdv.y);
				}
			}

			// Pop the clip rectangle.
			if (IsStickyLineNumbers()) {
				PopClipRect();
			}

			// Render the line number.
			if (IsShowLineNumbers()) {
				static char buf[16]; // Shared.
				switch (TextStart - 1) {
				case 5: snprintf(buf, countof(buf), "%4d", lineNo + 1); break;
				case 6: snprintf(buf, countof(buf), "%5d", lineNo + 1); break;
				default: snprintf(buf, countof(buf), "%6d", lineNo + 1); break;
				}
				drawList->AddText(ImVec2(lineStartX /*+ CharAdv.x * 1*/, lineStartScreenPos.y), Plt[(int)PaletteIndex::LineNumber], buf);
			}

			// Render the modification status.
			switch (line.Changed) {
			case LineState::None: // Do nothing.
				break;
			case LineState::Edited:
				drawList->AddRectFilled(
					ImVec2(lineStartX + lineNumberWidth, lineStartScreenPos.y),
					ImVec2(lineStartX + lineNumberWidth + CharAdv.x * 0.5f, lineStartScreenPos.y + CharAdv.y),
					Plt[(int)PaletteIndex::LineEdited]
				);

				break;
			case LineState::EditedSaved:
				drawList->AddRectFilled(
					ImVec2(lineStartX + lineNumberWidth, lineStartScreenPos.y),
					ImVec2(lineStartX + lineNumberWidth + CharAdv.x * 0.5f, lineStartScreenPos.y + CharAdv.y),
					Plt[(int)PaletteIndex::LineEditedSaved]
				);

				break;
			case LineState::EditedReverted:
				drawList->AddRectFilled(
					ImVec2(lineStartX + lineNumberWidth, lineStartScreenPos.y),
					ImVec2(lineStartX + lineNumberWidth + CharAdv.x * 0.5f, lineStartScreenPos.y + CharAdv.y),
					Plt[(int)PaletteIndex::LineEditedReverted]
				);

				break;
			}

			// Step one line.
			appendIndex = 0;
			lineStartScreenPos.y += CharAdv.y;
			textScreenPos.x = lineStartScreenPos.x + CharAdv.x * TextStart;
			textScreenPos.y = lineStartScreenPos.y;
			++lineNo;
		}

		// Render tooltip according to the current context.
		if (IsTooltipEnabled()) {
			std::string id = GetWordAt(ScreenPosToCoordinates(GetMousePos()));
			if (!id.empty()) {
				Identifiers::iterator it = LangDef.Ids.find(id);
				if (it != LangDef.Ids.end() && !it->second.Declaration.empty()) {
					BeginTooltip();
					TextUnformatted(it->second.Declaration.c_str());
					EndTooltip();
				} else {
					Identifiers::iterator pi = LangDef.PreprocIds.find(id);
					if (pi != LangDef.PreprocIds.end() && !pi->second.Declaration.empty()) {
						BeginTooltip();
						TextUnformatted(pi->second.Declaration.c_str());
						EndTooltip();
					}
				}
			}
		}
	}

	// Process selection.
	if (IsWindowHovered()) {
		if (!shift && !alt) {
			if (IsMouseClicked(ImGuiMouseButton_Left)) {
				State.CursorPosition = InteractiveStart = InteractiveEnd = SanitizeCoordinates(ScreenPosToCoordinates(GetMousePos()));
				if (ctrl)
					WordSelectionMode = true;
				SetSelection(InteractiveStart, InteractiveEnd, WordSelectionMode);
				EnsureCursorVisible(false, true);
				OnLineClicked(State.CursorPosition.Line, false);
				InputtedRangedPair = false;
			}
			if (IsMouseDoubleClicked(ImGuiMouseButton_Left) && !ctrl) {
				if (dblClkLineNo == -1) {
					State.CursorPosition = InteractiveStart = InteractiveEnd = SanitizeCoordinates(ScreenPosToCoordinates(GetMousePos()));
					WordSelectionMode = true;
					SetSelection(InteractiveStart, InteractiveEnd, WordSelectionMode);
					EnsureCursorVisible(false, true);
					State.CursorPosition = State.SelectionEnd;
					OnLineClicked(State.CursorPosition.Line, true);
				}
			} else if (IsMouseDragging(ImGuiMouseButton_Left) && IsMouseDown(ImGuiMouseButton_Left)) {
				io.WantCaptureMouse = true;
				State.CursorPosition = InteractiveEnd = SanitizeCoordinates(ScreenPosToCoordinates(GetMousePos()));
				SetSelection(InteractiveStart, InteractiveEnd, WordSelectionMode);
				EnsureCursorVisible(false, true);
			}
		} else if (shift) {
			if (IsMouseClicked(ImGuiMouseButton_Left)) {
				io.WantCaptureMouse = true;
				State.CursorPosition = InteractiveEnd = SanitizeCoordinates(ScreenPosToCoordinates(GetMousePos()));
				SetSelection(InteractiveStart, InteractiveEnd, WordSelectionMode);
				EnsureCursorVisible(false, true);
				InputtedRangedPair = false;
			}
		}

		if (!IsMouseDown(ImGuiMouseButton_Left)) {
			WordSelectionMode = false;
		}
	}

	// Finish.
	Dummy(ImVec2((longest + 2) * CharAdv.x, CodeLines.size() * CharAdv.y));

	if (ScrollToCursor) {
		if (ScrollToCursor == -1) {
			EnsureCursorVisible(true);
			if (!IsReadOnly())
				SetWindowFocus();
		} else {
			EnsureCursorVisible(false);
		}
		ScrollToCursor = 0;
	}

	PopAllowKeyboardFocus();
	EndChild();
	PopStyleVar();
	PopStyleColor();

	WithinRender = false;
}

void CodeEditor::SetKeyPressedHandler(const KeyPressed &aHandler) {
	KeyPressedHandler = aHandler;
}

void CodeEditor::SetColorizedHandler(const Colorized &aHandler) {
	ColorizedHandler = aHandler;
}

void CodeEditor::SetModifiedHandler(const Modified &aHandler) {
	ModifiedHandler = aHandler;
}

void CodeEditor::SetHeadClickedHandler(const HeadClicked &aHandler) {
	HeadClickedHandler = aHandler;
}

void CodeEditor::SetLineClickedHandler(const LineClicked &aHandler) {
	LineClickedHandler = aHandler;
}

bool CodeEditor::IsChangesSaved(void) const {
	return SavedIndex == UndoIndex;
}

void CodeEditor::SetChangesCleared(void) {
	for (Line &line : CodeLines) {
		if (line.Changed == LineState::Edited || line.Changed == LineState::EditedSaved || line.Changed == LineState::EditedReverted)
			line.Clear();
	}
}

void CodeEditor::SetChangesSaved(void) {
	SavedIndex = UndoIndex;

	for (Line &line : CodeLines) {
		if (line.Changed == LineState::Edited || line.Changed == LineState::EditedReverted)
			line.Save();
	}
}

void CodeEditor::SetText(const std::string &aText) {
	CodeLines.clear();
	char* str = (char*)aText.c_str();
	while (str < (char*)aText.c_str() + aText.length()) {
		int n = ImTextExpectUtf8Char(str);
		if (n == 0)
			n = 1; // 1 byte forward when meets invalid character.
		Char c = ImTextTakeUtf8Bytes(str, n);
		if (CodeLines.empty())
			CodeLines.push_back(Line());
		if (c == '\n')
			CodeLines.push_back(Line());
		else
			CodeLines.back().Glyphs.push_back(Glyph(c, PaletteIndex::Default));
		str += n;
	}
	if (CodeLines.empty())
		CodeLines.push_back(Line());

	ClearUndoRedoStack();

	Colorize();
}

std::string CodeEditor::GetText(const char* aNewline) const {
	return GetText(Coordinates(), Coordinates((int)CodeLines.size(), 0), aNewline);
}

std::vector<std::string> CodeEditor::GetTextLines(bool aIncludeComment, bool aIncludeString) const {
	std::vector<std::string> result;
	for (const Line &ln : CodeLines) {
		result.push_back(std::string());
		std::string &str = result.back();
		for (const Glyph &g : ln.Glyphs) {
			const bool multilinecomment =
				g.ColorIndex == (ImU32)PaletteIndex::Comment || g.ColorIndex == (ImU32)PaletteIndex::MultiLineComment ||
				ln.Glyphs[0].MultiLineComment;
			if (!aIncludeComment && multilinecomment)
				continue;
			if (!aIncludeString && (g.ColorIndex == (ImU32)PaletteIndex::String))
				continue;

			ImTextAppendUtf8ToStdStr(str, g.Character);
		}
	}

	return result;
}

std::string CodeEditor::GetTextLine(int aLine) const {
	return GetText(Coordinates(aLine, 0), Coordinates(aLine, GetColumnsAt(aLine)), nullptr);
}

std::vector<std::pair<std::string, CodeEditor::PaletteIndex> > CodeEditor::GetWordsAtLine(int aLine, bool aIncludeComment, bool aIncludeString, bool aIncludeSpace) const {
	std::vector<std::pair<std::string, PaletteIndex> > result;

	if (aLine < 0 || aLine >= (int)CodeLines.size())
		return result;

	const Line &line = CodeLines[aLine];
	if (line.Glyphs.empty())
		return result;

	auto append = [&result, &line] (PaletteIndex &cstart, std::string &word, int col) -> void {
		if (!word.empty())
			result.push_back(std::make_pair(word, cstart));

		if (col >= 0 && col < (int)line.Glyphs.size())
			cstart = (PaletteIndex)line.Glyphs[col].ColorIndex;
		word.clear();
	};

	int col = 0;
	PaletteIndex cstart = (PaletteIndex)line.Glyphs[col].ColorIndex;
	std::string word;
	while (col < (int)line.Glyphs.size()) {
		const Glyph &g = line.Glyphs[col];
		if (((PaletteIndex)g.ColorIndex == PaletteIndex::Comment || (PaletteIndex)g.ColorIndex == PaletteIndex::MultiLineComment) && !aIncludeComment) {
			if (cstart != (PaletteIndex)g.ColorIndex)
				append(cstart, word, col);

			++col;

			continue;
		}

		if ((PaletteIndex)g.ColorIndex == PaletteIndex::String && !aIncludeString) {
			if (cstart != (PaletteIndex)g.ColorIndex)
				append(cstart, word, col);

			++col;

			continue;
		}

		if ((g.Character == ' ' || g.Character == '\t') && !aIncludeSpace) {
			if (cstart != (PaletteIndex)g.ColorIndex)
				append(cstart, word, col);

			++col;

			continue;
		}

		if (cstart != (PaletteIndex)g.ColorIndex)
			append(cstart, word, col);

		ImTextAppendUtf8ToStdStr(word, g.Character);

		++col;
	}
	append(cstart, word, col);

	return result;
}

void CodeEditor::InsertText(const char* aValue) {
	if (aValue == nullptr)
		return;

	Coordinates pos = GetActualCursorCoordinates();
	Coordinates start = std::min(pos, State.SelectionStart);
	int totalLines = pos.Line - start.Line;

	totalLines += InsertTextAt(pos, aValue);

	SetSelection(pos, pos);
	SetCursorPosition(pos);
	Colorize(start.Line - 1, totalLines + 2);
}

void CodeEditor::AppendText(const char* aText, ImU32 aColor) {
	if (!aText)
		return;

	while (*aText) {
		int n = ImTextExpectUtf8Char(aText);
		Char c = ImTextTakeUtf8Bytes(aText, n);
		if (c != 0) {
			if (CodeLines.empty())
				CodeLines.push_back(Line());

			if (c == '\r' || c == '\n') {
				CodeLines.push_back(Line());
			} else {
				Line &line = CodeLines.back();
				line.Glyphs.push_back(Glyph(c, aColor));
			}
		}
		aText += n;
		if (n == 0)
			break;
	}
}

int CodeEditor::GetTotalLines(void) const {
	return (int)CodeLines.size();
}

int CodeEditor::GetColumnsAt(int aLine) const {
	if (aLine < 0 || aLine >= (int)CodeLines.size())
		return 0;

	const Line &l = CodeLines[aLine];

	return (int)l.Glyphs.size();
}

int CodeEditor::GetTotalTokens(void) const {
	int result = 0;
	ImU32 index = 0;
	for (const Line &l : CodeLines) {
		for (const Glyph &g : l.Glyphs) {
			if (g.ColorIndex == (ImU32)PaletteIndex::Space || g.ColorIndex == (ImU32)PaletteIndex::Default)
				continue;

			if (result == 0) {
				result = 1;
				index = g.ColorIndex;
			} else {
				if (index != g.ColorIndex) {
					++result;
					index = g.ColorIndex;
				}
			}
		}
	}

	return result;
}

void CodeEditor::SetCursorPosition(const Coordinates &aPosition) {
	if (State.CursorPosition != aPosition) {
		State.CursorPosition = aPosition;
		EnsureCursorVisible();
	}
}

CodeEditor::Coordinates CodeEditor::GetCursorPosition(void) const {
	return GetActualCursorCoordinates();
}

void CodeEditor::EnsureCursorVisible(bool aForceAbove, bool aSlowMode) {
	if (!WithinRender) {
		ScrollToCursor = aForceAbove ? -1 : 1;

		return;
	}

	float scrollX = GetScrollX();
	float scrollY = GetScrollY();

	float width = GetWindowWidth();
	float height = GetWindowHeight();

	int top = 1 + (int)ceil(scrollY / CharAdv.y);
	int bottom = (int)ceil((scrollY + height) / CharAdv.y);

	int left = (int)ceil(scrollX / CharAdv.x);
	int right = (int)ceil((scrollX + width) / CharAdv.x);

	Coordinates pos = GetActualCursorCoordinates();
	int len = TextDistanceToLineStart(pos);

	const int stepUp = 1;
	const int stepDown = aSlowMode ? 2 : 4;
	const int stepRight = aSlowMode ? 3 : 4;

	if (pos.Line < top || aForceAbove)
		SetScrollY(std::max(0.0f, (pos.Line - stepUp) * CharAdv.y));
	else if (pos.Line > bottom - stepDown)
		SetScrollY(std::max(0.0f, (pos.Line + stepDown) * CharAdv.y - height));
	if (len < left)
		SetScrollX(std::max(0.0f, len * CharAdv.x));
	else if (len + TextStart > right - stepRight)
		SetScrollX(std::max(0.0f, (len + TextStart + stepRight) * CharAdv.x - width));
}

float CodeEditor::GetScrollPositionY(void) {
	return ScrollY;
}

void CodeEditor::SetScrollPositionY(float val) {
	ScrollY = val;
	ToSetScrollY = true;
}

void CodeEditor::SetIndentWithTab(bool aValue) {
	IndentWithTab = aValue;
}

bool CodeEditor::GetIndentWithTab(void) const {
	return IndentWithTab;
}

void CodeEditor::SetTabSize(int aValue) {
	TabSize = aValue;
}

int CodeEditor::GetTabSize(void) const {
	return TabSize;
}

void CodeEditor::SetHeadSize(float aValue) {
	HeadSize = aValue;
}

float CodeEditor::GetHeadSize(void) const {
	return HeadSize;
}

void CodeEditor::SetOverwrite(bool aValue) {
	Overwrite = aValue;
}

bool CodeEditor::IsOverwrite(void) const {
	return Overwrite;
}

void CodeEditor::SetReadOnly(bool aValue) {
	ReadOnly = aValue;
}

bool CodeEditor::IsReadOnly(void) const {
	return ReadOnly;
}

void CodeEditor::SetShowLineNumbers(bool aValue) {
	ShowLineNumbers = aValue;
}

bool CodeEditor::IsShowLineNumbers(void) const {
	return ShowLineNumbers;
}


void CodeEditor::SetStickyLineNumbers(bool aValue) {
	StickyLineNumbers = aValue;
}

bool CodeEditor::IsStickyLineNumbers(void) const {
	return StickyLineNumbers;
}

void CodeEditor::SetHeadClickEnabled(bool aValue) {
	HeadClickEnabled = aValue;
}

bool CodeEditor::IsHeadClickEnabled(void) const {
	return HeadClickEnabled;
}

void CodeEditor::EnableShortcut(ShortcutType aType) {
	ShortcutsEnabled = (ShortcutType)(ShortcutsEnabled | aType);
}

void CodeEditor::DisableShortcut(ShortcutType aType) {
	ShortcutsEnabled = (ShortcutType)(ShortcutsEnabled & ~aType);
}

bool CodeEditor::IsShortcutsEnabled(ShortcutType aType) const {
	return !!(ShortcutsEnabled & aType);
}

void CodeEditor::SetErrorTipEnabled(bool aValue) {
	ErrorTipEnabled = aValue;
}

bool CodeEditor::IsErrorTipEnabled(void) const {
	return ErrorTipEnabled;
}

void CodeEditor::SetTooltipEnabled(bool aValue) {
	TooltipEnabled = aValue;
}

bool CodeEditor::IsTooltipEnabled(void) const {
	return TooltipEnabled;
}

void CodeEditor::SetShowWhiteSpaces(bool aValue) {
	ShowWhiteSpaces = aValue;
}

bool CodeEditor::IsShowWhiteSpaces(void) const {
	return ShowWhiteSpaces;
}

void CodeEditor::SetSafeColumnIndicatorOffset(int aValue) {
	SafeColumnIndicatorOffset = aValue;
}

int CodeEditor::GetSafeColumnIndicatorOffset(void) const {
	return SafeColumnIndicatorOffset;
}

bool CodeEditor::IsEditorFocused(void) const {
	return EditorFocused;
}

void CodeEditor::MoveUp(int aAmount, bool aSelect) {
	Coordinates oldPos = State.CursorPosition;
	State.CursorPosition.Line = std::max(0, State.CursorPosition.Line - aAmount);
	if (oldPos != State.CursorPosition) {
		if (aSelect) {
			if (oldPos == InteractiveStart) {
				InteractiveStart = State.CursorPosition;
			} else if (oldPos == InteractiveEnd) {
				InteractiveEnd = State.CursorPosition;
			} else {
				InteractiveStart = State.CursorPosition;
				InteractiveEnd = oldPos;
			}
		} else {
			InteractiveStart = InteractiveEnd = State.CursorPosition;
		}
		SetSelection(InteractiveStart, InteractiveEnd);

		EnsureCursorVisible();
	}

	InputtedRangedPair = false;
}

void CodeEditor::MoveDown(int aAmount, bool aSelect) {
	assert(State.CursorPosition.Column >= 0);
	Coordinates oldPos = State.CursorPosition;
	State.CursorPosition.Line = std::max(0, std::min((int)CodeLines.size() - 1, State.CursorPosition.Line + aAmount));

	if (State.CursorPosition != oldPos) {
		if (aSelect) {
			if (oldPos == InteractiveEnd) {
				InteractiveEnd = State.CursorPosition;
			} else if (oldPos == InteractiveStart) {
				InteractiveStart = State.CursorPosition;
			} else {
				InteractiveStart = oldPos;
				InteractiveEnd = State.CursorPosition;
			}
		} else {
			InteractiveStart = InteractiveEnd = State.CursorPosition;
		}
		SetSelection(InteractiveStart, InteractiveEnd);

		EnsureCursorVisible();
	}

	InputtedRangedPair = false;
}

void CodeEditor::MoveLeft(int aAmount, bool aSelect, bool aWordMode) {
	if (CodeLines.empty())
		return;

	Coordinates oldPos = State.CursorPosition;
	State.CursorPosition = GetActualCursorCoordinates();

	auto moveLeft = [this] (bool aWordMode) -> void {
		if (State.CursorPosition.Column == 0) {
			if (State.CursorPosition.Line > 0) {
				--State.CursorPosition.Line;
				State.CursorPosition.Column = (int)CodeLines[State.CursorPosition.Line].Glyphs.size();
			}
		} else {
			State.CursorPosition.Column = std::max(0, State.CursorPosition.Column - 1);
			if (aWordMode)
				State.CursorPosition = FindWordStart(State.CursorPosition);
		}
	};

	while (aAmount-- > 0) {
		moveLeft(aWordMode);
	}
	if (State.CursorPosition == oldPos) {
		moveLeft(false);
	}

	assert(State.CursorPosition.Column >= 0);
	if (aSelect) {
		if (oldPos == InteractiveStart) {
			InteractiveStart = State.CursorPosition;
		} else if (oldPos == InteractiveEnd) {
			InteractiveEnd = State.CursorPosition;
		} else {
			InteractiveStart = State.CursorPosition;
			InteractiveEnd = oldPos;
		}
	} else {
		InteractiveStart = InteractiveEnd = State.CursorPosition;
	}
	SetSelection(InteractiveStart, InteractiveEnd, aSelect && aWordMode);

	EnsureCursorVisible();

	InputtedRangedPair = false;
}

void CodeEditor::MoveRight(int aAmount, bool aSelect, bool aWordMode) {
	Coordinates oldPos = State.CursorPosition;

	if (CodeLines.empty())
		return;

	auto moreRight = [this] (bool aWordMode) -> void {
		const Line &line = CodeLines[State.CursorPosition.Line];
		if (State.CursorPosition.Column >= (int)line.Glyphs.size()) {
			if (State.CursorPosition.Line < (int)CodeLines.size() - 1) {
				State.CursorPosition.Line = std::max(0, std::min((int)CodeLines.size() - 1, State.CursorPosition.Line + 1));
				State.CursorPosition.Column = 0;
			}
		} else {
			State.CursorPosition.Column = std::max(0, std::min((int)line.Glyphs.size(), State.CursorPosition.Column + 1));
			if (aWordMode)
				State.CursorPosition = FindWordEnd(State.CursorPosition);
		}
	};

	while (aAmount-- > 0) {
		moreRight(aWordMode);
	}
	if (State.CursorPosition == oldPos) {
		moreRight(false);
	}

	if (aSelect) {
		if (oldPos == InteractiveEnd) {
			InteractiveEnd = SanitizeCoordinates(State.CursorPosition);
		} else if (oldPos == InteractiveStart) {
			InteractiveStart = State.CursorPosition;
		} else {
			InteractiveStart = oldPos;
			InteractiveEnd = State.CursorPosition;
		}
	} else {
		InteractiveStart = InteractiveEnd = State.CursorPosition;
	}
	SetSelection(InteractiveStart, InteractiveEnd, aSelect && aWordMode);

	EnsureCursorVisible();

	InputtedRangedPair = false;
}

void CodeEditor::MoveTop(bool aSelect) {
	Coordinates oldPos = State.CursorPosition;
	SetCursorPosition(Coordinates(0, 0));

	if (State.CursorPosition != oldPos && aSelect) {
		InteractiveEnd = oldPos;
		InteractiveStart = State.CursorPosition;
	} else {
		InteractiveStart = InteractiveEnd = State.CursorPosition;
	}
	SetSelection(InteractiveStart, InteractiveEnd);

	InputtedRangedPair = false;
}

void CodeEditor::CodeEditor::MoveBottom(bool aSelect) {
	Coordinates oldPos = GetCursorPosition();
	Coordinates newPos((int)CodeLines.size() - 1, (int)CodeLines.back().Glyphs.size());
	SetCursorPosition(newPos);
	if (aSelect) {
		InteractiveStart = oldPos;
		InteractiveEnd = newPos;
	} else {
		InteractiveStart = InteractiveEnd = newPos;
	}
	SetSelection(InteractiveStart, InteractiveEnd);

	InputtedRangedPair = false;
}

void CodeEditor::MoveHome(bool aSelect) {
	Coordinates oldPos = State.CursorPosition;
	int to = 0;
	if (oldPos.Line < (int)CodeLines.size()) {
		int head = 0;
		const Line &line = CodeLines[oldPos.Line];
		for (int i = 0; i < (int)line.Glyphs.size(); ++i) {
			const Glyph &g = line.Glyphs[i];
			if (g.Character == ' ' || g.Character == '\t')
				head = i + 1;
			else
				break;
		}
		if (head != 0) {
			if (oldPos.Column != head)
				to = head;
		}
	}
	if (HasSelection()) {
		Coordinates selStart, selEnd;
		GetSelection(selStart, selEnd);
		SetCursorPosition(Coordinates(selStart.Line, to));
	} else {
		SetCursorPosition(Coordinates(State.CursorPosition.Line, to));
	}

	if (State.CursorPosition != oldPos && aSelect) {
		if (oldPos == InteractiveStart) {
			InteractiveStart = State.CursorPosition;
		} else if (oldPos == InteractiveEnd) {
			InteractiveEnd = State.CursorPosition;
		} else {
			InteractiveStart = State.CursorPosition;
			InteractiveEnd = oldPos;
		}
	} else {
		InteractiveStart = InteractiveEnd = State.CursorPosition;
	}
	if (State.CursorPosition != oldPos || !aSelect)
		SetSelection(InteractiveStart, InteractiveEnd);

	InputtedRangedPair = false;
}

void CodeEditor::MoveEnd(bool aSelect) {
	Coordinates oldPos = State.CursorPosition;
	if (HasSelection()) {
		Coordinates selStart, selEnd;
		GetSelection(selStart, selEnd);
		SetCursorPosition(Coordinates(selEnd.Line, (int)CodeLines[selEnd.Line].Glyphs.size()));
	} else {
		SetCursorPosition(Coordinates(State.CursorPosition.Line, (int)CodeLines[oldPos.Line].Glyphs.size()));
	}

	if (State.CursorPosition != oldPos && aSelect) {
		if (oldPos == InteractiveEnd) {
			InteractiveEnd = State.CursorPosition;
		} else if (oldPos == InteractiveStart) {
			InteractiveStart = State.CursorPosition;
		} else {
			InteractiveStart = oldPos;
			InteractiveEnd = State.CursorPosition;
		}
	} else {
		InteractiveStart = InteractiveEnd = State.CursorPosition;
	}
	if (State.CursorPosition != oldPos || !aSelect)
		SetSelection(InteractiveStart, InteractiveEnd);

	InputtedRangedPair = false;
}

std::string CodeEditor::GetWordUnderCursor(Coordinates* aStart, Coordinates* aEnd) const {
	const Coordinates c = GetCursorPosition();

	return GetWordAt(c, aStart, aEnd);
}

void CodeEditor::SetSelectionStart(const Coordinates &aPosition) {
	State.SelectionStart = SanitizeCoordinates(aPosition);
	if (State.SelectionStart > State.SelectionEnd)
		std::swap(State.SelectionStart, State.SelectionEnd);
}

void CodeEditor::SetSelectionEnd(const Coordinates &aPosition) {
	State.SelectionEnd = SanitizeCoordinates(aPosition);
	if (State.SelectionStart > State.SelectionEnd)
		std::swap(State.SelectionStart, State.SelectionEnd);
}

void CodeEditor::SetSelection(const Coordinates &aStart, const Coordinates &aEnd, bool aWordMode) {
	State.SelectionStart = SanitizeCoordinates(aStart);
	State.SelectionEnd = SanitizeCoordinates(aEnd);
	if (State.SelectionStart > State.SelectionEnd)
		std::swap(State.SelectionStart, State.SelectionEnd);

	if (aWordMode) {
		State.SelectionStart = FindWordStart(State.SelectionStart);
		//if (!IsOnWordBoundary(State.SelectionEnd))
		State.SelectionEnd = FindWordEnd(FindWordStart(State.SelectionEnd));
		if (State.SelectionStart > State.SelectionEnd)
			std::swap(State.SelectionStart, State.SelectionEnd);
	}
}

void CodeEditor::SelectWordUnderCursor(void) {
	const Coordinates c = GetCursorPosition();
	SetSelection(FindWordStart(c), FindWordEnd(c));
}

void CodeEditor::SelectWordUnderMouse(void) {
	State.CursorPosition = InteractiveStart = InteractiveEnd = SanitizeCoordinates(ScreenPosToCoordinates(GetMousePos()));
	WordSelectionMode = true;
	SetSelection(InteractiveStart, InteractiveEnd, WordSelectionMode);
}

void CodeEditor::SelectAll(void) {
	SetSelection(Coordinates(0, 0), Coordinates((int)CodeLines.size(), 0));
}

bool CodeEditor::HasSelection(void) const {
	return State.SelectionEnd > State.SelectionStart;
}

void CodeEditor::ClearSelection(void) {
	State.SelectionEnd = State.SelectionStart;
}

void CodeEditor::GetSelection(Coordinates &aStart, Coordinates &aEnd) {
	aStart = State.SelectionStart;
	aEnd = State.SelectionEnd;
}

std::string CodeEditor::GetSelectionText(const char* aNewline) const {
	return GetText(State.SelectionStart, State.SelectionEnd, aNewline);
}

int CodeEditor::GetSelectionLines(void) const {
	if (!HasSelection())
		return 0;

	return std::abs(State.SelectionEnd.Line - State.SelectionStart.Line) + 1;
}

int CodeEditor::GetNonEmptySelectionLines(void) const {
	if (!HasSelection())
		return 0;

	int result = 0;
	const int start = std::min(State.SelectionStart.Line, State.SelectionEnd.Line);
	const int end = std::max(State.SelectionStart.Line, State.SelectionEnd.Line);
	for (int i = start; i <= end; ++i) {
		if (!CodeLines[i].Glyphs.empty())
			++result;
	}

	return result;
}

int CodeEditor::GetCommentLines(void) const {
	int commentedLines = 0;
	for (int i = State.SelectionStart.Line; i <= State.SelectionEnd.Line; ++i) {
		const Line &line = CodeLines[i];
		if (line.Glyphs.empty())
			continue;

		bool match = true;
		if (line.Glyphs.size() >= LangDef.SimpleCommentHead.size()) {
			for (int j = 0; j < (int)LangDef.SimpleCommentHead.length(); ++j) {
				Char cch = LangDef.SimpleCommentHead[j];
				const Glyph &g = line.Glyphs[j];
				if (cch != g.Character) {
					match = false;

					break;
				}
			}
		} else {
			match = false;
		}
		if (match) {
			++commentedLines;
		}
	}

	return commentedLines;
}

void CodeEditor::Copy(void) {
	if (HasSelection()) {
		SetClipboardText(GetSelectionText().c_str());
	} else {
		if (!CodeLines.empty()) {
			std::string str;
			const Line &line = CodeLines[GetActualCursorCoordinates().Line];
			for (const Glyph &g : line.Glyphs) {
				ImTextAppendUtf8ToStdStr(str, g.Character);
			}
			SetClipboardText(str.c_str());
		}
	}
}

void CodeEditor::Cut(void) {
	if (IsReadOnly()) {
		Copy();
	} else {
		if (HasSelection()) {
			UndoRecord u;
			u.Type = UndoType::Remove;
			u.Before = State;

			u.Content = GetSelectionText();
			u.Start = State.SelectionStart;
			u.End = State.SelectionEnd;

			Copy();
			DeleteSelection();

			u.After = State;
			AddUndo(u);

			OnModified(false, true);

			Coordinates pos = u.Start < u.End ? u.Start : u.End;
			OnChanged(pos, pos, 0, true);
		}
	}
}

void CodeEditor::Paste(void) {
	const char* const clipText = GetClipboardText();
	Paste(clipText);
}

void CodeEditor::Paste(const char* aTxt) {
	if (aTxt != nullptr && strlen(aTxt) > 0) {
		UndoRecord u;
		u.Type = UndoType::Add;
		u.Before = State;

		if (HasSelection()) {
			u.Overwritten = GetSelectionText();
			DeleteSelection();
		}

		u.Content = aTxt;
		u.Start = GetActualCursorCoordinates();

		InsertText(aTxt);

		u.End = GetActualCursorCoordinates();
		u.After = State;
		AddUndo(u);

		OnModified(false, true);

		OnChanged(u.Start, u.End, 0, true);

		InteractiveStart = InteractiveEnd = State.CursorPosition;
	}
}

void CodeEditor::Delete(void) {
	assert(!ReadOnly);

	if (CodeLines.empty())
		return;

	UndoRecord u;
	u.Type = UndoType::Remove;
	u.Before = State;

	if (HasSelection()) {
		u.Content = GetSelectionText();
		u.Start = State.SelectionStart;
		u.End = State.SelectionEnd;
		DeleteSelection();

		Coordinates pos = State.SelectionStart < State.SelectionEnd ? State.SelectionStart : State.SelectionEnd;
		OnChanged(pos, pos, 0, true);
	} else {
		Coordinates pos = GetActualCursorCoordinates();
		SetCursorPosition(pos);
		Line &line = CodeLines[pos.Line];

		if (pos.Column == (int)line.Glyphs.size()) {
			if (pos.Line == (int)CodeLines.size() - 1)
				return;

			u.Content = '\n';
			u.Start = u.End = GetActualCursorCoordinates();
			Advance(u.End);

			const Line &nextLine = CodeLines[pos.Line + 1];
			line.Glyphs.insert(line.Glyphs.end(), nextLine.Glyphs.begin(), nextLine.Glyphs.end());
			RemoveLine(pos.Line + 1);
		} else {
			u.Content.clear();
			ImTextAppendUtf8ToStdStr(u.Content, line.Glyphs[pos.Column].Character);
			u.Start = u.End = GetActualCursorCoordinates();
			u.End.Column++;

			line.Glyphs.erase(line.Glyphs.begin() + pos.Column);
		}

		Colorize(pos.Line, 1);

		OnChanged(pos, pos, 0, true);
	}

	u.After = State;
	AddUndo(u);

	OnModified(false, true);
}

void CodeEditor::DeleteLine(void) {
	assert(!ReadOnly);

	if (CodeLines.empty())
		return;
	if (CodeLines.front().Glyphs.empty())
		return;

	UndoRecord u;
	u.Type = UndoType::Remove;
	u.Before = State;

	if (HasSelection()) {
		u.Content = GetSelectionText();
		u.Start = State.SelectionStart;
		u.End = State.SelectionEnd;
		DeleteSelection();

		Coordinates pos = State.SelectionStart < State.SelectionEnd ? State.SelectionStart : State.SelectionEnd;
		OnChanged(pos, pos, 0, true);
	} else {
		Coordinates pos = GetActualCursorCoordinates();
		SetCursorPosition(pos);
		Line &line = CodeLines[pos.Line];

		State.SelectionStart = Coordinates(pos.Line, 0);
		if (pos.Line < (int)CodeLines.size() - 1)
			State.SelectionEnd = Coordinates(pos.Line + 1, 0);
		else
			State.SelectionEnd = Coordinates(pos.Line, (int)line.Glyphs.size());

		if (State.SelectionStart == State.SelectionEnd)
			return;

		u.Content = GetSelectionText();
		u.Start = State.SelectionStart;
		u.End = State.SelectionEnd;
		DeleteSelection();

		pos = State.SelectionStart < State.SelectionEnd ? State.SelectionStart : State.SelectionEnd;
		OnChanged(pos, pos, 0, true);
	}

	u.After = State;
	AddUndo(u);

	OnModified(false, true);
}

void CodeEditor::Indent(bool aByKey) {
	if (IsReadOnly())
		return;

	if ((HasSelection() && GetSelectionLines() > 1) || !aByKey) {
		UndoRecord u;
		u.Type = UndoType::Indent;
		u.Before = State;

		u.Start = State.SelectionStart;
		u.End = State.SelectionEnd;

		for (int i = u.Start.Line; i <= u.End.Line; ++i) {
			Line &line = CodeLines[i];
			if (line.Glyphs.empty()) {
				if (i == u.Start.Line)
					u.Content.push_back(0);

				continue;
			}
			if (IndentWithTab) {
				line.Glyphs.insert(line.Glyphs.begin(), Glyph('\t', PaletteIndex::Default));
				if (i == u.Start.Line)
					u.Content.push_back('\t');
			} else {
				for (int n = 0; n < TabSize; ++n) {
					line.Glyphs.insert(line.Glyphs.begin(), Glyph(' ', PaletteIndex::Default));
					if (i == u.Start.Line)
						u.Content.push_back(' ');
				}
			}

			Coordinates pos(i, 0);
			OnChanged(pos, pos, 0, true);
		}

		switch (GetSelectionLines()) {
		case 0:
			// Do nothing.

			break;
		case 1: {
				const int step = (int)u.Content.size();
				State.CursorPosition.Column += step;
				State.SelectionStart.Column += step;
				State.SelectionEnd.Column += step;
			}

			break;
		default:
			State.SelectionEnd.Column = (int)CodeLines[State.SelectionEnd.Line].Glyphs.size();
			if (State.SelectionStart > State.SelectionEnd)
				std::swap(State.SelectionStart, State.SelectionEnd);

			break;
		}

		u.After = State;
		AddUndo(u);

		OnModified(false, true);
	}
}

void CodeEditor::Unindent(bool aByKey) {
	(void)aByKey;

	if (IsReadOnly())
		return;

	UndoRecord u;
	u.Type = UndoType::Unindent;
	u.Before = State;

	u.Start = State.SelectionStart;
	u.End = State.SelectionEnd;

	int affectedLines = 0;
	for (int i = u.Start.Line; i <= u.End.Line; ++i) {
		Line &line = CodeLines[i];
		if (line.Glyphs.empty()) {
			if (i == u.Start.Line)
				u.Content.push_back(0);

			continue;
		}

		const Glyph &g = *line.Glyphs.begin();
		if (g.Character == '\t') {
			line.Glyphs.erase(line.Glyphs.begin());
			if (i == u.Start.Line)
				u.Content.push_back('\t');
			++affectedLines;

			Coordinates pos(i, 0);
			OnChanged(pos, pos, 0, true);
		} else if (g.Character == ' ') {
			int k = 0;
			for (int j = 0; j < TabSize; ++j, ++k) {
				if (line.Glyphs.empty())
					break;

				const Glyph &h = *line.Glyphs.begin();
				if (h.Character != ' ')
					break;

				line.Glyphs.erase(line.Glyphs.begin());
				if (i == u.Start.Line)
					u.Content.push_back(' ');
			}
			if (k)
				++affectedLines;

			Coordinates pos(i, 0);
			OnChanged(pos, pos, 0, true);
		} else {
			if (i == u.Start.Line)
				u.Content.push_back(0);
		}
	}

	switch (GetSelectionLines()) {
	case 0:
		// Do nothing.

		break;
	case 1: {
			const bool valid = !u.Content.empty() && u.Content.front() != '\0';
			if (!valid)
				break;

			const int step = (int)u.Content.size();
			State.CursorPosition.Column -= step;
			State.SelectionStart.Column -= step;
			State.SelectionEnd.Column -= step;
			if (State.SelectionStart > State.SelectionEnd)
				std::swap(State.SelectionStart, State.SelectionEnd);
		}

		break;
	default:
		if (affectedLines > 0) {
			const Line &line = CodeLines[State.SelectionEnd.Line];
			if ((int)line.Glyphs.size() < State.SelectionEnd.Column) {
				State.SelectionEnd.Column = (int)line.Glyphs.size();
				if (State.SelectionStart > State.SelectionEnd)
					std::swap(State.SelectionStart, State.SelectionEnd);
			}
		}

		break;
	}

	u.After = State;
	if (affectedLines > 0) {
		AddUndo(u);

		OnModified(false, true);
	}
}

void CodeEditor::ToLowerCase(void) {
	if (!HasSelection())
		return;

	UndoRecord u;
	u.Type = UndoType::ToLowerCase;
	u.Before = State;

	std::string txt = GetSelectionText();
	u.Overwritten = txt;
	DeleteSelection();

	ImTextToLowerCase(txt);
	u.Content = txt;
	u.Start = GetActualCursorCoordinates();

	InsertText(txt.c_str());

	u.End = GetActualCursorCoordinates();
	u.After = State;
	AddUndo(u);
	State = u.Before;

	OnModified(false, true);

	OnChanged(u.Start, u.End, 0, true);

	InteractiveStart = InteractiveEnd = State.CursorPosition;
}

void CodeEditor::ToUpperCase(void) {
	if (!HasSelection())
		return;

	UndoRecord u;
	u.Type = UndoType::ToUpperCase;
	u.Before = State;

	std::string txt = GetSelectionText();
	u.Overwritten = txt;
	DeleteSelection();

	ImTextToUpperCase(txt);
	u.Content = txt;
	u.Start = GetActualCursorCoordinates();

	InsertText(txt.c_str());

	u.End = GetActualCursorCoordinates();
	u.After = State;
	AddUndo(u);
	State = u.Before;

	OnModified(false, true);

	OnChanged(u.Start, u.End, 0, true);

	InteractiveStart = InteractiveEnd = State.CursorPosition;
}

void CodeEditor::Comment(void) {
	if (IsReadOnly())
		return;

	if (LangDef.SimpleCommentHead.empty())
		return;

	UndoRecord u;
	u.Type = UndoType::Comment;
	u.Before = State;

	u.Start = State.SelectionStart;
	u.End = State.SelectionEnd;

	for (int i = u.Start.Line; i <= u.End.Line; ++i) {
		Line &line = CodeLines[i];
		const int m = (int)line.Glyphs.size();

		if (line.Glyphs.empty()) {
			for (int j = (int)LangDef.SimpleCommentHead.length() - 1; j >= 0; --j)
				line.Glyphs.insert(line.Glyphs.begin(), Glyph(LangDef.SimpleCommentHead[j], PaletteIndex::Comment));
			u.Content.push_back(1);
		} else {
			line.Glyphs.insert(line.Glyphs.begin(), Glyph(' ', PaletteIndex::Default));
			for (int j = (int)LangDef.SimpleCommentHead.length() - 1; j >= 0; --j)
				line.Glyphs.insert(line.Glyphs.begin(), Glyph(LangDef.SimpleCommentHead[j], PaletteIndex::Comment));
			u.Content.push_back(2);
		}

		const int diff = (int)line.Glyphs.size() - m;
		if (diff > 0) {
			if (State.CursorPosition.Line == i) {
				State.CursorPosition.Column += diff;
			}
			if (State.SelectionStart.Line == i) {
				State.SelectionStart.Column += diff;
			}
			if (State.SelectionEnd.Line == i) {
				State.SelectionEnd.Column += diff;
			}
		}

		Coordinates pos(i, 0);
		OnChanged(pos, pos, 0, true);
	}

	if (State.SelectionStart > State.SelectionEnd)
		std::swap(State.SelectionStart, State.SelectionEnd);

	Colorize(State.SelectionStart.Line, State.SelectionEnd.Line - State.SelectionStart.Line + 1);

	u.After = State;
	AddUndo(u);

	OnModified(false, true);
}

void CodeEditor::Uncomment(void) {
	if (IsReadOnly())
		return;

	if (LangDef.SimpleCommentHead.empty())
		return;

	UndoRecord u;
	u.Type = UndoType::Uncomment;
	u.Before = State;

	u.Start = State.SelectionStart;
	u.End = State.SelectionEnd;

	int affectedLines = 0;
	for (int i = u.Start.Line; i <= u.End.Line; ++i) {
		Line &line = CodeLines[i];
		const int m = (int)line.Glyphs.size();

		if (line.Glyphs.empty()) {
			u.Content.push_back(0);

			continue;
		}

		bool match = true;
		if (line.Glyphs.size() >= LangDef.SimpleCommentHead.size()) {
			for (int j = 0; j < (int)LangDef.SimpleCommentHead.length(); ++j) {
				Char cch = LangDef.SimpleCommentHead[j];
				const Glyph &g = line.Glyphs[j];
				if (cch != g.Character) {
					match = false;

					break;
				}
			}
		} else {
			match = false;
		}
		if (match) {
			int n = 0;
			for (int j = 0; j < (int)LangDef.SimpleCommentHead.length(); ++j)
				line.Glyphs.erase(line.Glyphs.begin());
			++n;

			if (!line.Glyphs.empty()) {
				const Glyph &g_ = *line.Glyphs.begin();
				if (g_.Character == ' ') {
					line.Glyphs.erase(line.Glyphs.begin());
					++n;
				}
			}

			u.Content.push_back((std::string::value_type)n);
			++affectedLines;

			Coordinates pos(i, 0);
			OnChanged(pos, pos, 0, true);
		} else {
			u.Content.push_back(0);
		}

		const int diff = (int)line.Glyphs.size() - m;
		if (diff < 0) {
			if (State.CursorPosition.Line == i) {
				State.CursorPosition.Column += diff;
			}
			if (State.SelectionStart.Line == i) {
				State.SelectionStart.Column += diff;
			}
			if (State.SelectionEnd.Line == i) {
				State.SelectionEnd.Column += diff;
			}
		}
	}
	if (State.SelectionStart > State.SelectionEnd)
		std::swap(State.SelectionStart, State.SelectionEnd);

	Colorize(State.SelectionStart.Line, State.SelectionEnd.Line - State.SelectionStart.Line + 1);

	u.After = State;
	if (affectedLines > 0) {
		AddUndo(u);

		OnModified(false, true);
	}
}

void CodeEditor::MoveLineUp(void) {
	if (IsReadOnly())
		return;

	if (State.SelectionStart.Line == 0)
		return;

	UndoRecord u;
	u.Type = UndoType::MoveLineUp;
	u.Before = State;

	u.Start = State.SelectionStart;
	u.End = State.SelectionEnd;

	for (int i = u.Start.Line; i <= u.End.Line; ++i) {
		std::swap(CodeLines[i], CodeLines[i - 1]);
	}
	Coordinates pos(u.Start.Line - 1, 0);
	Coordinates pos_(u.End.Line, 0);
	OnChanged(pos, pos_, 0, true);

	u.Content.push_back(0);

	--State.SelectionStart.Line;
	--State.SelectionEnd.Line;
	--State.CursorPosition.Line;
	if (State.SelectionStart > State.SelectionEnd)
		std::swap(State.SelectionStart, State.SelectionEnd);

	Colorize(State.SelectionStart.Line - 1, State.SelectionEnd.Line - State.SelectionStart.Line + 2);

	u.After = State;
	AddUndo(u);

	OnModified(false, true);
}

void CodeEditor::MoveLineDown(void) {
	if (IsReadOnly())
		return;

	if (State.SelectionEnd.Line >= (int)CodeLines.size() - 1)
		return;

	UndoRecord u;
	u.Type = UndoType::MoveLineDown;
	u.Before = State;

	u.Start = State.SelectionStart;
	u.End = State.SelectionEnd;

	for (int i = u.End.Line; i >= u.Start.Line; --i) {
		std::swap(CodeLines[i], CodeLines[i + 1]);
	}
	Coordinates pos(u.Start.Line, 0);
	Coordinates pos_(u.End.Line + 1, 0);
	OnChanged(pos, pos_, 0, true);

	u.Content.push_back(0);

	++State.SelectionStart.Line;
	++State.SelectionEnd.Line;
	++State.CursorPosition.Line;
	if (State.SelectionStart > State.SelectionEnd)
		std::swap(State.SelectionStart, State.SelectionEnd);

	Colorize(State.SelectionStart.Line, State.SelectionEnd.Line - State.SelectionStart.Line + 2);

	u.After = State;
	AddUndo(u);

	OnModified(false, true);
}

void CodeEditor::ClearUndoRedoStack(void) {
	UndoBuf.clear();
	UndoIndex = 0;
	SavedIndex = 0;
}

bool CodeEditor::CanUndo(void) const {
	return UndoIndex > 0;
}

bool CodeEditor::CanRedo(void) const {
	return UndoIndex < (int)UndoBuf.size();
}

void CodeEditor::Undo(int aSteps) {
#if ICE_MERGE_UNDO_REDO
	if (aSteps == 1) {
		UndoRecord r;
		UndoRecord* p = nullptr;
		while (CanUndo() && (!p || UndoBuf[UndoIndex - 1].Similar(p))) {
			if (p == nullptr) {
				p = &r;
				*p = UndoBuf[UndoIndex - 1];
			}
			UndoBuf[--UndoIndex].Undo(this);
		}

		return;
	}
#endif /* ICE_MERGE_UNDO_REDO */

	while (CanUndo() && aSteps-- > 0) {
		UndoBuf[--UndoIndex].Undo(this);
	}
}

void CodeEditor::Redo(int aSteps) {
#if ICE_MERGE_UNDO_REDO
	if (aSteps == 1) {
		UndoRecord r;
		UndoRecord* p = nullptr;
		while (CanRedo() && (!p || (UndoIndex + 1 <= (int)UndoBuf.size() && UndoBuf[UndoIndex].Similar(p)))) {
			if (p == nullptr && UndoIndex + 1 < (int)UndoBuf.size()) {
				p = &r;
				*p = UndoBuf[UndoIndex + 1];
			}
			UndoBuf[UndoIndex++].Redo(this);
		}

		return;
	}
#endif /* ICE_MERGE_UNDO_REDO */

	while (CanRedo() && aSteps-- > 0) {
		UndoBuf[UndoIndex++].Redo(this);
	}
}

void CodeEditor::SyncTo(CodeEditor* aOther) {
	aOther->CodeLines = CodeLines;
	aOther->State = State;
	aOther->UndoBuf = UndoBuf;
	aOther->UndoIndex = UndoIndex;
	aOther->SavedIndex = SavedIndex;

	aOther->Brks = Brks;
	aOther->Errs = Errs;
	aOther->ProgramPointer = ProgramPointer;
}

const CodeEditor::Palette &CodeEditor::GetDarkPalette(void) {
	static const Palette plt = {
		0xffffffff, // None.
		0xffd69c56, // Keyword.
		0xffa8ceb5, // Number.
		0xff859dd6, // String.
		0xff70a0e0, // Char literal.
		0xffb4b4b4, // Punctuation.
		0xff409090, // Preprocessor.
		0xffb8d7a3, // Symbol.
		0xffdadada, // Identifier.
		0xffb0c94e, // Known identifier.
		0xffc040a0, // Preproc identifier.
		0xff4aa657, // Comment (single line).
		0xff4aa657, // Comment (multi-line).
		0x90909090, // Space.
		0xff2c2c2c, // Background.
		0xffe0e0e0, // Cursor.
		0x80a06020, // Selection.
		0x804d00ff, // Error marker.
		0x8005f0fa, // Warning marker.
		0xe00020f0, // Breakpoint.
		0xe000f0f0, // Program pointer.
		0xffaf912b, // Line number.
		0x40000000, // Current line fill.
		0x40808080, // Current line fill (inactive).
		0x40a0a0a0, // Current line edge.
		0xff84f2ef, // Line edited.
		0xff307457, // Line edited saved.
		0xfffa955f  // Line edited reverted.
	};

	return plt;
}

const CodeEditor::Palette &CodeEditor::GetLightPalette(void) {
	static const Palette plt = {
		0xff000000, // None.
		0xffff0c06, // Keyword.
		0xff008000, // Number.
		0xff2020a0, // String.
		0xff304070, // Char literal.
		0xff000000, // Punctuation.
		0xff409090, // Preprocessor.
		0xff5ac8c8, // Symbol.
		0xff404040, // Identifier.
		0xff606010, // Known identifier.
		0xffc040a0, // Preproc identifier.
		0xff205020, // Comment (single line).
		0xff405020, // Comment (multi-line).
		0xffaf912b, // Space.
		0xffffffff, // Background.
		0xff000000, // Cursor.
		0xffffd6ad, // Selection.
		0xa00010ff, // Error marker.
		0x8005f0fa, // Warning marker.
		0xe00020f0, // Breakpoint.
		0xe000f0f0, // Program pointer.
		0xffaf912b, // Line number.
		0x20000000, // Current line fill.
		0x20808080, // Current line fill (inactive).
		0x20000000, // Current line edge.
		0xff84f2ef, // Line edited.
		0xff307457, // Line edited saved.
		0xfffa955f  // Line edited reverted.
	};

	return plt;
}

const CodeEditor::Palette &CodeEditor::GetRetroBluePalette(void) {
	static const Palette plt = {
		0xff00ffff, // None.
		0xffffff00, // Keyword.
		0xff00ff00, // Number.
		0xff808000, // String.
		0xff808000, // Char literal.
		0xffffffff, // Punctuation.
		0xff008000, // Preprocessor.
		0xff5ac8c8, // Symbol.
		0xff00ffff, // Identifier.
		0xffffffff, // Known identifier.
		0xffff00ff, // Preproc identifier.
		0xffb0b0b0, // Comment (single line).
		0xffa0a0a0, // Comment (multi-line).
		0x90909090, // Space.
		0xff753929, // Background.
		0xff0080ff, // Cursor.
		0x80ffff00, // Selection.
		0xa00000ff, // Error marker.
		0x8005f0fa, // Warning marker.
		0xe00020f0, // Breakpoint.
		0xe000f0f0, // Program pointer.
		0xff808000, // Line number.
		0x40000000, // Current line fill.
		0x40808080, // Current line fill (inactive).
		0x40000000, // Current line edge.
		0xff84f2ef, // Line edited.
		0xff307457, // Line edited saved.
		0xfffa955f  // Line edited reverted.
	};

	return plt;
}

void CodeEditor::RenderText(int &aOffset, const ImVec2 &aPosition, ImU32 aPalette, ImU32 aColor, const char* aText, const std::list<Glyph> &aGlyphs, int aWidth) {
	ImDrawList* drawList = GetWindowDrawList();

	bool procSpaces =
		(aPalette == (ImU32)PaletteIndex::Default) ||
		(aPalette == (ImU32)PaletteIndex::String) ||
		(aPalette != (ImU32)PaletteIndex::MultiLineComment && (*aText == '\t' || *aText == ' '));
	if (procSpaces) {
		ImVec2 step = aPosition;
		std::list<Glyph>::const_iterator it = aGlyphs.begin();
		while (*aText) {
			const float size = GetFontSize();
			if (*aText == '\t') {
				int num = 0;
				if (aPalette == (ImU32)PaletteIndex::String)
					num = TabSize;
				else
					num = TabSize - aOffset % TabSize;
				const float x1 = step.x + 1;
				const float x2 = step.x + CharAdv.x * num - 2;
				const float y = step.y + size * 0.5f;
				const ImVec2 p1(x1, y);
				const ImVec2 p2(x2, y);
				const ImVec2 p3(x2 - size * 0.2f, y - size * 0.2f - 2);
				const ImVec2 p4(x2 - size * 0.2f, y + size * 0.2f);
				if (IsShowWhiteSpaces()) {
					drawList->AddLine(p1, p2, Plt[(int)PaletteIndex::Space]);
					drawList->AddLine(ImVec2(p2.x, p2.y - 1), p3, Plt[(int)PaletteIndex::Space]);
					drawList->AddLine(ImVec2(p2.x, p2.y - 1), p4, Plt[(int)PaletteIndex::Space]);
				}
				step.x += CharAdv.x * num;
				aOffset += num;

				++aText;
			} else if (*aText == ' ') {
				const float x = step.x + CharAdv.x * 0.5f;
				const float y = step.y + size * 0.5f;
				if (IsShowWhiteSpaces()) {
					drawList->AddCircleFilled(ImVec2(x, y), 1.5f, Plt[(int)PaletteIndex::Space], 4);
				}
				step.x += CharAdv.x;
				aOffset += 1;

				++aText;
			} else {
				const int n = ImTextExpectUtf8Char(aText);
				drawList->AddText(step, aColor, aText, aText + n);
				const int w = it->Width;
				step.x += CharAdv.x * w;
				aOffset += w;

				aText += n;
			}
			++it;
		}
	} else {
		drawList->AddText(aPosition, aColor, aText);
		aOffset += aWidth;
	}
}

void CodeEditor::Colorize(int aFromLine, int aLines) {
	int toLine = aLines == -1 ? (int)CodeLines.size() : std::min((int)CodeLines.size(), aFromLine + aLines);
	ColorRangeMin = std::min(ColorRangeMin, aFromLine);
	ColorRangeMax = std::max(ColorRangeMax, toLine);
	ColorRangeMin = std::max(0, ColorRangeMin);
	ColorRangeMax = std::max(ColorRangeMin, ColorRangeMax);

	if (!LangDef.CommentStart.empty() && !LangDef.CommentEnd.empty())
		CheckMultilineComments = GetFrameCount() + COLORIZE_DELAY_FRAME_COUNT;
}

void CodeEditor::ColorizeRange(int aFromLine, int aToLine) {
	if (CodeLines.empty() || aFromLine >= aToLine)
		return;

	auto colorize = [] (Line &line, size_t start, size_t end, PaletteIndex color) -> void {
		size_t k = 0;
		for (size_t j = 0; j < line.Glyphs.size(); ++j) {
			Glyph &g = line.Glyphs[j];
			if (k >= start)
				g.ColorIndex = (ImU32)color;
			k += ImTextCountUtf8Bytes(g.Character);
			if (k >= end)
				break;
		}
	};

	LastSymbol.clear();
	LastSymbolPalette = PaletteIndex::Default;

	std::string buffer;
	const int endLine = std::max(0, std::min((int)CodeLines.size(), aToLine));
	for (int i = aFromLine; i < endLine; ++i) {
		bool preproc = false;
		Line &line = CodeLines[i];
		buffer.clear();
		for (Glyph &g : CodeLines[i].Glyphs) {
			ImTextAppendUtf8ToStdStr(buffer, g.Character);
			g.ColorIndex = (ImU32)PaletteIndex::Default;
		}

		std::string::const_iterator first = buffer.cbegin();
		std::string::const_iterator last = buffer.cend();
		while (first != last) {
			if (LangDef.Tokenize != nullptr) {
				size_t offset = first - buffer.cbegin();
				const char* cursorBegin = buffer.c_str() + offset;
				const char* cursorEnd = buffer.c_str() + buffer.size();
				const char* tokenBegin = nullptr;
				const char* tokenEnd = nullptr;
				PaletteIndex color = PaletteIndex::Default;
				if (LangDef.Tokenize(cursorBegin, cursorEnd, tokenBegin, tokenEnd, color)) {
					const size_t tokenLen = tokenEnd - tokenBegin;
					first += tokenBegin - cursorBegin;
					offset = first - buffer.cbegin();
					colorize(line, offset, offset + tokenLen, color);
					first += tokenLen;

					LastSymbol.assign(tokenBegin, tokenLen);
					LastSymbolPalette = color;

					continue;
				}
			}

			for (RegexList::value_type &regex : Regexes) {
				std::match_results<std::string::const_iterator> results;
				const std::regex_constants::match_flag_type flag = std::regex_constants::match_continuous;
				bool matched = false;
				try {
					matched = std::regex_search<std::string::const_iterator>(first, last, results, regex.first, flag);
				} catch (const std::regex_error &err) {
					if (err.what())
						fprintf(stderr, "Regex match error: %s.\n", err.what());
					else
						fprintf(stderr, "Regex match error: %s.\n", "unknown");
				}
				if (matched) {
					auto v = *results.begin();
					auto start = v.first - buffer.begin();
					auto end = v.second - buffer.begin();
					PaletteIndex color = regex.second;
					LastSymbol = buffer.substr(start, end - start);
					LastSymbolPalette = color;
					if (color == PaletteIndex::Identifier) {
						if (!LangDef.CaseSensitive)
							std::transform(LastSymbol.begin(), LastSymbol.end(), LastSymbol.begin(), ICE_CASE_FUNC);

						if (!preproc) {
							if (LangDef.Keys.find(LastSymbol) != LangDef.Keys.end())
								color = PaletteIndex::Keyword;
							else if (LangDef.Symbols.find(LastSymbol) != LangDef.Symbols.end())
								color = PaletteIndex::Symbol;
							else if (LangDef.Ids.find(LastSymbol) != LangDef.Ids.end())
								color = PaletteIndex::KnownIdentifier;
							else if (LangDef.PreprocIds.find(LastSymbol) != LangDef.PreprocIds.end())
								color = PaletteIndex::PreprocIdentifier;
						} else {
							if (LangDef.PreprocIds.find(LastSymbol) != LangDef.PreprocIds.end())
								color = PaletteIndex::PreprocIdentifier;
							else
								color = PaletteIndex::Identifier;
						}
					} else if (color == PaletteIndex::Preprocessor) {
						preproc = true;
					}
					//for (int j = (int)start; j < (int)end; ++j)
					//	line[j].ColorIndex = color;
					colorize(line, start, end, color);
					first += end - start - 1;

					break;
				}
			}

			++first;
		}
	}
}

bool CodeEditor::ColorizeMultilineComments(void) {
	const std::string &startStr = LangDef.CommentStart;
	const std::string &endStr = LangDef.CommentEnd;
	if (startStr.empty() || endStr.empty())
		return false;

	Coordinates end((int)CodeLines.size(), 0);
	Coordinates commentStart = end;
	bool withinString = false;
	for (Coordinates i = Coordinates(0, 0); i < end; Advance(i)) {
		Line &line = CodeLines[i.Line];
		if (!line.Glyphs.empty()) {
			const Glyph &g = line.Glyphs[i.Column];
			Char c = g.Character;

			bool inComment = commentStart <= i;

			if (withinString) {
				line.Glyphs[i.Column].MultiLineComment = inComment;

				if (c == '\"') {
					if (i.Column + 1 < (int)line.Glyphs.size() && line.Glyphs[i.Column + 1].Character == '\"') {
						Advance(i);
						if (i.Column < (int)line.Glyphs.size())
							line.Glyphs[i.Column].MultiLineComment = inComment;
					} else {
						withinString = false;
					}
				} else if (c == '\\') {
					Advance(i);
					if (i.Column < (int)line.Glyphs.size())
						line.Glyphs[i.Column].MultiLineComment = inComment;
				}
			} else {
				if (c == '\"') {
					withinString = true;
					line.Glyphs[i.Column].MultiLineComment = inComment;
				} else {
					auto pred = [] (const char &a, const Glyph &b) {
						return a == (const char)b.Character;
					};
					bool except = false;
					auto from = line.Glyphs.begin() + i.Column;
					if (i.Column + startStr.size() <= line.Glyphs.size()) {
						if (LangDef.CommentException != '\0' && from != line.Glyphs.begin()) {
							auto prev = from - 1;
							if (prev->Character == LangDef.CommentException)
								except = true;
						}
						if (!except) {
							if (!startStr.empty() && std::equal(startStr.begin(), startStr.end(), from, from + startStr.size(), pred)) {
								commentStart = i;
							}
						}
					}

					inComment = commentStart <= i;

					line.Glyphs[i.Column].MultiLineComment = inComment;

					except = false;
					if (i.Column + 1 >= (int)endStr.size()) {
						auto till = from + 1 - endStr.size();
						if (LangDef.CommentException != '\0' && till != line.Glyphs.begin()) {
							auto prev = till - 1;
							if (prev->Character == LangDef.CommentException)
								except = true;
						}
						if (!except) {
							if (!endStr.empty() && std::equal(endStr.begin(), endStr.end(), till, from + 1, pred)) {
								commentStart = end;
							}
						}
					}
				}
			}
		}
	}

	return true;
}

void CodeEditor::ColorizeInternal(void) {
	if (CodeLines.empty())
		return;

	if (CheckMultilineComments && GetFrameCount() > CheckMultilineComments) {
		if (ColorizeMultilineComments())
			OnColorized(true);

		CheckMultilineComments = 0;

		return;
	}

	if (ColorRangeMin < ColorRangeMax) {
		int to = std::min(ColorRangeMin + 10, ColorRangeMax);
		ColorizeRange(ColorRangeMin, to);
		ColorRangeMin = to;

		if (ColorRangeMax == ColorRangeMin) {
			ColorRangeMin = std::numeric_limits<int>::max();
			ColorRangeMax = 0;
		}

		OnColorized(false);

		return;
	}
}

int CodeEditor::TextDistanceToLineStart(const Coordinates &aFrom) const {
	const Line &line = CodeLines[aFrom.Line];
	int len = 0;
	for (size_t it = 0; it < line.Glyphs.size() && it < (unsigned)aFrom.Column; ++it) {
		const Glyph &g = line.Glyphs[it];
		if (g.Character == '\t') {
			const bool literal = g.MultiLineComment ||
				g.ColorIndex == (ImU32)PaletteIndex::String ||
				g.ColorIndex == (ImU32)PaletteIndex::Comment ||
				g.ColorIndex == (ImU32)PaletteIndex::MultiLineComment;
			if (literal)
				len += TabSize;
			else
				len = (len / TabSize) * TabSize + TabSize;
		} else {
			if (g.Character <= 255) {
				++len;
			} else {
				const int w = GetCharacterWidth(g);
				len += w;
			}
		}
	}

	return len;
}

int CodeEditor::GetPageSize(void) const {
	float height = GetWindowHeight() - 20.0f;

	return (int)floor(height / CharAdv.y);
}

CodeEditor::Coordinates CodeEditor::GetActualCursorCoordinates(void) const {
	return SanitizeCoordinates(State.CursorPosition);
}

CodeEditor::Coordinates CodeEditor::SanitizeCoordinates(const Coordinates &aValue) const {
	int line = std::max(0, std::min((int)CodeLines.size() - 1, aValue.Line));
	int column = 0;
	if (!CodeLines.empty()) {
		if (line < aValue.Line) {
			column = (int)CodeLines[line].Glyphs.size();
		} else {
			column = std::min((int)CodeLines[line].Glyphs.size(), aValue.Column);
			column = std::max(column, 0);
		}
	}

	return Coordinates(line, column);
}

void CodeEditor::Advance(Coordinates &aCoordinates) const {
	if (aCoordinates.Line < (int)CodeLines.size()) {
		const Line &line = CodeLines[aCoordinates.Line];

		if (aCoordinates.Column + 1 < (int)line.Glyphs.size()) {
			++aCoordinates.Column;
		} else {
			++aCoordinates.Line;
			aCoordinates.Column = 0;
		}
	}
}

int CodeEditor::GetCharacterWidth(const Glyph &aGlyph) const {
	const ImGuiIO &io = GetIO();
	const ImFont* font = GetFont();
	if (!font)
		font = io.Fonts->Fonts[0];

	ImWchar cp = aGlyph.Codepoint;
	if (cp == 0) {
		const char* txt = (const char*)(&aGlyph.Character);
		const char* tend = txt + sizeof(Char);
		unsigned int codepoint = 0;
		ImTextCharFromUtf8(&codepoint, txt, tend);
		cp = (ImWchar)codepoint;
	}

	if ((int)cp < font->IndexAdvanceX.Size) {
		const float cadvx = font->IndexAdvanceX[cp];
		if (cadvx > CharAdv.x)
			return ICE_UTF_CHAR_WIDTH;
		else
			return 1;
	} else {
		return 1;
	}
}

CodeEditor::Coordinates CodeEditor::ScreenPosToCoordinates(const ImVec2 &aPosition) const {
	ImVec2 origin = CursorScreenPos;
	ImVec2 local(aPosition.x - origin.x, aPosition.y - origin.y);

	int lineNo = std::max(0, (int)floor(local.y / CharAdv.y));
	int columnCoord = std::max(0, (int)round(local.x / CharAdv.x) - TextStart);

	int column = 0;
	if (lineNo >= 0 && lineNo < (int)CodeLines.size()) {
		const Line &line = CodeLines[lineNo];
		int distance = 0;
		while (distance < columnCoord && column < (int)line.Glyphs.size()) {
			const Glyph &g = line.Glyphs[column];
			if (g.Character == '\t') {
				const bool literal = g.MultiLineComment ||
					g.ColorIndex == (ImU32)PaletteIndex::String ||
					g.ColorIndex == (ImU32)PaletteIndex::Comment ||
					g.ColorIndex == (ImU32)PaletteIndex::MultiLineComment;
				if (literal)
					distance += TabSize;
				else
					distance = (distance / TabSize) * TabSize + TabSize;
			} else {
				if (g.Character <= 255) {
					++distance;
				} else {
					const int w = GetCharacterWidth(g);
					distance += w;
				}
			}
			++column;
		}
	}

	return Coordinates(lineNo, column);
}

bool CodeEditor::IsOnWordBoundary(const Coordinates &aAt) const {
	if (aAt.Line >= (int)CodeLines.size() || aAt.Column == 0)
		return true;

	const Line &line = CodeLines[aAt.Line];
	if (aAt.Column >= (int)line.Glyphs.size())
		return true;

	return line.Glyphs[aAt.Column].ColorIndex != line.Glyphs[aAt.Column - 1].ColorIndex;
}

void CodeEditor::AddUndo(UndoRecord &aValue) {
	assert(!ReadOnly);

	UndoBuf.resize(UndoIndex + 1);
	UndoBuf.back() = aValue;
	++UndoIndex;
}

std::string CodeEditor::GetText(const Coordinates &aStart, const Coordinates &aEnd, const char* aNewline) const {
	std::string result;

	int prevLineNo = aStart.Line;
	for (Coordinates it = aStart; it <= aEnd; Advance(it)) {
		if (prevLineNo != it.Line && it.Line < (int)CodeLines.size()) {
			if (aNewline)
				result += aNewline;
			else
				result.push_back('\n');
		}

		if (it == aEnd)
			break;

		prevLineNo = it.Line;
		const Line &line = CodeLines[it.Line];
		if (!line.Glyphs.empty() && it.Column < (int)line.Glyphs.size()) {
			const Glyph &g = line.Glyphs[it.Column];
			ImTextAppendUtf8ToStdStr(result, g.Character);
		}
	}

	return result;
}

int CodeEditor::AppendBuffer(std::string &aBuffer, std::list<Glyph> &aGlyphs, Glyph &aGlyph, int aIndex, int &aWidth) {
	const Char chr = aGlyph.Character;

	int num = aGlyph.Width;
	if (num) {
		ImTextAppendUtf8ToStdStr(aBuffer, chr);

		aWidth += num;
		aGlyphs.push_back(aGlyph);

		return aIndex + num;
	}

	if (chr == '\t') {
		aBuffer.push_back('\t');

		const bool literal = aGlyph.MultiLineComment ||
			aGlyph.ColorIndex == (ImU32)PaletteIndex::String ||
			aGlyph.ColorIndex == (ImU32)PaletteIndex::Comment ||
			aGlyph.ColorIndex == (ImU32)PaletteIndex::MultiLineComment;
		if (literal)
			num = TabSize;
		else
			num = TabSize - aIndex % TabSize;

		aGlyph.Width = num;
		aWidth += num;
		aGlyphs.push_back(aGlyph);

		return aIndex + num;
	} else {
		const int n = ImTextAppendUtf8ToStdStr(aBuffer, chr);
		if (n <= 1)
			num = 1;
		else
			num = GetCharacterWidth(aGlyph);

		aGlyph.Width = num;
		aWidth += num;
		aGlyphs.push_back(aGlyph);

		return aIndex + num;
	}
}

int CodeEditor::InsertTextAt(Coordinates & /* inout */ aWhere, const char* aValue) {
	assert(!ReadOnly);

	int totalLines = 0;
	const char* str = aValue;
	while (*str != '\0') {
		if (CodeLines.empty())
			CodeLines.push_back(Line());

		int n = ImTextExpectUtf8Char(str);
		Char c = ImTextTakeUtf8Bytes(str, n);
		if (c == '\r') {
			// Do nothing.
		} else if (c == '\n') {
			if (aWhere.Column < (int)CodeLines[aWhere.Line].Glyphs.size()) {
				Line &newLine = InsertLine(aWhere.Line + 1);
				Line &line = CodeLines[aWhere.Line];
				newLine.Glyphs.insert(newLine.Glyphs.begin(), line.Glyphs.begin() + aWhere.Column, line.Glyphs.end());
				line.Glyphs.erase(line.Glyphs.begin() + aWhere.Column, line.Glyphs.end());
			} else {
				InsertLine(aWhere.Line + 1);
			}
			++aWhere.Line;
			aWhere.Column = 0;
			++totalLines;
		} else {
			Line &line = CodeLines[aWhere.Line];
			line.Glyphs.insert(line.Glyphs.begin() + aWhere.Column, Glyph(c, PaletteIndex::Default));
			++aWhere.Column;
		}
		str += n;
	}

	return totalLines;
}

void CodeEditor::DeleteRange(const Coordinates &aStart, const Coordinates &aEnd) {
	assert(aEnd >= aStart);
	assert(!ReadOnly);

	if (aEnd == aStart)
		return;

	if (aStart.Line == aEnd.Line) {
		Line &line = CodeLines[aStart.Line];
		if (aEnd.Column >= (int)line.Glyphs.size())
			line.Glyphs.erase(line.Glyphs.begin() + aStart.Column, line.Glyphs.end());
		else
			line.Glyphs.erase(line.Glyphs.begin() + aStart.Column, line.Glyphs.begin() + aEnd.Column);
	} else {
		Line &firstLine = CodeLines[aStart.Line];
		Line &lastLine = CodeLines[aEnd.Line];

		firstLine.Glyphs.erase(firstLine.Glyphs.begin() + aStart.Column, firstLine.Glyphs.end());
		lastLine.Glyphs.erase(lastLine.Glyphs.begin(), lastLine.Glyphs.begin() + aEnd.Column);

		if (aStart.Line < aEnd.Line)
			firstLine.Glyphs.insert(firstLine.Glyphs.end(), lastLine.Glyphs.begin(), lastLine.Glyphs.end());

		if (aStart.Line < aEnd.Line)
			RemoveLine(aStart.Line + 1, aEnd.Line + 1);
	}
}

void CodeEditor::DeleteSelection(void) {
	assert(State.SelectionEnd >= State.SelectionStart);

	if (State.SelectionEnd == State.SelectionStart)
		return;

	DeleteRange(State.SelectionStart, State.SelectionEnd);

	SetSelection(State.SelectionStart, State.SelectionStart);
	SetCursorPosition(State.SelectionStart);
	Colorize(State.SelectionStart.Line, 1);

	InteractiveStart = InteractiveEnd = State.CursorPosition;
}

CodeEditor::Line &CodeEditor::InsertLine(int aIndex) {
	assert(!ReadOnly);

	Line &result = *CodeLines.insert(CodeLines.begin() + aIndex, Line());

	ErrorMarkers etmp;
	for (ErrorMarkers::value_type &i : Errs)
		etmp.insert(ErrorMarkers::value_type(i.first >= aIndex ? i.first + 1 : i.first, i.second));
	Errs = std::move(etmp);

	Breakpoints btmp;
	for (auto brk : Brks)
		btmp.insert(std::make_pair(brk.first >= aIndex ? brk.first + 1 : brk.first, brk.second));
	Brks = std::move(btmp);

	return result;
}

void CodeEditor::RemoveLine(int aStart, int aEnd) {
	assert(!ReadOnly);

	ErrorMarkers etmp;
	for (ErrorMarkers::value_type &i : Errs) {
		ErrorMarkers::value_type e(i.first >= aStart ? i.first - 1 : i.first, i.second);
		if (e.first >= aStart && e.first <= aEnd)
			continue;

		etmp.insert(e);
	}
	Errs = std::move(etmp);

	Breakpoints btmp;
	for (auto brk : Brks) {
		if (brk.first >= aStart && brk.first <= aEnd)
			continue;

		btmp.insert(std::make_pair(brk.first >= aStart ? brk.first - (aEnd - aStart) : brk.first, brk.second));
	}
	Brks = std::move(btmp);

	CodeLines.erase(CodeLines.begin() + aStart, CodeLines.begin() + aEnd);
}

void CodeEditor::RemoveLine(int aIndex) {
	assert(!ReadOnly);

	ErrorMarkers etmp;
	for (ErrorMarkers::value_type &i : Errs) {
		ErrorMarkers::value_type e(i.first >= aIndex ? i.first - 1 : i.first, i.second);
		if (e.first == aIndex)
			continue;

		etmp.insert(e);
	}
	Errs = std::move(etmp);

	Breakpoints btmp;
	for (auto brk : Brks) {
		if (brk.first == aIndex)
			continue;

		btmp.insert(std::make_pair(brk.first >= aIndex ? brk.first - 1 : brk.first, brk.second));
	}
	Brks = std::move(btmp);

	CodeLines.erase(CodeLines.begin() + aIndex);
}

void CodeEditor::BackSpace(void) {
	assert(!ReadOnly);

	if (CodeLines.empty())
		return;

	UndoRecord u;
	u.Type = UndoType::Remove;
	u.Before = State;

	if (HasSelection()) {
		u.Content = GetSelectionText();
		u.Start = State.SelectionStart;
		u.End = State.SelectionEnd;
		DeleteSelection();

		OnChanged(State.SelectionStart, State.SelectionStart, 0, true);
	} else {
		Coordinates pos = GetActualCursorCoordinates();
		SetCursorPosition(pos);

		if (State.CursorPosition.Column == 0) {
			if (State.CursorPosition.Line == 0)
				return;

			const Line &line = CodeLines[State.CursorPosition.Line];
			Line &prevLine = CodeLines[State.CursorPosition.Line - 1];
			int prevSize = (int)prevLine.Glyphs.size();
			prevLine.Glyphs.insert(prevLine.Glyphs.end(), line.Glyphs.begin(), line.Glyphs.end());
			RemoveLine(State.CursorPosition.Line);
			--State.CursorPosition.Line;
			State.CursorPosition.Column = prevSize;

			u.Content = '\n';
			u.Start = GetActualCursorCoordinates();
			u.End = Coordinates(u.Start.Line + 1, 0);

			OnChanged(State.CursorPosition, State.CursorPosition, 0, true);
		} else {
			Line &line = CodeLines[State.CursorPosition.Line];

			u.Content.clear();
			ImTextAppendUtf8ToStdStr(u.Content, line.Glyphs[pos.Column - 1].Character);
			u.Start = u.End = GetActualCursorCoordinates();
			--u.Start.Column;

			--State.CursorPosition.Column;
			if (State.CursorPosition.Column < (int)line.Glyphs.size())
				line.Glyphs.erase(line.Glyphs.begin() + State.CursorPosition.Column);

			OnChanged(State.CursorPosition, State.CursorPosition, 0, true);
		}
		EnsureCursorVisible();
		Colorize(State.CursorPosition.Line, 1);
	}

	u.After = State;
	AddUndo(u);

	OnModified(false, true);
}

void CodeEditor::BackSpaceWordwise(void) {
	assert(!ReadOnly);

	if (CodeLines.empty())
		return;

	UndoRecord u;
	u.Type = UndoType::Remove;
	u.Before = State;

	if (HasSelection()) {
		u.Content = GetSelectionText();
		u.Start = State.SelectionStart;
		u.End = State.SelectionEnd;
		DeleteSelection();

		OnChanged(State.SelectionStart, State.SelectionStart, 0, true);
	} else {
		SelectWordUnderCursor();

		if (u.Start == u.End) {
			BackSpace();

			return;
		}

		u.Content = GetSelectionText();
		u.Start = State.SelectionStart;
		u.End = State.SelectionEnd;
		DeleteSelection();

		OnChanged(State.SelectionStart, State.SelectionStart, 0, true);
	}

	u.After = State;
	AddUndo(u);

	OnModified(false, true);
}

void CodeEditor::EnterCharacter(Char aChar) {
	assert(!ReadOnly);

	UndoRecord u;
	u.Type = UndoType::Add;
	u.Before = State;

	if (HasSelection()) {
		u.Overwritten = GetSelectionText();
		DeleteSelection();
	}

	Coordinates coord = GetActualCursorCoordinates();
	u.Start = coord;

	if (CodeLines.empty())
		CodeLines.push_back(Line());

	bool autoIndent = false;
	int moveCursor = 0;
	bool append = false;
	const bool isNewLine = aChar == '\n';
	const LanguageDefinition::RangedCharPairs::value_type* rangedPairStart = nullptr;
	const LanguageDefinition::RangedCharPairs::value_type* rangedPairEnd = nullptr;
	if (aChar != '\n') {
		rangedPairStart = FindRangedCharPairStart(aChar);
		rangedPairEnd = FindRangedCharPairEnd(aChar);
	}
	if (isNewLine) {
		int indent = 0;
		if (LastAutoIndent.hasValue) {
			if (LastAutoIndent.record.End == u.Start) {
				const Line &line = CodeLines[coord.Line];
				for (size_t i = 0; i < line.Glyphs.size(); ++i) {
					const Glyph &g = line.Glyphs[i];
					if (g.Character == ' ')
						++indent;
					else if (g.Character == '\t')
						indent += TabSize;
					else
						break;
				}

				const UndoRecord &v = UndoBuf.back();
				u.Before.SelectionStart = Coordinates(v.After.SelectionEnd.Line, 0);
				u.Before.SelectionEnd = v.After.SelectionEnd;

				const Coordinates start(LastAutoIndent.record.End.Line, 0);
				SetSelection(start, LastAutoIndent.record.End);
				if (HasSelection()) {
					u.Overwritten = GetSelectionText();
					DeleteSelection();
					coord.Column = 0;
					u.Start = coord;
				}
			} else {
				LastAutoIndent.clear();
			}
		}

		InsertLine(coord.Line + 1);
		Line &line = CodeLines[coord.Line];
		Line &newLine = CodeLines[coord.Line + 1];
		newLine.Glyphs.insert(newLine.Glyphs.begin(), line.Glyphs.begin() + coord.Column, line.Glyphs.end());
		line.Glyphs.erase(line.Glyphs.begin() + coord.Column, line.Glyphs.begin() + line.Glyphs.size());
		State.CursorPosition = Coordinates(coord.Line + 1, 0);

		ImTextAppendUtf8ToStdStr(u.Content, aChar);

		// Get indent spacec from the last line.
		if (indent == 0) {
			for (size_t i = 0; i < line.Glyphs.size(); ++i) {
				const Glyph &g = line.Glyphs[i];
				if (g.Character == ' ')
					++indent;
				else if (g.Character == '\t')
					indent += TabSize;
				else
					break;
			}
		}
		// Automatic indent for the new line.
		if (indent) {
			if (IndentWithTab) {
				const int spacec = indent % TabSize;
				const int tabs = indent / TabSize;
				for (int i = 0; i < spacec; ++i) {
					newLine.Glyphs.insert(newLine.Glyphs.begin(), Glyph(' ', PaletteIndex::Default));
					++State.CursorPosition.Column;
				}
				for (int i = 0; i < tabs; ++i) {
					newLine.Glyphs.insert(newLine.Glyphs.begin(), Glyph('\t', PaletteIndex::Default));
					++State.CursorPosition.Column;
				}
				for (int i = 0; i < tabs; ++i) {
					ImTextAppendUtf8ToStdStr(u.Content, '\t');
				}
				for (int i = 0; i < spacec; ++i) {
					ImTextAppendUtf8ToStdStr(u.Content, ' ');
				}
			} else {
				const int spacec = indent;
				for (int i = 0; i < spacec; ++i) {
					newLine.Glyphs.insert(newLine.Glyphs.begin(), Glyph(' ', PaletteIndex::Default));
					++State.CursorPosition.Column;
				}
				for (int i = 0; i < spacec; ++i) {
					ImTextAppendUtf8ToStdStr(u.Content, ' ');
				}
			}

			autoIndent = true;
		}

		SetSelection(State.CursorPosition, State.CursorPosition);

		OnChanged(coord, Coordinates(coord.Line + 1, 0), 0, true);
	} else if (rangedPairEnd && InputtedRangedPair) {
		InputtedRangedPair = false;
		Coordinates c = GetCursorPosition();
		++c.Column;
		const Char ch = GetCharAt(c);
		if ((Char)rangedPairEnd->second == ch) {
			++State.CursorPosition.Column;

			return;
		}
		append = true;
	} else if (rangedPairStart) {
		ImTextAppendUtf8ToStdStr(u.Content, rangedPairStart->first);
		if (!u.Overwritten.empty())
			u.Content += u.Overwritten;
		ImTextAppendUtf8ToStdStr(u.Content, rangedPairStart->second);
		u.Start = GetActualCursorCoordinates();

		InsertText(u.Content.c_str());

		u.End = GetActualCursorCoordinates();
		u.After = State;

		OnChanged(u.Start, u.End, 0, false);

		if (u.Overwritten.empty())
			moveCursor = -1;

		InputtedRangedPair = true;
	} else {
		append = true;
	}
	if (append) {
		Line &line = CodeLines[coord.Line];
		if (Overwrite && (int)line.Glyphs.size() > coord.Column)
			line.Glyphs[coord.Column] = Glyph(aChar, PaletteIndex::Default);
		else
			line.Glyphs.insert(line.Glyphs.begin() + coord.Column, Glyph(aChar, PaletteIndex::Default));
		State.CursorPosition = coord;
		++State.CursorPosition.Column;

		ImTextAppendUtf8ToStdStr(u.Content, aChar);

		OnChanged(coord, coord, 0, false);
	}

	InteractiveStart = InteractiveEnd = State.CursorPosition;

	u.End = GetActualCursorCoordinates();
	u.After = State;

	AddUndo(u);

	if (autoIndent)
		LastAutoIndent = AutoIndentRecord(u);
	else
		LastAutoIndent.clear();

	Colorize(coord.Line - 1, 3);
	EnsureCursorVisible();

	OnModified(isNewLine, false);

	if (moveCursor != 0)
		State.CursorPosition.Column += moveCursor;
}

CodeEditor::Coordinates CodeEditor::FindWordStart(const Coordinates &aFrom) const {
	Coordinates at = aFrom;
	if (at.Line >= (int)CodeLines.size())
		return at;

	const Line &line = CodeLines[at.Line];

	if (at.Column >= (int)line.Glyphs.size())
		return at;

	PaletteIndex cstart = (PaletteIndex)line.Glyphs[at.Column].ColorIndex;
	if (at.Column > 0) {
		while (at.Column > 0) {
			const Glyph &g = line.Glyphs[at.Column - 1];
			if ((cstart != PaletteIndex::Default && cstart != PaletteIndex::String) && (g.Character == ' ' || g.Character == '\t'))
				break;
			if (cstart != (PaletteIndex)g.ColorIndex)
				break;

			--at.Column;
		}
		if (cstart == PaletteIndex::String)
			++at.Column;
	}

	return at;
}

CodeEditor::Coordinates CodeEditor::FindWordEnd(const Coordinates &aFrom) const {
	Coordinates at = aFrom;
	if (at.Line >= (int)CodeLines.size())
		return at;

	const Line &line = CodeLines[at.Line];

	if (at.Column >= (int)line.Glyphs.size())
		return at;

	PaletteIndex cstart = (PaletteIndex)line.Glyphs[at.Column].ColorIndex;
	if (at.Column < (int)line.Glyphs.size()) {
		while (at.Column < (int)line.Glyphs.size()) {
			const Glyph &g = line.Glyphs[at.Column];
			if ((cstart != PaletteIndex::Default && cstart != PaletteIndex::String) && (g.Character == ' ' || g.Character == '\t'))
				break;
			if (cstart != (PaletteIndex)g.ColorIndex)
				break;

			++at.Column;
		}
		if (cstart == PaletteIndex::String)
			--at.Column;
	}

	return at;
}

std::string CodeEditor::GetWordAt(const Coordinates &aCoords, Coordinates* aStart, Coordinates* aEnd) const {
	Coordinates start = FindWordStart(aCoords);
	Coordinates end = FindWordEnd(aCoords);

	if (aStart)
		*aStart = start;
	if (aEnd)
		*aEnd = end;

	std::string r;

	for (Coordinates it = start; it < end; Advance(it)) {
		const Glyph &g = CodeLines[it.Line].Glyphs[it.Column];
		ImTextAppendUtf8ToStdStr(r, g.Character);
	}

	return r;
}

CodeEditor::Char CodeEditor::GetCharAt(const Coordinates &aCoords) const {
	Coordinates c = aCoords;
	if (--c.Column < 0)
		return '\0';

	if (c.Line < 0 || c.Line >= (int)CodeLines.size())
		return '\0';
	if (c.Column < 0 || c.Column >= (int)CodeLines[c.Line].Glyphs.size())
		return '\0';

	const Glyph &g = CodeLines[c.Line].Glyphs[c.Column];

	return g.Character;
}

CodeEditor::Char CodeEditor::GetCharUnderCursor(void) const {
	Coordinates c = GetCursorPosition();

	return GetCharAt(c);
}

const CodeEditor::LanguageDefinition::RangedCharPairs::value_type* CodeEditor::FindRangedCharPairStart(Char aChar) const {
	CodeEditor::LanguageDefinition::RangedCharPairs::const_iterator it = std::find_if(
		LangDef.RangedCharPatterns.begin(), LangDef.RangedCharPatterns.end(),
		[aChar] (const CodeEditor::LanguageDefinition::RangedCharPairs::value_type &pair) -> bool {
			return pair.first == aChar;
		}
	);
	if (it == LangDef.RangedCharPatterns.end())
		return nullptr;

	return &(*it);
}

const CodeEditor::LanguageDefinition::RangedCharPairs::value_type* CodeEditor::FindRangedCharPairEnd(Char aChar) const {
	CodeEditor::LanguageDefinition::RangedCharPairs::const_iterator it = std::find_if(
		LangDef.RangedCharPatterns.begin(), LangDef.RangedCharPatterns.end(),
		[aChar] (const CodeEditor::LanguageDefinition::RangedCharPairs::value_type &pair) -> bool {
			return pair.second == aChar;
		}
	);
	if (it == LangDef.RangedCharPatterns.end())
		return nullptr;

	return &(*it);
}

void CodeEditor::OnChanged(const Coordinates &aStart, const Coordinates &aEnd, int aOffset, bool aClearInputtedRangedPair) {
	Coordinates s, e;
	if (aStart < aEnd) {
		s = aStart;
		e = aEnd;
	} else {
		s = aEnd;
		e = aStart;
	}
	for (int ln = s.Line; ln <= e.Line && ln < (int)CodeLines.size(); ++ln) {
		if (ln < 0)
			continue;

		Line &line = CodeLines[ln];
		if (aOffset && SavedIndex == UndoIndex) {
			line.Revert();
		} else {
			line.Change();
		}
	}

	if (aClearInputtedRangedPair)
		InputtedRangedPair = false;
}

bool CodeEditor::OnKeyPressed(ImGuiKey aKey) {
	if (KeyPressedHandler == nullptr)
		return false;

	return KeyPressedHandler(aKey);
}

void CodeEditor::OnColorized(bool aMultilineComment) const {
	if (ColorizedHandler == nullptr)
		return;

	ColorizedHandler(aMultilineComment);
}

void CodeEditor::OnModified(bool aNewLine,bool aClearAutoIndent) const {
	if (aClearAutoIndent)
		LastAutoIndent.clear();

	if (ModifiedHandler == nullptr)
		return;

	ModifiedHandler(aNewLine);
}

void CodeEditor::OnHeadClicked(int aLine, bool aDoubleClicked) const {
	if (HeadClickedHandler == nullptr)
		return;

	HeadClickedHandler(aLine, aDoubleClicked);
}

void CodeEditor::OnLineClicked(int aLine, bool aDoubleClicked) const {
	if (LineClickedHandler == nullptr)
		return;

	LineClickedHandler(aLine, aDoubleClicked);
}

}
