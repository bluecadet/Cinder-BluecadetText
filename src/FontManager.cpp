#include "FontManager.h"
#include "cinder/Json.h"

using namespace ci;
using namespace ci::app;
using namespace std;

namespace bluecadet {
namespace text {

FontManager::FontManager() :
	mDefaultName("Arial"),
	mDefaultStyle(FontStyle::Normal),
	mDefaultWeight(FontWeight::Regular),
	mLogLevel(LogLevel::Error)
{
}

FontManager::~FontManager() {
}

void FontManager::setup(ci::fs::path jsonPath) {
	if (mLogLevel >= LogLevel::Info) cout << "FontManager: Loading json from '" << jsonPath << "'" << endl;

	if (jsonPath.empty()) {
		if (mLogLevel >= LogLevel::Error) cout << "FontManager: Error: Json path is empty or invalid" << endl;
		return;
	}

	DataSourceRef jsonData = loadFile(jsonPath);

	if (!jsonData) {
		if (mLogLevel >= LogLevel::Error) cout << "FontManager: Error: Can't load json at '" << jsonPath << "'" << endl;
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
		if (mLogLevel >= LogLevel::Error) cout << "FontManager: Error: Could not parse JSON: " << e.what() << endl;
	}
}

ci::Font& FontManager::getFont(Style style, FallbackMode fallbackMode) {
	return getFont(style.mFontFamily, style.mFontSize, style.mFontWeight, style.mFontStyle, fallbackMode);
}

ci::Font& FontManager::getFont(std::string family, float size, int weight, FontStyle style, FallbackMode fallbackMode) {
	auto weightsIt = mWeightsByFamily.find(family);

	if (weightsIt == mWeightsByFamily.end()) {
		// Check if we have the font family as a system font
		auto& systemFontNames = Font::getNames();
		auto fontIt = std::find(systemFontNames.begin(), systemFontNames.end(), family);
		if (fontIt == systemFontNames.end()) {
			if (family == mDefaultName) {
				if (mLogLevel >= LogLevel::Warning) cout << "FontManager: Warning: Can't find font with family '" << family << "'" << endl;
			}
			else {
				if (mLogLevel >= LogLevel::Warning) cout << "FontManager: Warning: Can't find font with family '" << family << "'; Returning default font '" << mDefaultName << "'" << endl;
			}
			family = mDefaultName;
		}
		return getCachedFontByName(family, size);
	}

	string path = getFontPath(weightsIt->second, weight, style, fallbackMode);

	//cout << "FontManager: Path for family '" << family << "', weight '" << to_string(weight) << "' and style '" << getStringFromStyle(style) << "': '" << path << "'" << endl;

	if (path.empty()) {
		if (mLogLevel >= LogLevel::Warning) cout << "FontManager: Warning: Can't find font weight '" << to_string(weight) << "' and style '" << getStringFromFontStyle(style) << "' for family '" << family << "'; Returning default font '" << mDefaultName << "'" << endl;
		return getCachedFontByName(mDefaultName, size);
	}

	return getCachedFontByPath(path, size);
}

std::string FontManager::getFontPath(StylesByWeight& weights, int targetWeight, FontStyle targetStyle, FallbackMode fallbackMode) {
	if (weights.empty()) {
		if (mLogLevel >= LogLevel::Warning) cout << "FontManager: Warning: No weights found for font" << endl;
		return "";
	}

	auto stylesIt = weights.find(targetWeight);

	if (stylesIt == weights.end() || stylesIt->second.count(targetStyle) == 0) {

		// Try the current style in a different weight
		stylesIt = getFallbackWeight(weights, targetWeight, targetStyle, fallbackMode);

		if (stylesIt != weights.end()) {
			// Found a fallback weight
			if (mLogLevel >= LogLevel::Warning) cout << "FontManager: Warning: Can't find font weight '" << to_string(targetWeight) << "' for style '" << getStringFromFontStyle(targetStyle) << "'; Falling back to '" << to_string(stylesIt->first) << "'" << endl;

		}
		else if (targetStyle != mDefaultStyle) {
			// Default to normal style if we couldn't find a weight with the target style
			if (mLogLevel >= LogLevel::Warning) cout << "FontManager: Warning: Can't find style '" << getStringFromFontStyle(targetStyle) << "'; Defaulting to '" << getStringFromFontStyle(mDefaultStyle) << "'" << endl;
			return getFontPath(weights, targetWeight, mDefaultStyle, fallbackMode);
		}
	}

	if (stylesIt == weights.end()) {
		return "";
	}

	auto pathIt = stylesIt->second.find(targetStyle);

	if (pathIt == stylesIt->second.end()) {
		if (mLogLevel >= LogLevel::Error) cout << "FontManager: Error: Could not find an alternate weight to '" << to_string(targetWeight) << "' with style '" << getStringFromFontStyle(targetStyle) << "'" << endl;
		return "";
	}

	return pathIt->second;
}

ci::Font& FontManager::getCachedFontByPath(std::string path, float size) {
	auto& sizeMap = getCachedSizesForFont(path);
	auto fontIt = sizeMap.find(size);

	if (fontIt == sizeMap.end()) {
		auto dataSource = loadFile(path);

		if (!dataSource) {
			if (mLogLevel >= LogLevel::Error) cout << "FontManager: Error: Can't load font at '" << path << "'; Returning default font '" << mDefaultName << "'" << endl;
			return getCachedFontByName(mDefaultName, size);
		}

		sizeMap[size] = ci::Font(dataSource, size);
		fontIt = sizeMap.find(size);
	}

	return fontIt->second;
}

ci::Font& FontManager::getCachedFontByName(std::string name, float size) {
	auto& sizeMap = getCachedSizesForFont(name);
	auto fontIt = sizeMap.find(size);

	if (fontIt == sizeMap.end()) {
		sizeMap[size] = ci::Font(mDefaultName, size);
		fontIt = sizeMap.find(size);
	}

	return fontIt->second;
}

std::map<float, ci::Font>& FontManager::getCachedSizesForFont(std::string key) {
	auto sizeMapIt = mCachedFonts.find(key);
	if (sizeMapIt == mCachedFonts.end()) {
		// create new map if it doesn't exist yet
		mCachedFonts[key] = std::map<float, ci::Font>();
		sizeMapIt = mCachedFonts.find(key);
	}
	return sizeMapIt->second;
}

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