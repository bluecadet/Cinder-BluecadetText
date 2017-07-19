#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

#include "bluecadet/text/FontManager.h"
#include "bluecadet/text/StyledTextParser.h"
#include "bluecadet/text/StyledTextLayout.h"

using namespace ci;
using namespace ci::app;
using namespace std;

using namespace bluecadet::text;

class BasicSampleApp : public App {
public:
	void setup() override;
	void update() override;
	void draw() override;

protected:
	StyledTextLayoutRef mTextLayout;
	gl::TextureRef mTextTexture;
};

void BasicSampleApp::setup() {
	/*
	The font manager loads font definitions from a single json file.
	These definitions determine which font families, weights and styles
	are available.

	Instances of StyledTextLayout will use the font manager to load the
	appropriate fonts for a certain family, weight and style and the
	FontManager will determine which font is returned based on the json.
	*/
	FontManager::getInstance()->setup(getAssetPath("Fonts/fonts.json"));

	/*
	A shared text parser is used to parse simple html tags from text
	and return instances of StyledText. Here we configure the shared
	instance to invert nested italics, which means that italics within
	italics will become regular (e.g. <i>italic <i>regular</i> italic</i>).
	*/
	StyledTextParser::getInstance()->setDefaultOptions(StyledTextParser::OptionFlags::INVERT_NESTED_ITALICS);

	// Now configure your text layout
	mTextLayout = StyledTextLayoutRef(new StyledTextLayout());
	mTextLayout->setClipMode(StyledTextLayout::ClipMode::NoClip);
	mTextLayout->setLayoutMode(StyledTextLayout::LayoutMode::WordWrap);
	mTextLayout->setPadding(16.f, 8.f, 16.f, 8.f);
	mTextLayout->setFontFamily("OpenSans"); // Use a font from your fonts.json
	mTextLayout->setFontSize(48.f);
	mTextLayout->setTextColor(Color::white());
	mTextLayout->setTextAlign(TextAlign::Center);
	mTextLayout->setTextTransform(TextTransform::Capitalize);
	mTextLayout->setText("Jaded zombies acted quaintly but kept driving their oxen forward.");

	// And you should be able to render
	mTextLayout->setText("Jaded <b><i>zombies</i> acted<br/>quaintly</b> but kept driving <i>their <i>oxen</i> forward</i>.");
}

void BasicSampleApp::update() {
	auto winMousePos = getMousePos() - getWindowPos();
	mTextLayout->setMaxWidth(winMousePos.x);

	if (mTextLayout->hasChanges()) {
		auto surface = mTextLayout->renderToSurface();

		if (mTextTexture && mTextTexture->getSize() == surface.getSize()) {
			mTextTexture->update(surface);
		} else {
			mTextTexture = gl::Texture::create(surface);
		}
	}
}

void BasicSampleApp::draw() {
	gl::clear();
	gl::draw(mTextTexture);

}

CINDER_APP(BasicSampleApp,
	RendererGl(RendererGl::Options().msaa(2)),
	[&](ci::app::App::Settings *settings) {
	settings->setWindowSize(900, 640);
})
