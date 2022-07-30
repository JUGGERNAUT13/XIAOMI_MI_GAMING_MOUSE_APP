#ifndef GENERAL_WIDGET_H
    #define GENERAL_WIDGET_H

    #include <QDir>
    #include <QDebug>
    #include <QWidget>
    #include <QSettings>
    #include <QMouseEvent>
    #include <QPushButton>
    #include <QApplication>
    #include <QFontDatabase>
    #include <QProcessEnvironment>

    class general_widget : public QWidget {
        Q_OBJECT
        public:
            general_widget(QWidget *prnt = nullptr);
            ~general_widget() override;

            QString get_app_path();
            QString get_color_button_stylesheet(QString color, QString disable_color);
            QString get_color_button_delete_stylesheet();
            QVariant get_setting(QSettings *settings, QString type);
            void save_setting(QSettings *settings, QString type, QVariant val);
            bool check_setting_exist(QSettings *settings, QString type, QVariant val, bool create);

        private:
            QWidget *parent = nullptr;
            QString app_path = "";
    };

    class Enter_Leave_Watcher : public QObject {
        Q_OBJECT
        public:
            explicit Enter_Leave_Watcher(QObject *parent = nullptr);
            virtual bool eventFilter(QObject *watched, QEvent *event) override;

        signals:
            void signal_object_enter_leave_event(QObject *object, QEvent::Type event_type);
    };

#endif // GENERAL_WIDGET_H
