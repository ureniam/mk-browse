#pragma once
#include "browser_tab.hpp"
#include <gtk/gtk.h>
#include <vector>
#include <memory>
#include <string>

class MainWindow {
public:
    GtkWidget* window = nullptr;
    GtkWidget* mainBox = nullptr;
    GtkWidget* toolbar = nullptr;
    GtkWidget* bookmarksBar = nullptr;
    GtkWidget* notebook = nullptr;
    GtkWidget* statusBar = nullptr;
    GtkWidget* statusLabel = nullptr;
    GtkWidget* memLabel = nullptr;

    // Nav controls
    GtkWidget* btnBack = nullptr;
    GtkWidget* btnForward = nullptr;
    GtkWidget* btnReload = nullptr;
    GtkWidget* btnHome = nullptr;
    GtkWidget* urlEntry = nullptr;
    GtkWidget* btnGo = nullptr;
    GtkWidget* btnNewTab = nullptr;
    GtkWidget* btnViewSource = nullptr;

    std::vector<std::shared_ptr<BrowserTab>> tabs;

    MainWindow();
    ~MainWindow();

    void show();
    void createNewTab(const std::string& url = "about:home");
    void closeTab(int index);
    std::shared_ptr<BrowserTab> getCurrentTab();

private:
    void setupUi();
    void setupStyles();
    void updateNavButtons();
    void updateMemoryUsage();
    void showSourceViewer();

    // Callbacks
    static void onEntryActivate(GtkEntry* entry, gpointer userData);
    static void onBackClicked(GtkButton* btn, gpointer userData);
    static void onForwardClicked(GtkButton* btn, gpointer userData);
    static void onReloadClicked(GtkButton* btn, gpointer userData);
    static void onHomeClicked(GtkButton* btn, gpointer userData);
    static void onGoClicked(GtkButton* btn, gpointer userData);
    static void onNewTabClicked(GtkButton* btn, gpointer userData);
    static void onViewSourceClicked(GtkButton* btn, gpointer userData);
    static void onNotebookSwitchPage(GtkNotebook* nb, GtkWidget* page, guint pageNum, gpointer userData);
    static gboolean onMemTimer(gpointer userData);
};
