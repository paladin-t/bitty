#define NOMINMAX
#include "imgui_code_editor.h"
#include "../imgui/imgui_internal.h"
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

	static const unsigned char range[] = {
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

	type = range[(unsigned char)c];
	codepoint = (0xff >> type) & (unsigned char)c;

	switch (type) {
	case 2: _TAIL(ch, c, result, codepoint, range); return result;
	case 3: _TAIL(ch, c, result, codepoint, range); _TAIL(ch, c, result, codepoint, range); return result;
	case 4: _COPY(ch, c, result, codepoint); _TRANS(0x50, codepoint, range); _TAIL(ch, c, result, codepoint, range); return result;
	case 5: _COPY(ch, c, result, codepoint); _TRANS(0x10, codepoint, range); _TAIL(ch, c, result, codepoint, range); _TAIL(ch, c, result, codepoint, range); return result;
	case 6: _TAIL(ch, c, result, codepoint, range); _TAIL(ch, c, result, codepoint, range); _TAIL(ch, c, result, codepoint, range); return result;
	case 10: _COPY(ch, c, result, codepoint); _TRANS(0x20, codepoint, range); _TAIL(ch, c, result, codepoint, range); return result;
	case 11: _COPY(ch, c, result, codepoint); _TRANS(0x60, codepoint, range); _TAIL(ch, c, result, codepoint, range); _TAIL(ch, c, result, codepoint, range); return result;
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
			"self"
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
		return ch >= ' ' && ch <= '\t';
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
		case UndoType::Add: {
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
				aEditor->OnChanged(Start, end, -1);
			}

			break;
		case UndoType::Remove: {
				Coordinates start = Start;
				aEditor->InsertTextAt(start, Content.c_str());
				aEditor->Colorize(Start.Line - 1, End.Line - Start.Line + 2);

				aEditor->OnChanged(Start, End, -1);
			}

			break;
		case UndoType::Indent: {
				assert(End.Line - Start.Line + 1 == (int)Content.length());

				for (int i = Start.Line; i <= End.Line; ++i) {
					Line &line = aEditor->CodeLines[i];
					const char op = Content[i - Start.Line];
					if (op == 0) {
						// Do nothing.
					} else if (op == std::numeric_limits<char>::max()) {
						const Glyph &g = *line.Glyphs.begin();
						if (g.Character == '\t') {
							line.Glyphs.erase(line.Glyphs.begin());
						} else {
							assert(false);
						}

						Coordinates pos(i, 0);
						aEditor->OnChanged(pos, pos, -1);
					} else {
						assert(false);
					}
				}
			}

			break;
		case UndoType::Unindent: {
				assert(End.Line - Start.Line + 1 == (int)Content.length());

				for (int i = Start.Line; i <= End.Line; ++i) {
					Line &line = aEditor->CodeLines[i];
					char op = Content[i - Start.Line];
					if (op == 0) {
						// Do nothing.
					} else if (op == std::numeric_limits<char>::max()) {
						line.Glyphs.insert(line.Glyphs.begin(), Glyph('\t', PaletteIndex::Default));

						Coordinates pos(i, 0);
						aEditor->OnChanged(pos, pos, -1);
					} else if (op > 0) {
						while (op--) {
							line.Glyphs.insert(line.Glyphs.begin(), Glyph(' ', PaletteIndex::Default));
						}

						Coordinates pos(i, 0);
						aEditor->OnChanged(pos, pos, -1);
					} else {
						assert(false);
					}
				}
			}

			break;
		}
	}

	aEditor->State = Before;
	aEditor->EnsureCursorVisible();

	aEditor->OnModified();
}

