#pragma once
#include "layout.hpp"
#include "gfx_canvas.hpp"
#include <cairo.h>

class Renderer {
public:
    static void render(
        cairo_t* cr, 
        const std::shared_ptr<LayoutBox>& rootBox, 
        double scrollY, 
        double viewportWidth, 
        double viewportHeight,
        GfxCanvas::PixmapCache& cache
    );

private:
    static void renderBox(
        cairo_t* cr, 
        const std::shared_ptr<LayoutBox>& box, 
        double scrollY, 
        double viewportWidth, 
        double viewportHeight,
        PangoLayout* pangoLayout,
        GfxCanvas::PixmapCache& cache,
        int depth = 0
    );
};
