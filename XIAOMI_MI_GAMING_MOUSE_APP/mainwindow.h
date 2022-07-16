#ifndef MAINWINDOW_H
    #define MAINWINDOW_H

    #include <QMenu>
    #include <QTimer>
    #include <QDebug>
    #include <QMouseEvent>
    #include <QMainWindow>
    #include <QApplication>
    #include <QColorDialog>
    #include <QSystemTrayIcon>
    #include "hidapi.h"

    #define USE_XIAOMI_MOUSE_NO_SLEEP_TIMER

    namespace Ui {
        class MainWindow;
    }

    class MainWindow : public QMainWindow {
        Q_OBJECT
        public:
            explicit MainWindow(QWidget *parent = nullptr);
            ~MainWindow() override;

        private:
            typedef enum interface_protocol_type {
                KEYBOARD        = 1,
                MOUSE           = 2
            } interface_protocol_type;

            typedef enum devices {
                TAIL            = 0,
                WHEEL           = 1
            } devices;

            typedef enum effects {      //modes 2..6 not allowed for WHEEL
                DISABLE         = 0,
                STATIC          = 1,
                BREATH          = 2,
                TIC_TAC         = 3,
                COLORS_CHANGING = 5,
                RGB             = 6
            } effects;

            typedef enum speed {
                SPEED_WRONG     = 0,    //the fastest blink for WHEEL, but has no effect for TAIL, maybe a bug(not usable value)
                SPEED_1         = 1,
                SPEED_2         = 2,
                SPEED_3         = 3,
                SPEED_4         = 4,
                SPEED_5         = 5,
                SPEED_6         = 6,
                SPEED_7         = 7,
                SPEED_8         = 8
            } speed;

            typedef enum pages {
                HOME            = 0,
                BUTTONS         = 1,
                LIGHTNING       = 2,
                SPEED           = 3,
                UPDATE          = 4
            } pages;

            int write_to_mouse_hid(QByteArray &data);
            int mouse_set_color_for_device();
#ifdef USE_XIAOMI_MOUSE_NO_SLEEP_TIMER
            int mouse_non_sleep();
#endif

            Ui::MainWindow *ui;
#ifdef USE_XIAOMI_MOUSE_NO_SLEEP_TIMER
            QTimer *no_sleep_timer = nullptr;
#endif
            QTimer *anim_timer = nullptr;
            QAction *minimizeAction = nullptr;
            QAction *maximizeAction = nullptr;
            QAction *restoreAction = nullptr;
            QAction *quitAction = nullptr;
            QMenu *trayIconMenu = nullptr;
            QSystemTrayIcon *trayIcon = nullptr;
            QList<QString> tail_addtnl_effcts{tr("Tic tac"), tr("Colors changing"), tr("RGB")};
            QPoint clck_pos;
            QString anim_img_nam;
            pages crrnt_page = HOME;
            pages prev_page = HOME;

            bool mnl_chng_effcts = false;
            bool is_frst_show = true;
            bool is_drag = false;
            int16_t crrnt_img = -1;
            int16_t img_end_val = -1;
            int8_t img_cnt_dir = -1;
            uint8_t crrnt_effct = 0;

        private slots:
            void on_pshBttn_chs_clr_clicked();
#ifdef USE_XIAOMI_MOUSE_NO_SLEEP_TIMER
            void slot_no_sleep_timeout();
#endif
            void slot_anim_timeout();
            void showEvent(QShowEvent *) override;
            void resizeEvent(QResizeEvent *) override;
            void mousePressEvent(QMouseEvent *event) override;
            void mouseMoveEvent(QMouseEvent *event) override;
            void mouseReleaseEvent(QMouseEvent *event) override;
    };

#endif // MAINWINDOW_H
