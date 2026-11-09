#include "main_window.hpp"
#include <fstream>
#include <sstream>
#include <unistd.h>
#include <iomanip>

static double getMemoryRssMb() {
    std::ifstream statm("/proc/self/statm");
    if (!statm.is_open()) return 0.0;
    long size = 0, resident = 0;
    statm >> size >> resident;
    long pageSize = sysconf(_SC_PAGESIZE);
    return (resident * pageSize) / (1024.0 * 1024.0);
}

MainWindow::MainWindow() {
    setupUi();
    setupStyles();
    createNewTab("about:home");
    g_timeout_add_seconds(2, onMemTimer, this);
}

MainWindow::~MainWindow() {
}

void MainWindow::setupStyles() {
    GtkCssProvider* provider = gtk_css_provider_new();
    const char* css = R"(
        window {
            background-color: #0f172a;
        }
        .header-bar {
            background: linear-gradient(to bottom, #1e293b, #0f172a);
            padding: 6px 10px;
            border-bottom: 1px solid #334155;
        }
        .nav-btn {
            background-color: #334155;
            color: #f8fafc;
            border-radius: 6px;
            padding: 4px 10px;
            margin: 0 2px;
            border: 1px solid #475569;
            font-weight: bold;
        }
        .nav-btn:hover {
            background-color: #0284c7;
            border-color: #38bdf8;
        }
        .bookmark-bar {
            background-color: #1e293b;
            padding: 3px 10px;
            border-bottom: 1px solid #334155;
        }
        .bookmark-btn {
            background: transparent;
            color: #94a3b8;
            border: none;
            padding: 2px 8px;
            font-size: 12px;
        }
        .bookmark-btn:hover {
            color: #38bdf8;
            background-color: #334155;
            border-radius: 4px;
        }
        .status-bar {
            background-color: #0f172a;
            color: #94a3b8;
            padding: 4px 12px;
            border-top: 1px solid #334155;
            font-size: 12px;
        }
        notebook tab {
            background-color: #1e293b;
            color: #94a3b8;
            padding: 4px 12px;
            border-radius: 6px 6px 0 0;
            margin-right: 2px;
        }
        notebook tab:checked {
            background-color: #0f172a;
            color: #38bdf8;
            font-weight: bold;
            border-top: 2px solid #0284c7;
        }
    )";
    gtk_css_provider_load_from_data(provider, css, -1, nullptr);
    gtk_style_context_add_provider_for_screen(
        gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );
    g_object_unref(provider);
}

