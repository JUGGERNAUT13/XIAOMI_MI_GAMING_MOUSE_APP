#include "mainwindow.h"

int main(int argc, char *argv[]) {
//    uint8_t r = 0;
//    uint8_t g = 0;
//    uint8_t b = 255;
//    mouse_set_color_for_device(TAIL, STATIC, SPEED_8, r, g, b);
////    mouse_non_sleep();

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
    Mw.show();
    return a.exec();
}
