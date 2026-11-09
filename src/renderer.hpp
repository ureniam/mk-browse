#pragma once
#include "layout.hpp"
#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <string>
#include <unordered_map>

class Renderer {
public:
    static void render(
        cairo_t* cr, 
        const std::shared_ptr<LayoutBox>& rootBox, 
        double scrollY, 
        double viewportWidth, 
        double viewportHeight
    );

private:
    static void renderBox(
        cairo_t* cr, 
        const std::shared_ptr<LayoutBox>& box, 
        double scrollY, 
        double viewportWidth, 
        double viewportHeight,
        PangoLayout* pangoLayout
    );
};
