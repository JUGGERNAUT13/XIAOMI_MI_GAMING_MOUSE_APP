#include "general_widget.h"

general_widget::general_widget() {
#ifdef __linux__
    app_path.append(QProcessEnvironment::systemEnvironment().value(QStringLiteral("APPIMAGE")));        //Path Of Image Deployed with 'linuxdeployqt'
#endif
    if(app_path.count() == 0) {
        app_path.append(qApp->applicationDirPath());
    } else {
        QRegExp tagExp("/");
        QStringList lst = app_path.split(tagExp);
        app_path.clear();
        for(int i = 0; i < (lst.count() - 1); i++) {
            app_path.append("/" + QString(lst.at(i)).remove("/"));
        }
    }
    QString sub_style_sheet = "background-color: lightblue; border-style: outset; color: black; font: bold 10p; border-style: outset; border-width: 1px; border-radius: 0px; border-color: blue; padding: 0px 0px;";
    style_sheet.append("pattern_1 { background-color: #F5F5F5; }"
                       "pattern_2"
                       "QPushButton:checked { " + sub_style_sheet + " }"
                       "QPushButton:pressed { " + sub_style_sheet + " }"
                       "QPushButton:disabled { " + sub_style_sheet + " }"
                       "QPushButton { background-color: lightGray; color: black; border-style: outset; border-width: 1px; border-radius: 0px; border-color: gray; padding: 0px 0px; }"
                       "QPushButton:hover { border-style: outset; border-width: 1px; border-radius: 0px; border-color: royalblue; }"
                       "QPushButton { min-width:73; min-height:21; }"
                      );
}

general_widget::~general_widget() {}

//-------------------------------------------------------------------------
// GET PATH TO CURRENT APPLICATION
//-------------------------------------------------------------------------
QString general_widget::get_app_path() {
    return app_path;
}

QString general_widget::get_color_button_stylesheet(QString color, QString disable_color) {
    QString str = "QRadioButton::indicator { width: 20px; height: 20px; }\n"
                  "QRadioButton::indicator::unchecked::disabled { background-color: qradialgradient(cx: 0.5, cy: 0.5, radius: 0.5, fx: 0.5, fy: 0.5, stop: 0.69 pattern_1, stop: 0.79 transparent); }\n"
                  "QRadioButton::indicator::checked::disabled { background-color: qradialgradient(cx: 0.5, cy: 0.5, radius:0.5, fx: 0.5, fy: 0.5, stop: 0.45 pattern_1, stop: 0.55 transparent, "
                                                     "stop: 0.8 transparent, stop: 0.90 pattern_1, stop: 0.98 transparent); }\n"
                  "QRadioButton::indicator::unchecked { background-color: qradialgradient(cx: 0.5, cy: 0.5, radius: 0.5, fx: 0.5, fy: 0.5, stop: 0.69 pattern_2, stop: 0.79 transparent); }\n"
                  "QRadioButton::indicator::checked { background-color: qradialgradient(cx: 0.5, cy: 0.5, radius:0.5, fx: 0.5, fy: 0.5, stop: 0.45 pattern_2, stop: 0.55 transparent, "
                                                     "stop: 0.8 transparent, stop: 0.90 pattern_2, stop: 0.98 transparent); }\n"
                  "QRadioButton:focus { outline: none; }";
    return str.replace("pattern_2", color).replace("pattern_1", disable_color);
}

QString general_widget::get_color_button_delete_stylesheet() {
    return "border-style: solid; border-color: #474a51; border-width: 1px; border-radius: 7px; outline: none;";
}

//-------------------------------------------------------------------------
// LOAD VALUE OF SEETING
//-------------------------------------------------------------------------
QVariant general_widget::get_setting(QSettings *settings, QString type) {
    QRegExp tagExp("/");
    QStringList lst = type.split(tagExp);
    QVariant val;
    if(!settings->group().contains(lst.at(0))) {
        settings->beginGroup(lst.at(0));
        val.setValue(settings->value(lst.at(1)));
        settings->endGroup();
    } else {
        val.setValue(settings->value(lst.at(1)));
    }
    return val;
}

//-------------------------------------------------------------------------
// SAVE SEETING WITH VALUE
//-------------------------------------------------------------------------
void general_widget::save_setting(QSettings *settings, QString type, QVariant val) {
    QRegExp tagExp("/");
    QStringList lst = type.split(tagExp);
    if(!settings->group().contains(lst.at(0))) {
        settings->beginGroup(lst.at(0));
        settings->setValue(lst.at(1), val);
        settings->endGroup();
    } else {
        settings->setValue(lst.at(1), val);
    }
    settings->sync();
}

//-------------------------------------------------------------------------
// CHECK EXISTANCE OF CERTAIN SETTING IN SETTIGNS-FILE
//-------------------------------------------------------------------------
bool general_widget::check_setting_exist(QSettings *settings, QString type, QVariant val, bool create) {
    if(!settings->contains(type)) {
        if(create) {
            save_setting(settings, type, val);
        }
        return create;
    } else {
        return true;
    }
}

//-------------------------------------------------------------------------
// GET STYLE_SHEET TO CUSTOMIZE SOME WINDOWS
//-------------------------------------------------------------------------
QString general_widget::get_style_sheet(QString pattern_1, QString pattern_2) {
    QString str = style_sheet;
    return str.replace("pattern_1", pattern_1).replace("pattern_2", pattern_2);
}

//-------------------------------------------------------------------------
// DISPLAYING WARNING/QUESTION/INFO MESSAGES
//-------------------------------------------------------------------------
int general_widget::show_message_box(QString title, QString message, QMessageBox::Icon type, QWidget *parent) {
    QMessageBox msgBox(type, title, QString("\n").append(message));
    msgBox.setStyleSheet(get_style_sheet("QMessageBox", "QMessageBox QLabel { color: #000000; }"));
    if(!title.count()) {
        msgBox.setWindowTitle(tr("Warning"));
        if(type == QMessageBox::Question) {
            msgBox.setWindowTitle(tr("Question"));
        } else if(type == QMessageBox::Information) {
            msgBox.setWindowTitle(tr("Information"));
        }
    }
    msgBox.setParent(parent);
    if(type == QMessageBox::Question) {
        msgBox.addButton(QMessageBox::Yes);
        msgBox.addButton(QMessageBox::No);
        msgBox.setButtonText(QMessageBox::Yes, tr("Yes"));
        msgBox.setButtonText(QMessageBox::No, tr("No"));
    }
    msgBox.setWindowModality(Qt::ApplicationModal);
    msgBox.setModal(true);
    msgBox.setWindowFlags(msgBox.windowFlags() | Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint);
    QGridLayout *layout = dynamic_cast<QGridLayout *>(msgBox.layout());
    layout->setColumnMinimumWidth(1, 20);
    msgBox.setLayout(layout);
    return msgBox.exec();
}

Enter_Leave_Watcher::Enter_Leave_Watcher(QObject *parent) : QObject(parent) {}

bool Enter_Leave_Watcher::eventFilter(QObject *object, QEvent *event) {
    if((event->type() == QEvent::Enter) || (event->type() == QEvent::Leave)) {
        emit signal_object_enter_leave_event(object, event->type());
        return true;
    }
    return false;
}
