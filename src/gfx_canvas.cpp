#include "gfx_canvas.hpp"
#include <curl/curl.h>
#include <vector>

struct CurlImageData {
    std::vector<unsigned char> buffer;
};

static size_t curlImageWriteCallback(void* ptr, size_t size, size_t nmemb, void* userp) {
    size_t total = size * nmemb;
    auto* mem = static_cast<CurlImageData*>(userp);
    const unsigned char* bytes = static_cast<const unsigned char*>(ptr);
    mem->buffer.insert(mem->buffer.end(), bytes, bytes + total);
    return total;
}

GdkPixbuf* GfxCanvas::PixmapCache::downloadAndDecode(const std::string& url, int targetWidth, int targetHeight) {
    CURL* curl = curl_easy_init();
    if (!curl) return nullptr;

    CurlImageData data;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlImageWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "MK-Browse/1.0 (MKCorpo; ImageLoader)");

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK || data.buffer.empty()) {
        return nullptr;
    }

    GError* err = nullptr;
    GdkPixbufLoader* loader = gdk_pixbuf_loader_new();
    if (!loader) return nullptr;

    if (!gdk_pixbuf_loader_write(loader, data.buffer.data(), data.buffer.size(), &err)) {
        if (err) g_error_free(err);
        gdk_pixbuf_loader_close(loader, nullptr);
        g_object_unref(loader);
        return nullptr;
    }

    gdk_pixbuf_loader_close(loader, nullptr);
    GdkPixbuf* original = gdk_pixbuf_loader_get_pixbuf(loader);
    if (!original) {
        g_object_unref(loader);
        return nullptr;
    }

    GdkPixbuf* result = nullptr;
    if (targetWidth > 0 && targetHeight > 0) {
        result = gdk_pixbuf_scale_simple(original, targetWidth, targetHeight, GDK_INTERP_BILINEAR);
    } else {
        result = gdk_pixbuf_copy(original);
    }

    g_object_unref(loader);
    return result;
}

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

GdkPixbuf* GfxCanvas::PixmapCache::get(const std::string& pathOrUrl, int targetWidth, int targetHeight) {
    if (pathOrUrl.empty()) return nullptr;

    std::lock_guard<std::mutex> lock(mtx_);
    std::string key = pathOrUrl + "@" + std::to_string(targetWidth) + "x" + std::to_string(targetHeight);

    // 1. Check cache
    auto it = map_.find(key);
    if (it != map_.end()) {
        lru_.erase(it->second.first);
        lru_.push_front(key);
        it->second.first = lru_.begin();
        return it->second.second.pixbuf;
    }

    // 2. Load from Web or Local Disk
    GdkPixbuf* pb = nullptr;
    if (pathOrUrl.rfind("http://", 0) == 0 || pathOrUrl.rfind("https://", 0) == 0) {
        pb = downloadAndDecode(pathOrUrl, targetWidth, targetHeight);
    } else {
        std::string localPath = pathOrUrl;
        if (localPath.rfind("file://", 0) == 0) localPath = localPath.substr(7);

        GError* err = nullptr;
        if (targetWidth > 0 && targetHeight > 0) {
            pb = gdk_pixbuf_new_from_file_at_scale(localPath.c_str(), targetWidth, targetHeight, TRUE, &err);
        } else {
            pb = gdk_pixbuf_new_from_file(localPath.c_str(), &err);
        }
        if (!pb && err) g_error_free(err);
    }

    if (!pb) return nullptr;

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
}
