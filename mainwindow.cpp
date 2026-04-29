/*
 * Flashcards-by-Ethan
 * Copyright (C) 2026 Ethan McCall
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "mainwindow.h"
#include <QToolBar>
#include <QAction>
#include <QSplitter>
#include <QVBoxLayout>
#include <QLabel>
#include <QIcon>
#include <QDebug>
#include <QPushButton>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QMenu>
#include <QHeaderView>
#include <QAbstractItemView>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QStandardPaths>
#include <QDir>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QTimer>
#include <QResizeEvent>
#include <QToolButton>
#include <QDropEvent>
#include <QMessageBox>
#include <QGraphicsOpacityEffect>
#include <QPair>
#include <algorithm>
#include <random>
#include <QTextEdit>
#include <QScrollArea>
#include <QEnterEvent>
#include <QSpinBox>
#include <QPainter>
#include <QPaintEvent>
#include <algorithm>
#include <QDesktopServices>
#include <QUrl>
#include <QRandomGenerator>
#include <QGroupBox>
#include <QGridLayout>
#include <QDateTime>
#include <QDate>
#include <QComboBox>
#include <QCoreApplication>
#include <QInputDialog>
#include <QFileDialog>
#include <QCheckBox>
#include <QApplication>
#include <QTextBrowser>

class MasteryRadial : public QWidget
{
public:
    explicit MasteryRadial(QWidget *parent = nullptr, int size = 56)
        : QWidget(parent), m_value(0)
    {
        setFixedSize(size, size);
    }
    void setValue(int value)
    {
        m_value = std::clamp(value, 0, 100);
        update();
    }
    int value() const { return m_value; }
private:
    int m_value;
    void paintEvent(QPaintEvent*) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        int margin = width() / 8;
        int penWidth = width() / 8;
        QRectF r = rect().adjusted(margin, margin, -margin, -margin);
        p.setPen(QPen(QColor(69, 90, 111), penWidth));
        p.drawArc(r, 0, 360 * 16);
        QColor color;
        if (m_value <= 10) color = QColor(231, 76, 60);
        else if (m_value <= 30) color = QColor(230, 126, 34);
        else if (m_value <= 60) color = QColor(241, 196, 15);
        else if (m_value <= 80) color = QColor(46, 204, 113);
        else color = QColor(39, 174, 96);
        p.setPen(QPen(color, penWidth));
        p.drawArc(r, -90 * 16, (m_value * 360 / 100) * 16);
        int fontSize = qMax(8, width() / 5);
        p.setPen(Qt::white);
        p.setFont(QFont("Segoe UI", fontSize, QFont::Bold));
        p.drawText(rect(), Qt::AlignCenter, QString::number(m_value));
    }
};

CustomTreeWidget::CustomTreeWidget(QWidget *parent) : QTreeWidget(parent) {}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , isFlashcardMode(false)
    , choicesContainer(nullptr)
    , score(0)
    , choiceListWidget(nullptr)
    , prevButton(nullptr)
    , quizResults()
    , isReviewMode(false)
    , reviewCardList()
    , quizCardList()
    , currentCardIndex(0)
    , cardFlipped(false)
    , answered(false)
    , inQuizMode(false)
    , cardArea(nullptr)
    , actionArea(nullptr)
    , currentDeckItem(nullptr)
    , deckIsDirty(false)
    , draggedCard(nullptr)
    , dropPlaceholder(nullptr)
    , startQuizButton(nullptr)
    , shuffleButton(nullptr)
    , correctButton(nullptr)
    , wrongButton(nullptr)
    , ratingContainer(nullptr)
    , numQuestionsSpinBox(nullptr)
    , allDeckBacks()
    , dailyStreak(0)
    , lastStreakDate()
    , choiceButtons()
    , choiceLabels()
    , isSettingsPage(false)
    , randomDeckBtn(nullptr)
    , libraryBtn(nullptr)
    , folderBtn(nullptr)
    , styleToggleBtn(nullptr)
{
    setWindowTitle("Flashcards");
    inQuizMode = false;
    quizWidget = nullptr;
    resultsWidget = nullptr;
    frontLabel = nullptr;
    backLabel = nullptr;
    feedbackLabel = nullptr;
    actionButton = nullptr;
    nextButton = nullptr;
    cardRowsLayout = nullptr;
    cardContainer = nullptr;
    quizStyleGroup = nullptr;
    lastUsedFlashcardMode = true;
    lastUsedShuffle = true;

    // Top Navigation Bar
    navToolBar = addToolBar("Navigation");
    navToolBar->setMovable(false);
    navToolBar->setIconSize(QSize(28, 28));
    hamburgerAction = navToolBar->addAction(QIcon::fromTheme("application-menu", QIcon::fromTheme("view-menu")), "Menu");
    connect(hamburgerAction, &QAction::triggered, this, &MainWindow::toggleSidebar);
    QWidget *leftSpacer = new QWidget();
    leftSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    navToolBar->addWidget(leftSpacer);

    // Deck Buttons
    addCardAction = navToolBar->addAction(QIcon::fromTheme("list-add"), "Add Flashcard");
    addCardAction->setVisible(false);
    connect(addCardAction, &QAction::triggered, this, [this]() { addCardRow(); });
    saveDeckAction = navToolBar->addAction(QIcon::fromTheme("document-save"), "Save Deck");
    saveDeckAction->setVisible(false);
    connect(saveDeckAction, &QAction::triggered, this, &MainWindow::saveCurrentDeckCards);
    deckSeparator1 = navToolBar->addSeparator();
    renameDeckAction = navToolBar->addAction(QIcon::fromTheme("edit-rename"), "Rename Title");
    renameDeckAction->setVisible(false);
    connect(renameDeckAction, &QAction::triggered, this, &MainWindow::renameCurrentDeck);
    duplicateDeckAction = navToolBar->addAction(QIcon::fromTheme("edit-copy"), "Duplicate");
    duplicateDeckAction->setVisible(false);
    connect(duplicateDeckAction, &QAction::triggered, this, &MainWindow::duplicateCurrentDeck);
    deckSeparator2 = navToolBar->addSeparator();
    resetMasteryAction = navToolBar->addAction(
        QIcon::fromTheme("edit-clear", QIcon::fromTheme("view-refresh")), "Reset Mastery");
    resetMasteryAction->setVisible(false);
    connect(resetMasteryAction, &QAction::triggered, this, &MainWindow::handleResetMasteryClick);
    deleteDeckAction = navToolBar->addAction(QIcon::fromTheme("edit-delete"), "Delete Deck");
    deleteDeckAction->setVisible(false);
    connect(deleteDeckAction, &QAction::triggered, this, &MainWindow::handleDeleteDeckClick);

    // Folder Buttons
    renameFolderAction = navToolBar->addAction(QIcon::fromTheme("edit-rename"), "Rename Folder");
    renameFolderAction->setVisible(false);
    connect(renameFolderAction, &QAction::triggered, this, &MainWindow::renameCurrentFolder);
    folderSeparator1 = navToolBar->addSeparator();
    duplicateFolderFullAction = navToolBar->addAction(QIcon::fromTheme("edit-copy"), "Duplicate Folders + Decks");
    duplicateFolderFullAction->setVisible(false);
    connect(duplicateFolderFullAction, &QAction::triggered, this, &MainWindow::duplicateFolderFull);
    duplicateFolderEmptyAction = navToolBar->addAction(QIcon::fromTheme("folder-new"), "Duplicate Folders");
    duplicateFolderEmptyAction->setVisible(false);
    connect(duplicateFolderEmptyAction, &QAction::triggered, this, &MainWindow::duplicateFolderEmpty);
    folderSeparator2 = navToolBar->addSeparator();
    deleteFolderAction = navToolBar->addAction(QIcon::fromTheme("edit-delete"), "Delete Folder");
    deleteFolderAction->setVisible(false);
    connect(deleteFolderAction, &QAction::triggered, this, &MainWindow::handleDeleteFolderClick);

    // End Quiz Button
    endQuizAction = navToolBar->addAction(QIcon::fromTheme("process-stop", QIcon::fromTheme("dialog-cancel")), "End Quiz");
    endQuizAction->setVisible(false);
    endQuizConfirmPending = false;
    connect(endQuizAction, &QAction::triggered, this, &MainWindow::handleEndQuizClick);

    QWidget *smallSpacer = new QWidget();
    smallSpacer->setFixedWidth(35);
    navToolBar->addWidget(smallSpacer);
    settingsAction = navToolBar->addAction(QIcon::fromTheme("configure", QIcon::fromTheme("gear")), "Settings");
    connect(settingsAction, &QAction::triggered, this, &MainWindow::showSettingsPage);

    auto styleBtn = [this](QAction* a){
        if (QToolButton* b = qobject_cast<QToolButton*>(navToolBar->widgetForAction(a)))
            b->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    };
    styleBtn(addCardAction); styleBtn(saveDeckAction); styleBtn(renameDeckAction);
    styleBtn(duplicateDeckAction); styleBtn(deleteDeckAction);
    styleBtn(resetMasteryAction);
    styleBtn(renameFolderAction); styleBtn(duplicateFolderFullAction);
    styleBtn(duplicateFolderEmptyAction); styleBtn(deleteFolderAction);
    styleBtn(endQuizAction);

    splitter = new QSplitter(Qt::Horizontal, this);

    // Left Panel
    sidePanel = new QWidget();
    sidePanel->setMinimumWidth(60);
    sidePanel->setStyleSheet("background-color: #2c3e50; color: white; padding: 8px;");
    QVBoxLayout *sideLayout = new QVBoxLayout(sidePanel);
    sideLayout->setSpacing(8);
    sideLayout->setContentsMargins(8, 8, 8, 8);

    // Add Deck/Folder Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(6);
    addFolderBtn = new QPushButton("＋ Add Folder", sidePanel);
    addFolderBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #3498db;
            font-size: 15px;
            padding: 8px;
            font-weight: bold;
            border: none;
            color: white;
            border-radius: 8px;
        }
        QPushButton:hover {
            background-color: #3498db;
            border: 2px solid #ffffff;
            padding: 6px;
        }
        QPushButton:disabled {
            background-color: #7f8c8d;
            color: #bdc3c7;
        }
    )");
    connect(addFolderBtn, &QPushButton::clicked, this, &MainWindow::addNewFolder);
    buttonLayout->addWidget(addFolderBtn);

    addDeckBtn = new QPushButton("＋ Add Deck", sidePanel);
    addDeckBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #27ae60;
            font-size: 15px;
            padding: 8px;
            font-weight: bold;
            border: none;
            color: white;
            border-radius: 8px;
        }
        QPushButton:hover {
            background-color: #27ae60;
            border: 2px solid #ffffff;
            padding: 6px;
        }
        QPushButton:disabled {
            background-color: #7f8c8d;
            color: #bdc3c7;
        }
    )");
    connect(addDeckBtn, &QPushButton::clicked, this, &MainWindow::addNewDeckFromButton);
    buttonLayout->addWidget(addDeckBtn);
    sideLayout->addLayout(buttonLayout);

    // Folder Tree
    deckTree = new CustomTreeWidget(sidePanel);
    deckTree->setHeaderHidden(true);
    deckTree->setStyleSheet(R"(
        QTreeWidget {
            background-color: #34495e;
            color: white;
            font-size: 20px;
        }
        QTreeWidget::item:selected {
            border: 2px solid #3498db;
            border-radius: 8px;
            outline: 0;
        }
        QTreeWidget::item:hover {
            color: #84BCE0;
            border-radius: 8px;
        }
    )");
    deckTree->setContextMenuPolicy(Qt::CustomContextMenu);
    deckTree->setEditTriggers(QAbstractItemView::NoEditTriggers);
    deckTree->viewport()->installEventFilter(this);
    connect(deckTree, &QTreeWidget::customContextMenuRequested, this, &MainWindow::showContextMenu);
    connect(deckTree, &QTreeWidget::itemChanged, this, &MainWindow::onItemChanged);
    connect(deckTree, &QTreeWidget::itemSelectionChanged, this, &MainWindow::onDeckSelectionChanged);

    // Drag & Drop
    deckTree->setDragEnabled(true);
    deckTree->setAcceptDrops(true);
    deckTree->setDropIndicatorShown(true);
    deckTree->setDragDropMode(QAbstractItemView::InternalMove);
    deckTree->setDefaultDropAction(Qt::MoveAction);
    deckTree->viewport()->installEventFilter(this);

    connect(deckTree->model(), &QAbstractItemModel::rowsMoved,
            this, &MainWindow::saveDecks);
    sideLayout->addWidget(deckTree, 1);

    QHBoxLayout *expandLayout = new QHBoxLayout();
    expandLayout->setSpacing(6);
    QPushButton *expandAllBtn = new QPushButton("Expand All", sidePanel);
    QPushButton *collapseAllBtn = new QPushButton("Collapse All", sidePanel);
    expandAllBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #3498db;
            padding: 6px;
            font-weight: bold;
            border: none;
            color: white;
            border-radius: 8px;
        }
        QPushButton:hover {
            background-color: #3498db;
            border: 2px solid #ffffff;
            padding: 4px;
        }
    )");
    collapseAllBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #3498db;
            padding: 6px;
            font-weight: bold;
            border: none;
            color: white;
            border-radius: 8px;
        }
        QPushButton:hover {
            background-color: #3498db;
            border: 2px solid #ffffff;
            padding: 4px;
        }
    )");
    connect(expandAllBtn, &QPushButton::clicked, deckTree, &QTreeWidget::expandAll);
    connect(collapseAllBtn, &QPushButton::clicked, deckTree, &QTreeWidget::collapseAll);
    expandLayout->addWidget(expandAllBtn);
    expandLayout->addWidget(collapseAllBtn);
    sideLayout->addLayout(expandLayout);

    // Main Content Area
    mainContent = new QWidget();
    mainContent->setStyleSheet("background-color: #2c3e50;");
    mainContentLayout = new QVBoxLayout(mainContent);
    mainContentLayout->setContentsMargins(0, 0, 0, 0);

    splitter->addWidget(sidePanel);
    splitter->addWidget(mainContent);
    splitter->setSizes({380, 1120});
    lastSidebarWidth = 380;
    splitter->setHandleWidth(8);
    splitter->setCollapsible(0, true);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);

    connect(splitter, &QSplitter::splitterMoved, this, [this](int pos, int index) {
        if (index == 0 && pos > 80) {
            lastSidebarWidth = qMax(80, pos);
            saveSettings();
        }
    });

    setCentralWidget(splitter);
    updateAddButtonsState();
    resize(1500, 1000);

    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);
    decksFilePath = dataDir + "/decks.json";
    settingsFilePath = dataDir + "/settings.json";

    loadDecks();
    connect(deckTree, &QTreeWidget::itemExpanded,  this, &MainWindow::saveDecks);
    connect(deckTree, &QTreeWidget::itemCollapsed, this, &MainWindow::saveDecks);
    loadSettings();

    checkDailyStreakAtLaunch();

    applyStartOnLaunch();
}

void MainWindow::addNewFolder()
{
    ensureSidebarVisible();
    QTreeWidgetItem *selected = deckTree->currentItem();
    QTreeWidgetItem *parent = nullptr;
    if (selected && selected->data(0, Qt::UserRole).toString() == "folder") {
        parent = selected;
        selected->setExpanded(true);
    }
    QTreeWidgetItem *newItem = new QTreeWidgetItem(parent ? parent : deckTree->invisibleRootItem(),
                                                   QStringList() << "New Folder");
    newItem->setIcon(0, QIcon::fromTheme("folder"));
    newItem->setData(0, Qt::UserRole, "folder");
    newItem->setFlags(newItem->flags() | Qt::ItemIsEditable);
    if (!parent) {
        deckTree->addTopLevelItem(newItem);
    }
    deckTree->setCurrentItem(newItem);
    deckTree->editItem(newItem, 0);
    saveDecks();
}

void MainWindow::addNewDeckFromButton()
{
    ensureSidebarVisible();
    QTreeWidgetItem *selected = deckTree->currentItem();

    QTreeWidgetItem *parent = nullptr;
    if (selected && selected->data(0, Qt::UserRole).toString() == "folder") {
        parent = selected;
        selected->setExpanded(true);
    }

    QTreeWidgetItem *newItem = new QTreeWidgetItem(
        parent ? parent : deckTree->invisibleRootItem(),
        QStringList() << "New Deck");

    newItem->setIcon(0, QIcon::fromTheme("document-edit"));
    newItem->setData(0, Qt::UserRole, "deck");
    newItem->setData(0, Qt::UserRole + 2, QDateTime::currentDateTime().toString(Qt::ISODate));
    newItem->setFlags(newItem->flags() | Qt::ItemIsEditable);

    deckTree->setCurrentItem(newItem);
    deckTree->editItem(newItem, 0);

    QTimer::singleShot(0, this, [this, newItem]() {
        currentDeckItem = newItem;
        onDeckSelectionChanged();
        updateToolbarActions();
    });

    saveDecks();
}

// Context Menu
void MainWindow::showContextMenu(const QPoint &pos)
{
    QTreeWidgetItem *item = deckTree->itemAt(pos);
    if (!item) return;
    QString type = item->data(0, Qt::UserRole).toString();
    QMenu contextMenu(this);
    if (type == "folder") {
        QAction *renameF = contextMenu.addAction(QIcon::fromTheme("edit-rename"), "Rename Folder");
        connect(renameF, &QAction::triggered, this, &MainWindow::renameCurrentFolder);
        contextMenu.addSeparator();
        QAction *dupFull = contextMenu.addAction(QIcon::fromTheme("edit-copy"), "Duplicate Folders + Decks");
        connect(dupFull, &QAction::triggered, this, &MainWindow::duplicateFolderFull);
        QAction *dupEmpty = contextMenu.addAction(QIcon::fromTheme("folder-new"), "Duplicate Folders");
        connect(dupEmpty, &QAction::triggered, this, &MainWindow::duplicateFolderEmpty);
        contextMenu.addSeparator();
        QAction *expandAll = contextMenu.addAction(QIcon::fromTheme("expand-all"), "Expand All");
        connect(expandAll, &QAction::triggered, deckTree, &QTreeWidget::expandAll);
        QAction *collapseAll = contextMenu.addAction(QIcon::fromTheme("collapse-all"), "Collapse All");
        connect(collapseAll, &QAction::triggered, deckTree, &QTreeWidget::collapseAll);
        contextMenu.addSeparator();
        QAction *delF = contextMenu.addAction(QIcon::fromTheme("edit-delete"), "Delete Folder");
        connect(delF, &QAction::triggered, this, &MainWindow::deleteCurrentFolder);
    } else {
        QAction *renameD = contextMenu.addAction(QIcon::fromTheme("edit-rename"), "Rename Deck");
        connect(renameD, &QAction::triggered, this, &MainWindow::renameCurrentDeck);
        QAction *dupD = contextMenu.addAction(QIcon::fromTheme("edit-copy"), "Duplicate Deck");
        connect(dupD, &QAction::triggered, this, &MainWindow::duplicateCurrentDeck);
        contextMenu.addSeparator();
        QAction *delD = contextMenu.addAction(QIcon::fromTheme("edit-delete"), "Delete Deck");
        connect(delD, &QAction::triggered, this, &MainWindow::deleteCurrentDeck);
    }
    contextMenu.exec(deckTree->viewport()->mapToGlobal(pos));
}

void MainWindow::deleteSelectedFolder()
{
    QTreeWidgetItem *item = deckTree->currentItem();
    if (item) delete item;
}

void MainWindow::saveDecks()
{
    QJsonArray rootArray;
    for (int i = 0; i < deckTree->topLevelItemCount(); ++i) {
        saveTreeItem(rootArray, deckTree->topLevelItem(i));
    }
    QJsonDocument doc(rootArray);
    QFile file(decksFilePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
    }
}

void MainWindow::saveTreeItem(QJsonArray &array, QTreeWidgetItem *item)
{
    if (!item) return;

    QJsonObject obj;
    obj["name"] = item->text(0);
    obj["type"] = item->data(0, Qt::UserRole).toString();

    if (item->data(0, Qt::UserRole).toString() == "deck") {
        QVariant cardsVar = item->data(0, Qt::UserRole + 1);
        if (cardsVar.isValid() && cardsVar.canConvert<QJsonArray>()) {
            obj["cards"] = cardsVar.toJsonArray();
        }
        obj["lastQuiz"] = item->data(0, Qt::UserRole + 2).toString();
    }
    else if (item->data(0, Qt::UserRole).toString() == "folder") {
        obj["expanded"] = item->isExpanded();
    }

    QJsonArray children;
    for (int i = 0; i < item->childCount(); ++i) {
        saveTreeItem(children, item->child(i));
    }
    obj["children"] = children;

    array.append(obj);
}

QTreeWidgetItem* MainWindow::loadTreeItem(const QJsonObject &obj, QTreeWidgetItem *parent)
{
    if (obj.isEmpty()) return nullptr;

    QString type = obj["type"].toString();
    QTreeWidgetItem *item = new QTreeWidgetItem(QStringList() << obj["name"].toString());

    if (type == "deck") {
        item->setIcon(0, QIcon::fromTheme("document-edit"));
        if (obj.contains("cards")) {
            item->setData(0, Qt::UserRole + 1, obj["cards"].toArray());
        }
        item->setData(0, Qt::UserRole + 2, obj.value("lastQuiz").toString());
    }
    else if (type == "folder") {
        item->setIcon(0, QIcon::fromTheme("folder"));

        bool expanded = obj.value("expanded").toBool(true);
        item->setExpanded(expanded);
    }

    item->setData(0, Qt::UserRole, type);
    item->setFlags(item->flags() | Qt::ItemIsEditable);

    if (parent) {
        parent->addChild(item);
    } else {
        deckTree->addTopLevelItem(item);
    }

    QJsonArray children = obj["children"].toArray();
    for (const QJsonValue &childVal : children) {
        loadTreeItem(childVal.toObject(), item);
    }

    if (type == "folder") {
        bool expanded = obj.value("expanded").toBool(true);
        item->setExpanded(expanded);
    }

    return item;
}

void MainWindow::loadDecks()
{
    QFile file(decksFilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QTreeWidgetItem *rootFolder = new QTreeWidgetItem(QStringList() << "General");
        rootFolder->setIcon(0, QIcon::fromTheme("folder"));
        rootFolder->setData(0, Qt::UserRole, "folder");
        rootFolder->setFlags(rootFolder->flags() | Qt::ItemIsEditable);
        deckTree->addTopLevelItem(rootFolder);
        QTreeWidgetItem *exampleDeck = new QTreeWidgetItem(rootFolder, QStringList() << "My First Deck");
        exampleDeck->setIcon(0, QIcon::fromTheme("document-edit"));
        exampleDeck->setData(0, Qt::UserRole, "deck");
        exampleDeck->setData(0, Qt::UserRole + 2, QDateTime::currentDateTime().toString(Qt::ISODate));
        exampleDeck->setFlags(exampleDeck->flags() | Qt::ItemIsEditable);
        rootFolder->setExpanded(true);
        deckTree->expandAll();
        return;
    }
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (!doc.isArray()) return;
    deckTree->clear();
    QJsonArray rootArray = doc.array();
    for (const QJsonValue &val : rootArray) {
        loadTreeItem(val.toObject());
    }
}

void MainWindow::onDeckSelectionChanged()
{
    if (inQuizMode) {
        QTreeWidgetItem *selected = deckTree->currentItem();
        bool isSwitching = (selected != currentDeckItem) ||
                           (selected == nullptr && currentDeckItem != nullptr) ||
                           (selected != nullptr && currentDeckItem == nullptr);

        if (isSwitching) {
            if (!confirmExitQuiz()) {
                QSignalBlocker blocker(deckTree);
                if (currentDeckItem) {
                    deckTree->setCurrentItem(currentDeckItem);
                } else {
                    deckTree->clearSelection();
                }
                return;
            }
            endQuiz();
        }
    }

    QTreeWidgetItem *selected = deckTree->currentItem();
    updateAddButtonsState();

    if (selected && selected->data(0, Qt::UserRole).toString() == "deck") {
        if (selected != currentDeckItem) {
            currentDeckItem = selected;
            showDeckContent(selected);
        }
    } else {
        currentDeckItem = nullptr;
        showHomePage();
    }
    updateToolbarActions();
}

void MainWindow::resetMainContent()
{
    isSettingsPage = false;
    inQuizMode = false;
    currentCardIndex = 0;
    cardRowsLayout = nullptr;
    cardContainer = nullptr;
    correctButton = nullptr;
    wrongButton = nullptr;
    ratingContainer = nullptr;
    startQuizButton = nullptr;
    shuffleButton = nullptr;
    numQuestionsSpinBox = nullptr;
    allDeckBacks.clear();
    frontLabel = nullptr;
    backLabel = nullptr;
    feedbackLabel = nullptr;
    actionButton = nullptr;
    nextButton = nullptr;
    prevButton = nullptr;
    cardArea = nullptr;
    actionArea = nullptr;
    choiceButtons.clear();
    choicesContainer = nullptr;
    choiceListWidget = nullptr;
    choiceLabels.clear();
    score = 0;

    if (progressActionLeft)   { navToolBar->removeAction(progressActionLeft);   progressActionLeft = nullptr; }
    if (progressActionCenter) { navToolBar->removeAction(progressActionCenter); progressActionCenter = nullptr; }
    if (progressActionRight)  { navToolBar->removeAction(progressActionRight);  progressActionRight = nullptr; }
    if (quizProgressLabel) {
        quizProgressLabel->deleteLater();
        quizProgressLabel = nullptr;
    }

    if (deckMasteryRadial) {
        delete deckMasteryRadial;
        deckMasteryRadial = nullptr;
    }
    if (quizStyleGroup) {
        delete quizStyleGroup;
        quizStyleGroup = nullptr;
    }
    if (quizWidget) {
        delete quizWidget;
        quizWidget = nullptr;
    }
    if (resultsWidget) {
        delete resultsWidget;
        resultsWidget = nullptr;
    }
    if (currentDeckContainer) {
        delete currentDeckContainer;
        currentDeckContainer = nullptr;
    }

    QLayoutItem *item;
    while ((item = mainContentLayout->takeAt(0)) != nullptr) {
        if (item->widget())
            item->widget()->deleteLater();
        delete item;
    }

    updateToolbarActions();
}

void MainWindow::clearMainContent()
{
    resetMainContent();
    showHomePage();
}

// Show Deck Content
void MainWindow::showDeckContent(QTreeWidgetItem *deckItem)
{
    if (!deckItem) return;
    resetMainContent();
    currentDeckContainer = new QWidget();
    currentDeckContainer->setStyleSheet("background-color: #4a5259;");
    QVBoxLayout *deckLayout = new QVBoxLayout(currentDeckContainer);
    deckLayout->setSpacing(12);
    deckLayout->setContentsMargins(16, 16, 16, 16);

    QWidget *topArea = new QWidget();
    topArea->setStyleSheet("background-color: #2c3e50; border-radius: 8px; padding: 15px 0px 0px 15px;");
    QVBoxLayout *topVL = new QVBoxLayout(topArea);
    topVL->setSpacing(16);

    // Title Row
    QHBoxLayout *titleRow = new QHBoxLayout();
    titleRow->setSpacing(16);
    titleRow->setContentsMargins(0, 0, 0, 0);
    QWidget *radialContainer = new QWidget(topArea);
    QVBoxLayout *radialL = new QVBoxLayout(radialContainer);
    radialL->setContentsMargins(10, 10, 0, 0);
    radialL->setSpacing(0);
    radialL->setAlignment(Qt::AlignTop);
    deckMasteryRadial = new MasteryRadial(radialContainer, 72);
    deckMasteryRadial->setValue(getDeckAverageMastery(deckItem));
    deckMasteryRadial->setToolTip(QString("Deck Mastery: %1%").arg(deckMasteryRadial->value()));
    radialL->addWidget(deckMasteryRadial);

    QLabel *deckTitle = new QLabel(deckItem->text(0), topArea);
    deckTitle->setWordWrap(true);
    deckTitle->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    deckTitle->setStyleSheet("font-size: 28px; font-weight: bold; color: white; line-height: 1.25;");
    int cardCount = currentDeckItem->data(0, Qt::UserRole + 1).toJsonArray().size();
    QLabel *countLabel = new QLabel(QString("%1 cards").arg(cardCount), topArea);
    countLabel->setStyleSheet("font-size: 16px; color: #bdc3c7;");
    countLabel->setContentsMargins(0, 0, 15, 0);
    titleRow->addWidget(radialContainer, 0, Qt::AlignTop);
    titleRow->addWidget(deckTitle, 1);
    titleRow->addStretch();
    titleRow->addWidget(countLabel, 0, Qt::AlignTop);
    topVL->addLayout(titleRow);

    // Quiz Controls
    QHBoxLayout *quizRow = new QHBoxLayout();
    quizRow->setSpacing(12);
    quizRow->setContentsMargins(0, 0, 0, 0);
    quizRow->addSpacing(18);

    // Start Quiz Button
    startQuizButton = new QPushButton("Start Quiz", topArea);
    startQuizButton->setFixedWidth(150);
    startQuizButton->setFixedHeight(50);
    startQuizButton->setStyleSheet(R"(
        QPushButton {
            background-color: #3498db;
            color: white;
            padding: 14px 24px;
            font-size: 15px;
            font-weight: bold;
            border-radius: 10px;
            border: none;
            min-width: 160px;
        }
        QPushButton:hover {
            background-color: #2980b9;
            border: 2px solid white;
        }
        QPushButton:disabled {
            background-color: #7f8c8d;
            color: #bdc3c7;
        }
    )");

    auto updateStartButton = [this]() {
        if (!startQuizButton) return;
        int cardCount = currentDeckItem ?
                            currentDeckItem->data(0, Qt::UserRole + 1).toJsonArray().size() : 0;

        bool isMCQ = !lastUsedFlashcardMode;
        bool enabled = (cardCount > 0) && !(isMCQ && cardCount == 1);
        startQuizButton->setEnabled(enabled);
    };

    updateStartButton();
    connect(startQuizButton, &QPushButton::clicked, this, &MainWindow::startQuiz);

    //startQuizButton->setEnabled(cardCount > 0);

    // Quiz Type Button
    QPushButton *quizTypeBtn = new QPushButton(topArea);
    quizTypeBtn->setCheckable(true);
    quizTypeBtn->setChecked(lastUsedFlashcardMode);
    quizTypeBtn->setText(lastUsedFlashcardMode ? "Flashcard Style" : "Multiple Choice");
    quizTypeBtn->setFixedWidth(200);
    quizTypeBtn->setFixedHeight(50);
    quizTypeBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #27ae60;
            color: white;
            padding: 14px 24px;
            font-size: 15px;
            font-weight: bold;
            border-radius: 10px;
            border: none;
            min-width: 180px;
        }
        QPushButton:hover {
            border: 2px solid #ffffff;
        }
    )");
    connect(quizTypeBtn, &QPushButton::clicked, this, [this, quizTypeBtn, updateStartButton](bool checked) {
        lastUsedFlashcardMode = checked;
        quizTypeBtn->setText(checked ? "Flashcard Style" : "Multiple Choice");
        saveSettings();
        updateStartButton();
    });

    // Shuffle Button
    shuffleButton = new QPushButton("Shuffle", topArea);
    shuffleButton->setFixedWidth(200);
    shuffleButton->setFixedHeight(50);
    shuffleButton->setCheckable(true);
    shuffleButton->setChecked(lastUsedShuffle);
    shuffleButton->setStyleSheet(R"(
        QPushButton {
            background-color: #2c3e50;
            color: white;
            padding: 14px 24px;
            font-size: 15px;
            font-weight: bold;
            border-radius: 10px;
            border: 2px solid #455a6f;
            min-width: 140px;
        }
        QPushButton:checked {
            background-color: #27ae60;
            border: none;
        }
        QPushButton:hover {
            border: 2px solid #3498db;
        }
        QPushButton:checked:hover {
            border: 2px solid #ffffff;
        }
    )");
    connect(shuffleButton, &QPushButton::toggled, this, [this](bool checked) {
        lastUsedShuffle = checked;
        saveSettings();
    });

    // Number of Questions
    QLabel *numLabel = new QLabel("Questions:", topArea);
    numLabel->setStyleSheet("color: #bdc3c7; font-size: 15px; font-weight: bold; padding: 14px 4px;");

    numQuestionsSpinBox = new QSpinBox(topArea);
    numQuestionsSpinBox->setMinimum(1);
    numQuestionsSpinBox->setMaximum(9999);
    numQuestionsSpinBox->setValue(cardCount);
    numQuestionsSpinBox->setFixedWidth(88);
    numQuestionsSpinBox->setStyleSheet(R"(
        QSpinBox {
            background-color: #2c3e50;
            color: white;
            border: 2px solid #455a6f;
            border-radius: 10px;
            padding: 6px 30px 6px 8px;
            font-size: 15px;
            font-weight: bold;
        }
        QSpinBox:hover {
            border: 2px solid #3498db;
        }

        /* Arrow buttons */
        QSpinBox::up-button {
            width: 25px;
            background-color: #34495e;
            border: none;
            border-top-right-radius: 5px;
        }
        QSpinBox::down-button {
            width: 25px;
            background-color: #34495e;
            border: none;
            border-bottom-right-radius: 5px;
        }
        QSpinBox::up-button:hover, QSpinBox::down-button:hover {
            background-color: #3498db;
        }

        /* === YOUR ARROWS (solid sides) === */
        QSpinBox::up-arrow {
            border-left: 6px solid #34495e;
            border-right: 6px solid #34495e;
            border-bottom: 8px solid #bdc3c7;
            width: 0px;
            height: 0px;
        }
        QSpinBox::down-arrow {
            border-left: 6px solid #34495e;
            border-right: 6px solid #34495e;
            border-top: 8px solid #bdc3c7;
            width: 0px;
            height: 0px;
        }

        /* Hover color override — this works reliably */
        QSpinBox::up-arrow:hover {
            border-left: 6px solid #3498db;
            border-right: 6px solid #3498db;
        }
        QSpinBox::down-arrow:hover {
            border-left: 6px solid #3498db;
            border-right: 6px solid #3498db;
        }
    )");

    quizRow->addWidget(startQuizButton);
    quizRow->addWidget(quizTypeBtn);
    quizRow->addWidget(shuffleButton);
    quizRow->addWidget(numLabel);
    quizRow->addWidget(numQuestionsSpinBox);
    quizRow->addStretch();

    topVL->addLayout(quizRow);
    topVL->addSpacing(16);
    deckLayout->addWidget(topArea);

    // Flashcard Section
    QWidget *bottomArea = new QWidget();
    bottomArea->setStyleSheet("background-color: #2c3e50; border-radius: 8px;");
    QVBoxLayout *bottomL = new QVBoxLayout(bottomArea);
    bottomL->setSpacing(10);
    bottomL->setContentsMargins(5, 16, 5, 0);

    QHBoxLayout *headerL = new QHBoxLayout();
    headerL->setSpacing(12);
    headerL->setContentsMargins(12, 0, 12, 0);
    headerL->addSpacing(50);
    QLabel *qLabel = new QLabel("Question", bottomArea);
    QLabel *aLabel = new QLabel("Answer", bottomArea);
    qLabel->setStyleSheet("font-weight: bold; font-size: 17px; color: white;");
    aLabel->setStyleSheet("font-weight: bold; font-size: 17px; color: white;");
    qLabel->setAlignment(Qt::AlignCenter);
    aLabel->setAlignment(Qt::AlignCenter);
    QPushButton *addCardBtn = new QPushButton("+", bottomArea);
    addCardBtn->setFixedSize(38, 38);
    addCardBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #27ae60;
            color: white;
            border: none;
            border-radius: 10px;
            font-size: 26px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #27ae60;
            border: 2px solid white;
        }
    )");
    connect(addCardBtn, &QPushButton::clicked, this, [this]() { addCardRow(); });
    headerL->addWidget(qLabel, 1);
    headerL->addWidget(aLabel, 1);
    headerL->addWidget(addCardBtn);
    bottomL->addLayout(headerL);

    QScrollArea *scrollArea = new QScrollArea(bottomArea);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet("background-color: transparent; border: none;");
    scrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    cardContainer = new QWidget(scrollArea);
    cardContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    cardContainer->setStyleSheet("background-color: transparent;");
    cardRowsLayout = new QVBoxLayout(cardContainer);
    cardRowsLayout->setSpacing(12);
    cardRowsLayout->setContentsMargins(8, 8, 8, 8);
    cardRowsLayout->setAlignment(Qt::AlignTop);
    scrollArea->setWidget(cardContainer);
    bottomL->addWidget(scrollArea, 1);
    deckLayout->addWidget(bottomArea, 1);

    mainContentLayout->addWidget(currentDeckContainer);
    loadCardsForCurrentDeck();
    deckIsDirty = false;
    updateSaveButtonState();
    updateToolbarActions();
}

