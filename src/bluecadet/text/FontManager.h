#pragma once
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

#include "Text.h"

namespace bluecadet {
namespace text {

typedef std::shared_ptr<class FontManager> FontManagerRef;

class FontManager {

public:
	enum FallbackMode {
		// Prioritize ligher weights when choosing a fallback weight
		PrioritizeLighter,
		// Prioritize heavier weights when choosing a fallback weight
		PrioritizeHeavier,
		// Prioritize lighter weights if the target weight is lighter than or equal to regular, otherwise prioritize
		// heavier weights
		Adaptive
	};

	enum LogLevel { Off = 0, Error = 1, Warning = 2, Info = 3 };

	static FontManagerRef getInstance() {
		static FontManagerRef instance = nullptr;
		if (!instance) instance = FontManagerRef(new FontManager());
		return instance;
	}

	FontManager();
	~FontManager();

	// Json containing all fonts and paths to font files.
	// Font files should be in the json directory or in one of its child directories.
	void setup(ci::fs::path jsonPath);

	ci::Font & getFont(Style style, FallbackMode fallbackMode = Adaptive);
	ci::Font & getFont(std::string family, float size, int weight = Regular, FontStyle style = Normal,
					   FallbackMode fallbackMode = Adaptive);
	ci::Font & getCachedFontByPath(std::string path, float size);
	ci::Font & getCachedFontByName(std::string name, float size);

	// Defaults to Arial
	inline const std::string & getDefaultName() const { return mDefaultName; }
	inline void setDefaultName(const std::string value) { mDefaultName = value; }

	// Defaults to Normal
	inline FontStyle getDefaultStyle() const { return mDefaultStyle; }
	inline void setDefaultStyle(const FontStyle value) { mDefaultStyle = value; }

	// Defaults to Regular
	inline int getDefaultWeight() const { return mDefaultWeight; }
	inline void setDefaultWeight(const int value) { mDefaultWeight = value; }

	// Defaults to Error
	inline LogLevel getLogLevel() const { return mLogLevel; }
	inline void setLogLevel(const LogLevel value) { mLogLevel = value; }

	// Applied to all created fonts. Defaults to 1.0.
	inline void setFontScale(float value) { mFontScale = value; }
	inline float getFontScale() const { return mFontScale; }

protected:
	// Internal types: family names -> weights -> styles -> file names
	typedef std::map<FontStyle, std::string> FilePathsByStyles;
	typedef std::map<int, FilePathsByStyles> StylesByWeight;
	typedef std::map<std::string, StylesByWeight> WeightsByFamily;

protected:
	std::string getFontPath(StylesByWeight & weights, int targetWeight, FontStyle style, FallbackMode fallbackMode);
	StylesByWeight::iterator getFallbackWeight(StylesByWeight & weights, int targetWeight, FontStyle style,
											   FallbackMode fallbackMode);

	// Get the font sizes map for a key (either font path or font name)
	std::map<float, ci::Font> & getCachedSizesForFont(std::string key);

protected:
	WeightsByFamily mWeightsByFamily;
	std::map<std::string, std::map<float, ci::Font>> mCachedFonts;

	std::string mDefaultName;
	FontStyle mDefaultStyle;
	int mDefaultWeight;
	float mFontScale;
	LogLevel mLogLevel;
};

}  // namespace text
}  // namespace bluecadet
