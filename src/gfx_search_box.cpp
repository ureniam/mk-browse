#include "gfx_search_box.hpp"
#include <curl/curl.h>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static std::string urlEncode(const std::string& raw) {
    CURL* curl = curl_easy_init();
    if (!curl) return raw;
    char* escaped = curl_easy_escape(curl, raw.c_str(), static_cast<int>(raw.length()));
    std::string res = (escaped ? std::string(escaped) : raw);
    if (escaped) curl_free(escaped);
    curl_easy_cleanup(curl);
    return res;
}

GfxSearchBox::GfxSearchBox(const std::string& placeholder, SearchCallback callback)
    : callback_(std::move(callback)) {
    entry_ = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_), placeholder.c_str());
    gtk_entry_set_has_frame(GTK_ENTRY(entry_), FALSE);

    GtkStyleContext* ctx = gtk_widget_get_style_context(entry_);
    gtk_style_context_add_class(ctx, "gfx-search-box");

    // CSS styling
    GtkCssProvider* provider = gtk_css_provider_new();
    const char* css = R"(
        entry.gfx-search-box {
            background-color: #0b1120;
            color: #38bdf8;
            caret-color: #38bdf8;
            font-family: monospace, sans-serif;
            font-size: 13px;
            padding: 6px 14px;
            border-radius: 8px;
            border: 1px solid #1e293b;
        }
        entry.gfx-search-box:focus {
            background-color: #111827;
            border-color: #0284c7;
        }
    )";
    gtk_css_provider_load_from_data(provider, css, -1, nullptr);
    gtk_style_context_add_provider(ctx, GTK_STYLE_PROVIDER(provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(provider);

    g_signal_connect(entry_, "draw", G_CALLBACK(onDraw), this);
    g_signal_connect(entry_, "activate", G_CALLBACK(onActivate), this);
}

GfxSearchBox::~GfxSearchBox() {
}

void GfxSearchBox::setText(const std::string& text) {
    if (entry_) {
        gtk_entry_set_text(GTK_ENTRY(entry_), text.c_str());
    }
}

std::string GfxSearchBox::getText() const {
    if (!entry_) return "";
    const char* cstr = gtk_entry_get_text(GTK_ENTRY(entry_));
    return cstr ? std::string(cstr) : "";
}

void GfxSearchBox::setCallback(SearchCallback callback) {
    callback_ = std::move(callback);
}

gboolean GfxSearchBox::onActivate(GtkEntry* entry, gpointer userData) {
    GfxSearchBox* self = static_cast<GfxSearchBox*>(userData);
    if (!self->callback_) return FALSE;

    const char* cstr = gtk_entry_get_text(entry);
    std::string text(cstr ? cstr : "");

    // Trim whitespace
    text.erase(0, text.find_first_not_of(" \t\n\r"));
    text.erase(text.find_last_not_of(" \t\n\r") + 1);

    if (text.empty()) return FALSE;

    // Smart query detection: if no protocol and no domain dot, route to DuckDuckGo with proper URL encoding
    if (text.rfind("http://", 0) != 0 &&
        text.rfind("https://", 0) != 0 &&
        text.rfind("about:", 0) != 0 &&
        text.rfind("file://", 0) != 0) {
        if (text.find(' ') != std::string::npos || text.find('.') == std::string::npos) {
            std::string encoded = urlEncode(text);
            text = "https://html.duckduckgo.com/html/?q=" + encoded;
        } else {
            text = "https://" + text;
        }
    }

    self->callback_(text);
    return FALSE;
}

gboolean GfxSearchBox::onDraw(GtkWidget* widget, cairo_t* cr, gpointer userData) {
    (void)userData;
    GtkAllocation alloc;
    gtk_widget_get_allocation(widget, &alloc);

    double r = 8.0;
    cairo_new_path(cr);
    cairo_arc(cr, alloc.width - r, r, r, -M_PI/2, 0);
    cairo_arc(cr, alloc.width - r, alloc.height - r, r, 0, M_PI/2);
    cairo_arc(cr, r, alloc.height - r, r, M_PI/2, M_PI);
    cairo_arc(cr, r, r, r, M_PI, 3*M_PI/2);
    cairo_close_path(cr);

    cairo_set_source_rgba(cr, 0.02, 0.52, 0.78, 0.4);
    cairo_set_line_width(cr, 1.0);
    cairo_stroke(cr);

    return FALSE;
}
