#include "mainwindow.h"
#include "ui_mainwindow.h"

#define ORIGINAL_MAX_COLORS     8
#define PART_SIZE               12
#define LBL_ANIM_INTERVAL_MS    17
#define COLOR_BUTTON_SIZE       20
#define IMG_ANIM_INTERVAL_MS    30
#define PACKET_SIZE             32
#define INPUT_PACKET_SIZE       64
#define INIT_INTERVAL_MS        1000
#define NO_SLEEP_INTERVAL_MS    290000
#define VENDOR_ID               0x2717
#define PRODUCT_ID_WIRE         0x5009
//#define PRODUCT_ID_WIRELESS     0x500b        //NOT WORKING IN WIRELESS MODE, BUT COMMANDS EXEC SUCCESSFULLY

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    this->setWindowFlags(this->windowFlags() | Qt::FramelessWindowHint);
    gen_widg = new general_widget(this);
    QDir::setCurrent(gen_widg->get_app_path());
    clr_scrl_area_max_width = (ORIGINAL_MAX_COLORS * COLOR_BUTTON_SIZE) + ((ORIGINAL_MAX_COLORS - 1) * ui->frm_clr_bttns->layout()->spacing());
//    QFontDatabase::addApplicationFont(":/data/tahoma.ttf");
    settings = new QSettings("./user_color.ini", QSettings::IniFormat);
    create_base_settings();
    create_color_buttons();
    ui->frm_dlt_clr_bttns->setVisible(false);
    connect(ui->pshBttn_edt_clrs, &QPushButton::toggled, this, [=](bool tggld) {
        QList<QString> bttn_txt{tr("Edit color"), tr("Done")};
        change_color_frame_size();
        ui->pshBttn_edt_clrs->setText(bttn_txt.at(static_cast<int>(tggld)));
        ui->frm_dlt_clr_bttns->setVisible(tggld);
        ui->pshBttn_add_clr->setVisible(!tggld);
        for(int i = 0; i < clrs_bttns_lst.count(); i++) {
            clrs_bttns_lst[i]->setCheckable(!tggld);
        }
        if(!tggld && (crrnt_devs_clr_indxs_lst[ui->pshBttn_lghtnng_head->isChecked()] != -1)) {
            mnl_chng = true;
            clrs_bttns_lst[crrnt_devs_clr_indxs_lst[ui->pshBttn_lghtnng_head->isChecked()]]->setChecked(true);
            mnl_chng = false;
        }
        ui->frm_clr_bttns->update();
    });
    crrnt_devs_clr_indxs_lst = {-1, -1};
    connect(ui->pshBttn_close, &QPushButton::released, this, &MainWindow::/*hide*/close);
    connect(ui->pshBttn_mnmz, &QPushButton::released, this, &MainWindow::/*showMinimized*/hide);
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
    chrctrstc_txt_lst = {tr("Welcome to Xiaomi Gaming Mouse!"),
                         tr("The 6 buttons can be set independently according to the player's needs using\n the software, and Mi Mouse can be made more convenient by setting the combination key."),
                         tr("Mi Mouse uses a foot pad made of teflon material, which is more durable and can greatly\n increase the life of the mouse."),
                         tr("Mi Mouse has two connection modes: wired and 2.4G wireless.\n The transmission speed using the wired connection is the fastest. Swift speed is a key to a prolific gaming\n"
                            "experience. When switching to wireless mode, Mi Mouse can be useful for fun and office work."),
                         tr("Mi Mouse is designed to fit really well in the hand, providing the player with a seamless gaming experience. Coupled with\n two rubber side skirts, it helps players get a "
                            "great feel of the mouse."),
                         tr("Mi Mouse is equipped with a 5-speed adjustable 7200DPI optical sensor to\n ensure efficient data tracking, providing high levels of accuracy, tracking speed, and "
                            "operational consistency.\n The player is able to fully attune himself to the in-game action without missing any key instances."),
                         tr("Mi Mouse is designed with an aim button. When the player presses the aim button during the game,\n the DPI will be adjusted to a pre-set low value to help the player make "
                            "a more precise aim while playing."),
                         tr("The special light setting function of the Mi Mouse allows the user to set\n colorful RGB lights of the headlights and taillights, when also exhibiting various lightning effects."),
                         tr("Mi Mouse is equipped with a 32-bit ARM processor, making the device more responsive and more efficient. The\n software can also set the refresh rate of four gears 125, "
                            "250, 500, 1000, which can fully adapt to individual needs of players.")};
    QVector<QPushButton *> effects_lst{ui->pshBttn_lghtnng_disable, ui->pshBttn_lghtnng_static, ui->pshBttn_lghtnng_breath, ui->pshBttn_lghtnng_tic_tac, ui->pshBttn_lghtnng_switching, ui->pshBttn_lghtnng_rgb};
    for(int i = 0; i < effects_lst.count(); i++) {
        connect(effects_lst[i], &QPushButton::toggled, this, [=]() {
            ui->frm_spd->setEnabled(i > STATIC);
            ui->frm_clr->setEnabled(((i + static_cast<int>(i > TIC_TAC)) < COLORS_CHANGING) && (i > DISABLE));
            QVector<effects *> devs_effcts_lst{&crrnt_tail_effct, &crrnt_wheel_effct};
            *(devs_effcts_lst[ui->pshBttn_lghtnng_head->isChecked()]) = static_cast<effects>(i);
            mouse_set_color_for_device();
        });
    }
    ui->lbl_logo->setPixmap(QPixmap(":/images/logo.png"));
    QIcon ico(":/images/icon.ico");
    trayIcon->setIcon(ico);
    qApp->setWindowIcon(ico);
    std::function<void(QString img_nam, int16_t crnt_val, int16_t end_val, int8_t cnt_dir)> anim_1 = [=](QString img_nam, int16_t crnt_val, int16_t end_val, int8_t cnt_dir) {
        anim_img_nam = img_nam;
        crrnt_img = crnt_val;
        img_end_val = end_val;
        img_cnt_dir = cnt_dir;
        if(!is_frst_show) {
            slot_anim_timeout();
        }
    };
    std::function<void(int16_t crnt_val, int16_t end_val, int8_t cnt_dir)> anim_2 = [=](int16_t crnt_val, int16_t end_val, int8_t cnt_dir) {
        if(ui->pshBttn_bttns_top->isChecked()) {
            anim_img_nam = "positionToStrabismus_0";
        } else {
            anim_img_nam = "siderToStrabismus_0";
        }
        anim_1(anim_img_nam, crnt_val, end_val, cnt_dir);
    };
    std::function<void(pages crrnt_page)> anim_3 = [=](pages crrnt_page) {
        if(ui->pshBttn_bttns_top->isChecked()) {
            anim_img_nam = "trailToPosition_0";
        } else {
            anim_img_nam = "siderToTrail_0";
        }
        if(((crrnt_page == LIGHTNING) && ui->pshBttn_bttns_top->isChecked()) || ((crrnt_page == BUTTONS) && !ui->pshBttn_bttns_top->isChecked()))  {
            anim_1(anim_img_nam, 15, -1, -1);
        } else {
            anim_1(anim_img_nam, 0, 16, 1);
        }
    };
    QList<QWidget *> pnl_chldrns_lst = ui->frm_slider->findChildren<QWidget *>();
    for(int i = 0; i < pnl_chldrns_lst.count(); i++) {
        pnl_chldrns_lst[i]->setAttribute(Qt::WA_TranslucentBackground, true);
    }
    ui->frm_bttns_top_side_swtch->setVisible(false);
    QVector<QFrame *> chrctrstc_frms_outr_lst{ui->frm_1_pgrmmbl_outr, ui->frm_2_drbl_outr, ui->frm_3_dual_mod_outr, ui->frm_4_ergnmc_outr, ui->frm_5_optcl_sns_outr, ui->frm_6_aimng_outr,
                                              ui->frm_7_lghtnng_outr, ui->frm_8_rfrsh_rate_outr};
    for(int i = 0; i < chrctrstc_frms_outr_lst.count(); i++) {
        chrctrstc_dirctns_lst.push_back(0);
        chrctrstc_frms_wtchrs_lst.push_back(new Enter_Leave_Watcher(this));
        chrctrstc_frms_outr_lst[i]->installEventFilter(chrctrstc_frms_wtchrs_lst[i]);
        chrctrstc_frms_tmrs_lst.push_back(new QTimer());
        connect(chrctrstc_frms_tmrs_lst[i], &QTimer::timeout, this, [=]() {
            chrctrstc_frms_tmrs_lst[i]->stop();
            int top = chrctrstc_frms_outr_lst[i]->layout()->contentsMargins().top() - chrctrstc_dirctns_lst[i];
            int bottom = chrctrstc_frms_outr_lst[i]->layout()->contentsMargins().bottom() + chrctrstc_dirctns_lst[i];
            if(((bottom != -1) && (chrctrstc_dirctns_lst[i] == -1)) || ((top != -1) && (chrctrstc_dirctns_lst[i] == 1))) {
                chrctrstc_frms_outr_lst[i]->layout()->setContentsMargins(chrctrstc_frms_outr_lst[i]->layout()->contentsMargins().left(), top, chrctrstc_frms_outr_lst[i]->layout()->contentsMargins().right(), bottom);
                chrctrstc_frms_tmrs_lst[i]->setInterval(LBL_ANIM_INTERVAL_MS);
                chrctrstc_frms_tmrs_lst[i]->start();
            } else {
                chrctrstc_dirctns_lst[i] = 0;
            }
        });
        connect(chrctrstc_frms_wtchrs_lst[i], &Enter_Leave_Watcher::signal_object_enter_leave_event, this, [=](QObject *obj, QEvent::Type evnt_typ) {
            QFrame *frame = qobject_cast<QFrame*>(obj);
            if(frame) {
                int8_t leave_flg = static_cast<int8_t>(QEvent::Leave - evnt_typ);
                QVector<QLabel *> icon_lbls_lst{ui->lbl_prgmmbl_img, ui->lbl_drbl_img, ui->lbl_dual_mod_img, ui->lbl_ergnmc_img, ui->lbl_optcl_sns_img, ui->lbl_aimng_img,
                                                ui->lbl_lghtnng_img, ui->lbl_rfrsh_rate_img};
                QVector<QVector<QString>> icon_clsrs{{"#242830", "#232c33"}, {"#00e39b", "#00e39b"}};
                QVector<QString> icon_nams_lst{"programmable", "durable", "dual_mode", "ergonomic", "optical_sens", "aiming_plus", "lightning", "refresh_rate"};
                QString styleseet = "QLabel { background-image: url(:/images/icons/home/icon_" + icon_nams_lst[i] + ".png); background-position: center center; "
                                    "background-repeat: no-repeat; background-color: " + icon_clsrs[leave_flg][0] + "; border-color: " + icon_clsrs[leave_flg][1] + "; "
                                    "border-style: solid; border-width: 1px; border-radius: 30px; }\nQLabel:hover { background-color: #00e39b; border-color: #00e39b; }";
                icon_lbls_lst[i]->setStyleSheet(styleseet);
                ui->lbl_chrctrstc_txt->setText(chrctrstc_txt_lst[leave_flg + (leave_flg * i)]);
                chrctrstc_dirctns_lst[i] = leave_flg - static_cast<int8_t>(evnt_typ == QEvent::Leave);
                ui->lbl_chrctrstc_txt->setFont(QFont(ui->lbl_chrctrstc_txt->font().family(), (ui->lbl_chrctrstc_txt->font().pointSize() - (5 * chrctrstc_dirctns_lst[i]))));
                ui->frm_chrctrstc_spcr->setFixedHeight(ui->frm_chrctrstc_spcr->height() - (5 * chrctrstc_dirctns_lst[i]));
                ui->lbl_chrctrstc_txt->setFixedHeight(ui->lbl_chrctrstc_txt->height() + (5 * chrctrstc_dirctns_lst[i]));
                chrctrstc_frms_tmrs_lst[i]->setInterval(LBL_ANIM_INTERVAL_MS);
                chrctrstc_frms_tmrs_lst[i]->start();
            }
        });
    }
    QVector<QString> icons_names{"page_home", "page_buttons", "page_lightning", "page_speed", "page_update", "minimize", "close"};
    QVector<QPushButton *>bttns_lst{ui->pshBttn_1_home, ui->pshBttn_2_buttons, ui->pshBttn_3_lightning, ui->pshBttn_4_speed, ui->pshBttn_5_update, ui->pshBttn_mnmz, ui->pshBttn_close};
    for(int i = 0; i < bttns_lst.count(); i++) {
        bttns_wtchrs_lst.push_back(new Enter_Leave_Watcher(this));
        bttns_lst[i]->installEventFilter(bttns_wtchrs_lst[i]);
        connect(bttns_wtchrs_lst[i], &Enter_Leave_Watcher::signal_object_enter_leave_event, this, [=](QObject *obj, QEvent::Type evnt_typ) {
            QPushButton *button = qobject_cast<QPushButton*>(obj);
            if(button && !button->isChecked()) {
                if(evnt_typ == QEvent::Enter) {
                    button->setIcon(QIcon(":/images/icons/icon_" + icons_names[i] + "_checked.png"));
                } else if(evnt_typ == QEvent::Leave) {
                    button->setIcon(QIcon(":/images/icons/icon_" + icons_names[i] + "_unchecked.png"));
                }
            }
        });
        if(i < PAGES_COUNT) {
            bttns_lst[i]->setText("    " + bttns_lst[i]->text());
            connect(bttns_lst[i], &QPushButton::toggled, this, [=]() {
                ui->pshBttn_strt_stp_rcrd_mcrs->setChecked(false);
                ui->lbl_cstm_key_cmb->clearFocus();
                ui->stckdWdgt_main_pages->setCurrentIndex(i);
                ui->frm_bttns_top_side_swtch->setVisible(i == BUTTONS);
                prev_page = crrnt_page;
                crrnt_page = static_cast<pages>(i);
                bttns_lst[prev_page]->setIcon(QIcon(":/images/icons/icon_" + icons_names[prev_page] + "_unchecked.png"));
                if((prev_page == HOME) || (prev_page == SPEED) || (prev_page == UPDATE)) {
                    if(crrnt_page == BUTTONS) {
                        anim_2(15, -1, -1);
                    } else if((crrnt_page == LIGHTNING) && ui->pshBttn_lghtnng_tail->isChecked()) {
                        anim_1("trailToStrabismus_0", 15, -1, -1);
                    }
                } else if(prev_page == BUTTONS) {
                    if((crrnt_page == HOME) || (crrnt_page == SPEED) || (crrnt_page == UPDATE)) {
                        anim_2(0, 16, 1);
                    } else if(crrnt_page == LIGHTNING) {
                        if(ui->pshBttn_lghtnng_head->isChecked()) {
                            anim_2(0, 16, 1);
                        } else {
                            anim_3(crrnt_page);
                        }
                    }
                } else if(prev_page == LIGHTNING) {
                    if((crrnt_page == HOME) || (crrnt_page == SPEED) || (crrnt_page == UPDATE)) {
                        if(ui->pshBttn_lghtnng_tail->isChecked()) {
                            anim_1("trailToStrabismus_0", 0, 16, 1);
                        }
                    } else if(crrnt_page == BUTTONS) {
                        if(ui->pshBttn_lghtnng_head->isChecked()) {
                            anim_2(15, -1, -1);
                        } else {
                            anim_3(crrnt_page);
                        }
                    }
                }
            });
        }
    }
    std::function<void(int16_t crnt_val, int16_t end_val, int8_t cnt_dir)> change_currnt_dev = [=](int16_t crnt_val, int16_t end_val, int8_t cnt_dir) {
        mnl_chng = true;
        QVector<effects> devs_effcts_lst{crrnt_tail_effct, crrnt_wheel_effct};
        QVector<speed> devs_speed_lst{crrnt_tail_spped, crrnt_wheel_speed};
        QVector<QString> devs_clrs_lst{crrnt_tail_clr, crrnt_wheel_clr};
        QVector<QPushButton *> effects_lst{ui->pshBttn_lghtnng_disable, ui->pshBttn_lghtnng_static, ui->pshBttn_lghtnng_breath, ui->pshBttn_lghtnng_tic_tac, ui->pshBttn_lghtnng_switching, ui->pshBttn_lghtnng_rgb};
        ui->hrzntlSldr_effct_spd->setValue(devs_speed_lst[ui->pshBttn_lghtnng_head->isChecked()]);
        ui->pshBttn_lghtnng_tic_tac->setVisible(ui->pshBttn_lghtnng_tail->isChecked());
        ui->pshBttn_lghtnng_switching->setVisible(ui->pshBttn_lghtnng_tail->isChecked());
        ui->pshBttn_lghtnng_rgb->setVisible(ui->pshBttn_lghtnng_tail->isChecked());
        QString avlbl_clr;
        for(int i = 0; i < clrs_bttns_lst.count(); i++) {
            avlbl_clr = "#" + clrs_bttns_lst[i]->styleSheet().split("#").last().split(",").first();
            if(devs_clrs_lst[ui->pshBttn_lghtnng_head->isChecked()].compare(avlbl_clr, Qt::CaseInsensitive) == 0) {
                clrs_bttns_lst[i]->setChecked(true);
                break;
            } else if((crrnt_devs_clr_indxs_lst[ui->pshBttn_lghtnng_head->isChecked()] == -1) && clrs_bttns_lst[i]->isChecked()) {
                clrs_bttns_lst[i]->setAutoExclusive(false);
                clrs_bttns_lst[i]->setChecked(false);
                clrs_bttns_lst[i]->setAutoExclusive(true);
                break;
            }
        }
        effects_lst[devs_effcts_lst[ui->pshBttn_lghtnng_head->isChecked()]]->setChecked(true);
        emit effects_lst[devs_effcts_lst[ui->pshBttn_lghtnng_head->isChecked()]]->toggled(true);
        anim_1("trailToStrabismus_0", crnt_val, end_val, cnt_dir);
        mnl_chng = false;
    };
    connect(ui->pshBttn_bttns_top, &QPushButton::toggled, this, [=]() {
        ui->pshBttn_bttns_key_cmbntns->click();
        anim_1("siderToPosition_0", 0, 16, 1);
    });
    connect(ui->pshBttn_bttns_side, &QPushButton::toggled, this, [=]() {
        ui->pshBttn_bttns_key_fnctns->click();
        anim_1("siderToPosition_0", 15, -1, -1);
    });
    connect(ui->pshBttn_lghtnng_head, &QPushButton::toggled, this, [=]() {
        change_currnt_dev(0, 16, 1);
    });
    connect(ui->pshBttn_lghtnng_tail, &QPushButton::toggled, this, [=]() {
        change_currnt_dev(15, -1, -1);
    });
    connect(ui->hrzntlSldr_effct_spd, &QSlider::valueChanged, this, &MainWindow::mouse_set_color_for_device);
    QVector<QPushButton *> speed_bttns_lst{ui->pshBttn_speed_rfrsh_rate, ui->pshBttn_speed_dpi};
    for(int i = 0; i < speed_bttns_lst.count(); i++) {
        connect(speed_bttns_lst[i], &QPushButton::toggled, this, [=]() {
            ui->stckdWdgt_rfrsh_rate_n_dpi->setCurrentIndex(i);
            QByteArray tmp_in, tmp_out;
            QVector<QSlider *> sldrs_lst{ui->hrzntlSldr_rfrsh_rate_lvl_1, ui->hrzntlSldr_rfrsh_rate_lvl_2, ui->hrzntlSldr_rfrsh_rate_lvl_3, ui->hrzntlSldr_rfrsh_rate_lvl_4, ui->hrzntlSldr_rfrsh_rate_lvl_5};
            prepare_data_for_mouse_read_write(&tmp_out, &tmp_in, "\x4d\xc2");
            mnl_chng = true;
            emit sldrs_lst[tmp_in.mid(8, 1).toHex().toInt(nullptr, 16)]->sliderReleased();
            mnl_chng = false;
        });
    }
    QVector<QSlider *> sldrs_lst{ui->hrzntlSldr_rfrsh_rate_lvl_1, ui->hrzntlSldr_rfrsh_rate_lvl_2, ui->hrzntlSldr_rfrsh_rate_lvl_3, ui->hrzntlSldr_rfrsh_rate_lvl_4, ui->hrzntlSldr_rfrsh_rate_lvl_5};
    QVector<QLabel *> lbls_vals_lst{ui->lbl_crrnt_rfrsh_rate_lvl_1, ui->lbl_crrnt_rfrsh_rate_lvl_2, ui->lbl_crrnt_rfrsh_rate_lvl_3, ui->lbl_crrnt_rfrsh_rate_lvl_4, ui->lbl_crrnt_rfrsh_rate_lvl_5};
    QVector<QLabel *> lbls_txt_lst{ui->lbl_txt_rfrsh_rate_lvl_1, ui->lbl_txt_rfrsh_rate_lvl_2, ui->lbl_txt_rfrsh_rate_lvl_3, ui->lbl_txt_rfrsh_rate_lvl_4, ui->lbl_txt_rfrsh_rate_lvl_5};
    std::function<void(uint8_t slctd_lbl)> select_rfrsh_rate_label = [=](uint8_t slctd_lbl) {
        QByteArray dpi_arr = "\x4d\xc1";
        for(uint8_t k = 0; k < lbls_txt_lst.count(); k++) {
            dpi_arr.append(sldrs_lst.at(k)->value() / sldrs_lst.at(k)->singleStep());
            if(k == slctd_lbl) {
                lbls_txt_lst[k]->setStyleSheet("color: mediumspringgreen;");
            } else {
                lbls_txt_lst[k]->setStyleSheet("color: white;");
            }
        }
        dpi_arr.append(slctd_lbl);
        dpi_arr.append("\x00\x05", 2);
        dpi_arr.append((PACKET_SIZE - dpi_arr.count()), '\x00');
        write_to_mouse_hid(dpi_arr);
    };
    for(uint8_t i = 0; i < sldrs_lst.count(); i++) {
        connect(sldrs_lst[i], &QSlider::valueChanged, this, [=](int val) {
            lbls_vals_lst[i]->setNum(val);
            int lbl_pos = round((static_cast<double>(sldrs_lst[i]->width()) / static_cast<double>(sldrs_lst[i]->maximum())) * val) -
                          (static_cast<double>(round((static_cast<double>(lbls_vals_lst[i]->width()) / static_cast<double>(sldrs_lst[i]->maximum())) * val)) / 1.3);
            lbls_vals_lst[i]->move(lbl_pos, lbls_vals_lst[i]->pos().y());
            select_rfrsh_rate_label(i);
        });
        connect(sldrs_lst[i], &QSlider::sliderMoved, this, [=](int val) {
            int mod = val % sldrs_lst[i]->singleStep();
            sldrs_lst[i]->setValue(val - mod + ((round(static_cast<double>(mod * 2) / 100.0) * 100) / 2));
        });
        connect(sldrs_lst[i], &QSlider::sliderReleased, this, [=]() {
            select_rfrsh_rate_label(i);
        });
    }
    QVector<QPushButton *> rfrsh_rate_bttns_lst{ui->pshBttn_rfrsh_rate_1000, ui->pshBttn_rfrsh_rate_500, ui->pshBttn_rfrsh_rate_250, ui->pshBttn_rfrsh_rate_125};
    for(uint8_t i = 0; i < rfrsh_rate_bttns_lst.count(); i++) {
        connect(rfrsh_rate_bttns_lst[i], &QPushButton::clicked, this, [=]() {
            QByteArray rfrsh_rate = "\x4d\xc3";
            rfrsh_rate.append(1 << i);
            rfrsh_rate.append((PACKET_SIZE - rfrsh_rate.count()), '\x00');
            write_to_mouse_hid(rfrsh_rate);
        });
    }
    QVector<QPushButton *> key_bttns_lst{ui->pshBttn_bttns_key_fnctns, ui->pshBttn_bttns_key_cmbntns, ui->pshBttn_bttns_key_macros};
    for(int i = 0; i < key_bttns_lst.count(); i++) {
        connect(key_bttns_lst[i], &QPushButton::clicked, this, [=]() {
            ui->stckdWdgt_key_fnctns_cmbntns_macros->setCurrentIndex(i);
            ui->pshBttn_strt_stp_rcrd_mcrs->setChecked(false);
            ui->lbl_cstm_key_cmb->clearFocus();
        });
    }
    ui->pshBttn_bttns_key_cmbntns->click();
    connect(ui->pshBttn_strt_stp_rcrd_mcrs, &QPushButton::toggled, this, [=](bool tggld) {
        QVector<QString> strt_stp_rcrd_names{tr("Start record"), tr("Stop record")};
        ui->pshBttn_strt_stp_rcrd_mcrs->setText(strt_stp_rcrd_names[static_cast<int>(tggld)]);
        while(tggld && (ui->lstWdgt_mcrs_evnts_lst->count() > 0)) {
            delete ui->lstWdgt_mcrs_evnts_lst->takeItem(0);
        }
        pressed_keys_lst.clear();
        pressed_keys_tmr_lst.clear();
        ui->pshBttn_save_mcrs->setEnabled(!tggld);
        ui->rdBttn_dly_btwn_evnts->setEnabled(!tggld);
    });
    anim_timer = new QTimer();
    connect(anim_timer, &QTimer::timeout, this, &MainWindow::slot_anim_timeout);
    no_sleep_timer = new QTimer();
    connect(no_sleep_timer, &QTimer::timeout, this, &MainWindow::slot_no_sleep_timeout);
    slot_no_sleep_timeout();
    if(init_flg != 2) {
        init_flg = 1;
    }
}

