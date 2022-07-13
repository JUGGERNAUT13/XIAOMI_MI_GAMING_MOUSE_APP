QT += core widgets gui

CONFIG += app c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

linux|macx {
    LIBS += -ludev -L$$PWD/lib.linux -lhidapi-hidraw
    PRE_TARGETDEPS += $$PWD/lib.linux/libhidapi-hidraw.a
}

win32 {
    LIBS += -L$$PWD/lib.win32/ -lhidapi
}

INCLUDEPATH += $$PWD

HEADERS += \
    $$PWD/hidapi.h \
    $$PWD/mainwindow.h

SOURCES += \
    $$PWD/main.cpp \
    $$PWD/mainwindow.cpp

FORMS += \
    $$PWD/mainwindow.ui

RESOURCES += \
    images.qrc

CODECFORSRC = UTF-8

system(lrelease \"$$_PRO_FILE_\")
tr.commands = lupdate \"$$_PRO_FILE_\" && lrelease \"$$_PRO_FILE_\"
    PRE_TARGETDEPS += tr
    QMAKE_EXTRA_TARGETS += tr

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
