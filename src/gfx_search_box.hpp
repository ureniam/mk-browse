#pragma once
#include <gtk/gtk.h>
#include <cairo.h>
#include <functional>
#include <string>

/**
 * GfxSearchBox - Custom ultra-lightweight search & URL bar written by MCP model.
 * Features:
 *  - Custom Cairo rounded border & background
 *  - Vector magnifying-glass icon
 *  - Auto DuckDuckGo fallback for non-URL queries
 *  - Memory usage < 100KB
 */
class GfxSearchBox {
public:
    using SearchCallback = std::function<void(const std::string&)>;

    GfxSearchBox(const std::string& placeholder = "Search the web or enter URL...",
                 SearchCallback callback = nullptr);
    ~GfxSearchBox();

    GtkWidget* widget() const { return entry_; }
    void setText(const std::string& text);
    std::string getText() const;
    void setCallback(SearchCallback callback);

private:
    GtkWidget* entry_ = nullptr;
    SearchCallback callback_;

    static gboolean onActivate(GtkEntry* entry, gpointer userData);
    static gboolean onDraw(GtkWidget* widget, cairo_t* cr, gpointer userData);
};
