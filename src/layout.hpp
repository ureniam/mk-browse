#pragma once
#include "dom.hpp"
#include "style.hpp"
#include <string>
#include <vector>
#include <memory>
#include <pango/pangocairo.h>

enum class BoxType {
    CONTAINER,
    TEXT_RUN,
    LINE_BREAK,
    HR,
    IMAGE
};

class LayoutBox {
public:
    BoxType type = BoxType::CONTAINER;
    double x = 0.0;
    double y = 0.0;
    double width = 0.0;
    double height = 0.0;

    Style style;
    std::string text;
    std::string linkHref;
    std::string imageSrc;
    std::shared_ptr<DOMNode> node;

    std::vector<std::shared_ptr<LayoutBox>> children;

    LayoutBox(BoxType t = BoxType::CONTAINER) : type(t) {}

    void addChild(std::shared_ptr<LayoutBox> child) {
        children.push_back(child);
    }

    // Hit-test for clicking hyperlinks
    std::string findLinkAt(double px, double py) const {
        if (!linkHref.empty() && 
            px >= x && px <= x + width && 
            py >= y && py <= y + height) {
            return linkHref;
        }
        for (const auto& child : children) {
            std::string found = child->findLinkAt(px, py);
            if (!found.empty()) return found;
        }
        return "";
    }
};

class LayoutEngine {
public:
    static std::shared_ptr<LayoutBox> layout(
        const std::shared_ptr<DOMNode>& root, 
        double viewportWidth, 
        cairo_t* cr, 
        double& outTotalHeight
    );

private:
    static void layoutNode(
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
    );
};
