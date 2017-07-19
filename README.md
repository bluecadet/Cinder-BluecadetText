# Cinder-BluecadetText

This block contains a set of classes to deal with text and typography in Cinder on Windows.

Built around the need to have multi-line, auto-wrapping text with inline styles, the block grew into a framework to load and define font sets, styles and parse very simple styled text.

The core text rendering code is based on Cinder's own [`TextLayout`](https://libcinder.org/docs/classcinder_1_1_text_layout.html), but expands on layout, styling and caching functionality.

Built for and tested with [Cinder v0.9.1](https://github.com/cinder/Cinder/tree/v0.9.1) on Windows 7, 8.1 and 10. See [notes below](#notes) for setup instructions.

*Windows only! StyledTextLayout uses Windows-only GDI+ and currently has no Linux/Mac support.*

This block is compatible with and used by [Cinder-BluecadetViews](https://github.com/bluecadet/Cinder-BluecadetViews), which implements automatic rendering of text to textures in a scene-graph.

![](docs/media/class-hierarchy.png)

## Key Features

### StyledTextView

* Multi-line text layout with basic inline styling support
* HTML tags: `<b>`, `<i>`, `<br>`, `<p>`
* Configurable styles: `fontFamily`, `fontStyle`, `fontWeight`, `fontSize`, `leadingOffset`, `textColor`, `textAlign`, `textTransform`
* Automatic word-wrapping and other layout modes (single line, strip line-breaks, multi-line clip, multi-line auto-wrap)
* Full `string` and `wstring` support for all features
* Layout-caching minimizes re-calculation of layout while maintaining ability to call methods like `getSize()` at any time
* Ability to define a style from the `StyleManager`, which will be automatically applied to all text
* Multiple convenience overloads to define invidual styles and properties

### FontManager

* Load TTF font family defined on simple json
* Define multiple weights and styles per family
* Configurable auto-selection of closest font-weight (e.g. weight `500` is needed, but only `300` and `600` are available)

### StyleManager

* Loads basic styles from json
* Styles are hierarchical and can inherit properties from their parent styles

### StyledTextParser

* Parses `string` and `wstring`
* Outputs `StyledText` pairs of `string` (or `wstring`) combined with `Style`s
* Supports nested styles and (and nested/inverted italics)

## Getting Started

### Basic StyledTextLayout Usage

`StyledTextLayout` can be used to render simple text out of the box. In order to parse styled text and bold/italic tags, styles and fonts for those font-weights/font-styles have to be defined (see following sections).

```c++
#include "bluecadet/text/StyledTextLayout.h"

// ...

auto textLayout = StyledTextLayoutRef(new StyledTextLayout());
textLayout->setClipMode(StyledTextLayout::ClipMode::NoClip);
textLayout->setLayoutMode(StyledTextLayout::LayoutMode::WordWrap);
textLayout->setPadding(16.f, 8.f, 16.f, 8.f);
textLayout->setFontFamily("Arial");
textLayout->setFontSize(48.f);
textLayout->setTextColor(Color::white());
textLayout->setTextAlign(TextAlign::Center);
textLayout->setTextTransform(TextTransform::Capitalize);
textLayout->setText("Jaded zombies acted quaintly but kept driving their oxen forward.");

// ...

auto surface = textLayout->renderToSurface();
auto texture = gl::Texture::create(surface);
gl::draw(texture);
```

### Rendering Font Weights and Styles

In order to display custom font weights and styles, you need to define a `json` file that points to local font files for each weight and style. Here's an example file:

```json
{
    "fonts": {
        "OpenSans": {
            "300":{
                "normal": "OpenSans/OpenSans-Light.ttf",
                "italic": "OpenSans/OpenSans-LightItalic.ttf"
            },
            "400":{
                "normal": "OpenSans/OpenSans-Regular.ttf",
                "italic": "OpenSans/OpenSans-Italic.ttf"
            },
            "500":{
                "normal": "OpenSans/OpenSans-Semibold.ttf",
                "italic": "OpenSans/OpenSans-SemiboldItalic.ttf"
            },
            "600":{
                "normal": "OpenSans/OpenSans-Bold.ttf",
                "italic": "OpenSans/OpenSans-BoldItalic.ttf"
            },
            "700": {
                "normal": "OpenSans/OpenSans-ExtraBold.ttf",
                "italic": "OpenSans/OpenSans-ExtraBoldItalic.ttf"
            }
        }
    }
}
```

And initializing the fonts is a single line using the `FontManager`. Instances of `StyledTextLayout` will use the font manager to load the appropriate fonts for a certain family, weight and style and the `FontManager` will determine which font is returned based on the json.

```c++

#include "bluecadet/text/FontManager.h"
#include "bluecadet/text/StyledTextLayout.h"

// ...

FontManager::getInstance()->setup(getAssetPath("Fonts/fonts.json"));

// ...

auto textLayout = StyledTextLayoutRef(new StyledTextLayout());
textLayout->setFontFamily("OpenSans");
textLayout->setText("Jaded <b><i>zombies</i> acted<br/>quaintly</b> but kept driving <i>their oxen</i>.");

// ...

auto surface = textLayout->renderToSurface();
auto texture = gl::Texture::create(surface);
gl::draw(texture);
```

### Managing and Using Styles

```c++
// TODO
```

## Known Issues

* Leading and line-height calculations are currently tricky and limited by Windows' font APIs; Better line-height support is in the works
* Convenience over performance: The main purpose of this block is convenience and flexibility with layouts. While certain optimizations like layout caching for existing runs and the ability to append text have been made, this block is not built for large amounts of text.

## Future Wishlist

* Allow for nested styles within one `StyledTextView` using XML markup (e.g. `"<body>And then he said: <quote>\"That's just nice\"</quote></body>"`; could also just use HTML tags)
* Better line-height calculations and override using multiples of font-size
* Actual CSS or SASS support (instead of JSON)
* Inline style support (`<p style="color: #ff0000">not blue</p>`)

## Notes

Version 1.1.0

Based on [Cinder v0.9.1](https://github.com/cinder/Cinder/tree/v0.9.1)

Cinder setup instructions:

```bash
git clone -b v0.9.1 --depth 1 --recursive https://github.com/cinder/Cinder.git
```

Cloning with all dependencies:

```bash
cd Cinder/blocks
git clone git@github.com:bluecadet/Cinder-BluecadetText.git
git clone git@github.com:bluecadet/Cinder-BluecadetView.git
```
