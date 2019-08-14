#pragma once

#include "cinder/Color.h"

#include <codecvt>
#include <cstdint>
#include <map>
#include <stack>
#include <string>
#include <sstream> 

#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>

namespace bluecadet {
namespace text {

//==================================================
// Types
//

typedef std::wstring StringType;

enum TextAlign { Left, Right, Center };

enum FontStyle { Normal, Italic, Oblique };

enum TextTransform { None, Uppercase, Lowercase, Capitalize };

enum FontWeight {
	Thin = 100,
	ExtraLight = 200,
	Light = 300,
	Regular = 400,
	Medium = 500,
	Semibold = 600,
	Bold = 700,
	ExtraBold = 800,
	Heavy = 900
};

struct Style {
	//! Properties
	std::string mFontFamily = "Arial";
	int mFontWeight = FontWeight::Regular;
	FontStyle mFontStyle = FontStyle::Normal;
	float mFontSize = 16.0f;
	ci::ColorA mColor = ci::ColorA(0.0f, 0.0f, 0.0f, 1.0f);
	TextAlign mTextAlign = TextAlign::Left;
	TextTransform mTextTransform = TextTransform::None;
	float mLeadingOffset = 0;

	//! Convenience setters
	Style & fontFamily(std::string fontFamily) {
		mFontFamily = fontFamily;
		return *this;
	}
	Style & fontWeight(int fontWeight) {
		mFontWeight = fontWeight;
		return *this;
	}
	Style & fontStyle(FontStyle fontStyle) {
		mFontStyle = fontStyle;
		return *this;
	}
	Style & fontSize(float fontSize) {
		mFontSize = fontSize;
		return *this;
	}
	Style & color(ci::ColorA color) {
		mColor = color;
		return *this;
	}
	Style & textAlign(TextAlign textAlign) {
		mTextAlign = textAlign;
		return *this;
	}
	Style & textTransform(TextTransform textTransform) {
		mTextTransform = textTransform;
		return *this;
	}
	Style & leadingOffset(float leadingOffset) {
		mLeadingOffset = leadingOffset;
		return *this;
	}

	//! Operators
	bool operator==(const Style & rhs) const {
		return mFontFamily == rhs.mFontFamily && mFontWeight == rhs.mFontWeight && mFontStyle == rhs.mFontStyle &&
			   mFontSize == rhs.mFontSize && mColor == rhs.mColor && mTextAlign == rhs.mTextAlign &&
			   mTextTransform == rhs.mTextTransform && mLeadingOffset == rhs.mLeadingOffset;
	}

