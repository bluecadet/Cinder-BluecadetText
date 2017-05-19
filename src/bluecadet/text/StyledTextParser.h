#pragma once
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

#include <stack>

#include "Text.h"

namespace bluecadet {
namespace text {

typedef std::shared_ptr<class StyledTextParser> StyledTextParserRef;

class StyledTextParser {

public:

	enum OptionFlags {
		INVERT_NESTED_ITALICS = 0x1 << 0,
		STRIP_PARAGRAPH_TAG = 0x1 << 1,
		STRIP_BREAK_TAGS = 0x1 << 2,
		TRIM_WHITESPACE = 0x1 << 3,
		TRIM_LEADING_BREAKS = 0x1 << 4,
		TRIM_TRAILING_BREAKS = 0x1 << 5
	};

	static StyledTextParserRef getInstance() {
		static StyledTextParserRef instance = nullptr;
		if (!instance) instance = StyledTextParserRef(new StyledTextParser());
		return instance;
	}

	StyledTextParser();
	~StyledTextParser();

	std::vector<StyledText> parse(const text::StringType& str, Style baseStyle);
	std::vector<StyledText> parse(const text::StringType& str, Style baseStyle, int options);

	int getDefaultOptions() const { return mDefaultOptions; }
	void setDefaultOptions(const int value) { mDefaultOptions = value; }

protected:
	std::vector<text::StringType> splitStringIntoTokens(text::StringType str);

	int mDefaultOptions;
};

}
}