QList<QTreeWidgetItem*> MainWindow::collectDecksRecursive(QTreeWidgetItem* item) const
{
    QList<QTreeWidgetItem*> decks;
    if (!item) return decks;
    if (item->data(0, Qt::UserRole).toString() == "deck") {
        decks << item;
        return decks;
    }
    for (int i = 0; i < item->childCount(); ++i) {
        decks << collectDecksRecursive(item->child(i));
    }
    return decks;
}

QList<QPair<QString,QString>> MainWindow::getAllLibraryCards() const
{
    QList<QPair<QString,QString>> allCards;
    QList<QTreeWidgetItem*> allDecks = collectDecksRecursive(deckTree->invisibleRootItem());

    for (QTreeWidgetItem *deck : allDecks) {
        QJsonArray cardsJson = deck->data(0, Qt::UserRole + 1).toJsonArray();
        for (const QJsonValue &val : cardsJson) {
            QJsonObject obj = val.toObject();
            QString front = obj["front"].toString();
            QString back  = obj["back"].toString();

            if (!isCardCompletelyEmpty(front, back)) {
                allCards.append(qMakePair(front, back));
            }
        }
    }
    return allCards;
}

QList<QPair<QString,QString>> MainWindow::getAllCardsInFolder(QTreeWidgetItem* folder) const
{
    QList<QPair<QString,QString>> allCards;
    if (!folder) return allCards;

    QList<QTreeWidgetItem*> decksInFolder = collectDecksRecursive(folder);

    for (QTreeWidgetItem *deck : decksInFolder) {
        QJsonArray cardsJson = deck->data(0, Qt::UserRole + 1).toJsonArray();
        for (const QJsonValue &val : cardsJson) {
            QJsonObject obj = val.toObject();
            QString front = obj["front"].toString();
            QString back  = obj["back"].toString();

            if (!isCardCompletelyEmpty(front, back)) {
                allCards.append(qMakePair(front, back));
            }
        }
    }
    return allCards;
}

