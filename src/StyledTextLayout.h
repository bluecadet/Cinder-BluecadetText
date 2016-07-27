/*
 Copyright (c) 2010, The Barbarian Group
 All rights reserved.

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

#pragma once

#include "cinder/Cinder.h"
#include "cinder/Surface.h"

#include <vector>
#include <string>

#include "Text.h"

//! Core Text forward declarations
#if defined( CINDER_COCOA )
struct __CTFrame;
struct __CTLine;
#endif

namespace bluecadet {
namespace text {

typedef std::shared_ptr<class StyledTextLayout> StyledTextLayoutRef;

class StyledTextLayout {
public:

	enum LayoutMode {
		WordWrap,	//! Default: Wraps automatically at max width. Will keep words that are longer than max wdith on a single line, but not break them.
		NoWrap,		//! Does not wrap automatically but respects explicit line breaks.
		SingleLine,	//! Forces all text onto a single line and discards any explicit line breaks.
		StripBreaks	//! Strips forced line breaks from text but still wraps automatically
	};

	enum ClipMode {
		Clip,	//! Default: Clips at max width if max width is greater than or equal to 0
		NoClip	//! Ignores max width and renders all text regardless of max width. Can cause wider surfaces than expected even with WordWrap enabled (e.g. words that are wider than max width).
	};


	/*!
	 Makes a StyledTextLayout Object.
	 */
	StyledTextLayout();
	virtual ~StyledTextLayout();

	static StyledTextLayoutRef create(Style style = Style());



	//! Removes all text
	void clearText();

	//! Returns a ci::Surface into which the StyledTextLayout is rendered. If \a useAlpha the ci::Surface will contain an alpha channel. If \a premultiplied the alpha will be premulitplied.
	ci::Surface renderToSurface(bool useAlpha = true, bool premultiplied = false);

	//! Returns true if the current size or layouts are invalid and require a call to getSurface()
	bool hasChanges() const;



	//! Replaces the current text and keeps the current style. Parses supported style tags.
	void setText(const std::string& text);
	//! Replaces the current text and and sets the current style by loading it from the StyleManager. Parses supported style tags.
	void setText(const std::string& text, const std::string styleName);
	//! Replaces the current text and and sets the current style. Parses supported style tags.
	void setText(const std::string& text, const Style& style);

	//! Appends text to any existing text. More efficient than resetting all text if you just want to add to existing text. Parses supported style tags.
	void appendText(const std::string& text);
	//! Appends text to any existing text and and sets the current style by loading it from the StyleManager. Parses supported style tags.
	void appendText(const std::string& text, const std::string& styleName);
	//! Appends text to any existing text and and sets the current style. Parses supported style tags.
	void appendText(const std::string& text, const Style& style);

	//! Replaces the current text with plain text and keeps the current style. Text will not be parsed for style tags, making this method slightly more efficient than its text counterpart.
	void setPlainText(const std::string& text);
	//! Replaces the current text with plain text. Text will not be parsed for style tags, making this method slightly more efficient than its text counterpart.
	void setPlainText(const std::string& text, const std::string styleName);
	//! Replaces the current text with plain text. Text will not be parsed for style tags, making this method slightly more efficient than its text counterpart.
	void setPlainText(const std::string& text, const Style& style);

	//! Appends text to any existing text. More efficient than resetting all text if you just want to add to existing text. Text will not be parsed for style tags, making this method slightly more efficient than its text counterpart.
	void appendPlainText(const std::string& text);
	//! Appends text to any existing text and and sets the current style by loading it from the StyleManager. Text will not be parsed for style tags, making this method slightly more efficient than its text counterpart.
	void appendPlainText(const std::string& text, const std::string& styleName);
	//! Appends text to any existing text and and sets the current style. Text will not be parsed for style tags, making this method slightly more efficient than its text counterpart.
	void appendPlainText(const std::string& text, const Style& style);


	//! Replaces the current text and keeps the current style. Parses supported style tags.
	void setText(const std::wstring& text);
	//! Replaces the current text and and sets the current style by loading it from the StyleManager. Parses supported style tags.
	void setText(const std::wstring& text, const std::string styleName);
	//! Replaces the current text and and sets the current style. Parses supported style tags.
	void setText(const std::wstring& text, const Style& style);

	//! Appends text to any existing text. More efficient than resetting all text if you just want to add to existing text. Parses supported style tags.
	void appendText(const std::wstring& text);
	//! Appends text to any existing text and and sets the current style by loading it from the StyleManager. Parses supported style tags.
	void appendText(const std::wstring& text, const std::string& styleName);
	//! Appends text to any existing text and and sets the current style. Parses supported style tags.
	void appendText(const std::wstring& text, const Style& style);

	//! Replaces the current text with plain text. Text will not be parsed for style tags, making this method slightly more efficient than its text counterpart.
	void setPlainText(const std::wstring& text);
	//! Replaces the current text with plain text. Text will not be parsed for style tags, making this method slightly more efficient than its text counterpart.
	void setPlainText(const std::wstring& text, const std::string styleName);
	//! Replaces the current text with plain text. Text will not be parsed for style tags, making this method slightly more efficient than its text counterpart.
	void setPlainText(const std::wstring& text, const Style& style);

	//! Appends text to any existing text. More efficient than resetting all text if you just want to add to existing text. Text will not be parsed for style tags, making this method slightly more efficient than its text counterpart.
	void appendPlainText(const std::wstring& text);
	//! Appends text to any existing text and and sets the current style by loading it from the StyleManager. Text will not be parsed for style tags, making this method slightly more efficient than its text counterpart.
	void appendPlainText(const std::wstring& text, const std::string& styleName);
	//! Appends text to any existing text and and sets the current style. Text will not be parsed for style tags, making this method slightly more efficient than its text counterpart.
	void appendPlainText(const std::wstring& text, const Style& style);



	//! Defaults to \a WordWrap but only applies if a max width is set
	LayoutMode getLayoutMode() const;
	void setLayoutMode(const LayoutMode value);

	//! Defaults to \a Clip but only applies if a max width is set
	ClipMode getClipMode() const;
	void setClipMode(const ClipMode value);

	// Combines max width and height. Defaults to (-1, -1), which disables max width and height, allowing both to expand infinitely.
	virtual ci::vec2 getMaxSize() const;
	virtual void setMaxSize(const ci::vec2& size);

	//! Max width to use in combination with layout Clip and WordWrap. Defaults to -1.0. Widths smaller than 0 disable word wrapping and clipping.
	virtual float getMaxWidth() const;
	virtual void setMaxWidth(const float value);

	//! Max height to use in combination with layout Clip. If Clip is enabled and maxHeight is > 0, then text will be clipped at maxHeight.
	virtual float getMaxHeight() const;
	virtual void setMaxHeight(const float value);

	//! Returns the text size including padding according to the current clip and wrap modes. If the StyledTextLayout has changes calling this method will trigger internal recalculations.
	ci::ivec2 getTextSize();

	//! Returns the width of text including padding and according to the current clip mode. If the StyledTextLayout has changes calling this method will trigger internal recalculations.
	int getTextWidth();

	//! Returns the height of text including padding and according to the current clip mode. If the StyledTextLayout has changes calling this method will trigger internal recalculations.
	int getTextHeight();


	//! Will trim text size if it's smaller than maxSize when enabled. Default is false.
	bool getSizeTrimmingEnabled() const;
	void setSizeTrimmingEnabled(const bool value);


	//! Apples padding to text. Positive padding adds space towards the inside, negative padding towards the outside. Works similar to css box-sizing: border-box.
	void setPadding(const float top, const float right, const float bottom, const float left);
	void setPadding(const float vertical, const float horizontal);
	void setPaddingTop(const float padding);
	void setPaddingRight(const float padding);
	void setPaddingBottom(const float padding);
	void setPaddingLeft(const float padding);

	float getPaddingTop() const;
	float getPaddingRight() const;
	float getPaddingBottom() const;
	float getPaddingLeft() const;



	//! Sets the style for any future text where style is not explicitly set
	void setCurrentStyle(Style style);
	void setCurrentStyle(const std::string& styleName);
	Style getCurrentStyle() const;

	//! Sets the currently active color. Implicit opqaue alpha.
	void setTextColor(const ci::Color& color, bool updateExistingText = false);

	//! Sets the currently active color and alpha.
	void setTextColor(const ci::ColorA& color, bool updateExistingText = false);

	//! Sets an offset relative to the default leading (the vertical space between lines)  for any future text.
	void setLeadingOffset(float leadingOffset, bool updateExistingText = false);

	//! Will ignore any font-defined leading when set to true. Default is true.
	bool getLeadingDisabled() const;
	void setLeadingDisabled(const bool value, bool updateExistingText = false);

	//! Sets the justification for any future text
	void setTextAlign(const TextAlign value, bool updateExistingText = false);

	//! Sets the font family for any future text
	void setFontFamily(const std::string& fontFamily, bool updateExistingText = false);

	//! Sets the font size for any future text
	void setFontSize(const float fontSize, bool updateExistingText = false);

	//! Sets the font style for any future text
	void setFontStyle(const FontStyle fontStyle, bool updateExistingText = false);

	//! Sets the font weight for any future text
	void setFontWeight(const FontWeight fontWeight, bool updateExistingText = false);





	//! Replaces the current text and styles. Will preserve the current style before and after calling this method.
	inline void setSegment(const StyledText& segment);

	//! Replaces the current text and styles. Will preserve the current style before and after calling this method.
	inline void setSegments(const std::vector<StyledText>& segments);

	//! Appends a single segment of styled text. Will preserve the current style before and after calling this method.
	inline void appendSegment(const StyledText& segment);

	//! Appends segments of styled text. Will preserve the current style before and after calling this method.
	inline void appendSegments(const std::vector<StyledText>& segments);

	//! Returns all of the current segments of text
	inline const std::vector<StyledText>& getSegments() const;



