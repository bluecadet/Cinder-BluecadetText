#pragma once
#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Json.h"

#include "Text.h"

namespace bluecadet {
namespace text {

typedef std::shared_ptr<class StyleManager> StyleManagerRef;

class StyleManager {

public:

	static StyleManagerRef getInstance() {
		static StyleManagerRef instance = nullptr;
		if (!instance) instance = StyleManagerRef(new StyleManager());
		return instance;
	}

	StyleManager();
	~StyleManager();


	//! Same as parseStyles, but kept here for naming consistency
	void setup(ci::fs::path jsonPath, const std::string basePath = "styles");



	//! Parses the json document at jsonPath using the default style as base. The base path will be stripped from all style keys. E.g. "my.path" will turn "my.path.myStyle" into "myStyle".
	void parseStyles(ci::fs::path jsonPath, const std::string basePath = "styles");

	//! Parses the json document at jsonPath using the default style as base. The base path will be stripped from all style keys. E.g. "my.path" will turn "my.path.myStyle" into "myStyle".
	void parseStyles(const ci::JsonTree& node, const std::string basePath = "styles");

	//! Parses the json document at jsonPath. The base path will be stripped from all style keys. E.g. "my.path" will turn "my.path.myStyle" into "myStyle".
	void parseStyles(ci::fs::path jsonPath, const Style& baseStyle, const std::string basePath = "styles");

	//! Parses a json document. The base path will be stripped from all style keys. E.g. "my.path" will turn "my.path.myStyle" into "myStyle".
	void parseStyles(const ci::JsonTree& node, const Style& baseStyle, const std::string basePath = "styles");


	//! Returns a copy of an existing style or a default style if no style with that name is found. 
	Style getStyle(const std::string& name);

	Style getDefaultStyle() const { return mDefaultStyle; }
	void setDefaultStyle(const Style value) { mDefaultStyle = value; }

protected:
	std::string getStrippedPath(const std::string& path, const std::string& basePath);
	std::map<std::string, Style> mStyles;
	Style mDefaultStyle;
};

}
}