bool MainWindow::isCardCompletelyEmpty(const QString &front, const QString &back) const
{
    return front.trimmed().isEmpty() && back.trimmed().isEmpty();
}

QPushButton* MainWindow::createDeckCard(QTreeWidgetItem* deckItem)
{
    if (!deckItem) return nullptr;
    int mastery = getDeckAverageMastery(deckItem);
    QPushButton* card = new QPushButton();
    card->setFixedSize(210, 100);
    card->setCursor(Qt::PointingHandCursor);
    card->setFlat(true);
    card->setStyleSheet(R"(
        QPushButton {
            background-color: #34495e;
            border-radius: 12px;
            border: 2px solid #455a6f;
        }
        QPushButton:hover {
            border: 2px solid #3498db;
        }
    )");
    QHBoxLayout* mainL = new QHBoxLayout(card);
    mainL->setSpacing(12);
    mainL->setContentsMargins(16, 8, 12, 8);

    QVBoxLayout* textL = new QVBoxLayout();
    textL->setSpacing(4);
    QLabel* name = new QLabel(deckItem->text(0), card);
    name->setWordWrap(true);
    name->setStyleSheet("background-color: #34495e; font-size: 15px; font-weight: bold; color: white;");
    int count = deckItem->data(0, Qt::UserRole + 1).toJsonArray().size();
    QLabel* countLbl = new QLabel(QString("%1 cards").arg(count), card);
    countLbl->setStyleSheet("background-color: #34495e; font-size: 13px; color: #95a5a6;");
    textL->addWidget(name);
    textL->addWidget(countLbl);
    textL->addStretch();

    MasteryRadial* radial = new MasteryRadial(card, 46);
    radial->setValue(mastery);
    radial->setToolTip(QString("Mastery: %1%").arg(mastery));
    radial->setStyleSheet("background-color: #34495e;");
    mainL->addLayout(textL, 1);
    mainL->addWidget(radial);

    connect(card, &QPushButton::clicked, this, [this, deckItem]() {
        deckTree->setCurrentItem(deckItem);
    });
    return card;
}

QWidget* MainWindow::createHorizontalDeckRow(const QString& title, const QList<QTreeWidgetItem*>& decks, const QString& accentColor)
{
    if (decks.isEmpty()) return nullptr;
    QWidget* rowWidget = new QWidget();
    rowWidget->setStyleSheet("border: transparent;");
    QVBoxLayout* v = new QVBoxLayout(rowWidget);
    v->setSpacing(10);

    QHBoxLayout* titleLayout = new QHBoxLayout();
    QWidget* colorBar = new QWidget();
    colorBar->setFixedSize(5, 26);
    colorBar->setStyleSheet(QString("background-color: %1; border-radius: 8px;").arg(accentColor));
    titleLayout->addWidget(colorBar);
    QLabel* rowTitle = new QLabel(title, rowWidget);
    rowTitle->setStyleSheet(QString("border: transparent; font-size: 21px; font-weight: bold; color: %1;").arg(accentColor));
    titleLayout->addWidget(rowTitle);
    titleLayout->addStretch(1);
    v->addLayout(titleLayout);

    QScrollArea* scroll = new QScrollArea(rowWidget);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setWidgetResizable(true);
    scroll->setFixedHeight(130);
    scroll->setStyleSheet("QScrollArea { background: transparent; border: none; }");
    QWidget* inner = new QWidget();
    QHBoxLayout* h = new QHBoxLayout(inner);
    h->setSpacing(20);
    h->setContentsMargins(0, 0, 0, 0);
    h->setAlignment(Qt::AlignLeft);
    for (QTreeWidgetItem* d : decks) {
        if (QPushButton* c = createDeckCard(d))
            h->addWidget(c);
    }
    scroll->setWidget(inner);
    v->addWidget(scroll);
    return rowWidget;
}

// Home Page
void MainWindow::showHomePage()
{
    resetMainContent();

    QTreeWidgetItem *selectedFolder = nullptr;
    QTreeWidgetItem *curr = deckTree ? deckTree->currentItem() : nullptr;
    if (curr && curr->data(0, Qt::UserRole).toString() == "folder") {
        selectedFolder = curr;
    }

    QWidget* content = new QWidget();
    content->setStyleSheet("background-color: #4a5259;");
    QVBoxLayout* vl = new QVBoxLayout(content);
    vl->setSpacing(20);
    vl->setContentsMargins(16, 16, 16, 40);

    // Stats Banner
    QWidget* statsBanner = new QWidget(content);
    statsBanner->setStyleSheet("background-color: #2c3e50; border-radius: 8px;");
    QVBoxLayout* bannerVL = new QVBoxLayout(statsBanner);
    bannerVL->setSpacing(0);
    QLabel* overviewTitle = new QLabel("Overview", content);
    overviewTitle->setStyleSheet("font-size: 26px; font-weight: bold; color: white;");
    overviewTitle->setAlignment(Qt::AlignCenter);
    bannerVL->addWidget(overviewTitle);

    QHBoxLayout* statsL = new QHBoxLayout();
    statsL->setSpacing(40);
    QLabel* decksLabel = new QLabel(QString("Decks\n%1").arg(getTotalDecks()), content);
    decksLabel->setStyleSheet("font-size: 18px; color: #bdc3c7; font-weight: bold;");
    decksLabel->setAlignment(Qt::AlignCenter);
    decksLabel->setContentsMargins(15, 0, 0, 0);
    QLabel* cardsLabel = new QLabel(QString("Cards\n%1").arg(getTotalCards()), content);
    cardsLabel->setStyleSheet("font-size: 18px; color: #bdc3c7; font-weight: bold;");
    cardsLabel->setAlignment(Qt::AlignCenter);
    QLabel* streakLabel = new QLabel(QString("Daily Streak\n%1 🔥").arg(dailyStreak), content);
    streakLabel->setStyleSheet("font-size: 18px; color: #e74c3c; font-weight: bold;");
    streakLabel->setAlignment(Qt::AlignCenter);

    QHBoxLayout* masteryHL = new QHBoxLayout();
    masteryHL->setSpacing(12);
    masteryHL->setAlignment(Qt::AlignCenter);
    masteryHL->setContentsMargins(0, 0, 15, 0);
    QLabel* masteryLabel = new QLabel("Overall\nMastery", content);
    masteryLabel->setStyleSheet("font-size: 18px; color: #bdc3c7; font-weight: bold;");
    masteryLabel->setAlignment(Qt::AlignCenter);
    MasteryRadial* globalRadial = new MasteryRadial(content, 72);
    globalRadial->setValue(getOverallMastery());
    masteryHL->addWidget(masteryLabel);
    masteryHL->addWidget(globalRadial);

    statsL->addWidget(decksLabel);
    statsL->addWidget(cardsLabel);
    statsL->addWidget(streakLabel);
    statsL->addStretch();
    statsL->addLayout(masteryHL);
    bannerVL->addLayout(statsL);
    vl->addWidget(statsBanner);

    // Quick Action Buttons
    QHBoxLayout *actionRow = new QHBoxLayout();
    actionRow->setSpacing(12);
    actionRow->setAlignment(Qt::AlignCenter);

    QLabel *quizFromLabel = new QLabel("Start Quiz from:", content);
    quizFromLabel->setStyleSheet("color: #bdc3c7; font-size: 15px; font-weight: bold; padding: 14px 8px 14px 0;");
    actionRow->addWidget(quizFromLabel);

    // Random Deck Quiz
    randomDeckBtn = new QPushButton("🎲 Random Deck", content);
    randomDeckBtn->setToolTip("Starts a 10 question quiz from a random deck in your library");
    randomDeckBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #3498db;
            color: white;
            padding: 14px 24px;
            font-size: 15px;
            font-weight: bold;
            border-radius: 10px;
            border: none;
            min-width: 140px;
            max-width: 140px;
            max-height: 21px;
        }
        QPushButton:hover {
            border: 2px solid white;
        }
        QPushButton:disabled {
            background-color: #7f8c8d;
            color: #bdc3c7;
        }
    )");
    connect(randomDeckBtn, &QPushButton::clicked, this, &MainWindow::startRandomDeckQuiz);

    // Whole Library Quiz
    libraryBtn = new QPushButton("📚 Whole Library", content);
    libraryBtn->setToolTip("Starts a quiz with 10 random flashcards taken from your entire library");
    libraryBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #3498db;
            color: white;
            padding: 14px 24px;
            font-size: 15px;
            font-weight: bold;
            border-radius: 10px;
            border: none;
            min-width: 140px;
            max-width: 140px;
            max-height: 21px;
        }
        QPushButton:hover {
            border: 2px solid white;
        }
        QPushButton:disabled {
            background-color: #7f8c8d;
            color: #bdc3c7;
        }
    )");
    connect(libraryBtn, &QPushButton::clicked, this, &MainWindow::startLibraryQuiz);

    // Current Folder Quiz
    folderBtn = nullptr;
    QTreeWidgetItem *selectedItem = deckTree ? deckTree->currentItem() : nullptr;
    if (selectedItem && selectedItem->data(0, Qt::UserRole).toString() == "folder") {
        folderBtn = new QPushButton("📁 This Folder", content);
        folderBtn->setToolTip("Starts a quiz with 10 random flashcards taken from all the decks of this selected folder");
        folderBtn->setStyleSheet(R"(
            QPushButton {
                background-color: #3498db;
                color: white;
                padding: 14px 24px;
                font-size: 15px;
                font-weight: bold;
                border-radius: 10px;
                border: none;
                min-width: 140px;
                max-width: 140px;
                max-height: 21px;
            }
            QPushButton:hover {
                border: 2px solid white;
            }
            QPushButton:disabled {
                background-color: #7f8c8d;
                color: #bdc3c7;
            }
        )");
        connect(folderBtn, &QPushButton::clicked, this, &MainWindow::startFolderQuiz);
    }

    // Quiz Type Button
    styleToggleBtn = new QPushButton(content);
    styleToggleBtn->setCheckable(true);
    styleToggleBtn->setChecked(lastUsedFlashcardMode);
    styleToggleBtn->setText(lastUsedFlashcardMode ? "Flashcard Style" : "Multiple Choice");
    styleToggleBtn->setToolTip("Switch between Flashcard and Multiple Choice quiz mode");
    styleToggleBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #27ae60;
            color: white;
            padding: 14px 24px;
            font-size: 15px;
            font-weight: bold;
            border-radius: 10px;
            min-width: 140px;
            max-width: 140px;
            max-height: 21px;
        }
        QPushButton:hover {
            border: 2px solid white;
        }
    )");
    connect(styleToggleBtn, &QPushButton::clicked, this, [this](bool checked) {
        lastUsedFlashcardMode = checked;
        styleToggleBtn->setText(checked ? "Flashcard Style" : "Multiple Choice");
        saveSettings();
    });

    actionRow->addWidget(randomDeckBtn);
    actionRow->addWidget(libraryBtn);
    if (folderBtn) actionRow->addWidget(folderBtn);
    actionRow->addStretch(1);
    actionRow->addWidget(styleToggleBtn);

    vl->addLayout(actionRow);

    bool hasAnyCards = (getTotalCards() > 0);
    randomDeckBtn->setEnabled(hasAnyCards);
    libraryBtn->setEnabled(hasAnyCards);
    if (folderBtn) folderBtn->setEnabled(hasAnyCards);

    auto createBoxedRow = [&](const QString& title, const QList<QTreeWidgetItem*>& decks, const QString& accentColor) -> QWidget* {
        if (decks.isEmpty()) return nullptr;
        QWidget* box = new QWidget();
        box->setStyleSheet("background-color: #2c3e50; border-radius: 8px;");
        box->setContentsMargins(15, 15, 0, 0);
        QVBoxLayout* boxL = new QVBoxLayout(box);
        if (QWidget* r = createHorizontalDeckRow(title, decks, accentColor))
            boxL->addWidget(r);
        return box;
    };

    QList<QTreeWidgetItem*> allDecks;
    if (selectedFolder) {
        allDecks = collectDecksRecursive(selectedFolder);
    } else {
        allDecks = collectDecksRecursive(deckTree->invisibleRootItem());
    }

    std::sort(allDecks.begin(), allDecks.end(), [](QTreeWidgetItem* a, QTreeWidgetItem* b){
        return a->data(0, Qt::UserRole + 2).toString() > b->data(0, Qt::UserRole + 2).toString();
    });

    // Continue Where You Left Off
    if (QWidget* r = createBoxedRow("Continue Where You Left Off", allDecks.mid(0, qMin(5, allDecks.size())), "#1abc9c"))
        vl->addWidget(r);

    // Ready for Review
    QList<QTreeWidgetItem*> reviewDecks;
    QDate today = QDate::currentDate();

    for (auto d : allDecks) {
        int mastery = getDeckAverageMastery(d);
        if (mastery == 0 || mastery >= 100) continue;

        QString lastQuizStr = d->data(0, Qt::UserRole + 2).toString().trimmed();

        if (lastQuizStr.isEmpty()) {
            reviewDecks << d;
        } else {
            QDateTime lastQuiz = QDateTime::fromString(lastQuizStr, Qt::ISODate);
            if (!lastQuiz.isValid() || lastQuiz.date() != today) {
                reviewDecks << d;
            }
        }
    }

    std::sort(reviewDecks.begin(), reviewDecks.end(), [this](QTreeWidgetItem* a, QTreeWidgetItem* b) {
        return getDeckAverageMastery(a) > getDeckAverageMastery(b);
    });

    if (reviewDecks.size() > 5)
        reviewDecks = reviewDecks.mid(0, 5);

    if (QWidget* r = createBoxedRow("Ready for Review", reviewDecks, "#e74c3c"))
        vl->addWidget(r);

    if (selectedFolder) {
        QString folderName = selectedFolder->text(0);
        if (QWidget* r = createBoxedRow("Decks in " + folderName, allDecks, "#3498db"))
            vl->addWidget(r);
    } else {
        for (int i = 0; i < deckTree->topLevelItemCount(); ++i) {
            QTreeWidgetItem* top = deckTree->topLevelItem(i);
            if (top->data(0, Qt::UserRole).toString() == "folder") {
                auto folderDecks = collectDecksRecursive(top);
                if (QWidget* r = createBoxedRow(top->text(0), folderDecks, "#3498db"))
                    vl->addWidget(r);
            }
        }

        QList<QTreeWidgetItem*> folderlessDecks;
        for (int i = 0; i < deckTree->topLevelItemCount(); ++i) {
            QTreeWidgetItem* item = deckTree->topLevelItem(i);
            if (item->data(0, Qt::UserRole).toString() == "deck") {
                folderlessDecks << item;
            }
        }

        if (!folderlessDecks.isEmpty()) {
            if (QWidget* r = createBoxedRow("Folderless Decks", folderlessDecks, "#3498db"))
                vl->addWidget(r);
        }
    }

    if (allDecks.isEmpty()) {
        QWidget* empty = new QWidget(content);
        QVBoxLayout* eL = new QVBoxLayout(empty);
        eL->setAlignment(Qt::AlignCenter);
        eL->setSpacing(20);
        QLabel* emoji = new QLabel("📚", empty);
        emoji->setStyleSheet("font-size: 120px;");
        emoji->setAlignment(Qt::AlignCenter);
        QLabel* msg = new QLabel("No decks yet!\nCreate a deck here.", empty);
        msg->setStyleSheet("font-size: 22px; color: #bdc3c7; text-align: center;");
        msg->setAlignment(Qt::AlignCenter);
        QPushButton* createBtn = new QPushButton("＋ Create Deck", empty);
        createBtn->setStyleSheet(R"(
            QPushButton {
                background-color: #27ae60;
                color: white;
                padding: 16px 40px;
                font-size: 18px;
                border-radius: 12px;
                font-weight: bold;
            }
        )");
        connect(createBtn, &QPushButton::clicked, this, &MainWindow::addNewDeckFromButton);
        eL->addWidget(emoji);
        eL->addWidget(msg);
        eL->addWidget(createBtn);
        vl->addWidget(empty);
    }

    vl->addStretch(1);
    QScrollArea* mainScroll = new QScrollArea();
    mainScroll->setWidgetResizable(true);
    mainScroll->setWidget(content);
    mainScroll->setStyleSheet("QScrollArea { border: none; background: #4a5259; }");
    mainScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    mainScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    mainContentLayout->addWidget(mainScroll);
}

void MainWindow::onItemChanged(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);
    if (!item) return;
    saveDecks();
    if (item == currentDeckItem) {
        showDeckContent(item);
    }
}

