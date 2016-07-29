#include "StyledTextParser.h"
#include "cinder/Json.h"

using namespace ci;
using namespace ci::app;
using namespace std;

namespace bluecadet {
namespace text {

StyledTextParser::StyledTextParser() {
	mDefaultOptions = 0;
}

StyledTextParser::~StyledTextParser() {
}

std::vector<StyledText> StyledTextParser::parse(const StringType& str, Style baseStyle) {
	return parse(str, baseStyle, mDefaultOptions);
}

std::vector<StyledText> StyledTextParser::parse(const StringType& str, Style baseStyle, int options) {
	std::vector<StyledText> segments;

	try {
		std::stack<Style> styles;
		styles.push(baseStyle);

		const StringType& text = options & TRIM_WHITESPACE ? boost::trim_copy(str) : str;

		vector<StringType> tokens = splitStringIntoTokens(L"<root>" + text + L"</root>");

		for (auto& token : tokens) {
			if (token == L"<root>" || token == L"</root>") {
				continue;
			}

			// Lowercase tag for consistent tag checks
			const auto tag = boost::to_lower_copy(token);

			if (tag == L"<i>" || tag == L"<em>") {
				// Italic
				FontStyle targetFontStyle = FontStyle::Italic;
				const bool shouldInvert = (options & INVERT_NESTED_ITALICS) && (styles.top().mFontStyle == FontStyle::Italic);
				Style style = Style(styles.top()).fontStyle(shouldInvert ? FontStyle::Normal : FontStyle::Italic);
				styles.push(style);

			}
			else if (tag == L"<b>" || tag == L"<strong>") {
				// Bold
				Style style = Style(styles.top()).fontWeight(FontWeight::Bold);
				styles.push(style);

			}
			else if (tag == L"</i>" || tag == L"</em>" || tag == L"</b>" || tag == L"</strong>") {
				// Remove styles from stack when tag is closed
				if (!styles.empty()) styles.pop();

			}
			else {
				// Escape tags
				if (tag == L"<br/>" || tag == L"<br>") {
					if (options & STRIP_BREAK_TAGS) continue;
					token = L"\n";
				}
				else if (tag == L"<p>") {
					if (options & STRIP_PARAGRAPH_TAG) continue;
					token = L"\n";
				}
				else if (tag == L"</p>") {
					if (options & STRIP_PARAGRAPH_TAG) continue;
					token = L"\n";
				}
				else if (tag == L"&lt;") {
					token = L"<";
				}
				else if (tag == L"&gt;") {
					token = L">";
				}

				if (options & TRIM_LEADING_BREAKS && segments.empty() && token == L"\n") {
					continue;
				}

				// Add text
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

	}
	catch (Exception e) {
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
	while ((openTagPos = str.find_first_of(L"<", openTagStartSearchPos)) != StringType::npos)
	{

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
		}
		else {
			wcout << L"StyledTextParser: Warning: Malformed style tag: " << str.substr(openTagPos, openTagPos - str.length()) << endl;
			break;
		}
	}
	return tokens;
}
}
}