#pragma once
#include "dom.hpp"
#include <string>
#include <memory>

struct Color {
    double r = 0.0;
    double g = 0.0;
    double b = 0.0;
    double a = 1.0;

    static Color fromHex(const std::string& hex);
    static Color fromName(const std::string& name);
    static Color parse(const std::string& str, const Color& defaultColor);
};

enum class DisplayType {
    BLOCK,
    INLINE,
    NONE
};

enum class TextAlign {
    LEFT,
    CENTER,
    RIGHT
};

struct Style {
    DisplayType display = DisplayType::INLINE;
    Color color{0.15, 0.15, 0.15, 1.0}; // Default dark gray
    Color bgColor{1.0, 1.0, 1.0, 0.0};  // Transparent
    bool hasBg = false;

    double fontSize = 15.0; // In points/pixels
    bool isBold = false;
    bool isItalic = false;
    bool isUnderline = false;
    bool isMonospace = false;

    TextAlign align = TextAlign::LEFT;

    double marginTop = 0.0;
    double marginBottom = 0.0;
    double marginLeft = 0.0;
    double marginRight = 0.0;

    double paddingTop = 0.0;
    double paddingBottom = 0.0;
    double paddingLeft = 0.0;
    double paddingRight = 0.0;

    double borderWidth = 0.0;
    Color borderColor{0.8, 0.8, 0.8, 1.0};

    bool isLink = false;
    std::string linkHref;

    static Style computeStyle(const std::shared_ptr<DOMNode>& node, const Style& parentStyle);
    static void parseInlineStyle(const std::string& styleAttr, Style& style);
};
