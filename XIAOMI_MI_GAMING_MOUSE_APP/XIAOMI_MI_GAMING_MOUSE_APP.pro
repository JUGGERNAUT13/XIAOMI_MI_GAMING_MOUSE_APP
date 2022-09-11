QT += core widgets gui

CONFIG += app c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

INCLUDEPATH += $$PWD

linux|macx {
    LIBS += "$$PWD/lib.linux/libhidapi-hidraw.a" -no-pie
    LIBS += -ludev
}

win32 {
    LIBS += -lhid -lsetupapi
}

HEADERS += \
    $$PWD/general_widget.h \
    $$PWD/hidapi.h \
    $$PWD/mainwindow.h

SOURCES += \
    $$PWD/general_widget.cpp \
    $$PWD/main.cpp \
    $$PWD/mainwindow.cpp

FORMS += \
    $$PWD/dialog_get_color.ui \
    $$PWD/dialog_reset_settings.ui \
    $$PWD/mainwindow.ui

RESOURCES += \
    $$PWD/images.qrc

CODECFORSRC = UTF-8

win32:RC_ICONS += $$PWD/images/icon.ico

system(lrelease \"$$_PRO_FILE_\")
tr.commands = lupdate \"$$_PRO_FILE_\" && lrelease \"$$_PRO_FILE_\"
    PRE_TARGETDEPS += tr
    QMAKE_EXTRA_TARGETS += tr

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
