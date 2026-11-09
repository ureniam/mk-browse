#include "renderer.hpp"
#include <gdk/gdk.h>
#include <algorithm>

static void setCairoColor(cairo_t* cr, const Color& c) {
    cairo_set_source_rgba(cr, c.r, c.g, c.b, c.a);
}

void Renderer::render(
    cairo_t* cr, 
    const std::shared_ptr<LayoutBox>& rootBox, 
    double scrollY, 
    double viewportWidth, 
    double viewportHeight
) {
    // Fill background with clean white / theme background
    cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
    cairo_paint(cr);

    if (!rootBox) return;

    PangoLayout* pangoLayout = pango_cairo_create_layout(cr);

    // Render tree with scroll offset
    renderBox(cr, rootBox, scrollY, viewportWidth, viewportHeight, pangoLayout);

    g_object_unref(pangoLayout);
}

void Renderer::renderBox(
    cairo_t* cr, 
    const std::shared_ptr<LayoutBox>& box, 
    double scrollY, 
    double viewportWidth, 
    double viewportHeight,
    PangoLayout* pangoLayout
) {
    if (!box) return;

    double drawY = box->y - scrollY;

    // Viewport cull check
    if (drawY + box->height < -50.0 || drawY > viewportHeight + 50.0) {
        // Skip drawing if outside visible window
    } else {
        // 1. Draw background
        if (box->style.hasBg) {
            setCairoColor(cr, box->style.bgColor);
            cairo_rectangle(cr, box->x, drawY, box->width, box->height);
            cairo_fill(cr);
        }

        // 2. Draw border
        if (box->style.borderWidth > 0.0) {
            setCairoColor(cr, box->style.borderColor);
            cairo_set_line_width(cr, box->style.borderWidth);
            cairo_rectangle(cr, box->x, drawY, box->width, box->height);
            cairo_stroke(cr);
        }

        // 3. Draw horizontal rule
        if (box->type == BoxType::HR) {
            cairo_set_source_rgb(cr, 0.75, 0.78, 0.82);
            cairo_set_line_width(cr, 1.5);
            cairo_move_to(cr, box->x, drawY);
            cairo_line_to(cr, box->x + box->width, drawY);
            cairo_stroke(cr);
        }

        // 4. Draw Image
        if (box->type == BoxType::IMAGE && !box->imageSrc.empty()) {
            GError* err = nullptr;
            GdkPixbuf* pixbuf = gdk_pixbuf_new_from_file_at_scale(
                box->imageSrc.c_str(), 
                static_cast<int>(box->width), 
                static_cast<int>(box->height), 
                TRUE, 
                &err
            );
            if (pixbuf) {
                gdk_cairo_set_source_pixbuf(cr, pixbuf, box->x, drawY);
                cairo_paint(cr);
                g_object_unref(pixbuf);
            } else {
                if (err) g_error_free(err);
                // Draw fallback placeholder
                cairo_set_source_rgb(cr, 0.9, 0.92, 0.95);
                cairo_rectangle(cr, box->x, drawY, box->width, box->height);
                cairo_fill(cr);
                cairo_set_source_rgb(cr, 0.6, 0.65, 0.7);
                cairo_set_line_width(cr, 1.0);
                cairo_rectangle(cr, box->x, drawY, box->width, box->height);
                cairo_stroke(cr);
            }
        }

        // 5. Draw Text Run
        if (box->type == BoxType::TEXT_RUN && !box->text.empty()) {
            std::string fontDescStr;
            if (box->style.isMonospace) fontDescStr = "Monospace ";
            else fontDescStr = "Sans ";

            if (box->style.isBold) fontDescStr += "Bold ";
            if (box->style.isItalic) fontDescStr += "Italic ";
            fontDescStr += std::to_string(static_cast<int>(box->style.fontSize)) + "px";

            PangoFontDescription* desc = pango_font_description_from_string(fontDescStr.c_str());
            pango_layout_set_font_description(pangoLayout, desc);
            pango_font_description_free(desc);

            pango_layout_set_text(pangoLayout, box->text.c_str(), -1);
            pango_layout_set_width(pangoLayout, -1);

            setCairoColor(cr, box->style.color);
            cairo_move_to(cr, box->x, drawY);
            pango_cairo_show_layout(cr, pangoLayout);

            // Underline if link
            if (box->style.isUnderline) {
                double lineY = drawY + box->height - 1.0;
                cairo_set_line_width(cr, 1.0);
                setCairoColor(cr, box->style.color);
                cairo_move_to(cr, box->x, lineY);
                cairo_line_to(cr, box->x + box->width, lineY);
                cairo_stroke(cr);
            }
        }
    }

    // Recursively render children
    for (const auto& child : box->children) {
        renderBox(cr, child, scrollY, viewportWidth, viewportHeight, pangoLayout);
    }
}
