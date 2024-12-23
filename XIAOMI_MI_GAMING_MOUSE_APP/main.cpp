#include "mainwindow.h"

int main(int argc, char *argv[]) {
    QStringList paths = QCoreApplication::libraryPaths();
    paths.append(".");
    paths.append("iconengines");
    paths.append("imageformats");
    paths.append("platforms");
    paths.append("styles");
    QCoreApplication::setLibraryPaths(paths);
    QApplication a(argc, argv);
    a.setStyle("windowsvista");
//    QFontDatabase::addApplicationFont(":/data/tahoma.ttf");
    try {
        MainWindow Mw((argc == 2) && (QString(argv[1]).compare("-h", Qt::CaseInsensitive) == 0));
        return a.exec();
    } catch(const std::runtime_error& error) {
        return 0;
    }
}
