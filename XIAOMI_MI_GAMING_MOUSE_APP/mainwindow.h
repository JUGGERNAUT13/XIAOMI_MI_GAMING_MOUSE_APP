#ifndef MAINWINDOW_H
    #define MAINWINDOW_H

    #include <QTime>
    #include <QMenu>
    #include <QTimer>
    #include <QPainter>
    #include <QMainWindow>
    #include <QColorDialog>
    #include <QRadioButton>
    #include <QButtonGroup>
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
                KEYBOARD            = 1,
                MOUSE               = 2
            } interface_protocol_type;

            typedef enum devices {
                TAIL                = 0,
                WHEEL               = 1
            } devices;

            typedef enum effects {      //modes 2..6 not allowed for WHEEL
                DISABLE             = 0,
                STATIC              = 1,
                BREATH              = 2,
                TIC_TAC             = 3,
                COLORS_CHANGING     = 5,
                RGB                 = 6
            } effects;

            typedef enum speed {
                SPEED_WRONG         = 0,    //the fastest blink for WHEEL, but has no effect for TAIL, maybe a bug(not usable value)
                SPEED_1             = 1,
                SPEED_2             = 2,
                SPEED_3             = 3,
                SPEED_4             = 4,
                SPEED_5             = 5,
                SPEED_6             = 6,
                SPEED_7             = 7,
                SPEED_8             = 8
            } speed;

            typedef enum pages {
                HOME                = 0,
                BUTTONS             = 1,
                LIGHTNING           = 2,
                SPEED               = 3,
                UPDATE              = 4
            } pages;

            typedef enum key_modifiers {
#ifdef __linux__
                LEFT_SHIFT          = 50,
                RIGHT_SHIFT         = 62,
                LEFT_CTRL           = 37,
                RIGHT_CTRL          = 105,
                LEFT_ALT            = 64,
                RIGHT_ALT           = 108,
                LEFT_WIN            = 133,
                RIGHT_WIN           = 134
#elif WIN32
                LEFT_SHIFT          = 42,
                RIGHT_SHIFT         = 54,
                LEFT_CTRL           = 29,
                RIGHT_CTRL          = 285,
                LEFT_ALT            = 56,
                RIGHT_ALT           = 312,
                LEFT_WIN            = 133,      //??
                RIGHT_WIN           = 348
#endif
            } key_modifiers;

            typedef enum mouse_buttons {
                SCROLL_BUTTON       = 2,
                M5_BUTTON           = 3,
                M4_BUTTON           = 4,
                RISE_DPI_BUTTON     = 5,
                LOWER_DPI_BUTTON    = 6,
                AIMING_BUTTON       = 7
            } mouse_buttons;

            typedef enum mouse_key_combos {
                LEFT_CLICK          = 0x0001,
                RIGHT_CLICK         = 0x0002,
                MIDDLE_CLICK        = 0x0003,
                MOVE_BACK           = 0x0004,
                MOVE_FORWARD        = 0x0005,
                RISE_DPI            = 0x0006,
                LOWER_DPI           = 0x0007,
                TURN_DPI            = 0x0008,
                VOLUME_UP           = 0x010A,
                VOLUME_DOWN         = 0x010B,
                SILENT_MODE         = 0x010C,
                ALT_F4              = 0x0101,
                CTRL_SHIFT_TAB      = 0x0104,
                CTRL_X              = 0x0105,
                WIN_D               = 0x0102,
                CTRL_TAB            = 0x0103,
                CTRL_C              = 0x0109,
                CTRL_Z              = 0x0107,
                CTRL_Y              = 0x0108,
                CTRL_V              = 0x0106,
                CUSTOM_KEY_COMBO    = 0x0100,
                COMBOS_COUNT        = 21
            } mouse_key_combos;

            typedef enum mouse_key_modifiers {
                KEY_LEFT_CTRL       = 0x01,
                KEY_LEFT_SHIFT      = 0x02,
                KEY_LEFT_ALT        = 0x04,
                KEY_LEFT_WIN        = 0x08,
                KEY_RIGHT_CTRL      = 0x10,
                KEY_RIGHT_SHIFT     = 0x20,
                KEY_RIGHT_ALT       = 0x40,
                KEY_RIGHT_WIN       = 0x80
            } mouse_key_modifiers;

            typedef enum mouse_keys {
                KEY_CTRL_BREAK      = 0x03,
                KEY_A               = 0x04,
                KEY_B               = 0x05,
                KEY_C               = 0x06,
                KEY_D               = 0x07,
                KEY_E               = 0x08,
                KEY_F               = 0x09,
                KEY_G               = 0x0A,
                KEY_H               = 0x0B,
                KEY_I               = 0x0C,
                KEY_J               = 0x0D,
                KEY_K               = 0x0E,
                KEY_L               = 0x0F,
                KEY_M               = 0x10,
                KEY_N               = 0x11,
                KEY_O               = 0x12,
                KEY_P               = 0x13,
                KEY_Q               = 0x14,
                KEY_R               = 0x15,
                KEY_S               = 0x16,
                KEY_T               = 0x17,
                KEY_U               = 0x18,
                KEY_V               = 0x19,
                KEY_W               = 0x1A,
                KEY_X               = 0x1B,
                KEY_Y               = 0x1C,
                KEY_Z               = 0x1D,
                KEY_1               = 0x1E,
                KEY_2               = 0x1F,
                KEY_3               = 0x20,
                KEY_4               = 0x21,
                KEY_5               = 0x22,
                KEY_6               = 0x23,
                KEY_7               = 0x24,
                KEY_8               = 0x25,
                KEY_9               = 0x26,
                KEY_0               = 0x27,
                KEY_ENTER           = 0x28,
                KEY_ESCAPE          = 0x29,
                KEY_BACKSPACE       = 0x2A,
                KEY_TAB             = 0x2B,
                KEY_SPACE           = 0x2C,
                KEY_SUB             = 0x2D,
                KEY_EQUAL           = 0x2E,
                KEY_SQR_BRCKT_OPEN  = 0x2F,
                KEY_SQR_BRCKT_CLOSE = 0x30,
                KEY_BACKSLASH       = 0x31,

                KEY_SEMICOLON       = 0x33,
                KEY_APOSTROPHE      = 0x34,
                KEY_BACKTICK        = 0x35,
                KEY_LESS            = 0x36,
                KEY_GREATER         = 0x37,
                KEY_SLASH           = 0x38,
                KEY_CAPS_LOCK       = 0x39,
                KEY_F1              = 0x3A,
                KEY_F2              = 0x3B,
                KEY_F3              = 0x3C,
                KEY_F4              = 0x3D,
                KEY_F5              = 0x3E,
                KEY_F6              = 0x3F,
                KEY_F7              = 0x40,
                KEY_F8              = 0x41,
                KEY_F9              = 0x42,
                KEY_F10             = 0x43,
                KEY_F11             = 0x44,
                KEY_F12             = 0x45,
                KEY_PRINT_SCREEN    = 0x46,
                KEY_SCROLL_LOCK     = 0x47,
                KEY_PAUSE           = 0x48,
                KEY_INSERT          = 0x49,
                KEY_HOME            = 0x4A,
                KEY_PAGE_UP         = 0x4B,
                KEY_DELETE          = 0x4C,
                KEY_END             = 0x4D,
                KEY_PAGE_DOWN       = 0x4E,
                KEY_RIGHT_ARROW     = 0x4F,
                KEY_LEFT_ARROW      = 0x50,
                KEY_DOWN_ARROW      = 0x51,
                KEY_UP_ARROW        = 0x52,
                KEY_NUM_LOCK        = 0x53,
                KEY_NUMPAD_DIV      = 0x54,
                KEY_NUMPAD_MULT     = 0x55,
                KEY_NUMPAD_SUB      = 0x56,
                KEY_NUMPAD_ADD      = 0x57,

                KEY_NUMPAD_1        = 0x59,
                KEY_NUMPAD_2        = 0x5A,
                KEY_NUMPAD_3        = 0x5B,
                KEY_NUMPAD_4        = 0x5C,
                KEY_NUMPAD_5        = 0x5D,
                KEY_NUMPAD_6        = 0x5E,
                KEY_NUMPAD_7        = 0x5F,
                KEY_NUMPAD_8        = 0x60,
                KEY_NUMPAD_9        = 0x61,
                KEY_NUMPAD_0        = 0x62,
                KEY_NUMPAD_DOT      = 0x63,

                KEY_APPS_MENU       = 0x65
            } mouse_keys;

            void finish_init();
            void resize_images();
            void create_base_settings();
            void create_color_buttons();
            void remove_color_buttons(int new_clrs_cnt);
            void remove_color_buttons_from_ui();
            void change_color_frame_size();
            void change_state_of_ui(bool flg);
            template <typename T>
            void clear_vector(QVector<T *> *vctr);
            uint8_t get_current_mouse_button();
            QString get_key_name(QKeyEvent *event, bool *is_modifier_flg = nullptr);
            void form_keys_combination();
            void change_backgound_for_page_widget(QWidget *page = nullptr, bool draw_gear = true);
            QPixmap apply_effects_on_mouse_image();
            void draw_mouse_anim_img(QString path_to_img, bool apply_effects);
            void prepare_data_for_mouse_read_write(QByteArray *arr_out, QByteArray *arr_in, QByteArray header);
            void read_mouse_parameters();
            int write_to_mouse_hid(QByteArray &data, bool read = false, QByteArray *output = nullptr);
            int mouse_set_color_for_device();
            int bind_mouse_button(uint8_t mouse_button, uint8_t mouse_key_combo, uint8_t mouse_key_modifiers = 0, uint8_t mouse_key = 0);
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
            QButtonGroup* key_fnc_bttns_grp = nullptr;
            QVector<QRadioButton *> clrs_bttns_lst;
            QVector<QPushButton *> clrs_dlt_bttns_lst;
            QVector<Enter_Leave_Watcher *> bttns_wtchrs_lst;
            QVector<Enter_Leave_Watcher *> chrctrstc_frms_wtchrs_lst;
            QVector<QTimer *> chrctrstc_frms_tmrs_lst;
            QVector<int8_t> chrctrstc_dirctns_lst;
            QVector<int> crrnt_devs_clr_indxs_lst;
            QVector<QString> chrctrstc_txt_lst;
            QList<QTime> pressed_keys_tmr_lst;
            QList<int> pressed_keys_lst;
            QList<QString> cmb_mdfrs_lst;
            QString anim_img_nam;
            QString crrnt_tail_clr;
            QString crrnt_wheel_clr;
            QString cmb_key = "";
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
            bool mnl_rdng = false;
            bool crrnt_ui_state = true;
            bool prev_ui_state;
            uint16_t clr_scrl_area_max_width = 0;
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
            void on_pshBttn_rst_sttngs_clicked();
            void on_pshBttn_add_clr_clicked();
            void slot_no_sleep_timeout();
            void slot_anim_timeout();
            void on_pshBttn_clr_cstm_key_cmb_clicked();
            void on_pshBttn_sav_cstm_key_cmb_clicked();
    };

    //////////////////////////////////////////////////DIALOG RESET SETTINGS///////////////////////////////////////////////////
    QT_BEGIN_NAMESPACE
    namespace Ui {
        class Dialog_Reset_Settings;
    }
    QT_END_NAMESPACE

    class Dialog_Reset_Settings : public QDialog {
        Q_OBJECT
        public:
            Dialog_Reset_Settings(QWidget *parent = nullptr);
            ~Dialog_Reset_Settings() override;

        private:
            Ui::Dialog_Reset_Settings *ui;
    };

#endif // MAINWINDOW_H