void MainWindow::setupUi() {
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "MK-Browse | MKCorpo Low-RAM Browser");
    gtk_window_set_default_size(GTK_WINDOW(window), 1080, 720);

    // Set Window Icon from MKCorpo logo
    GError* iconErr = nullptr;
    GdkPixbuf* iconPixbuf = gdk_pixbuf_new_from_file_at_scale(
        "/home/ubuntu/mk-browse/assets/mkcorpo_logo.png", 64, 64, TRUE, &iconErr
    );
    if (!iconPixbuf && iconErr) {
        g_error_free(iconErr);
        iconErr = nullptr;
        iconPixbuf = gdk_pixbuf_new_from_file_at_scale(
            "/home/ubuntu/mk-browse/assets/mkcorpo_favicon.png", 64, 64, TRUE, &iconErr
        );
    }
    if (iconPixbuf) {
        gtk_window_set_icon(GTK_WINDOW(window), iconPixbuf);
        g_object_unref(iconPixbuf);
    }

    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), nullptr);

    mainBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(window), mainBox);

    // 1. Navigation Toolbar
    toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_style_context_add_class(gtk_widget_get_style_context(toolbar), "header-bar");
    gtk_box_pack_start(GTK_BOX(mainBox), toolbar, FALSE, FALSE, 0);

    // Brand Badge with MKCorpo icon
    GtkWidget* brandBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    GdkPixbuf* smallLogo = gdk_pixbuf_new_from_file_at_scale(
        "/home/ubuntu/mk-browse/assets/mkcorpo_favicon.png", 22, 22, TRUE, nullptr
    );
    if (smallLogo) {
        GtkWidget* logoImg = gtk_image_new_from_pixbuf(smallLogo);
        gtk_box_pack_start(GTK_BOX(brandBox), logoImg, FALSE, FALSE, 0);
        g_object_unref(smallLogo);
    }
    GtkWidget* brandLabel = gtk_label_new("MK-Browse");
    gtk_box_pack_start(GTK_BOX(brandBox), brandLabel, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(toolbar), brandBox, FALSE, FALSE, 4);

    // Navigation Buttons
    btnBack = gtk_button_new_with_label("<");
    gtk_style_context_add_class(gtk_widget_get_style_context(btnBack), "nav-btn");
    g_signal_connect(btnBack, "clicked", G_CALLBACK(onBackClicked), this);
    gtk_box_pack_start(GTK_BOX(toolbar), btnBack, FALSE, FALSE, 0);

    btnForward = gtk_button_new_with_label(">");
    gtk_style_context_add_class(gtk_widget_get_style_context(btnForward), "nav-btn");
    g_signal_connect(btnForward, "clicked", G_CALLBACK(onForwardClicked), this);
    gtk_box_pack_start(GTK_BOX(toolbar), btnForward, FALSE, FALSE, 0);

    btnReload = gtk_button_new_with_label("Reload");
    gtk_style_context_add_class(gtk_widget_get_style_context(btnReload), "nav-btn");
    g_signal_connect(btnReload, "clicked", G_CALLBACK(onReloadClicked), this);
    gtk_box_pack_start(GTK_BOX(toolbar), btnReload, FALSE, FALSE, 0);

    btnHome = gtk_button_new_with_label("Home");
    gtk_style_context_add_class(gtk_widget_get_style_context(btnHome), "nav-btn");
    g_signal_connect(btnHome, "clicked", G_CALLBACK(onHomeClicked), this);
    gtk_box_pack_start(GTK_BOX(toolbar), btnHome, FALSE, FALSE, 0);

    // GfxSearchBox written by MCP Model
    searchBox = std::make_unique<GfxSearchBox>(
        "Search web with DuckDuckGo or enter URL...",
        [this](const std::string& targetUrl) {
            auto tab = getCurrentTab();
            if (tab) tab->loadUrl(targetUrl);
        }
    );
    gtk_box_pack_start(GTK_BOX(toolbar), searchBox->widget(), TRUE, TRUE, 4);

    btnGo = gtk_button_new_with_label("Go");
    gtk_style_context_add_class(gtk_widget_get_style_context(btnGo), "nav-btn");
    g_signal_connect(btnGo, "clicked", G_CALLBACK(onGoClicked), this);
    gtk_box_pack_start(GTK_BOX(toolbar), btnGo, FALSE, FALSE, 0);

    btnViewSource = gtk_button_new_with_label("</>");
    gtk_widget_set_tooltip_text(btnViewSource, "View Page Source");
    gtk_style_context_add_class(gtk_widget_get_style_context(btnViewSource), "nav-btn");
    g_signal_connect(btnViewSource, "clicked", G_CALLBACK(onViewSourceClicked), this);
    gtk_box_pack_start(GTK_BOX(toolbar), btnViewSource, FALSE, FALSE, 0);

    btnNewTab = gtk_button_new_with_label("+");
    gtk_widget_set_tooltip_text(btnNewTab, "Open New Tab");
    gtk_style_context_add_class(gtk_widget_get_style_context(btnNewTab), "nav-btn");
    g_signal_connect(btnNewTab, "clicked", G_CALLBACK(onNewTabClicked), this);
    gtk_box_pack_start(GTK_BOX(toolbar), btnNewTab, FALSE, FALSE, 0);

    // 2. Bookmarks Toolbar
    bookmarksBar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_style_context_add_class(gtk_widget_get_style_context(bookmarksBar), "bookmark-bar");
    gtk_box_pack_start(GTK_BOX(mainBox), bookmarksBar, FALSE, FALSE, 0);

    struct Bookmark { const char* label; const char* url; };
    Bookmark bookmarks[] = {
        {"Speed Dial", "about:home"},
        {"DuckDuckGo", "https://html.duckduckgo.com/html/"},
        {"Hacker News", "https://news.ycombinator.com"},
        {"Wikipedia", "https://en.wikipedia.org"},
        {"MKCorpo GitHub", "https://github.com/ureniam"},
        {"Engine Specs", "about:mkcorpo"}
    };

    for (const auto& bm : bookmarks) {
        GtkWidget* btn = gtk_button_new_with_label(bm.label);
        gtk_style_context_add_class(gtk_widget_get_style_context(btn), "bookmark-btn");
        g_object_set_data_full(G_OBJECT(btn), "target-url", g_strdup(bm.url), g_free);
        g_signal_connect(btn, "clicked", G_CALLBACK(+[](GtkButton* b, gpointer u) {
            MainWindow* w = static_cast<MainWindow*>(u);
            const char* target = static_cast<const char*>(g_object_get_data(G_OBJECT(b), "target-url"));
            auto tab = w->getCurrentTab();
            if (tab && target) tab->loadUrl(target);
        }), this);
        gtk_box_pack_start(GTK_BOX(bookmarksBar), btn, FALSE, FALSE, 0);
    }

    // 3. Tab Notebook
    notebook = gtk_notebook_new();
    gtk_notebook_set_scrollable(GTK_NOTEBOOK(notebook), TRUE);
    g_signal_connect(notebook, "switch-page", G_CALLBACK(onNotebookSwitchPage), this);
    gtk_box_pack_start(GTK_BOX(mainBox), notebook, TRUE, TRUE, 0);

    // 4. Status Bar
    statusBar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 12);
    gtk_style_context_add_class(gtk_widget_get_style_context(statusBar), "status-bar");
    gtk_box_pack_start(GTK_BOX(mainBox), statusBar, FALSE, FALSE, 0);

    statusLabel = gtk_label_new("Ready");
    gtk_label_set_xalign(GTK_LABEL(statusLabel), 0.0);
    gtk_box_pack_start(GTK_BOX(statusBar), statusLabel, TRUE, TRUE, 0);

    memLabel = gtk_label_new("RAM: -- MB");
    gtk_box_pack_end(GTK_BOX(statusBar), memLabel, FALSE, FALSE, 0);

    updateMemoryUsage();
}

