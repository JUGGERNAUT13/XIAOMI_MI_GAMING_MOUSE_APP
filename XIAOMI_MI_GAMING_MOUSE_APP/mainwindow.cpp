#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "ui_dialog_reset_settings.h"

#define ORIGINAL_MAX_COLORS     8
#define PART_SIZE               12
#define LBL_ANIM_INTERVAL_MS    17
#define COLOR_BUTTON_SIZE       20
#define IMG_ANIM_INTERVAL_MS    30/*500*/
#define PACKET_SIZE             32
#define INPUT_PACKET_SIZE       64
#define KEY_MACRO_LENGHT        64
#define INIT_INTERVAL_MS        1000
#define NO_SLEEP_INTERVAL_MS    /*290000*/30000
#define VENDOR_ID               0x2717
#define PRODUCT_ID_WIRE         0x5009
//#define PRODUCT_ID_WIRELESS     0x500b        //NOT WORKING IN WIRELESS MODE, BUT COMMANDS EXEC SUCCESSFULLY

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    this->setWindowFlags(this->windowFlags() | Qt::FramelessWindowHint);
    ui->frm_mouse_img_anim->setAutoFillBackground(true);
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
    for(uint8_t i = 0; i < effects_lst.count(); i++) {
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
//            anim_1(anim_img_nam, 15, -1, -1);     //orig
            anim_1(anim_img_nam, 14, -1, -1);
        } else {
//            anim_1(anim_img_nam, 0, 16, 1);        //orig
            anim_1(anim_img_nam, 1, 16, 1);
        }
    };
    QList<QWidget *> pnl_chldrns_lst = ui->frm_slider->findChildren<QWidget *>();
    for(uint8_t i = 0; i < pnl_chldrns_lst.count(); i++) {
        pnl_chldrns_lst[i]->setAttribute(Qt::WA_TranslucentBackground, true);
    }
    ui->frm_bttns_top_side_swtch->setVisible(false);
    QVector<QFrame *> chrctrstc_frms_outr_lst{ui->frm_1_pgrmmbl_outr, ui->frm_2_drbl_outr, ui->frm_3_dual_mod_outr, ui->frm_4_ergnmc_outr, ui->frm_5_optcl_sns_outr, ui->frm_6_aimng_outr,
                                              ui->frm_7_lghtnng_outr, ui->frm_8_rfrsh_rate_outr};
    for(uint8_t i = 0; i < chrctrstc_frms_outr_lst.count(); i++) {
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
                QVector<QVector<QString>> icon_clsrs{{"#80242830", "#c01b272d"}, {"#00e39b", "#00e39b"}};
                QVector<QString> icon_nams_lst{"programmable", "durable", "dual_mode", "ergonomic", "optical_sens", "aiming_plus", "lightning", "refresh_rate"};
                QString styleseet = "QLabel { background-image: url(:/images/icons/home/icon_" + icon_nams_lst[i] + ".png); background-position: center center; background-repeat: no-repeat; "
                                    "background-color: qradialgradient(cx: 0.5, cy: 0.5, radius: 0.5, fx: 0.5, fy: 0.5, stop: 0.93 " + icon_clsrs[leave_flg][0] + ", stop: 0.95 " +
                                    icon_clsrs[leave_flg][1] + ", stop: 0.99 transparent); }\nQLabel:hover { background-color: qradialgradient(cx: 0.5, cy: 0.5, radius: 0.5, fx: 0.5, fy: 0.5, "
                                    "stop: 0.95 #00e39b, stop: 0.99 transparent); }";
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
    QVector<QString> icons_names{"page_home", "page_buttons", "page_lightning", "page_speed", "page_update"};
    QVector<QString> bttn_icon_types{"unchecked", "checked"};
    QVector<QPushButton *>page_bttns_lst{ui->pshBttn_1_home, ui->pshBttn_2_buttons, ui->pshBttn_3_lightning, ui->pshBttn_4_speed, ui->pshBttn_5_update};
    for(uint8_t i = 0; i < page_bttns_lst.count(); i++) {
        bttns_wtchrs_lst.push_back(new Enter_Leave_Watcher(this));
        page_bttns_lst[i]->installEventFilter(bttns_wtchrs_lst[i]);
        connect(bttns_wtchrs_lst[i], &Enter_Leave_Watcher::signal_object_enter_leave_event, this, [=](QObject *obj, QEvent::Type evnt_typ) {
            QPushButton *button = qobject_cast<QPushButton*>(obj);
            if(button && !button->isChecked()) {
                button->setIcon(QIcon(":/images/icons/icon_" + icons_names[i] + "_" + bttn_icon_types[QEvent::Leave - evnt_typ] + ".png"));
            }
        });
        page_bttns_lst[i]->setText("    " + page_bttns_lst[i]->text());
        connect(page_bttns_lst[i], &QPushButton::toggled, this, [=](bool tggld) {
            ui->pshBttn_strt_stp_rcrd_mcrs->setChecked(false);
            ui->lbl_cstm_key_cmb->clearFocus();
            page_bttns_lst[i]->setIcon(QIcon(":/images/icons/icon_" + icons_names[i] + "_" + bttn_icon_types[static_cast<int>(tggld)] + ".png"));
            if(tggld) {
                ui->stckdWdgt_mouse_img_overlay->setCurrentWidget(ui->page_1_1_overlay_empty);
                ui->stckdWdgt_all_pages->setCurrentIndex(i == UPDATE);
                ui->frm_bttns_top_side_swtch->setVisible(i == BUTTONS);
                if(i == UPDATE) {
                    change_backgound_for_page_widget(ui->page_2_update);
                } else {
                    ui->stckdWdgt_main_pages->setCurrentIndex(i);
                    prev_page = crrnt_page;
                    crrnt_page = static_cast<pages>(i);
                }
                if((prev_page == HOME) || (prev_page == SPEED)) {
                    if(crrnt_page == BUTTONS) {
//                        anim_2(15, -1, -1);       //orig
                        anim_2(14, -1, -1);
                    } else if((crrnt_page == LIGHTNING) && ui->pshBttn_lghtnng_tail->isChecked()) {
//                        anim_1("trailToStrabismus_0", 15, -1, -1);        //orig
                        anim_1("trailToStrabismus_0", 14, -1, -1);
                    }
                } else if(prev_page == BUTTONS) {
                    if((crrnt_page == HOME) || (crrnt_page == SPEED)) {
//                        anim_2(0, 16, 1);           //orig
                        anim_2(1, 16, 1);
                    } else if(crrnt_page == LIGHTNING) {
                        if(ui->pshBttn_lghtnng_head->isChecked()) {
//                            anim_2(0, 16, 1);       //orig
                            anim_2(1, 16, 1);
                        } else {
                            anim_3(crrnt_page);
                        }
                    }
                } else if(prev_page == LIGHTNING) {
                    if((crrnt_page == HOME) || (crrnt_page == SPEED)) {
                        if(ui->pshBttn_lghtnng_tail->isChecked()) {
//                            anim_1("trailToStrabismus_0", 0, 16, 1);        //orig
                            anim_1("trailToStrabismus_0", 1, 16, 1);
                        }
                    } else if(crrnt_page == BUTTONS) {
                        if(ui->pshBttn_lghtnng_head->isChecked()) {
//                            anim_2(15, -1, -1);       //orig
                            anim_2(14, -1, -1);
                        } else {
                            anim_3(crrnt_page);
                        }
                    }
                }
            }
        });
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
        for(uint8_t i = 0; i < clrs_bttns_lst.count(); i++) {
            avlbl_clr = "#" + clrs_bttns_lst[i]->styleSheet().split("#").last().split(",").first();
            if(devs_clrs_lst[ui->pshBttn_lghtnng_head->isChecked()].compare(avlbl_clr, Qt::CaseInsensitive) == 0) {
                clrs_bttns_lst[i]->setChecked(true);
                break;
            } else if((crrnt_devs_clr_indxs_lst[ui->pshBttn_lghtnng_head->isChecked()] == -1) && clrs_bttns_lst[i]->isChecked()) {
                clrs_bttns_lst[i]->setAutoExclusive(false);
                clrs_bttns_lst[i]->setChecked(false);
                clrs_bttns_lst[i]->setAutoExclusive(true);
            }
        }
        effects_lst[devs_effcts_lst[ui->pshBttn_lghtnng_head->isChecked()]]->setChecked(true);
        emit effects_lst[devs_effcts_lst[ui->pshBttn_lghtnng_head->isChecked()]]->toggled(true);
        if(!mnl_rdng) {
            anim_1("trailToStrabismus_0", crnt_val, end_val, cnt_dir);
        }
        mnl_chng = false;
    };
    QVector<QPushButton *> bttns_lst{ui->pshBttn_bttns_top, ui->pshBttn_bttns_side, ui->pshBttn_lghtnng_head, ui->pshBttn_lghtnng_tail};
//    QVector<QVector<int>> anim_params_lst{{0, 16, 1}, {15, -1, -1}};         //orig
    QVector<QVector<int>> anim_params_lst{{1, 16, 1}, {14, -1, -1}};
    for(uint8_t i = 0; i < bttns_lst.count(); i++) {
        connect(bttns_lst[i], &QPushButton::toggled, this, [=](bool tggld) {
            if(tggld) {
                if(crrnt_page == BUTTONS) {
                    ui->stckdWdgt_mouse_img_overlay->setCurrentWidget(ui->page_1_1_overlay_empty);
                    ui->pshBttn_bttns_key_cmbntns->click();
                    anim_1("siderToPosition_0", anim_params_lst[i % 2][0], anim_params_lst[i % 2][1], anim_params_lst[i % 2][2]);
                } else if(crrnt_page == LIGHTNING) {
                    change_currnt_dev(anim_params_lst[i % 2][0], anim_params_lst[i % 2][1], anim_params_lst[i % 2][2]);
                }
            }
        });
    }
    key_fnc_bttns_grp = new QButtonGroup(this);
    QVector<QRadioButton *> key_fnc_bttns_lst{ui->rdBttn_key_func_lft_clck, ui->rdBttn_key_func_rght_clck, ui->rdBttn_key_func_mddl_clck, ui->rdBttn_key_func_mv_bck, ui->rdBttn_key_func_mv_frwrd,
                                              ui->rdBttn_key_func_rise_dpi, ui->rdBttn_key_func_lwr_dpi, ui->rdBttn_key_func_trn_dpi, ui->rdBttn_key_func_vlm_up, ui->rdBttn_key_func_vlm_dwn,
                                              ui->rdBttn_key_func_slnt_mod, ui->rdBttn_key_cmb_cls_wndw, ui->rdBttn_key_cmb_prev_tab_in_brwsr, ui->rdBttn_key_cmb_cut, ui->rdBttn_key_cmb_shw_dsktp,
                                              ui->rdBttn_key_cmb_nxt_tab_in_brwsr, ui->rdBttn_key_cmb_copy, ui->rdBttn_key_cmb_undo, ui->rdBttn_key_cmb_redo, ui->rdBttn_key_cmb_paste};
    for(uint8_t i = 0; i < key_fnc_bttns_lst.count(); i++) {
        key_fnc_bttns_grp->addButton(key_fnc_bttns_lst[i]);
        connect(key_fnc_bttns_lst[i], &QRadioButton::toggled, this, [=](bool tggld) {
            if(tggld) {
                bind_mouse_button(get_current_mouse_button(), i);
            }
        });
    }
    key_fnc_bttns_grp->setExclusive(true);
    connect(ui->hrzntlSldr_effct_spd, &QSlider::valueChanged, this, &MainWindow::mouse_set_color_for_device);
    QVector<QPushButton *> speed_bttns_lst{ui->pshBttn_speed_rfrsh_rate, ui->pshBttn_speed_dpi};
    for(uint8_t i = 0; i < speed_bttns_lst.count(); i++) {
        connect(speed_bttns_lst[i], &QPushButton::toggled, this, [=]() {
            ui->stckdWdgt_rfrsh_rate_n_dpi->setCurrentIndex(i);
            QByteArray tmp_in, tmp_out;
            QVector<QSlider *> sldrs_lst{ui->hrzntlSldr_rfrsh_rate_lvl_1, ui->hrzntlSldr_rfrsh_rate_lvl_2, ui->hrzntlSldr_rfrsh_rate_lvl_3, ui->hrzntlSldr_rfrsh_rate_lvl_4, ui->hrzntlSldr_rfrsh_rate_lvl_5};
            prepare_data_for_mouse_read_write(&tmp_out, &tmp_in, "\x4d\xc2");
            mnl_chng = true;
            int crrnt_rfrsh_rate_lvl = abs(tmp_in.mid(8, 1).toHex().toInt(nullptr, 16));
            if(crrnt_rfrsh_rate_lvl > (sldrs_lst.count() - 1)) {
                crrnt_rfrsh_rate_lvl = sldrs_lst.count() - 1;
            }
            emit sldrs_lst[crrnt_rfrsh_rate_lvl]->sliderReleased();
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
    for(uint8_t i = 0; i < key_bttns_lst.count(); i++) {
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
    delete key_fnc_bttns_grp;
    delete gen_widg;
    delete ui;
}

void MainWindow::showEvent(QShowEvent *) {
    if(is_frst_show) {
        is_frst_show = false;
    }
    resizeEvent(nullptr);
    if(!ui->stckdWdgt_main_pages->isEnabled()) {
        crrnt_ui_state = ui->stckdWdgt_main_pages->isEnabled();
        change_state_of_ui(crrnt_ui_state);
    }
}

void MainWindow::resizeEvent(QResizeEvent *) {
    resize_images();
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
            cmb_key = key.toUpper();
        }
        form_keys_combination();
    }
}

void MainWindow::keyReleaseEvent(QKeyEvent *event) {
    QString key = get_key_name(event);
    if(ui->pshBttn_strt_stp_rcrd_mcrs->isChecked() && !event->isAutoRepeat() && key.count()) {
        for(uint8_t i = 0; i < pressed_keys_lst.count(); i++) {
            if(event->key() == pressed_keys_lst[i]) {
                if(ui->rdBttn_dly_btwn_evnts->isChecked()) {
                    ui->lstWdgt_mcrs_evnts_lst->addItem(new QListWidgetItem(QIcon(":/images/icons/buttons/icon_key_hold_delay.png"), (QString::number(pressed_keys_tmr_lst[i].elapsed()) + " Milliseconds delay")));
                }
                ui->lstWdgt_mcrs_evnts_lst->addItem(new QListWidgetItem(QIcon(":/images/icons/buttons/icon_key_released.png"), key));
                ui->lstWdgt_mcrs_evnts_lst->setCurrentRow(ui->lstWdgt_mcrs_evnts_lst->count() - 1);
                pressed_keys_lst.removeAt(i);
                pressed_keys_tmr_lst.removeAt(i);
                mcrs_prssd_cnt++;
                if(mcrs_prssd_cnt == KEY_MACRO_LENGHT) {
                    mcrs_prssd_cnt = 0;
                    ui->pshBttn_strt_stp_rcrd_mcrs->setChecked(false);
                }
                break;
            }
        }
    }
}

void MainWindow::on_pshBttn_rst_sttngs_clicked() {
    Dialog_Reset_Settings rst_sttngs_dlg(this);
    if(rst_sttngs_dlg.exec() == QDialog::Accepted) {
        crrnt_devs_clr_indxs_lst = {-1, -1};
        crrnt_tail_clr = crrnt_wheel_clr = QColor(Qt::green).name();
        crrnt_tail_effct = crrnt_wheel_effct = STATIC;
        crrnt_tail_spped = crrnt_wheel_speed = SPEED_4;
        QVector<devices> mouse_dvcs{TAIL, WHEEL};
        QVector<QColor> mouse_dflt_clrs{QColor(crrnt_tail_clr), QColor(crrnt_wheel_clr)};
        QVector<effects> mouse_dflt_effcts{crrnt_tail_effct, crrnt_wheel_effct};
        QVector<speed> mouse_dflt_spd{crrnt_tail_spped, crrnt_wheel_speed};
        QByteArray clr_mod_spd_arr;
        for(uint8_t i = 0; i < mouse_dvcs.count(); i++) {
            clr_mod_spd_arr.clear();
            clr_mod_spd_arr.append("\x4d\xa1");
            clr_mod_spd_arr.append(mouse_dvcs[i]);
            clr_mod_spd_arr.append(mouse_dflt_effcts[i]);
            clr_mod_spd_arr.append(mouse_dflt_spd[i]);
            clr_mod_spd_arr.append("\x08\x08");
            clr_mod_spd_arr.append(mouse_dflt_clrs[i].red());
            clr_mod_spd_arr.append(mouse_dflt_clrs[i].green());
            clr_mod_spd_arr.append(mouse_dflt_clrs[i].blue());
            clr_mod_spd_arr.append((PACKET_SIZE - clr_mod_spd_arr.count()), '\x00');
            write_to_mouse_hid(clr_mod_spd_arr);
        }
        remove_color_buttons(gen_widg->get_setting(settings, "USER_COLOR/Num").toInt());
        ui->pshBttn_rfrsh_rate_1000->setChecked(true);
        emit ui->pshBttn_rfrsh_rate_1000->toggled(true);
        QVector<uint16_t> crrnt_dpi{100, 800, 1600, 2400, 4800};
        QVector<QSlider *> crrnt_dpi_sldrs{ui->hrzntlSldr_rfrsh_rate_lvl_1, ui->hrzntlSldr_rfrsh_rate_lvl_2, ui->hrzntlSldr_rfrsh_rate_lvl_3, ui->hrzntlSldr_rfrsh_rate_lvl_4, ui->hrzntlSldr_rfrsh_rate_lvl_5};
        for(uint8_t i = 0; i < crrnt_dpi_sldrs.count(); i++) {
            crrnt_dpi_sldrs[i]->setValue(crrnt_dpi[i]);
        }
        emit ui->hrzntlSldr_rfrsh_rate_lvl_3->sliderReleased();
        ui->pshBttn_bttns_top->setChecked(true);
        emit ui->pshBttn_bttns_top->toggled(true);
        ui->pshBttn_speed_rfrsh_rate->setChecked(true);
        emit ui->pshBttn_speed_rfrsh_rate->toggled(true);
        ui->pshBttn_lghtnng_head->setChecked(true);
        emit ui->pshBttn_lghtnng_head->toggled(true);
        ui->pshBttn_1_home->setChecked(true);
        emit ui->pshBttn_1_home->toggled(true);
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

void MainWindow::on_pshBttn_sav_cstm_key_cmb_clicked() {
    if(ui->lbl_cstm_key_cmb->text().count()) {
        QAbstractButton *checked = key_fnc_bttns_grp->checkedButton();
        if(checked) {
            key_fnc_bttns_grp->setExclusive(false);
            checked->setChecked(false);
            key_fnc_bttns_grp->setExclusive(true);
        }
        QList<QString> cmb_keys_lst = ui->lbl_cstm_key_cmb->text().split("+");
        cmb_keys_lst.removeAll(QString(""));
        if(cmb_key.contains("+")) {
            cmb_keys_lst.append("+");
        }
        QVector<mouse_keys> mouse_keys_lst{KEY_CTRL_BREAK, KEY_A, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G, KEY_H, KEY_I, KEY_J, KEY_K, KEY_L, KEY_M, KEY_N, KEY_O, KEY_P, KEY_Q, KEY_R, KEY_S, KEY_T,
                                           KEY_U, KEY_V, KEY_W, KEY_X, KEY_Y, KEY_Z, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9, KEY_0, KEY_ENTER, KEY_ESCAPE, KEY_BACKSPACE,
                                           KEY_TAB, KEY_SPACE, KEY_SUB, KEY_EQUAL, KEY_SQR_BRCKT_OPEN, KEY_SQR_BRCKT_CLOSE, KEY_BACKSLASH, KEY_SEMICOLON, KEY_APOSTROPHE, KEY_BACKTICK, KEY_LESS,
                                           KEY_GREATER, KEY_SLASH, KEY_CAPS_LOCK, KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12, KEY_PRINT_SCREEN,
                                           KEY_SCROLL_LOCK, KEY_PAUSE, KEY_INSERT, KEY_HOME, KEY_PAGE_UP, KEY_DELETE, KEY_END, KEY_PAGE_DOWN, KEY_RIGHT_ARROW, KEY_LEFT_ARROW, KEY_DOWN_ARROW,
                                           KEY_UP_ARROW, KEY_NUM_LOCK, KEY_NUMPAD_DIV, KEY_NUMPAD_MULT, KEY_NUMPAD_SUB, KEY_NUMPAD_ADD, KEY_NUMPAD_1, KEY_NUMPAD_2, KEY_NUMPAD_3, KEY_NUMPAD_4,
                                           KEY_NUMPAD_5, KEY_NUMPAD_6, KEY_NUMPAD_7, KEY_NUMPAD_8, KEY_NUMPAD_9, KEY_NUMPAD_0, KEY_NUMPAD_DOT, KEY_APPS_MENU};
        QVector<QString> keys_lst{"CTRLBREAK", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "Q", "P", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "1", "2", "3", "4",
                                  "5", "6", "7", "8", "9", "0", "RETURN", "ESC", "BACKSPACE", "TAB", "SPACE", "-", "=", "[", "]", "\\", ";", "'", "`", "<", ">", "/", "CAPSLOCK", "F1", "F2", "F3",
                                  "F4", "F5", "F6", "F7", "F8", "F9", "F10", "F11", "F12", "PRINT", "SCROLLLOCK", "PAUSE", "INS", "HOME", "PGUP", "DEL", "END", "PGDOWN", "RIGHT", "LEFT", "DOWN",
                                  "UP", "NUMLOCK", "/", "*", "-", "+", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", ".", "MENU"};
        QVector<mouse_key_modifiers> mouse_keys_mdfrs_lst{KEY_LEFT_CTRL, KEY_LEFT_SHIFT, KEY_LEFT_ALT, KEY_LEFT_WIN, KEY_RIGHT_CTRL, KEY_RIGHT_SHIFT, KEY_RIGHT_ALT, KEY_RIGHT_WIN};
        QVector<QString> keys_mdfrs_lst{"L CTRL", "L SHIFT", "L ALT", "L WIN", "R CTRL", "R SHIFT", "R ALT", "R WIN"};
        uint8_t key = 0;
        if(cmb_key.count()) {
            for(uint8_t i = 0; i < keys_lst.count(); i++) {
                if(cmb_key.compare(keys_lst[i], Qt::CaseInsensitive) == 0) {
                    key = mouse_keys_lst[i];
                    break;
                }
            }
            cmb_keys_lst.removeAt(cmb_keys_lst.count() - 1);
        }
        uint8_t modifiers = 0;
        for(uint8_t i = 0; i < cmb_keys_lst.count(); i++) {
            for(uint8_t k = 0; k < keys_mdfrs_lst.count(); k++) {
                if(cmb_keys_lst[i].compare(keys_mdfrs_lst[k], Qt::CaseInsensitive) == 0) {
                    modifiers += mouse_keys_mdfrs_lst[k];
                    break;
                }
            }
        }
        bind_mouse_button(get_current_mouse_button(), (COMBOS_COUNT - 1), modifiers, key);
    }
}

void MainWindow::finish_init() {
    read_mouse_parameters();
    anim_img_nam = ":/images/anim/positionToStrabismus_015.png";
    trayIcon->show();
    this->show();
}

void MainWindow::resize_images() {
    QPixmap bkgnd(":/images/background.png");
    QPalette main_palette;
    main_palette.setBrush(this->backgroundRole(), bkgnd.scaled(this->size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
    this->setPalette(main_palette);
    ui->frm_slider->setStyleSheet("background-image: url(:/images/background_slider.png); border-right-width: 1px; border-right-style: solid; border-right-color: #1c2228;");
    draw_mouse_anim_img(anim_img_nam, true);
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
    if(!mnl_rdng) {
        ui->stckdWdgt_main_pages->setEnabled(flg);
        ui->frm_mouse_img_anim->setEnabled(flg);
        ui->frm_slider->setEnabled(flg);
        ui->lbl_chrg_lvl->setVisible(flg);
        ui->stckdWdgt_all_pages->setCurrentIndex(ui->stckdWdgt_all_pages->indexOf(ui->page_3_unit_not_detected) * !flg);
        if(!flg && !is_frst_show/* && !this->isHidden() && this->isActiveWindow()*/) {
            change_backgound_for_page_widget(ui->page_3_unit_not_detected, flg);
        } else if(!is_frst_show/* && !this->isHidden() && this->isActiveWindow()*/) {
            mnl_rdng = true;
            read_mouse_parameters();
            mnl_rdng = false;
            resize_images();
        }
    }
}

template <typename T>
void MainWindow::clear_vector(QVector<T *> *vctr) {
    while(vctr->count() != 0) {
        delete (*vctr)[0];
        vctr->removeAt(0);
    }
}

uint8_t MainWindow::get_current_mouse_button() {
    QVector<QRadioButton *> mouse_bttns{ui->rdBtn_lft_bttn, ui->rdBttn_rght_bttn, ui->rdBttn_scrll_bttn, ui->rdBttn_m5_bttn,
                                        ui->rdBttn_m4_bttn, ui->rdBttn_rise_dpi, ui->rdBttn_lwr_dpi, ui->rdBttn_amng_bttn};
    QVector<QPushButton *> crrnt_mouse_pos_bttn{ui->pshBttn_bttns_top, ui->pshBttn_bttns_top, ui->pshBttn_bttns_top, ui->pshBttn_bttns_side,
                                                ui->pshBttn_bttns_side, ui->pshBttn_bttns_top, ui->pshBttn_bttns_top, ui->pshBttn_bttns_side};
    uint8_t crrnt_mouse_bttn = 0;
    for(uint8_t i = 0; i < mouse_bttns.count(); i++) {
        if(crrnt_mouse_pos_bttn[i]->isChecked() && mouse_bttns[i]->isChecked()) {
            crrnt_mouse_bttn = i;
            break;
        }
    }
    return crrnt_mouse_bttn;
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
        return key;
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

void MainWindow::change_backgound_for_page_widget(QWidget *page, bool draw_gear) {
    if(page) {
        QPoint crrnt_frm_abslt_pos = page->mapTo(ui->centralwidget, ui->centralwidget->rect().topLeft());
        QImage cmbn_img(page->size(), QImage::Format_ARGB32);
        cmbn_img.fill(Qt::transparent);
        QPainter pntr(&cmbn_img);
        pntr.setCompositionMode(QPainter::CompositionMode_SourceOver);
        ui->frm_mouse_img_anim->render(&pntr, QPoint(((page->width() - ui->frm_mouse_img_anim->width()) / 2), 0), QRegion(), QWidget::IgnoreMask);
        pntr.drawImage(0, 0, QImage(":/images/background.png"), crrnt_frm_abslt_pos.x(), crrnt_frm_abslt_pos.y(), page->width(), page->height());
        pntr.setCompositionMode(QPainter::CompositionMode_Plus);
        if(draw_gear) {
            QImage gear_img(":/images/gear.png");
            pntr.drawImage(((page->width() - gear_img.width()) / 2), ((ui->frm_mouse_img_anim->height() / 2) - gear_img.height()), gear_img);
        }
        page->setAutoFillBackground(true);
        QPalette palette;
        palette.setBrush(page->backgroundRole(), cmbn_img);
        page->setPalette(palette);
    }
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
    QImage src_img = QImage(anim_img_nam).scaled(ui->frm_mouse_img_anim->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
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
        }
    }
    QImage cmbn_img(ui->frm_mouse_img_anim->size(), QImage::Format_ARGB32);
    cmbn_img.fill(Qt::transparent);
    QPainter pntr(&cmbn_img);
    pntr.setCompositionMode(QPainter::CompositionMode_DestinationOver);
    pntr.drawImage(((ui->frm_mouse_img_anim->width() - src_img.width()) / 2), ((ui->frm_mouse_img_anim->height() - src_img.height()) / 2), src_img);
    pntr.drawImage(((ui->frm_mouse_img_anim->width() - src_img.width()) / 2), ((ui->frm_mouse_img_anim->height() - src_img.height()) / 2), tail_img);
    pntr.drawImage(((ui->frm_mouse_img_anim->width() - src_img.width()) / 2), ((ui->frm_mouse_img_anim->height() - src_img.height()) / 2), wheel_img);
    ui->stckdWdgt_mouse_img_overlay->setCurrentIndex((crrnt_page == BUTTONS) * (static_cast<int>(ui->pshBttn_bttns_side->isChecked()) + 1));
    return QPixmap::fromImage(cmbn_img);
}

void MainWindow::draw_mouse_anim_img(QString path_to_img, bool apply_effects) {
    QPixmap pxmp;
    QPalette palette;
    if(apply_effects) {
        pxmp = apply_effects_on_mouse_image();
    } else {
        QImage src_img = QImage(path_to_img).scaled(ui->frm_mouse_img_anim->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        QImage cmbn_img(ui->frm_mouse_img_anim->size(), QImage::Format_ARGB32);
        cmbn_img.fill(Qt::transparent);
        QPainter pntr(&cmbn_img);
        pntr.setCompositionMode(QPainter::CompositionMode_DestinationOver);
        pntr.drawImage(((ui->frm_mouse_img_anim->width() - src_img.width()) / 2), ((ui->frm_mouse_img_anim->height() - src_img.height()) / 2), src_img);
        pxmp = QPixmap::fromImage(cmbn_img);
    }
    palette.setBrush(ui->frm_mouse_img_anim->backgroundRole(), pxmp);
    ui->frm_mouse_img_anim->setPalette(palette);
}

void MainWindow::prepare_data_for_mouse_read_write(QByteArray *arr_out, QByteArray *arr_in, QByteArray header) {
    arr_in->fill('\x00', INPUT_PACKET_SIZE);
    arr_out->clear();
    arr_out->append(header);
    arr_out->append((PACKET_SIZE - arr_out->count()), '\x00');
    write_to_mouse_hid((*arr_out), true, arr_in);
}

void MainWindow::read_mouse_parameters() {
    QVector<devices> devs_lst{TAIL, WHEEL};
    QVector<effects *> devs_effcts_lst{&crrnt_tail_effct, &crrnt_wheel_effct};
    QVector<speed *> devs_speed_lst{&crrnt_tail_spped, &crrnt_wheel_speed};
    QVector<QString *> devs_clrs_lst{&crrnt_tail_clr, &crrnt_wheel_clr};
    QByteArray tmp_in, tmp_out;
    for(uint8_t i = 0; i < devs_lst.count(); i++) {
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
    for(uint8_t i = 0; i < sldrs_lst.count(); i++) {
        sldrs_lst[i]->setValue(tmp_in.mid((3 + i), 1).toHex().toInt(nullptr, 16) * sldrs_lst[i]->singleStep());
        emit sldrs_lst[i]->valueChanged(sldrs_lst[i]->value());
    }
    int crrnt_rfrsh_rate_lvl = abs(tmp_in.mid(8, 1).toHex().toInt(nullptr, 16));
    if(crrnt_rfrsh_rate_lvl > (sldrs_lst.count() - 1)) {
        crrnt_rfrsh_rate_lvl = sldrs_lst.count() - 1;
    }
    emit sldrs_lst[crrnt_rfrsh_rate_lvl]->sliderReleased();
    if((crrnt_rfrsh_rate < 0) || (crrnt_rfrsh_rate >= rfrsh_rate_bttns_lst.count())) {
        crrnt_rfrsh_rate = 0;
    }
    rfrsh_rate_bttns_lst[crrnt_rfrsh_rate]->click();
    pages tmp_crrnt_page = crrnt_page;
    crrnt_page = LIGHTNING;
    ui->pshBttn_lghtnng_head->setChecked(true);
    emit ui->pshBttn_lghtnng_head->toggled(true);
    crrnt_page = tmp_crrnt_page;
    mnl_chng = false;
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
    if(!cur_dev && crrnt_ui_state) {
        crrnt_ui_state = false;
        change_state_of_ui(crrnt_ui_state);
        return -1;
    }
    hid_free_enumeration(cur_dev);
    if(cnt == 2) {
        crrnt_prdct_id = PRODUCT_ID_WIRE;
    }
    QString path;
    for(uint16_t i = 0; i < path_lst.count(); i++) {
        if(prod_id_lst[i] == crrnt_prdct_id) {
            path = path_lst[i];
        }
    }
    hid_device *handle = hid_open_path(path.toLatin1().data());
    if(crrnt_ui_state != (crrnt_prdct_id == PRODUCT_ID_WIRE)) {
        crrnt_ui_state = (crrnt_prdct_id == PRODUCT_ID_WIRE);
        change_state_of_ui(crrnt_ui_state);
    }
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
        draw_mouse_anim_img(anim_img_nam, true);
    }
    return write_to_mouse_hid(clr_mod_spd_arr);
}

int MainWindow::bind_mouse_button(uint8_t mouse_button, uint8_t mouse_key_combo, uint8_t mouse_key_modifiers, uint8_t mouse_key) {
    QVector<mouse_buttons> mouse_bttns{LEFT_BUTTON, RIGHT_BUTTON, SCROLL_BUTTON, M5_BUTTON, M4_BUTTON, RISE_DPI_BUTTON, LOWER_DPI_BUTTON, AIMING_BUTTON};
    QVector<mouse_key_combos> mouse_key_cmbs{LEFT_CLICK, RIGHT_CLICK, MIDDLE_CLICK, MOVE_BACK, MOVE_FORWARD, RISE_DPI, LOWER_DPI, TURN_DPI, VOLUME_UP, VOLUME_DOWN, SILENT_MODE, ALT_F4,
                                             CTRL_SHIFT_TAB, CTRL_X, WIN_D, CTRL_TAB, CTRL_C, CTRL_Z, CTRL_Y, CTRL_V, CUSTOM_KEY_COMBO};
    QByteArray bnd_key_arr = "\x4d\xb1";
    bnd_key_arr.append(mouse_bttns[mouse_button]);
    bnd_key_arr.append(mouse_key_cmbs[mouse_key_combo] >> 8);
    bnd_key_arr.append(mouse_key_cmbs[mouse_key_combo]);
    bnd_key_arr.append(mouse_key_modifiers);
    bnd_key_arr.append(mouse_key);
    bnd_key_arr.append((PACKET_SIZE - bnd_key_arr.count()), '\x00');
    return write_to_mouse_hid(bnd_key_arr);
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
    draw_mouse_anim_img(":/images/anim/" + anim_img_nam + tmp + ".png", false);
    crrnt_img += img_cnt_dir;
    if(crrnt_img != img_end_val) {
        anim_timer->setInterval(IMG_ANIM_INTERVAL_MS);
        anim_timer->start();
    } else {
        anim_img_nam = ":/images/anim/" + anim_img_nam + tmp + ".png";
        draw_mouse_anim_img(anim_img_nam, true);
    }
}

//////////////////////////////////////////////////DIALOG RESET SETTINGS///////////////////////////////////////////////////
Dialog_Reset_Settings::Dialog_Reset_Settings(QWidget *parent) : QDialog(parent), ui(new Ui::Dialog_Reset_Settings) {
    ui->setupUi(this);
    this->setWindowFlags(this->windowFlags() | Qt::FramelessWindowHint);
    this->setAttribute(Qt::WA_TranslucentBackground);
    connect(ui->pshBttn_close, &QPushButton::clicked, this, [=]() {
        QDialog::done(QDialog::Rejected);
    });
    connect(ui->pshBttn_ok, &QPushButton::clicked, this, [=]() {
        QDialog::done(QDialog::Accepted);
    });
    connect(ui->pshBttn_cncl, &QPushButton::clicked, this, [=]() {
        QDialog::done(QDialog::Rejected);
    });
}

Dialog_Reset_Settings::~Dialog_Reset_Settings() {
    delete ui;
}
