#include "cinder/app/App.h"
#include "cinder/app/RendererGl.h"
#include "cinder/gl/gl.h"

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
	mTextLayout = StyledTextLayoutRef(new StyledTextLayout());
	mTextLayout->setClipMode(StyledTextLayout::ClipMode::NoClip);
	mTextLayout->setLayoutMode(StyledTextLayout::LayoutMode::WordWrap);
	mTextLayout->setPadding(16.f, 8.f, 16.f, 8.f);
	mTextLayout->setFontFamily("Arial");
	mTextLayout->setFontSize(48.f);
	mTextLayout->setTextColor(Color::white());
	mTextLayout->setTextAlign(TextAlign::Center);
	mTextLayout->setTextTransform(TextTransform::Capitalize);
	mTextLayout->setText("Jaded zombies acted quaintly but kept driving their oxen forward.");
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
