#include "general_widget.h"

general_widget::general_widget(QWidget *prnt) {
    parent = prnt;
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
            return true;
        }
        return false;
    } else {
        return true;
    }
}

ButtonHoverWatcher::ButtonHoverWatcher(QString _icon_name, QObject *parent) : QObject(parent) {
    icon_name = _icon_name;
}

bool ButtonHoverWatcher::eventFilter(QObject *watched, QEvent *event) {
    QPushButton *button = qobject_cast<QPushButton*>(watched);
    if(!button || button->isChecked()) {
        return false;
    }
    if(event->type() == QEvent::Enter) {
        button->setIcon(QIcon(":/images/icons/icon_" + icon_name + "_checked.png"));
        return true;
    }
    if(event->type() == QEvent::Leave) {
        uncheck_button(button);
        return true;
    }
    return false;
}

void ButtonHoverWatcher::uncheck_button(QPushButton *button) {
    button->setIcon(QIcon(":/images/icons/icon_" + icon_name + "_unchecked.png"));
}
