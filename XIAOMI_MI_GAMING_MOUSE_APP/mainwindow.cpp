#include "mainwindow.h"
#include "ui_mainwindow.h"

#define VENDOR_ID               0x2717
#define PRODUCT_ID_WIRE         0x5009
//#define PRODUCT_ID_WIRELESS     0x500b        //NOT WORKING IN WIRELESS MODE, BUT COMMANDS EXEC SUCCESSFULLY
#define PACKET_SIZE             32
#define NO_SLEEP_INTERVAL_MS    290000
#define ANIM_INTERVAL_MS        30

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    emit ui->cmbBx_dev_lst->currentIndexChanged(ui->cmbBx_dev_lst->currentIndex());
    emit ui->cmbBx_effcts_lst->currentIndexChanged(ui->cmbBx_effcts_lst->currentIndex());
    this->setWindowFlags(this->windowFlags() | Qt::FramelessWindowHint);
    connect(ui->pshBttn_close, &QPushButton::clicked, this, &MainWindow::close);
    connect(ui->pshBttn_mnmz, &QPushButton::clicked, this, &MainWindow::hide);
    minimizeAction = new QAction(tr("Minimize"), this);
    connect(minimizeAction, &QAction::triggered, this, &MainWindow::hide);
    maximizeAction = new QAction(tr("Maximize"), this);
    connect(maximizeAction, &QAction::triggered, this, &MainWindow::showMaximized);
    restoreAction = new QAction(tr("&Restore"), this);
    connect(restoreAction, &QAction::triggered, this, &MainWindow::showNormal);
    quitAction = new QAction(tr("&Quit"), this);
    connect(quitAction, &QAction::triggered, qApp, &QCoreApplication::quit);
    trayIconMenu = new QMenu(this);
    trayIconMenu->addAction(minimizeAction);
    trayIconMenu->addAction(maximizeAction);
    trayIconMenu->addAction(restoreAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(quitAction);
    trayIcon = new QSystemTrayIcon(this);
    trayIcon->setContextMenu(trayIconMenu);
    connect(trayIcon, &QSystemTrayIcon::activated, this, [=]() {
        if(QApplication::mouseButtons().testFlag(Qt::LeftButton)) {
            this->setVisible(!this->isVisible());
        }
    });
    QIcon ico(":/images/icon.ico");
    trayIcon->setIcon(ico);
    qApp->setWindowIcon(ico);
    trayIcon->show();
    std::function<void(int16_t crnt_val, int16_t end_val, int16_t cnt_dir)> anim_1 = [=](int16_t crnt_val, int16_t end_val, int16_t cnt_dir) {
        if(ui->pshBttn_bttns_top->isChecked()) {
            anim_img_nam = "positionToStrabismus_0";
        } else {
            anim_img_nam = "siderToStrabismus_0";
        }
        crrnt_img = crnt_val;
        img_end_val = end_val;
        img_cnt_dir = cnt_dir;
        slot_anim_timeout();
    };
    std::function<void(pages crrnt_page)> anim_2 = [=](pages crrnt_page) {
        if(ui->pshBttn_bttns_top->isChecked()) {
            anim_img_nam = "trailToPosition_0";
        } else {
            anim_img_nam = "siderToTrail_0";
        }
        if(((crrnt_page == LIGHTNING) && ui->pshBttn_bttns_top->isChecked()) || ((crrnt_page == BUTTONS) && !ui->pshBttn_bttns_top->isChecked()))  {
            crrnt_img = 15;
            img_end_val = -1;
            img_cnt_dir = -1;
        } else {
            crrnt_img = 0;
            img_end_val = 16;
            img_cnt_dir = 1;
        }
        slot_anim_timeout();
    };
    std::function<void(QString img_nam, int16_t crnt_val, int16_t end_val, int16_t cnt_dir)> anim_3 = [=](QString img_nam, int16_t crnt_val, int16_t end_val, int16_t cnt_dir) {
        anim_img_nam = img_nam;
        crrnt_img = crnt_val;
        img_end_val = end_val;
        img_cnt_dir = cnt_dir;
        slot_anim_timeout();
    };
    QVector<QPushButton *>bttns_lst {ui->pshBttn_1_home, ui->pshBttn_2_buttons, ui->pshBttn_3_lightning, ui->pshBttn_4_speed, ui->pshBttn_5_update};
    for(int i = 0; i < bttns_lst.count(); i++) {
        connect(bttns_lst[i], &QPushButton::clicked, this, [=]() {
            ui->stckdWdgt_main_pages->setCurrentIndex(i);
            prev_page = crrnt_page;
            crrnt_page = static_cast<pages>(i);
            if((prev_page == HOME) || (prev_page == SPEED) || (prev_page == UPDATE)) {
                if(crrnt_page == BUTTONS) {
                    anim_1(15, -1, -1);
                } else if((crrnt_page == LIGHTNING) && ui->pshBttn_lghtnng_tail->isChecked()) {
                    anim_3("trailToStrabismus_0", 15, -1, -1);
                }
            } else if(prev_page == BUTTONS) {
                if((crrnt_page == HOME) || (crrnt_page == SPEED) || (crrnt_page == UPDATE)) {
                    anim_1(0, 16, 1);
                } else if(crrnt_page == LIGHTNING) {
                    if(ui->pshBttn_lghtnng_head->isChecked()) {
                        anim_1(0, 16, 1);
                    } else {
                        anim_2(crrnt_page);
                    }
                }
            } else if(prev_page == LIGHTNING) {
                if((crrnt_page == HOME) || (crrnt_page == SPEED) || (crrnt_page == UPDATE)) {
                    if(ui->pshBttn_lghtnng_tail->isChecked()) {
                        anim_3("trailToStrabismus_0", 0, 16, 1);
                    }
                } else if(crrnt_page == BUTTONS) {
                    if(ui->pshBttn_lghtnng_head->isChecked()) {
                        anim_1(15, -1, -1);
                    } else {
                        anim_2(crrnt_page);
                    }
                }
            }
        });
    }
    connect(ui->pshBttn_bttns_top, &QPushButton::clicked, this, [=]() {
        anim_3("siderToPosition_0", 0, 16, 1);
    });
    connect(ui->pshBttn_bttns_side, &QPushButton::clicked, this, [=]() {
        anim_3("siderToPosition_0", 15, -1, -1);
    });
    connect(ui->pshBttn_lghtnng_head, &QPushButton::clicked, this, [=]() {
        anim_3("trailToStrabismus_0", 0, 16, 1);
    });
    connect(ui->pshBttn_lghtnng_tail, &QPushButton::clicked, this, [=]() {
        anim_3("trailToStrabismus_0", 15, -1, -1);
    });
    anim_timer = new QTimer();
    connect(anim_timer, &QTimer::timeout, this, &MainWindow::slot_anim_timeout);
#ifdef USE_XIAOMI_MOUSE_NO_SLEEP_TIMER
    no_sleep_timer = new QTimer();
    connect(no_sleep_timer, &QTimer::timeout, this, &MainWindow::slot_no_sleep_timeout);
    slot_no_sleep_timeout();
#endif
}

