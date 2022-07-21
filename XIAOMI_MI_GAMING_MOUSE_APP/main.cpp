#include "mainwindow.h"

int main(int argc, char *argv[]) {
    QStringList paths = QCoreApplication::libraryPaths();
    paths.append(".");
    paths.append("imageformats");
    paths.append("platforms");
    paths.append("sqldrivers");
    QCoreApplication::setLibraryPaths(paths);
    QApplication a(argc, argv);
    a.setStyle("windowsvista");
//    QFontDatabase::addApplicationFont(":/data/tahoma.ttf");
    MainWindow Mw;
    return a.exec();
}