QWidget* MainWindow::createCardRow(const QString &front, const QString &back, int mastery)
{
    QWidget *rowWidget = new QWidget();
    rowWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    rowWidget->setStyleSheet("background-color: #34495e; border-radius: 12px; border: 2px solid #2c3e50;");
    QHBoxLayout *rowL = new QHBoxLayout(rowWidget);
    rowL->setSpacing(12);
    rowL->setContentsMargins(12, 16, 12, 16);

    QLabel *grip = new QLabel("⋮⋮", rowWidget);
    grip->setFixedWidth(32);
    grip->setAlignment(Qt::AlignCenter);
    grip->setStyleSheet("QLabel {background-color: #2c3e50; color: #95a5a6; font-size: 28px; font-weight: bold; border: 2px solid #2c3e50;} "
                        "QLabel:hover {border: 2px solid #3498db;}");
    grip->setCursor(Qt::OpenHandCursor);
    grip->setToolTip("Click and drag to reorder");
    grip->setProperty("rowWidget", QVariant::fromValue(rowWidget));
    grip->installEventFilter(this);

    QWidget *radialContainer = new QWidget(rowWidget);
    radialContainer->setFixedWidth(56);
    radialContainer->setStyleSheet("border: transparent;");
    QVBoxLayout *radialL = new QVBoxLayout(radialContainer);
    radialL->setContentsMargins(0, 15, 0, 0);
    radialL->setSpacing(0);
    radialL->setAlignment(Qt::AlignTop);
    MasteryRadial *radial = new MasteryRadial(radialContainer);
    radial->setValue(mastery);
    radial->setToolTip(QString("Mastery: %1%").arg(mastery));
    radialL->addWidget(radial);

    QTextEdit *qEdit = new QTextEdit(front, rowWidget);
    QTextEdit *aEdit = new QTextEdit(back, rowWidget);
    QFont textFont("Segoe UI", 14);
    qEdit->setFont(textFont);
    aEdit->setFont(textFont);
    qEdit->setPlaceholderText("Question Text");
    aEdit->setPlaceholderText("Answer Text");
    qEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    aEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    qEdit->setMinimumWidth(180);
    aEdit->setMinimumWidth(180);
    qEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    aEdit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    qEdit->setAlignment(Qt::AlignCenter);
    aEdit->setAlignment(Qt::AlignCenter);
    QTextOption centerOption;
    centerOption.setAlignment(Qt::AlignCenter);
    qEdit->document()->setDefaultTextOption(centerOption);
    aEdit->document()->setDefaultTextOption(centerOption);
    QString editStyle = R"(
        QTextEdit { background-color: #2c3e50; color: white; border: none;
                    border-radius: 6px; padding: 12px; text-align: center; }
        QTextEdit:placeholder { text-align: center; color: #95a5a6; }
    )";
    qEdit->setStyleSheet(editStyle);
    aEdit->setStyleSheet(editStyle);

    QWidget *controlsPanel = new QWidget(rowWidget);
    controlsPanel->setFixedWidth(36);
    controlsPanel->setStyleSheet("background: transparent; border: none;");
    QVBoxLayout *controlsL = new QVBoxLayout(controlsPanel);
    controlsL->setSpacing(10);
    controlsL->setContentsMargins(0, 0, 0, 0);
    controlsL->setAlignment(Qt::AlignTop);
    QPushButton *dupBtn = new QPushButton();
    dupBtn->setIcon(QIcon::fromTheme("edit-copy"));
    dupBtn->setFixedSize(36,36);
    dupBtn->setStyleSheet("QPushButton {background: #2c3e50; border: 1px solid #2c3e50; border-radius: 8px;} "
                          "QPushButton:hover {background: #2c3e50; border: 2px solid #3498db;}");
    QPushButton *delBtn = new QPushButton();
    delBtn->setIcon(QIcon::fromTheme("edit-delete"));
    delBtn->setFixedSize(36,36);
    delBtn->setStyleSheet("QPushButton {background: #2c3e50; border: 1px solid #2c3e50; border-radius: 8px;} "
                          "QPushButton:hover {background: #2c3e50; border: 2px solid #3498db;}");
    controlsL->addWidget(dupBtn);
    controlsL->addWidget(delBtn);
    controlsL->addStretch();

    rowL->addWidget(grip);
    rowL->addWidget(qEdit, 1, Qt::AlignTop);
    rowL->addWidget(aEdit, 1, Qt::AlignTop);
    rowL->addWidget(radialContainer, 0, Qt::AlignTop);
    rowL->addWidget(controlsPanel);

    rowWidget->setProperty("mastery", mastery);
    connect(dupBtn, &QPushButton::clicked, this, [this, rowWidget]() { duplicateCardRow(rowWidget); });
    connect(delBtn, &QPushButton::clicked, this, [this, rowWidget]() {
        if (QMessageBox::question(this, "Delete Flashcard", "Delete this flashcard permanently?\n\nThis cannot be undone.", QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
            removeCardRow(rowWidget);
    });
    connect(qEdit, &QTextEdit::textChanged, this, [this, qEdit]() { resizeRowToContent(qEdit); markDeckAsDirty(); });
    connect(aEdit, &QTextEdit::textChanged, this, [this, aEdit]() { resizeRowToContent(aEdit); markDeckAsDirty(); });

    rowWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(rowWidget, &QWidget::customContextMenuRequested, this,
            [this, rowWidget](const QPoint &pos) {
                QMenu menu(this);
                QAction *swapAction = menu.addAction(QIcon::fromTheme("object-flip-horizontal"),
                                                     "Swap Front ↔ Back");
                connect(swapAction, &QAction::triggered, this,
                        [this, rowWidget]() { swapCardFrontAndBack(rowWidget); });

                menu.exec(rowWidget->mapToGlobal(pos));
            });

    return rowWidget;
}

void MainWindow::swapCardFrontAndBack(QWidget *rowWidget)
{
    if (!rowWidget) return;

    QHBoxLayout *h = qobject_cast<QHBoxLayout*>(rowWidget->layout());
    if (!h) return;

    QTextEdit *frontEdit = qobject_cast<QTextEdit*>(h->itemAt(1)->widget());
    QTextEdit *backEdit  = qobject_cast<QTextEdit*>(h->itemAt(2)->widget());

    if (!frontEdit || !backEdit) return;

    QString front = frontEdit->toPlainText();
    QString back  = backEdit->toPlainText();

    frontEdit->setPlainText(back);
    backEdit->setPlainText(front);

    markDeckAsDirty();
    resizeRowToContent(frontEdit);  // update heights
}

void MainWindow::addCardRow(const QString &front, const QString &back, int mastery)
{
    if (!cardRowsLayout) return;
    cardRowsLayout->addWidget(createCardRow(front, back, mastery));
    QTimer::singleShot(10, this, &MainWindow::updateNumQuestionsRange);
}

void MainWindow::resizeRowToContent(QTextEdit *edit)
{
    if (!edit || !edit->document()) return;
    QWidget *rowWidget = qobject_cast<QWidget*>(edit->parentWidget());
    if (!rowWidget) return;
    QHBoxLayout *rowL = qobject_cast<QHBoxLayout*>(rowWidget->layout());
    if (!rowL) return;
    QTextEdit *qEdit = qobject_cast<QTextEdit*>(rowL->itemAt(1)->widget());
    QTextEdit *aEdit = qobject_cast<QTextEdit*>(rowL->itemAt(2)->widget());
    if (!qEdit || !aEdit) return;
    int viewportW = edit->viewport()->width();
    if (viewportW < 80) return;
    qEdit->document()->setTextWidth(viewportW);
    aEdit->document()->setTextWidth(viewportW);
    int qHeight = static_cast<int>(qEdit->document()->size().height()) + 24;
    int aHeight = static_cast<int>(aEdit->document()->size().height()) + 24;
    int finalTextHeight = qMax(qMax(qHeight, aHeight), 80);
    if (qAbs(qEdit->height() - finalTextHeight) > 3)
        qEdit->setFixedHeight(finalTextHeight);
    if (qAbs(aEdit->height() - finalTextHeight) > 3)
        aEdit->setFixedHeight(finalTextHeight);
    int rowHeight = finalTextHeight + 32;
    if (qAbs(rowWidget->height() - rowHeight) > 3) {
        rowWidget->setFixedHeight(rowHeight);
    }
}

void MainWindow::updateAllCardHeights()
{
    if (inQuizMode || !cardRowsLayout) return;
    for (int i = 0; i < cardRowsLayout->count(); ++i) {
        QLayoutItem *item = cardRowsLayout->itemAt(i);
        if (!item || !item->widget()) continue;
        QWidget *row = item->widget();
        QHBoxLayout *rowL = qobject_cast<QHBoxLayout*>(row->layout());
        if (!rowL) continue;
        QTextEdit *qEdit = qobject_cast<QTextEdit*>(rowL->itemAt(1)->widget());
        QTextEdit *aEdit = qobject_cast<QTextEdit*>(rowL->itemAt(2)->widget());
        if (qEdit) resizeRowToContent(qEdit);
        if (aEdit) resizeRowToContent(aEdit);
    }
}

void MainWindow::loadCardsForCurrentDeck()
{
    if (!currentDeckItem || !cardRowsLayout) return;
    QLayoutItem *item;
    while ((item = cardRowsLayout->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }
    QJsonArray cards = currentDeckItem->data(0, Qt::UserRole + 1).toJsonArray();
    for (const QJsonValue &val : cards) {
        QJsonObject obj = val.toObject();
        int m = obj.value("mastery").toInt(0);
        addCardRow(obj["front"].toString(), obj["back"].toString(), m);
    }
    if (cards.isEmpty()) {
        addCardRow("", "", 0);
    }
    cardRowsLayout->invalidate();
    cardContainer->updateGeometry();
    QTimer::singleShot(30, this, &MainWindow::updateAllCardHeights);
    deckIsDirty = false;
    updateSaveButtonState();
    updateNumQuestionsRange();
}

int MainWindow::getDeckAverageMastery(QTreeWidgetItem *deckItem) const
{
    if (!deckItem) return 0;
    QJsonArray cards = deckItem->data(0, Qt::UserRole + 1).toJsonArray();
    if (cards.isEmpty()) return 0;
    int sum = 0;
    for (const QJsonValue &val : cards) {
        sum += val.toObject().value("mastery").toInt(0);
    }
    return sum / cards.size();
}

void MainWindow::removeCardRow(QWidget *rowWidget)
{
    if (!rowWidget || !cardRowsLayout) return;
    markDeckAsDirty();
    cardRowsLayout->removeWidget(rowWidget);
    rowWidget->deleteLater();
    updateNumQuestionsRange();
}

void MainWindow::saveCurrentDeckCards()
{
    if (!currentDeckItem || !cardRowsLayout) return;
    QJsonArray cardsArray;
    for (int i = 0; i < cardRowsLayout->count(); ++i) {
        QLayoutItem *item = cardRowsLayout->itemAt(i);
        if (!item || !item->widget()) continue;
        QWidget *row = item->widget();
        QHBoxLayout *h = qobject_cast<QHBoxLayout*>(row->layout());
        if (!h) continue;
        QTextEdit *frontEdit = qobject_cast<QTextEdit*>(h->itemAt(1)->widget());
        QTextEdit *backEdit = qobject_cast<QTextEdit*>(h->itemAt(2)->widget());
        if (frontEdit && backEdit) {
            QString front = frontEdit->toPlainText().trimmed();
            QString back = backEdit->toPlainText().trimmed();
            if (front.isEmpty() && back.isEmpty()) continue;
            QJsonObject cardObj;
            cardObj["front"] = front;
            cardObj["back"] = back;
            cardObj["mastery"] = row->property("mastery").toInt();
            cardsArray.append(cardObj);
        }
    }
    currentDeckItem->setData(0, Qt::UserRole + 1, cardsArray);
    saveDecks();
    deckIsDirty = false;
    updateSaveButtonState();
    updateNumQuestionsRange();
    if (deckMasteryRadial && currentDeckItem) {
        deckMasteryRadial->setValue(getDeckAverageMastery(currentDeckItem));
    }
}

void MainWindow::applyMasteryFromQuiz()
{
    if (!currentDeckItem) return;
    QJsonArray cards = currentDeckItem->data(0, Qt::UserRole + 1).toJsonArray();
    for (const auto &r : quizResults) {
        if (r.second == -1) continue;
        QString front = r.first.first;
        for (int j = 0; j < cards.size(); ++j) {
            QJsonObject obj = cards[j].toObject();
            if (obj["front"].toString() == front) {
                int m = obj.value("mastery").toInt(0);
                m += (r.second == 1) ? 8 : -6;
                obj["mastery"] = std::clamp(m, 0, 100);
                cards[j] = obj;
                break;
            }
        }
    }
    currentDeckItem->setData(0, Qt::UserRole + 1, cards);
    saveDecks();
    if (deckMasteryRadial && currentDeckItem) {
        deckMasteryRadial->setValue(getDeckAverageMastery(currentDeckItem));
    }
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (QLabel *grip = qobject_cast<QLabel*>(obj)) {
        QWidget *rowWidget = grip->property("rowWidget").value<QWidget*>();
        if (!rowWidget || !cardRowsLayout || !cardContainer) return false;
        if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent *me = static_cast<QMouseEvent*>(event);
            if (me->button() == Qt::LeftButton) {
                draggedCard = rowWidget;
                dragOffset = me->pos();
                grip->setCursor(Qt::ClosedHandCursor);
                int oldIndex = cardRowsLayout->indexOf(draggedCard);
                cardRowsLayout->removeWidget(draggedCard);
                dropPlaceholder = new QWidget(cardContainer);
                dropPlaceholder->setFixedHeight(draggedCard->height());
                dropPlaceholder->setStyleSheet("background: qlineargradient(x1:0, y1:0, x2:1, y2:0, "
                                               "stop:0 #3498db, stop:0.5 #2980b9, stop:1 #3498db); "
                                               "border-radius: 4px; margin: 4px 8px;");
                cardRowsLayout->insertWidget(oldIndex, dropPlaceholder);
                draggedCard->setParent(cardContainer);
                draggedCard->raise();
                QGraphicsOpacityEffect *opacity = new QGraphicsOpacityEffect(draggedCard);
                opacity->setOpacity(0.78);
                draggedCard->setGraphicsEffect(opacity);
                return true;
            }
        }
        else if (event->type() == QEvent::MouseMove && draggedCard) {
            QMouseEvent *me = static_cast<QMouseEvent*>(event);
            QPoint localPos = cardContainer->mapFromGlobal(me->globalPosition().toPoint());
            draggedCard->move(localPos - dragOffset);
            int targetIndex = cardRowsLayout->count() - 1;
            for (int i = 0; i < cardRowsLayout->count(); ++i) {
                QWidget *w = cardRowsLayout->itemAt(i)->widget();
                if (w && w != draggedCard && localPos.y() < w->y() + w->height() / 2) {
                    targetIndex = i;
                    break;
                }
            }
            if (dropPlaceholder && cardRowsLayout->indexOf(dropPlaceholder) != targetIndex) {
                cardRowsLayout->removeWidget(dropPlaceholder);
                cardRowsLayout->insertWidget(targetIndex, dropPlaceholder);
                cardContainer->updateGeometry();
            }
            return true;
        }
        else if (event->type() == QEvent::MouseButtonRelease && draggedCard) {
            grip->setCursor(Qt::OpenHandCursor);
            if (dropPlaceholder) {
                int finalIndex = cardRowsLayout->indexOf(dropPlaceholder);
                cardRowsLayout->removeWidget(dropPlaceholder);
                dropPlaceholder->deleteLater();
                dropPlaceholder = nullptr;
                draggedCard->setGraphicsEffect(nullptr);
                cardRowsLayout->insertWidget(finalIndex, draggedCard);
            }
            draggedCard = nullptr;
            markDeckAsDirty();
            updateAllCardHeights();
            return true;
        }
    }
    if (obj == deckTree->viewport() && event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *me = static_cast<QMouseEvent*>(event);
        if (!deckTree->itemAt(me->pos())) {
            if (inQuizMode) {
                if (!confirmExitQuiz()) {
                    return true;
                }
                endQuiz();
            }

            if (deckTree->currentItem() != nullptr) {
                deckTree->clearSelection();
                deckTree->setCurrentItem(nullptr);
                onDeckSelectionChanged();
            } else {
                deckTree->clearSelection();
            }
            return true;
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::updateAddButtonsState()
{
    QTreeWidgetItem *selected = deckTree->currentItem();
    bool isFolder = (selected && selected->data(0, Qt::UserRole).toString() == "folder");
    bool isRootOrFolder = !selected || isFolder;

    addFolderBtn->setEnabled(true);
    addDeckBtn->setEnabled(isRootOrFolder);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    if (inQuizMode && cardArea) {
        QTimer::singleShot(15, this, [this]() {
            if (frontLabel) {
                if (isFlashcardMode) {
                    adjustCardFontSize(frontLabel, frontLabel->text(), false);
                    if (backLabel && backLabel->isVisible()) {
                        adjustCardFontSize(backLabel, backLabel->text(), false);
                    }
                } else {
                    adjustCardFontSize(frontLabel, frontLabel->text(), true);
                }
            }
        });
    }
}

void MainWindow::updateToolbarActions()
{
    if (isSettingsPage) {
        if (addCardAction) addCardAction->setVisible(false);
        if (saveDeckAction) saveDeckAction->setVisible(false);
        if (renameDeckAction) renameDeckAction->setVisible(false);
        if (duplicateDeckAction) duplicateDeckAction->setVisible(false);
        if (deleteDeckAction) deleteDeckAction->setVisible(false);
        if (resetMasteryAction) resetMasteryAction->setVisible(false);
        if (renameFolderAction) renameFolderAction->setVisible(false);
        if (duplicateFolderFullAction) duplicateFolderFullAction->setVisible(false);
        if (duplicateFolderEmptyAction) duplicateFolderEmptyAction->setVisible(false);
        if (deleteFolderAction) deleteFolderAction->setVisible(false);
        if (deckSeparator1) deckSeparator1->setVisible(false);
        if (deckSeparator2) deckSeparator2->setVisible(false);
        if (folderSeparator1) folderSeparator1->setVisible(false);
        if (folderSeparator2) folderSeparator2->setVisible(false);
        if (endQuizAction) endQuizAction->setVisible(false);
        return;
    }

    if (inQuizMode) {
        endQuizAction->setText("End Quiz");
        endQuizAction->setIcon(QIcon::fromTheme("process-stop", QIcon::fromTheme("dialog-cancel")));
        endQuizAction->setVisible(true);
        if (addCardAction) addCardAction->setVisible(false);
        if (saveDeckAction) saveDeckAction->setVisible(false);
        if (renameDeckAction) renameDeckAction->setVisible(false);
        if (duplicateDeckAction) duplicateDeckAction->setVisible(false);
        if (deleteDeckAction) deleteDeckAction->setVisible(false);
        if (resetMasteryAction) resetMasteryAction->setVisible(false);
        if (renameFolderAction) renameFolderAction->setVisible(false);
        if (duplicateFolderFullAction) duplicateFolderFullAction->setVisible(false);
        if (duplicateFolderEmptyAction) duplicateFolderEmptyAction->setVisible(false);
        if (deleteFolderAction) deleteFolderAction->setVisible(false);
        if (deckSeparator1) deckSeparator1->setVisible(false);
        if (deckSeparator2) deckSeparator2->setVisible(false);
        if (folderSeparator1) folderSeparator1->setVisible(false);
        if (folderSeparator2) folderSeparator2->setVisible(false);
        return;
    }

    if (endQuizAction) endQuizAction->setVisible(false);

    QTreeWidgetItem *item = deckTree ? deckTree->currentItem() : nullptr;
    bool isDeck = item && item->data(0, Qt::UserRole).toString() == "deck";
    bool isFolder = item && item->data(0, Qt::UserRole).toString() == "folder";

    if (addCardAction) addCardAction->setVisible(false);
    if (saveDeckAction) saveDeckAction->setVisible(isDeck);
    if (renameDeckAction) renameDeckAction->setVisible(isDeck);
    if (duplicateDeckAction) duplicateDeckAction->setVisible(isDeck);
    if (resetMasteryAction) resetMasteryAction->setVisible(isDeck);
    if (deleteDeckAction) deleteDeckAction->setVisible(isDeck);
    if (renameFolderAction) renameFolderAction->setVisible(isFolder);
    if (duplicateFolderFullAction) duplicateFolderFullAction->setVisible(isFolder);
    if (duplicateFolderEmptyAction) duplicateFolderEmptyAction->setVisible(isFolder);
    if (deleteFolderAction) deleteFolderAction->setVisible(isFolder);

    if (deckSeparator1) deckSeparator1->setVisible(isDeck);
    if (deckSeparator2) deckSeparator2->setVisible(isDeck);
    if (folderSeparator1) folderSeparator1->setVisible(isFolder);
    if (folderSeparator2) folderSeparator2->setVisible(isFolder);

    updateSaveButtonState();
}

void CustomTreeWidget::dropEvent(QDropEvent *event)
{
    QTreeWidget::dropEvent(event);

    if (MainWindow* mw = qobject_cast<MainWindow*>(window())) {
        mw->saveDecks();
    }
}

void MainWindow::renameCurrentDeck()
{
    ensureSidebarVisible();
    if (!currentDeckItem) return;
    deckTree->editItem(currentDeckItem, 0);
}

void MainWindow::renameCurrentFolder()
{
    ensureSidebarVisible();
    if (deckTree->currentItem()) deckTree->editItem(deckTree->currentItem(), 0);
}

void MainWindow::deleteCurrentDeck()
{
    if (!currentDeckItem) return;

    delete currentDeckItem;
    currentDeckItem = nullptr;
    clearMainContent();
    saveDecks();
}

void MainWindow::deleteCurrentFolder()
{
    QTreeWidgetItem *item = deckTree->currentItem();
    if (!item || item->data(0, Qt::UserRole).toString() != "folder") return;

    delete item;
    currentDeckItem = nullptr;
    saveDecks();
    clearMainContent();
}

void MainWindow::resetDeckMastery()
{
    if (!currentDeckItem) return;

    QJsonArray cards = currentDeckItem->data(0, Qt::UserRole + 1).toJsonArray();
    for (int i = 0; i < cards.size(); ++i) {
        QJsonObject obj = cards[i].toObject();
        obj["mastery"] = 0;
        cards[i] = obj;
    }
    currentDeckItem->setData(0, Qt::UserRole + 1, cards);

    currentDeckItem->setData(0, Qt::UserRole + 2,
                             QDateTime::currentDateTime().toString(Qt::ISODate));

    saveDecks();

    if (deckMasteryRadial && currentDeckItem) {
        deckMasteryRadial->setValue(0);
    }

    showDeckContent(currentDeckItem);
}

QTreeWidgetItem* MainWindow::duplicateTreeItemRecursive(QTreeWidgetItem *source, QTreeWidgetItem *parent)
{
    if (!source) return nullptr;

    QString baseName = source->text(0);
    QString newName = baseName + " (Copy)";
    int counter = 1;

    while (true) {
        bool exists = false;
        int count = parent ? parent->childCount() : deckTree->topLevelItemCount();

        for (int i = 0; i < count; ++i) {
            QTreeWidgetItem *sib = parent ? parent->child(i) : deckTree->topLevelItem(i);
            if (sib && sib->text(0) == newName) {
                exists = true;
                break;
            }
        }
        if (!exists) break;

        counter++;
        newName = baseName + " (" + QString::number(counter) + ")";
    }

    QTreeWidgetItem *newItem = new QTreeWidgetItem(QStringList() << newName);

    QString type = source->data(0, Qt::UserRole).toString();
    newItem->setData(0, Qt::UserRole, type);
    newItem->setFlags(source->flags() | Qt::ItemIsEditable);

    if (type == "deck") {
        newItem->setIcon(0, QIcon::fromTheme("document-edit"));
        QVariant cardsVar = source->data(0, Qt::UserRole + 1);
        if (cardsVar.isValid() && cardsVar.canConvert<QJsonArray>()) {
            newItem->setData(0, Qt::UserRole + 1, cardsVar);
        }
        newItem->setData(0, Qt::UserRole + 2, QDateTime::currentDateTime().toString(Qt::ISODate));
    } else if (type == "folder") {
        newItem->setIcon(0, QIcon::fromTheme("folder"));
    }

    if (parent) {
        parent->addChild(newItem);
    } else {
        deckTree->addTopLevelItem(newItem);
    }

    for (int i = 0; i < source->childCount(); ++i) {
        duplicateTreeItemRecursive(source->child(i), newItem);
    }

    return newItem;
}

QTreeWidgetItem* MainWindow::duplicateFolderStructureOnly(QTreeWidgetItem *source, QTreeWidgetItem *parent)
{
    if (!source) return nullptr;

    QString baseName = source->text(0);
    QString newName = baseName + " (Empty Copy)";
    int counter = 1;

    while (true) {
        bool exists = false;
        int count = parent ? parent->childCount() : deckTree->topLevelItemCount();
        for (int i = 0; i < count; ++i) {
            QTreeWidgetItem *sib = parent ? parent->child(i) : deckTree->topLevelItem(i);
            if (sib && sib->text(0) == newName) {
                exists = true;
                break;
            }
        }
        if (!exists) break;
        counter++;
        newName = baseName + " (" + QString::number(counter) + ")";
    }

    QTreeWidgetItem *newItem = new QTreeWidgetItem(QStringList() << newName);
    newItem->setIcon(0, QIcon::fromTheme("folder"));
    newItem->setData(0, Qt::UserRole, "folder");
    newItem->setFlags(newItem->flags() | Qt::ItemIsEditable);

    if (parent) {
        parent->addChild(newItem);
    } else {
        deckTree->addTopLevelItem(newItem);
    }

    for (int i = 0; i < source->childCount(); ++i) {
        QTreeWidgetItem *child = source->child(i);
        if (child->data(0, Qt::UserRole).toString() == "folder") {
            duplicateFolderStructureOnly(child, newItem);
        }
    }

    return newItem;
}

void MainWindow::duplicateCurrentDeck()
{
    QTreeWidgetItem *source = deckTree->currentItem();
    if (!source || source->data(0, Qt::UserRole).toString() != "deck") {
        return;
    }

    QTreeWidgetItem *parent = source->parent();
    QTreeWidgetItem *newDeck = duplicateTreeItemRecursive(source, parent);

    if (newDeck) {
        deckTree->setCurrentItem(newDeck);
        deckTree->scrollToItem(newDeck);
        saveDecks();
        updateToolbarActions();
    }
}

void MainWindow::duplicateFolderFull()
{
    QTreeWidgetItem *item = deckTree->currentItem();
    if (!item || item->data(0, Qt::UserRole) != "folder") return;
    QTreeWidgetItem *parent = item->parent();
    if (!parent) parent = deckTree->invisibleRootItem();
    duplicateTreeItemRecursive(item, parent);
    saveDecks();
}

void MainWindow::duplicateFolderEmpty()
{
    QTreeWidgetItem *source = deckTree->currentItem();
    if (!source || source->data(0, Qt::UserRole).toString() != "folder") {
        return;
    }

    QTreeWidgetItem *parent = source->parent();
    if (!parent) parent = deckTree->invisibleRootItem();

    QTreeWidgetItem *newFolder = duplicateFolderStructureOnly(source, parent);

    if (newFolder) {
        deckTree->setCurrentItem(newFolder);
        deckTree->scrollToItem(newFolder);
        saveDecks();
        updateToolbarActions();
    }
}

void MainWindow::ensureSidebarVisible()
{
    if (!splitter || splitter->count() < 2) return;
    QList<int> sizes = splitter->sizes();
    if (sizes.isEmpty() || sizes[0] < 150) {
        int targetWidth = qMax(280, lastSidebarWidth);
        splitter->setSizes({targetWidth, splitter->width() - targetWidth});
    }
}

void MainWindow::markDeckAsDirty()
{
    if (currentDeckItem && !deckIsDirty) {
        deckIsDirty = true;
        updateSaveButtonState();
    }
}

void MainWindow::updateSaveButtonState()
{
    if (!saveDeckAction) return;
    saveDeckAction->setEnabled(deckIsDirty);
    if (QToolButton* btn = qobject_cast<QToolButton*>(navToolBar->widgetForAction(saveDeckAction))) {
        if (deckIsDirty) {
            btn->setStyleSheet("QToolButton { "
                               "background-color: #f39c12;"
                               "border-radius: 8px;"
                               "padding: 5px 0px 5px 0px;"
                               "}");
        } else {
            btn->setStyleSheet("");
        }
    }
}

void MainWindow::updateNumQuestionsRange()
{
    if (!numQuestionsSpinBox || !currentDeckItem) return;
    int totalCards = currentDeckItem->data(0, Qt::UserRole + 1).toJsonArray().size();
    if (totalCards == 0) totalCards = 1;
    numQuestionsSpinBox->setMaximum(9999);
    if (numQuestionsSpinBox->value() < 1) {
        numQuestionsSpinBox->setValue(totalCards);
    }
}

void MainWindow::duplicateCardRow(QWidget *sourceRow)
{
    if (!cardRowsLayout || !sourceRow) return;
    QHBoxLayout *h = qobject_cast<QHBoxLayout*>(sourceRow->layout());
    if (!h) return;
    QTextEdit *q = qobject_cast<QTextEdit*>(h->itemAt(1)->widget());
    QTextEdit *a = qobject_cast<QTextEdit*>(h->itemAt(2)->widget());
    if (!q || !a) return;
    int mastery = sourceRow->property("mastery").toInt();
    int idx = cardRowsLayout->indexOf(sourceRow);
    QWidget *newRow = createCardRow(q->toPlainText(), a->toPlainText(), mastery);
    cardRowsLayout->insertWidget(idx + 1, newRow);
    markDeckAsDirty();
    updateNumQuestionsRange();
}

// Quiz
void MainWindow::startQuiz()
{
    bool userWantsFlashcard = lastUsedFlashcardMode;
    bool userWantsShuffle = lastUsedShuffle;
    if (shuffleButton) {
        userWantsShuffle = shuffleButton->isChecked();
        lastUsedShuffle = userWantsShuffle;
    }

    int desiredQuestions = -1;
    if (!isReviewMode && numQuestionsSpinBox) {
        desiredQuestions = numQuestionsSpinBox->value();
    }

    resetMainContent();

    bool usingSavedQuiz = false;
    if (useExactQuizCards && !pendingExactQuizCards.isEmpty()) {
        quizCardList = pendingExactQuizCards;
        usingSavedQuiz = true;
        useExactQuizCards = false;
        pendingExactQuizCards.clear();
    }

    allDeckBacks.clear();

    if (!usingSavedQuiz) {
        quizCardList.clear();

        if (currentDeckItem) {
            QJsonArray cardsJson = currentDeckItem->data(0, Qt::UserRole + 1).toJsonArray();
            for (const QJsonValue &val : cardsJson) {
                QString back = val.toObject()["back"].toString();
                if (!allDeckBacks.contains(back)) {
                    allDeckBacks << back;
                }
                QJsonObject obj = val.toObject();
                quizCardList.append(qMakePair(obj["front"].toString(), obj["back"].toString()));
            }
        } else {
            quizCardList = getAllLibraryCards();
            for (const auto &p : quizCardList) {
                if (!allDeckBacks.contains(p.second)) {
                    allDeckBacks << p.second;
                }
            }
        }

        if (quizCardList.isEmpty()) {
            QMessageBox::warning(this, "No Cards", "Your library has no flashcards yet!\nAdd some cards first.");
            if (currentDeckItem) showDeckContent(currentDeckItem);
            else showHomePage();
            return;
        }
    }

    int totalAvailable = quizCardList.size();
    int numToUse = (!isReviewMode && desiredQuestions > 0) ? desiredQuestions : totalAvailable;
    if (numToUse > totalAvailable) numToUse = totalAvailable;

    if (!userWantsFlashcard && totalAvailable < 4) {
        if (totalAvailable == 1) {
            QMessageBox::information(this, "Not Enough Cards",
                                     "Multiple Choice needs at least 2 cards.\n\nSwitch to Flashcard mode or add more cards.");
            if (currentDeckItem) showDeckContent(currentDeckItem);
            return;
        }
        // 2 or 3 cards are allowed - we'll handle gracefully in loadCurrentQuestion()
    }

    /*
    int numToUse;
    if (!currentDeckItem && !isReviewMode) {
        numToUse = 10;
    } else {
        numToUse = (!isReviewMode && desiredQuestions > 0) ? desiredQuestions : quizCardList.size();
    }
    if (numToUse > quizCardList.size()) numToUse = quizCardList.size();
    */

    if (userWantsShuffle) {
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(quizCardList.begin(), quizCardList.end(), g);
    }
    quizCardList.resize(numToUse);

    quizResults.clear();
    for (const auto &p : quizCardList) {
        quizResults.append(qMakePair(p, -1));
    }
    currentCardIndex = 0;
    cardFlipped = false;
    answered = false;
    score = 0;
    inQuizMode = true;
    updateToolbarActions();
    isFlashcardMode = userWantsFlashcard;

    quizWidget = new QWidget();
    quizWidget->setStyleSheet("background-color: #4A5259;");
    QVBoxLayout *quizLayout = new QVBoxLayout(quizWidget);
    quizLayout->setContentsMargins(20, 30, 20, 30);
    quizLayout->setSpacing(25);

    if (!quizProgressLabel) {
        quizProgressLabel = new QLabel(this);
        quizProgressLabel->setAlignment(Qt::AlignCenter);
        quizProgressLabel->setStyleSheet("font-size: 21px; color: #95a5a6; font-weight: bold;");
    }
    quizProgressLabel->setText(QString("%1 / %2").arg(1).arg(quizCardList.size()));

    QWidget *spacerLeft = new QWidget(this);
    spacerLeft->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    QWidget *spacerRight = new QWidget(this);
    spacerRight->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    spacerRight->setFixedWidth(140);

    int endQuizIndex = navToolBar->actions().indexOf(endQuizAction);
    if (endQuizIndex != -1) {
        progressActionLeft = navToolBar->insertWidget(navToolBar->actions().at(endQuizIndex), spacerLeft);
        progressActionCenter = navToolBar->insertWidget(navToolBar->actions().at(endQuizIndex), quizProgressLabel);
        progressActionRight = navToolBar->insertWidget(navToolBar->actions().at(endQuizIndex), spacerRight);
    } else {
        navToolBar->addWidget(spacerLeft);
        progressActionCenter = navToolBar->addWidget(quizProgressLabel);
        navToolBar->addWidget(spacerRight);
    }

    // Card Area
    cardArea = new QWidget(quizWidget);
    if (isFlashcardMode) {
        // Flashcard Mode
        cardArea->setStyleSheet("background-color: #34495e; border-radius: 20px; padding: 10px 40px 10px 40px;");
        QVBoxLayout *cardL = new QVBoxLayout(cardArea);
        cardL->setAlignment(Qt::AlignCenter);
        cardL->setSpacing(0);
        cardL->setContentsMargins(0, 0, 0, 0);
        frontLabel = new QLabel("", cardArea);
        frontLabel->setWordWrap(true);
        frontLabel->setAlignment(Qt::AlignCenter);
        frontLabel->setStyleSheet("font-weight: bold;");
        backLabel = new QLabel("", cardArea);
        backLabel->setWordWrap(true);
        backLabel->setAlignment(Qt::AlignCenter);
        backLabel->setStyleSheet("font-weight: bold; color: #2ecc71;");
        backLabel->setVisible(false);
        cardL->addStretch(1);
        cardL->addWidget(frontLabel);
        cardL->addSpacing(0);
        cardL->addWidget(backLabel);
        cardL->addStretch(1);

        QWidget *navContainer = new QWidget(quizWidget);
        QHBoxLayout *navLayout = new QHBoxLayout(navContainer);
        navLayout->setContentsMargins(0, 0, 0, 0);
        navLayout->setSpacing(15);
        prevButton = new QPushButton("← Previous", navContainer);
        actionButton = new QPushButton("Flip Card", navContainer);
        nextButton = new QPushButton("Next →", navContainer);
        QString navBtnStyle = R"(
            QPushButton {
                background-color: #2c3e50; color: white; padding: 12px 28px;
                font-size: 16px; font-weight: bold; border-radius: 8px;
                border: 2px solid #455a6f;
            }
            QPushButton:hover { border: 2px solid #3498db; }
            QPushButton:disabled { background-color: #1f2a36; color: #7f8c8e; border: 2px solid #455a6f; }
            QPushButton:focus { border: 2px solid #455a6f; outline: none; }
        )";
        prevButton->setStyleSheet(navBtnStyle);
        actionButton->setStyleSheet(navBtnStyle);
        nextButton->setStyleSheet(navBtnStyle);
        prevButton->setFocusPolicy(Qt::NoFocus);
        actionButton->setFocusPolicy(Qt::NoFocus);
        nextButton->setFocusPolicy(Qt::NoFocus);
        connect(prevButton, &QPushButton::clicked, this, &MainWindow::prevCard);
        connect(actionButton, &QPushButton::clicked, this, &MainWindow::flipCard);
        connect(nextButton, &QPushButton::clicked, this, &MainWindow::nextCard);

        ratingContainer = new QWidget(navContainer);
        QHBoxLayout *ratingL = new QHBoxLayout(ratingContainer);
        ratingL->setSpacing(12);
        ratingL->setContentsMargins(0,0,0,0);
        correctButton = new QPushButton("Correct ✓", ratingContainer);
        wrongButton = new QPushButton("Wrong ✗", ratingContainer);
        correctButton->setMaximumHeight(50);
        wrongButton->setMaximumHeight(50);
        correctButton->setStyleSheet(R"(
            QPushButton { background-color: #27ae60; color: white; padding: 12px 40px;
                          font-size: 17px; font-weight: bold; border-radius: 8px; border: 2px solid #27ae60; }
            QPushButton:hover { border: 2px solid #ffffff; }
        )");
        wrongButton->setStyleSheet(R"(
            QPushButton { background-color: #e74c3c; color: white; padding: 5px 40px 0px 40px;
                          font-size: 17px; font-weight: bold; border-radius: 8px; border: 2px solid #e74c3c; }
            QPushButton:hover { border: 2px solid #ffffff; }
        )");
        connect(correctButton, &QPushButton::clicked, this, &MainWindow::markCorrectAndNext);
        connect(wrongButton, &QPushButton::clicked, this, &MainWindow::markWrongAndNext);
        ratingL->addWidget(correctButton);
        ratingL->addWidget(wrongButton);
        ratingContainer->setVisible(false);

        navLayout->addWidget(prevButton);
        navLayout->addStretch(1);
        navLayout->addWidget(actionButton);
        navLayout->addWidget(ratingContainer);
        navLayout->addStretch(1);
        navLayout->addWidget(nextButton);
        quizLayout->addWidget(navContainer);
        quizLayout->addWidget(cardArea, 1);
    } else {
        // Mutliple Choice Mode
        cardArea->setStyleSheet("background-color: #34495e; border-radius: 20px; padding: 10px 30px 10px 30px;");
        QVBoxLayout *cardL = new QVBoxLayout(cardArea);
        cardL->setAlignment(Qt::AlignCenter);
        cardL->setSpacing(0);
        cardL->setContentsMargins(0, 0, 0, 0);
        frontLabel = new QLabel("", cardArea);
        frontLabel->setWordWrap(true);
        frontLabel->setAlignment(Qt::AlignCenter);
        frontLabel->setStyleSheet("font-weight: bold;");
        cardL->addStretch(1);
        cardL->addWidget(frontLabel);
        cardL->addStretch(1);

        choicesContainer = new QWidget(quizWidget);
        choicesContainer->setStyleSheet("background-color: transparent;");
        QVBoxLayout *choicesLayout = new QVBoxLayout(choicesContainer);
        choicesLayout->setSpacing(12);
        choicesLayout->setContentsMargins(0, 0, 0, 0);
        choiceButtons.clear();
        choiceLabels.clear();
        for (int i = 0; i < 4; ++i) {
            QPushButton *btn = new QPushButton(choicesContainer);
            btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
            btn->setCursor(Qt::PointingHandCursor);
            QLabel *label = new QLabel(btn);
            label->setWordWrap(true);
            label->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            label->setStyleSheet("background: transparent; color: white; font-size: 18px; font-weight: bold;");
            QVBoxLayout *btnLayout = new QVBoxLayout(btn);
            btnLayout->setContentsMargins(20, 14, 20, 14);
            btnLayout->addWidget(label);
            btn->setStyleSheet(R"(
                QPushButton {
                    background-color: #2c3e50;
                    border: 2px solid #455a6f;
                    border-radius: 12px;
                }
                QPushButton:hover {
                    border: 3px solid #3498db;
                }
            )");
            choicesLayout->addWidget(btn);
            choiceButtons.append(btn);
            choiceLabels.append(label);
            connect(btn, &QPushButton::clicked, this, [this, btn]() {
                onMultipleChoiceButtonClicked(btn);
            });
        }
        feedbackLabel = new QLabel("", quizWidget);
        feedbackLabel->setAlignment(Qt::AlignCenter);
        feedbackLabel->setStyleSheet("font-size: 22px; font-weight: bold; min-height: 50px;");

        QWidget *navContainer = new QWidget(quizWidget);
        QHBoxLayout *navLayout = new QHBoxLayout(navContainer);
        navLayout->setContentsMargins(0, 0, 0, 0);
        navLayout->setSpacing(30);
        prevButton = new QPushButton("← Previous", navContainer);
        nextButton = new QPushButton("Next →", navContainer);
        QString navBtnStyle = R"(
            QPushButton {
                background-color: #2c3e50; color: white; padding: 12px 28px;
                font-size: 16px; font-weight: bold; border-radius: 8px;
                border: 2px solid #455a6f;
            }
            QPushButton:hover { border: 2px solid #3498db; }
            QPushButton:disabled { background-color: #1f2a36; color: #7f8c8e; border: 2px solid #34495e; }
        )";
        prevButton->setStyleSheet(navBtnStyle);
        nextButton->setStyleSheet(navBtnStyle);
        connect(prevButton, &QPushButton::clicked, this, &MainWindow::prevCard);
        connect(nextButton, &QPushButton::clicked, this, &MainWindow::nextCard);

        navLayout->addWidget(prevButton);
        navLayout->addStretch(1);
        navLayout->addWidget(feedbackLabel);
        navLayout->addStretch(1);
        navLayout->addWidget(nextButton);
        quizLayout->addWidget(cardArea, 1);
        quizLayout->addWidget(navContainer);
        quizLayout->addWidget(choicesContainer);
    }

    mainContentLayout->addWidget(quizWidget);
    loadCurrentQuestion();
}

void MainWindow::flipOrNextCard()
{
    if (!cardFlipped) {
        cardFlipped = true;
        backLabel->setVisible(true);
        actionButton->setText("Got It ✓");
        actionButton->setStyleSheet("background-color: #27ae60; color: white; padding: 12px 28px; "
                                    "font-size: 16px; font-weight: bold; border-radius: 8px; "
                                    "border: 2px solid #27ae60;");
    } else {
        quizResults[currentCardIndex].second = true;
        nextCard();
    }
}

void MainWindow::nextCard()
{
    if (isFlashcardMode && currentCardIndex < quizResults.size()) {
        if (quizResults[currentCardIndex].second == -1) {
        }
    }
    if (currentCardIndex >= quizCardList.size() - 1) {
        showResultsPage();
    } else {
        currentCardIndex++;
        loadCurrentQuestion();
    }
}

void MainWindow::prevCard()
{
    if (currentCardIndex > 0) {
        currentCardIndex--;
        loadCurrentQuestion();
    }
}

void MainWindow::flipCard()
{
    if (cardFlipped) return;
    cardFlipped = true;

    if (backLabel && currentCardIndex < quizCardList.size()) {
        QString displayBack = quizCardList[currentCardIndex].second;

        if (displayBack.trimmed().isEmpty()) {
            displayBack = "[Empty Answer]";
        }

        backLabel->setWordWrap(true);
        adjustCardFontSize(backLabel, displayBack, false);
        backLabel->setText(displayBack);
        backLabel->setVisible(true);
    }

    if (actionButton) actionButton->setVisible(false);
    if (ratingContainer) ratingContainer->setVisible(true);
}

void MainWindow::markCorrectAndNext()
{
    if (currentCardIndex < quizResults.size())
        quizResults[currentCardIndex].second = 1;
    nextCard();
}

void MainWindow::markWrongAndNext()
{
    if (currentCardIndex < quizResults.size())
        quizResults[currentCardIndex].second = 0;
    nextCard();
}

void MainWindow::endQuiz()
{
    endQuizConfirmPending = false;
    if (endQuizAction) {
        endQuizAction->setText("End Quiz");
        endQuizAction->setIcon(QIcon::fromTheme("process-stop", QIcon::fromTheme("dialog-cancel")));
    }
    isReviewMode = false;
    resetMainContent();
    isSettingsPage = false;
    if (currentDeckItem) showDeckContent(currentDeckItem);
    else clearMainContent();
}

void MainWindow::handleEndQuizClick()
{
    if (!inQuizMode || !endQuizAction) return;
    if (endQuizConfirmPending) {
        endQuizConfirmPending = false;
        endQuizAction->setText("End Quiz");
        endQuizAction->setIcon(QIcon::fromTheme("process-stop", QIcon::fromTheme("dialog-cancel")));
        if (QToolButton* btn = qobject_cast<QToolButton*>(navToolBar->widgetForAction(endQuizAction))) {
            btn->setStyleSheet("");
        }
        endQuiz();
    } else {
        endQuizConfirmPending = true;
        endQuizAction->setText("You sure?");
        endQuizAction->setIcon(QIcon::fromTheme("dialog-warning"));
        if (QToolButton* btn = qobject_cast<QToolButton*>(navToolBar->widgetForAction(endQuizAction))) {
        }
        QTimer::singleShot(5000, this, [this]() {
            if (endQuizConfirmPending) {
                endQuizConfirmPending = false;
                if (endQuizAction) {
                    endQuizAction->setText("End Quiz");
                    endQuizAction->setIcon(QIcon::fromTheme("process-stop", QIcon::fromTheme("dialog-cancel")));
                    if (QToolButton* btn = qobject_cast<QToolButton*>(navToolBar->widgetForAction(endQuizAction))) {
                        btn->setStyleSheet("");
                    }
                }
            }
        });
    }
}

bool MainWindow::confirmExitQuiz()
{
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Exit Quiz");
    msgBox.setText("You are currently taking a quiz.\n\nExit now and lose your progress?");
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    return msgBox.exec() == QMessageBox::Yes;
}

void MainWindow::handleDeleteDeckClick()
{
    if (!currentDeckItem) return;
    if (deleteDeckConfirmPending) {
        deleteDeckConfirmPending = false;
        deleteDeckAction->setText("Delete Deck");
        deleteDeckAction->setIcon(QIcon::fromTheme("edit-delete"));
        if (QToolButton* btn = qobject_cast<QToolButton*>(navToolBar->widgetForAction(deleteDeckAction)))
            btn->setStyleSheet("");
        deleteCurrentDeck();
    } else {
        deleteDeckConfirmPending = true;
        deleteDeckAction->setText("Confirm Delete?");
        deleteDeckAction->setIcon(QIcon::fromTheme("dialog-warning"));
        if (QToolButton* btn = qobject_cast<QToolButton*>(navToolBar->widgetForAction(deleteDeckAction)))
            btn->setStyleSheet("QToolButton { background-color: #e74c3c; color: white; font-weight: bold; border: 1px solid #e74c3c; border-radius: 5px; padding: 3px 0px }"
                               "QToolButton:hover { border: 1px solid #ffffff; }");
        QTimer::singleShot(5000, this, [this]() {
            if (deleteDeckConfirmPending) {
                deleteDeckConfirmPending = false;
                deleteDeckAction->setText("Delete Deck");
                deleteDeckAction->setIcon(QIcon::fromTheme("edit-delete"));
                if (QToolButton* btn = qobject_cast<QToolButton*>(navToolBar->widgetForAction(deleteDeckAction)))
                    btn->setStyleSheet("");
            }
        });
    }
}

void MainWindow::handleDeleteFolderClick()
{
    QTreeWidgetItem *item = deckTree->currentItem();
    if (!item || item->data(0, Qt::UserRole).toString() != "folder") return;
    if (deleteFolderConfirmPending) {
        deleteFolderConfirmPending = false;
        deleteFolderAction->setText("Delete Folder");
        deleteFolderAction->setIcon(QIcon::fromTheme("edit-delete"));
        if (QToolButton* btn = qobject_cast<QToolButton*>(navToolBar->widgetForAction(deleteFolderAction)))
            btn->setStyleSheet("");
        deleteCurrentFolder();
    } else {
        deleteFolderConfirmPending = true;
        deleteFolderAction->setText("Confirm Delete?");
        deleteFolderAction->setIcon(QIcon::fromTheme("dialog-warning"));
        if (QToolButton* btn = qobject_cast<QToolButton*>(navToolBar->widgetForAction(deleteFolderAction)))
            btn->setStyleSheet("QToolButton { background-color: #e74c3c; color: white; font-weight: bold; border: 1px solid #e74c3c; border-radius: 5px; padding: 3px 0px }"
                               "QToolButton:hover { border: 1px solid #ffffff; }");
        QTimer::singleShot(5000, this, [this]() {
            if (deleteFolderConfirmPending) {
                deleteFolderConfirmPending = false;
                deleteFolderAction->setText("Delete Folder");
                deleteFolderAction->setIcon(QIcon::fromTheme("edit-delete"));
                if (QToolButton* btn = qobject_cast<QToolButton*>(navToolBar->widgetForAction(deleteFolderAction)))
                    btn->setStyleSheet("");
            }
        });
    }
}

void MainWindow::handleResetMasteryClick()
{
    if (!currentDeckItem) return;
    if (resetMasteryConfirmPending) {
        resetMasteryConfirmPending = false;
        resetMasteryAction->setText("Reset Mastery");
        resetMasteryAction->setIcon(QIcon::fromTheme("edit-clear", QIcon::fromTheme("view-refresh")));
        if (QToolButton* btn = qobject_cast<QToolButton*>(navToolBar->widgetForAction(resetMasteryAction)))
            btn->setStyleSheet("");
        resetDeckMastery();
    } else {
        resetMasteryConfirmPending = true;
        resetMasteryAction->setText("Reset really?");
        resetMasteryAction->setIcon(QIcon::fromTheme("dialog-warning"));
        if (QToolButton* btn = qobject_cast<QToolButton*>(navToolBar->widgetForAction(resetMasteryAction)))
            btn->setStyleSheet("QToolButton { background-color: #e74c3c; color: white; }");
        QTimer::singleShot(5000, this, [this]() {
            if (resetMasteryConfirmPending) {
                resetMasteryConfirmPending = false;
                resetMasteryAction->setText("Reset Mastery");
                resetMasteryAction->setIcon(QIcon::fromTheme("edit-clear", QIcon::fromTheme("view-refresh")));
                if (QToolButton* btn = qobject_cast<QToolButton*>(navToolBar->widgetForAction(resetMasteryAction)))
                    btn->setStyleSheet("");
            }
        });
    }
}

// Quiz Results Page
void MainWindow::showResultsPage()
{
    applyMasteryFromQuiz();
    if (currentDeckItem) updateDeckLastQuiz(currentDeckItem);
    updateDailyStreak();
    resetMainContent();
    inQuizMode = true;
    updateToolbarActions();
    resultsWidget = new QWidget();
    resultsWidget->setStyleSheet("background-color: #4A5259;");
    QVBoxLayout *resLayout = new QVBoxLayout(resultsWidget);
    resLayout->setContentsMargins(30, 30, 30, 30);
    resLayout->setSpacing(25);

    int total = quizResults.size();
    int correctCount = 0;
    for (const auto &r : quizResults) {
        if (r.second == 1) correctCount++;
    }
    int wrongCount = 0;
    for (const auto &r : quizResults) {
        if (r.second == 0) wrongCount++;
    }
    QLabel *scoreLabel = new QLabel(QString("%1 / %2 correct 🎉").arg(correctCount).arg(total), resultsWidget);
    scoreLabel->setAlignment(Qt::AlignCenter);
    scoreLabel->setStyleSheet("font-size: 42px; font-weight: bold; color: white;");
    resLayout->addWidget(scoreLabel);

    QScrollArea *scroll = new QScrollArea(resultsWidget);
    scroll->setWidgetResizable(true);
    scroll->setStyleSheet("background: transparent; border: none;");
    QWidget *listContainer = new QWidget();
    QVBoxLayout *listL = new QVBoxLayout(listContainer);
    listL->setSpacing(12);
    listL->setContentsMargins(0, 0, 0, 0);
    for (const auto &r : quizResults) {
        QWidget *row = new QWidget();
        QString statusText;
        QString rowStyle;
        QString statusColor;
        if (r.second == 1) {
            statusText = "✅ Correct";
            rowStyle = "background-color: #34495e; border-radius: 12px; padding: 16px; border: 2px solid #2ecc71;";
            statusColor = "#2ecc71";
        } else if (r.second == 0) {
            statusText = "❌ Wrong";
            rowStyle = "background-color: #34495e; border-radius: 12px; padding: 16px; border: 2px solid #e74c3c;";
            statusColor = "#e74c3c";
        } else {
            statusText = "⏭ Skipped";
            rowStyle = "background-color: #34495e; border-radius: 12px; padding: 16px; border: 2px solid #f39c12;";
            statusColor = "#f39c12";
        }
        row->setStyleSheet(rowStyle);
        QHBoxLayout *rowL = new QHBoxLayout(row);
        rowL->setSpacing(20);
        QLabel *front = new QLabel(r.first.first, row);
        front->setStyleSheet("font-size: 20px; font-weight: bold; color: white;");
        front->setWordWrap(true);
        QLabel *back = new QLabel(r.first.second, row);
        back->setStyleSheet("font-size: 20px; color: #95a5a6;");
        back->setWordWrap(true);
        QLabel *status = new QLabel(statusText, row);
        status->setStyleSheet(QString("color: %1; font-weight: bold; font-size: 18px;").arg(statusColor));
        rowL->addWidget(front, 1);
        rowL->addWidget(back, 1);
        rowL->addWidget(status);
        listL->addWidget(row);
    }
    scroll->setWidget(listContainer);
    resLayout->addWidget(scroll, 1);

    QWidget *btnRow = new QWidget();
    QHBoxLayout *btnL = new QHBoxLayout(btnRow);
    btnL->setSpacing(20);
    QPushButton *retakeBtn = new QPushButton("Retake Quiz", btnRow);
    QPushButton *reviewBtn = new QPushButton("Review Wrong Answers", btnRow);
    QPushButton *backBtn = new QPushButton("Back to Deck", btnRow);
    QString blueStyle = R"(
        QPushButton {
            background-color: #3498db;
            color: white;
            padding: 14px 32px;
            font-size: 16px;
            font-weight: bold;
            border-radius: 10px;
            border: none;
        }
        QPushButton:hover {
            background-color: #4aa3df;
            border: 2px solid #ffffff;
            padding: 12px 30px;
        }
    )";
    QString grayStyle = R"(
        QPushButton {
            background-color: #2c3e50;
            color: white;
            padding: 14px 32px;
            font-size: 16px;
            font-weight: bold;
            border-radius: 10px;
            border: none;
        }
        QPushButton:hover {
            background-color: #34495e;
            border: 2px solid #3498db;
            padding: 12px 30px;
        }
    )";
    retakeBtn->setStyleSheet(blueStyle);
    reviewBtn->setStyleSheet(blueStyle);
    backBtn->setStyleSheet(grayStyle);
    reviewBtn->setEnabled(wrongCount > 0);
    if (wrongCount == 0) {
        reviewBtn->setText("No mistakes 🎉");
    }
    connect(retakeBtn, &QPushButton::clicked, this, [this]() {
        isReviewMode = false;
        if (!quizCardList.isEmpty()) {
            pendingExactQuizCards = quizCardList;
            useExactQuizCards = true;
            lastUsedFlashcardMode = isFlashcardMode;
        }
        QTimer::singleShot(0, this, [this]() { startQuiz(); });
    });
    connect(reviewBtn, &QPushButton::clicked, this, [this]() {
        reviewCardList.clear();
        for (const auto &r : quizResults) {
            if (r.second == 0) {
                reviewCardList << r.first;
            }
        }
        if (reviewCardList.isEmpty()) {
            QMessageBox::information(this, "All Good!", "No wrong answers to review!");
            return;
        }
        isReviewMode = true;
        QTimer::singleShot(0, this, [this]() { startQuiz(); });
    });
    connect(backBtn, &QPushButton::clicked, this, &MainWindow::endQuiz);
    btnL->addWidget(retakeBtn);
    btnL->addWidget(reviewBtn);
    btnL->addWidget(backBtn);
    resLayout->addWidget(btnRow);
    mainContentLayout->addWidget(resultsWidget);
}

void MainWindow::loadCurrentQuestion()
{
    if (currentCardIndex >= quizCardList.size() || currentCardIndex >= quizResults.size()) {
        qWarning() << "Invalid state in loadCurrentQuestion - index" << currentCardIndex;
        endQuiz();
        return;
    }

    answered = false;
    auto card = quizCardList[currentCardIndex];

    QString displayFront = card.first;
    QString displayBack  = card.second;

    if (displayFront.trimmed().isEmpty()) {
        displayFront = "[Empty Question]";
    }
    if (displayBack.trimmed().isEmpty()) {
        displayBack = "[Empty Answer]";
    }

    if (quizProgressLabel)
        quizProgressLabel->setText(QString("%1 / %2").arg(currentCardIndex + 1).arg(quizCardList.size()));

    if (isFlashcardMode) {
        adjustCardFontSize(frontLabel, displayFront, false);
        if (backLabel) {
            backLabel->setText(displayBack);
            backLabel->setVisible(false);
        }
        if (actionButton) {
            actionButton->setText("Flip Card");
            actionButton->setVisible(true);
            actionButton->setEnabled(true);
        }
        if (ratingContainer) ratingContainer->setVisible(false);
        if (correctButton) correctButton->setEnabled(true);
        if (wrongButton) wrongButton->setEnabled(true);
        cardFlipped = false;
        if (prevButton) prevButton->setEnabled(currentCardIndex > 0);
        if (nextButton) {
            bool isLast = (currentCardIndex == quizCardList.size() - 1);
            nextButton->setText(isLast ? "Finish Quiz" : "Next →");
            nextButton->setEnabled(true);
        }
    } else {
        frontLabel->setText(card.first);
        adjustCardFontSize(frontLabel, card.first, true);
        if (feedbackLabel) feedbackLabel->setText("");
        if (nextButton) nextButton->setEnabled(false);

        QString correct = card.second;

        QStringList options = {correct};
        QStringList possibleDistractors;
        for (const QString &back : allDeckBacks) {
            if (back != correct) possibleDistractors << back;
        }

        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(possibleDistractors.begin(), possibleDistractors.end(), g);

        for (int i = 0; i < possibleDistractors.size() && options.size() < 4; ++i) {
            options << possibleDistractors[i];
        }

        std::shuffle(options.begin(), options.end(), g);

        int numChoices = options.size();

        for (int i = 0; i < 4 && i < choiceButtons.size(); ++i) {
            if (i < numChoices) {
                QString text = options[i];
                choiceLabels[i]->setText(text);
                choiceButtons[i]->setEnabled(true);
                choiceButtons[i]->setVisible(true);
            } else {
                choiceButtons[i]->setVisible(false);
            }
        }

        if (prevButton) prevButton->setEnabled(currentCardIndex > 0);
        bool isLast = (currentCardIndex == quizCardList.size() - 1);
        if (nextButton) nextButton->setText(isLast ? "Finish Quiz" : "Next →");
    }
}

void MainWindow::adjustCardFontSize(QLabel* label, const QString& text, bool isMultipleChoice)
{
    if (text.isEmpty()) {
        label->clear();
        return;
    }
    label->setText(text);
    label->setWordWrap(true);
    label->setAlignment(Qt::AlignCenter);

    if (!cardArea || cardArea->width() <= 200) {
        QTimer::singleShot(10, this, [this, label, text, isMultipleChoice]() {
            adjustCardFontSize(label, text, isMultipleChoice);
        });
        return;
    }

    const int availableWidth = cardArea->width() - 90;

    int availableHeight;
    if (isMultipleChoice) {
        availableHeight = cardArea->height() - 140;
    } else {
        availableHeight = (cardArea->height() - 180) / 2;
    }

    const int minSize = 12;
    const int maxSize = isMultipleChoice ? 56 : 42;

    QFont font = label->font();
    font.setBold(true);

    int bestSize = minSize;
    int low = minSize;
    int high = maxSize;
    while (low <= high) {
        int mid = low + (high - low) / 2;
        font.setPointSize(mid);
        QFontMetrics fm(font);
        QRect bounding = fm.boundingRect(0, 0, availableWidth, 99999,
                                         Qt::TextWordWrap | Qt::AlignCenter, text);
        if (bounding.height() <= availableHeight) {
            bestSize = mid;
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }

    font.setPointSize(bestSize);
    label->setFont(font);
    label->updateGeometry();
}

void MainWindow::onMultipleChoiceButtonClicked(QPushButton *clickedButton)
{
    if (answered) return;
    answered = true;

    QString selectedText = clickedButton->findChild<QLabel*>()->text();
    QString correctAnswer = quizCardList[currentCardIndex].second;
    bool isCorrect = (selectedText == correctAnswer);

    quizResults[currentCardIndex].second = isCorrect ? 1 : 0;
    if (isCorrect) score++;

    for (int i = 0; i < choiceButtons.size(); ++i) {
        QPushButton *btn = choiceButtons[i];
        QLabel *lbl = choiceLabels[i];
        btn->setEnabled(false);

        if (btn == clickedButton) {
            QString borderColor = isCorrect ? "#27ae60" : "#e74c3c";
            btn->setStyleSheet(QString(R"(
                QPushButton {
                    background-color: #2c3e50;
                    border: 4px solid %1;
                    border-radius: 12px;
                }
            )").arg(borderColor));
        }
        else if (btn->findChild<QLabel*>()->text() == correctAnswer) {
            btn->setStyleSheet(R"(
                QPushButton {
                    background-color: #2c3e50;
                    border: 4px solid #27ae60;
                    border-radius: 12px;
                }
            )");
        }
    }

    if (feedbackLabel) {
        feedbackLabel->setText(isCorrect ? "✓ Correct!" : "✗ Incorrect");
        feedbackLabel->setStyleSheet(QString("color: %1; font-size: 22px; font-weight: bold;")
                                         .arg(isCorrect ? "#2ecc71" : "#e74c3c"));
    }

    if (nextButton) nextButton->setEnabled(true);
}

// Settings Page
void MainWindow::showSettingsPage()
{
    resetMainContent();
    isSettingsPage = true;
    QWidget *settingsWidget = new QWidget();
    settingsWidget->setStyleSheet("background-color: #4a5259;");
    QVBoxLayout *mainL = new QVBoxLayout(settingsWidget);
    mainL->setContentsMargins(20, 20, 20, 20);
    mainL->setSpacing(25);

    QHBoxLayout *topRow = new QHBoxLayout();
    QPushButton *backBtn = new QPushButton("Back", settingsWidget);
    backBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #2c3e50;
            color: white;
            padding: 10px 50px;
            font-size: 15px;
            border-radius: 8px;
            border: 2px solid #455a6f;
            min-width: 160px;
        }
        QPushButton:hover { border: 2px solid #3498db; }
    )");
    connect(backBtn, &QPushButton::clicked, this, &MainWindow::endQuiz);

    QLabel *title = new QLabel("Settings", settingsWidget);
    title->setStyleSheet("font-size: 32px; font-weight: bold; color: white;");
    title->setContentsMargins(18, 0, 0, 0);
    title->setAlignment(Qt::AlignCenter);

    topRow->addWidget(backBtn);
    topRow->addStretch(1);
    topRow->addWidget(title);
    topRow->addStretch(1);
    QWidget *rightSpacer = new QWidget(settingsWidget);
    rightSpacer->setFixedWidth(backBtn->sizeHint().width() + 15);
    topRow->addWidget(rightSpacer);
    mainL->addLayout(topRow);

    // General Settings
    QGroupBox *generalGroup = new QGroupBox("General", settingsWidget);
    generalGroup->setStyleSheet("QGroupBox { background-color: #2c3e50; font-weight: bold; font-size: 18px; color: white; padding: 10px; border-radius: 8px; }");
    QVBoxLayout *generalL = new QVBoxLayout(generalGroup);
    generalL->setContentsMargins(15, 30, 15, 15);

    // Start on Page
    QHBoxLayout *startRow = new QHBoxLayout();
    QLabel *startLabel = new QLabel("Start on page:", settingsWidget);
    startLabel->setStyleSheet("background-color: #2c3e50; color: #bdc3c7; font-size: 15px;");

    QComboBox *typeCombo = new QComboBox(settingsWidget);
    typeCombo->addItem("Homepage", "home");
    typeCombo->addItem("Folder", "folder");
    typeCombo->addItem("Deck", "deck");
    typeCombo->setCurrentIndex(typeCombo->findData(startOnLaunchType));

    QComboBox *targetCombo = new QComboBox(settingsWidget);

    auto updateTargetCombo = [this, targetCombo](const QString &type) {
        targetCombo->clear();
        if (type == "folder") {
            populateFolderCombo(targetCombo, nullptr, 0);
        } else if (type == "deck") {
            QList<QTreeWidgetItem*> allDecks = collectDecksRecursive(deckTree->invisibleRootItem());
            for (auto *deck : allDecks) {
                targetCombo->addItem(deck->text(0));
            }
        }
        targetCombo->setCurrentText(startOnLaunchTarget);
    };

    connect(typeCombo, &QComboBox::currentIndexChanged,
            this, [this, typeCombo, targetCombo, updateTargetCombo](int index) {
                QString type = typeCombo->itemData(index).toString();
                startOnLaunchType = type;
                if (type == "home") {
                    startOnLaunchTarget = "";
                }
                updateTargetCombo(type);
                targetCombo->setVisible(type != "home");
                saveSettings();
            });

    connect(targetCombo, &QComboBox::currentTextChanged,
            this, [this](const QString &text) {
                if (!text.isEmpty() && startOnLaunchType != "home") {
                    startOnLaunchTarget = text.trimmed();
                    saveSettings();
                }
            });

    updateTargetCombo(startOnLaunchType);
    targetCombo->setVisible(startOnLaunchType != "home");

    startRow->addWidget(startLabel);
    startRow->addWidget(typeCombo);
    startRow->addWidget(targetCombo);
    startRow->addStretch();

    generalL->addLayout(startRow);

    mainL->addWidget(generalGroup);

    // Mastery Settings
    QGroupBox *masteryGroup = new QGroupBox("Mastery", settingsWidget);
    masteryGroup->setStyleSheet("QGroupBox { background-color: #2c3e50; font-weight: bold; font-size: 18px; color: white; padding: 10px; border-radius: 8px; }");
    QVBoxLayout *masteryL = new QVBoxLayout(masteryGroup);
    masteryL->setContentsMargins(15, 30, 15, 15);
    masteryL->setSpacing(12);

    // Reset All Masteries
    QPushButton *resetMasteryBtn = new QPushButton("Reset All Masteries", settingsWidget);
    resetMasteryBtn->setToolTip("Resets every flashcard mastery to 0%");
    resetMasteryBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #e74c3c;
            color: white;
            padding: 12px 24px;
            font-size: 15px;
            font-weight: bold;
            border-radius: 8px;
            border: none;
            min-width: 160px;
        }
        QPushButton:hover { background-color: #e74c3c; border: 2px solid white }
    )");
    connect(resetMasteryBtn, &QPushButton::clicked, this, &MainWindow::resetAllMasteries);

    QHBoxLayout *masteryCenterL = new QHBoxLayout();
    masteryCenterL->addStretch(1);
    masteryCenterL->addWidget(resetMasteryBtn);
    masteryCenterL->addStretch(1);
    masteryL->addLayout(masteryCenterL);

    mainL->addWidget(masteryGroup);

    // System Settings
    QGroupBox *systemGroup = new QGroupBox("System", settingsWidget);
    systemGroup->setStyleSheet("QGroupBox { background-color: #2c3e50; font-weight: bold; font-size: 18px; color: white; padding: 10px; border-radius: 8px; }");
    QVBoxLayout *systemL = new QVBoxLayout(systemGroup);
    systemL->setContentsMargins(15, 30, 15, 15);
    systemL->setSpacing(12);

    // Import/Export Library
    QHBoxLayout *ioLayout = new QHBoxLayout();
    ioLayout->setSpacing(12);

    QPushButton *importBtn = new QPushButton("Import Library", settingsWidget);
    importBtn->setToolTip("Replace your entire library with a previously exported .json file");
    importBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #27ae60;
            color: white;
            padding: 12px 24px;
            font-size: 15px;
            font-weight: bold;
            border-radius: 8px;
            border: none;
            min-width: 160px;
        }
        QPushButton:hover { background-color: #27ae60; border: 2px solid white }
    )");

    QPushButton *exportBtn = new QPushButton("Export Library", settingsWidget);
    exportBtn->setToolTip("Save a full backup of your entire library");
    exportBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #3498db;
            color: white;
            padding: 12px 24px;
            font-size: 15px;
            font-weight: bold;
            border-radius: 8px;
            border: none;
            min-width: 160px;
        }
        QPushButton:hover { background-color: #3498db; border: 2px solid white }
    )");


    ioLayout->addStretch(1);
    ioLayout->addWidget(importBtn);
    ioLayout->addWidget(exportBtn);
    ioLayout->addStretch(1);

    systemL->addLayout(ioLayout);

    // Clean Start Settings
    QHBoxLayout *dangerLayout = new QHBoxLayout();
    dangerLayout->setSpacing(12);

    QPushButton *deleteAllBtn = new QPushButton("Clean Library", settingsWidget);
    deleteAllBtn->setToolTip("Start from scratch, delete every folder, deck and flashcard");
    deleteAllBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #e74c3c;
            color: white;
            padding: 12px 24px;
            font-size: 15px;
            font-weight: bold;
            border-radius: 8px;
            border: none;
            min-width: 160px;
        }
        QPushButton:hover { background-color: #e74c3c; border: 2px solid white }
    )");

    QPushButton *resetStreakBtn = new QPushButton("Reset Daily Streak", settingsWidget);
    resetStreakBtn->setToolTip("Reset your current daily study streak to zero");
    resetStreakBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #e74c3c;
            color: white;
            padding: 12px 24px;
            font-size: 15px;
            font-weight: bold;
            border-radius: 8px;
            border: none;
            min-width: 160px;
        }
        QPushButton:hover { background-color: #e74c3c; border: 2px solid white }
    )");

    dangerLayout->addStretch(1);
    dangerLayout->addWidget(deleteAllBtn);
    dangerLayout->addWidget(resetStreakBtn);
    dangerLayout->addStretch(1);

    systemL->addLayout(dangerLayout);

    connect(importBtn, &QPushButton::clicked, this, &MainWindow::importLibrary);
    connect(exportBtn, &QPushButton::clicked, this, &MainWindow::exportLibrary);
    connect(deleteAllBtn, &QPushButton::clicked, this, &MainWindow::deleteAllData);
    connect(resetStreakBtn, &QPushButton::clicked, this, &MainWindow::resetDailyStreak);

    mainL->addWidget(systemGroup);

    // Bottom Row
    QHBoxLayout *bottomRow = new QHBoxLayout();
    bottomRow->setSpacing(15);
    bottomRow->setContentsMargins(0, 0, 0, 0);

    QPushButton *changelogBtn = new QPushButton("What's New", settingsWidget);
    changelogBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #2c3e50;
            color: white;
            font-size: 18px;
            font-weight: bold;
            padding: 11px 24px;
            border-radius: 8px;
            border: 2px solid #455a6f;
        }
        QPushButton:hover { border: 2px solid #3498db; }
    )");
    connect(changelogBtn, &QPushButton::clicked, this, &MainWindow::showChangelog);

    QPushButton *aboutBtn = new QPushButton("About", settingsWidget);
    aboutBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #2c3e50;
            color: white;
            font-size: 18px;
            font-weight: bold;
            padding: 11px 24px;
            border-radius: 8px;
            border: 2px solid #455a6f;
        }
        QPushButton:hover { border: 2px solid #3498db; }
    )");
    connect(aboutBtn, &QPushButton::clicked, this, &MainWindow::showAboutDialog);

    QPushButton *donateBtn = new QPushButton("Buy me a Coffee", settingsWidget);
    donateBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #2c3e50;
            color: white;
            font-size: 18px;
            font-weight: bold;
            padding: 11px 24px;
            border-radius: 8px;
            border: 2px solid #455a6f;
        }
        QPushButton:hover { border: 2px solid #3498db; }
    )");
    connect(donateBtn, &QPushButton::clicked, this, [](){
        QDesktopServices::openUrl(QUrl("https://buymeacoffee.com/ethan_mccall"));
    });

    QStringList verses = { "Genesis 1:1", "Exodus 20:12", "Deuteronomy 6:4-5", "Psalm 23:1", "Psalm 46:1",
                            "Psalm 46:10", "Psalm 27:1", "Psalm 37:4", "Psalm 119:105", "Psalm 139:14",
                            "Proverbs 3:5-6", "Proverbs 18:10", "Proverbs 22:6", "Ecclesiastes 3:1", "Isaiah 40:31",
                            "Isaiah 41:10", "Isaiah 53:5", "Jeremiah 29:11", "Lamentations 3:22-23", "Micah 6:8",
                            "Habakkuk 3:17-18", "Zephaniah 3:17",
                            "Matthew 5:3", "Matthew 6:33-34", "Matthew 11:28-30", "Matthew 19:26", "Matthew 28:19-20",
                            "Mark 8:36", "Luke 6:31", "John 3:16-21", "John 8:32", "John 10:10",
                            "John 14:6", "John 14:27", "John 15:13", "John 16:33", "Acts 4:12",
                            "Romans 3:23", "Romans 5:8", "Romans 8:28", "Romans 12:2", "Romans 12:12",
                            "Romans 15:13", "1 Corinthians 10:13", "1 Corinthians 13:4-7", "2 Corinthians 5:17", "Galatians 2:20",
                            "Galatians 5:22-23", "Ephesians 2:8-9", "Philippians 4:6-7", "Philippians 4:13", "Colossians 3:23",
                            "1 Thessalonians 5:16-18", "2 Timothy 3:16", "Titus 2:11-12", "Hebrews 4:12", "Hebrews 11:1",
                            "James 1:2-3", "James 4:7", "1 Peter 5:7", "1 John 4:8", "Revelation 3:20",
                            "Revelation 21:4"
    };
    QLabel *randVerse = new QLabel(verses[QRandomGenerator::global()->bounded(verses.size())], settingsWidget);
    randVerse->setWordWrap(false);
    randVerse->setStyleSheet("font-size: 17px; color: #95a5a6; background-color: #4A5259; padding: 12px 25px; border-radius: 12px;");

    bottomRow->addWidget(changelogBtn);
    bottomRow->addWidget(aboutBtn);
    bottomRow->addWidget(donateBtn);
    bottomRow->addStretch(1);
    bottomRow->addWidget(randVerse);

    mainL->addStretch(1);
    mainL->addLayout(bottomRow);

    mainContentLayout->addWidget(settingsWidget);
    updateToolbarActions();
}

void MainWindow::applyStartOnLaunch()
{
    if (startOnLaunchType == "home" || startOnLaunchTarget.isEmpty()) {
        showHomePage();
        return;
    }

    QTreeWidgetItem *targetItem = nullptr;

    if (startOnLaunchType == "folder") {
        auto findFolder = [&](auto&& self, QTreeWidgetItem* item) -> QTreeWidgetItem* {
            if (!item) return nullptr;

            if (item->data(0, Qt::UserRole).toString() == "folder" &&
                item->text(0) == startOnLaunchTarget) {
                return item;
            }

            for (int i = 0; i < item->childCount(); ++i) {
                QTreeWidgetItem* found = self(self, item->child(i));
                if (found) return found;
            }
            return nullptr;
        };

        for (int i = 0; i < deckTree->topLevelItemCount(); ++i) {
            targetItem = findFolder(findFolder, deckTree->topLevelItem(i));
            if (targetItem) break;
        }
    }
    else if (startOnLaunchType == "deck") {
        QList<QTreeWidgetItem*> allDecks = collectDecksRecursive(deckTree->invisibleRootItem());
        for (auto *deck : allDecks) {
            if (deck->text(0) == startOnLaunchTarget) {
                targetItem = deck;
                break;
            }
        }
    }

    if (targetItem) {
        deckTree->setCurrentItem(targetItem);
        onDeckSelectionChanged();
        deckTree->scrollToItem(targetItem);
    } else {
        showHomePage();
    }
    checkDailyStreakAtLaunch();
    updateDailyStreak();
}

void MainWindow::populateFolderCombo(QComboBox* combo, QTreeWidgetItem* parent, int depth)
{
    if (!combo) return;

    QString indent = QString(" ").repeated(depth * 2);

    if (!parent) {
        for (int i = 0; i < deckTree->topLevelItemCount(); ++i) {
            QTreeWidgetItem* item = deckTree->topLevelItem(i);
            if (item->data(0, Qt::UserRole).toString() == "folder") {
                combo->addItem(indent + item->text(0));
                populateFolderCombo(combo, item, depth + 1);
            }
        }
    } else {
        for (int i = 0; i < parent->childCount(); ++i) {
            QTreeWidgetItem* item = parent->child(i);
            if (item->data(0, Qt::UserRole).toString() == "folder") {
                combo->addItem(indent + item->text(0));
                populateFolderCombo(combo, item, depth + 1);
            }
        }
    }
}

void MainWindow::showChangelog()
{
    QFile resourceFile(":/changelog.html");
    QString html;
    if (resourceFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        html = resourceFile.readAll();
        resourceFile.close();
    } else {
        html = "<h2 style='color:#e74c3c;'>Could not load changelog.html</h2>"
               "<p>Please make sure the file is added to your resources.qrc</p>";
    }

    QDialog *changelogDialog = new QDialog(this);
    changelogDialog->setWindowTitle("What's New in Flashcards");
    changelogDialog->resize(720, 560);
    changelogDialog->setStyleSheet("background-color: #2c3e50; color: white;");

    QVBoxLayout *layout = new QVBoxLayout(changelogDialog);
    layout->setSpacing(20);
    layout->setContentsMargins(30, 30, 30, 30);

    QTextBrowser *browser = new QTextBrowser(changelogDialog);
    browser->setHtml(html);
    browser->setStyleSheet(R"(
        QTextBrowser {
            background-color: #34495e;
            color: #ecf0f1;
            border: none;
            border-radius: 8px;
            padding: 15px;
            font-size: 14px;
        }
    )");
    browser->setOpenExternalLinks(true);
    layout->addWidget(browser, 1);

    QPushButton *closeBtn = new QPushButton("Close", changelogDialog);
    closeBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #34495e;
            color: white;
            padding: 12px 32px;
            font-size: 14px;
            font-weight: bold;
            border-radius: 8px;
            border: 2px solid #455a6f;
        }
        QPushButton:hover {
            border: 2px solid #3498db;
        }
    )");
    connect(closeBtn, &QPushButton::clicked, changelogDialog, &QDialog::accept);
    layout->addWidget(closeBtn);

    changelogDialog->exec();
    delete changelogDialog;
}

void MainWindow::updateDeckLastQuiz(QTreeWidgetItem *deck)
{
    if (!deck || deck->data(0, Qt::UserRole).toString() != "deck") return;
    deck->setData(0, Qt::UserRole + 2, QDateTime::currentDateTime().toString(Qt::ISODate));
    saveDecks();
}

int MainWindow::getTotalDecks()
{
    return collectDecksRecursive(deckTree->invisibleRootItem()).size();
}

int MainWindow::getTotalCards()
{
    int total = 0;
    auto decks = collectDecksRecursive(deckTree->invisibleRootItem());
    for (auto d : decks) {
        total += d->data(0, Qt::UserRole + 1).toJsonArray().size();
    }
    return total;
}

int MainWindow::getOverallMastery()
{
    auto all = collectDecksRecursive(deckTree->invisibleRootItem());
    if (all.isEmpty()) return 0;
    int sum = 0;
    for (auto d : all) sum += getDeckAverageMastery(d);
    return sum / all.size();
}

void MainWindow::checkDailyStreakAtLaunch()
{
    QDate today = QDate::currentDate();

    if (!lastStreakDate.isValid()) {
        dailyStreak = 0;
        return;
    }

    if (lastStreakDate == today) {
        return;
    }

    if (lastStreakDate.daysTo(today) > 1) {
        dailyStreak = 0;
    }
}

void MainWindow::updateDailyStreak()
{
    QDate today = QDate::currentDate();

    if (!lastStreakDate.isValid() || lastStreakDate.daysTo(today) > 1) {
        dailyStreak = 1;
    }
    else if (lastStreakDate == today) {
        return;
    }
    else {
        dailyStreak++;
    }

    lastStreakDate = today;
    saveSettings();
}

void MainWindow::startGlobalQuiz(bool flashcardMode)
{
    deckTree->clearSelection();
    currentDeckItem = nullptr;
    lastUsedFlashcardMode = flashcardMode;
    isReviewMode = false;
    startQuiz();
}

void MainWindow::startRandomDeckQuiz()
{
    QList<QTreeWidgetItem*> allDecks = collectDecksRecursive(deckTree->invisibleRootItem());
    if (allDecks.isEmpty()) {
        QMessageBox::information(this, "No Decks", "You have no decks yet!");
        return;
    }
    QTreeWidgetItem *randomDeck = allDecks[QRandomGenerator::global()->bounded(allDecks.size())];
    deckTree->setCurrentItem(randomDeck);
    currentDeckItem = randomDeck;
    if (numQuestionsSpinBox) numQuestionsSpinBox->setValue(10);
    startQuiz();
}

void MainWindow::startLibraryQuiz()
{
    deckTree->clearSelection();
    currentDeckItem = nullptr;
    if (numQuestionsSpinBox) numQuestionsSpinBox->setValue(10);
    startGlobalQuiz(lastUsedFlashcardMode);
}

void MainWindow::startFolderQuiz()
{
    QTreeWidgetItem *selectedFolder = deckTree->currentItem();
    if (!selectedFolder || selectedFolder->data(0, Qt::UserRole).toString() != "folder") {
        QMessageBox::information(this, "No Folder", "Please select a folder first.");
        return;
    }

    QList<QPair<QString,QString>> folderCards = getAllCardsInFolder(selectedFolder);

    if (folderCards.isEmpty()) {
        QMessageBox::information(this, "No Cards", "This folder (and its subfolders) has no flashcards yet!");
        return;
    }

    deckTree->clearSelection();
    currentDeckItem = nullptr;

    if (numQuestionsSpinBox) numQuestionsSpinBox->setValue(10);

    pendingExactQuizCards = folderCards;
    useExactQuizCards = true;

    startQuiz();
}

void MainWindow::resetAllMasteries()
{
    if (QMessageBox::question(this, "Reset All Masteries",
                              "This will reset mastery of EVERY card in ALL decks to 0%.\n\nAre you sure?",
                              QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;

    QList<QTreeWidgetItem*> allDecks = collectDecksRecursive(deckTree->invisibleRootItem());
    for (QTreeWidgetItem *deck : allDecks) {
        QJsonArray cards = deck->data(0, Qt::UserRole + 1).toJsonArray();
        for (int i = 0; i < cards.size(); ++i) {
            QJsonObject obj = cards[i].toObject();
            obj["mastery"] = 0;
            cards[i] = obj;
        }
        deck->setData(0, Qt::UserRole + 1, cards);
    }
    saveDecks();
    QMessageBox::information(this, "Success", "All masteries have been reset to 0%.");
}

void MainWindow::resetDailyStreak()
{
    if (QMessageBox::question(this, "Reset Daily Streak",
                              "This will reset your current daily streak to 0.\n\nAre you sure?",
                              QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;

    dailyStreak = 0;
    lastStreakDate = QDate();
    saveSettings();

    if (!currentDeckItem && !inQuizMode) {
        showHomePage();
    }

    QMessageBox::information(this, "Streak Reset", "Daily streak has been reset to 0 🔥");
}

void MainWindow::deleteAllData()
{
    bool ok;
    QString confirm = QInputDialog::getText(this, "Clean Library",
                                            "⚠️ This will permanently delete ALL folders, decks, and cards.\n\n"
                                            "There is no undo!\n\nType \"DELETE\" to confirm:",
                                            QLineEdit::Normal, "", &ok);

    if (!ok || confirm != "DELETE") {
        QMessageBox::information(this, "Cancelled", "Operation cancelled.");
        return;
    }

    deckTree->clear();
    saveDecks();
    clearMainContent();
    QMessageBox::information(this, "Success", "All data has been deleted.");
}

void MainWindow::saveSettings()
{
    QJsonObject settingsObj;
    settingsObj["sidebarWidth"] = lastSidebarWidth;
    settingsObj["quizFlashcardMode"] = lastUsedFlashcardMode;
    settingsObj["quizShuffle"] = lastUsedShuffle;
    settingsObj["startOnLaunchType"] = startOnLaunchType;
    settingsObj["startOnLaunchTarget"] = startOnLaunchTarget;
    settingsObj["dailyStreak"] = dailyStreak;
    if (lastStreakDate.isValid())
        settingsObj["lastStreakDate"] = lastStreakDate.toString(Qt::ISODate);

    QJsonDocument doc(settingsObj);
    QFile file(settingsFilePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson(QJsonDocument::Compact));
        file.close();
    }
}

void MainWindow::loadSettings()
{
    QFile file(settingsFilePath);
    if (!file.open(QIODevice::ReadOnly)) {
        startOnLaunchType = "home";
        startOnLaunchTarget = "";
        dailyStreak = 0;
        lastStreakDate = QDate();
        return;
    }
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    if (doc.isObject()) {
        QJsonObject settings = doc.object();

        if (settings.contains("sidebarWidth"))
            lastSidebarWidth = settings["sidebarWidth"].toInt(380);
        if (settings.contains("quizFlashcardMode"))
            lastUsedFlashcardMode = settings["quizFlashcardMode"].toBool(true);
        if (settings.contains("quizShuffle"))
            lastUsedShuffle = settings["quizShuffle"].toBool(true);
        if (settings.contains("startOnLaunchType"))
            startOnLaunchType = settings["startOnLaunchType"].toString("home");
        if (settings.contains("startOnLaunchTarget"))
            startOnLaunchTarget = settings["startOnLaunchTarget"].toString("");

        dailyStreak = settings.value("dailyStreak").toInt(0);
        QString dateStr = settings.value("lastStreakDate").toString();
        lastStreakDate = dateStr.isEmpty() ? QDate() : QDate::fromString(dateStr, Qt::ISODate);
    }
}

MainWindow::~MainWindow()
{
    saveSettings();
}

void MainWindow::toggleSidebar()
{
    QList<int> sizes = splitter->sizes();
    if (sizes.isEmpty()) return;
    if (sizes[0] <= 70) {
        sidePanel->setMinimumWidth(60);
        int totalWidth = splitter->width();
        int newWidth = qBound(200, lastSidebarWidth, 420);
        splitter->setSizes({newWidth, totalWidth - newWidth});
    }
    else {
        lastSidebarWidth = sizes[0];
        sidePanel->setMinimumWidth(0);
        splitter->setSizes({0, splitter->width()});
    }
}

void MainWindow::exportLibrary()
{
    QString fileName = QFileDialog::getSaveFileName(
        this,
        "Export Library",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/MyFlashcardsLibrary.json",
        "Flashcard Library (*.json)");

    if (fileName.isEmpty()) return;
    if (!fileName.endsWith(".json", Qt::CaseInsensitive))
        fileName += ".json";

    QJsonArray rootArray;
    for (int i = 0; i < deckTree->topLevelItemCount(); ++i) {
        saveTreeItem(rootArray, deckTree->topLevelItem(i));
    }

    QJsonDocument doc(rootArray);
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
        QMessageBox::information(this, "Export Complete ✅",
                                 "Your entire library was saved to:\n" + fileName);
    } else {
        QMessageBox::warning(this, "Export Failed", "Could not write the file.");
    }
}

void MainWindow::importLibrary()
{
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "Import Library",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        "Flashcard Library (*.json)");

    if (fileName.isEmpty()) return;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "Import Failed", "Could not open the selected file.");
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isArray()) {
        QMessageBox::warning(this, "Invalid File", "The selected file is not a valid library export.");
        return;
    }

    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Import Library");
    msgBox.setText("The selected file will be added to your current library.");
    msgBox.setIcon(QMessageBox::Question);

    QCheckBox *mergeCheckBox = new QCheckBox("Merge folders & decks with the same name (recommended)", &msgBox);
    mergeCheckBox->setChecked(true);
    msgBox.setCheckBox(mergeCheckBox);

    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::Yes);

    if (msgBox.exec() != QMessageBox::Yes) return;

    bool mergeMode = mergeCheckBox->isChecked();

    QApplication::setOverrideCursor(Qt::WaitCursor);

    QJsonArray importedArray = doc.array();

    if (!mergeMode) {
        for (const QJsonValue &val : importedArray) {
            loadTreeItem(val.toObject(), nullptr);
            QApplication::processEvents();
        }
    } else {
        for (const QJsonValue &val : importedArray) {
            mergeImportedItem(val.toObject(), nullptr);
            QApplication::processEvents();
        }
    }

    QApplication::restoreOverrideCursor();

    deckTree->expandAll();
    saveDecks();
    clearMainContent();

    QMessageBox::information(this, "Import Complete 🎉",
                             QString("Successfully imported %1 top-level item(s) in %2 mode.")
                                 .arg(importedArray.size())
                                 .arg(mergeMode ? "Merge" : "Append"));
}

