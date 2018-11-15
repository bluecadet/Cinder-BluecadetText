#include "StyledTextParser.h"
#include "cinder/Json.h"

using namespace ci;
using namespace ci::app;
using namespace std;

namespace bluecadet {
namespace text {

StyledTextParser::StyledTextParser() {
	mDefaultOptions = 0;

	// Construct default token parsers
	{
		// root
		TokenParser tokenParser = [](StringType &token, const int options, std::vector<StyledText> &segments, std::stack<Style> &styles) {
			return false;
		};

		mDefaultTokenParsers[L"<root>"] = tokenParser;
		mDefaultTokenParsers[L"</root>"] = tokenParser;
	}

	{
		// Italic
		TokenParser tokenParser = [](StringType &token, const int options, std::vector<StyledText> &segments, std::stack<Style> &styles) {
			const bool shouldInvert = (options & INVERT_NESTED_ITALICS) && (styles.top().mFontStyle == FontStyle::Italic);
			Style style = Style(styles.top()).fontStyle(shouldInvert ? FontStyle::Normal : FontStyle::Italic);
			styles.push(style);

			return false;
		};

		mDefaultTokenParsers[L"<i>"] = tokenParser;
		mDefaultTokenParsers[L"<em>"] = tokenParser;
	}

	{
		// Bold
		TokenParser tokenParser = [](StringType &token, const int options, std::vector<StyledText> &segments, std::stack<Style> &styles) {
			Style style = Style(styles.top()).fontWeight(FontWeight::Bold);
			styles.push(style);

			return false;
		};

		mDefaultTokenParsers[L"<b>"] = tokenParser;
		mDefaultTokenParsers[L"<strong>"] = tokenParser;
	}

	{
		// Remove styles from stack when tag is closed
		TokenParser tokenParser = [](StringType &token, const int options, std::vector<StyledText> &segments, std::stack<Style> &styles) {
			if (!styles.empty()) styles.pop();

			return false;
		};

		mDefaultTokenParsers[L"</i>"] = tokenParser;
		mDefaultTokenParsers[L"</em>"] = tokenParser;
		mDefaultTokenParsers[L"</b>"] = tokenParser;
		mDefaultTokenParsers[L"</strong>"] = tokenParser;
	}

	{
		// Break
		TokenParser tokenParser = [](StringType &token, const int options, std::vector<StyledText> &segments, std::stack<Style> &styles) {
			if (options & STRIP_BREAK_TAGS) {
				return false;
			} else {
				token = L"\n";
				return true;
			}
		};

		mDefaultTokenParsers[L"<br />"] = tokenParser;
		mDefaultTokenParsers[L"<br/>"] = tokenParser;
		mDefaultTokenParsers[L"<br>"] = tokenParser;
	}

	{
		// Paragraph
		TokenParser tokenParser = [](StringType &token, const int options, std::vector<StyledText> &segments, std::stack<Style> &styles) {
			if (options & STRIP_PARAGRAPH_TAG) {
				return false;
			} else {
				token = L"\n";
				return true;
			}
		};

		mDefaultTokenParsers[L"<p>"] = tokenParser;
		mDefaultTokenParsers[L"</p>"] = tokenParser;
	}

	{
		// Angled bracket
		TokenParser tokenParser = [](StringType &token, const int options, std::vector<StyledText> &segments, std::stack<Style> &styles) {
			token = L"<";
			return true;
		};

		mDefaultTokenParsers[L"&lt;"] = tokenParser;
	}

	{
		// Angled bracket
		TokenParser tokenParser = [](StringType &token, const int options, std::vector<StyledText> &segments, std::stack<Style> &styles) {
			token = L">";
			return true;
		};

		mDefaultTokenParsers[L"&gt;"] = tokenParser;
	}

	{
		// Link break
		TokenParser tokenParser = [](StringType &token, const int options, std::vector<StyledText> &segments, std::stack<Style> &styles) {
			if (options & TRIM_LEADING_BREAKS && segments.empty()) {
				return false;
			} else {
				return true;
			}
		};

		mDefaultTokenParsers[L"\n"] = tokenParser;
	}
}

StyledTextParser::~StyledTextParser() {
}

std::vector<StyledText> StyledTextParser::parse(const StringType& str, Style baseStyle) {
	return parse(str, baseStyle, mDefaultOptions);
}

std::vector<StyledText> StyledTextParser::parse(const StringType& str, Style baseStyle, int options, const TokenParserMapRef customTokenParsers) {
	std::vector<StyledText> segments;

	try {
		std::stack<Style> styles;
		styles.push(baseStyle);

		const StringType& text = options & TRIM_WHITESPACE ? boost::trim_copy(str) : str;

		vector<StringType> tokens = splitStringIntoTokens(L"<root>" + text + L"</root>");

		for (auto& token : tokens) {
			// Lowercase tag for consistent tag checks
			const auto tag = boost::to_lower_copy(token);

			if (customTokenParsers != nullptr
				&& customTokenParsers->count(tag) > 0) {
				if (customTokenParsers->at(tag)(token, options, segments, styles) == true) {
					if (mDefaultTokenParsers.count(tag) > 0) {
						if (mDefaultTokenParsers[tag](token, options, segments, styles) == true) {
							StyledText segment(styles.top(), token);
							segments.push_back(segment);
						};
					} else {
						StyledText segment(styles.top(), token);
						segments.push_back(segment);
					}
				};
			} else	if (mDefaultTokenParsers.count(tag) > 0) {
				if (mDefaultTokenParsers[tag](token, options, segments, styles) == true) {
					StyledText segment(styles.top(), token);
					segments.push_back(segment);
				};
			} else {
				StyledText segment(styles.top(), token);
				segments.push_back(segment);
			}
		}

		if (options & TRIM_TRAILING_BREAKS) {
			while (!segments.empty()) {
				const auto& segment = segments.back();
				if (segment.mWText == L"\n") segments.pop_back();
				else break;
			}
		}

	} catch (Exception e) {
		cout << "StyledTextParser: Error: Could not parse xml StringType: " << e.what() << endl;
	}

	if (segments.empty()) {
		segments.push_back(StyledText(baseStyle, L""));
	}

	return segments;
}

std::vector<text::StringType> StyledTextParser::splitStringIntoTokens(StringType str) {
	vector<StringType> tokens;
	size_t openTagPos = 0, openTagStartSearchPos = 0;
	while ((openTagPos = str.find_first_of(L"<", openTagStartSearchPos)) != StringType::npos) {

		// Get the text that is inbetween the metatags
		if (openTagStartSearchPos > 0) {
			StringType tokenString = str.substr(openTagStartSearchPos, openTagPos - openTagStartSearchPos);
			if (tokenString.size() > 0) tokens.push_back(tokenString);
		}

		size_t closeTagPos = 0;

		if ((closeTagPos = str.find_first_of(L">", openTagPos + 1)) != StringType::npos) {

			StringType tokenString = str.substr(openTagPos, closeTagPos - openTagPos + 1);
			tokens.push_back(tokenString);
			openTagStartSearchPos = closeTagPos + 1;
		} else {
			wcout << L"StyledTextParser: Warning: Malformed style tag: " << str.substr(openTagPos, openTagPos - str.length()) << endl;
			break;
		}
	}
	return tokens;
}
}
}
