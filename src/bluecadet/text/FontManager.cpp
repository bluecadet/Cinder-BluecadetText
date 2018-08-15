#include "FontManager.h"
#include "cinder/Json.h"
#include "cinder/Log.h"

using namespace ci;
using namespace ci::app;
using namespace std;

namespace bluecadet {
namespace text {

FontManager::FontManager() :
	mDefaultName("Arial"),
	mDefaultStyle(FontStyle::Normal),
	mDefaultWeight(FontWeight::Regular),
	mLogLevel(LogLevel::Warning)
{
}

FontManager::~FontManager() {
}

void FontManager::setup(ci::fs::path jsonPath) {
	if (mLogLevel >= LogLevel::Info) CI_LOG_D("FontManager: Loading json from '" << jsonPath << "'");

	if (jsonPath.empty()) {
		if (mLogLevel >= LogLevel::Error) CI_LOG_E("FontManager: Error: Json path is empty or invalid");
		return;
	}

	DataSourceRef jsonData = loadFile(jsonPath);

	if (!jsonData) {
		if (mLogLevel >= LogLevel::Error) CI_LOG_E("FontManager: Error: Can't load json at '" << jsonPath << "'");
		return;
	}

	try {
		JsonTree json = JsonTree(jsonData);
		std::string jsonDirPath = jsonPath.remove_filename().string();

		auto& familiesJson = json.getChild("fonts").getChildren();

		for (auto& familyJson : familiesJson) {
			StylesByWeight styles;

			for (auto& weightJson : familyJson.getChildren()) {
				FilePathsByStyles filePaths;
				int weight = stoi(weightJson.getKey());

				for (auto& styleJson : weightJson.getChildren()) {
					FontStyle style = getFontStyleFromString(styleJson.getKey());
					string fileName = styleJson.getValue();
					fs::path filePath = fs::path(jsonDirPath + "/" + fileName);
					filePaths[style] = filePath.string();
				}

				styles[weight] = filePaths;
			}

			mWeightsByFamily[familyJson.getKey()] = styles;
		}

	}
	catch (Exception e) {
		if (mLogLevel >= LogLevel::Error) CI_LOG_E("FontManager: Error: Could not parse JSON: " << e.what());
	}
}

const ci::gl::SdfText::Font& FontManager::getFont(Style style, FallbackMode fallbackMode) {
	return getFont(style.mFontFamily, style.mFontSize, style.mFontWeight, style.mFontStyle, fallbackMode);
}

const ci::gl::SdfText::Font& FontManager::getFont(std::string family, float size, int weight, FontStyle style, FallbackMode fallbackMode) {
	auto weightsIt = mWeightsByFamily.find(family);

	if (weightsIt == mWeightsByFamily.end()) {
		// Check if we have the font family as a system font
		auto& systemFontNames = Font::getNames();
		auto fontIt = std::find(systemFontNames.begin(), systemFontNames.end(), family);
		if (fontIt == systemFontNames.end()) {
			if (family == mDefaultName) {
				if (mLogLevel >= LogLevel::Warning) CI_LOG_W("FontManager: Warning: Can't find font with family '" << family << "'");
			}
			else {
				if (mLogLevel >= LogLevel::Warning) CI_LOG_W("FontManager: Warning: Can't find font with family '" << family << "'; Returning default font '" << mDefaultName << "'");
			}
			family = mDefaultName;
		}
		return getCachedFontByName(family, size);
	}

	string path = getFontPath(weightsIt->second, weight, style, fallbackMode);

	//CI_LOG_D("FontManager: Path for family '" << family << "', weight '" << to_string(weight) << "' and style '" << getStringFromStyle(style) << "': '" << path << "'");

	if (path.empty()) {
		if (mLogLevel >= LogLevel::Warning) CI_LOG_W("FontManager: Warning: Can't find font weight '" << to_string(weight) << "' and style '" << getStringFromFontStyle(style) << "' for family '" << family << "'; Returning default font '" << mDefaultName << "'");
		return getCachedFontByName(mDefaultName, size);
	}

	return getCachedFontByPath(path, family, size);
}

std::string FontManager::getFontPath(StylesByWeight& weights, int targetWeight, FontStyle targetStyle, FallbackMode fallbackMode) {
	if (weights.empty()) {
		if (mLogLevel >= LogLevel::Warning) CI_LOG_W("FontManager: Warning: No weights found for font");
		return "";
	}

	auto stylesIt = weights.find(targetWeight);

	if (stylesIt == weights.end() || stylesIt->second.count(targetStyle) == 0) {

		// Try the current style in a different weight
		stylesIt = getFallbackWeight(weights, targetWeight, targetStyle, fallbackMode);

		if (stylesIt != weights.end()) {
			// Found a fallback weight
			if (mLogLevel >= LogLevel::Warning) CI_LOG_W("FontManager: Warning: Can't find font weight '" << to_string(targetWeight) << "' for style '" << getStringFromFontStyle(targetStyle) << "'; Falling back to '" << to_string(stylesIt->first) << "'");

		}
		else if (targetStyle != mDefaultStyle) {
			// Default to normal style if we couldn't find a weight with the target style
			if (mLogLevel >= LogLevel::Warning) CI_LOG_W("FontManager: Warning: Can't find style '" << getStringFromFontStyle(targetStyle) << "'; Defaulting to '" << getStringFromFontStyle(mDefaultStyle) << "'");
			return getFontPath(weights, targetWeight, mDefaultStyle, fallbackMode);
		}
	}

	if (stylesIt == weights.end()) {
		return "";
	}

	auto pathIt = stylesIt->second.find(targetStyle);

	if (pathIt == stylesIt->second.end()) {
		if (mLogLevel >= LogLevel::Error) CI_LOG_E("FontManager: Error: Could not find an alternate weight to '" << to_string(targetWeight) << "' with style '" << getStringFromFontStyle(targetStyle) << "'");
		return "";
	}

	return pathIt->second;
}

ci::gl::SdfTextRef FontManager::getText(Style style, FallbackMode fallbackMode) {
	return getText(style.mFontFamily, style.mFontSize, style.mFontWeight, style.mFontStyle, fallbackMode);
}

ci::gl::SdfTextRef FontManager::getText(std::string family, float size, int weight, FontStyle style, FallbackMode fallbackMode) {

	auto weightsIt = mWeightsByFamily.find(family);

	string path = getFontPath(weightsIt->second, weight, style, fallbackMode);

	//CI_LOG_D("FontManager: Path for family '" << family << "', weight '" << to_string(weight) << "' and style '" << getStringFromStyle(style) << "': '" << path << "'");

	if (path.empty()) {
		if (mLogLevel >= LogLevel::Warning) CI_LOG_W("FontManager: Warning: Can't find font weight '" << to_string(weight) << "' and style '" << getStringFromFontStyle(style) << "' for family '" << family << "'; Returning default font '" << mDefaultName << "'");
		return nullptr;
	}

	auto textSizesIt = mCachedTexts.find(path);

	if (textSizesIt != mCachedTexts.end()) {
		auto textIt = textSizesIt->second.find(size);
		if (textIt != textSizesIt->second.end()) {
			return textIt->second;
		}
	}

	auto & font = getCachedFontByPath(path, family, size);
	auto text = ci::gl::SdfText::create(path + "." + to_string(font.getSize()) + ".sdfcache", font);
	
	mCachedTexts[path][size] = text;
	mCachedFonts[path][size] = text->getFont(); // overwrite font to ensure it's measured correctly

	return text;
}

const ci::gl::SdfText::Font & FontManager::getCachedFontByPath(const std::string & path, const std::string & family, float size) {
	auto & sizeMap = mCachedFonts[path];
	auto fontIt = sizeMap.find(size);

	if (fontIt == sizeMap.end()) {
		CI_LOG_I("FontManager: Loading size '" << to_string(size) << "' for font '" << path << "'");

		ci::DataSourceRef dataSource = nullptr;

		// try loading font file
		try {
			dataSource = loadFile(path);

		} catch (Exception e) {
			if (mLogLevel >= LogLevel::Error) CI_LOG_EXCEPTION("FontManager: Error: Can't load font file at '" << path << "'; Returning default font '" << mDefaultName << "'", e);
			return getCachedFontByName(family, size);
		}

		// try creating font
		try {
			sizeMap[size] = ci::gl::SdfText::Font(dataSource, size);

		} catch (Exception e) {
			if (mLogLevel >= LogLevel::Error) CI_LOG_EXCEPTION("FontManager: Error: Can't create font from file at '" << path << "'; Returning default font '" << mDefaultName << "'", e);
			return getCachedFontByName(family, size);
		}

		fontIt = sizeMap.find(size);

		CI_LOG_I("font scale: " + to_string(fontIt->second.getFontScale()));

	}

	return fontIt->second;

}

const ci::gl::SdfText::Font& FontManager::getCachedFontByName(const std::string & name, float size) {
	auto & sizeMap = mCachedFonts[name];
	auto fontIt = sizeMap.find(size);

	if (fontIt == sizeMap.end()) {
		sizeMap[size] = ci::gl::SdfText::Font(name, size);
		fontIt = sizeMap.find(size);
	}

	return fontIt->second;
}

//std::map<float, ci::gl::SdfText::Font>& FontManager::getCachedSizesForFont(std::string key) {
//	auto sizeMapIt = mCachedFonts.find(key);
//	if (sizeMapIt == mCachedFonts.end()) {
//		// create new map if it doesn't exist yet
//		mCachedFonts[key] = std::map<float, ci::gl::SdfText::Font>();
//		sizeMapIt = mCachedFonts.find(key);
//	}
//	return sizeMapIt->second;
//}

FontManager::StylesByWeight::iterator FontManager::getFallbackWeight(StylesByWeight& weights, int targetWeight, FontStyle targetStyle, FallbackMode fallbackMode) {
	int nextLowest = INT_MIN;
	int nextHighest = INT_MAX;
	StylesByWeight::iterator nextLowestIt = weights.end();
	StylesByWeight::iterator nextHighestIt = weights.end();

	// Find alternate weights with the same style
	for (auto stylesIt = weights.begin(); stylesIt != weights.end(); ++stylesIt) {
		int currentWeight = stylesIt->first;
		FilePathsByStyles& paths = stylesIt->second;
		if (paths.count(targetStyle) == 0) continue; // Only check if we have the right style

		if (currentWeight < targetWeight && currentWeight > nextLowest) {
			nextLowest = currentWeight;
			nextLowestIt = stylesIt;

		}
		else if (currentWeight > targetWeight && currentWeight < nextHighest) {
			nextHighest = currentWeight;
			nextHighestIt = stylesIt;
		}
	}

	switch (fallbackMode) {
	case PrioritizeLighter: {
		if (nextLowest != INT_MIN)	return nextLowestIt;
		if (nextHighest != INT_MAX)	return nextHighestIt;
	}
	case PrioritizeHeavier: {
		if (nextHighest != INT_MAX)	return nextHighestIt;
		if (nextLowest != INT_MIN)	return nextLowestIt;
	}
	case Adaptive: {
		if (nextLowest != INT_MIN && (targetWeight <= Regular || nextHighest == INT_MAX))	return nextLowestIt;
		if (nextHighest != INT_MAX && (targetWeight > Regular || nextLowest == INT_MIN))	return nextHighestIt;
	}
	}

	return weights.end();
}

}
}