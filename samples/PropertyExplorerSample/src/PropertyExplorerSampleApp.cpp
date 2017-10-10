#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Utilities.h"
#include "cinder/params/Params.h"

#include "bluecadet/text/StyleManager.h"
#include "bluecadet/text/FontManager.h"
#include "bluecadet/text/StyledTextLayout.h"
#include "bluecadet/text/StyledTextParser.h"

using namespace ci;
using namespace ci::app;
using namespace std;
using namespace bluecadet::text;

class PropertyExplorerSampleApp : public App {
public:
	void setup() override;
	void update() override;
	void draw() override;
	void resize() override;

	void loadText();
	void updateText();

protected:

	StyledTextLayoutRef mTextLayout;
	ci::Surface mTextSurface;
	gl::TextureRef mTextTexture;
	std::string mLoremIpsum;

	// Debug properties
	ci::params::InterfaceGlRef mParams;
	vector<string> mParamClipModes;
	vector<string> mParamLayoutModes;
	bool mForceUpdates = false;
	bool mOscillate = false;
	bool mDrawTextBoxOnTop = false;
};

void PropertyExplorerSampleApp::setup()
{

	/*
	The font manager loads font definitions from a single json file.
	These definitions determine which font families, weights and styles
	are available.

	Instances of StyledTextLayout will use the font manager to load the
	appropriate fonts for a certain family, weight and style and the
	FontManager will determine which font is returned based on the json.
	*/
	FontManager::getInstance()->setLogLevel(FontManager::LogLevel::Warning);
	FontManager::getInstance()->setup(getAssetPath("fonts.json"));

	/*
	The style manager loads a single json that defines the default
	style and all other styles that inherit from the default.

	Any styles defined in that json can be used to format text in
	the app. All StyledTextLayout instances use the shared style
	manager to load styles.
	*/
	StyleManager::getInstance()->setup(getAssetPath("styles.json"), "styles");

	/*
	A shared text parser is used to parse simple html tags from text
	and return instances of StyledText. Here we configure the shared
	instance to invert nested italics, which means that italics within
	italics will become regular (e.g. <i>italic <i>regular</i> italic</i>).
	*/
	StyledTextParser::getInstance()->setDefaultOptions(StyledTextParser::OptionFlags::INVERT_NESTED_ITALICS);

	// Loads a file with some placeholder text
	loadText();

	// Configure text layout
	mTextLayout = StyledTextLayoutRef(new StyledTextLayout());
	mTextLayout->setMaxWidth((float)getWindowWidth() * 0.5f);
	mTextLayout->setClipMode(StyledTextLayout::ClipMode::Clip);
	mTextLayout->setLayoutMode(StyledTextLayout::LayoutMode::WordWrap);
	mTextLayout->setPadding(10.f, 10.f, 10.f, 10.f);

	// Sets the text of the StyledTextLayout instance and renders it to a texture
	mTextLayout->setText("This is a test title", "test.title");
	mTextLayout->appendText("<br>" + mLoremIpsum, "test.body");
	updateText();

	// Configure debug parameters
	mParamClipModes.push_back("Clip");
	mParamClipModes.push_back("NoClip");

	mParamLayoutModes.push_back("WordWrap");
	mParamLayoutModes.push_back("NoWrap");
	mParamLayoutModes.push_back("SingleLine");

	mParams = ci::params::InterfaceGl::create("Settings", ivec2(400, 400), ColorA(0, 0, 0, 0.7f));
	mParams->addParam("Text", &mLoremIpsum).updateFn([&] { updateText(); });
	mParams->addButton("Reload", [&] { loadText(); updateText(); });
	mParams->addSeparator();
	mParams->addParam("Clip Mode", mParamClipModes, [&](int v) { mTextLayout->setClipMode((StyledTextLayout::ClipMode)v); }, [&] { return (int)mTextLayout->getClipMode(); });
	mParams->addParam("Layout Mode", mParamLayoutModes, [&](int v) { mTextLayout->setLayoutMode((StyledTextLayout::LayoutMode)v); }, [&] { return (int)mTextLayout->getLayoutMode(); });
	mParams->addParam<bool>("Trim Size", [&](bool v) { mTextLayout->setSizeTrimmingEnabled(v); }, [&] { return mTextLayout->getSizeTrimmingEnabled(); });
	mParams->addSeparator();
	mParams->addParam<float>("Max Width", [&](float v) { mTextLayout->setMaxWidth(v); }, [&] { return mTextLayout->getMaxWidth(); });
	mParams->addParam<float>("Max Height", [&](float v) { mTextLayout->setMaxHeight(v); }, [&] { return mTextLayout->getMaxHeight(); });
	mParams->addParam<float>("Padding Top", [&](float v) { mTextLayout->setPaddingTop(v); }, [&] { return mTextLayout->getPaddingTop(); });
	mParams->addParam<float>("Padding Right", [&](float v) { mTextLayout->setPaddingRight(v); }, [&] { return mTextLayout->getPaddingRight(); });
	mParams->addParam<float>("Padding Bottom", [&](float v) { mTextLayout->setPaddingBottom(v); }, [&] { return mTextLayout->getPaddingBottom(); });
	mParams->addParam<float>("Padding Left", [&](float v) { mTextLayout->setPaddingLeft(v); }, [&] { return mTextLayout->getPaddingLeft(); });
	mParams->addSeparator();
	mParams->addParam<bool>("Leading Disabled", [&](bool v) { mTextLayout->setLeadingDisabled(v); }, [&] { return mTextLayout->getLeadingDisabled(); });
	mParams->addParam<float>("Leading Offset", [&](float v) { mTextLayout->setLeadingOffset(v); }, [&] { return mTextLayout->getCurrentStyle().mLeadingOffset; }).step(1);
	mParams->addSeparator();
	mParams->addParam("Oscillate", &mOscillate);
	mParams->addParam("Force Updates", &mForceUpdates);
}

