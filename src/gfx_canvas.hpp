#pragma once
#include <cairo.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtk.h>
#include <vector>
#include <unordered_map>
#include <list>
#include <mutex>
#include <string>
#include <memory>

/**
 * GfxCanvas - Double-buffered Cairo rendering canvas with bounded LRU image cache.
 * Designed for low-RAM devices (50-100MB budget).
 */
class GfxCanvas {
public:
    class PixmapCache {
    public:
        explicit PixmapCache(size_t maxBytes = 40 * 1024 * 1024); // 40 MB max image cache
        ~PixmapCache();

        GdkPixbuf* get(const std::string& path, int targetWidth, int targetHeight);
        size_t usedBytes() const { return usedBytes_; }

    private:
        struct Entry {
            GdkPixbuf* pixbuf = nullptr;
            size_t sizeBytes = 0;
        };

        size_t maxBytes_;
        size_t usedBytes_ = 0;
        std::list<std::string> lru_;
        std::unordered_map<std::string, std::pair<std::list<std::string>::iterator, Entry>> map_;
        std::mutex mtx_;

        void evictUntilFits(size_t needed);
    };

    explicit GfxCanvas(GtkWidget* drawingArea, size_t cacheMaxBytes = 40 * 1024 * 1024);
    ~GfxCanvas();

    PixmapCache& cache() { return cache_; }
    void ensureSurfaces(int w, int h);
    void resize(int w, int h);

private:
    GtkWidget* drawingArea_ = nullptr;
    PixmapCache cache_;
    cairo_surface_t* front_ = nullptr;
    cairo_surface_t* back_ = nullptr;
    cairo_t* crBack_ = nullptr;
    int width_ = 0;
    int height_ = 0;
    std::mutex drawMtx_;
};
