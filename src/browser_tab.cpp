#include "browser_tab.hpp"
#include "html_parser.hpp"
#include "renderer.hpp"

BrowserTab::BrowserTab(const std::string& initialUrl) {
    setupUi();
    loadUrl(initialUrl);
}

BrowserTab::~BrowserTab() {
}

void BrowserTab::setupUi() {
    containerWidget = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

    drawingArea = gtk_drawing_area_new();
    gtk_widget_set_can_focus(drawingArea, TRUE);
    gtk_widget_add_events(drawingArea, 
        GDK_BUTTON_PRESS_MASK | 
        GDK_POINTER_MOTION_MASK | 
        GDK_SCROLL_MASK | 
        GDK_LEAVE_NOTIFY_MASK
    );

    // Initialize GfxCanvas for this tab
    canvas = std::make_unique<GfxCanvas>(drawingArea, 40 * 1024 * 1024);

    gtk_box_pack_start(GTK_BOX(containerWidget), drawingArea, TRUE, TRUE, 0);

    // Vertical scrollbar
    vAdjustment = gtk_adjustment_new(0.0, 0.0, 1000.0, 20.0, 100.0, 600.0);
    vScrollbar = gtk_scrollbar_new(GTK_ORIENTATION_VERTICAL, vAdjustment);
    gtk_box_pack_start(GTK_BOX(containerWidget), vScrollbar, FALSE, FALSE, 0);

    // Signals
    g_signal_connect(drawingArea, "draw", G_CALLBACK(onDraw), this);
    g_signal_connect(drawingArea, "button-press-event", G_CALLBACK(onButtonPress), this);
    g_signal_connect(drawingArea, "motion-notify-event", G_CALLBACK(onMotionNotify), this);
    g_signal_connect(drawingArea, "scroll-event", G_CALLBACK(onScroll), this);
    g_signal_connect(drawingArea, "size-allocate", G_CALLBACK(onSizeAllocate), this);
    g_signal_connect(vAdjustment, "value-changed", G_CALLBACK(onAdjustmentValueChanged), this);

    gtk_widget_show_all(containerWidget);
}

void BrowserTab::loadUrl(const std::string& url, bool isHistoryNav) {
    std::string normUrl = NetworkEngine::normalizeUrl(url);

    if (onStatusUpdateCallback) {
        onStatusUpdateCallback("Connecting to " + normUrl + "...");
    }

    lastResponse = NetworkEngine::fetch(normUrl);
    currentUrl = lastResponse.finalUrl;
    rawHtml = lastResponse.body;

    if (!isHistoryNav) {
        if (historyIndex >= 0 && historyIndex + 1 < static_cast<int>(history.size())) {
            history.erase(history.begin() + historyIndex + 1, history.end());
        }
        history.push_back(currentUrl);
        historyIndex = history.size() - 1;
    }

    // Parse HTML
    domRoot = HTMLParser::parse(rawHtml);
    pageTitle = HTMLParser::extractTitle(domRoot);

    if (onTitleChangeCallback) {
        onTitleChangeCallback(pageTitle);
    }
    if (onNavigateCallback) {
        onNavigateCallback(currentUrl);
    }

    scrollY = 0.0;
    gtk_adjustment_set_value(vAdjustment, 0.0);

    relayout();

    if (onStatusUpdateCallback) {
        std::string status = "Done (" + std::to_string(static_cast<int>(lastResponse.elapsedMs)) + "ms) - HTTP " + 
                             std::to_string(lastResponse.statusCode);
        onStatusUpdateCallback(status);
    }
}

void BrowserTab::reload() {
    loadUrl(currentUrl, true);
}

void BrowserTab::goBack() {
    if (canGoBack()) {
        historyIndex--;
        loadUrl(history[historyIndex], true);
    }
}

void BrowserTab::goForward() {
    if (canGoForward()) {
        historyIndex++;
        loadUrl(history[historyIndex], true);
    }
}

void BrowserTab::relayout() {
    if (!drawingArea || !gtk_widget_get_realized(drawingArea)) return;

    GtkAllocation alloc;
    gtk_widget_get_allocation(drawingArea, &alloc);
    viewportWidth = alloc.width;
    viewportHeight = alloc.height;

    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1);
    cairo_t* cr = cairo_create(surface);

    layoutRoot = LayoutEngine::layout(domRoot, viewportWidth, cr, totalHeight);

    cairo_destroy(cr);
    cairo_surface_destroy(surface);

    // Update scrollbar
    gtk_adjustment_set_upper(vAdjustment, std::max(totalHeight, viewportHeight));
    gtk_adjustment_set_page_size(vAdjustment, viewportHeight);

    gtk_widget_queue_draw(drawingArea);
}

