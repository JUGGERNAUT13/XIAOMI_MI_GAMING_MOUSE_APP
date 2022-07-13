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

int MainWindow::write_to_mouse(QByteArray &data) {
    struct libusb_device_handle * device_handle = NULL;
    struct libusb_config_descriptor *cfg_desc = NULL;
    libusb_context *context = NULL;
    int result;
    result = libusb_init(&context);             // Initialize libusb
    if(result < 0) {
        qDebug() << "Error initializing libusb: " << libusb_error_name(result);
        return -1;
    }
//    libusb_set_option(context, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_WARNING);      // Set debugging output to max level
    libusb_device **list;
    libusb_device *found = NULL;
    ssize_t cnt = libusb_get_device_list(context, &list);
    libusb_device *device;
    struct libusb_device_descriptor desc;
    for(int i = 0; i < cnt; i++) {
        device = list[i];
        result = libusb_get_device_descriptor(device, &desc);
//        qDebug() << "Vendor:Device = " << hex << desc.idVendor << " " << hex << desc.idProduct << dec << i;
        if((result == LIBUSB_SUCCESS) && (desc.idVendor == VENDOR_ID) && ((desc.idProduct == PRODUCT_ID_WIRE)/* || (desc.idProduct == PRODUCT_ID_WIRELESS)*/)) {
            found = device;
            break;
        }
    }
    if(!found) {
        return -2;
    }
    libusb_open(found, &device_handle);
    if(!device_handle) {
        qDebug() << "Error finding USB device";
        libusb_exit(context);
        return -3;
    }
    // Enable auto-detaching of the kernel driver. If a kernel driver currently has an interface claimed, it will be automatically be detached when we claim that interface.
    // When the interface is restored, the kernel driver is allowed to be re-attached. This can alternatively be manually done via libusb_detach_kernel_driver().
//    libusb_set_auto_detach_kernel_driver(device_handle, 1);
    libusb_get_active_config_descriptor(found, &cfg_desc);
    int interface_number = -1;
    for(int i = 0; i < static_cast<int>(cfg_desc->bNumInterfaces); i++) {
        for(int j = 0; j < static_cast<int>(cfg_desc->interface[i].num_altsetting); j++) {
            if(static_cast<int>(cfg_desc->interface[i].altsetting[j].bInterfaceProtocol) == KEYBOARD) {
                interface_number = static_cast<int>(cfg_desc->interface[i].altsetting[j].bInterfaceNumber);
            }
        }
    }
    bool flg_active = static_cast<bool>(libusb_kernel_driver_active(device_handle, interface_number));
    if(flg_active) {
        libusb_detach_kernel_driver(device_handle, interface_number);
//        qDebug() << "Detached";
    }
//    qDebug() << "Interface number: " << interface_number;
    result = libusb_claim_interface(device_handle, interface_number);
    if(result < 0) {
        qDebug() << "Error claiming interface: " << libusb_error_name(result);
        if(device_handle) {
            libusb_close(device_handle);
        }
        libusb_exit(context);
        return -4;
    }
    result = libusb_control_transfer(device_handle, 0x0021, 0x0009, 0x024d, 0x0001, reinterpret_cast<unsigned char *>(data.data()), data.count(), 50);
    if((result == LIBUSB_SUCCESS) || (result == PACKET_SIZE)) {
        result = 0;
//        qDebug() << "Sucess";
    } else {
        result = -5;
        qDebug() << "Failure " << libusb_error_name(result) << ";  code: " << result;
    }
    libusb_release_interface(device_handle, interface_number);          // We are done with our device and will now release the interface we previously claimed as well as the device
//    qDebug() << "Released";
    if(flg_active) {
        libusb_attach_kernel_driver(device_handle, interface_number);
//        qDebug() << "Attached";
    }
    libusb_close(device_handle);
//    qDebug() << "Lib closed";
    libusb_exit(context);                                               // Shutdown libusb
//    qDebug() << "Context exit";
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
    return write_to_mouse(clr_mod_spd_arr);
}

int MainWindow::mouse_non_sleep() {
    QByteArray non_sleep_arr = "\x4d\x90\xde\x30\xfe\xff\xff\xff\xda\x98\x20\x76\xd5\xd1\xae\x68\xa0\xe6\xe9\x03\xbc\xd8\x28\x00\xf3\xe0\x81\x77\xb8\xee\x37\x06";
    return write_to_mouse(non_sleep_arr);
}

#ifdef USE_XIAOMI_MOUSE_NO_SLEEP_TIMER
void MainWindow::slot_timeout() {
    no_sleep_timer->stop();
    mouse_non_sleep();
    no_sleep_timer->setInterval(MINUTE);
    no_sleep_timer->start();
}
#endif
