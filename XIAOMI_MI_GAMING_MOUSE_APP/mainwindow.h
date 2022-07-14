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
            int mouse_set_color_for_device(devices dev, effects effct, speed spd, uint8_t r, uint8_t g, uint8_t b);
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
            pages prev_page = BUTTONS;

            bool mnl_chng_effcts = false;
            int16_t crrnt_img = -1;
            int16_t img_end_val = -1;
            int8_t img_cnt_dir = -1;


        private slots:
            void on_cmbBx_dev_lst_currentIndexChanged(int index);
            void on_cmbBx_effcts_lst_currentIndexChanged(int index);
            void on_pshBttn_chs_clr_clicked();
            void on_pshBttn_apply_to_mouse_clicked();
#ifdef USE_XIAOMI_MOUSE_NO_SLEEP_TIMER
            void slot_no_sleep_timeout();
#endif
            void slot_anim_timeout();
            void showEvent(QShowEvent *) override;
            void resizeEvent(QResizeEvent *) override;
            void mousePressEvent(QMouseEvent *event) override;
            void mouseMoveEvent(QMouseEvent *event) override;
    };

#endif // MAINWINDOW_H
