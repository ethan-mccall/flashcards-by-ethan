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
        setMouseTracking(true);
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

class FlowLayout : public QLayout
{
public:
    explicit FlowLayout(QWidget *parent, int margin = -1, int hSpacing = -1, int vSpacing = -1);
    ~FlowLayout();

    void addItem(QLayoutItem *item) override;
    int horizontalSpacing() const;
    int verticalSpacing() const;
    Qt::Orientations expandingDirections() const override;
    QSize sizeHint() const override;
    QSize minimumSize() const override;
    int count() const override;
    QLayoutItem *itemAt(int index) const override;
    QLayoutItem *takeAt(int index) override;
    void setGeometry(const QRect &rect) override;

    bool hasHeightForWidth() const override { return true; }
    int heightForWidth(int width) const override;

    void setAlignment(Qt::Alignment alignment);
    Qt::Alignment alignment() const { return m_alignment; }

private:
    int doLayout(const QRect &rect, bool testOnly) const;
    int smartSpacing(QStyle::PixelMetric pm) const;

    QList<QLayoutItem *> itemList;
    int m_hSpace;
    int m_vSpace;
    Qt::Alignment m_alignment = Qt::AlignLeft;
};

FlowLayout::FlowLayout(QWidget *parent, int margin, int hSpacing, int vSpacing)
    : QLayout(parent), m_hSpace(hSpacing), m_vSpace(vSpacing)
{
    setContentsMargins(margin, margin, margin, margin);
}

FlowLayout::~FlowLayout()
{
    qDeleteAll(itemList);
}

void FlowLayout::addItem(QLayoutItem *item)
{
    itemList.append(item);
}

int FlowLayout::horizontalSpacing() const
{
    if (m_hSpace >= 0) return m_hSpace;
    return smartSpacing(QStyle::PM_LayoutHorizontalSpacing);
}

int FlowLayout::verticalSpacing() const
{
    if (m_vSpace >= 0) return m_vSpace;
    return smartSpacing(QStyle::PM_LayoutVerticalSpacing);
}

Qt::Orientations FlowLayout::expandingDirections() const
{
    return Qt::Horizontal | Qt::Vertical;
}

int FlowLayout::heightForWidth(int width) const
{
    return doLayout(QRect(0, 0, width, 0), true);
}

QSize FlowLayout::sizeHint() const
{
    const int preferred = 620;

    int w = preferred;
    if (parentWidget())
        w = qMin(w, parentWidget()->width());

    return QSize(w, heightForWidth(w));
}

QSize FlowLayout::minimumSize() const
{
    if (itemList.isEmpty())
        return QSize(0, 0);

    int maxItemWidth = 0;
    for (QLayoutItem *item : std::as_const(itemList)) {
        maxItemWidth = qMax(maxItemWidth, item->minimumSize().width());
    }

    int left, top, right, bottom;
    getContentsMargins(&left, &top, &right, &bottom);

    int minW = maxItemWidth + left + right + horizontalSpacing() * 2;

    return QSize(minW, heightForWidth(minW));
}

int FlowLayout::count() const { return itemList.size(); }

QLayoutItem *FlowLayout::itemAt(int index) const { return itemList.value(index); }

QLayoutItem *FlowLayout::takeAt(int index)
{
    if (index < 0 || index >= itemList.size()) return nullptr;
    return itemList.takeAt(index);
}

void FlowLayout::setGeometry(const QRect &rect)
{
    QLayout::setGeometry(rect);
    doLayout(rect, false);
}

int FlowLayout::doLayout(const QRect &rect, bool testOnly) const
{
    int left, top, right, bottom;
    getContentsMargins(&left, &top, &right, &bottom);
    QRect effectiveRect = rect.adjusted(+left, +top, -right, -bottom);

    int x = effectiveRect.x();
    int y = effectiveRect.y();
    int lineHeight = 0;
    int lineStartX = effectiveRect.x();
    int lineItemCount = 0;
    QList<QLayoutItem*> lineItems;

    for (QLayoutItem *item : std::as_const(itemList)) {
        QSize itemSize = item->sizeHint();
        int spaceX = horizontalSpacing();
        int spaceY = verticalSpacing();

        int nextX = x + itemSize.width() + spaceX;

        if (nextX - spaceX > effectiveRect.right() && lineHeight > 0) {
            if (!testOnly && lineItemCount > 0) {
                int totalWidth = x - lineStartX - spaceX;
                int offset = (effectiveRect.width() - totalWidth) / 2;

                int currentX = lineStartX + offset;
                for (QLayoutItem *lineItem : lineItems) {
                    QSize s = lineItem->sizeHint();
                    lineItem->setGeometry(QRect(QPoint(currentX, y), s));
                    currentX += s.width() + spaceX;
                }
            }

            x = effectiveRect.x();
            y = y + lineHeight + spaceY;
            lineStartX = x;
            lineItems.clear();
            lineItemCount = 0;
            nextX = x + itemSize.width() + spaceX;
            lineHeight = 0;
        }

        if (!testOnly) {
            lineItems.append(item);
        }

        x = nextX;
        lineHeight = qMax(lineHeight, itemSize.height());
        lineItemCount++;
    }

    if (!testOnly && lineItemCount > 0) {
        int totalWidth = x - lineStartX - horizontalSpacing();
        int offset = (effectiveRect.width() - totalWidth) / 2;

        int currentX = lineStartX + offset;
        for (QLayoutItem *lineItem : lineItems) {
            QSize s = lineItem->sizeHint();
            lineItem->setGeometry(QRect(QPoint(currentX, y), s));
            currentX += s.width() + horizontalSpacing();
        }
    }

    return y + lineHeight - rect.y() + bottom;
}

int FlowLayout::smartSpacing(QStyle::PixelMetric pm) const
{
    QObject *parent = this->parent();
    if (!parent) return -1;
    if (QWidget *pw = qobject_cast<QWidget*>(parent))
        return pw->style()->pixelMetric(pm, nullptr, pw);
    return -1;
}

void FlowLayout::setAlignment(Qt::Alignment alignment)
{
    m_alignment = alignment;
    if (parentWidget())
        parentWidget()->updateGeometry();
}

static void setCardText(QLabel* label, const QString& text)
{
    if (!label) return;
    label->setTextFormat(Qt::PlainText);
    label->setText(text);
}

void MainWindow::setupCardTextEdit(QTextEdit* edit)
{
    if (!edit) return;

    edit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    edit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    edit->setWordWrapMode(QTextOption::WordWrap);
    edit->setMinimumHeight(68);
    edit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    connect(edit, &QTextEdit::textChanged, this, [edit]() {
        QTextDocument* doc = edit->document();
        int newHeight = static_cast<int>(doc->size().height()) + 12;
        newHeight = qBound(68, newHeight, 240);
        if (edit->height() != newHeight) {
            edit->setFixedHeight(newHeight);
            if (edit->parentWidget())
                edit->parentWidget()->updateGeometry();
        }
    });
}

CustomTreeWidget::CustomTreeWidget(QWidget *parent)
    : QTreeWidget(parent)
{
}

void CustomTreeWidget::dropEvent(QDropEvent *event)
{
    QTreeWidgetItem *targetItem = itemAt(event->position().toPoint());
    if (targetItem && targetItem->data(0, Qt::UserRole).toString() == "deck") {
        event->ignore();
        return;
    }
    QTreeWidget::dropEvent(event);
}