void MainWindow::show() {
    gtk_widget_show_all(window);
}

void MainWindow::createNewTab(const std::string& url) {
    auto tab = std::make_shared<BrowserTab>(url);

    // Header label for tab with Close 'x' button
    GtkWidget* tabHeaderBox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    GtkWidget* titleLbl = gtk_label_new(tab->pageTitle.c_str());
    gtk_label_set_max_width_chars(GTK_LABEL(titleLbl), 15);
    gtk_label_set_ellipsize(GTK_LABEL(titleLbl), PANGO_ELLIPSIZE_END);
    gtk_box_pack_start(GTK_BOX(tabHeaderBox), titleLbl, FALSE, FALSE, 0);

    GtkWidget* closeBtn = gtk_button_new_with_label("x");
    gtk_style_context_add_class(gtk_widget_get_style_context(closeBtn), "bookmark-btn");
    gtk_box_pack_start(GTK_BOX(tabHeaderBox), closeBtn, FALSE, FALSE, 0);

    tab->tabLabelWidget = titleLbl;

    // Attach callbacks
    tab->onTitleChangeCallback = [this, titleLbl](const std::string& newTitle) {
        gtk_label_set_text(GTK_LABEL(titleLbl), newTitle.c_str());
        gtk_window_set_title(GTK_WINDOW(window), (newTitle + " - MK-Browse (MKCorpo)").c_str());
    };

    tab->onNavigateCallback = [this](const std::string& newUrl) {
        if (searchBox) searchBox->setText(newUrl);
        updateNavButtons();
    };

    tab->onStatusUpdateCallback = [this](const std::string& status) {
        gtk_label_set_text(GTK_LABEL(statusLabel), status.c_str());
        updateMemoryUsage();
    };

    tabs.push_back(tab);
    int pageIdx = gtk_notebook_append_page(GTK_NOTEBOOK(notebook), tab->getWidget(), tabHeaderBox);
    gtk_widget_show_all(tabHeaderBox);

    g_signal_connect(closeBtn, "clicked", G_CALLBACK(+[](GtkButton* b, gpointer u) {
        MainWindow* w = static_cast<MainWindow*>(u);
        int current = gtk_notebook_get_current_page(GTK_NOTEBOOK(w->notebook));
        w->closeTab(current);
    }), this);

    gtk_notebook_set_current_page(GTK_NOTEBOOK(notebook), pageIdx);
}

void MainWindow::closeTab(int index) {
    if (tabs.size() <= 1) {
        tabs[0]->loadUrl("about:home");
        return;
    }
    if (index >= 0 && index < static_cast<int>(tabs.size())) {
        gtk_notebook_remove_page(GTK_NOTEBOOK(notebook), index);
        tabs.erase(tabs.begin() + index);
    }
}

std::shared_ptr<BrowserTab> MainWindow::getCurrentTab() {
    int idx = gtk_notebook_get_current_page(GTK_NOTEBOOK(notebook));
    if (idx >= 0 && idx < static_cast<int>(tabs.size())) {
        return tabs[idx];
    }
    return nullptr;
}

