#include "mainwindow.h"
#include <QApplication>
#include <QIcon>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    a.setApplicationName("Flashcards");
    a.setWindowIcon(QIcon(":/icons/icon.ico"));

    MainWindow w;
    w.setWindowTitle("Flashcards");
    w.setWindowIcon(QIcon(":/icons/icon.ico"));
    w.show();
    return a.exec();
}
