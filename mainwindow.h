/*
 * Flashcards-by-Ethan
 * Copyright (C) 2026 Ethan McCall
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QToolBar>
#include <QAction>
#include <QSplitter>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QMenu>
#include <QScrollArea>
#include <QTextEdit>
#include <QList>
#include <QPair>
#include <QPoint>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QJsonArray>
#include <QButtonGroup>
#include <QListWidget>
#include <QSpinBox>
#include <QPainter>
#include <QPaintEvent>
#include <QDropEvent>
#include <QToolButton>
#include <QComboBox>
#include <QGroupBox>
#include <QDateTime>
#include <QDate>
#include <QVector>

class MasteryRadial;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class CustomTreeWidget : public QTreeWidget
{
    Q_OBJECT
public:
    explicit CustomTreeWidget(QWidget *parent = nullptr);
protected:
    void dropEvent(QDropEvent *event) override;
};

// Main Window
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void saveDecks();

private:
    // UI/Layout
    Ui::MainWindow *ui;
    QToolBar *navToolBar;
    QSplitter *splitter;
    QWidget *sidePanel;
    QWidget *mainContent;
    QVBoxLayout *mainContentLayout = nullptr;

    QWidget *currentDeckContainer = nullptr;
    QWidget *cardContainer = nullptr;
    QVBoxLayout *cardRowsLayout = nullptr;

    // Sidebar
    CustomTreeWidget *deckTree;
    QPushButton *addFolderBtn;
    QPushButton *addDeckBtn;
    QPushButton *randomDeckBtn;
    QPushButton *libraryBtn;
    QPushButton *folderBtn;
    QPushButton *styleToggleBtn;

    // Toolbar Actions
    QAction *hamburgerAction;
    QAction *settingsAction;
    QAction *addCardAction;
    QAction *saveDeckAction;
    QAction *renameDeckAction;
    QAction *deleteDeckAction;
    QAction *duplicateDeckAction;
    QAction *resetMasteryAction = nullptr;
    QAction *renameFolderAction;
    QAction *deleteFolderAction;
    QAction *duplicateFolderFullAction;
    QAction *duplicateFolderEmptyAction;
    QAction *folderSeparator1;
    QAction *folderSeparator2;
    QAction *deckSeparator1;
    QAction *deckSeparator2;
    QAction *endQuizAction;
    QTreeWidgetItem* duplicateFolderStructureOnly(QTreeWidgetItem *source, QTreeWidgetItem *parent);

    // Data/State
    QString decksFilePath;
    QString settingsFilePath;
    QTreeWidgetItem *currentDeckItem = nullptr;
    bool deckIsDirty = false;
    int dailyStreak;
    QDate lastStreakDate;
    int lastSidebarWidth = 320;

    QWidget *draggedCard = nullptr;
    QWidget *dropPlaceholder = nullptr;
    QPoint dragOffset;
    MasteryRadial *deckMasteryRadial = nullptr;

    // Quiz Controls
    QPushButton *startQuizButton = nullptr;
    QPushButton *shuffleButton = nullptr;
    QSpinBox *numQuestionsSpinBox = nullptr;

    // Quiz State
    bool inQuizMode = false;
    bool isFlashcardMode = true;
    bool cardFlipped = false;
    bool answered = false;
    int currentCardIndex = 0;
    int score = 0;
    bool isReviewMode = false;

    // Quiz
    QWidget *quizWidget = nullptr;
    QWidget *resultsWidget = nullptr;
    QWidget *cardArea = nullptr;
    QWidget *actionArea = nullptr;
    QLabel  *quizProgressLabel = nullptr;
    QAction *progressActionLeft   = nullptr;
    QAction *progressActionCenter = nullptr;
    QAction *progressActionRight  = nullptr;
    QLabel *frontLabel = nullptr;
    QLabel *backLabel = nullptr;
    QLabel *feedbackLabel = nullptr;
    QPushButton *actionButton = nullptr;
    QPushButton *nextButton = nullptr;
    QPushButton *prevButton = nullptr;
    QPushButton *correctButton = nullptr;
    QPushButton *wrongButton = nullptr;
    QWidget *ratingContainer = nullptr;
    QListWidget *choiceListWidget = nullptr;
    QList<QPushButton*> choiceButtons;
    QList<QLabel*> choiceLabels;
    QWidget *choicesContainer = nullptr;
    void onMultipleChoiceButtonClicked(QPushButton *clickedButton);
    QButtonGroup *quizStyleGroup = nullptr;

    // Quiz Data
    QList<QPair<QString, QString>> quizCardList;
    QList<QPair<QPair<QString, QString>, int>> quizResults;
    QList<QPair<QString, QString>> reviewCardList;
    QList<QPair<QString, QString>> pendingExactQuizCards;
    bool useExactQuizCards = false;
    QStringList allDeckBacks;

    bool lastUsedFlashcardMode = true;
    bool lastUsedShuffle = true;

    bool endQuizConfirmPending = false;
    bool deleteDeckConfirmPending = false;
    bool deleteFolderConfirmPending = false;
    bool resetMasteryConfirmPending = false;

    // Helpers
    void loadDecks();
    QTreeWidgetItem* loadTreeItem(const QJsonObject &obj, QTreeWidgetItem *parent = nullptr);
    void saveTreeItem(QJsonArray &array, QTreeWidgetItem *item);

    // Deck View
    void showHomePage();
    void showDeckContent(QTreeWidgetItem *deckItem);
    void loadCardsForCurrentDeck();
    void clearMainContent();
    void resetMainContent();
    void onDeckSelectionChanged();
    void onItemChanged(QTreeWidgetItem *item, int column);
    void updateAddButtonsState();
    void ensureSidebarVisible();
    void updateToolbarActions();
    void updateSaveButtonState();
    void markDeckAsDirty();

    // Card Editor
    QWidget* createCardRow(const QString &front = "", const QString &back = "", int mastery = 0);
    void addCardRow(const QString &front = "", const QString &back = "", int mastery = 0);
    void duplicateCardRow(QWidget *sourceRow);
    void removeCardRow(QWidget *rowWidget);
    void saveCurrentDeckCards();
    void resizeRowToContent(QTextEdit *edit);
    void updateAllCardHeights();
    void updateNumQuestionsRange();
    void swapCardFrontAndBack(QWidget *rowWidget);

    // Home Page Helpers
    QList<QTreeWidgetItem*> collectDecksRecursive(QTreeWidgetItem* item) const;
    QPushButton* createDeckCard(QTreeWidgetItem* deckItem);
    QWidget* createHorizontalDeckRow(const QString& title, const QList<QTreeWidgetItem*>& decks, const QString& accentColor);
    QList<QPair<QString, QString>> getAllLibraryCards() const;
    QList<QPair<QString, QString>> getAllCardsInFolder(QTreeWidgetItem* folder) const;
    bool isCardCompletelyEmpty(const QString &front, const QString &back) const;

    // Mastery
    int getDeckAverageMastery(QTreeWidgetItem *deckItem) const;
    void resetDeckMastery();
    void applyMasteryFromQuiz();
    void updateDeckLastQuiz(QTreeWidgetItem *deck);

    // Global Stats
    int getTotalDecks();
    int getTotalCards();
    int getOverallMastery();

    // Quiz
    void startQuiz();
    void startGlobalQuiz(bool flashcardMode);
    void loadCurrentQuestion();
    void flipCard();
    void nextCard();
    void prevCard();
    void markCorrectAndNext();
    void markWrongAndNext();
    void flipOrNextCard();
    void showResultsPage();
    void endQuiz();
    void startRandomDeckQuiz();
    void startLibraryQuiz();
    void startFolderQuiz();
    void adjustCardFontSize(QLabel* label, const QString& text, bool isMultipleChoice = false);

    // Settings
    void showSettingsPage();
    void showChangelog();
    QString startOnLaunchType = "home";
    QString startOnLaunchTarget = "";
    void resetAllMasteries();
    void deleteAllData();
    void populateFolderCombo(QComboBox* combo, QTreeWidgetItem* parent = nullptr, int depth = 0);
    void applyStartOnLaunch();
    bool isSettingsPage;

    // Drag/Drop
    bool eventFilter(QObject *obj, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

    // Save/Load Settings
    void saveSettings();
    void loadSettings();

    // Context Menu/Actions
    void showContextMenu(const QPoint &pos);
    void renameCurrentDeck();
    void renameCurrentFolder();
    void deleteCurrentDeck();
    void deleteCurrentFolder();
    void duplicateCurrentDeck();
    void duplicateFolderFull();
    void duplicateFolderEmpty();

    void handleEndQuizClick();
    void handleDeleteDeckClick();
    void handleDeleteFolderClick();
    void handleResetMasteryClick();

    void onMultipleChoiceSelected(QListWidgetItem *item);

    QTreeWidgetItem* duplicateTreeItemRecursive(QTreeWidgetItem *source, QTreeWidgetItem *parent);

private slots:
    void toggleSidebar();
    void addNewFolder();
    void addNewDeckFromButton();
    void deleteSelectedFolder();
    void checkDailyStreakAtLaunch();
    void updateDailyStreak();
    void resetDailyStreak();
    bool confirmExitQuiz();
    void exportLibrary();
    void importLibrary();
    void showAboutDialog();
    void showFullLicense();
    void mergeImportedItem(const QJsonObject &importedObj, QTreeWidgetItem *targetParent);
};

#endif
