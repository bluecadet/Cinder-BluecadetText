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

#if defined(CINDER_MSW)
#else
#error "StyledTextLayout: Error: This class is only supported on Windows 7+"
#endif

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
// TextManager Helper
//
class TextManager : private ci::Noncopyable {
public:
	TextManager() :
		mDummyDC(::CreateCompatibleDC(0)),
		mGraphics(mDummyDC)
	{
	}
	~TextManager() {
		::DeleteDC(mDummyDC);
	}
	static TextManager*			instance() {
		static TextManager* instance = nullptr;
		if (!instance) instance = new TextManager();
		return instance;
	}
	const HDC&					getDc() { return mDummyDC; }
	const Gdiplus::Graphics&	getGraphics() { return mGraphics; }

private:
	HDC					mDummyDC;
	Gdiplus::Graphics	mGraphics;
};

//==================================================
// Run Helper
//

// We have to use a shared pointer (or pointers that is) to support non-copyable Gdiplus::StringFormat
typedef shared_ptr<class Run> RunRef;

class Run {
public:
	Run(const ci::Font& aFont, const ci::ColorA& aColor) :
		mFont(aFont),
		mColor(aColor),
		mHasInvalidExtents(true),
		mFormat(Gdiplus::StringFormat::GenericTypographic())	// trims and measures string correctly
		//mFormat(Gdiplus::StringFormat::GenericDefault())		// adds 1/6em left/right padding
	{
		mFormat.SetAlignment(Gdiplus::StringAlignmentNear);
		mFormat.SetLineAlignment(Gdiplus::StringAlignmentNear);

		int flags = 0;
		flags |= Gdiplus::StringFormatFlagsMeasureTrailingSpaces;	// Important when calculating layout of multiple runs
		flags |= Gdiplus::StringFormatFlagsNoClip;					// Don't clip words
		flags |= Gdiplus::StringFormatFlagsNoFitBlackBox;			// Don't try to compress and fit (only applies when passing a rect)
		mFormat.SetFormatFlags(flags);
	}
	~Run() {};

	const ci::vec2&					getSize() { calcExtents(); return mSize; };
	const StringType&				getText() const { return mWideText; }
	const ci::ColorA&				getColor() const { return mColor; }
	const ci::Font&					getFont() const { return mFont; }
	const Gdiplus::StringFormat&	getFormat() const { return mFormat; }

	void append(const StringType& text) {
		mWideText.append(text);
		mHasInvalidExtents = true;
	}

	void setText(const StringType& text) {
		mWideText = text;
		mHasInvalidExtents = true;
	}

protected:
	void calcExtents() {
		if (!mHasInvalidExtents) {
			return;
		}

		// Important: explicitly enable kerning for character range
		auto range = Gdiplus::CharacterRange(0, (int)mWideText.length());
		mFormat.SetMeasurableCharacterRanges(1, &range);

		Gdiplus::RectF sizeRect;
		TextManager::instance()->getGraphics().MeasureString(mWideText.c_str(), -1, mFont.getGdiplusFont(), Gdiplus::PointF(0, 0), &mFormat, &sizeRect);
		mSize.x = sizeRect.Width;
		mSize.y = sizeRect.Height;
		mHasInvalidExtents = false;
	}

protected:
	Gdiplus::StringFormat	mFormat;
	bool					mHasInvalidExtents;
	ci::Font				mFont;
	ci::ColorA				mColor;
	StringType				mWideText;
	ci::vec2				mSize;
};

//==================================================
// Line Helper
//

typedef shared_ptr<class Line> LineRef;

class Line {
public:
	Line(TextAlign aTextAlign, float aLeadingOffset, bool aLeadingDisabled) :
		mTextAlign(aTextAlign),
		mLeadingOffset(aLeadingOffset),
		mLeadingDisabled(aLeadingDisabled),
		mSize(0, 0), mAscent(0), mDescent(0), mLeading(0),
		mHasInvalidExtents(false)
	{}
	~Line() {}

