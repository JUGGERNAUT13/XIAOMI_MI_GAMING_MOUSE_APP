#include "mainwindow.h"
#include "ui_mainwindow.h"

#define VENDOR_ID               0x2717
#define PRODUCT_ID_WIRE         0x5009
//#define PRODUCT_ID_WIRELESS     0x500b        //NOT WORKING IN WIRELESS MODE, BUT COMMANDS EXEC SUCCESSFULLY
#define PACKET_SIZE             32
#define MINUTE                  60000

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    emit ui->cmbBx_dev_lst->currentIndexChanged(ui->cmbBx_dev_lst->currentIndex());
    emit ui->cmbBx_effcts_lst->currentIndexChanged(ui->cmbBx_effcts_lst->currentIndex());
#ifdef USE_XIAOMI_MOUSE_NO_SLEEP_TIMER
    no_sleep_timer = new QTimer();
    connect(no_sleep_timer, &QTimer::timeout, this, &MainWindow::slot_timeout);
    slot_timeout();
#endif
}

MainWindow::~MainWindow() {
#ifdef USE_XIAOMI_MOUSE_NO_SLEEP_TIMER
    no_sleep_timer->stop();
    delete no_sleep_timer;
#endif
    delete ui;
}

void MainWindow::on_cmbBx_dev_lst_currentIndexChanged(int index) {
    if((index == TAIL) && (ui->cmbBx_effcts_lst->count() < COLORS_CHANGING)) {
        ui->cmbBx_effcts_lst->addItems(tail_addtnl_effcts);
    } else if((index == WHEEL) && (ui->cmbBx_effcts_lst->count() > TIC_TAC)) {
        while(ui->cmbBx_effcts_lst->count() > TIC_TAC) {
            ui->cmbBx_effcts_lst->removeItem(ui->cmbBx_effcts_lst->count() - 1);
        }
    }
}

void MainWindow::on_cmbBx_effcts_lst_currentIndexChanged(int index) {
    ui->frm_spd->setEnabled(index > STATIC);
    ui->frm_clr->setEnabled(((index + static_cast<int>(ui->cmbBx_effcts_lst->currentIndex() > TIC_TAC)) < COLORS_CHANGING) && (index > DISABLE));
}

void MainWindow::on_pshBttn_chs_clr_clicked() {
    QColorDialog dlg(this);
    dlg.setOption(QColorDialog::DontUseNativeDialog, true);
    dlg.setCurrentColor(QColor(ui->pshBttn_chs_clr->styleSheet().split(";").first().split(" ").last()));
    if(dlg.exec() == QDialog::Accepted) {
        QColor color = dlg.selectedColor();
        if(color.isValid()) {
            QColor dsbl_color(color.red(), color.green(), color.blue(), 127);
            ui->pshBttn_chs_clr->setStyleSheet("QPushButton{background-color: " + color.name(QColor::HexArgb) + "; " + "border: 0px;}\n"
                                               "QPushButton:disabled{background-color: " + dsbl_color.name(QColor::HexArgb) + "; " + "border: 0px;}");
        }
    }
}

void MainWindow::on_pshBttn_apply_to_mouse_clicked() {
    QColor clr(ui->pshBttn_chs_clr->styleSheet().split(";").first().split(" ").last());
    int effct = ui->cmbBx_effcts_lst->currentIndex() + static_cast<int>(ui->cmbBx_effcts_lst->currentIndex() > TIC_TAC);
    mouse_set_color_for_device(static_cast<devices>(ui->cmbBx_dev_lst->currentIndex()), static_cast<effects>(effct), static_cast<speed>(ui->hrzntlSldr_effct_spd->value()), clr.red(), clr.green(), clr.blue());
}

int MainWindow::write_to_mouse_hid(QByteArray &data) {
    hid_device *handle = hid_open(VENDOR_ID, PRODUCT_ID_WIRE, NULL);
    int result = -1;
    if(handle) {
        result = hid_send_feature_report(handle, reinterpret_cast<unsigned char *>(data.data()), data.count());
    }
    if((result == 0) || (result == PACKET_SIZE)) {
        result = 0;
        qDebug() << "Sucess";
    } else {
        const wchar_t *string = hid_error(handle);
        qDebug() << "Failure: " << QString::fromWCharArray(string, (sizeof (string) / sizeof(const wchar_t *))) << ";  code:" << result;
    }
    return result;
}

int MainWindow::mouse_set_color_for_device(devices dev, effects effct, speed spd, uint8_t r, uint8_t g, uint8_t b) {
    QByteArray clr_mod_spd_arr = "\x4d\xa1";
    clr_mod_spd_arr.append(dev);
    clr_mod_spd_arr.append(effct);
    clr_mod_spd_arr.append(spd * static_cast<int>(effct > 1));
    clr_mod_spd_arr.append("\x08\x08");
    clr_mod_spd_arr.append(r);
    clr_mod_spd_arr.append(g);
    clr_mod_spd_arr.append(b);
    clr_mod_spd_arr.append("\x00", (PACKET_SIZE - clr_mod_spd_arr.count()));
    return write_to_mouse_hid(clr_mod_spd_arr);
}

#ifdef USE_XIAOMI_MOUSE_NO_SLEEP_TIMER
int MainWindow::mouse_non_sleep() {
    QByteArray non_sleep_arr = "\x4d\x90\xde\x30\xfe\xff\xff\xff\xda\x98\x20\x76\xd5\xd1\xae\x68\xa0\xe6\xe9\x03\xbc\xd8\x28";
    non_sleep_arr.append("\x00", 1);
    non_sleep_arr.append("\xf3\xe0\x81\x77\xb8\xee\x37\x06");
    return write_to_mouse_hid(non_sleep_arr);
}

void MainWindow::slot_timeout() {
    no_sleep_timer->stop();
    mouse_non_sleep();
    no_sleep_timer->setInterval(MINUTE);
    no_sleep_timer->start();
}
#endif
