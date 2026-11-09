#include "layout.hpp"
#include <sstream>
#include <algorithm>
#include <cmath>

static void configurePangoFont(PangoLayout* pangoLayout, const Style& style) {
    std::string fontDescStr;
    if (style.isMonospace) {
        fontDescStr = "Monospace ";
    } else {
        fontDescStr = "Sans ";
    }

    if (style.isBold) fontDescStr += "Bold ";
    if (style.isItalic) fontDescStr += "Italic ";

    fontDescStr += std::to_string(static_cast<int>(style.fontSize)) + "px";

    PangoFontDescription* desc = pango_font_description_from_string(fontDescStr.c_str());
    pango_layout_set_font_description(pangoLayout, desc);
    pango_font_description_free(desc);
}

static void measureWord(PangoLayout* pangoLayout, const std::string& word, const Style& style, double& w, double& h) {
    configurePangoFont(pangoLayout, style);
    pango_layout_set_text(pangoLayout, word.c_str(), -1);
    pango_layout_set_width(pangoLayout, -1);

    int pw = 0, ph = 0;
    pango_layout_get_pixel_size(pangoLayout, &pw, &ph);
    w = pw;
    h = std::max(ph, static_cast<int>(style.fontSize * 1.3));
}

std::shared_ptr<LayoutBox> LayoutEngine::layout(
    const std::shared_ptr<DOMNode>& root, 
    double viewportWidth, 
    cairo_t* cr, 
    double& outTotalHeight
) {
    auto rootBox = std::make_shared<LayoutBox>(BoxType::CONTAINER);
    rootBox->x = 0;
    rootBox->y = 0;
    rootBox->width = viewportWidth;

    if (!root) {
        outTotalHeight = 0;
        return rootBox;
    }

    PangoLayout* pangoLayout = pango_cairo_create_layout(cr);

    Style defaultStyle;
    defaultStyle.display = DisplayType::BLOCK;
    defaultStyle.fontSize = 15.0;

    double currentX = 20.0;
    double currentY = 20.0;
    double availableWidth = std::max(200.0, viewportWidth - 40.0);
    double startX = 20.0;

    for (const auto& child : root->children) {
        layoutNode(child, defaultStyle, currentX, currentY, availableWidth, startX, rootBox, cr, pangoLayout, 0);
    }

    g_object_unref(pangoLayout);

    outTotalHeight = currentY + 40.0;
    rootBox->height = outTotalHeight;
    return rootBox;
}

void LayoutEngine::layoutNode(
    const std::shared_ptr<DOMNode>& node,
    const Style& parentStyle,
    double& currentX,
    double& currentY,
    double availableWidth,
    double startX,
    std::shared_ptr<LayoutBox> parentBox,
    cairo_t* cr,
    PangoLayout* pangoLayout,
    int depth
) {
    if (!node || depth > 256) return; // Prevent stack overflow on deeply nested elements

    if (node->type == NodeType::TEXT) {
        std::string raw = node->textContent;
        std::istringstream stream(raw);
        std::string word;
        
        while (stream >> word) {
            double wordW = 0, wordH = 0;
            measureWord(pangoLayout, word, parentStyle, wordW, wordH);

            double spaceW = 0, spaceH = 0;
            measureWord(pangoLayout, " ", parentStyle, spaceW, spaceH);

            if (currentX + wordW > startX + availableWidth && currentX > startX) {
                currentX = startX;
                currentY += wordH + 3.0;
            }

            auto wordBox = std::make_shared<LayoutBox>(BoxType::TEXT_RUN);
            wordBox->x = currentX;
            wordBox->y = currentY;
            wordBox->width = wordW;
            wordBox->height = wordH;
            wordBox->text = word;
            wordBox->style = parentStyle;
            wordBox->node = node;
            if (parentStyle.isLink) {
                wordBox->linkHref = parentStyle.linkHref;
            }

            parentBox->addChild(wordBox);
            currentX += wordW + spaceW;
        }
        return;
    }

    if (node->type != NodeType::ELEMENT) return;

    Style style = Style::computeStyle(node, parentStyle);
    if (style.display == DisplayType::NONE) return;

    const std::string& tag = node->tagName;

    // Handle horizontal rule <hr>
    if (tag == "hr") {
        if (currentX > startX) {
            currentX = startX;
            currentY += 20.0;
        }
        currentY += style.marginTop;
        auto hrBox = std::make_shared<LayoutBox>(BoxType::HR);
        hrBox->x = startX;
        hrBox->y = currentY;
        hrBox->width = availableWidth;
        hrBox->height = 2.0;
        hrBox->style = style;
        parentBox->addChild(hrBox);
        currentY += 2.0 + style.marginBottom;
        return;
    }

    // Handle line break <br>
    if (tag == "br") {
        currentX = startX;
        currentY += style.fontSize * 1.3;
        return;
    }

    // Handle Image <img>
    if (tag == "img") {
        std::string src = node->getAttribute("src");
        double imgW = 100.0, imgH = 60.0;
        try {
            if (node->hasAttribute("width")) imgW = std::stod(node->getAttribute("width"));
            if (node->hasAttribute("height")) imgH = std::stod(node->getAttribute("height"));
        } catch (...) {}

        if (currentX + imgW > startX + availableWidth && currentX > startX) {
            currentX = startX;
            currentY += imgH + 6.0;
        }

        auto imgBox = std::make_shared<LayoutBox>(BoxType::IMAGE);
        imgBox->x = currentX;
        imgBox->y = currentY;
        imgBox->width = imgW;
        imgBox->height = imgH;
        imgBox->imageSrc = src;
        imgBox->style = style;
        imgBox->node = node;
        if (style.isLink) imgBox->linkHref = style.linkHref;

        parentBox->addChild(imgBox);
        currentX += imgW + 10.0;
        return;
    }

    // Block container elements
    bool isBlock = (style.display == DisplayType::BLOCK);

    if (isBlock) {
        if (currentX > startX) {
            currentX = startX;
            currentY += style.fontSize * 1.3;
        }
        currentY += style.marginTop + style.paddingTop;
        startX += style.marginLeft + style.paddingLeft;
        availableWidth -= (style.marginLeft + style.marginRight + style.paddingLeft + style.paddingRight);
        currentX = startX;
    }

    // Bullet points for <li>
    if (tag == "li") {
        auto bulletBox = std::make_shared<LayoutBox>(BoxType::TEXT_RUN);
        bulletBox->x = currentX - 14.0;
        bulletBox->y = currentY;
        bulletBox->width = 10.0;
        bulletBox->height = style.fontSize * 1.3;
        bulletBox->text = "*";
        bulletBox->style = style;
        parentBox->addChild(bulletBox);
    }

    // Layout all children with incremented depth
    for (const auto& child : node->children) {
        layoutNode(child, style, currentX, currentY, availableWidth, startX, parentBox, cr, pangoLayout, depth + 1);
    }

    if (isBlock) {
        currentX = startX - (style.marginLeft + style.paddingLeft);
        currentY += style.fontSize * 1.3 + style.marginBottom + style.paddingBottom;
        startX = currentX;
        availableWidth += (style.marginLeft + style.marginRight + style.paddingLeft + style.paddingRight);
    }
}