void CustomTreeWidget::dragMoveEvent(QDragMoveEvent *event)
{
    QTreeWidgetItem *targetItem = itemAt(event->position().toPoint());
    if (targetItem && targetItem->data(0, Qt::UserRole).toString() == "deck") {
        event->ignore();
        return;
    }
    QTreeWidget::dragMoveEvent(event);
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    // Core state
    , isFlashcardMode(false)
    , inQuizMode(false)
    , isReviewMode(false)
    , isSettingsPage(false)
    , deckIsDirty(false)
    , cardFlipped(false)
    , answered(false)
    , currentCardIndex(0)
    , score(0)
    , dailyStreak(0)

    // Pointers
    , choicesContainer(nullptr)
    , choiceListWidget(nullptr)
    , prevButton(nullptr)
    , cardArea(nullptr)
    , actionArea(nullptr)
    , currentDeckItem(nullptr)
    , draggedCard(nullptr)
    , dropPlaceholder(nullptr)
    , startQuizButton(nullptr)
    , shuffleButton(nullptr)
    , correctButton(nullptr)
    , wrongButton(nullptr)
    , ratingContainer(nullptr)
    , numQuestionsSpinBox(nullptr)
    , quizWidget(nullptr)
    , resultsWidget(nullptr)
    , frontLabel(nullptr)
    , backLabel(nullptr)
    , feedbackLabel(nullptr)
    , actionButton(nullptr)
    , nextButton(nullptr)
    , cardRowsLayout(nullptr)
    , cardContainer(nullptr)
    , quizStyleGroup(nullptr)
    , randomDeckBtn(nullptr)
    , libraryBtn(nullptr)
    , folderBtn(nullptr)
    , styleToggleBtn(nullptr)

    // Containers
    , quizResults()
    , reviewCardList()
    , quizCardList()
    , allDeckBacks()
    , choiceButtons()
    , choiceLabels()
    , lastStreakDate()

    // Mastery Settings
    , masteryCorrectPoints(10)
    , masteryIncorrectPoints(-5)

    // Last Used Quiz Preferences
    , lastUsedFlashcardMode(true)
    , lastUsedShuffle(true)
    , lastUsedQuizDirection(QuizDirection::FrontToBack)
{
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
    lastUsedQuizDirection = QuizDirection::FrontToBack;

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
    sidePanel->setObjectName("sidePanel");
    sidePanel->setMinimumWidth(60);
    sidePanel->setStyleSheet(R"(
        #sidePanel {
            background-color: #2c3e50;
            color: white;
            padding: 8px;
        }

        #sidePanel QScrollBar:vertical,
        #sidePanel QScrollBar:horizontal {
            background-color: #2c3e50;
            border: none;
            margin: 0px;
        }
        #sidePanel QScrollBar:vertical { width: 16px; }
        #sidePanel QScrollBar:horizontal { height: 14px; }

        #sidePanel QScrollBar::handle:vertical,
        #sidePanel QScrollBar::handle:horizontal {
            background-color: #3498db;
            border-radius: 8px;
            min-height: 40px;
            min-width: 40px;
        }
        #sidePanel QTreeWidget QLineEdit {
            background-color: #2c3e50;
            color: white;
            border: 2px solid #3498db;
            border-radius: 6px;
            padding: 4px 8px;
            font-size: 20px;
            selection-background-color: #2980b9;
        }
    )");
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
    resize(1510, 1000);

    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);
    decksFilePath = dataDir + "/decks.json";
    settingsFilePath = dataDir + "/settings.json";

    loadDecks();
    migrateOldDeckHeaders();
    connect(deckTree, &QTreeWidget::itemExpanded,  this, &MainWindow::saveDecks);
    connect(deckTree, &QTreeWidget::itemCollapsed, this, &MainWindow::saveDecks);
    loadSettings();

    checkDailyStreakAtLaunch();

    applyStartOnLaunch();

    qApp->setStyleSheet(R"(
        QToolTip {
            background-color: #34495e;
            color: white;
            border: 2px solid #3498db;
            border-radius: 8px;
            padding: 2px 2px;
            font-size: 15px;
            font-weight: bold;
        }
        QScrollBar:vertical {
            background: #2c3e50;
            width: 14px;
            margin: 0px;
        }
        QScrollBar::handle:vertical {
            background: #3498db;
            border-radius: 7px;
            min-height: 30px;
        }
        QScrollBar::handle:vertical:hover {
            background: #2980b9;
        }
        QScrollBar:horizontal {
            background: #2c3e50;
            height: 14px;
            margin: 0px;
        }
        QScrollBar::handle:horizontal {
            background: #3498db;
            border-radius: 7px;
            min-width: 30px;
        }
        QScrollBar::handle:horizontal:hover {
            background: #2980b9;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical,
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
            height: 0px;
            width: 0px;
            background: none;
        }
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical,
        QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal {
            background: none;
        }
    )");
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
    newItem->setFlags(newItem->flags() | Qt::ItemIsEditable | Qt::ItemNeverHasChildren);

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
        connect(expandAll, &QAction::triggered, this, [this, item]() {
            expandSubtree(item);
        });
        QAction *collapseAll = contextMenu.addAction(QIcon::fromTheme("collapse-all"), "Collapse All");
        connect(collapseAll, &QAction::triggered, this, [this, item]() {
            collapseSubtree(item);
        });
        contextMenu.addSeparator();
        QAction *delF = contextMenu.addAction(QIcon::fromTheme("edit-delete"), "Delete Folder");
        connect(delF, &QAction::triggered, this, &MainWindow::confirmDeleteCurrentFolder);
    } else {
        QAction *renameD = contextMenu.addAction(QIcon::fromTheme("edit-rename"), "Rename Deck");
        connect(renameD, &QAction::triggered, this, &MainWindow::renameCurrentDeck);
        QAction *dupD = contextMenu.addAction(QIcon::fromTheme("edit-copy"), "Duplicate Deck");
        connect(dupD, &QAction::triggered, this, &MainWindow::duplicateCurrentDeck);
        contextMenu.addSeparator();
        QAction *delD = contextMenu.addAction(QIcon::fromTheme("edit-delete"), "Delete Deck");
        connect(delD, &QAction::triggered, this, &MainWindow::confirmDeleteCurrentDeck);
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

        int prefNum = item->data(0, Qt::UserRole + 3).toInt(0);
        if (prefNum > 0) {
            obj["preferredNumQuestions"] = prefNum;
        }
        if (item == currentDeckItem) {
            obj["frontHeader"] = currentFrontHeader;
            obj["backHeader"]  = currentBackHeader;
        } else {
            obj["frontHeader"] = item->data(0, Qt::UserRole + 4).toString();
            obj["backHeader"]  = item->data(0, Qt::UserRole + 5).toString();
        }
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

        if (obj.contains("preferredNumQuestions")) {
            item->setData(0, Qt::UserRole + 3, obj["preferredNumQuestions"].toInt());
        }
        item->setFlags(item->flags() | Qt::ItemIsEditable | Qt::ItemNeverHasChildren);
        if (obj.contains("frontHeader"))
            item->setData(0, Qt::UserRole + 4, obj["frontHeader"].toString());
        if (obj.contains("backHeader"))
            item->setData(0, Qt::UserRole + 5, obj["backHeader"].toString());
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

void MainWindow::migrateOldDeckHeaders()
{
    QList<QTreeWidgetItem*> allDecks = collectDecksRecursive(deckTree->invisibleRootItem());
    bool needsSave = false;

    for (QTreeWidgetItem *deck : allDecks) {
        QString front = deck->data(0, Qt::UserRole + 4).toString().trimmed();
        QString back  = deck->data(0, Qt::UserRole + 5).toString().trimmed();

        if (front.isEmpty()) {
            deck->setData(0, Qt::UserRole + 4, "Question");
            needsSave = true;
        }
        if (back.isEmpty()) {
            deck->setData(0, Qt::UserRole + 5, "Answer");
            needsSave = true;
        }
    }

    if (needsSave) {
        saveDecks();
    }
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
        exampleDeck->setFlags(exampleDeck->flags() | Qt::ItemIsEditable | Qt::ItemNeverHasChildren);
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
                if (currentDeckItem) deckTree->setCurrentItem(currentDeckItem);
                else deckTree->clearSelection();
                return;
            }

            endQuiz();

            selected = deckTree->currentItem();
            if (selected && selected->data(0, Qt::UserRole).toString() == "deck") {
                currentDeckItem = selected;
                showDeckContent(selected);
            } else {
                currentDeckItem = nullptr;
                showHomePage();
            }

            updateAddButtonsState();
            updateToolbarActions();
            return;
        }
    }

    QTreeWidgetItem *selected = deckTree->currentItem();

    if (selected && selected->data(0, Qt::UserRole).toString() == "deck") {
        if (selected != currentDeckItem) {
            currentDeckItem = selected;
            showDeckContent(selected);
        }
    } else {
        currentDeckItem = nullptr;
        showHomePage();
    }

    updateAddButtonsState();
    updateToolbarActions();
}

