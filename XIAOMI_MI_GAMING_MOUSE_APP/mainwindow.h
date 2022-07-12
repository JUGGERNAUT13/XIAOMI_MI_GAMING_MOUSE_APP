#ifndef MAINWINDOW_H
    #define MAINWINDOW_H

    #include <QByteArray>
    #include <QMainWindow>
    #include <QApplication>
    #include <QColorDialog>
    #include <libusb-1.0/libusb.h>

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

            int write_to_mouse(QByteArray &data);
            int mouse_set_color_for_device(devices dev, effects effct, speed spd, uint8_t r, uint8_t g, uint8_t b);
            int mouse_non_sleep();

            Ui::MainWindow *ui;
            QList<QString> tail_addtnl_effcts{tr("Tic tac"), tr("Colors changing"), tr("RGB")};

            bool mnl_chng_effcts = false;

        private slots:
            void on_cmbBx_dev_lst_currentIndexChanged(int index);
            void on_cmbBx_effcts_lst_currentIndexChanged(int index);
            void on_pshBttn_chs_clr_clicked();
            void on_pshBttn_apply_to_mouse_clicked();
    };

#endif // MAINWINDOW_H
