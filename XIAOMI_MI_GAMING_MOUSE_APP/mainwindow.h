#ifndef MAINWINDOW_H
    #define MAINWINDOW_H

    #include <QTime>
    #include <QMenu>
    #include <QTimer>
    #include <QMainWindow>
    #include <QColorDialog>
    #include <QRadioButton>
    #include <QSystemTrayIcon>
    #include <math.h>
    #include "hidapi.h"
    #include "general_widget.h"

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
                UPDATE          = 4,
                PAGES_COUNT     = 5
            } pages;

            void finish_init();
            void create_base_settings();
            void create_color_buttons();
            void remove_color_buttons(int new_clrs_cnt);
            void remove_color_buttons_from_ui();
            void change_state_of_ui(bool flg);
            void write_key_to_list(QKeyEvent *event, QString icon_type);
            int write_to_mouse_hid(QByteArray &data, bool read = false, QByteArray *output = nullptr);
            int mouse_set_color_for_device();
            int mouse_non_sleep();

            Ui::MainWindow *ui;
            general_widget *gen_widg = nullptr;
            QTimer *no_sleep_timer = nullptr;
            QTimer *anim_timer = nullptr;
            QAction *minimizeAction = nullptr;
            QAction *maximizeAction = nullptr;
            QAction *restoreAction = nullptr;
            QAction *quitAction = nullptr;
            QMenu *trayIconMenu = nullptr;
            QSystemTrayIcon *trayIcon = nullptr;
            QSettings *settings = nullptr;
            QVector<QRadioButton *> clrs_bttns_lst;
            QVector<QPushButton *> clrs_dlt_bttns_lst;
            QVector<ButtonHoverWatcher *> bttns_wtchrs_lst;
            QVector<int> crrnt_devs_clr_indxs;
            QList<QTime> pressed_keys_tmr_lst;
            QList<int> pressed_keys_lst;
            QString anim_img_nam;
            QString crrnt_tail_clr;
            QString crrnt_wheel_clr;
            pages crrnt_page = HOME;
            pages prev_page = HOME;
            effects crrnt_tail_effct;
            effects crrnt_wheel_effct;
            speed crrnt_tail_spped;
            speed crrnt_wheel_speed;
            QPoint clck_pos;
            QTime key_hold_timer;

            bool mnl_chng = false;
            bool is_frst_show = true;
            bool is_drag = false;
            int16_t crrnt_img = -1;
            int16_t img_end_val = -1;
            int8_t img_cnt_dir = -1;
            uint8_t init_flg = 0;
            uint8_t mcrs_prssd_cnt = 0;

        private slots:
            void showEvent(QShowEvent *) override;
            void resizeEvent(QResizeEvent *) override;
            void mousePressEvent(QMouseEvent *event) override;
            void mouseMoveEvent(QMouseEvent *event) override;
            void mouseReleaseEvent(QMouseEvent *event) override;
            void keyPressEvent(QKeyEvent *event) override;
            void keyReleaseEvent(QKeyEvent *event) override;
            void on_pshBttn_add_clr_clicked();
            void slot_no_sleep_timeout();
            void slot_anim_timeout();
    };

#endif // MAINWINDOW_H