void MainWindow::resetMainContent()
{
    if (currentDeckItem) {
        if (deckTree->indexOfTopLevelItem(currentDeckItem) == -1 &&
            !currentDeckItem->parent()) {
            currentDeckItem = nullptr;
        }
    }

    frontLabel = nullptr;
    backLabel = nullptr;
    feedbackLabel = nullptr;
    actionButton = nullptr;
    nextButton = nullptr;
    prevButton = nullptr;
    directionButton = nullptr;

    cardArea = nullptr;
    actionArea = nullptr;
    choicesContainer = nullptr;
    choiceListWidget = nullptr;
    ratingContainer = nullptr;
    startQuizButton = nullptr;
    shuffleButton = nullptr;
    numQuestionsSpinBox = nullptr;
    deckMasteryRadial = nullptr;
    deckTitle = nullptr;
    countLabel = nullptr;
    quizStyleGroup = nullptr;

    quizWidget = nullptr;
    resultsWidget = nullptr;
    currentDeckContainer = nullptr;

    cardRowsLayout = nullptr;
    cardContainer = nullptr;

    correctButton = nullptr;
    wrongButton = nullptr;

    choiceButtons.clear();
    choiceLabels.clear();
    allDeckBacks.clear();

    if (mainContentLayout) {
        QLayoutItem *item;
        while ((item = mainContentLayout->takeAt(0)) != nullptr) {
            if (QWidget *w = item->widget()) {
                w->hide();
                w->setParent(nullptr);
                w->deleteLater();
            }
            delete item;
        }
    }

    if (progressActionLeft)   { navToolBar->removeAction(progressActionLeft);   progressActionLeft = nullptr; }
    if (progressActionCenter) { navToolBar->removeAction(progressActionCenter); progressActionCenter = nullptr; }
    if (progressActionRight)  { navToolBar->removeAction(progressActionRight);  progressActionRight = nullptr; }

    if (quizProgressLabel) {
        quizProgressLabel->deleteLater();
        quizProgressLabel = nullptr;
    }

    QCoreApplication::processEvents();

    isSettingsPage = false;
    inQuizMode = false;
    currentCardIndex = 0;
    score = 0;

    resultsWidget = nullptr;
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
    if (inQuizMode) return;

    resetMainContent();

    currentDeckContainer = new QWidget();
    currentDeckContainer->setStyleSheet("background-color: #4a5259;");
    QVBoxLayout *deckLayout = new QVBoxLayout(currentDeckContainer);
    deckLayout->setSpacing(12);
    deckLayout->setContentsMargins(16, 16, 16, 16);

    QWidget *topArea = new QWidget();
    topArea->setStyleSheet("background-color: #2c3e50; border-radius: 8px; padding: 15px 0px 0px 15px;");
    topArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    QVBoxLayout *topVL = new QVBoxLayout(topArea);
    topVL->setSpacing(16);

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
    deckMasteryRadial->setStyleSheet(R"(
        QToolTip {
            background-color: #34495e;
            color: white;
            border: 2px solid #3498db;
            border-radius: 8px;
            padding: 2px 2px;
            font-size: 15px;
            font-weight: bold;
        }
    )");
    radialL->addWidget(deckMasteryRadial);

    deckTitle = new QLabel(deckItem->text(0), topArea);
    deckTitle->setWordWrap(true);
    deckTitle->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    deckTitle->setStyleSheet("font-size: 28px; font-weight: bold; color: white; line-height: 1.25;");

    int cardCount = currentDeckItem->data(0, Qt::UserRole + 1).toJsonArray().size();
    countLabel = new QLabel(QString("%1 cards").arg(cardCount), topArea);
    countLabel->setStyleSheet("font-size: 16px; color: #bdc3c7;");
    countLabel->setContentsMargins(0, 0, 15, 0);

    titleRow->addWidget(radialContainer, 0, Qt::AlignTop);
    titleRow->addWidget(deckTitle, 1);
    titleRow->addStretch();
    titleRow->addWidget(countLabel, 0, Qt::AlignTop);
    topVL->addLayout(titleRow);

    QWidget *quizControlsContainer = new QWidget(topArea);
    FlowLayout *flowLayout = new FlowLayout(quizControlsContainer, 0, 12, 10);
    quizControlsContainer->setLayout(flowLayout);
    quizControlsContainer->setMinimumWidth(0);
    flowLayout->setAlignment(Qt::AlignHCenter);
    quizControlsContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    flowLayout->setContentsMargins(18, 0, 0, 0);

    // Start Quiz Button
    startQuizButton = new QPushButton("Start Quiz", topArea);
    startQuizButton->setFixedWidth(190);
    startQuizButton->setFixedHeight(50);
    startQuizButton->setStyleSheet(R"(
        QPushButton { background-color: #3498db; color: white; padding: 14px 24px;
                      font-size: 15px; font-weight: bold; border-radius: 10px;
                      border: none; }
        QPushButton:hover { background-color: #2980b9; border: 2px solid white; }
        QPushButton:disabled { background-color: #7f8c8d; color: #bdc3c7; }
    )");
    auto updateStartButton = [this]() {
        if (!startQuizButton) return;
        int cc = currentDeckItem ? currentDeckItem->data(0, Qt::UserRole + 1).toJsonArray().size() : 0;
        bool isMCQ = !lastUsedFlashcardMode;
        startQuizButton->setEnabled((cc > 0) && !(isMCQ && cc == 1));
    };
    updateStartButton();
    connect(startQuizButton, &QPushButton::clicked, this, &MainWindow::startQuiz);

    // Quiz Type Button
    QPushButton *quizTypeBtn = new QPushButton(topArea);
    quizTypeBtn->setCheckable(true);
    quizTypeBtn->setChecked(lastUsedFlashcardMode);
    quizTypeBtn->setText(lastUsedFlashcardMode ? "Flashcard Style" : "Multiple Choice");
    quizTypeBtn->setFixedWidth(190);
    quizTypeBtn->setFixedHeight(50);
    quizTypeBtn->setStyleSheet(R"(
        QPushButton { background-color: #27ae60; color: white; padding: 14px 24px;
                      font-size: 15px; font-weight: bold; border-radius: 10px;
                      border: none; }
        QPushButton:hover { border: 2px solid #ffffff; }
    )");
    connect(quizTypeBtn, &QPushButton::clicked, this, [this, quizTypeBtn, updateStartButton](bool checked) {
        lastUsedFlashcardMode = checked;
        quizTypeBtn->setText(checked ? "Flashcard Style" : "Multiple Choice");
        saveSettings();
        updateStartQuizButton();
    });

    // Shuffle Button
    shuffleButton = new QPushButton("Shuffle", topArea);
    shuffleButton->setFixedWidth(190);
    shuffleButton->setFixedHeight(50);
    shuffleButton->setCheckable(true);
    shuffleButton->setChecked(lastUsedShuffle);
    shuffleButton->setStyleSheet(R"(
        QPushButton { background-color: #2c3e50; color: white; padding: 14px 24px;
                      font-size: 15px; font-weight: bold; border-radius: 10px;
                      border: 2px solid #455a6f; }
        QPushButton:checked { background-color: #27ae60; border: none; }
        QPushButton:hover { border: 2px solid #3498db; }
        QPushButton:checked:hover { border: 2px solid #ffffff; }
    )");
    connect(shuffleButton, &QPushButton::toggled, this, [this](bool checked) {
        lastUsedShuffle = checked;
        saveSettings();
    });

    // Direction Button
    directionButton = new QPushButton(topArea);
    directionButton->setFixedWidth(190);
    directionButton->setFixedHeight(50);
    directionButton->setCheckable(false);
    updateDirectionButtonText();
    connect(directionButton, &QPushButton::clicked, this, [this]() {
        lastUsedQuizDirection = (lastUsedQuizDirection == QuizDirection::FrontToBack)
        ? QuizDirection::BackToFront : QuizDirection::FrontToBack;
        updateDirectionButtonText();
        saveSettings();
    });

    // Number of Questions
    QWidget *questionsGroup = new QWidget(topArea);
    QHBoxLayout *qLayout = new QHBoxLayout(questionsGroup);
    qLayout->setContentsMargins(0, 0, 0, 0);
    qLayout->setSpacing(3);

    QLabel *numLabel = new QLabel("Questions:", topArea);
    numLabel->setStyleSheet("color: #bdc3c7; font-size: 15px; font-weight: bold; padding: 14px 4px;");
    flowLayout->addWidget(numLabel);

    numQuestionsSpinBox = new QSpinBox(topArea);
    numQuestionsSpinBox->setMinimum(1);
    numQuestionsSpinBox->setMaximum(9999);
    numQuestionsSpinBox->setFixedWidth(90);
    numQuestionsSpinBox->setMinimumHeight(50);
    numQuestionsSpinBox->setAlignment(Qt::AlignCenter);
    numQuestionsSpinBox->setStyleSheet(R"(
        QSpinBox { background-color: #2c3e50; color: white; border: 2px solid #455a6f;
                   border-radius: 10px; padding: 6px 8px; font-size: 15px; font-weight: bold; }
        QSpinBox:hover { border: 2px solid #3498db; }
        QSpinBox::up-button { width: 25px; background-color: #34495e; border: none; border-top-right-radius: 8px; }
        QSpinBox::down-button { width: 25px; background-color: #34495e; border: none; border-bottom-right-radius: 8px; }
        QSpinBox::up-button:hover, QSpinBox::down-button:hover { background-color: #3498db; }
        QSpinBox::up-arrow { border-left:6px solid #34495e; border-right:6px solid #34495e; border-bottom:8px solid #bdc3c7; width:0; height:0; }
        QSpinBox::down-arrow { border-left:6px solid #34495e; border-right:6px solid #34495e; border-top:8px solid #bdc3c7; width:0; height:0; }
        QSpinBox::up-arrow:hover, QSpinBox::down-arrow:hover { border-left:6px solid #3498db; border-right:6px solid #3498db; }
    )");

    int preferredNum = currentDeckItem->data(0, Qt::UserRole + 3).toInt(0);
    numQuestionsSpinBox->setValue(preferredNum > 0 ? preferredNum : (cardCount > 0 ? cardCount : 10));

    connect(numQuestionsSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this](int value) {
                if (!currentDeckItem) return;
                QSignalBlocker blocker(deckTree);
                currentDeckItem->setData(0, Qt::UserRole + 3, value);
                QTimer::singleShot(30, this, [this]() { saveDecks(); });
            });

    flowLayout->addWidget(startQuizButton);
    flowLayout->addWidget(quizTypeBtn);
    flowLayout->addWidget(shuffleButton);
    flowLayout->addWidget(directionButton);
    qLayout->addWidget(numLabel);
    qLayout->addWidget(numQuestionsSpinBox);
    flowLayout->addWidget(questionsGroup);

    topVL->addWidget(quizControlsContainer);
    topVL->addSpacing(16);

    QTimer::singleShot(0, this, [quizControlsContainer, topArea, topVL, flowLayout]() {
        flowLayout->invalidate();
        quizControlsContainer->updateGeometry();
        topVL->invalidate();
        topVL->activate();
        topArea->updateGeometry();
        topArea->adjustSize();
    });

    deckLayout->addWidget(topArea);

    // Flashcard Section
    QWidget *bottomArea = new QWidget();
    bottomArea->setStyleSheet("background-color: #2c3e50; border-radius: 8px;");
    QVBoxLayout *bottomL = new QVBoxLayout(bottomArea);
    bottomL->setSpacing(10);
    bottomL->setContentsMargins(5, 16, 5, 0);

    QHBoxLayout *headerL = new QHBoxLayout();
    headerL->setSpacing(12);
    headerL->setContentsMargins(64, 0, 12, 0);

    currentFrontHeader = "Question";
    currentBackHeader  = "Answer";

    QVariant frontHeaderVar = deckItem->data(0, Qt::UserRole + 4);
    QVariant backHeaderVar  = deckItem->data(0, Qt::UserRole + 5);
    if (frontHeaderVar.isValid()) currentFrontHeader = frontHeaderVar.toString();
    if (backHeaderVar.isValid())  currentBackHeader  = backHeaderVar.toString();

    QLineEdit *frontHeaderEdit = new QLineEdit(currentFrontHeader, bottomArea);
    QLineEdit *backHeaderEdit  = new QLineEdit(currentBackHeader, bottomArea);

    frontHeaderEdit->setFrame(false);
    backHeaderEdit->setFrame(false);
    frontHeaderEdit->setAlignment(Qt::AlignCenter);
    backHeaderEdit->setAlignment(Qt::AlignCenter);

    frontHeaderEdit->setToolTip("Click to edit label");
    backHeaderEdit->setToolTip("Click to edit label");
    frontHeaderEdit->setCursor(Qt::IBeamCursor);
    backHeaderEdit->setCursor(Qt::IBeamCursor);

    frontHeaderEdit->setStyleSheet(R"(
        QLineEdit {
            background: transparent;
            color: white;
            font-size: 17px;
            font-weight: bold;
            border: none;
            padding: 4px;
        }
        QLineEdit:hover {
            border: 1px solid #5dade2;
            border-radius: 6px;
        }
        QLineEdit:focus {
            border: 2px solid #3498db;
            border-radius: 6px;
        }
    )");
    backHeaderEdit->setStyleSheet(frontHeaderEdit->styleSheet());

    connect(frontHeaderEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
        currentFrontHeader = text.trimmed();
        if (currentDeckItem) markDeckAsDirty();
    });
    connect(backHeaderEdit, &QLineEdit::textChanged, this, [this](const QString &text) {
        currentBackHeader = text.trimmed();
        if (currentDeckItem) markDeckAsDirty();
    });

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

    headerL->addWidget(frontHeaderEdit, 1);
    headerL->addWidget(backHeaderEdit, 1);
    headerL->addSpacing(74);
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
        QList<QPair<QString,QString>> folderCards = getAllCardsInFolder(selectedItem);
        bool hasCards = !folderCards.isEmpty();

        folderBtn = new QPushButton("📁 This Folder", content);
        folderBtn->setToolTip(hasCards ?
                                  "Starts a quiz with 10 random flashcards taken from all the decks of this selected folder" :
                                  "Starts a quiz with 10 random flashcards taken from all the decks of this selected folder\n(This folder is empty - add some decks/cards first)");

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

        folderBtn->setEnabled(hasCards);
        connect(folderBtn, &QPushButton::clicked, this, &MainWindow::startFolderQuiz);
        folderBtn->update();
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

    auto createBoxedRow = [&](const QString& title, const QList<QTreeWidgetItem*>& decks, const QString& accentColor) -> QWidget* {
        if (decks.isEmpty()) return nullptr;
        QWidget* box = new QWidget();
        box->setStyleSheet("background-color: #2c3e50; border-radius: 8px;");
        box->setContentsMargins(15, 15, 0, 0);
        QVBoxLayout* boxL = new QVBoxLayout();
        box->setLayout(boxL);
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

    if (item == currentDeckItem && numQuestionsSpinBox != nullptr) {
        saveDecks();
        if (deckTitle) {
            deckTitle->setText(item->text(0));
        }
        return;
    }

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

    QTextEdit *qEdit = new QTextEdit(rowWidget);
    QTextEdit *aEdit = new QTextEdit(rowWidget);
    qEdit->setPlainText(front);
    aEdit->setPlainText(back);

    qEdit->setAcceptRichText(false);
    aEdit->setAcceptRichText(false);

    QFont textFont("Segoe UI", 14);
    qEdit->setFont(textFont);
    aEdit->setFont(textFont);
    qEdit->setPlaceholderText("Question Text");
    aEdit->setPlaceholderText("Answer Text");
    setupCardTextEdit(qEdit);
    setupCardTextEdit(aEdit);
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
                    border-radius: 12px; padding: 12px; text-align: center; }
        QTextEdit:placeholder { text-align: center; color: #95a5a6; }
        QTextEdit:hover { border: 2px solid #3498db; }
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

    QTimer::singleShot(0, this, [this, qEdit, aEdit]() {
        if (qEdit) resizeRowToContent(qEdit);
        if (aEdit) resizeRowToContent(aEdit);
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
    resizeRowToContent(frontEdit);
}

void MainWindow::addCardRow(const QString &front, const QString &back, int mastery)
{
    if (!cardRowsLayout) return;

    QWidget *newRow = createCardRow(front, back, mastery);
    cardRowsLayout->addWidget(newRow);

    QTimer::singleShot(10, this, [this, newRow]() {
        QHBoxLayout *rowL = qobject_cast<QHBoxLayout*>(newRow->layout());
        if (rowL) {
            QTextEdit *qEdit = qobject_cast<QTextEdit*>(rowL->itemAt(1)->widget());
            QTextEdit *aEdit = qobject_cast<QTextEdit*>(rowL->itemAt(2)->widget());
            if (qEdit) resizeRowToContent(qEdit);
            if (aEdit) resizeRowToContent(aEdit);
        }
    });

    updateNumQuestionsRange();
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
    if (viewportW < 100) return;

    qEdit->document()->setTextWidth(viewportW);
    aEdit->document()->setTextWidth(viewportW);

    int qHeight = static_cast<int>(qEdit->document()->size().height()) + 24;
    int aHeight = static_cast<int>(aEdit->document()->size().height()) + 24;
    int finalTextHeight = qMax(qMax(qHeight, aHeight), 82);

    if (qAbs(qEdit->height() - finalTextHeight) > 5)
        qEdit->setFixedHeight(finalTextHeight);
    if (qAbs(aEdit->height() - finalTextHeight) > 5)
        aEdit->setFixedHeight(finalTextHeight);

    int rowHeight = finalTextHeight + 32;
    if (qAbs(rowWidget->height() - rowHeight) > 5) {
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

    if (countLabel) {
        int newCount = cardsArray.size();
        countLabel->setText(QString("%1 cards").arg(newCount));
    }

    saveDecks();
    deckIsDirty = false;
    updateSaveButtonState();

    if (startQuizButton) {
        updateStartQuizButton();
    }
    if (currentDeckItem) {
        currentDeckItem->setData(0, Qt::UserRole + 4, currentFrontHeader);
        currentDeckItem->setData(0, Qt::UserRole + 5, currentBackHeader);
    }
    updateNumQuestionsRange();
    if (deckMasteryRadial && currentDeckItem) {
        deckMasteryRadial->setValue(getDeckAverageMastery(currentDeckItem));
    }
}

// Update Mastery
void MainWindow::applyMasteryFromQuiz()
{
    if (!currentDeckItem) return;
    QJsonArray cards = currentDeckItem->data(0, Qt::UserRole + 1).toJsonArray();
    bool changed = false;
    for (const auto &resultPair : quizResults) {
        const QPair<QString, QString>& quizCard = resultPair.first;
        int result = resultPair.second;
        if (result == -1) continue;
        for (int j = 0; j < cards.size(); ++j) {
            QJsonObject cardObj = cards[j].toObject();
            QString deckFront = cardObj["front"].toString();
            QString deckBack  = cardObj["back"].toString();
            if (deckFront == quizCard.first && deckBack == quizCard.second) {
                int oldMastery = cardObj["mastery"].toInt(0);
                int delta = (result == 1) ? masteryCorrectPoints : masteryIncorrectPoints;
                int newMastery = std::clamp(oldMastery + delta, 0, 100);
                if (newMastery != oldMastery) {
                    cardObj["mastery"] = newMastery;
                    cards[j] = cardObj;
                    changed = true;
                }
                break;
            }
        }
    }
    if (changed) {
        currentDeckItem->setData(0, Qt::UserRole + 1, cards);
        saveDecks();
        if (deckMasteryRadial && currentDeckItem) {
            deckMasteryRadial->setValue(getDeckAverageMastery(currentDeckItem));
        }
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
                if (!confirmExitQuiz()) return true;
                endQuiz();
            }
            QSignalBlocker blocker(deckTree);
            deckTree->clearSelection();
            deckTree->setCurrentItem(nullptr);
            onDeckSelectionChanged();
            return true;
        }
    }

    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::updateAddButtonsState()
{
    QTreeWidgetItem *selected = deckTree->currentItem();
    QString type = selected ? selected->data(0, Qt::UserRole).toString() : QString();

    bool isDeckSelected = (type == "deck");

    addFolderBtn->setEnabled(!isDeckSelected);
    addDeckBtn->setEnabled(!isDeckSelected);

    if (isDeckSelected) {
        addFolderBtn->setToolTip("Cannot add folder or deck inside a deck");
        addDeckBtn->setToolTip("Cannot add folder or deck inside a deck");
    } else {
        addFolderBtn->setToolTip("Add new folder");
        addDeckBtn->setToolTip("Add new deck");
    }
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

    else if (!inQuizMode && cardRowsLayout && cardContainer) {
        QTimer::singleShot(30, this, [this]() {
            updateAllCardHeights();
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

    QTreeWidgetItem *itemToDelete = currentDeckItem;

    {
        QSignalBlocker blocker(deckTree);
        currentDeckItem = nullptr;
        delete itemToDelete;

        deckTree->clearSelection();
        deckTree->setCurrentItem(nullptr);
        deckTree->viewport()->update();
    }

    resetMainContent();
    saveDecks();
    updateToolbarActions();
    updateAddButtonsState();
    showHomePage();
}

void MainWindow::deleteCurrentFolder()
{
    QTreeWidgetItem *item = deckTree->currentItem();
    if (!item || item->data(0, Qt::UserRole).toString() != "folder") return;

    {
        QSignalBlocker blocker(deckTree);
        currentDeckItem = nullptr;
        delete item;

        deckTree->clearSelection();
        deckTree->setCurrentItem(nullptr);
        deckTree->viewport()->update();
    }

    resetMainContent();
    saveDecks();
    updateToolbarActions();
    updateAddButtonsState();
    showHomePage();
}

void MainWindow::confirmDeleteCurrentDeck()
{
    if (!currentDeckItem) return;

    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Delete Deck");
    msgBox.setText(QString("Delete deck \"%1\" and all its flashcards?\n\n"
                           "This action cannot be undone.")
                       .arg(currentDeckItem->text(0)));
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);

    if (msgBox.exec() == QMessageBox::Yes) {
        deleteCurrentDeck();
    }
}

void MainWindow::confirmDeleteCurrentFolder()
{
    QTreeWidgetItem *item = deckTree->currentItem();
    if (!item || item->data(0, Qt::UserRole).toString() != "folder") return;

    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Delete Folder");
    msgBox.setText(QString("Delete folder \"%1\" and ALL decks + flashcards inside it?\n\n"
                           "This action cannot be undone.")
                       .arg(item->text(0)));
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);

    if (msgBox.exec() == QMessageBox::Yes) {
        deleteCurrentFolder();
    }
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

        int preferredNum = source->data(0, Qt::UserRole + 3).toInt(0);
        if (preferredNum > 0) {
            newItem->setData(0, Qt::UserRole + 3, preferredNum);
        }
    }
    else if (type == "folder") {
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

void MainWindow::updateStartQuizButton()
{
    if (startQuizButton) {
        int cardCount = currentDeckItem ?
                            currentDeckItem->data(0, Qt::UserRole + 1).toJsonArray().size() : 0;

        bool isMCQ = !lastUsedFlashcardMode;
        bool enabled = (cardCount > 0) && !(isMCQ && cardCount == 1);
        startQuizButton->setEnabled(enabled);
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
    cardContainer->updateGeometry();
    QTimer::singleShot(30, this, [this]() {
        updateAllCardHeights();
    });
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

    int desiredQuestions = 10;

    if (!isReviewMode && currentDeckItem && numQuestionsSpinBox) {
        desiredQuestions = numQuestionsSpinBox->value();
    }

    resetMainContent();

    bool usingSavedQuiz = false;
    if (useExactQuizCards && !pendingExactQuizCards.isEmpty()) {
        quizCardList = pendingExactQuizCards;
        usingSavedQuiz = true;
        useExactQuizCards = false;
        pendingExactQuizCards.clear();

        if (lastUsedShuffle && quizCardList.size() > 1) {
            std::random_device rd;
            std::mt19937 g(rd());
            std::shuffle(quizCardList.begin(), quizCardList.end(), g);
        }
    }

    allDeckBacks.clear();

    if (usingSavedQuiz && !quizCardList.isEmpty()) {
        if (currentDeckItem) {
            QJsonArray cardsJson = currentDeckItem->data(0, Qt::UserRole + 1).toJsonArray();
            for (const QJsonValue &val : cardsJson) {
                QString back = val.toObject()["back"].toString();
                if (!back.isEmpty() && !allDeckBacks.contains(back))
                    allDeckBacks << back;
            }
        } else {
            QList<QPair<QString, QString>> allCards = getAllLibraryCards();
            for (const auto &p : allCards) {
                if (!p.second.isEmpty() && !allDeckBacks.contains(p.second))
                    allDeckBacks << p.second;
            }
        }
    }

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
    int targetSize = (!isReviewMode && desiredQuestions > 0)
                         ? desiredQuestions
                         : totalAvailable;
    if (targetSize < 1) targetSize = 1;

    if (!usingSavedQuiz) {
        if (targetSize > totalAvailable && totalAvailable > 0) {
            QList<QPair<QString, QString>> basePool = quizCardList;

            if (userWantsShuffle) {
                std::random_device rd;
                std::mt19937 g(rd());
                std::shuffle(basePool.begin(), basePool.end(), g);
            }

            quizCardList.clear();
            while (quizCardList.size() < targetSize) {
                quizCardList += basePool;
            }
            quizCardList.resize(targetSize);
        } else {
            if (userWantsShuffle && quizCardList.size() > 1) {
                std::random_device rd;
                std::mt19937 g(rd());
                std::shuffle(quizCardList.begin(), quizCardList.end(), g);
            }
            quizCardList.resize(targetSize);
        }
    } else {
        quizCardList.resize(targetSize);
    }

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
        connect(nextButton, &QPushButton::clicked, this, [this]() {
            if (!answered) {
                if (currentCardIndex < quizResults.size()) {
                    quizResults[currentCardIndex].second = -1;
                }
            }

            currentCardIndex++;

            if (currentCardIndex >= quizCardList.size()) {
                showResultsPage();
            } else {
                loadCurrentQuestion();
            }
        });

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
        QString displayBack = getDisplayedBack(quizCardList[currentCardIndex]);

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
    inQuizMode = false;
    currentCardIndex = 0;
    score = 0;
    cardFlipped = false;
    answered = false;
    isReviewMode = false;
    useExactQuizCards = false;
    pendingExactQuizCards.clear();
    quizCardList.clear();
    quizResults.clear();

    quizWidget = nullptr;
    resultsWidget = nullptr;
    frontLabel = nullptr;
    backLabel = nullptr;
    feedbackLabel = nullptr;
    actionButton = nullptr;
    nextButton = nullptr;
    prevButton = nullptr;
    directionButton = nullptr;
    ratingContainer = nullptr;
    correctButton = nullptr;
    wrongButton = nullptr;
    choiceButtons.clear();
    choiceLabels.clear();

    if (mainContentLayout) {
        QLayoutItem *item;
        while ((item = mainContentLayout->takeAt(0)) != nullptr) {
            if (QWidget *w = item->widget()) {
                w->hide();
                w->setParent(nullptr);
                w->deleteLater();
            }
            delete item;
        }
    }
}

void MainWindow::handleEndQuizClick()
{
    if (!inQuizMode || !endQuizAction) return;
    if (endQuizConfirmPending) {
        endQuizConfirmPending = false;
        endQuizAction->setText("End Quiz");
        endQuizAction->setIcon(QIcon::fromTheme("process-stop", QIcon::fromTheme("dialog-cancel")));
        if (QToolButton* btn = qobject_cast<QToolButton*>(navToolBar->widgetForAction(endQuizAction)))
            btn->setStyleSheet("");

        endQuiz();

        QTimer::singleShot(0, this, [this]() {
            returnToPreviousLocation();
        });
    } else {
        endQuizConfirmPending = true;
        endQuizAction->setText("You sure?");
        endQuizAction->setIcon(QIcon::fromTheme("dialog-warning"));
        if (QToolButton* btn = qobject_cast<QToolButton*>(navToolBar->widgetForAction(endQuizAction)))
            btn->setStyleSheet("QToolButton { background-color: #e74c3c; color: white; font-weight: bold; }");

        QTimer::singleShot(5000, this, [this]() {
            if (endQuizConfirmPending) {
                endQuizConfirmPending = false;
                if (endQuizAction) {
                    endQuizAction->setText("End Quiz");
                    endQuizAction->setIcon(QIcon::fromTheme("process-stop", QIcon::fromTheme("dialog-cancel")));
                    if (QToolButton* btn = qobject_cast<QToolButton*>(navToolBar->widgetForAction(endQuizAction)))
                        btn->setStyleSheet("");
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

void MainWindow::returnToPreviousLocation()
{
    if (currentDeckItem && currentDeckItem->data(0, Qt::UserRole).toString() == "deck") {
        showDeckContent(currentDeckItem);
    } else {
        showHomePage();
    }
    updateToolbarActions();
    updateAddButtonsState();
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
            btn->setStyleSheet("QToolButton { background-color: #e74c3c; color: white; font-weight: bold; border: 1px solid #e74c3c; border-radius: 5px; padding: 3px 0px }"
                               "QToolButton:hover { border: 1px solid #ffffff; }");
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
    QTimer::singleShot(0, this, [this]() {
        applyMasteryFromQuiz();

        if (currentDeckItem) {
            updateDeckLastQuiz(currentDeckItem);
        }
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
        int wrongCount = 0;
        for (const auto &r : quizResults) {
            if (r.second == 1) correctCount++;
            else if (r.second == 0) wrongCount++;
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
            QString statusText, rowStyle, statusColor;

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

            QLabel *front = new QLabel(row);
            setCardText(front, r.first.first);
            front->setStyleSheet("font-size: 20px; font-weight: bold; color: white;");
            front->setWordWrap(true);

            QLabel *back = new QLabel(row);
            setCardText(back, r.first.second);
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

        // Bottom buttons
        QWidget *btnRow = new QWidget();
        QHBoxLayout *btnL = new QHBoxLayout(btnRow);
        btnL->setSpacing(20);

        QPushButton *retakeBtn = new QPushButton("Retake Quiz", btnRow);
        QPushButton *reviewBtn = new QPushButton("Review Wrong Answers", btnRow);
        QPushButton *backBtn = new QPushButton("Back to Deck", btnRow);

        QString blueStyle = R"(
            QPushButton { background-color: #3498db; color: white; padding: 14px 32px;
                          font-size: 16px; font-weight: bold; border-radius: 10px; border: none; }
            QPushButton:hover { background-color: #4aa3df; border: 2px solid #ffffff; padding: 12px 30px; }
        )";
        QString grayStyle = R"(
            QPushButton { background-color: #2c3e50; color: white; padding: 14px 32px;
                          font-size: 16px; font-weight: bold; border-radius: 10px; border: none; }
            QPushButton:hover { background-color: #34495e; border: 2px solid #3498db; padding: 12px 30px; }
        )";

        retakeBtn->setStyleSheet(blueStyle);
        reviewBtn->setStyleSheet(blueStyle);
        backBtn->setStyleSheet(grayStyle);

        reviewBtn->setEnabled(wrongCount > 0);
        if (wrongCount == 0) reviewBtn->setText("No mistakes 🎉");

        connect(retakeBtn, &QPushButton::clicked, this, [this]() {
            QTimer::singleShot(0, this, [this]() {
                isReviewMode = false;
                if (!quizCardList.isEmpty()) {
                    pendingExactQuizCards = quizCardList;
                    useExactQuizCards = true;
                }
                startQuiz();
            });
        });

        connect(reviewBtn, &QPushButton::clicked, this, [this]() {
            QTimer::singleShot(0, this, [this]() {
                QList<QPair<QString, QString>> wrongCards;
                for (const auto &r : quizResults) {
                    if (r.second == 0) wrongCards << r.first;
                }
                if (wrongCards.isEmpty()) {
                    QMessageBox::information(this, "All Good!", "No wrong answers to review!");
                    return;
                }
                pendingExactQuizCards = wrongCards;
                useExactQuizCards = true;
                isReviewMode = true;
                startQuiz();
            });
        });

        connect(backBtn, &QPushButton::clicked, this, [this]() {
            QTimer::singleShot(0, this, [this]() {
                endQuiz();

                if (currentDeckItem && currentDeckItem->data(0, Qt::UserRole).toString() == "deck") {
                    showDeckContent(currentDeckItem);
                } else {
                    showHomePage();
                }
                updateToolbarActions();
                updateAddButtonsState();
            });
        });

        btnL->addWidget(retakeBtn);
        btnL->addWidget(reviewBtn);
        btnL->addWidget(backBtn);
        resLayout->addWidget(btnRow);

        QTimer::singleShot(0, this, [this]() {
            if (mainContentLayout && resultsWidget) {
                mainContentLayout->addWidget(resultsWidget);
            }
        });
    });
}

void MainWindow::loadCurrentQuestion()
{
    if (currentCardIndex >= quizCardList.size()) {
        endQuiz();
        return;
    }

    if (currentCardIndex >= quizResults.size()) {
        quizResults.resize(quizCardList.size(), qMakePair(QPair<QString,QString>(), -1));
    }

    answered = false;
    auto card = quizCardList[currentCardIndex];

    QString displayFront = getDisplayedFront(card);
    QString displayBack  = getDisplayedBack(card);

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
            setCardText(backLabel, displayBack);
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
        answered = false;

        auto card = quizCardList[currentCardIndex];

        QString questionSide  = getDisplayedFront(card);
        QString correctAnswer = getDisplayedBack(card);

        frontLabel->setText(questionSide);
        adjustCardFontSize(frontLabel, questionSide, true);

        if (feedbackLabel) feedbackLabel->setText("");
        if (nextButton) nextButton->setEnabled(false);

        resetChoiceButtonStyles();

        bool questionIsOriginalFront = (questionSide == card.first);

        QStringList options = {correctAnswer};
        QStringList possibleDistractors;

        if (!allDeckBacks.isEmpty()) {
            for (const QString &answer : allDeckBacks) {
                if (!answer.trimmed().isEmpty() && answer != correctAnswer) {
                    possibleDistractors << answer;
                }
            }
        } else {
            for (const auto &c : quizCardList) {
                QString distractor = questionIsOriginalFront ? c.second : c.first;
                if (!distractor.trimmed().isEmpty() &&
                    distractor != correctAnswer &&
                    !possibleDistractors.contains(distractor)) {
                    possibleDistractors << distractor;
                }
            }
        }

        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(possibleDistractors.begin(), possibleDistractors.end(), g);

        for (int i = 0; i < possibleDistractors.size() && options.size() < 4; ++i) {
            options << possibleDistractors[i];
        }

        std::shuffle(options.begin(), options.end(), g);

        for (int i = 0; i < 4 && i < choiceButtons.size(); ++i) {
            if (i < options.size()) {
                setCardText(choiceLabels[i], options[i]);
                choiceButtons[i]->setVisible(true);
                choiceButtons[i]->setEnabled(true);
            } else {
                choiceButtons[i]->setVisible(false);
            }
        }

        if (prevButton) prevButton->setEnabled(currentCardIndex > 0);
        bool isLast = (currentCardIndex == quizCardList.size() - 1);
        if (nextButton) nextButton->setText(isLast ? "Finish Quiz" : "Next →");
    }
}

void MainWindow::resetChoiceButtonStyles()
{
    QString defaultChoiceStyle = R"(
        QPushButton {
            background-color: #2c3e50;
            border: 3px solid #455a6f;
            border-radius: 12px;
            padding: 14px 16px;
            font-size: 17px;
            font-weight: bold;
            color: white;
        }
        QPushButton:hover {
            border-color: #3498db;
        }
    )";

    for (QPushButton *btn : choiceButtons) {
        if (btn) btn->setStyleSheet(defaultChoiceStyle);
        btn->setEnabled(true);
    }
}

void MainWindow::adjustCardFontSize(QLabel* label, const QString& text, bool isMultipleChoice)
{
    if (text.isEmpty()) {
        label->clear();
        return;
    }

    setCardText(label, text);
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
    QString correctAnswer = getDisplayedBack(quizCardList[currentCardIndex]);
    bool isCorrect = (selectedText == correctAnswer);

    quizResults[currentCardIndex].second = isCorrect ? 1 : 0;
    if (isCorrect) score++;

    for (int i = 0; i < choiceButtons.size(); ++i) {
        QPushButton *btn = choiceButtons[i];
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
    if (inQuizMode) {
        if (!confirmExitQuiz()) {
            return;
        }
        endQuiz();
    }

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
    connect(backBtn, &QPushButton::clicked, this, &MainWindow::returnToPreviousLocation);

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
    startRow->setSpacing(15);

    QHBoxLayout *startContent = new QHBoxLayout();
    startContent->setSpacing(12);

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

    startContent->addWidget(startLabel);
    startContent->addWidget(typeCombo);
    startContent->addWidget(targetCombo);

    startRow->addStretch(1);
    startRow->addLayout(startContent);
    startRow->addStretch(1);

    generalL->addLayout(startRow);

    mainL->addWidget(generalGroup);

    // Mastery Settings
    QGroupBox *masteryGroup = new QGroupBox("Mastery", settingsWidget);
    masteryGroup->setStyleSheet("QGroupBox { background-color: #2c3e50; font-weight: bold; font-size: 18px; color: white; padding: 10px; border-radius: 8px; }");
    QVBoxLayout *masteryL = new QVBoxLayout(masteryGroup);
    masteryL->setContentsMargins(15, 30, 15, 15);
    masteryL->setSpacing(18);

    QHBoxLayout *masteryRow = new QHBoxLayout();
    masteryRow->setSpacing(12);

    QHBoxLayout *contentGroup = new QHBoxLayout();
    contentGroup->setSpacing(12);

    QLabel *masteryRateLabel = new QLabel("Mastery gain / penalty per answer:", settingsWidget);
    masteryRateLabel->setStyleSheet("background-color: #2c3e50; color: #bdc3c7; font-size: 15px;");

    QSpinBox *correctSpin = new QSpinBox(settingsWidget);
    correctSpin->setMinimum(0);
    correctSpin->setMaximum(100);
    correctSpin->setValue(masteryCorrectPoints);
    correctSpin->setSuffix(" pts");
    correctSpin->setFixedWidth(110);
    correctSpin->setAlignment(Qt::AlignCenter);

    QSpinBox *wrongSpin = new QSpinBox(settingsWidget);
    wrongSpin->setMinimum(0);
    wrongSpin->setMaximum(100);
    wrongSpin->setValue(qAbs(masteryIncorrectPoints));
    wrongSpin->setSuffix(" pts");
    wrongSpin->setPrefix("-");
    wrongSpin->setFixedWidth(110);
    wrongSpin->setAlignment(Qt::AlignCenter);

    QString spinStyle = R"(
        QSpinBox {
            background-color: #2c3e50;
            color: white;
            border: 2px solid #455a6f;
            border-radius: 10px;
            padding: 4px 8px;
            font-size: 15px;
            font-weight: bold;
        }
        QSpinBox:hover { border: 2px solid #3498db; }
        QSpinBox::up-button { width: 25px; background-color: #34495e; border: none; border-top-right-radius: 8px; }
        QSpinBox::down-button { width: 25px; background-color: #34495e; border: none; border-bottom-right-radius: 8px; }
        QSpinBox::up-button:hover, QSpinBox::down-button:hover { background-color: #3498db; }
        QSpinBox::up-arrow { border-left:6px solid #34495e; border-right:6px solid #34495e; border-bottom:8px solid #bdc3c7; width:0; height:0; }
        QSpinBox::down-arrow { border-left:6px solid #34495e; border-right:6px solid #34495e; border-top:8px solid #bdc3c7; width:0; height:0; }
        QSpinBox::up-arrow:hover, QSpinBox::down-arrow:hover { border-left:6px solid #3498db; border-right:6px solid #3498db; }
    )";

    correctSpin->setStyleSheet(spinStyle);
    wrongSpin->setStyleSheet(spinStyle);

    connect(correctSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this](int val){ masteryCorrectPoints = val; saveSettings(); });
    connect(wrongSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, [this](int val){ masteryIncorrectPoints = -val; saveSettings(); });

    contentGroup->addWidget(masteryRateLabel);
    contentGroup->addWidget(correctSpin);
    contentGroup->addWidget(wrongSpin);

    // Restore Defaults button
    QPushButton *restoreDefaultsBtn = new QPushButton("Restore Defaults", settingsWidget);
    restoreDefaultsBtn->setToolTip("Reset to +10 / -5");
    restoreDefaultsBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #f39c12;
            color: white;
            padding: 12px 24px;
            font-size: 15px;
            font-weight: bold;
            border-radius: 8px;
            border: none;
            min-width: 160px;
            margin-left: 10px;
        }
        QPushButton:hover { background-color: #e67e22; border: 2px solid white; }
        QPushButton:disabled { background-color: #7f8c8d; color: #bdc3c7; }
    )");

    auto updateRestoreButton = [restoreDefaultsBtn, correctSpin, wrongSpin, this]() {
        bool isDefault = (correctSpin->value() == 10 && wrongSpin->value() == 5);
        restoreDefaultsBtn->setEnabled(!isDefault);
    };

    connect(correctSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, updateRestoreButton);
    connect(wrongSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, updateRestoreButton);

    updateRestoreButton();

    connect(restoreDefaultsBtn, &QPushButton::clicked, this, [this, correctSpin, wrongSpin]() {
        masteryCorrectPoints = 10;
        masteryIncorrectPoints = -5;
        correctSpin->setValue(10);
        wrongSpin->setValue(5);
        saveSettings();
        QMessageBox::information(this, "Defaults Restored", "Mastery points reset to +10 / -5.");
    });

    contentGroup->addWidget(restoreDefaultsBtn);

    masteryRow->addStretch(1);
    masteryRow->addLayout(contentGroup);
    masteryRow->addStretch(1);

    masteryL->addLayout(masteryRow);

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
        QPushButton:hover { background-color: #e74c3c; border: 2px solid white; }
    )");
    connect(resetMasteryBtn, &QPushButton::clicked, this, &MainWindow::resetAllMasteries);

    QHBoxLayout *resetRow = new QHBoxLayout();
    resetRow->addStretch(1);
    resetRow->addWidget(resetMasteryBtn);
    resetRow->addStretch(1);
    masteryL->addLayout(resetRow);

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
    if (!deck || deck->data(0, Qt::UserRole).toString() != "deck") {
        return;
    }
    deck->setData(0, Qt::UserRole + 2, QDateTime::currentDateTime().toString(Qt::ISODate));
    saveDecks();
}

int MainWindow::getTotalDecks()
{
    if (!deckTree) return 0;
    return collectDecksRecursive(deckTree->invisibleRootItem()).size();
}

int MainWindow::getTotalCards()
{
    if (!deckTree) return 0;
    int total = 0;
    auto decks = collectDecksRecursive(deckTree->invisibleRootItem());
    for (auto d : decks) {
        total += d->data(0, Qt::UserRole + 1).toJsonArray().size();
    }
    return total;
}

int MainWindow::getOverallMastery()
{
    if (!deckTree) return 0;
    auto all = collectDecksRecursive(deckTree->invisibleRootItem());
    if (all.isEmpty()) return 0;
    int sum = 0;
    for (auto d : all) {
        sum += getDeckAverageMastery(d);
    }
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
    lastUsedShuffle = true;
    lastUsedQuizDirection = QuizDirection::FrontToBack;
    startQuiz();
}

void MainWindow::startRandomDeckQuiz()
{
    QList<QTreeWidgetItem*> allDecks = collectDecksRecursive(deckTree->invisibleRootItem());
    if (allDecks.isEmpty()) {
        QMessageBox::information(this, "No Decks", "You have no decks yet!");
        return;
    }
    lastUsedShuffle = true;
    lastUsedQuizDirection = QuizDirection::FrontToBack;

    QTreeWidgetItem *randomDeck = allDecks[QRandomGenerator::global()->bounded(allDecks.size())];
    deckTree->setCurrentItem(randomDeck);
    currentDeckItem = randomDeck;
    if (numQuestionsSpinBox) numQuestionsSpinBox->setValue(10);
    startQuiz();
}

void MainWindow::startLibraryQuiz()
{
    lastUsedShuffle = true;
    lastUsedQuizDirection = QuizDirection::FrontToBack;

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

    lastUsedShuffle = true;
    lastUsedQuizDirection = QuizDirection::FrontToBack;

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

    deckTree->clearSelection();
    currentDeckItem = nullptr;

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
    settingsObj["quizDirection"] = static_cast<int>(lastUsedQuizDirection);
    settingsObj["startOnLaunchType"] = startOnLaunchType;
    settingsObj["startOnLaunchTarget"] = startOnLaunchTarget;
    settingsObj["dailyStreak"] = dailyStreak;
    if (lastStreakDate.isValid())
        settingsObj["lastStreakDate"] = lastStreakDate.toString(Qt::ISODate);

    settingsObj["masteryCorrectPoints"] = masteryCorrectPoints;
    settingsObj["masteryIncorrectPoints"] = masteryIncorrectPoints;

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
        if (settings.contains("quizDirection"))
            lastUsedQuizDirection = static_cast<QuizDirection>(settings["quizDirection"].toInt(0));
        if (settings.contains("startOnLaunchType"))
            startOnLaunchType = settings["startOnLaunchType"].toString("home");
        if (settings.contains("startOnLaunchTarget"))
            startOnLaunchTarget = settings["startOnLaunchTarget"].toString("");

        masteryCorrectPoints = settings.value("masteryCorrectPoints").toInt(10);
        masteryIncorrectPoints = settings.value("masteryIncorrectPoints").toInt(-5);

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

    QLabel *versionLabel = new QLabel("Version 1.2.1", aboutDialog);
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

void MainWindow::updateDirectionButtonText()
{
    if (!directionButton) return;

    QString text = (lastUsedQuizDirection == QuizDirection::FrontToBack)
                       ? "Front → Back"
                       : "Back → Front";

    directionButton->setText(text);
    directionButton->setStyleSheet(R"(
        QPushButton {
            background-color: #8e44ad;
            color: white;
            padding: 14px 24px;
            font-size: 15px;
            font-weight: bold;
            border-radius: 10px;
            border: none;
        }
        QPushButton:hover { border: 2px solid white; }
    )");
}

void MainWindow::expandSubtree(QTreeWidgetItem *item)
{
    if (!item) return;
    item->setExpanded(true);
    for (int i = 0; i < item->childCount(); ++i) {
        expandSubtree(item->child(i));
    }
}

void MainWindow::collapseSubtree(QTreeWidgetItem *item)
{
    if (!item) return;
    item->setExpanded(false);
    for (int i = 0; i < item->childCount(); ++i) {
        collapseSubtree(item->child(i));
    }
}

QString MainWindow::getDisplayedFront(const QPair<QString, QString>& card) const
{
    if (lastUsedQuizDirection == QuizDirection::BackToFront)
        return card.second;
    return card.first;
}

QString MainWindow::getDisplayedBack(const QPair<QString, QString>& card) const
{
    if (lastUsedQuizDirection == QuizDirection::BackToFront)
        return card.first;
    return card.second;
}
