#include "style.hpp"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <unordered_map>

static std::string toLower(const std::string& str) {
    std::string s = str;
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    return s;
}

static std::string trim(const std::string& s) {
    auto start = s.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) return "";
    auto end = s.find_last_not_of(" \t\n\r");
    return s.substr(start, end - start + 1);
}

Color Color::fromHex(const std::string& hexStr) {
    std::string hex = hexStr;
    if (!hex.empty() && hex[0] == '#') hex = hex.substr(1);
    
    if (hex.size() == 3) {
        // Expand shorthand #RGB to #RRGGBB
        std::string expanded;
        expanded += hex[0]; expanded += hex[0];
        expanded += hex[1]; expanded += hex[1];
        expanded += hex[2]; expanded += hex[2];
        hex = expanded;
    }
    
    if (hex.size() >= 6) {
        unsigned int val = 0;
        std::stringstream ss;
        ss << std::hex << hex.substr(0, 6);
        ss >> val;
        double r = ((val >> 16) & 0xFF) / 255.0;
        double g = ((val >> 8) & 0xFF) / 255.0;
        double b = (val & 0xFF) / 255.0;
        return Color{r, g, b, 1.0};
    }
    return Color{0.0, 0.0, 0.0, 1.0};
}

Color Color::fromName(const std::string& name) {
    std::string lower = toLower(name);
    static const std::unordered_map<std::string, Color> colorMap = {
        {"black",   {0.0, 0.0, 0.0, 1.0}},
        {"white",   {1.0, 1.0, 1.0, 1.0}},
        {"red",     {0.9, 0.2, 0.2, 1.0}},
        {"green",   {0.15, 0.65, 0.2, 1.0}},
        {"blue",    {0.1, 0.45, 0.9, 1.0}},
        {"yellow",  {0.95, 0.85, 0.1, 1.0}},
        {"cyan",    {0.0, 0.7, 0.8, 1.0}},
        {"magenta", {0.8, 0.1, 0.8, 1.0}},
        {"gray",    {0.5, 0.5, 0.5, 1.0}},
        {"grey",    {0.5, 0.5, 0.5, 1.0}},
        {"lightgray", {0.85, 0.85, 0.85, 1.0}},
        {"darkgray",  {0.25, 0.25, 0.25, 1.0}},
        {"orange",  {0.95, 0.55, 0.1, 1.0}},
        {"purple",  {0.5, 0.15, 0.7, 1.0}},
        {"teal",    {0.0, 0.5, 0.5, 1.0}},
        {"navy",    {0.0, 0.0, 0.5, 1.0}}
    };
    auto it = colorMap.find(lower);
    if (it != colorMap.end()) return it->second;
    return Color{0.0, 0.0, 0.0, 1.0};
}

Color Color::parse(const std::string& str, const Color& defaultColor) {
    std::string s = trim(str);
    if (s.empty()) return defaultColor;
    if (s[0] == '#') return fromHex(s);
    if (s.rfind("rgb", 0) == 0) {
        size_t openParen = s.find('(');
        size_t closeParen = s.find(')');
        if (openParen != std::string::npos && closeParen != std::string::npos) {
            std::string inside = s.substr(openParen + 1, closeParen - openParen - 1);
            std::replace(inside.begin(), inside.end(), ',', ' ');
            std::istringstream iss(inside);
            double r = 0, g = 0, b = 0;
            if (iss >> r >> g >> b) {
                return Color{r / 255.0, g / 255.0, b / 255.0, 1.0};
            }
        }
    }
    return fromName(s);
}