	const ci::vec2&			getSize() { calcExtents(); return mSize; }
	const vector<RunRef>&	getRuns() const { return mRuns; }
	const TextAlign		getTextAlign() const { return mTextAlign; }
	float					getLeadingOffset() const { return mLeadingOffset; };
	bool					getLeadingDisabled() const { return mLeadingDisabled; }
	float					getDescent() { calcExtents(); return mDescent; };
	float					getLeading() { calcExtents(); return mLeading; };
	float					getAscent() { calcExtents(); return mAscent; };

	void addRun(const RunRef run) {
		mRuns.push_back(run);
		mHasInvalidExtents = true;
	}

	void calcExtents() {
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
			}
			else {
				mLeading = std::max(run->getFont().getLeading(), mLeading);
			}

			mSize.y = std::max(mSize.y, run->getSize().y);
		}

		mSize.y = std::max(mSize.y, mAscent + mDescent + mLeading);
		mHasInvalidExtents = false;
	}

	void render(Gdiplus::Graphics *graphics, float currentY, float paddingLeft, float paddingRight, float maxWidth) {
		float currentX = paddingLeft;
		if (mTextAlign == TextAlign::Center) {
			currentX = (maxWidth - mSize.x) * 0.5f;
		}
		else if (mTextAlign == TextAlign::Right) {
			currentX = maxWidth - mSize.x - paddingRight;
		}

		for (auto run : mRuns) {
			const Gdiplus::Font *font = run->getFont().getGdiplusFont();
			ci::ColorA8u nativeColor(run->getColor());
			Gdiplus::SolidBrush brush(Gdiplus::Color(nativeColor.a, nativeColor.r, nativeColor.g, nativeColor.b));
			graphics->DrawString(run->getText().c_str(), -1, font, Gdiplus::PointF(currentX, currentY + (mAscent - run->getFont().getAscent())), &run->getFormat(), &brush);
			currentX += run->getSize().x;
		}
	}

