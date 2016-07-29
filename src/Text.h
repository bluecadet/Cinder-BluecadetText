#pragma once

#include "cinder/Color.h"

#include <codecvt>
#include <string>

#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>

namespace bluecadet {
namespace text {

//==================================================
// Types
//

typedef std::wstring StringType;

enum TextAlign {
	Left,
	Right,
	Center
};

enum FontStyle {
	Normal,
	Italic,
	Oblique
};

enum TextTransform {
	None,
	Uppercase,
	Lowercase,
	Capitalize
};

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
	std::string		mFontFamily = "Arial";
	int				mFontWeight = FontWeight::Regular;
	FontStyle		mFontStyle = FontStyle::Normal;
	float			mFontSize = 16.0f;
	ci::ColorA		mColor = ci::ColorA(0.0f, 0.0f, 0.0f, 1.0f);
	TextAlign		mTextAlign = TextAlign::Left;
	TextTransform	mTextTransform = TextTransform::None;
	float			mLeadingOffset = 0;

	//! Convenience setters
	Style& fontFamily(std::string fontFamily) { mFontFamily = fontFamily; return *this; }
	Style& fontWeight(int fontWeight) { mFontWeight = fontWeight; return *this; }
	Style& fontStyle(FontStyle fontStyle) { mFontStyle = fontStyle; return *this; }
	Style& fontSize(float fontSize) { mFontSize = fontSize; return *this; }
	Style& color(ci::ColorA color) { mColor = color; return *this; }
	Style& textAlign(TextAlign textAlign) { mTextAlign = textAlign; return *this; }
	Style& textTransform(TextTransform textTransform) { mTextTransform = textTransform; return *this; }
	Style& leadingOffset(float leadingOffset) { mLeadingOffset = leadingOffset; return *this; }

	//! Operators
	bool operator == (const Style &rhs) const {
		return
			mFontFamily == rhs.mFontFamily &&
			mFontWeight == rhs.mFontWeight &&
			mFontStyle == rhs.mFontStyle &&
			mFontSize == rhs.mFontSize &&
			mColor == rhs.mColor &&
			mTextAlign == rhs.mTextAlign &&
			mTextTransform == rhs.mTextTransform &&
			mLeadingOffset == rhs.mLeadingOffset;
	}

