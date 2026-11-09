#include "gfx_canvas.hpp"

GfxCanvas::PixmapCache::PixmapCache(size_t maxBytes) : maxBytes_(maxBytes) {
}

GfxCanvas::PixmapCache::~PixmapCache() {
    std::lock_guard<std::mutex> lock(mtx_);
    for (auto& pair : map_) {
        if (pair.second.second.pixbuf) {
            g_object_unref(pair.second.second.pixbuf);
        }
    }
    map_.clear();
    lru_.clear();
}

GdkPixbuf* GfxCanvas::PixmapCache::get(const std::string& path, int targetWidth, int targetHeight) {
    std::lock_guard<std::mutex> lock(mtx_);
    std::string key = path + "@" + std::to_string(targetWidth) + "x" + std::to_string(targetHeight);

    auto it = map_.find(key);
    if (it != map_.end()) {
        lru_.erase(it->second.first);
        lru_.push_front(key);
        it->second.first = lru_.begin();
        return it->second.second.pixbuf;
    }

    GError* err = nullptr;
    GdkPixbuf* pb = nullptr;
    if (targetWidth > 0 && targetHeight > 0) {
        pb = gdk_pixbuf_new_from_file_at_scale(path.c_str(), targetWidth, targetHeight, TRUE, &err);
    } else {
        pb = gdk_pixbuf_new_from_file(path.c_str(), &err);
    }

    if (!pb) {
        if (err) g_error_free(err);
        return nullptr;
    }

    size_t size = gdk_pixbuf_get_width(pb) * gdk_pixbuf_get_height(pb) * gdk_pixbuf_get_n_channels(pb);
    evictUntilFits(size);

    lru_.push_front(key);
    Entry entry{pb, size};
    map_[key] = {lru_.begin(), entry};
    usedBytes_ += size;

    return pb;
}

void GfxCanvas::PixmapCache::evictUntilFits(size_t needed) {
    while (usedBytes_ + needed > maxBytes_ && !lru_.empty()) {
        const std::string& oldKey = lru_.back();
        auto it = map_.find(oldKey);
        if (it != map_.end()) {
            if (it->second.second.pixbuf) {
                g_object_unref(it->second.second.pixbuf);
            }
            usedBytes_ -= it->second.second.sizeBytes;
            map_.erase(it);
        }
        lru_.pop_back();
    }
}

GfxCanvas::GfxCanvas(GtkWidget* drawingArea, size_t cacheMaxBytes)
    : drawingArea_(drawingArea), cache_(cacheMaxBytes) {
}

GfxCanvas::~GfxCanvas() {
    std::lock_guard<std::mutex> lock(drawMtx_);
    if (crBack_) cairo_destroy(crBack_);
    if (back_) cairo_surface_destroy(back_);
    if (front_) cairo_surface_destroy(front_);
}

void GfxCanvas::ensureSurfaces(int w, int h) {
    if (width_ == w && height_ == h) return;
    std::lock_guard<std::mutex> lock(drawMtx_);

    if (crBack_) { cairo_destroy(crBack_); crBack_ = nullptr; }
    if (back_) { cairo_surface_destroy(back_); back_ = nullptr; }
    if (front_) { cairo_surface_destroy(front_); front_ = nullptr; }

    if (w > 0 && h > 0) {
        back_ = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
        crBack_ = cairo_create(back_);
        width_ = w;
        height_ = h;
    }
}

void GfxCanvas::resize(int w, int h) {
    ensureSurfaces(w, h);
}