protected:
	//! Marks the current size and layout as invalid. Call this method when making any style or content changes to queue a validation when necessary.
	virtual inline void invalidate(const bool layout = true, const bool size = true);

	//! Recalculates the current layout by clearing all content and re-adding it if the current layout is invalid.
	inline void	validateLayout();

	//! Recalculates the current size if the size is currently invalid.
	inline void	validateSize();

	//! Helper to modify all styles of existing segments and the current style
	void		modifyStyles(bool updateExistingText, std::function<void(Style& style)> fn);

	//! Adds a single, empty line with the current style and returns it.
	std::shared_ptr<class Line>	addLine(const Style& style);



	// Layout properties
	bool		mHasInvalidLayout;
	bool		mHasInvalidSize;
	ci::ivec2	mTextSize;

	std::vector<StyledText> mSegments;
	std::vector<std::shared_ptr<class Line>> mLines;

	LayoutMode	mLayoutMode;
	ClipMode	mClipMode;
	bool		mSizeTrimmingEnabled;

	// Styling properties
	bool		mLeadingDisabled;
	int			mParseOptions;

	ci::vec2	mMaxSize;
	float		mPaddingTop;
	float		mPaddingRight;
	float		mPaddingBottom;
	float		mPaddingLeft;
	Style		mCurrentStyle;
};

}
}