	bool operator != (const Style &rhs) const {
		return !(*this == rhs);
	}
};

struct StyledText {
	Style mStyle;
	StringType mWText;
	StyledText(const Style& style, const StringType& wtext) : mStyle(style), mWText(wtext) {}
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
	std::cout << "text: Warning: TextAlign '" << TextAlignStr << "' is not supported. Defaulting to 'left'" << std::endl;
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
	std::cout << "text: Warning: TextTransform '" << textTransformStr << "' is not supported. Defaulting to 'none'" << std::endl;
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
	std::cout << "text: Warning: Font style '" << styleString << "' is not supported. Defaulting to 'normal'" << std::endl;
	return Normal;
}

inline ci::ColorA getColorFromRGBA(uint32_t hexValue) {
	uint8_t red = (hexValue >> 24) & 255;
	uint8_t green = (hexValue >> 16) & 255;
	uint8_t blue = (hexValue >> 8) & 255;
	uint8_t alpha = hexValue & 255;
	return ci::ColorA(ci::CHANTRAIT<float>::convert(red), ci::CHANTRAIT<float>::convert(green), ci::CHANTRAIT<float>::convert(blue), ci::CHANTRAIT<float>::convert(alpha));
}

inline ci::ColorA getColorFromRGB(uint32_t hexValue) {
	uint8_t red = (hexValue >> 16) & 255;
	uint8_t green = (hexValue >> 8) & 255;
	uint8_t blue = hexValue & 255;
	return ci::ColorA(ci::CHANTRAIT<float>::convert(red), ci::CHANTRAIT<float>::convert(green), ci::CHANTRAIT<float>::convert(blue));
}

inline ci::ColorA getColorFromString(std::string colorStr) {
	if (colorStr.empty()) return ci::Color::black();
	if (colorStr.substr(0, 1) == "#") colorStr = colorStr.substr(1, colorStr.length() - 1);
	if (colorStr.substr(0, 2) != "0x") colorStr = "0x" + colorStr;
	uint32_t c = std::stoul(colorStr, nullptr, 16);
	if (colorStr.length() <= 8) {
		return getColorFromRGB(c);
	}
	return getColorFromRGBA(c);
}

//==================================================
// Text helpers
//

//! Splits a string into tokens based on delimiters. All delimiters are returned as tokens themselves.
template <typename StringType, typename ContainerType>
inline void tokenize(const StringType& str, ContainerType& tokenContainer, const StringType& delimiters) {
	typedef StringType::value_type CharType;
	typedef boost::tokenizer<boost::char_separator<wchar_t>, StringType::const_iterator, StringType> tokenizer;
	boost::char_separator<wchar_t> sep{ StringType().c_str(), delimiters.c_str() };
	tokenizer tok{ str, sep };
	for (const auto &t : tok) {
		tokenContainer.push_back(t);
	}
}

//! Splits a string into tokens based on delimiters. All delimiters are returned as tokens themselves.
template <typename StringType>
inline std::list<StringType> tokenize(const StringType& str, const StringType& delimiters) {
	std::list<StringType> tokenContainer;
	typedef StringType::value_type CharType;
	typedef boost::tokenizer<boost::char_separator<wchar_t>, StringType::const_iterator, StringType> tokenizer;
	boost::char_separator<wchar_t> sep{ StringType().c_str(), delimiters.c_str() };
	tokenizer tok{ str, sep };
	for (const auto &t : tok) {
		tokenContainer.push_back(t);
	}
	return tokenContainer;
}

//! Transforms text case. Returns a copy of the original text.
template <typename StringType>
inline StringType transform(const StringType& text, const TextTransform transform) {
	switch (transform) {
	case TextTransform::None: return text;
		// using boost::locale is better for international case conversion, but not included in default cinder boost build
		/*case TextTransform::Uppercase: run->append(boost::locale::to_upper(token)); break;
		case TextTransform::Lowercase: run->append(boost::locale::to_lower(token)); break;
		case TextTransform::Capitalize: run->append(boost::locale::to_title(token)); break;*/
	case TextTransform::Uppercase: return boost::algorithm::to_upper_copy(text);
	case TextTransform::Lowercase: return boost::algorithm::to_lower_copy(text);
	case TextTransform::Capitalize: return capitalize(text);
	default: return text;
	}
}

template <typename StringType>
inline StringType capitalize(const StringType& text) {
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

template <typename CharType>	inline CharType isAlpha(const CharType& c) {}
template <>						inline char isAlpha<char>(const char& c) { return std::isalpha(c); }
template <>						inline wchar_t isAlpha<wchar_t>(const wchar_t& c) { return std::iswalpha(c); }

template <typename CharType>	inline CharType isAlNum(const CharType& c) {}
template <>						inline char isAlNum<char>(const char& c) { return std::isalnum(c); }
template <>						inline wchar_t isAlNum<wchar_t>(const wchar_t& c) { return std::iswalnum(c); }

template <typename CharType>	inline CharType isPunct(const CharType& c) {}
template <>						inline char isPunct<char>(const char& c) { return std::ispunct(c); }
template <>						inline wchar_t isPunct<wchar_t>(const wchar_t& c) { return std::ispunct(c); }

template <typename CharType>	inline CharType toUpper(const CharType& c) {}
template <>						inline char toUpper<char>(const char& c) { return std::toupper(c); }
template <>						inline wchar_t toUpper<wchar_t>(const wchar_t& c) { return std::towupper(c); }

//==================================================
// String type conversion
//

inline std::u16string u16String(const std::string& narrow_utf8_source_string) {
	static std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> converter;
	return converter.from_bytes(narrow_utf8_source_string);
}

inline std::wstring wideString(const std::string& narrow_utf8_source_string) {
	static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> converter;
	return converter.from_bytes(narrow_utf8_source_string);
}

inline std::string narrowString(const std::wstring& wide_source_string) {
	static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> converter;
	return converter.to_bytes(wide_source_string);
}

inline std::string narrowString(const std::u16string& wide_utf16_source_string) {
	static std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> converter;
	return converter.to_bytes(wide_utf16_source_string);
}

}
}