void CodeEditor::UndoRecord::Redo(CodeEditor* aEditor) {
	if (!Content.empty()) {
		switch (Type) {
		case UndoType::Add: {
				aEditor->State = Before;

				aEditor->DeleteSelection();

				Coordinates start = Start;
				aEditor->InsertTextAt(start, Content.c_str());
				aEditor->Colorize(Start.Line - 1, End.Line - Start.Line + 1 + 1);

				aEditor->OnChanged(Start, End, 1);
			}

			break;
		case UndoType::Remove: {
				aEditor->DeleteRange(Start, End);
				aEditor->Colorize(Start.Line - 1, End.Line - Start.Line + 1);

				aEditor->OnChanged(Start, Start, 1);
			}

			break;
		case UndoType::Indent: {
				assert(End.Line - Start.Line + 1 == (int)Content.length());

				for (int i = Start.Line; i <= End.Line; ++i) {
					Line &line = aEditor->CodeLines[i];
					const char op = Content[i - Start.Line];
					if (op == 0) {
						// Do nothing.
					} else if (op == std::numeric_limits<char>::max()) {
						line.Glyphs.insert(line.Glyphs.begin(), Glyph('\t', PaletteIndex::Default));

						Coordinates pos(i, 0);
						aEditor->OnChanged(pos, pos, 1);
					} else {
						assert(false);
					}
				}
			}

			break;
		case UndoType::Unindent: {
				assert(End.Line - Start.Line + 1 == (int)Content.length());

				for (int i = Start.Line; i <= End.Line; ++i) {
					Line &line = aEditor->CodeLines[i];
					char op = Content[i - Start.Line];
					if (op == 0) {
						// Do nothing.
					} else if (op == std::numeric_limits<char>::max()) {
						const Glyph &g = *line.Glyphs.begin();
						if (g.Character == '\t') {
							line.Glyphs.erase(line.Glyphs.begin());
						} else {
							assert(false);
						}

						Coordinates pos(i, 0);
						aEditor->OnChanged(pos, pos, 1);
					} else if (op > 0) {
						while (op--) {
							const Glyph &g = *line.Glyphs.begin();
							if (g.Character == ' ') {
								line.Glyphs.erase(line.Glyphs.begin());
							} else {
								assert(false);
							}
						}

						Coordinates pos(i, 0);
						aEditor->OnChanged(pos, pos, 1);
					} else {
						assert(false);
					}
				}
			}

			break;
		}
	}

	aEditor->State = After;
	aEditor->EnsureCursorVisible();

	aEditor->OnModified();
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
	TabSize(4),
	TextStart(7),
	Overwrite(false),
	ReadOnly(false),
	ShowLineNumbers(true),
	HeadClickEnabled(false),
	ShortcutsEnabled(ShortcutType::All),
	WithinRender(false),
	ScrollToCursor(0),
	WordSelectionMode(false),
	ColorRangeMin(0),
	ColorRangeMax(0),
	CheckMultilineComments(0),
	TooltipEnabled(true),
	ShowWhiteSpaces(true),
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

	const bool shift = io.KeyShift;
	const bool ctrl = io.KeyCtrl;
	const bool alt = io.KeyAlt;

	CursorScreenPos = GetCursorScreenPos();
	EditorFocused = IsWindowFocused();

	if (IsEditorFocused()) {
		const ImVec2 start(GetWindowPos());
		const ImVec2 end(start.x + GetWindowWidth(), start.y + GetWindowHeight());
		if (IsMouseHoveringRect(start, end))
			SetMouseCursor(ImGuiMouseCursor_TextInput);
		CaptureKeyboardFromApp(true);

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
			else if (!IsReadOnly() && !ctrl && !shift && !alt && IsKeyPressed(GetKeyIndex(ImGuiKey_Delete)))
				Delete();
		}

		if (!ctrl && !alt && IsKeyPressed(GetKeyIndex(ImGuiKey_UpArrow)))
			MoveUp(1, shift);
		else if (!ctrl && !alt && IsKeyPressed(GetKeyIndex(ImGuiKey_DownArrow)))
			MoveDown(1, shift);
		else if (!alt && IsKeyPressed(GetKeyIndex(ImGuiKey_LeftArrow)))
			MoveLeft(1, shift, ctrl);
		else if (!alt && IsKeyPressed(GetKeyIndex(ImGuiKey_RightArrow)))
			MoveRight(1, shift, ctrl);
		else if (!alt && IsKeyPressed(GetKeyIndex(ImGuiKey_PageUp)))
			MoveUp(GetPageSize() - 4, shift);
		else if (!alt && IsKeyPressed(GetKeyIndex(ImGuiKey_PageDown)))
			MoveDown(GetPageSize() - 4, shift);
		else if (!alt && ctrl && IsKeyPressed(GetKeyIndex(ImGuiKey_Home)))
			MoveTop(shift);
		else if (ctrl && !alt && IsKeyPressed(GetKeyIndex(ImGuiKey_End)))
			MoveBottom(shift);
		else if (!ctrl && !alt && IsKeyPressed(GetKeyIndex(ImGuiKey_Home)))
			MoveHome(shift);
		else if (!ctrl && !alt && IsKeyPressed(GetKeyIndex(ImGuiKey_End)))
			MoveEnd(shift);
		else if (!IsReadOnly() && !ctrl && !shift && !alt && IsKeyPressed(GetKeyIndex(ImGuiKey_Backspace)))
			BackSpace();

		if (!IsReadOnly()) {
			if (IsKeyPressed(GetKeyIndex(ImGuiKey_Enter)) || OnKeyPressed(ImGuiKey_Enter)) {
				unsigned int c = '\n'; // Insert new line.
				io.AddInputCharacter((ImWchar)c);
			} else if (IsKeyPressed(GetKeyIndex(ImGuiKey_Tab))) {
				if (HasSelection() && GetSelectionLines() > 1) {
					if (IsShortcutsEnabled(ShortcutType::IndentUnindent)) {
						if (!ctrl && !alt && !shift) // Indent multi-lines.
							Indent();
						else if (!ctrl && !alt && shift) // Unindent multi-lines.
							Unindent();
						else if (ctrl && !alt && shift) // Unindent multi-lines.
							Unindent();
					}
				} else {
					if (!ctrl && !alt && !shift) {
						unsigned int c = '\t'; // Insert tab.
						io.AddInputCharacter((ImWchar)c);
					} else if (!ctrl && !alt && shift) {
						Char cc = GetCharUnderCursor();
						if (cc == '\t' || cc == ' ')
							BackSpace(); // Unindent single line.
					} else if (ctrl && !alt && shift) {
						Char cc = GetCharUnderCursor();
						if (cc == '\t' || cc == ' ')
							BackSpace(); // Unindent single line.
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
					EnterCharacter(c);
				}
				str += n;
			}
		}
	}

	if (IsWindowHovered()) {
		if (!shift && !alt) {
			if (IsMouseClicked(0)) {
				State.CursorPosition = InteractiveStart = InteractiveEnd = SanitizeCoordinates(ScreenPosToCoordinates(GetMousePos()));
				if (ctrl)
					WordSelectionMode = true;
				SetSelection(InteractiveStart, InteractiveEnd, WordSelectionMode);
			}
			if (IsMouseDoubleClicked(0) && !ctrl) {
				State.CursorPosition = InteractiveStart = InteractiveEnd = SanitizeCoordinates(ScreenPosToCoordinates(GetMousePos()));
				WordSelectionMode = true;
				SetSelection(InteractiveStart, InteractiveEnd, WordSelectionMode);
				State.CursorPosition = State.SelectionEnd;
			} else if (IsMouseDragging(0) && IsMouseDown(0)) {
				io.WantCaptureMouse = true;
				State.CursorPosition = InteractiveEnd = SanitizeCoordinates(ScreenPosToCoordinates(GetMousePos()));
				SetSelection(InteractiveStart, InteractiveEnd, WordSelectionMode);
			}
		} else if (shift) {
			if (IsMouseClicked(0)) {
				io.WantCaptureMouse = true;
				State.CursorPosition = InteractiveEnd = SanitizeCoordinates(ScreenPosToCoordinates(GetMousePos()));
				SetSelection(InteractiveStart, InteractiveEnd, WordSelectionMode);
			}
		}

		if (!IsMouseDown(0)) {
			WordSelectionMode = false;
		}
	}

	ColorizeInternal();

	static std::string buffer; // Shared.
	ImVec2 contentSize = GetWindowContentRegionMax();
	ImDrawList* drawList = GetWindowDrawList();
	int appendIndex = 0;
	int longest = TextStart;

	SetHeadSize(CharAdv.x * TextStart);
	ImVec2 cursorScreenPos = GetCursorScreenPos();
	const float scrollX = GetScrollX();
	const float scrollY = GetScrollY();

	int lineNo = (int)floor(scrollY / CharAdv.y);
	const int lineMax = std::max(0, std::min((int)CodeLines.size() - 1, lineNo + (int)ceil((scrollY + contentSize.y) / CharAdv.y)));
	if (!CodeLines.empty()) {
		while (lineNo <= lineMax) {
			ImVec2 lineStartScreenPos = ImVec2(cursorScreenPos.x, cursorScreenPos.y + lineNo * CharAdv.y);
			ImVec2 textScreenPos = ImVec2(lineStartScreenPos.x + CharAdv.x * TextStart, lineStartScreenPos.y);

			const Line &line = CodeLines[lineNo];
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

			if (sstart != -1 && ssend != -1 && sstart < ssend) {
				const ImVec2 vstart(lineStartScreenPos.x + (CharAdv.x) * (sstart + TextStart), lineStartScreenPos.y);
				const ImVec2 vend(lineStartScreenPos.x + (CharAdv.x) * (ssend + TextStart), lineStartScreenPos.y + CharAdv.y);
				drawList->AddRectFilled(vstart, vend, Plt[(int)PaletteIndex::Selection]);
			}

			const ImVec2 start(lineStartScreenPos.x + scrollX, lineStartScreenPos.y);

			if (IsHeadClickEnabled() && IsEditorFocused()) {
				const ImVec2 end(lineStartScreenPos.x + CharAdv.x * std::min((TextStart - 1), 3), lineStartScreenPos.y + CharAdv.y);
				if (IsMouseHoveringRect(start, end)) {
					SetMouseCursor(ImGuiMouseCursor_Hand);
					if (IsMouseClicked(0))
						OnHeadClicked(lineNo);
				}
			}

			auto brk = Brks.find(lineNo);
			if (brk != Brks.end()) {
				//ImVec2 end(lineStartScreenPos.x + contentSize.x + 2.0f * scrollX, lineStartScreenPos.y + CharAdv.y);
				//drawList->AddRectFilled(start, end, Plt[(int)PaletteIndex::Breakpoint]);

				const float offsetX = 2;
				const ImVec2 end(lineStartScreenPos.x + CharAdv.x * (TextStart - 1), lineStartScreenPos.y + CharAdv.y);
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
			if (GetProgramPointer() >= 0 && GetProgramPointer() == lineNo) {
				const float margin = brk != Brks.end() ? 1.0f : 0.0f;
				const float offsetX = 2;
				const ImVec2 end(lineStartScreenPos.x + CharAdv.x * (TextStart - 1), lineStartScreenPos.y + CharAdv.y);
				const ImVec2 points[5] = {
					ImVec2(offsetX + start.x + margin, start.y + margin),
					ImVec2(end.x - CharAdv.y * 0.5f - margin, start.y + margin),
					ImVec2(end.x - margin, start.y + CharAdv.y * 0.5f - 1),
					ImVec2(end.x - CharAdv.y * 0.5f - margin, end.y - margin),
					ImVec2(offsetX + start.x + margin, end.y - margin)
				};
				drawList->AddConvexPolyFilled(points, countof(points), Plt[(int)PaletteIndex::ProgramPointer]);
			}

			ErrorMarkers::iterator errorIt = Errs.find(lineNo);
			if (errorIt != Errs.end()) {
				const ImVec2 end(lineStartScreenPos.x + contentSize.x + 2.0f * scrollX, lineStartScreenPos.y + CharAdv.y);
				drawList->AddRectFilled(start, end, Plt[(int)PaletteIndex::ErrorMarker]);

				if (IsTooltipEnabled()) {
					if (IsMouseHoveringRect(lineStartScreenPos, end)) {
						BeginTooltip();
						PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.2f, 0.2f, 1.0f));
						Text("Error at line %d:", errorIt->first);
						PopStyleColor();
						Separator();
						PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.2f, 1.0f));
						Text("%s", errorIt->second.c_str());
						PopStyleColor();
						EndTooltip();
					}
				}
			}

			if (IsShowLineNumbers()) {
				static char buf[16]; // Shared.
				switch (TextStart - 1) {
				case 5: snprintf(buf, countof(buf), "%4d", lineNo + 1); break;
				case 6: snprintf(buf, countof(buf), "%5d", lineNo + 1); break;
				default: snprintf(buf, countof(buf), "%6d", lineNo + 1); break;
				}
				drawList->AddText(ImVec2(lineStartScreenPos.x /*+ CharAdv.x * 1*/, lineStartScreenPos.y), Plt[(int)PaletteIndex::LineNumber], buf);
			}
			switch (line.Changed) {
			case LineState::None: // Do nothing.
				break;
			case LineState::Edited:
				drawList->AddRectFilled(
					ImVec2(lineStartScreenPos.x + CharAdv.x * (TextStart - 1), lineStartScreenPos.y),
					ImVec2(lineStartScreenPos.x + CharAdv.x * (TextStart - 1) + CharAdv.x * 0.5f, lineStartScreenPos.y + CharAdv.y),
					Plt[(int)PaletteIndex::LineEdited]
				);

				break;
			case LineState::EditedSaved:
				drawList->AddRectFilled(
					ImVec2(lineStartScreenPos.x + CharAdv.x * (TextStart - 1), lineStartScreenPos.y),
					ImVec2(lineStartScreenPos.x + CharAdv.x * (TextStart - 1) + CharAdv.x * 0.5f, lineStartScreenPos.y + CharAdv.y),
					Plt[(int)PaletteIndex::LineEditedSaved]
				);

				break;
			case LineState::EditedReverted:
				drawList->AddRectFilled(
					ImVec2(lineStartScreenPos.x + CharAdv.x * (TextStart - 1), lineStartScreenPos.y),
					ImVec2(lineStartScreenPos.x + CharAdv.x * (TextStart - 1) + CharAdv.x * 0.5f, lineStartScreenPos.y + CharAdv.y),
					Plt[(int)PaletteIndex::LineEditedReverted]
				);

				break;
			}

			if (State.CursorPosition.Line == lineNo) {
				bool focused = IsEditorFocused();

				if (!IsReadOnly() && !HasSelection()) {
					const ImVec2 end(start.x + contentSize.x + scrollX, start.y + CharAdv.y);
					drawList->AddRectFilled(start, end, Plt[(int)(focused ? PaletteIndex::CurrentLineFill : PaletteIndex::CurrentLineFillInactive)]);
					drawList->AddRect(start, end, Plt[(int)PaletteIndex::CurrentLineEdge], 1.0f);
				}

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

			appendIndex = 0;
			unsigned prevColor = line.Glyphs.empty() ? (ImU32)PaletteIndex::Default : (line.Glyphs[0].MultiLineComment ? (ImU32)PaletteIndex::MultiLineComment : line.Glyphs[0].ColorIndex);
			const Glyph* prevGlyph = nullptr;

			int width = 0;
			int offset = 0;
			for (const Glyph &glyph : line.Glyphs) {
				unsigned color = glyph.MultiLineComment ? (ImU32)PaletteIndex::MultiLineComment : glyph.ColorIndex;

				const bool sameColor = color == prevColor ||
					(glyph.Codepoint > 255 &&
						color == (unsigned)PaletteIndex::Default && prevColor == (unsigned)PaletteIndex::Identifier) ||
					((prevGlyph && prevGlyph->Codepoint > 255) &&
						color == (unsigned)PaletteIndex::Identifier && prevColor == (unsigned)PaletteIndex::Default);
				if (!sameColor && !buffer.empty()) {
					const ImU32 targetColor = prevColor >= (ImU32)PaletteIndex::Max ? prevColor : Plt[(int)prevColor];
					RenderText(offset, textScreenPos, prevColor, targetColor, buffer.c_str(), width);
					textScreenPos.x += CharAdv.x * width;
					buffer.clear();
					prevColor = color;
					width = 0;
				}
				appendIndex = AppendBuffer(buffer, glyph, appendIndex, width);
				++columnNo;
				prevGlyph = &glyph;
			}

			if (!buffer.empty()) {
				const ImU32 targetColor = prevColor >= (ImU32)PaletteIndex::Max ? prevColor : Plt[(int)prevColor];
				RenderText(offset, textScreenPos, prevColor, targetColor, buffer.c_str(), width);
				buffer.clear();
			}
			appendIndex = 0;
			lineStartScreenPos.y += CharAdv.y;
			textScreenPos.x = lineStartScreenPos.x + CharAdv.x * TextStart;
			textScreenPos.y = lineStartScreenPos.y;
			++lineNo;
		}

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