gboolean BrowserTab::onDraw(GtkWidget* widget, cairo_t* cr, gpointer userData) {
    (void)widget;
    BrowserTab* tab = static_cast<BrowserTab*>(userData);
    Renderer::render(cr, tab->layoutRoot, tab->scrollY, tab->viewportWidth, tab->viewportHeight, tab->canvas->cache());
    return FALSE;
}

gboolean BrowserTab::onButtonPress(GtkWidget* widget, GdkEventButton* event, gpointer userData) {
    (void)widget;
    BrowserTab* tab = static_cast<BrowserTab*>(userData);
    if (event->button == GDK_BUTTON_PRIMARY && tab->layoutRoot) {
        double clickX = event->x;
        double clickY = event->y + tab->scrollY;
        std::string targetLink = tab->layoutRoot->findLinkAt(clickX, clickY);
        if (!targetLink.empty()) {
            if (targetLink.find("://") == std::string::npos && targetLink.rfind("about:", 0) != 0) {
                if (targetLink[0] == '/') {
                    size_t slashSlash = tab->currentUrl.find("://");
                    if (slashSlash != std::string::npos) {
                        size_t firstPathSlash = tab->currentUrl.find('/', slashSlash + 3);
                        std::string base = (firstPathSlash == std::string::npos) ? tab->currentUrl : tab->currentUrl.substr(0, firstPathSlash);
                        targetLink = base + targetLink;
                    }
                } else {
                    size_t lastSlash = tab->currentUrl.find_last_of('/');
                    if (lastSlash != std::string::npos && lastSlash > 7) {
                        targetLink = tab->currentUrl.substr(0, lastSlash + 1) + targetLink;
                    }
                }
            }
            tab->loadUrl(targetLink);
            return TRUE;
        }
    }
    return FALSE;
}

gboolean BrowserTab::onMotionNotify(GtkWidget* widget, GdkEventMotion* event, gpointer userData) {
    BrowserTab* tab = static_cast<BrowserTab*>(userData);
    if (tab->layoutRoot) {
        double mx = event->x;
        double my = event->y + tab->scrollY;
        std::string targetLink = tab->layoutRoot->findLinkAt(mx, my);

        GdkWindow* window = gtk_widget_get_window(widget);
        if (window) {
            if (!targetLink.empty()) {
                GdkCursor* cursor = gdk_cursor_new_from_name(gdk_display_get_default(), "pointer");
                gdk_window_set_cursor(window, cursor);
                if (cursor) g_object_unref(cursor);

                if (tab->onStatusUpdateCallback && tab->hoveredLink != targetLink) {
                    tab->onStatusUpdateCallback(targetLink);
                }
            } else {
                gdk_window_set_cursor(window, nullptr);
                if (tab->onStatusUpdateCallback && !tab->hoveredLink.empty()) {
                    tab->onStatusUpdateCallback("Ready");
                }
            }
        }
        tab->hoveredLink = targetLink;
    }
    return FALSE;
}

gboolean BrowserTab::onScroll(GtkWidget* widget, GdkEventScroll* event, gpointer userData) {
    (void)widget;
    BrowserTab* tab = static_cast<BrowserTab*>(userData);
    double delta = 40.0;
    if (event->direction == GDK_SCROLL_UP) {
        tab->scrollY = std::max(0.0, tab->scrollY - delta);
    } else if (event->direction == GDK_SCROLL_DOWN) {
        double maxScroll = std::max(0.0, tab->totalHeight - tab->viewportHeight);
        tab->scrollY = std::min(maxScroll, tab->scrollY + delta);
    }
    gtk_adjustment_set_value(tab->vAdjustment, tab->scrollY);
    gtk_widget_queue_draw(tab->drawingArea);
    return TRUE;
}

void BrowserTab::onSizeAllocate(GtkWidget* widget, GtkAllocation* allocation, gpointer userData) {
    (void)widget;
    BrowserTab* tab = static_cast<BrowserTab*>(userData);
    if (tab->viewportWidth != allocation->width || tab->viewportHeight != allocation->height) {
        tab->viewportWidth = allocation->width;
        tab->viewportHeight = allocation->height;
        tab->relayout();
    }
}

void BrowserTab::onAdjustmentValueChanged(GtkAdjustment* adj, gpointer userData) {
    BrowserTab* tab = static_cast<BrowserTab*>(userData);
    tab->scrollY = gtk_adjustment_get_value(adj);
    gtk_widget_queue_draw(tab->drawingArea);
}