void PropertyExplorerSampleApp::update() {
	if (mTextLayout->hasChanges() && !mForceUpdates) {
		updateText();
	}
}

void PropertyExplorerSampleApp::loadText() {
	mLoremIpsum = loadString(loadAsset("lorem_ipsum.txt"));
}

void PropertyExplorerSampleApp::updateText() {
	mTextSurface = mTextLayout->renderToSurface();
	mTextTexture = gl::Texture::create(mTextSurface);
}

void PropertyExplorerSampleApp::draw()
{
	gl::clear(Color(0.3f, 0.3f, 0.3f));

	{
		gl::ScopedMatrices scopedMatrices;

		// update in draw() to react to window resize immediately
		if (mForceUpdates) {
			updateText();
		}

		gl::translate(20.0f, 20.0f);

		// oscilate text position to test aliasing on subpixels
		if (mOscillate) {
			gl::translate(
				20.0f * cosf((float)getElapsedSeconds()),
				20.0f * sinf(0.3f * (float)getElapsedSeconds())
			);
		}

		// draw text
		gl::color(Color::white());
		gl::draw(mTextTexture);

		const Rectf bounds = Rectf(vec2(0.0f), mTextLayout->getTextSize());

		// draw padding outline
		gl::color(ColorA(1.0f, 0.0f, 0.0f, 0.75f));
		gl::drawStrokedRect(Rectf(bounds.x1 + mTextLayout->getPaddingLeft(), bounds.y1 + mTextLayout->getPaddingTop(), bounds.x2 - mTextLayout->getPaddingRight(), bounds.y2 - mTextLayout->getPaddingBottom()));

		// draw max width and height outline
		gl::color(ColorA(0.0f, 0.0f, 1.0f, 0.75f));
		gl::drawStrokedRect(Rectf(vec2(0.0f), mTextLayout->getTextSize()));
	}

	// draw debug info
	mParams->draw();
}

void PropertyExplorerSampleApp::resize() {
	float width = (float)getWindowWidth() * 0.5f;
	mTextLayout->setMaxWidth(width);

	updateText();
	
	ivec2 paramsPos = getWindowSize() - mParams->getSize() - ivec2(16);
	if (paramsPos.x < 0) paramsPos.x = 0;
	if (paramsPos.y < 0) paramsPos.y = 0;
	mParams->setPosition(paramsPos);
}

CINDER_APP(PropertyExplorerSampleApp,
	RendererGl(RendererGl::Options().msaa(4)),
	[&](ci::app::App::Settings *settings)
{
	settings->setWindowSize(900, 640);
})
