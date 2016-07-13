#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"
#include "cinder/Utilities.h"
#include "cinder/params/Params.h"

#include "StyleManager.h"
#include "FontManager.h"
#include "StyledTextLayout.h"
#include "StyledTextParser.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class TextLayoutTestApp : public App {
public:
	void setup() override;
	void mouseDown(MouseEvent event) override;
	void mouseUp(MouseEvent event) override;
	void update() override;
	void draw() override;
	void resize() override;

	void loadText();
	void updateText();

protected:

	StyledTextLayout mTextLayout;
	ci::Surface mTextSurface;
	gl::TextureRef mTextTexture;

	TextBox mTextBox;
	ci::Surface mTextBoxSurface;
	gl::TextureRef mTextBoxTexture;

	ci::params::InterfaceGlRef mParams;

	vector<string> mParamClipModes;
	vector<string> mParamLayoutModes;
	std::string mLoremIpsum;

	bool mForceUpdates = false;
	bool mOscillate = false;
	bool mDrawTextBoxOnTop = false;
};

void TextLayoutTestApp::setup()
{
	// Add main project's assets dir
	fs::path baseProjectPath = getAssetPath("").parent_path().parent_path().parent_path();
	fs::path mainProjectAssetPath = fs::path(baseProjectPath.string() + "\\MediaWall\\assets");
	addAssetDirectory(mainProjectAssetPath);

	// Set up text parser
	StyledTextParser::getInstance()->setDefaultOptions(StyledTextParser::OptionFlags::INVERT_NESTED_ITALICS);

	// Set up style manager
	StyleManager::getInstance()->setup(getAssetPath("Fonts/styles.json"), "styles");
	/*StyleManager::getInstance()->getStyle("testTitle");
	StyleManager::getInstance()->getStyle("testSubtitle");
	StyleManager::getInstance()->getStyle("testDesc");
	StyleManager::getInstance()->getStyle("emptyStyle");
	StyleManager::getInstance()->getStyle("nothing");
	StyleManager::getInstance()->getStyle("");*/

	// Set up font manager
	FontManager::getInstance()->setLogLevel(FontManager::LogLevel::Warning);
	FontManager::getInstance()->setup(getAssetPath("Fonts/fonts.json"));
	//FontManager::getInstance()->getFont("Test", 12, FontWeight::Regular, FontStyle::Normal);
	//FontManager::getInstance()->getFont("Enzo", 12, FontWeight::Regular, FontStyle::Normal);
	//FontManager::getInstance()->getFont("Enzo", 12, FontWeight::Regular, FontStyle::Italic);
	//FontManager::getInstance()->getFont("SofiaPro", 12, 330, FontStyle::Normal, FontManager::Adaptive);			// should give 300 normal
	//FontManager::getInstance()->getFont("SofiaPro", 12, 330, FontStyle::Normal, FontManager::PrioritizeLighter);	// should give 300 normal
	//FontManager::getInstance()->getFont("SofiaPro", 12, 330, FontStyle::Normal, FontManager::PrioritizeHeavier);	// should give 400 normal
	//FontManager::getInstance()->getFont("SofiaPro", 12, 550, FontStyle::Normal, FontManager::Adaptive);			// should give 600 normal
	//FontManager::getInstance()->getFont("SofiaPro", 12, 550, FontStyle::Normal, FontManager::PrioritizeLighter);	// should give 500 normal
	//FontManager::getInstance()->getFont("SofiaPro", 12, 550, FontStyle::Normal, FontManager::PrioritizeHeavier);	// should give 600 normal
	//FontManager::getInstance()->getFont("Enzo", 12, FontWeight::Light, FontStyle::Normal);
	//FontManager::getInstance()->getFont("Enzo", 12, FontWeight::Light, FontStyle::Italic); // should give 300 italic
	//FontManager::getInstance()->getFont("Enzo", 12, 10000, FontStyle::Normal);
	//FontManager::getInstance()->getFont("Enzo", 12, 10000, FontStyle::Oblique);
	//FontManager::getInstance()->getFont("Enzo", 12, 10000, FontStyle::Italic);
	//FontManager::getInstance()->getFont("Foo", 12, FontWeight::Regular, FontStyle::Normal);

	// Test content
	loadText();
	
	// Configure text
	//mTextLayout.setBackgroundColor(ColorA(0, 0, 0, 0.5f));
	mTextLayout.setMaxWidth((float)getWindowWidth() * 0.5f);
	mTextLayout.setClipMode(StyledTextLayout::ClipMode::Clip);
	mTextLayout.setLayoutMode(StyledTextLayout::LayoutMode::WordWrap);
	//mTextLayout.setPadding(16.0f, 16.0f, 16.0f, 16.0f);
	mTextLayout.setPadding(-1.0f, 0, 0, 3.0);
	//mTextLayout.setLeadingOffset(-mRegularFont.getLeading());
	//mTextLayout.setText(mLoremIpsum, "test.body", true);
	//mTextLayout.setText("To the world, I say hello", "test.body");
	mTextLayout.setCurrentStyle("test.body");

	// Configure text box
	Style boxStyle = mTextLayout.getCurrentStyle();
	mTextBox = TextBox();
	//mTextBox
	//	//.text("To the world, I say hello")
	//	.text(mLoremIpsum)
	//	.color(boxStyle.mColor)
	//	.font(FontManager::getInstance()->getFont(boxStyle))
	//	.size(vec2(mTextLayout.getMaxWidth(), TextBox::GROW));

	updateText(); // sets text and max width

	// Configure params
	mParamClipModes.push_back("Clip");
	mParamClipModes.push_back("NoClip");

	mParamLayoutModes.push_back("WordWrap");
	mParamLayoutModes.push_back("NoWrap");
	mParamLayoutModes.push_back("SingleLine");

	mParams = ci::params::InterfaceGl::create("Settings", ivec2(400, 220), ColorA(0, 0, 0, 0.7f));
	mParams->addParam("Text", &mLoremIpsum).updateFn([&] { updateText(); });
	mParams->addButton("Reload", [&] { loadText(); updateText(); });
	mParams->addSeparator();
	mParams->addParam("Clip Mode", mParamClipModes, [&](int v) { mTextLayout.setClipMode((StyledTextLayout::ClipMode)v); mTextView->setClipMode((StyledTextLayout::ClipMode)v); }, [&] { return (int)mTextLayout.getClipMode(); });
	mParams->addParam("Layout Mode", mParamLayoutModes, [&](int v) { mTextLayout.setLayoutMode((StyledTextLayout::LayoutMode)v); mTextView->setLayoutMode((StyledTextLayout::LayoutMode)v); }, [&] { return (int)mTextLayout.getLayoutMode(); });
	mParams->addSeparator();
	mParams->addParam<float>("Max Width", [&](float v) { mTextLayout.setMaxWidth(v); mTextView->setMaxWidth(v); }, [&] { return mTextLayout.getMaxWidth(); });
	mParams->addParam<float>("Padding Top", [&](float v) { mTextLayout.setPaddingTop(v); mTextView->setPaddingTop(v); }, [&] { return mTextLayout.getPaddingTop(); });
	mParams->addParam<float>("Padding Right", [&](float v) { mTextLayout.setPaddingRight(v); mTextView->setPaddingRight(v); }, [&] { return mTextLayout.getPaddingRight(); });
	mParams->addParam<float>("Padding Bottom", [&](float v) { mTextLayout.setPaddingBottom(v); mTextView->setPaddingBottom(v); }, [&] { return mTextLayout.getPaddingBottom(); });
	mParams->addParam<float>("Padding Left", [&] (float v) { mTextLayout.setPaddingLeft(v); mTextView->setPaddingLeft(v); }, [&] { return mTextLayout.getPaddingLeft(); });
	mParams->addSeparator();
	mParams->addParam("Oscillate", &mOscillate);
	mParams->addParam("Force Updates", &mForceUpdates);
}