void CodeEditor::SetCursorPosition(const Coordinates &aPosition) {
	if (State.CursorPosition != aPosition) {
		State.CursorPosition = aPosition;
		EnsureCursorVisible();
	}
}

CodeEditor::Coordinates CodeEditor::GetCursorPosition(void) const {
	return GetActualCursorCoordinates();
}

void CodeEditor::EnsureCursorVisible(bool aForceAbove) {
	if (!WithinRender) {
		ScrollToCursor = aForceAbove ? -1 : 1;

		return;
	}

	float scrollX = GetScrollX();
	float scrollY = GetScrollY();

	float height = GetWindowHeight();
	float width = GetWindowWidth();

	int top = 1 + (int)ceil(scrollY / CharAdv.y);
	int bottom = (int)ceil((scrollY + height) / CharAdv.y);

	int left = (int)ceil(scrollX / CharAdv.x);
	int right = (int)ceil((scrollX + width) / CharAdv.x);

	Coordinates pos = GetActualCursorCoordinates();
	int len = TextDistanceToLineStart(pos);

	if (pos.Line < top || aForceAbove)
		SetScrollY(std::max(0.0f, (pos.Line - 1) * CharAdv.y));
	else if (pos.Line > bottom - 4)
		SetScrollY(std::max(0.0f, (pos.Line + 4) * CharAdv.y - height));
	if (len + TextStart < left + 4)
		SetScrollX(std::max(0.0f, (len + TextStart - 4) * CharAdv.x));
	else if (len + TextStart > right - 4)
		SetScrollX(std::max(0.0f, (len + TextStart + 4) * CharAdv.x - width));
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
}

