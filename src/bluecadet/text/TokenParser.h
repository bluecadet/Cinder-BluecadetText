#pragma once

#include <stack>

#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

#include "Text.h"

namespace bluecadet {
namespace text {

typedef std::shared_ptr<class StyledTextParser> StyledTextParserRef;

// Process token; modify token, segments, or styles if needed. Return true if additional parsers is needed for the token (for example, you modified token, but still need the token to be appended to segments).
typedef std::function<bool(StringType &token, const int options, std::vector<StyledText> &segments, std::stack<Style> &styles)> TokenParser;

typedef std::map<std::wstring, TokenParser> TokenParserMap;
typedef std::shared_ptr<TokenParserMap> TokenParserMapRef;

}
}