void MainWindow::mergeImportedItem(const QJsonObject &importedObj, QTreeWidgetItem *targetParent)
{
    if (importedObj.isEmpty()) return;

    QString originalName = importedObj["name"].toString();
    QString type = importedObj["type"].toString();

    auto findExisting = [&](QTreeWidgetItem *parent, const QString &searchName) -> QTreeWidgetItem* {
        if (!parent) {
            for (int i = 0; i < deckTree->topLevelItemCount(); ++i) {
                QTreeWidgetItem *item = deckTree->topLevelItem(i);
                if (item->text(0) == searchName && item->data(0, Qt::UserRole).toString() == type)
                    return item;
            }
            return nullptr;
        }
        for (int i = 0; i < parent->childCount(); ++i) {
            QTreeWidgetItem *item = parent->child(i);
            if (item->text(0) == searchName && item->data(0, Qt::UserRole).toString() == type)
                return item;
        }
        return nullptr;
    };

    if (type == "folder") {
        QTreeWidgetItem *existing = findExisting(targetParent, originalName);
        if (existing) {
            QJsonArray importedChildren = importedObj["children"].toArray();
            for (const QJsonValue &childVal : importedChildren) {
                mergeImportedItem(childVal.toObject(), existing);
            }
        } else {
            loadTreeItem(importedObj, targetParent);
        }
    }
    else if (type == "deck") {
        QTreeWidgetItem *existing = findExisting(targetParent, originalName);
        if (existing) {
            QJsonObject newObj = importedObj;
            int counter = 2;
            QString candidateName = originalName + " (" + QString::number(counter) + ")";
            while (findExisting(targetParent, candidateName)) {
                counter++;
                candidateName = originalName + " (" + QString::number(counter) + ")";
            }
            newObj["name"] = candidateName;
            loadTreeItem(newObj, targetParent);
        } else {
            loadTreeItem(importedObj, targetParent);
        }
    }
}