Style Style::computeStyle(const std::shared_ptr<DOMNode>& node, const Style& parentStyle) {
    Style s = parentStyle;

    if (!node || node->type != NodeType::ELEMENT) {
        return s;
    }

    const std::string& tag = node->tagName;

    // Reset box model properties per element
    s.marginTop = 0;
    s.marginBottom = 0;
    s.marginLeft = 0;
    s.marginRight = 0;
    s.paddingTop = 0;
    s.paddingBottom = 0;
    s.paddingLeft = 0;
    s.paddingRight = 0;
    s.borderWidth = 0;
    s.hasBg = false;

    // Default tag-based styles
    if (tag == "h1") {
        s.display = DisplayType::BLOCK;
        s.fontSize = 28.0;
        s.isBold = true;
        s.marginTop = 16.0;
        s.marginBottom = 10.0;
    } else if (tag == "h2") {
        s.display = DisplayType::BLOCK;
        s.fontSize = 22.0;
        s.isBold = true;
        s.marginTop = 14.0;
        s.marginBottom = 8.0;
    } else if (tag == "h3") {
        s.display = DisplayType::BLOCK;
        s.fontSize = 18.0;
        s.isBold = true;
        s.marginTop = 12.0;
        s.marginBottom = 6.0;
    } else if (tag == "h4") {
        s.display = DisplayType::BLOCK;
        s.fontSize = 16.0;
        s.isBold = true;
        s.marginTop = 8.0;
        s.marginBottom = 4.0;
    } else if (tag == "p") {
        s.display = DisplayType::BLOCK;
        s.marginTop = 6.0;
        s.marginBottom = 10.0;
    } else if (tag == "div" || tag == "section" || tag == "article" || tag == "header" || tag == "footer" || tag == "nav") {
        s.display = DisplayType::BLOCK;
    } else if (tag == "a") {
        s.display = DisplayType::INLINE;
        s.color = Color{0.1, 0.45, 0.9, 1.0}; // Blue link
        s.isUnderline = true;
        s.isLink = true;
        s.linkHref = node->getAttribute("href");
    } else if (tag == "b" || tag == "strong") {
        s.isBold = true;
    } else if (tag == "i" || tag == "em") {
        s.isItalic = true;
    } else if (tag == "code") {
        s.isMonospace = true;
        s.fontSize = std::max(12.0, s.fontSize - 1.0);
        s.bgColor = Color{0.93, 0.94, 0.96, 1.0};
        s.hasBg = true;
        s.paddingLeft = 4.0;
        s.paddingRight = 4.0;
    } else if (tag == "pre") {
        s.display = DisplayType::BLOCK;
        s.isMonospace = true;
        s.fontSize = 13.0;
        s.bgColor = Color{0.95, 0.95, 0.97, 1.0};
        s.hasBg = true;
        s.paddingTop = 8.0;
        s.paddingBottom = 8.0;
        s.paddingLeft = 12.0;
        s.paddingRight = 12.0;
        s.marginTop = 8.0;
        s.marginBottom = 8.0;
        s.borderWidth = 1.0;
        s.borderColor = Color{0.8, 0.85, 0.9, 1.0};
    } else if (tag == "blockquote") {
        s.display = DisplayType::BLOCK;
        s.isItalic = true;
        s.marginLeft = 16.0;
        s.paddingLeft = 12.0;
        s.borderWidth = 3.0; // Left border
        s.borderColor = Color{0.7, 0.7, 0.7, 1.0};
        s.marginTop = 8.0;
        s.marginBottom = 8.0;
    } else if (tag == "ul" || tag == "ol") {
        s.display = DisplayType::BLOCK;
        s.marginTop = 6.0;
        s.marginBottom = 8.0;
        s.marginLeft = 20.0;
    } else if (tag == "li") {
        s.display = DisplayType::BLOCK;
        s.marginBottom = 4.0;
    } else if (tag == "hr") {
        s.display = DisplayType::BLOCK;
        s.marginTop = 12.0;
        s.marginBottom = 12.0;
    } else if (tag == "table") {
        s.display = DisplayType::BLOCK;
        s.marginTop = 10.0;
        s.marginBottom = 10.0;
        s.borderWidth = 1.0;
        s.borderColor = Color{0.7, 0.7, 0.7, 1.0};
    } else if (tag == "th") {
        s.isBold = true;
        s.bgColor = Color{0.9, 0.92, 0.95, 1.0};
        s.hasBg = true;
        s.paddingTop = 6.0;
        s.paddingBottom = 6.0;
        s.paddingLeft = 8.0;
        s.paddingRight = 8.0;
    } else if (tag == "td") {
        s.paddingTop = 4.0;
        s.paddingBottom = 4.0;
        s.paddingLeft = 8.0;
        s.paddingRight = 8.0;
    } else if (tag == "body") {
        s.display = DisplayType::BLOCK;
        s.bgColor = Color{1.0, 1.0, 1.0, 1.0};
        s.hasBg = true;
        s.paddingTop = 16.0;
        s.paddingBottom = 24.0;
        s.paddingLeft = 20.0;
        s.paddingRight = 20.0;
    } else if (tag == "head" || tag == "style" || tag == "meta" || tag == "title") {
        s.display = DisplayType::NONE;
    }

    // Parse inline style attribute if present
    if (node->hasAttribute("style")) {
        parseInlineStyle(node->getAttribute("style"), s);
    }

    return s;
}

void Style::parseInlineStyle(const std::string& styleAttr, Style& s) {
    std::istringstream stream(styleAttr);
    std::string decl;

    while (std::getline(stream, decl, ';')) {
        size_t colon = decl.find(':');
        if (colon == std::string::npos) continue;

        std::string prop = toLower(trim(decl.substr(0, colon)));
        std::string val = toLower(trim(decl.substr(colon + 1)));

        if (prop == "color") {
            s.color = Color::parse(val, s.color);
        } else if (prop == "background-color" || prop == "background") {
            s.bgColor = Color::parse(val, s.bgColor);
            s.hasBg = true;
        } else if (prop == "font-size") {
            try {
                size_t idx = 0;
                double size = std::stod(val, &idx);
                if (size > 0) s.fontSize = size;
            } catch (...) {}
        } else if (prop == "font-weight") {
            if (val == "bold" || val == "700" || val == "800" || val == "900") {
                s.isBold = true;
            } else if (val == "normal" || val == "400") {
                s.isBold = false;
            }
        } else if (prop == "font-style") {
            s.isItalic = (val == "italic");
        } else if (prop == "text-decoration") {
            s.isUnderline = (val.find("underline") != std::string::npos);
        } else if (prop == "text-align") {
            if (val == "center") s.align = TextAlign::CENTER;
            else if (val == "right") s.align = TextAlign::RIGHT;
            else s.align = TextAlign::LEFT;
        } else if (prop == "padding") {
            try {
                double p = std::stod(val);
                s.paddingTop = s.paddingBottom = s.paddingLeft = s.paddingRight = p;
            } catch (...) {}
        } else if (prop == "margin") {
            try {
                double m = std::stod(val);
                s.marginTop = s.marginBottom = s.marginLeft = s.marginRight = m;
            } catch (...) {}
        } else if (prop == "border") {
            s.borderWidth = 1.0;
            s.borderColor = Color{0.7, 0.7, 0.7, 1.0};
        } else if (prop == "display") {
            if (val == "none") s.display = DisplayType::NONE;
            else if (val == "block") s.display = DisplayType::BLOCK;
            else if (val == "inline") s.display = DisplayType::INLINE;
        }
    }
}
