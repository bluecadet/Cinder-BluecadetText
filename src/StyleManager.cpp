#include "StyleManager.h"

using namespace ci;
using namespace ci::app;
using namespace std;

namespace bluecadet {
namespace text {

StyleManager::StyleManager() {
}

StyleManager::~StyleManager() {
}

Style StyleManager::getStyle(const std::string& key) {
	auto styleIt = mStyles.find(key);
	if (styleIt == mStyles.end()) {
		console() << "StyleManager: Warning: Could not find style with key '" << key << "'" << endl;
		return mDefaultStyle;
	}
	return styleIt->second;
}

void StyleManager::setup(ci::fs::path jsonPath, const std::string basePath) {
	parseStyles(jsonPath, mDefaultStyle, basePath);
}

void StyleManager::parseStyles(ci::fs::path jsonPath, const std::string basePath) {
	parseStyles(jsonPath, mDefaultStyle, basePath);
}

void StyleManager::parseStyles(ci::fs::path jsonPath, const Style& baseStyle, const std::string basePath) {
	if (jsonPath.empty()) {
		cout << "StyleManager: Error: Json path is empty" << endl;
		return;
	}

	DataSourceRef jsonData = loadFile(jsonPath);

	if (!jsonData) {
		cout << "StyleManager: Error: Can't load json at '" << jsonPath << "'" << endl;
		return;
	}

	try {
		JsonTree json = JsonTree(jsonData);
		parseStyles(json, baseStyle, basePath);

	}
	catch (Exception e) {
		cout << "StyleManager: Error: Could not parse JSON: " << e.what() << endl;
	}
}

void StyleManager::parseStyles(const ci::JsonTree& node, const std::string basePath) {
	parseStyles(node, mDefaultStyle, basePath);
}

void StyleManager::parseStyles(const ci::JsonTree& node, const Style& baseStyle, const std::string basePath) {
	try {

		if (node.getNodeType() != JsonTree::NodeType::NODE_OBJECT) {
			return;
		}

		Style style = Style(baseStyle);

		if (node.hasChild("fontFamily"))	style.mFontFamily = node.getValueForKey<string>("fontFamily");
		if (node.hasChild("fontWeight")) 	style.mFontWeight = node.getValueForKey<int>("fontWeight");
		if (node.hasChild("fontStyle"))		style.mFontStyle = getFontStyleFromString(node.getValueForKey<string>("fontStyle"));
		if (node.hasChild("fontSize"))		style.mFontSize = node.getValueForKey<float>("fontSize");
		if (node.hasChild("color"))			style.mColor = getColorFromString(node.getValueForKey<string>("color"));
		if (node.hasChild("textAlign"))		style.mTextAlign = getTextAlignFromString(node.getValueForKey<string>("textAlign"));
		if (node.hasChild("textTransform"))	style.mTextTransform = getTextTransformFromString(node.getValueForKey<string>("textTransform"));
		if (node.hasChild("leadingOffset"))	style.mLeadingOffset = node.getValueForKey<float>("leadingOffset");

		const std::string& path = node.getPath();

		// Save all styles except for root element (root only defines base style)
		if (path != basePath && path != "" && node.hasParent()) {
			const string& styleKey = getStrippedPath(path, basePath + ".");
			mStyles[styleKey] = style;
		}

		for (auto& child : node.getChildren()) {
			parseStyles(child, style);
		}

	}
	catch (Exception e) {
		cout << "StyleManager: Error: Could not parse JSON: " << e.what() << endl;
	}

}

std::string StyleManager::getStrippedPath(const std::string& path, const std::string& basePath) {
	const size_t pathLength = path.length();
	const size_t basePathLength = basePath.length();
	if (basePathLength == 0 || pathLength == 0 || path.length() < basePathLength) {
		return path;
	}
	if (path.substr(0, basePathLength) == basePath) {
		return path.substr(basePathLength, pathLength - basePathLength);
	}
	return path;
}

}
}