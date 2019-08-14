/*
 Based on Cinder's original TextLayout. Modified and exented by Bluecadet.
*/

/*
Copyright (c) 2010, The Barbarian Group
All rights reserved.

Copyright (c) Microsoft Open Technologies, Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that
the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this list of conditions and
the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
*/

#include "StyledTextLayout.h"

#include "cinder/ip/Fill.h"
#include "cinder/ip/Premultiply.h"
#include "cinder/Utilities.h"
#include "cinder/Noncopyable.h"
#include "cinder/Font.h"
#include "cinder/Vector.h"

#if !defined(CINDER_MSW)

#warning "StyledTextLayout: Warning: This class is only supported on Windows 7+"

#else

#include <Windows.h>
#define max(a, b) (((a) > (b)) ? (a) : (b))
#define min(a, b) (((a) < (b)) ? (a) : (b))
#include <gdiplus.h>
//#include <WinGdi.h> # Use this for GDI (not GDI+)
#undef min
#undef max
#include "cinder/msw/CinderMsw.h"
#include "cinder/msw/CinderMswGdiPlus.h"
#pragma comment(lib, "gdiplus")
#include "cinder/Unicode.h"

#include <limits.h>
#include <locale>
#include <codecvt>
#include <string>

#include "FontManager.h"
#include "StyleManager.h"
#include "StyledTextParser.h"

using namespace std;

