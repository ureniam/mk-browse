#pragma once
#include "dom.hpp"
#include "layout.hpp"
#include "network.hpp"
#include <gtk/gtk.h>
#include <string>
#include <vector>
#include <memory>
#include <functional>

class BrowserTab {
public:
    std::string currentUrl;
    std::string pageTitle = "New Tab";
    std::string rawHtml;

    std::vector<std::string> history;
    int historyIndex = -1;

    std::shared_ptr<DOMNode> domRoot;
    std::shared_ptr<LayoutBox> layoutRoot;

    double scrollY = 0.0;
    double totalHeight = 0.0;
    double viewportWidth = 800.0;
    double viewportHeight = 600.0;

    std::string hoveredLink;
    HttpResponse lastResponse;

    GtkWidget* containerWidget = nullptr;
    GtkWidget* drawingArea = nullptr;
    GtkWidget* vScrollbar = nullptr;
    GtkAdjustment* vAdjustment = nullptr;
    GtkWidget* tabLabelWidget = nullptr;

    std::function<void(const std::string& url)> onNavigateCallback;
    std::function<void(const std::string& title)> onTitleChangeCallback;
    std::function<void(const std::string& status)> onStatusUpdateCallback;

    BrowserTab(const std::string& initialUrl = "about:home");
    ~BrowserTab();

    GtkWidget* getWidget() { return containerWidget; }
    void loadUrl(const std::string& url, bool isHistoryNav = false);
    void reload();
    void goBack();
    void goForward();
    bool canGoBack() const { return historyIndex > 0; }
    bool canGoForward() const { return historyIndex + 1 < static_cast<int>(history.size()); }

    void relayout();

private:
    void setupUi();
    static gboolean onDraw(GtkWidget* widget, cairo_t* cr, gpointer userData);
    static gboolean onButtonPress(GtkWidget* widget, GdkEventButton* event, gpointer userData);
    static gboolean onMotionNotify(GtkWidget* widget, GdkEventMotion* event, gpointer userData);
    static gboolean onScroll(GtkWidget* widget, GdkEventScroll* event, gpointer userData);
    static void onSizeAllocate(GtkWidget* widget, GtkAllocation* allocation, gpointer userData);
    static void onAdjustmentValueChanged(GtkAdjustment* adj, gpointer userData);
};