protected:
	vector<RunRef>		mRuns;
	TextAlign			mTextAlign;
	ci::vec2			mSize;
	float				mLeadingOffset;
	bool				mLeadingDisabled;
	float				mDescent, mLeading, mAscent;
	bool				mHasInvalidExtents;
};



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
	mTextSize(0, 0)
{
	// force any globals we need to be initialized, particularly GDI+ on Windows
	TextManager::instance();
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


void StyledTextLayout::setText(const wstring& text) { clearText(); appendText(text); }
void StyledTextLayout::setText(const wstring& text, const string styleName) { clearText(); appendText(text, styleName); }
void StyledTextLayout::setText(const wstring& text, const Style& style) { clearText(); appendText(text, style); }

void StyledTextLayout::appendText(const wstring& text) {
	appendSegments(StyledTextParser::getInstance()->parse(text, mCurrentStyle, mParseOptions));
}
void StyledTextLayout::appendText(const wstring& text, const string& styleName) {
	Style style = StyleManager::getInstance()->getStyle(styleName);
	setCurrentStyle(style);
	appendSegments(StyledTextParser::getInstance()->parse(text, style, mParseOptions));
}
void StyledTextLayout::appendText(const wstring& text, const Style& style) {
	setCurrentStyle(style);
	appendSegments(StyledTextParser::getInstance()->parse(text, style, mParseOptions));
}

void StyledTextLayout::setPlainText(const wstring& text) { clearText(); appendPlainText(text); }
void StyledTextLayout::setPlainText(const wstring& text, const string styleName) { clearText(); appendPlainText(text, styleName); }
void StyledTextLayout::setPlainText(const wstring& text, const Style& style) { clearText(); appendPlainText(text, style); }

void StyledTextLayout::appendPlainText(const wstring& text) {
	appendSegment(StyledText(mCurrentStyle, text));
}
void StyledTextLayout::appendPlainText(const wstring& text, const string& styleName) {
	Style style = StyleManager::getInstance()->getStyle(styleName);
	setCurrentStyle(style);
	appendSegment(StyledText(style, text));
}
void StyledTextLayout::appendPlainText(const wstring& text, const Style& style) {
	setCurrentStyle(style);
	appendSegment(StyledText(style, text));
}


// std::string helpers to convert to widestring
void StyledTextLayout::setText(const string& text) { setText(wideString(text)); }
void StyledTextLayout::setText(const string& text, const string styleName) { setText(wideString(text), styleName); }
void StyledTextLayout::setText(const string& text, const Style& style) { setText(wideString(text), style); }

void StyledTextLayout::appendText(const string& text) { appendText(wideString(text)); }
void StyledTextLayout::appendText(const string& text, const string& styleName) { appendText(wideString(text), styleName); }
void StyledTextLayout::appendText(const string& text, const Style& style) { appendText(wideString(text), style); }

void StyledTextLayout::setPlainText(const string& text) { setPlainText(wideString(text)); }
void StyledTextLayout::setPlainText(const string& text, const string styleName) { setPlainText(wideString(text), styleName); }
void StyledTextLayout::setPlainText(const string& text, const Style& style) { setPlainText(wideString(text), style); }

void StyledTextLayout::appendPlainText(const string& text) { appendPlainText(wideString(text)); }
void StyledTextLayout::appendPlainText(const string& text, const string& styleName) { appendPlainText(wideString(text), styleName); }
void StyledTextLayout::appendPlainText(const string& text, const Style& style) { appendPlainText(wideString(text), style); }


//==================================================
// Getter/setters
//

StyledTextLayout::LayoutMode StyledTextLayout::getLayoutMode() const { return mLayoutMode; }
void StyledTextLayout::setLayoutMode(const LayoutMode value) { mLayoutMode = value; invalidate(); }

StyledTextLayout::ClipMode StyledTextLayout::getClipMode() const { return mClipMode; }
void StyledTextLayout::setClipMode(const ClipMode value) { mClipMode = value; invalidate(); }

void StyledTextLayout::setCurrentStyle(Style style) { mCurrentStyle = style; }
void StyledTextLayout::setCurrentStyle(const std::string& styleName) { mCurrentStyle = StyleManager::getInstance()->getStyle(styleName); }
Style StyledTextLayout::getCurrentStyle() const { return mCurrentStyle; }

void StyledTextLayout::setFontFamily(const string& family, bool updateExistingText) { modifyStyles(updateExistingText, [&](Style& s) { s.mFontFamily = family; }); }
void StyledTextLayout::setFontSize(const float fontSize, bool updateExistingText) { modifyStyles(updateExistingText, [&](Style& s) { s.mFontSize = fontSize; }); }
void StyledTextLayout::setFontStyle(const FontStyle fontStyle, bool updateExistingText) { modifyStyles(updateExistingText, [&](Style& s) { s.mFontStyle = fontStyle; }); }
void StyledTextLayout::setFontWeight(const FontWeight fontWeight, bool updateExistingText) { modifyStyles(updateExistingText, [&](Style& s) { s.mFontWeight = fontWeight; }); }

void StyledTextLayout::setTextColor(const ci::Color &color, bool updateExistingText) { modifyStyles(updateExistingText, [&](Style& s) { s.mColor = color; }); }
void StyledTextLayout::setTextColor(const ci::ColorA &color, bool updateExistingText) { modifyStyles(updateExistingText, [&](Style& s) { s.mColor = color; }); }

void StyledTextLayout::setTextAlign(const TextAlign textAlign, bool updateExistingText) { modifyStyles(updateExistingText, [&](Style& s) { s.mTextAlign = textAlign; }); invalidate(); }
void StyledTextLayout::setLeadingOffset(float leadingOffset, bool updateExistingText) { modifyStyles(updateExistingText, [&](Style& s) { s.mLeadingOffset = leadingOffset; }); invalidate(); }

bool StyledTextLayout::getLeadingDisabled() const { return mLeadingDisabled; }
void StyledTextLayout::setLeadingDisabled(const bool value, bool updateExistingText) { mLeadingDisabled = value; invalidate(); }

ci::vec2 StyledTextLayout::getMaxSize() const { return mMaxSize; }
void StyledTextLayout::setMaxSize(const ci::vec2& value) { mMaxSize = value; invalidate(); }

float StyledTextLayout::getMaxWidth() const { return mMaxSize.x; }
void StyledTextLayout::setMaxWidth(const float value) { mMaxSize.x = value; invalidate(); }

float StyledTextLayout::getMaxHeight() const { return mMaxSize.y; }
void StyledTextLayout::setMaxHeight(const float value) { mMaxSize.y = value; invalidate(); }

void StyledTextLayout::setPadding(const float vertical, const float horizontal) { mPaddingTop = mPaddingBottom = vertical; mPaddingRight = mPaddingLeft = horizontal; invalidate(); }
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

void StyledTextLayout::setSegment(const StyledText& segment) { clearText(); appendSegment(segment); }
void StyledTextLayout::setSegments(const std::vector<StyledText>& segments) { clearText(); appendSegments(segments); }

void StyledTextLayout::appendSegments(const std::vector<StyledText>& segments) {
	const auto baseStyle = mCurrentStyle;
	for (auto& segment : segments) {
		appendSegment(segment);
	}
	setCurrentStyle(baseStyle); // re-apply base style
}
void StyledTextLayout::appendSegment(const StyledText& segment) {
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

	RunRef run(new Run(font, color));

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
	const auto untransformedText = segment.mWText;
	const auto wideText = text::transform(untransformedText, segment.mStyle.mTextTransform);
	const auto delimiters = text::wideString(" \n");
	const auto tokens = tokenize(wideText, delimiters);


	for (const auto& token : tokens) {
		if (token.empty()) continue;

		const CharType c = token.at(0);
		const bool isNewline = c == cNewline;
		const bool isWhitespace = c == cWhitespace || c == cTab;

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
			run = RunRef(new Run(font, color));

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

shared_ptr<Line> StyledTextLayout::addLine(const Style& style) {
	invalidate(false, true);
	shared_ptr<Line> line(new Line(style.mTextAlign, style.mLeadingOffset, mLeadingDisabled));
	mLines.push_back(line);
	return line;
}


//==================================================
// Rendering
//

bool StyledTextLayout::hasChanges() const {
	return mHasInvalidLayout || mHasInvalidSize;
}

ci::Surface	StyledTextLayout::renderToSurface(bool useAlpha, bool premultiplied)
{
	validateLayout();
	validateSize();

	ci::Surface result;
	ci::ivec2 bitmapSize = getTextSize();

	// Odd failure - return a NULL Surface
	if (bitmapSize.x < 0 || bitmapSize.y < 0) {
		return result;
	}

	// I don't have a great explanation for this other than it seems to be necessary
	bitmapSize.y += 1;

	// prep our GDI and GDI+ resources
	result = ci::Surface8u(bitmapSize.x, bitmapSize.y, useAlpha, ci::SurfaceConstraintsGdiPlus());
	result.setPremultiplied(premultiplied);

	Gdiplus::Bitmap *offscreenBitmap = ci::msw::createGdiplusBitmap(result);
	Gdiplus::Graphics *offscreenGraphics = Gdiplus::Graphics::FromImage(offscreenBitmap);
	offscreenGraphics->SetTextRenderingHint(Gdiplus::TextRenderingHint::TextRenderingHintAntiAlias);
	offscreenGraphics->Clear(Gdiplus::Color::Transparent);

	// walk the lines and getSurface them, advancing our Y offset along the way
	float currentY = mPaddingTop;
	for (auto& line : mLines) {
		currentY += line->getLeadingOffset() + line->getLeading();
		line->render(offscreenGraphics, currentY, mPaddingLeft, mPaddingRight, (float)bitmapSize.x);
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