MainWindow::~MainWindow() {
    no_sleep_timer->stop();
    delete no_sleep_timer;
    remove_color_buttons_from_ui();
    anim_timer->stop();
    delete anim_timer;
    while(ui->lstWdgt_mcrs_evnts_lst->count() > 0) {
        delete ui->lstWdgt_mcrs_evnts_lst->takeItem(0);
    }
    delete minimizeAction;
    delete maximizeAction;
    delete restoreAction;
    delete quitAction;
    delete trayIconMenu;
    delete trayIcon;
    delete settings;
    clear_vector(&bttns_wtchrs_lst);
    clear_vector(&chrctrstc_frms_wtchrs_lst);
    clear_vector(&chrctrstc_frms_tmrs_lst);
    delete gen_widg;
    delete ui;
}

void MainWindow::showEvent(QShowEvent *) {
    if(is_frst_show) {
        is_frst_show = false;
    }
    resizeEvent(nullptr);
}

void MainWindow::resizeEvent(QResizeEvent *) {
    QPixmap bkgnd(":/images/background.png");
    QPalette main_palette;
    main_palette.setBrush(QPalette::Background, bkgnd.scaled(this->size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    this->setPalette(main_palette);
    ui->frm_slider->setStyleSheet("background-image: url(:/images/background_slider.png); border-right-width: 1px; border-right-style: solid; border-right-color: #1c2228;");
    ui->lbl_mouse_img_anim->setPixmap(apply_effects_on_mouse_image());
}

void MainWindow::mousePressEvent(QMouseEvent *event) {
    if(event->button() == Qt::LeftButton) {
        clck_pos = event->pos();
        is_drag = true;
    }
}

void MainWindow::mouseMoveEvent(QMouseEvent *event) {
    if(is_drag) {
        move((event->globalX() - clck_pos.x()), (event->globalY() - clck_pos.y()));
    }
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event) {
    if(event->button() == Qt::LeftButton) {
        is_drag = false;
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
    bool is_modifier_flg = true;
    QString key = get_key_name(event, &is_modifier_flg);
    if(ui->pshBttn_strt_stp_rcrd_mcrs->isChecked() && !event->isAutoRepeat() && key.count()) {
        pressed_keys_lst.append(event->key());
        if(ui->rdBttn_dly_btwn_evnts->isChecked()) {
            pressed_keys_tmr_lst.append(QTime());
            pressed_keys_tmr_lst.last().restart();
            if(pressed_keys_lst.count() > 1) {
                ui->lstWdgt_mcrs_evnts_lst->addItem(new QListWidgetItem(QIcon(":/images/icons/buttons/icon_key_hold_delay.png"), (QString::number(key_hold_timer.elapsed()) + " Milliseconds delay")));
            }
            key_hold_timer.restart();
        }
        ui->lstWdgt_mcrs_evnts_lst->addItem(new QListWidgetItem(QIcon(":/images/icons/buttons/icon_key_pressed.png"), key));
        ui->lstWdgt_mcrs_evnts_lst->setCurrentRow(ui->lstWdgt_mcrs_evnts_lst->count() - 1);
    } else if(ui->lbl_cstm_key_cmb->hasFocus() && !event->isAutoRepeat() && key.count()) {
        if(is_modifier_flg) {
            bool append_key = true;
            for(uint8_t i = 0; i < cmb_mdfrs_lst.count(); i++) {
                if(cmb_mdfrs_lst[i].compare(key, Qt::CaseInsensitive) == 0) {
                    append_key = false;
                    break;
                }
            }
            if(append_key) {
                cmb_mdfrs_lst.append(key);
            }
        } else {
            cmb_key = key;
        }
        form_keys_combination();
    }
}

void MainWindow::keyReleaseEvent(QKeyEvent *event) {
    QString key = get_key_name(event);
    if(ui->pshBttn_strt_stp_rcrd_mcrs->isChecked() && !event->isAutoRepeat() && key.count()) {
        for(int i = 0; i < pressed_keys_lst.count(); i++) {
            if(event->key() == pressed_keys_lst[i]) {
                if(ui->rdBttn_dly_btwn_evnts->isChecked()) {
                    ui->lstWdgt_mcrs_evnts_lst->addItem(new QListWidgetItem(QIcon(":/images/icons/buttons/icon_key_hold_delay.png"), (QString::number(pressed_keys_tmr_lst[i].elapsed()) + " Milliseconds delay")));
                }
                ui->lstWdgt_mcrs_evnts_lst->addItem(new QListWidgetItem(QIcon(":/images/icons/buttons/icon_key_released.png"), key));
                ui->lstWdgt_mcrs_evnts_lst->setCurrentRow(ui->lstWdgt_mcrs_evnts_lst->count() - 1);
                pressed_keys_lst.removeAt(i);
                pressed_keys_tmr_lst.removeAt(i);
                mcrs_prssd_cnt++;
                if(mcrs_prssd_cnt == 64) {
                    mcrs_prssd_cnt = 0;
                    ui->pshBttn_strt_stp_rcrd_mcrs->setChecked(false);
                }
                break;
            }
        }
    }
}

void MainWindow::on_pshBttn_add_clr_clicked() {
    QColorDialog dlg(this);
    QVector<QString> devs_clrs_lst{crrnt_tail_clr, crrnt_wheel_clr};
    dlg.setOption(QColorDialog::DontUseNativeDialog, true);
    dlg.setCurrentColor(QColor(devs_clrs_lst[ui->pshBttn_lghtnng_head->isChecked()]));
    if(dlg.exec() == QDialog::Accepted) {
        QColor color = dlg.selectedColor();
        if(color.isValid()) {
            int clrs_cnt = gen_widg->get_setting(settings, "USER_COLOR/Num").toInt() + 1;
            gen_widg->check_setting_exist(settings, "COLOR" + QString::number(clrs_cnt) + "/Red", color.red(), true);
            gen_widg->check_setting_exist(settings, "COLOR" + QString::number(clrs_cnt) + "/Green", color.green(), true);
            gen_widg->check_setting_exist(settings, "COLOR" + QString::number(clrs_cnt) + "/Blue", color.blue(), true);
            remove_color_buttons(clrs_cnt);
        }
    }
}

void MainWindow::on_pshBttn_clr_cstm_key_cmb_clicked() {
    cmb_key.clear();
    cmb_mdfrs_lst.clear();
    form_keys_combination();
}

void MainWindow::finish_init() {
    QVector<devices> devs_lst{TAIL, WHEEL};
    QVector<effects *> devs_effcts_lst{&crrnt_tail_effct, &crrnt_wheel_effct};
    QVector<speed *> devs_speed_lst{&crrnt_tail_spped, &crrnt_wheel_speed};
    QVector<QString *> devs_clrs_lst{&crrnt_tail_clr, &crrnt_wheel_clr};
    QByteArray tmp_in, tmp_out;
    for(int i = 0; i < devs_lst.count(); i++) {
        prepare_data_for_mouse_read_write(&tmp_out, &tmp_in, QByteArray("\x4d\xa0").append(devs_lst[i]));
        *(devs_effcts_lst[i]) = static_cast<effects>(tmp_in.mid(4, 1).toHex().toInt(nullptr, 16) - static_cast<int>(tmp_in.mid(4, 1).toHex().toInt(nullptr, 16) > TIC_TAC));
        *(devs_speed_lst[i]) = static_cast<speed>(tmp_in.mid(5, 1).toHex().toInt(nullptr, 16));
        *(devs_clrs_lst[i]) = (QColor("#" + QString(tmp_in.mid(8, 3).toHex())).name(QColor::HexRgb));
    }
    QVector<QPushButton *> rfrsh_rate_bttns_lst{ui->pshBttn_rfrsh_rate_1000, ui->pshBttn_rfrsh_rate_500, ui->pshBttn_rfrsh_rate_250, ui->pshBttn_rfrsh_rate_125};
    prepare_data_for_mouse_read_write(&tmp_out, &tmp_in, "\x4d\xc4");
    int crrnt_rfrsh_rate = log2(tmp_in.mid(3, 1).toHex().toInt(nullptr, 16));
    QVector<QSlider *> sldrs_lst{ui->hrzntlSldr_rfrsh_rate_lvl_1, ui->hrzntlSldr_rfrsh_rate_lvl_2, ui->hrzntlSldr_rfrsh_rate_lvl_3, ui->hrzntlSldr_rfrsh_rate_lvl_4, ui->hrzntlSldr_rfrsh_rate_lvl_5};
    prepare_data_for_mouse_read_write(&tmp_out, &tmp_in, "\x4d\xc2");
    mnl_chng = true;
    for(int i = 0; i < sldrs_lst.count(); i++) {
        sldrs_lst[i]->setValue(tmp_in.mid((3 + i), 1).toHex().toInt(nullptr, 16) * sldrs_lst[i]->singleStep());
        emit sldrs_lst[i]->valueChanged(sldrs_lst[i]->value());
    }
    emit sldrs_lst[tmp_in.mid(8, 1).toHex().toInt(nullptr, 16)]->sliderReleased();
    if((crrnt_rfrsh_rate < 0) || (crrnt_rfrsh_rate >= rfrsh_rate_bttns_lst.count())) {
        crrnt_rfrsh_rate = 0;
    }
    rfrsh_rate_bttns_lst[crrnt_rfrsh_rate]->click();
    mnl_chng = false;
    ui->pshBttn_lghtnng_head->setChecked(true);
    emit ui->pshBttn_lghtnng_head->toggled(true);
    anim_img_nam = ":/images/anim/positionToStrabismus_015.png";
    trayIcon->show();
    this->show();
}

void MainWindow::create_base_settings() {
    gen_widg->check_setting_exist(settings, "USER_COLOR/Num", 0, true);
}

void MainWindow::create_color_buttons() {
    int clrs_cnt = gen_widg->get_setting(settings, "USER_COLOR/Num").toInt();
    if(clrs_cnt == 0) {
        ui->pshBttn_edt_clrs->setChecked(false);
    }
    ui->pshBttn_edt_clrs->setEnabled(clrs_cnt != 0);
    QColor tmp_clr;
    QColor tmp_clr_dsbld;
    QVector<QString> devs_clrs_lst{crrnt_tail_clr, crrnt_wheel_clr};
    for(int i = 0; i < clrs_cnt; i++) {
        clrs_bttns_lst.push_back(new QRadioButton(ui->frm_clr_bttns));
        clrs_dlt_bttns_lst.push_back(new QPushButton(ui->frm_dlt_clr_bttns));
        tmp_clr.setRgb(gen_widg->get_setting(settings, "COLOR" + QString::number(i + 1) + "/Red").toUInt(), gen_widg->get_setting(settings, "COLOR" + QString::number(i + 1) + "/Green").toUInt(),
                       gen_widg->get_setting(settings, "COLOR" + QString::number(i + 1) + "/Blue").toUInt());
        clrs_bttns_lst[i]->setMinimumSize(COLOR_BUTTON_SIZE, COLOR_BUTTON_SIZE);
        clrs_bttns_lst[i]->setMaximumSize(COLOR_BUTTON_SIZE, COLOR_BUTTON_SIZE);
        clrs_bttns_lst[i]->setFocusPolicy(Qt::NoFocus);
        tmp_clr_dsbld.setRgb(tmp_clr.rgb());
        tmp_clr_dsbld.setAlpha(128);
        clrs_bttns_lst[i]->setStyleSheet(gen_widg->get_color_button_stylesheet(tmp_clr.name(QColor::HexRgb), tmp_clr_dsbld.name(QColor::HexArgb)));
        clrs_dlt_bttns_lst[i]->setStyleSheet(gen_widg->get_color_button_delete_stylesheet());
        if(QColor(devs_clrs_lst[ui->pshBttn_lghtnng_head->isChecked()]) == tmp_clr) {
            if(ui->pshBttn_edt_clrs->isChecked()) {
                crrnt_devs_clr_indxs_lst[ui->pshBttn_lghtnng_head->isChecked()] = i;
            } else {
                clrs_bttns_lst[i]->setChecked(true);
            }
        }
        ui->frm_clr_bttns->layout()->addWidget(clrs_bttns_lst[i]);
        connect(clrs_bttns_lst[i], &QRadioButton::clicked, this, [=]() {
            if(ui->pshBttn_edt_clrs->isChecked()) {
                QColorDialog dlg(this);
                dlg.setOption(QColorDialog::DontUseNativeDialog, true);
                QColor tmp_clr;
                if(crrnt_devs_clr_indxs_lst[ui->pshBttn_lghtnng_head->isChecked()] != -1) {
                    tmp_clr.setNamedColor("#" + clrs_bttns_lst[crrnt_devs_clr_indxs_lst[ui->pshBttn_lghtnng_head->isChecked()]]->styleSheet().split("#").last().split(",").first());
                } else {
                    tmp_clr.setNamedColor(devs_clrs_lst[ui->pshBttn_lghtnng_head->isChecked()]);
                }
                dlg.setCurrentColor(tmp_clr);
                if(dlg.exec() == QDialog::Accepted) {
                    QColor color = dlg.selectedColor();
                    if(color.isValid()) {
                        tmp_clr.setRgb(color.rgb());
                        tmp_clr.setAlpha(128);
                        clrs_bttns_lst[i]->setStyleSheet(gen_widg->get_color_button_stylesheet(color.name(QColor::HexRgb), tmp_clr.name(QColor::HexArgb)));
                        gen_widg->save_setting(settings, "COLOR" + QString::number(i + 1) + "/Red", color.red());
                        gen_widg->save_setting(settings, "COLOR" + QString::number(i + 1) + "/Green", color.green());
                        gen_widg->save_setting(settings, "COLOR" + QString::number(i + 1) + "/Blue", color.blue());
                    }
                }
            }
        });
        connect(clrs_bttns_lst[i], &QRadioButton::toggled, this, [=](bool tggld) {
            if(tggld) {
                crrnt_devs_clr_indxs_lst[ui->pshBttn_lghtnng_head->isChecked()] = i;
                mouse_set_color_for_device();
            }
        });
        clrs_dlt_bttns_lst[i]->setMinimumSize(14, 14);
        clrs_dlt_bttns_lst[i]->setMaximumSize(14, 14);
        clrs_dlt_bttns_lst[i]->setFocusPolicy(Qt::NoFocus);
        clrs_dlt_bttns_lst[i]->setIcon(QIcon(":/images/icons/lightning/icon_delete_color.png"));
        ui->frm_dlt_clr_bttns->layout()->addWidget(clrs_dlt_bttns_lst[i]);
        connect(clrs_dlt_bttns_lst[i], &QPushButton::clicked, this, [=]() {
            if(crrnt_devs_clr_indxs_lst[ui->pshBttn_lghtnng_head->isChecked()] > i) {
                crrnt_devs_clr_indxs_lst[ui->pshBttn_lghtnng_head->isChecked()]--;
            } else if(crrnt_devs_clr_indxs_lst[ui->pshBttn_lghtnng_head->isChecked()] == i) {
                crrnt_devs_clr_indxs_lst[ui->pshBttn_lghtnng_head->isChecked()] = -1;
            }
            int clrs_cnt_tmp = gen_widg->get_setting(settings, "USER_COLOR/Num").toInt();
            settings->beginGroup("COLOR" + QString::number(i + 1));
            settings->remove("");
            settings->endGroup();
            for(int k = (i + 1); k < clrs_cnt_tmp; k++) {
                gen_widg->check_setting_exist(settings, "COLOR" + QString::number(k) + "/Red", gen_widg->get_setting(settings, "COLOR" + QString::number(k + 1) + "/Red"), true);
                gen_widg->check_setting_exist(settings, "COLOR" + QString::number(k) + "/Green", gen_widg->get_setting(settings, "COLOR" + QString::number(k + 1) + "/Green"), true);
                gen_widg->check_setting_exist(settings, "COLOR" + QString::number(k) + "/Blue", gen_widg->get_setting(settings, "COLOR" + QString::number(k + 1) + "/Blue"), true);
                settings->beginGroup("COLOR" + QString::number(k + 1));
                settings->remove("");
                settings->endGroup();
            }
            remove_color_buttons(clrs_cnt_tmp - 1);
        });
    }
    change_color_frame_size();
}

void MainWindow::remove_color_buttons(int new_clrs_cnt) {
    remove_color_buttons_from_ui();
    gen_widg->save_setting(settings, "USER_COLOR/Num", new_clrs_cnt);
    create_color_buttons();
}

void MainWindow::remove_color_buttons_from_ui() {
    while(clrs_dlt_bttns_lst.count() != 0) {
        clrs_bttns_lst[0]->disconnect();
        clrs_dlt_bttns_lst[0]->disconnect();
        ui->frm_clr_bttns->layout()->removeWidget(clrs_bttns_lst[0]);
        ui->frm_dlt_clr_bttns->layout()->removeWidget(clrs_dlt_bttns_lst[0]);
        delete clrs_bttns_lst[0];
        clrs_bttns_lst.removeAt(0);
        delete clrs_dlt_bttns_lst[0];
        clrs_dlt_bttns_lst.removeAt(0);
    }
}

void MainWindow::change_color_frame_size() {
    int clrs_cnt = gen_widg->get_setting(settings, "USER_COLOR/Num").toInt();
    int max_clr_bttns_width = (clrs_cnt * COLOR_BUTTON_SIZE) + ((clrs_cnt - 1) * static_cast<bool>(clrs_cnt) * ui->frm_clr_bttns->layout()->spacing());
    ui->frm_clr_bttns->setFixedSize(max_clr_bttns_width, ui->frm_clr_bttns->height());
    bool shw_scrllbr = (max_clr_bttns_width > clr_scrl_area_max_width);
    if(shw_scrllbr) {
        max_clr_bttns_width = clr_scrl_area_max_width;
    }
    ui->scrollArea->setFixedWidth(max_clr_bttns_width);
    int clrs_sldr_hght = ui->scrollArea->styleSheet().split("padding").first().split("height").last().replace(":", "").replace(";", "").replace("px", "").replace(" ", "").toInt();
    int clrs_dlt_bttns_hght = ui->frm_dlt_clr_bttns->minimumHeight() + dynamic_cast<QGridLayout *>(ui->scrollAreaWidgetContents->layout())->verticalSpacing();
    ui->frm_clr_uppr_spcr->setFixedHeight(ui->frm_clr_uppr_spcr_inner->height() + (clrs_sldr_hght * static_cast<int>(shw_scrllbr)));
    ui->scrollArea->setFixedHeight(((ui->pshBttn_edt_clrs->isChecked() || !shw_scrllbr) * clrs_dlt_bttns_hght) + ui->frm_clr_bttns->minimumHeight() + (clrs_sldr_hght * static_cast<int>(shw_scrllbr)));
    ui->frm_clr_lwr_spcr->setFixedHeight(static_cast<int>(!ui->pshBttn_edt_clrs->isChecked() && (shw_scrllbr)) * clrs_dlt_bttns_hght);
}

void MainWindow::change_state_of_ui(bool flg) {
    ui->stckdWdgt_main_pages->setEnabled(flg);
    ui->lbl_mouse_img_anim->setEnabled(flg);
    ui->frm_slider->setEnabled(flg);
}

template <typename T>
void MainWindow::clear_vector(QVector<T *> *vctr) {
    while(vctr->count() != 0) {
        delete (*vctr)[0];
        vctr->removeAt(0);
    }
}

QString MainWindow::get_key_name(QKeyEvent *event, bool *is_modifier_flg) {
    QString key = QKeySequence(event->key()).toString();
    if((event->key() == Qt::Key_Shift) && (event->nativeScanCode() == LEFT_SHIFT)) {
        return "L SHIFT";
    } else if((event->key() == Qt::Key_Shift) && (event->nativeScanCode() == RIGHT_SHIFT)) {
        return "R SHIFT";
    } else if((event->key() == Qt::Key_Control) && (event->nativeScanCode() == LEFT_CTRL)) {
        return "L CTRL";
    } else if((event->key() == Qt::Key_Control) && (event->nativeScanCode() == RIGHT_CTRL)) {
        return "R CTRL";
    } else if((event->key() == Qt::Key_Alt) && (event->nativeScanCode() == LEFT_ALT)) {
        return "L ALT";
    } else if((event->key() == Qt::Key_Alt) && (event->nativeScanCode() == RIGHT_ALT)) {
        return "R ALT";
    } else if((event->key() == Qt::Key_Meta) && (event->nativeScanCode() == LEFT_WIN)) {
        return "L WIN";
    } else if((event->key() == Qt::Key_Meta) && (event->nativeScanCode() == RIGHT_WIN)) {
        return "R WIN";
    } else if((event->key() == 0) || ((key.count() != 0) && !key.at(0).isLetterOrNumber() && !key.at(0).isPunct() && !key.at(0).isSymbol())) {
        return "";
    } else {
        if(is_modifier_flg) {
            *is_modifier_flg = false;
        }
        return QKeySequence(event->key()).toString();
    }
}

void MainWindow::form_keys_combination() {
    QVector<QString> cmb_names{"L CTRL", "L ALT", "L SHIFT", "L WIN", "R CTRL", "R ALT", "R SHIFT", "R WIN"};
    QString cmb_str = "";
    for(uint8_t i = 0; i < cmb_names.count(); i++) {
        for(uint8_t j = 0; j < cmb_mdfrs_lst.count(); j++) {
            if(cmb_names[i].compare(cmb_mdfrs_lst[j], Qt::CaseInsensitive) == 0) {
                cmb_str.append(cmb_mdfrs_lst[j] + "+");
                break;
            }
        }
    }
    if(cmb_key.count() != 0) {
        cmb_str.append(cmb_key);
    } else {
        cmb_str.remove((cmb_str.count() - 1), 1);
    }
    ui->lbl_cstm_key_cmb->setText(cmb_str);
}

QPixmap MainWindow::apply_effects_on_mouse_image() {
    QVector<QString *> devs_clrs_lst{&crrnt_tail_clr, &crrnt_wheel_clr};
    QVector<effects> curr_dev_effect{crrnt_tail_effct, crrnt_wheel_effct};
    QVector<QColor> clrs_lst;
    for(uint8_t i = 0; i < devs_clrs_lst.count(); i++) {
        if(curr_dev_effect[i] == DISABLE) {
            clrs_lst.push_back(QColor(Qt::darkGray));
        } else if(crrnt_devs_clr_indxs_lst[i] != -1) {
            clrs_lst.push_back(QColor("#" + clrs_bttns_lst[crrnt_devs_clr_indxs_lst[i]]->styleSheet().split("#").last().split(",").first()));
        } else {
            clrs_lst.push_back(QColor(*(devs_clrs_lst[i])));
        }
    }
    QImage src_img(anim_img_nam);
    src_img = src_img.scaled(ui->lbl_mouse_img_anim->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    QVector<QString> mask_path{"", ""};
    QImage tail_img(src_img.size(), QImage::Format_ARGB32);
    QImage wheel_img(src_img.size(), QImage::Format_ARGB32);
    if((crrnt_page == BUTTONS) && (ui->pshBttn_bttns_side->isChecked())) {
        mask_path[0] = ":/images/effects/tail_rgb_mask_sider.png";
        mask_path[1] = ":/images/effects/wheel_mask_sider.png";
    } else if((crrnt_page == LIGHTNING) && (ui->pshBttn_lghtnng_tail->isChecked())) {
        mask_path[0] = ":/images/effects/tail_rgb_mask_trail.png";
    } else if(!((crrnt_page == BUTTONS) && (ui->pshBttn_bttns_top->isChecked()))) {
        mask_path[0] = ":/images/effects/tail_rgb_mask_strabismus.png";
        if(!((crrnt_page == LIGHTNING) && (ui->pshBttn_lghtnng_tail->isChecked()))) {
            mask_path[1] = ":/images/effects/wheel_mask_strabismus.png";
        }
    } else if((crrnt_page == BUTTONS) && (ui->pshBttn_bttns_top->isChecked())) {
        mask_path[1] = ":/images/effects/wheel_mask_position.png";
    }
    QVector<QImage *> mask_images{&tail_img, &wheel_img};
    for(uint8_t i = 0; i < mask_images.count(); i++) {
        mask_images[i]->fill(Qt::transparent);
        if(mask_path[i].count()) {
            QImage mask(mask_path[i]);
            mask = mask.scaled(src_img.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
            QPainter pntr(mask_images[i]);
            if(curr_dev_effect[i] < COLORS_CHANGING) {
                pntr.setPen(QPen(clrs_lst[i], Qt::SolidLine));
                pntr.setBrush(QBrush(clrs_lst[i], Qt::SolidPattern));
                pntr.drawRect(0, 0, src_img.width(), src_img.height());
            }
            pntr.setCompositionMode(QPainter::CompositionMode_Screen);
            if(curr_dev_effect[i] < COLORS_CHANGING) {
                pntr.drawImage(0, 0, mask.createHeuristicMask());
                pntr.setCompositionMode(QPainter::CompositionMode_DestinationIn);
            }
            pntr.drawImage(0, 0, mask);
            pntr.end();
        }
    }
    QPainter pntr(&src_img);
    pntr.setCompositionMode(QPainter::CompositionMode_DestinationOver);
    pntr.drawImage(0, 0, tail_img);
    pntr.drawImage(0, 0, wheel_img);
    pntr.end();
    return QPixmap::fromImage(src_img);
}

void MainWindow::prepare_data_for_mouse_read_write(QByteArray *arr_out, QByteArray *arr_in, QByteArray header) {
    arr_in->fill('\x00', INPUT_PACKET_SIZE);
    arr_out->clear();
    arr_out->append(header);
    arr_out->append((PACKET_SIZE - arr_out->count()), '\x00');
    write_to_mouse_hid((*arr_out), true, arr_in);
}

int MainWindow::write_to_mouse_hid(QByteArray &data, bool read, QByteArray *output) {
    if(mnl_chng) {
        return 0;
    }
    struct hid_device_info *devs = hid_enumerate(0x0, 0x0);
    struct hid_device_info *cur_dev = nullptr;
    int cnt = 0;
    uint crrnt_prdct_id = 0;
    QList<QString> path_lst;
    QList<uint> prod_id_lst;
    while(devs) {
        if((devs->vendor_id == VENDOR_ID) && ((devs->product_id == PRODUCT_ID_WIRE)/* || (devs->product_id == PRODUCT_ID_WIRELESS)*/) && (devs->interface_number == KEYBOARD) && (crrnt_prdct_id != devs->product_id)) {
            cur_dev = devs;
            crrnt_prdct_id = devs->product_id;
            path_lst.append(devs->path);
            prod_id_lst.append(crrnt_prdct_id);
            cnt++;
        }
        devs = devs->next;
    }
    hid_free_enumeration(devs);
    if(!cur_dev) {
        change_state_of_ui(false);
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
    hid_device *handle = hid_open_path(path.toLatin1().data());
    change_state_of_ui(crrnt_prdct_id == PRODUCT_ID_WIRE);
    int result = -1;
    if(handle) {
        result = hid_send_feature_report(handle, reinterpret_cast<unsigned char *>(data.data()), data.count());
    }
    if((result == 0) || (result == PACKET_SIZE)) {
        result = 0;
        if(read) {
            result = hid_read(handle, reinterpret_cast<unsigned char *>(output->data()), INPUT_PACKET_SIZE);
        }
    } else {
        const wchar_t *string = hid_error(handle);
        qDebug() << "Failure: " << QString::fromWCharArray(string, (sizeof(string) / sizeof(const wchar_t *))) << ";  code:" << result;
    }
    hid_close(handle);
    return result;
}

int MainWindow::mouse_set_color_for_device() {
    QByteArray clr_mod_spd_arr = "\x4d\xa1";
    QVector<effects> devs_effcts_lst{crrnt_tail_effct, crrnt_wheel_effct};
    QVector<speed *> devs_speed_lst{&crrnt_tail_spped, &crrnt_wheel_speed};
    QVector<QString *> devs_clrs_lst{&crrnt_tail_clr, &crrnt_wheel_clr};
    QColor clr;
    if(crrnt_devs_clr_indxs_lst[ui->pshBttn_lghtnng_head->isChecked()] != -1) {
        clr.setNamedColor("#" + clrs_bttns_lst[crrnt_devs_clr_indxs_lst[ui->pshBttn_lghtnng_head->isChecked()]]->styleSheet().split("#").last().split(",").first());
    } else {
        clr.setNamedColor(*(devs_clrs_lst[ui->pshBttn_lghtnng_head->isChecked()]));
    }
    *(devs_clrs_lst[ui->pshBttn_lghtnng_head->isChecked()]) = clr.name(QColor::HexRgb);
    *(devs_speed_lst[ui->pshBttn_lghtnng_head->isChecked()]) = static_cast<speed>(ui->hrzntlSldr_effct_spd->value());
    clr_mod_spd_arr.append(static_cast<devices>(ui->pshBttn_lghtnng_head->isChecked()));
    clr_mod_spd_arr.append(devs_effcts_lst[ui->pshBttn_lghtnng_head->isChecked()] + static_cast<int>(devs_effcts_lst[ui->pshBttn_lghtnng_head->isChecked()] > TIC_TAC));
    clr_mod_spd_arr.append(*(devs_speed_lst[ui->pshBttn_lghtnng_head->isChecked()]));
    clr_mod_spd_arr.append("\x08\x08");
    clr_mod_spd_arr.append(clr.red());
    clr_mod_spd_arr.append(clr.green());
    clr_mod_spd_arr.append(clr.blue());
    clr_mod_spd_arr.append((PACKET_SIZE - clr_mod_spd_arr.count()), '\x00');
    if(!mnl_chng) {
        ui->lbl_mouse_img_anim->setPixmap(apply_effects_on_mouse_image());
    }
    return write_to_mouse_hid(clr_mod_spd_arr);
}

int MainWindow::mouse_non_sleep() {
    QByteArray non_sleep_arr{"\x4d\x90\xde\x30\xfe\xff\xff\xff\xda\x98\x20\x76\xd5\xd1\xae\x68\xa0\xe6\xe9\x03\xbc\xd8\x28\x00\xf3\xe0\x81\x77\xb8\xee\x37\x06", PACKET_SIZE};
    return write_to_mouse_hid(non_sleep_arr);
}

void MainWindow::slot_no_sleep_timeout() {
    no_sleep_timer->stop();
    int res = mouse_non_sleep();
    int interval_ms = NO_SLEEP_INTERVAL_MS;
    if((init_flg == 0) && (res == -1)) {
        interval_ms = INIT_INTERVAL_MS;
    }
    no_sleep_timer->setInterval(interval_ms);
    no_sleep_timer->start();
    if((init_flg == 1) || ((res != -1) && (init_flg == 0))) {
        init_flg = 2;
        finish_init();
    }
}

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
        anim_timer->setInterval(IMG_ANIM_INTERVAL_MS);
        anim_timer->start();
    } else {
        anim_img_nam = ":/images/anim/" + anim_img_nam + tmp + ".png";
        ui->lbl_mouse_img_anim->setPixmap(apply_effects_on_mouse_image());
    }
}