void TextLayoutTestApp::mouseDown(MouseEvent event)
{
	mDrawTextBoxOnTop = true;
}

void TextLayoutTestApp::mouseUp(MouseEvent event)
{
	mDrawTextBoxOnTop = false;
}

void TextLayoutTestApp::update()
{
	if (mTextLayout.hasChanges() && !mForceUpdates) {
		updateText();
	}
}

void TextLayoutTestApp::loadText() {
	//mLoremIpsum = loadString(loadAsset("lorem_ipsum.txt"));
	mLoremIpsum = "hello!";
}

void TextLayoutTestApp::updateText() {
	mTextLayout.setText(mLoremIpsum, "test.body");
	mTextSurface = mTextLayout.renderToSurface();
	mTextTexture = gl::Texture::create(mTextSurface);

	mTextBox
		.text(mLoremIpsum)
		.color(mTextLayout.getCurrentStyle().mColor)
		.font(FontManager::getInstance()->getFont(mTextLayout.getCurrentStyle()))
		.size(vec2(mTextLayout.getMaxWidth(), TextBox::GROW));
	mTextBoxSurface = mTextBox.render();
	mTextBoxTexture = gl::Texture::create(mTextBoxSurface);
}

void TextLayoutTestApp::draw()
{
	gl::clear(Color(0.3f, 0.3f, 0.3f));

	{
		gl::ScopedMatrices scopedMatrices;
		
		// update here to react to window resize immediately
		if (mForceUpdates) {
			updateText();
		}

		// oscilate view pos
		if (mOscillate) {
			gl::translate(
				20.0f * cosf((float)getElapsedSeconds()),
				20.0f * sinf(0.3f * (float)getElapsedSeconds())
			);
		}

		// draw text
		gl::color(Color::white());
		if (!mDrawTextBoxOnTop) {
			gl::draw(mTextTexture);
		}

		// draw padding
		gl::color(ColorA(1.0f, 0.0f, 0.0f, 0.75f));
		gl::drawStrokedRect(ci::Rectf(mTextLayout.getPaddingLeft(), mTextLayout.getPaddingTop(), mTextTexture->getWidth() - mTextLayout.getPaddingRight(), mTextTexture->getHeight() - mTextLayout.getPaddingBottom()));

		// draw max width
		gl::color(ColorA(0.0f, 0.0f, 1.0f, 0.75f));
		gl::drawStrokedRect(ci::Rectf(0.0f, 0.0f, mTextLayout.getMaxWidth(), (float)mTextTexture->getHeight()));
	}

	if (mDrawTextBoxOnTop) {
		gl::pushMatrices();
		// draw text box on top
		gl::color(ColorA(1.0f, 1.0f, 1.0f, 1.0f));
		gl::draw(mTextBoxTexture);
		gl::popMatrices();
	}

	//{
	//	gl::pushMatrices();
	//	// draw text box at rhs
	//	gl::translate(mTextLayout.getWidth(), 0);
	//	gl::color(Color::white());
	//	gl::draw(mTextBoxTexture);
	//	gl::popMatrices();
	//}

	//{
	//	gl::pushMatrices();
	//	// draw text box at bottom
	//	gl::translate(0, mTextLayout.getHeight());
	//	gl::color(Color::white());
	//	gl::draw(mTextBoxTexture);
	//	gl::popMatrices();
	//}

	// draw debug info
	const float infoLeft = 0.0f;
	const float infoTop = (float)getWindowHeight() - 50.0f;

	mParams->draw();

	gl::color(ColorA(0, 0, 0, 0.8f));
	gl::drawSolidRect(Rectf(0.0f, infoTop, 300.0f, (float)getWindowHeight()));

	const std::string textType = mDrawTextBoxOnTop ? "TextBox" : "StyledTextLayout";
	gl::drawString(textType, vec2(infoLeft, infoTop), ColorA(1.0f, 0.0f, 0.0f, 1.0f), FontManager::getInstance()->getFont("Arial", 32));

	gl::drawString(to_string(getAverageFps()), vec2(infoLeft, infoTop + 25.0f), ColorA(1.0f, 0.0f, 0.0f, 1.0f), FontManager::getInstance()->getFont("Arial", 32));

}

void TextLayoutTestApp::resize() {
	updateText();
	ivec2 paramsPos = getWindowSize() - mParams->getSize() - ivec2(16);
	if (paramsPos.x < 0) paramsPos.x = 0;
	if (paramsPos.y < 0) paramsPos.y = 0;
	mParams->setPosition(paramsPos);
}

CINDER_APP(TextLayoutTestApp,
	RendererGl(RendererGl::Options().msaa(2)),
	[&](ci::app::App::Settings *settings)
{
	settings->setWindowSize(900, 640);
	//settings->setConsoleWindowEnabled(true);
})