namespace bluecadet {
namespace text {


typedef std::wstring StringType;
typedef wchar_t CharType;

//==================================================
// DeviceContextManager Helper
//

class DeviceContextManager : private ci::Noncopyable {
public:
	DeviceContextManager() :
		mDummyDC(::CreateCompatibleDC(0)),
		mGraphics(mDummyDC),
		mStringFormat(Gdiplus::StringFormat::GenericTypographic())
	{
		int flags = 0;
		flags |= Gdiplus::StringFormatFlagsMeasureTrailingSpaces;	// Important when calculating layout of multiple runs
		flags |= Gdiplus::StringFormatFlagsNoClip;					// Don't clip words
		flags |= Gdiplus::StringFormatFlagsNoFitBlackBox;			// Don't try to compress and fit (only applies when passing a rect)
		mStringFormat.SetFormatFlags(flags);
		mStringFormat.SetAlignment(Gdiplus::StringAlignmentNear);
		mStringFormat.SetLineAlignment(Gdiplus::StringAlignmentNear);
	}
	~DeviceContextManager() {
		::DeleteDC(mDummyDC);
	}
	static DeviceContextManager * instance() {
		static DeviceContextManager* instance = nullptr;
		if (!instance) instance = new DeviceContextManager();
		return instance;
	}
	const HDC &						getDc() { return mDummyDC; }
	const Gdiplus::Graphics &		getGraphics() { return mGraphics; }
	Gdiplus::StringFormat &			getStringFormat() { return mStringFormat; }


private:
	HDC						mDummyDC;
	Gdiplus::Graphics		mGraphics;
	Gdiplus::StringFormat	mStringFormat;
};


//==================================================
// Run Helper
//

StyledTextLayout::Run::Run(Style style, const ci::Font & aFont, const ci::ColorA & aColor) :
	mStyle(style),
	mFont(aFont),
	mColor(aColor),
	mHasInvalidExtents(true) {
}
StyledTextLayout::Run::~Run() {};

void StyledTextLayout::Run::append(const StringType & text) {
	mWideText.append(text);
	mHasInvalidExtents = true;
}

void StyledTextLayout::Run::setText(const StringType & text) {
	mWideText = text;
	mHasInvalidExtents = true;
}

void StyledTextLayout::Run::calcExtents() {
	if (!mHasInvalidExtents) {
		return;
	}

	// Important: explicitly enable kerning for character range
	auto range = Gdiplus::CharacterRange(0, (int)mWideText.length());
	//mFormat.SetMeasurableCharacterRanges(1, &range);
	auto & format = DeviceContextManager::instance()->getStringFormat();
	format.SetMeasurableCharacterRanges(1, &range);

	Gdiplus::RectF sizeRect;
	DeviceContextManager::instance()->getGraphics().MeasureString(mWideText.c_str(), -1,
																  mFont.getGdiplusFont(), Gdiplus::PointF(0, 0), &format, &sizeRect);
	
	mSize.x = sizeRect.Width;
	mSize.y = sizeRect.Height;
	mHasInvalidExtents = false;
}

//==================================================
// Line Helper
//

StyledTextLayout::Line::Line(TextAlign aTextAlign, float aLeadingOffset, bool aLeadingDisabled) :
	mTextAlign(aTextAlign),
	mLeadingOffset(aLeadingOffset),
	mLeadingDisabled(aLeadingDisabled),
	mSize(0, 0), mAscent(0), mDescent(0), mLeading(0),
	mHasInvalidExtents(false) {
}
StyledTextLayout::Line::~Line() {}

void StyledTextLayout::Line::addRun(const StyledTextLayout::RunRef run) {
	mRuns.push_back(run);
	mHasInvalidExtents = true;
}

void StyledTextLayout::Line::calcExtents() {
	if (!mHasInvalidExtents) {
		return;
	}

	mSize.x = mSize.y = mAscent = mDescent = mLeading = 0.0f;

	for (auto run : mRuns) {
		mSize.x += run->getSize().x;
		mAscent = std::max(run->getFont().getAscent(), mAscent);
		mDescent = std::max(run->getFont().getDescent(), mDescent);

		if (mLeadingDisabled) {
			//mLeading = 1.0f; // seems necessary to correctly measure layout when using a typographic format
			mLeading = 0.0f;
		} else {
			mLeading = std::max(run->getFont().getLeading(), mLeading);
		}

		mSize.y = std::max(mSize.y, run->getSize().y);
	}

	mSize.y = std::max(mSize.y, mAscent + mDescent + mLeading);
	mHasInvalidExtents = false;
}

//==================================================
// StyledTextLayout
//

StyledTextLayout::StyledTextLayout() :
	mPaddingTop(0.0f),
	mPaddingRight(0.0f),
	mPaddingBottom(0.0f),
	mPaddingLeft(0.0f),
	mLayoutMode(WordWrap),
	mClipMode(Clip),
	mMaxSize(-1.0f, -1.0f),
	mLeadingDisabled(true),
	mHasInvalidSize(false),
	mHasInvalidLayout(false),
	mSizeTrimmingEnabled(false),
	mTextSize(0, 0) {
	// forces any globals we need to be initialized, particularly GDI+ on Windows
	DeviceContextManager::instance();

	mCurrentStyle = StyleManager::getInstance()->getDefaultStyle();
	mParseOptions = StyledTextParser::getInstance()->getDefaultOptions();
}

StyledTextLayout::~StyledTextLayout() {
}

StyledTextLayoutRef StyledTextLayout::create(Style style) {
	StyledTextLayoutRef textLayout(new StyledTextLayout());
	textLayout->setCurrentStyle(style);
	return textLayout;
}

//==================================================
// Text manipulation
//

void StyledTextLayout::clearText() {
	mSegments.clear();
	mLines.clear();
	invalidate();
}


void StyledTextLayout::setText(const wstring & text, const TokenParserMapRef customTokenParsers) { clearText(); appendText(text, customTokenParsers); }
void StyledTextLayout::setText(const wstring & text, const string styleName, const TokenParserMapRef customTokenParsers) { clearText(); appendText(text, styleName, true, customTokenParsers); }
void StyledTextLayout::setText(const wstring & text, const Style& style, const TokenParserMapRef customTokenParsers) { clearText(); appendText(text, style, true, customTokenParsers); }

void StyledTextLayout::appendText(const wstring & text, const TokenParserMapRef customTokenParsers) {
	appendSegments(StyledTextParser::getInstance()->parse(text, mCurrentStyle, mParseOptions, customTokenParsers));
}
void StyledTextLayout::appendText(const wstring & text, const string & styleName, bool saveAsCurrentStyle, const TokenParserMapRef customTokenParsers) {
	Style style = StyleManager::getInstance()->getStyle(styleName);
	if (saveAsCurrentStyle) setCurrentStyle(style);
	appendSegments(StyledTextParser::getInstance()->parse(text, style, mParseOptions, customTokenParsers));
}
void StyledTextLayout::appendText(const wstring & text, const Style& style, bool saveAsCurrentStyle, const TokenParserMapRef customTokenParsers) {
	if (saveAsCurrentStyle) setCurrentStyle(style);
	appendSegments(StyledTextParser::getInstance()->parse(text, style, mParseOptions, customTokenParsers));
}

void StyledTextLayout::setPlainText(const wstring & text) { clearText(); appendPlainText(text); }
void StyledTextLayout::setPlainText(const wstring & text, const string styleName) { clearText(); appendPlainText(text, styleName); }
void StyledTextLayout::setPlainText(const wstring & text, const Style& style) { clearText(); appendPlainText(text, style); }

void StyledTextLayout::appendPlainText(const wstring & text) {
	appendSegment(StyledText(mCurrentStyle, text));
}
void StyledTextLayout::appendPlainText(const wstring & text, const string & styleName, bool saveAsCurrentStyle) {
	Style style = StyleManager::getInstance()->getStyle(styleName);
	if (saveAsCurrentStyle) setCurrentStyle(style);
	appendSegment(StyledText(style, text));
}
void StyledTextLayout::appendPlainText(const wstring & text, const Style& style, bool saveAsCurrentStyle) {
	if (saveAsCurrentStyle) setCurrentStyle(style);
	appendSegment(StyledText(style, text));
}


// std::string helpers to convert to widestring
void StyledTextLayout::setText(const string & text, const TokenParserMapRef customTokenParsers) { setText(wideString(text), customTokenParsers); }
void StyledTextLayout::setText(const string & text, const string styleName, const TokenParserMapRef customTokenParsers) { setText(wideString(text), styleName, customTokenParsers); }
void StyledTextLayout::setText(const string & text, const Style& style, const TokenParserMapRef customTokenParsers) { setText(wideString(text), style, customTokenParsers); }

void StyledTextLayout::appendText(const string & text, const TokenParserMapRef customTokenParsers) { appendText(wideString(text), customTokenParsers); }
void StyledTextLayout::appendText(const string & text, const string & styleName, bool saveAsCurrentStyle, const TokenParserMapRef customTokenParsers) { appendText(wideString(text), styleName, saveAsCurrentStyle, customTokenParsers); }
void StyledTextLayout::appendText(const string & text, const Style& style, bool saveAsCurrentStyle, const TokenParserMapRef customTokenParsers) { appendText(wideString(text), style, saveAsCurrentStyle, customTokenParsers); }

void StyledTextLayout::setPlainText(const string & text) { setPlainText(wideString(text)); }
void StyledTextLayout::setPlainText(const string & text, const string styleName) { setPlainText(wideString(text), styleName); }
void StyledTextLayout::setPlainText(const string & text, const Style& style) { setPlainText(wideString(text), style); }

void StyledTextLayout::appendPlainText(const string & text) { appendPlainText(wideString(text)); }
void StyledTextLayout::appendPlainText(const string & text, const string & styleName, bool saveAsCurrentStyle) { appendPlainText(wideString(text), styleName, saveAsCurrentStyle); }
void StyledTextLayout::appendPlainText(const string & text, const Style& style, bool saveAsCurrentStyle) { appendPlainText(wideString(text), style, saveAsCurrentStyle); }


//==================================================
// Getter/setters
//

StyledTextLayout::LayoutMode StyledTextLayout::getLayoutMode() const { return mLayoutMode; }
void StyledTextLayout::setLayoutMode(const LayoutMode value) { mLayoutMode = value; invalidate(); }

StyledTextLayout::ClipMode StyledTextLayout::getClipMode() const { return mClipMode; }
void StyledTextLayout::setClipMode(const ClipMode value) { mClipMode = value; invalidate(); }

void StyledTextLayout::setCurrentStyle(Style style) { mCurrentStyle = style; }
void StyledTextLayout::setCurrentStyle(const std::string & styleName) { mCurrentStyle = StyleManager::getInstance()->getStyle(styleName); }
Style StyledTextLayout::getCurrentStyle() const { return mCurrentStyle; }

void StyledTextLayout::setFontFamily(const string & family, bool updateExistingText) { modifyStyles(updateExistingText, [&](Style& s) { s.mFontFamily = family; }); }
void StyledTextLayout::setFontSize(const float fontSize, bool updateExistingText) { modifyStyles(updateExistingText, [&](Style& s) { s.mFontSize = fontSize; }); }
void StyledTextLayout::setFontStyle(const FontStyle fontStyle, bool updateExistingText) { modifyStyles(updateExistingText, [&](Style& s) { s.mFontStyle = fontStyle; }); }
void StyledTextLayout::setFontWeight(const FontWeight fontWeight, bool updateExistingText) { modifyStyles(updateExistingText, [&](Style& s) { s.mFontWeight = fontWeight; }); }

void StyledTextLayout::setTextColor(const ci::Color & color, bool updateExistingText) { modifyStyles(updateExistingText, [&](Style& s) { s.mColor = color; }); }
void StyledTextLayout::setTextColor(const ci::ColorA & color, bool updateExistingText) { modifyStyles(updateExistingText, [&](Style& s) { s.mColor = color; }); }

void StyledTextLayout::setTextAlign(const TextAlign value, bool updateExistingText) { modifyStyles(updateExistingText, [&](Style& s) { s.mTextAlign = value; }); invalidate(); }

void StyledTextLayout::setTextTransform(const TextTransform value, bool updateExistingText) { modifyStyles(updateExistingText, [&](Style& s) { s.mTextTransform = value; }); invalidate(); }

void StyledTextLayout::setLeadingOffset(float leadingOffset, bool updateExistingText) { modifyStyles(updateExistingText, [&](Style& s) { s.mLeadingOffset = leadingOffset; }); invalidate(); }

bool StyledTextLayout::getLeadingDisabled() const { return mLeadingDisabled; }
void StyledTextLayout::setLeadingDisabled(const bool value, bool updateExistingText) { mLeadingDisabled = value; invalidate(); }

const ci::vec2 & StyledTextLayout::getMaxSize() const { return mMaxSize; }
void StyledTextLayout::setMaxSize(const ci::vec2 & value) { mMaxSize = value; invalidate(); }

float StyledTextLayout::getMaxWidth() const { return mMaxSize.x; }
void StyledTextLayout::setMaxWidth(const float value) { mMaxSize.x = value; invalidate(); }

float StyledTextLayout::getMaxHeight() const { return mMaxSize.y; }
void StyledTextLayout::setMaxHeight(const float value) { mMaxSize.y = value; invalidate(); }

void StyledTextLayout::setPadding(const float vertical, const float horizontal) { mPaddingTop = mPaddingBottom = vertical; mPaddingRight = mPaddingLeft = horizontal; invalidate(); }
void StyledTextLayout::setPadding(const float padding) { mPaddingTop = mPaddingRight = mPaddingBottom = mPaddingLeft = padding; invalidate(); }
void StyledTextLayout::setPadding(const float top, const float right, const float bottom, const float left) { mPaddingTop = top; mPaddingRight = right;  mPaddingBottom = bottom; mPaddingLeft = left; invalidate(); };
void StyledTextLayout::setPaddingTop(const float padding) { mPaddingTop = padding; invalidate(); };
void StyledTextLayout::setPaddingRight(const float padding) { mPaddingRight = padding; invalidate(); };
void StyledTextLayout::setPaddingBottom(const float padding) { mPaddingBottom = padding; invalidate(); };
void StyledTextLayout::setPaddingLeft(const float padding) { mPaddingLeft = padding; invalidate(); };
float StyledTextLayout::getPaddingTop() const { return mPaddingTop; };
float StyledTextLayout::getPaddingRight() const { return mPaddingRight; };
float StyledTextLayout::getPaddingBottom() const { return mPaddingBottom; };
float StyledTextLayout::getPaddingLeft() const { return mPaddingLeft; };

int StyledTextLayout::getTextWidth() { validateSize(); return mTextSize.x; }
int StyledTextLayout::getTextHeight() { validateSize(); return mTextSize.y; }
ci::ivec2 StyledTextLayout::getTextSize() { validateSize(); return mTextSize; }

bool StyledTextLayout::getSizeTrimmingEnabled() const { return mSizeTrimmingEnabled; }
void StyledTextLayout::setSizeTrimmingEnabled(const bool value) { mSizeTrimmingEnabled = value; invalidate(false, true); }


//==================================================
// Layout
//
const std::vector<StyledText>& StyledTextLayout::getSegments() const { return mSegments; }

void StyledTextLayout::setSegment(const StyledText & segment) { clearText(); appendSegment(segment); }
void StyledTextLayout::setSegments(const std::vector<StyledText> & segments) { clearText(); appendSegments(segments); }

void StyledTextLayout::appendSegments(const std::vector<StyledText> & segments) {
	const auto baseStyle = mCurrentStyle;
	for (auto& segment : segments) {
		appendSegment(segment);
	}
	setCurrentStyle(baseStyle); // re-apply base style
}
void StyledTextLayout::appendSegment(const StyledText & segment) {
	//setCurrentStyle(segment.mStyle);
	mSegments.push_back(segment);

	// Only calculate layout for segment if our current layout is valid;
	// Otherwise layout will be calculated in renderToSurface()
	if (mHasInvalidLayout && !mLines.empty()) {
		return;
	}

	if (mLines.empty()) {
		addLine(segment.mStyle);
	}

	shared_ptr<Line> line = mLines.back();

	if (line->getTextAlign() != segment.mStyle.mTextAlign) {
		// add new line if we have a new text textAlign
		line = addLine(segment.mStyle);
	}

	const ci::Font& font = FontManager::getInstance()->getFont(segment.mStyle);
	const ci::ColorA& color = segment.mStyle.mColor;


	auto run = make_shared<Run>(segment.mStyle, font, color);

	int lastNonWordIndex = -1;
	int lastWordIndex = 0;

	static const CharType cNewline = L'\n';
	static const CharType cWhitespace = L' ';
	static const CharType cTab = L'\t';

	const float maxWidth = mMaxSize.x - mPaddingLeft - mPaddingRight;
	bool shouldAutoWrap = mLayoutMode == LayoutMode::WordWrap;
	bool isWrapDisabled = mLayoutMode == LayoutMode::SingleLine;
	const bool shouldStripBreaks = mLayoutMode == LayoutMode::StripBreaks;

	if (shouldStripBreaks) {
		shouldAutoWrap = true;
		isWrapDisabled = false;
	}

	//const auto untransformedText = segment.mWText.empty() ? text::wideString(segment.mText) : segment.mWText;
	const StringType untransformedText = segment.mWText;
	const StringType wideText = text::transform(untransformedText, segment.mStyle.mTextTransform);
	const StringType delimiters = text::wideString(" \n\t");
	const auto tokens = text::tokenize(wideText, delimiters);

	for (const auto& token : tokens) {
		if (token.empty()) continue;

		const CharType c = token.at(0);
		const bool isNewline = c == cNewline;
		const bool isWhitespace = text::isSpace(c);

		// strip line breaks
		if (shouldStripBreaks && isNewline) {
			continue;
		}

		// create temp run with new word but w/o cWhitespace for measurement
		const StringType prevRunText = run->getText();
		const size_t prevRunTextLength = run->getText().length();

		// append text (but skip if it's a newline char when wrapping is disabled)
		if (!isNewline || !isWrapDisabled) {
			run->append(token);
		}

		const float lineWidth = line->getSize().x + run->getSize().x;

		// check if line is too wide, but only if the current word is not the only word on the line
		const bool isFirstWordOnLine = line->getRuns().empty() && prevRunTextLength == 0;
		const bool hasReachedMaxWidth = shouldAutoWrap && maxWidth > 0 && (lineWidth > maxWidth);
		const bool shouldBreak = hasReachedMaxWidth && !isFirstWordOnLine;

		if ((shouldBreak || isNewline) && !isWrapDisabled) {
			// save run without new word to current line
			run->setText(prevRunText);
			line->addRun(run);

			// start new line and run
			line = addLine(segment.mStyle);
			run = make_shared<Run>(segment.mStyle, font, color);

			if (!isWhitespace && !isNewline) {
				// move word to next line
				run->append(token);
			}
		}
	}

	line->addRun(run);

	invalidate(false, true); // mark size as invalid
	mHasInvalidLayout = false; // mark layout as valid
}

shared_ptr<StyledTextLayout::Line> StyledTextLayout::addLine(const Style & style) {
	invalidate(false, true);
	auto line = make_shared<Line>(style.mTextAlign, style.mLeadingOffset, mLeadingDisabled);
	mLines.push_back(line);
	return line;
}


//==================================================
// Rendering
//

bool StyledTextLayout::hasChanges() const {
	return mHasInvalidLayout || mHasInvalidSize;
}

ci::Surface	StyledTextLayout::renderToSurface(bool useAlpha, bool premultiplied, const ci::ColorA8u & clearColor) {
	validateLayout();
	validateSize();

	ci::Surface result;
	ci::ivec2 bitmapSize = ci::vec2(getTextSize());

	// Odd failure - return a NULL Surface
	if (bitmapSize.x < 0 || bitmapSize.y < 0) {
		return result;
	}

	// I don't have a great explanation for this other than it seems to be necessary
	bitmapSize.y += 1;

	// Prep our GDI and GDI+ resources
	result = ci::Surface8u(bitmapSize.x, bitmapSize.y, useAlpha, ci::SurfaceConstraintsGdiPlus());
	result.setPremultiplied(premultiplied);

	Gdiplus::Bitmap *offscreenBitmap = ci::msw::createGdiplusBitmap(result);
	Gdiplus::Graphics *offscreenGraphics = Gdiplus::Graphics::FromImage(offscreenBitmap);
	offscreenGraphics->SetTextRenderingHint(Gdiplus::TextRenderingHint::TextRenderingHintAntiAlias);
	offscreenGraphics->Clear(Gdiplus::Color(clearColor.a, clearColor.r, clearColor.g, clearColor.b));

	// Walk the lines and getSurface them, advancing our Y offset along the way
	const float maxWidth = (float)bitmapSize.x;
	const float paddingLeft = mPaddingLeft;
	const float paddingRight =  mPaddingRight;
	float currentY = mPaddingTop;

	for (const auto & line : mLines) {
		currentY += line->getLeadingOffset() + line->getLeading();

		float currentX = paddingLeft;

		if (line->getTextAlign() == TextAlign::Center) {
			currentX = (maxWidth - line->getSize().x) * 0.5f;

		} else if (line->getTextAlign() == TextAlign::Right) {
			currentX = maxWidth - line->getSize().x - mPaddingRight;
		}

		for (const auto & run : line->getRuns()) {
			const ci::ColorA8u & color = run->getColor();
			const Gdiplus::Font * font = run->getFont().getGdiplusFont();
			const Gdiplus::SolidBrush brush(Gdiplus::Color(color.a, color.r, color.g, color.b));
			const Gdiplus::PointF origin(currentX, currentY + (line->getAscent() - run->getFont().getAscent()));
			const Gdiplus::CharacterRange range(0, (int)run->getText().length());
			auto & format = DeviceContextManager::instance()->getStringFormat();
			format.SetMeasurableCharacterRanges(1, &range);
			offscreenGraphics->DrawString(run->getText().c_str(), -1, font, origin, &format, &brush);
			currentX += run->getSize().x;
		}

		currentY += line->getAscent() + line->getDescent();
	}

	GdiFlush();

	delete offscreenBitmap;
	delete offscreenGraphics;
	return result;
}


//==================================================
// Internal helpers
//
void StyledTextLayout::invalidate(const bool layout, const bool size) {
	if (layout) mHasInvalidLayout = true;
	if (size) mHasInvalidSize = true;
}

void StyledTextLayout::validateLayout() {
	if (!mHasInvalidLayout) {
		return;
	}

	// re-apply all segments using a copy of existing segments
	vector<StyledText> segmentsCopy(mSegments.begin(), mSegments.end());
	setSegments(segmentsCopy);

	mHasInvalidLayout = false;
}

void StyledTextLayout::validateSize() {
	validateLayout();

	if (!mHasInvalidSize) {
		return;
	}

	mTextSize = ci::ivec2(0, 0);

	// Make sure padding doesn't exceed max width
	if (mMaxSize.x < 0.0f || mPaddingLeft + mPaddingRight <= mMaxSize.x) {

		// Determine the extents for all the lines and the result surface
		float totalHeight = mPaddingTop + mPaddingBottom;
		float totalWidth = mPaddingLeft + mPaddingRight;

		for (auto line : mLines) {
			totalWidth = max(totalWidth, line->getSize().x + mPaddingLeft + mPaddingRight);
			totalHeight = max(totalHeight, totalHeight + line->getSize().y + line->getLeadingOffset());
		}

		if (mMaxSize.x >= 0.0f) {
			if (!mSizeTrimmingEnabled && mClipMode != NoClip) {
				totalWidth = mMaxSize.x;
			} else if (mClipMode == Clip) {
				totalWidth = min(totalWidth, mMaxSize.x);
			}
		}

		// round up from the floating point sizes to get the number of pixels we'll need
		int pixelWidth = (int)ci::math<float>::ceil(totalWidth);
		int pixelHeight = (int)ci::math<float>::ceil(totalHeight);

		// Don't allow negative sizes (e.g. with negative padding)
		if (pixelWidth < 0 || pixelHeight < 0) {
			pixelWidth = 0;
			pixelHeight = 0;
		}

		mTextSize = ci::ivec2(pixelWidth, pixelHeight);
	}

	if (mMaxSize.y >= 0 || mPaddingTop + mPaddingBottom <= mMaxSize.y) {
		if (!mSizeTrimmingEnabled && mClipMode != NoClip) {
			mTextSize.y = (int)ci::math<float>::ceil(mMaxSize.y);
		} else if (mClipMode == Clip) {
			mTextSize.y = (int)ci::math<float>::ceil(std::min((float)mTextSize.y, mMaxSize.y));
		}
	}

	mHasInvalidSize = false;
}

void StyledTextLayout::modifyStyles(bool updateExistingText, std::function<void(Style& style)> fn) {
	if (updateExistingText) {
		for (auto& segment : mSegments) {
			fn(segment.mStyle);
		}
		invalidate();
	}
	fn(mCurrentStyle);
}


}
}

#endif