void MainWindow::showAboutDialog()
{
    QDialog *aboutDialog = new QDialog(this);
    aboutDialog->setWindowTitle("About Flashcards");
    aboutDialog->resize(600, 440);
    aboutDialog->setStyleSheet("background-color: #2c3e50; color: white;");

    QVBoxLayout *layout = new QVBoxLayout(aboutDialog);
    layout->setSpacing(20);
    layout->setContentsMargins(30, 30, 30, 30);

    QLabel *titleLabel = new QLabel("<h1 style='color:#3498db;'>Flashcards by Ethan</h1>", aboutDialog);
    titleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(titleLabel);

    QLabel *versionLabel = new QLabel("Version 1.1", aboutDialog);
    versionLabel->setAlignment(Qt::AlignCenter);
    versionLabel->setStyleSheet("font-size: 16px; color: #bdc3c7;");
    layout->addWidget(versionLabel);

    layout->addSpacing(10);

    QLabel *desc = new QLabel("A clean and simple flashcard app to help you study and memorize anything effectively.\n\n"
                              "Create folders and decks with drag-and-drop, add your cards, and track your progress with mastery levels and daily streaks.", aboutDialog);
    desc->setAlignment(Qt::AlignCenter);
    desc->setWordWrap(true);
    layout->addWidget(desc);

    layout->addStretch();

    QLabel *licenseLabel = new QLabel("Licensed under the <b>GPL-3.0+ License</b><br>"
                                      "Copyright © 2026 Ethan McCall", aboutDialog);
    licenseLabel->setAlignment(Qt::AlignCenter);
    licenseLabel->setStyleSheet("font-size: 15px; color: #95a5a6;");
    layout->addWidget(licenseLabel);

    // Buttons
    QPushButton *githubBtn = new QPushButton("View on GitHub", aboutDialog);
    QPushButton *bugBtn    = new QPushButton("Report a Bug", aboutDialog);
    QPushButton *viewLicenseBtn = new QPushButton("View Full GPL License", aboutDialog);

    QString btnStyle = R"(
        QPushButton {
            background-color: #34495e;
            color: white;
            padding: 12px 24px;
            border-radius: 8px;
            border: 2px solid #455a6f;
            font-size: 14px;
            font-weight: bold;
        }
        QPushButton:hover {
            border: 2px solid #3498db;
            background-color: #3d4f63;
        }
    )";

    githubBtn->setStyleSheet(btnStyle);
    bugBtn->setStyleSheet(btnStyle);
    viewLicenseBtn->setStyleSheet(btnStyle);

    connect(githubBtn, &QPushButton::clicked, this, [](){
        QDesktopServices::openUrl(QUrl("https://github.com/ethan-mccall/flashcards-by-ethan"));
    });

    connect(bugBtn, &QPushButton::clicked, this, [](){
        QDesktopServices::openUrl(QUrl("https://github.com/ethan-mccall/flashcards-by-ethan/issues"));
    });

    connect(viewLicenseBtn, &QPushButton::clicked, this, &MainWindow::showFullLicense);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->addWidget(githubBtn);
    btnLayout->addWidget(bugBtn);
    btnLayout->addWidget(viewLicenseBtn);
    layout->addLayout(btnLayout);

    // Close button
    QPushButton *closeBtn = new QPushButton("Close", aboutDialog);
    closeBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #27ae60;
            color: white;
            padding: 12px 32px;
            font-size: 16px;
            font-weight: bold;
            border-radius: 8px;
        }
        QPushButton:hover {
            border: 2px solid #ffffff;
        }
    )");
    connect(closeBtn, &QPushButton::clicked, aboutDialog, &QDialog::accept);
    layout->addWidget(closeBtn);

    aboutDialog->exec();
    delete aboutDialog;
}

void MainWindow::showFullLicense()
{
    QFile file(":/LICENSE");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "License", "Could not open LICENSE file.");
        return;
    }

    QString licenseText = file.readAll();
    file.close();

    QDialog *licenseDlg = new QDialog(this);
    licenseDlg->setWindowTitle("GPL-3.0+ License");
    licenseDlg->resize(720, 560);

    QVBoxLayout *lay = new QVBoxLayout(licenseDlg);
    QTextBrowser *browser = new QTextBrowser(licenseDlg);
    browser->setPlainText(licenseText);
    browser->setStyleSheet("background-color: #2c3e50; color: #bdc3c7; font-size: 14px;");
    lay->addWidget(browser);

    QPushButton *closeBtn = new QPushButton("Close", licenseDlg);
    closeBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #34495e;
            color: white;
            padding: 12px 32px;
            border-radius: 8px;
            border: 2px solid #455a6f;
            font-size: 14px;
            font-weight: bold;
        }
        QPushButton:hover {
            border: 2px solid #3498db;
        }
    )");
    connect(closeBtn, &QPushButton::clicked, licenseDlg, &QDialog::accept);
    lay->addWidget(closeBtn);

    licenseDlg->exec();
    delete licenseDlg;
}