void MainWindow::updateNavButtons() {
    auto tab = getCurrentTab();
    if (tab) {
        gtk_widget_set_sensitive(btnBack, tab->canGoBack());
        gtk_widget_set_sensitive(btnForward, tab->canGoForward());
    }
}

void MainWindow::updateMemoryUsage() {
    double mb = getMemoryRssMb();
    std::stringstream ss;
    ss << std::fixed << std::setprecision(1) << "RAM: " << mb << " MB (Low-Spec Target: 50-100MB)";
    gtk_label_set_text(GTK_LABEL(memLabel), ss.str().c_str());
}

void MainWindow::showSourceViewer() {
    auto tab = getCurrentTab();
    if (!tab) return;

    GtkWidget* dialog = gtk_dialog_new_with_buttons(
        ("Source: " + tab->currentUrl).c_str(),
        GTK_WINDOW(window),
        GTK_DIALOG_DESTROY_WITH_PARENT,
        "Close", GTK_RESPONSE_CLOSE,
        nullptr
    );
    gtk_window_set_default_size(GTK_WINDOW(dialog), 750, 500);

    GtkWidget* content = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
    GtkWidget* scroll = gtk_scrolled_window_new(nullptr, nullptr);
    GtkWidget* textView = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(textView), FALSE);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(textView), TRUE);

    GtkTextBuffer* buf = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textView));
    gtk_text_buffer_set_text(buf, tab->rawHtml.c_str(), -1);

    gtk_container_add(GTK_CONTAINER(scroll), textView);
    gtk_box_pack_start(GTK_BOX(content), scroll, TRUE, TRUE, 0);

    gtk_widget_show_all(dialog);
    g_signal_connect(dialog, "response", G_CALLBACK(gtk_widget_destroy), nullptr);
}

void MainWindow::onBackClicked(GtkButton* btn, gpointer userData) {
    (void)btn;
    MainWindow* w = static_cast<MainWindow*>(userData);
    auto tab = w->getCurrentTab();
    if (tab) tab->goBack();
}

void MainWindow::onForwardClicked(GtkButton* btn, gpointer userData) {
    (void)btn;
    MainWindow* w = static_cast<MainWindow*>(userData);
    auto tab = w->getCurrentTab();
    if (tab) tab->goForward();
}

void MainWindow::onReloadClicked(GtkButton* btn, gpointer userData) {
    (void)btn;
    MainWindow* w = static_cast<MainWindow*>(userData);
    auto tab = w->getCurrentTab();
    if (tab) tab->reload();
}

void MainWindow::onHomeClicked(GtkButton* btn, gpointer userData) {
    (void)btn;
    MainWindow* w = static_cast<MainWindow*>(userData);
    auto tab = w->getCurrentTab();
    if (tab) tab->loadUrl("about:home");
}

void MainWindow::onGoClicked(GtkButton* btn, gpointer userData) {
    (void)btn;
    MainWindow* w = static_cast<MainWindow*>(userData);
    if (!w->searchBox) return;
    std::string text = w->searchBox->getText();
    auto tab = w->getCurrentTab();
    if (tab && !text.empty()) tab->loadUrl(text);
}

void MainWindow::onNewTabClicked(GtkButton* btn, gpointer userData) {
    (void)btn;
    MainWindow* w = static_cast<MainWindow*>(userData);
    w->createNewTab("about:home");
}

void MainWindow::onViewSourceClicked(GtkButton* btn, gpointer userData) {
    (void)btn;
    MainWindow* w = static_cast<MainWindow*>(userData);
    w->showSourceViewer();
}

void MainWindow::onNotebookSwitchPage(GtkNotebook* nb, GtkWidget* page, guint pageNum, gpointer userData) {
    (void)nb; (void)page;
    MainWindow* w = static_cast<MainWindow*>(userData);
    if (pageNum < w->tabs.size()) {
        auto tab = w->tabs[pageNum];
        if (w->searchBox) w->searchBox->setText(tab->currentUrl);
        gtk_window_set_title(GTK_WINDOW(w->window), (tab->pageTitle + " - MK-Browse (MKCorpo)").c_str());
        w->updateNavButtons();
    }
}

gboolean MainWindow::onMemTimer(gpointer userData) {
    MainWindow* w = static_cast<MainWindow*>(userData);
    w->updateMemoryUsage();
    return TRUE;
}