	bool operator!=(const Style & rhs) const { return !(*this == rhs); }
};

struct StyledText {
	Style mStyle;
	StringType mWText;
	StyledText(const Style & style, const StringType & wtext) : mStyle(style), mWText(wtext) {}
};

//==================================================
// Property parsing helpers
//

inline std::string getStringFromTextAlign(TextAlign TextAlign) {
	switch (TextAlign) {
		case Left: return "left";
		case Center: return "center";
		case Right: return "right";
		default: return "left";
	}
}

inline TextAlign getTextAlignFromString(std::string TextAlignStr) {
	if (TextAlignStr == "left") return Left;
	if (TextAlignStr == "center") return Center;
	if (TextAlignStr == "right") return Right;
	std::cout << "text: Warning: TextAlign '" << TextAlignStr << "' is not supported. Defaulting to 'left'"
			  << std::endl;
	return Left;
}

inline std::string getStringFromTextTransform(TextTransform textTransform) {
	switch (textTransform) {
		case None: return "none";
		case Uppercase: return "uppercase";
		case Lowercase: return "lowercase";
		case Capitalize: return "capitalize";
		default: return "none";
	}
}

inline TextTransform getTextTransformFromString(std::string textTransformStr) {
	if (textTransformStr == "none") return None;
	if (textTransformStr == "uppercase") return Uppercase;
	if (textTransformStr == "lowercase") return Lowercase;
	if (textTransformStr == "capitalize") return Capitalize;
	std::cout << "text: Warning: TextTransform '" << textTransformStr << "' is not supported. Defaulting to 'none'"
			  << std::endl;
	return None;
}

inline std::string getStringFromFontStyle(FontStyle style) {
	switch (style) {
		case Normal: return "normal";
		case Italic: return "italic";
		case Oblique: return "oblique";
		default: return "normal";
	}
}

inline FontStyle getFontStyleFromString(std::string styleString) {
	if (styleString == "normal") return Normal;
	if (styleString == "italic") return Italic;
	if (styleString == "oblique") return Oblique;
	std::cout << "text: Warning: Font style '" << styleString << "' is not supported. Defaulting to 'normal'"
			  << std::endl;
	return Normal;
}

// supports #RRGGBB, 0xRRGGBB, #AARRGGBB and 0xAARRGGBB
inline ci::ColorA hexStrToColor(std::string colorStr) {
	if (colorStr.empty()) return ci::ColorA::black();
	if (colorStr.substr(0, 1) == "#") colorStr = colorStr.substr(1, colorStr.length() - 1);
	if (colorStr.substr(0, 2) == "0x") colorStr = colorStr.substr(2, colorStr.length() - 2);

	if (colorStr.length() < 8) {
		uint32_t c = (uint32_t)std::stoul(colorStr.substr(0, 6), nullptr, 16);
		return ci::Color::hex(c);
	} else {
		uint32_t c = (uint32_t)std::stoul(colorStr.substr(0, 8), nullptr, 16);
		return ci::ColorA::hexA(c);
	}
}

// Legacy alias for hexStrToColor
inline ci::ColorA getColorFromString(const std::string & colorStr) {
	return hexStrToColor(colorStr);
}

inline std::string intToHexStr(const uint32_t i, const uint8_t numDigits = 8, const std::string & prefix = "0x") {
	// See https://stackoverflow.com/a/5100745/782899
	std::stringstream stream;
	stream << prefix << std::setfill('0') << std::setw(numDigits) << std::hex << i;
	return stream.str();
}

inline uint32_t colorToHex(const ci::Color8u & color) {
	return (((uint32_t)color.r & 0xff) << 16) | (((uint32_t)color.g & 0xff) << 8) | ((uint32_t)color.b & 0xff);
}

inline uint32_t colorToHexA(const ci::ColorA8u & color) {
	return (((uint32_t)color.a & 0xff) << 24) | (((uint32_t)color.r & 0xff) << 16) | (((uint32_t)color.g & 0xff) << 8) |
		   ((uint32_t)color.b & 0xff);
}

inline uint32_t colorToHexA(const ci::Color & color) {
	return colorToHex(ci::Color8u(color));
}

inline uint32_t colorToHexA(const ci::ColorA & color) {
	return colorToHexA(ci::ColorA8u(color));
}

inline std::string colorToHexStr(const ci::Color8u & color, const std::string & prefix = "0x") {
	return intToHexStr(colorToHex(color), 6, prefix);
}

inline std::string colorToHexStr(const ci::ColorA8u & color, const std::string & prefix = "0x") {
	return intToHexStr(colorToHexA(color), 8, prefix);
}

inline std::string colorToHexStr(const ci::Color & color, const std::string & prefix = "0x") {
	return colorToHexStr(ci::Color8u(color), prefix);
}

inline std::string colorToHexStr(const ci::ColorA & color, const std::string & prefix = "0x") {
	return colorToHexStr(ci::ColorA8u(color), prefix);
}

//==================================================
// Text helpers
//

//! Splits a string into tokens based on delimiters. All delimiters are returned as tokens themselves.
template <typename StringType, typename ContainerType>
inline void tokenize(const StringType & str, ContainerType & tokenContainer, const StringType & delimiters) {
	typedef typename StringType::value_type CharType;
	typedef boost::tokenizer<boost::char_separator<wchar_t>, typename StringType::const_iterator, StringType> tokenizer;
	boost::char_separator<wchar_t> sep{StringType().c_str(), delimiters.c_str()};
	tokenizer tok{str, sep};
	for (const auto & t : tok) {
		tokenContainer.push_back(t);
	}
}

//! Splits a string into tokens based on delimiters. All delimiters are returned as tokens themselves.
template <typename StringType>
inline std::list<StringType> tokenize(const StringType & str, const StringType & delimiters) {
	std::list<StringType> tokenContainer;
	typedef typename StringType::value_type CharType;
	typedef boost::tokenizer<boost::char_separator<wchar_t>, typename StringType::const_iterator, StringType> tokenizer;
	boost::char_separator<wchar_t> sep{StringType().c_str(), delimiters.c_str()};
	tokenizer tok{str, sep};
	for (const auto & t : tok) {
		tokenContainer.push_back(t);
	}
	return tokenContainer;
}

//! Joins a list of strings with a join character.
template <typename StringType, typename ContainerType>
StringType join(const ContainerType & items, StringType & delimiter = ".") {
	StringType s;
	for (const auto & item : items) {
		if (!s.empty()) {
			s += delimiter;
		}
		s += item;
	}
	return s;
}

//! Splits a string into tokens based on delimiters. Does not include delimiters.
template <typename StringType, typename ContainerType> inline const auto split(const StringType & s, char delimiter) {
	ContainerType tokens;
	StringType token;
	std::istringstream tokenStream(s);
	while (std::getline(tokenStream, token, delimiter)) {
		tokens.push_back(token);
	}
	return tokens;
}

//! std::vector specialization of split().
template <typename StringType> inline const auto split(const StringType & s, char delimiter) {
	return split<StringType, std::vector<StringType>>(s, delimiter);
}

//! Transforms text case. Returns a copy of the original text.
template <typename StringType> inline StringType transform(const StringType & text, const TextTransform transform) {
	switch (transform) {
		case TextTransform::None:
			return text;
			// using boost::locale is better for international case conversion, but not included in default cinder boost
			// build
			/*case TextTransform::Uppercase: run->append(boost::locale::to_upper(token)); break;
			case TextTransform::Lowercase: run->append(boost::locale::to_lower(token)); break;
			case TextTransform::Capitalize: run->append(boost::locale::to_title(token)); break;*/
		case TextTransform::Uppercase: return boost::algorithm::to_upper_copy(text);
		case TextTransform::Lowercase: return boost::algorithm::to_lower_copy(text);
		case TextTransform::Capitalize: return capitalize(text);
		default: return text;
	}
}

template <typename StringType> inline StringType capitalize(const StringType & text) {
	StringType result(text);
	bool isWordCapped = false;

	for (int i = 0; i < result.length(); ++i) {
		const auto c = result[i];

		if (!isAlpha(c)) {
			isWordCapped = false;
			continue;
		}

		if (!isWordCapped) {
			result[i] = toUpper(c);
			isWordCapped = true;
		}
	}
	return result;
}

//==================================================
// Templates for stl string functions
//

inline char isAlpha(const char & c) {
	return std::isalpha(c) != 0;
}
inline wchar_t isAlpha(const wchar_t & c) {
	return std::iswalpha(c) != 0;
}

inline char isAlNum(const char & c) {
	return std::isalnum(c) != 0;
}
inline wchar_t isAlNum(const wchar_t & c) {
	return std::iswalnum(c) != 0;
}

inline char isPunct(const char & c) {
	return std::ispunct(c) != 0;
}

inline wchar_t isPunct(const wchar_t & c) {
	return std::iswpunct(c) != 0;
}

inline bool isSpace(const char c) {
	return std::isspace(c) != 0;
}
inline bool isSpace(const wchar_t c) {
	return std::iswspace(c) != 0;
}

inline char toUpper(const char c) {
	return std::toupper(c);
}
inline wchar_t toUpper(const wchar_t c) {
	return std::towupper(c);
}

//==================================================
// String type conversion
//

inline std::u16string u16String(const std::string & narrow_utf8_source_string) {
	static std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> converter;
	return converter.from_bytes(narrow_utf8_source_string);
}

inline std::wstring wideString(const std::string & narrow_utf8_source_string) {
	static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> converter;
	return converter.from_bytes(narrow_utf8_source_string);
}

inline std::string narrowString(const std::wstring & wide_source_string) {
	static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> converter;
	return converter.to_bytes(wide_source_string);
}

inline std::string narrowString(const std::u16string & wide_utf16_source_string) {
	static std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> converter;
	return converter.to_bytes(wide_utf16_source_string);
}

//==================================================
// Token parser definition used by StyledTextParser
//

// Define token parsers according to this signature. Process token. Modify segments or styles if needed.
typedef std::function<void(StringType token, const int options, std::vector<StyledText> & segments,
						   std::stack<Style> & styles)>
	TokenParserFn;

typedef std::map<std::wstring, TokenParserFn> TokenParserMap;
typedef std::shared_ptr<TokenParserMap> TokenParserMapRef;

}  // namespace text
}  // namespace bluecadet
