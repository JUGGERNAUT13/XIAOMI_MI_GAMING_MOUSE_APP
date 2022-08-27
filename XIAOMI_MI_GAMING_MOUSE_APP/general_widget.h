#ifndef GENERAL_WIDGET_H
    #define GENERAL_WIDGET_H

    #include <QDir>
    #include <QDebug>
    #include <QWidget>
    #include <QSettings>
    #include <QMessageBox>
    #include <QMouseEvent>
    #include <QPushButton>
    #include <QGridLayout>
    #include <QApplication>
    #include <QFontDatabase>
    #include <QProcessEnvironment>

    class general_widget : public QObject {
        Q_OBJECT
        public:
            general_widget();
            ~general_widget() override;

            QString get_app_path();
            QString get_color_button_stylesheet(QString color, QString disable_color);
            QString get_color_button_delete_stylesheet();
            QVariant get_setting(QSettings *settings, QString type);
            void save_setting(QSettings *settings, QString type, QVariant val);
            bool check_setting_exist(QSettings *settings, QString type, QVariant val, bool create);
            QString get_style_sheet(QString pattern_1, QString pattern_2);
            int show_message_box(QString title, QString message, QMessageBox::Icon type, QWidget *parent);

        private:
            QString style_sheet = "";
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