MainWindow::~MainWindow() {
#ifdef USE_XIAOMI_MOUSE_NO_SLEEP_TIMER
    no_sleep_timer->stop();
    delete no_sleep_timer;
#endif
    anim_timer->stop();
    delete anim_timer;
    delete minimizeAction;
    delete maximizeAction;
    delete restoreAction;
    delete quitAction;
    delete trayIconMenu;
    delete trayIcon;
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
    struct hid_device_info *devs = hid_enumerate(0x0, 0x0);
    struct hid_device_info *cur_dev = nullptr;
    int cnt = 0;
    uint crrnt_prdct_id = 0;
    QList<QString> path_lst;
    QList<uint> prod_id_lst;
    while(devs) {
        if((devs->vendor_id == VENDOR_ID) && ((devs->product_id == PRODUCT_ID_WIRE)/* || (devs->product_id == PRODUCT_ID_WIRELESS)*/) && (devs->interface_number == KEYBOARD) && (crrnt_prdct_id != devs->product_id)) {
            cur_dev = devs;
//            qDebug() << "Device Found\n  type: " << hex << cur_dev->vendor_id << " " << cur_dev->product_id << "\n  path: " << cur_dev->path << "\n  serial_number: " << cur_dev->serial_number;
            crrnt_prdct_id = devs->product_id;
            path_lst.append(devs->path);
            prod_id_lst.append(crrnt_prdct_id);
            cnt++;
        }
        devs = devs->next;
    }
    hid_free_enumeration(devs);
    if(!cur_dev) {
        ui->centralwidget->setEnabled(false);
        return -1;
    }
    hid_free_enumeration(cur_dev);
    if(cnt == 2) {
        crrnt_prdct_id = PRODUCT_ID_WIRE;
    }
    QString path;
    for(int i = 0; i < path_lst.count(); i++) {
        if(prod_id_lst[i] == crrnt_prdct_id) {
            path = path_lst[i];
        }
    }
//    hid_device *handle = hid_open(VENDOR_ID, crrnt_prdct_id, NULL);
//    if(!handle) {
//        handle = hid_open(VENDOR_ID, PRODUCT_ID_WIRELESS, NULL);
//    }
    hid_device *handle = hid_open_path(path.toLatin1().data());
    ui->centralwidget->setEnabled(crrnt_prdct_id == PRODUCT_ID_WIRE);
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
    hid_close(handle);
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

void MainWindow::slot_no_sleep_timeout() {
    no_sleep_timer->stop();
    mouse_non_sleep();
    no_sleep_timer->setInterval(NO_SLEEP_INTERVAL_MS);
    no_sleep_timer->start();
}
#endif

void MainWindow::slot_anim_timeout() {
    anim_timer->stop();
    QString tmp = QString::number(crrnt_img);
    if(crrnt_img < 10) {
        tmp.prepend("0");
    }
    QPixmap mouse_img(":/images/anim/" + anim_img_nam + tmp + ".png");
    ui->lbl_mouse_img_anim->setPixmap(QPixmap(mouse_img.scaled(ui->lbl_mouse_img_anim->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation)));
    crrnt_img += img_cnt_dir;
    if(crrnt_img != img_end_val) {
        anim_timer->setInterval(ANIM_INTERVAL_MS);
        anim_timer->start();
    }
}

void MainWindow::resizeEvent(QResizeEvent *) {
    QPixmap bkgnd(":/images/background.png");
    QPalette main_palette;
    main_palette.setBrush(QPalette::Background, bkgnd.scaled(this->size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    this->setPalette(main_palette);
    ui->frm_slider->setStyleSheet("background-image: url(:/images/background_slider.png); border-right-width: 1px; border-right-style: solid; border-right-color: #1c2228;");
    QPixmap mouse_img(":/images/anim/positionToStrabismus_015.png");
    ui->lbl_mouse_img_anim->setPixmap(QPixmap(mouse_img.scaled(ui->lbl_mouse_img_anim->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation)));
}

void MainWindow::mousePressEvent(QMouseEvent *event) {
    clck_pos = event->pos();
}

void MainWindow::mouseMoveEvent(QMouseEvent *event) {
    move((event->globalX() - clck_pos.x()), (event->globalY() - clck_pos.y()));
}