void CodeEditor::MoveLeft(int aAmount, bool aSelect, bool aWordMode) {
	if (CodeLines.empty())
		return;

	Coordinates oldPos = State.CursorPosition;
	State.CursorPosition = GetActualCursorCoordinates();

	while (aAmount-- > 0) {
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
}

void CodeEditor::MoveRight(int aAmount, bool aSelect, bool aWordMode) {
	Coordinates oldPos = State.CursorPosition;

	if (CodeLines.empty())
		return;

	while (aAmount-- > 0) {
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
}

void CodeEditor::MoveHome(bool aSelect) {
	Coordinates oldPos = State.CursorPosition;
	if (HasSelection()) {
		Coordinates selStart, selEnd;
		GetSelection(selStart, selEnd);
		SetCursorPosition(Coordinates(selStart.Line, 0));
	} else {
		SetCursorPosition(Coordinates(State.CursorPosition.Line, 0));
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
	if (aStart > aEnd)
		std::swap(State.SelectionStart, State.SelectionEnd);

	if (aWordMode) {
		State.SelectionStart = FindWordStart(State.SelectionStart);
		if (!IsOnWordBoundary(State.SelectionEnd))
			State.SelectionEnd = FindWordEnd(FindWordStart(State.SelectionEnd));
	}
}

void CodeEditor::SelectWordUnderCursor(void) {
	Coordinates c = GetCursorPosition();
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

			OnModified();

			Coordinates pos = u.Start < u.End ? u.Start : u.End;
			OnChanged(pos, pos, 0);
		}
	}
}

void CodeEditor::Paste(void) {
	const char* const clipText = GetClipboardText();
	if (clipText != nullptr && strlen(clipText) > 0) {
		UndoRecord u;
		u.Type = UndoType::Add;
		u.Before = State;

		if (HasSelection()) {
			u.Overwritten = GetSelectionText();
			DeleteSelection();
		}

		u.Content = clipText;
		u.Start = GetActualCursorCoordinates();

		InsertText(clipText);

		u.End = GetActualCursorCoordinates();
		u.After = State;
		AddUndo(u);

		OnModified();

		OnChanged(u.Start, u.End, 0);

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
		OnChanged(pos, pos, 0);
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

		OnChanged(pos, pos, 0);
	}

	u.After = State;
	AddUndo(u);

	OnModified();
}

void CodeEditor::Indent(bool aByKey) {
	if (IsReadOnly())
		return;

	if (HasSelection() && GetSelectionLines() > 1) {
		UndoRecord u;
		u.Type = UndoType::Indent;
		u.Before = State;

		u.Start = State.SelectionStart;
		u.End = State.SelectionEnd;

		for (int i = u.Start.Line; i <= u.End.Line; ++i) {
			Line &line = CodeLines[i];
			if (line.Glyphs.empty()) {
				u.Content.push_back(0);

				continue;
			}
			line.Glyphs.insert(line.Glyphs.begin(), Glyph('\t', PaletteIndex::Default));
			u.Content.push_back(std::numeric_limits<char>::max());

			Coordinates pos(i, 0);
			OnChanged(pos, pos, 0);
		}

		State.SelectionEnd.Column = (int)CodeLines[State.SelectionEnd.Line].Glyphs.size();

		u.After = State;
		AddUndo(u);

		OnModified();
	} else {
		if (!aByKey)
			EnterCharacter('\t');
	}
}

void CodeEditor::Unindent(bool aByKey) {
	if (IsReadOnly())
		return;

	if (HasSelection() && GetSelectionLines() > 1) {
		UndoRecord u;
		u.Type = UndoType::Unindent;
		u.Before = State;

		u.Start = State.SelectionStart;
		u.End = State.SelectionEnd;

		int affectedLines = 0;
		for (int i = u.Start.Line; i <= u.End.Line; ++i) {
			Line &line = CodeLines[i];
			if (line.Glyphs.empty()) {
				u.Content.push_back(0);

				continue;
			}

			const Glyph &g = *line.Glyphs.begin();
			if (g.Character == '\t') {
				line.Glyphs.erase(line.Glyphs.begin());
				u.Content.push_back(std::numeric_limits<char>::max());
				++affectedLines;

				Coordinates pos(i, 0);
				OnChanged(pos, pos, 0);
			} else if (g.Character == ' ') {
				int k = 0;
				for (int j = 0; j < TabSize; ++j, ++k) {
					if (line.Glyphs.empty())
						break;
					const Glyph &h = *line.Glyphs.begin();
					if (h.Character != ' ')
						break;
					line.Glyphs.erase(line.Glyphs.begin());
				}
				u.Content.push_back((char)k);
				if (k)
					++affectedLines;

				Coordinates pos(i, 0);
				OnChanged(pos, pos, 0);
			} else {
				u.Content.push_back(0);
			}
		}
		if (affectedLines > 0) {
			const Line &line = CodeLines[State.SelectionEnd.Line];
			if ((int)line.Glyphs.size() < State.SelectionEnd.Column)
				State.SelectionEnd.Column = (int)line.Glyphs.size();
		}

		u.After = State;
		if (affectedLines > 0) {
			AddUndo(u);

			OnModified();
		}
	} else {
		if (!aByKey) {
			Char cc = GetCharUnderCursor();
			if (cc == '\t' || cc == ' ')
				BackSpace(); // Unindent single line.
		}
	}
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

const CodeEditor::Palette &CodeEditor::GetDarkPalette(void) {
	static Palette p = {
		0xffffffff, // None.
		0xffd69c56, // Keyword.
		0xffa8ceb5, // Number.
		0xff859dd6, // String.
		0xff70a0e0, // Char literal.
		0xffb4b4b4, // Punctuation.
		0xff409090, // Preprocessor.
		0xffdadada, // Identifier.
		0xffb0c94e, // Known identifier.
		0xffc040a0, // Preproc identifier.
		0xff4aa657, // Comment (single line).
		0xff4aa657, // Comment (multi line).
		0x90909090, // Space.
		0xff2C2C2C, // Background.
		0xffe0e0e0, // Cursor.
		0x80a06020, // Selection.
		0x804d00ff, // ErrorMarker.
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

	return p;
}

const CodeEditor::Palette &CodeEditor::GetLightPalette(void) {
	static Palette p = {
		0xff000000, // None.
		0xffff0c06, // Keyword.
		0xff008000, // Number.
		0xff2020a0, // String.
		0xff304070, // Char literal.
		0xff000000, // Punctuation.
		0xff409090, // Preprocessor.
		0xff404040, // Identifier.
		0xff606010, // Known identifier.
		0xffc040a0, // Preproc identifier.
		0xff205020, // Comment (single line).
		0xff405020, // Comment (multi line).
		0xffaf912b, // Space.
		0xffffffff, // Background.
		0xff000000, // Cursor.
		0xffffd6ad, // Selection.
		0xa00010ff, // ErrorMarker.
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

	return p;
}

const CodeEditor::Palette &CodeEditor::GetRetroBluePalette(void) {
	static Palette p = {
		0xff00ffff, // None.
		0xffffff00, // Keyword.
		0xff00ff00, // Number.
		0xff808000, // String.
		0xff808000, // Char literal.
		0xffffffff, // Punctuation.
		0xff008000, // Preprocessor.
		0xff00ffff, // Identifier.
		0xffffffff, // Known identifier.
		0xffff00ff, // Preproc identifier.
		0xffb0b0b0, // Comment (single line).
		0xffa0a0a0, // Comment (multi line).
		0x90909090, // Space.
		0xff753929, // Background.
		0xff0080ff, // Cursor.
		0x80ffff00, // Selection.
		0xa00000ff, // ErrorMarker.
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

	return p;
}

void CodeEditor::RenderText(int &aOffset, const ImVec2 &aPosition, ImU32 aPalette, ImU32 aColor, const char* aText, int aWidth) {
	ImDrawList* drawList = GetWindowDrawList();

	if (aPalette != (ImU32)PaletteIndex::MultiLineComment && (*aText == '\t' || *aText == ' ')) {
		ImVec2 step = aPosition;
		while (*aText) {
			const float size = GetFontSize();
			if (*aText == '\t') {
				const int num = TabSize - aOffset % TabSize;
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
				if (n == 1) {
					step.x += CharAdv.x;
					aOffset += 1;
				} else {
					step.x += CharAdv.x * ICE_UTF_CHAR_WIDTH;
					aOffset += ICE_UTF_CHAR_WIDTH;
				}

				aText += n;
			}
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
					std::string id = buffer.substr(start, end - start);
					PaletteIndex color = regex.second;
					if (color == PaletteIndex::Identifier) {
						if (!LangDef.CaseSensitive)
							std::transform(id.begin(), id.end(), id.begin(), ICE_CASE_FUNC);

						if (!preproc) {
							if (LangDef.Keys.find(id) != LangDef.Keys.end())
								color = PaletteIndex::Keyword;
							else if (LangDef.Ids.find(id) != LangDef.Ids.end())
								color = PaletteIndex::KnownIdentifier;
							else if (LangDef.PreprocIds.find(id) != LangDef.PreprocIds.end())
								color = PaletteIndex::PreprocIdentifier;
						} else {
							if (LangDef.PreprocIds.find(id) != LangDef.PreprocIds.end())
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

void CodeEditor::ColorizeInternal(void) {
	if (CodeLines.empty())
		return;

	if (CheckMultilineComments && GetFrameCount() > CheckMultilineComments) {
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
						const std::string &startStr = LangDef.CommentStart;
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
						const std::string &endStr = LangDef.CommentEnd;
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
		CheckMultilineComments = 0;

		OnColorized(true);

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
		if (line < aValue.Line)
			column = (int)CodeLines[line].Glyphs.size();
		else
			column = std::min((int)CodeLines[line].Glyphs.size(), aValue.Column);
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

int CodeEditor::AppendBuffer(std::string &aBuffer, const Glyph &g, int aIndex, int &aWidth) {
	Char chr = g.Character;
	if (chr == '\t') {
		int num = 0;
		const bool literal = g.MultiLineComment ||
			g.ColorIndex == (ImU32)PaletteIndex::String ||
			g.ColorIndex == (ImU32)PaletteIndex::Comment ||
			g.ColorIndex == (ImU32)PaletteIndex::MultiLineComment;
		if (literal)
			num = TabSize;
		else
			num = TabSize - aIndex % TabSize;
		aBuffer.push_back('\t');
		aWidth += num;

		return aIndex + num;
	} else {
		if (ImTextAppendUtf8ToStdStr(aBuffer, chr) <= 1) {
			++aWidth;

			return aIndex + 1;
		} else {
			const int w = GetCharacterWidth(g);
			aWidth += w;

			return aIndex + w;
		}
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

		OnChanged(State.SelectionStart, State.SelectionStart, 0);
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

			OnChanged(State.CursorPosition, State.CursorPosition, 0);
		} else {
			Line &line = CodeLines[State.CursorPosition.Line];

			u.Content.clear();
			ImTextAppendUtf8ToStdStr(u.Content, line.Glyphs[pos.Column - 1].Character);
			u.Start = u.End = GetActualCursorCoordinates();
			--u.Start.Column;

			--State.CursorPosition.Column;
			if (State.CursorPosition.Column < (int)line.Glyphs.size())
				line.Glyphs.erase(line.Glyphs.begin() + State.CursorPosition.Column);

			OnChanged(State.CursorPosition, State.CursorPosition, 0);
		}
		EnsureCursorVisible();
		Colorize(State.CursorPosition.Line, 1);
	}

	u.After = State;
	AddUndo(u);

	OnModified();
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

	const Coordinates coord = GetActualCursorCoordinates();
	u.Start = coord;

	if (CodeLines.empty())
		CodeLines.push_back(Line());

	if (aChar == '\n') {
		InsertLine(coord.Line + 1);
		Line &line = CodeLines[coord.Line];
		Line &newLine = CodeLines[coord.Line + 1];
		newLine.Glyphs.insert(newLine.Glyphs.begin(), line.Glyphs.begin() + coord.Column, line.Glyphs.end());
		line.Glyphs.erase(line.Glyphs.begin() + coord.Column, line.Glyphs.begin() + line.Glyphs.size());
		State.CursorPosition = Coordinates(coord.Line + 1, 0);

		ImTextAppendUtf8ToStdStr(u.Content, aChar);

		// Get indent spacec from the last line.
		int indent = 0;
		bool broken = false;
		for (size_t i = 0; i < line.Glyphs.size(); ++i) {
			const Glyph &g = line.Glyphs[i];
			if (g.Character == ' ') {
				++indent;
			} else if (g.Character == '\t') {
				indent += TabSize;
			} else {
				broken = true;

				break;
			}
		}
		// Automatic indent for the new line.
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

		OnChanged(coord, Coordinates(coord.Line + 1, 0), 0);
	} else {
		Line &line = CodeLines[coord.Line];
		if (Overwrite && (int)line.Glyphs.size() > coord.Column)
			line.Glyphs[coord.Column] = Glyph(aChar, PaletteIndex::Default);
		else
			line.Glyphs.insert(line.Glyphs.begin() + coord.Column, Glyph(aChar, PaletteIndex::Default));
		State.CursorPosition = coord;
		++State.CursorPosition.Column;

		ImTextAppendUtf8ToStdStr(u.Content, aChar);

		OnChanged(coord, coord, 0);
	}

	InteractiveStart = InteractiveEnd = State.CursorPosition;

	u.End = GetActualCursorCoordinates();
	u.After = State;

	AddUndo(u);

	Colorize(coord.Line - 1, 3);
	EnsureCursorVisible();

	OnModified();
}

CodeEditor::Coordinates CodeEditor::FindWordStart(const Coordinates &aFrom) const {
	Coordinates at = aFrom;
	if (at.Line >= (int)CodeLines.size())
		return at;

	const Line &line = CodeLines[at.Line];

	if (at.Column >= (int)line.Glyphs.size())
		return at;

	PaletteIndex cstart = (PaletteIndex)line.Glyphs[at.Column].ColorIndex;
	while (at.Column > 0) {
		if (cstart != (PaletteIndex)line.Glyphs[at.Column - 1].ColorIndex)
			break;

		--at.Column;
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
	while (at.Column < (int)line.Glyphs.size()) {
		if (cstart != (PaletteIndex)line.Glyphs[at.Column].ColorIndex)
			break;

		++at.Column;
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

CodeEditor::Char CodeEditor::GetCharUnderCursor(void) const {
	Coordinates c = GetCursorPosition();
	if (--c.Column < 0)
		return '\0';

	const Glyph &g = CodeLines[c.Line].Glyphs[c.Column];

	return g.Character;
}

void CodeEditor::OnChanged(const Coordinates &aStart, const Coordinates &aEnd, int aOffset) {
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

void CodeEditor::OnModified(void) const {
	if (ModifiedHandler == nullptr)
		return;

	ModifiedHandler();
}

void CodeEditor::OnHeadClicked(int aLine) const {
	if (HeadClickedHandler == nullptr)
		return;

	HeadClickedHandler(aLine);
}

}
