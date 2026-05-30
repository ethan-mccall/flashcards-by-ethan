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

// Tree
class CustomTreeWidget : public QTreeWidget
{
    Q_OBJECT
public:
    explicit CustomTreeWidget(QWidget *parent = nullptr);

protected:
    void dropEvent(QDropEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
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
    // UI / Layout
    Ui::MainWindow *ui = nullptr;
    QToolBar *navToolBar = nullptr;
    QSplitter *splitter = nullptr;
    QWidget *sidePanel = nullptr;
    QWidget *mainContent = nullptr;
    QVBoxLayout *mainContentLayout = nullptr;

    QWidget *currentDeckContainer = nullptr;
    QWidget *cardContainer = nullptr;
    QVBoxLayout *cardRowsLayout = nullptr;

    // Deck view headers
    QLabel *frontHeaderLabel = nullptr;
    QLabel *backHeaderLabel = nullptr;

    // Sidebar
    CustomTreeWidget *deckTree = nullptr;
    QPushButton *addFolderBtn = nullptr;
    QPushButton *addDeckBtn = nullptr;
    QPushButton *randomDeckBtn = nullptr;
    QPushButton *libraryBtn = nullptr;
    QPushButton *folderBtn = nullptr;
    QPushButton *styleToggleBtn = nullptr;
    QLabel *countLabel = nullptr;

    // Toolbar & Actions
    QAction *hamburgerAction = nullptr;
    QAction *settingsAction = nullptr;
    QAction *addCardAction = nullptr;
    QAction *saveDeckAction = nullptr;
    QAction *renameDeckAction = nullptr;
    QAction *deleteDeckAction = nullptr;
    QAction *duplicateDeckAction = nullptr;
    QAction *resetMasteryAction = nullptr;
    QAction *renameFolderAction = nullptr;
    QAction *deleteFolderAction = nullptr;
    QAction *duplicateFolderFullAction = nullptr;
    QAction *duplicateFolderEmptyAction = nullptr;

    QAction *folderSeparator1 = nullptr;
    QAction *folderSeparator2 = nullptr;
    QAction *deckSeparator1 = nullptr;
    QAction *deckSeparator2 = nullptr;
    QAction *endQuizAction = nullptr;

    // Progress actions used in quiz toolbar
    QAction *progressActionLeft = nullptr;
    QAction *progressActionCenter = nullptr;
    QAction *progressActionRight = nullptr;

    // Data & State
    QString decksFilePath;
    QString settingsFilePath;

    QTreeWidgetItem *currentDeckItem = nullptr;
    bool deckIsDirty = false;

    int dailyStreak = 0;
    QDate lastStreakDate;

    int lastSidebarWidth = 320;

    QString currentFrontHeader = "Question";
    QString currentBackHeader  = "Answer";

    // Drag & drop
    QWidget *draggedCard = nullptr;
    QWidget *dropPlaceholder = nullptr;
    QPoint dragOffset;

    MasteryRadial *deckMasteryRadial = nullptr;

    // Quiz Controls
    QPushButton *startQuizButton = nullptr;
    QPushButton *shuffleButton = nullptr;
    QSpinBox *numQuestionsSpinBox = nullptr;
    QPushButton *directionButton = nullptr;

    // Quiz UI elements
    QWidget *quizWidget = nullptr;
    QWidget *resultsWidget = nullptr;
    QWidget *cardArea = nullptr;
    QWidget *actionArea = nullptr;
    QLabel *quizProgressLabel = nullptr;
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
    QButtonGroup *quizStyleGroup = nullptr;

    // Quiz State & Data
    bool inQuizMode = false;
    bool isFlashcardMode = true;
    bool cardFlipped = false;
    bool answered = false;
    int currentCardIndex = 0;
    int score = 0;
    bool isReviewMode = false;

    bool lastUsedFlashcardMode = true;
    bool lastUsedShuffle = true;

    enum class QuizDirection {
        FrontToBack,
        BackToFront
    };
    QuizDirection lastUsedQuizDirection = QuizDirection::FrontToBack;

    // Quiz card lists
    QList<QPair<QString, QString>> quizCardList;
    QList<QPair<QPair<QString, QString>, int>> quizResults;
    QList<QPair<QString, QString>> reviewCardList;
    QList<QPair<QString, QString>> pendingExactQuizCards;
    bool useExactQuizCards = false;
    QStringList allDeckBacks;

    // Confirmation flags
    bool endQuizConfirmPending = false;
    bool deleteDeckConfirmPending = false;
    bool deleteFolderConfirmPending = false;
    bool resetMasteryConfirmPending = false;

    // Settings
    QString startOnLaunchType = "home";
    QString startOnLaunchTarget = "";
    bool isSettingsPage = false;
    int masteryCorrectPoints;
    int masteryIncorrectPoints;

    // Private Helper Methods
    void loadDecks();
    QTreeWidgetItem* loadTreeItem(const QJsonObject &obj, QTreeWidgetItem *parent = nullptr);
    void saveTreeItem(QJsonArray &array, QTreeWidgetItem *item);
    void migrateOldDeckHeaders();
    void updateDirectionButtonText();

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
    void updateStartQuizButton();

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

    // Quiz Logic
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
    void resetChoiceButtonStyles();

    // Settings & Misc
    void showSettingsPage();
    void showChangelog();
    void resetAllMasteries();
    void deleteAllData();
    void populateFolderCombo(QComboBox* combo, QTreeWidgetItem* parent = nullptr, int depth = 0);
    void applyStartOnLaunch();

    // Drag & Drop
    bool eventFilter(QObject *obj, QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

    // Save / Load
    void saveSettings();
    void loadSettings();

    // Context Menu & Confirmations
    void showContextMenu(const QPoint &pos);
    void renameCurrentDeck();
    void renameCurrentFolder();
    void deleteCurrentDeck();
    void deleteCurrentFolder();
    void confirmDeleteCurrentDeck();
    void confirmDeleteCurrentFolder();
    void duplicateCurrentDeck();
    void duplicateFolderFull();
    void duplicateFolderEmpty();

    void handleEndQuizClick();
    void handleDeleteDeckClick();
    void handleDeleteFolderClick();
    void handleResetMasteryClick();

    void onMultipleChoiceSelected(QListWidgetItem *item);
    void onMultipleChoiceButtonClicked(QPushButton *clickedButton);

    // Tree duplication helpers
    QTreeWidgetItem* duplicateTreeItemRecursive(QTreeWidgetItem *source, QTreeWidgetItem *parent);
    QTreeWidgetItem* duplicateFolderStructureOnly(QTreeWidgetItem *source, QTreeWidgetItem *parent);

    // Card helpers
    void setupCardTextEdit(QTextEdit* edit);

    // Display helpers (quiz mode)
    QString getDisplayedFront(const QPair<QString, QString>& card) const;
    QString getDisplayedBack(const QPair<QString, QString>& card) const;

    // Tree expansion helpers
    void expandSubtree(QTreeWidgetItem *item);
    void collapseSubtree(QTreeWidgetItem *item);

    // Private Slots
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
    void returnToPreviousLocation();
};

#endif
