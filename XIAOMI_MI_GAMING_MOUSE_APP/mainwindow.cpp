#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "ui_dialog_reset_settings.h"
#include "ui_dialog_get_color.h"

#define BYTE                    8
#define ORIGINAL_MAX_COLORS     8
#define MACROS_HEADER_LENGHT    8
#define MACROS_BUTTON_LENGHT    8
#define PART_SIZE               12
#define LBL_ANIM_INTERVAL_MS    17
#define COLOR_BUTTON_SIZE       20
#define IMG_ANIM_INTERVAL_MS    30
#define INPUT_PACKET_SIZE       64
#define KEY_MACRO_LENGHT        64
#define INIT_INTERVAL_MS        1000
#define NO_SLEEP_INTERVAL_MS    /*290000*/30000
#define VENDOR_ID               0x2717
#define PRODUCT_ID_WIRE         0x5009
//#define PRODUCT_ID_WIRELESS     0x500b        //NOT WORKING IN WIRELESS MODE, BUT COMMANDS EXEC SUCCESSFULLY

MainWindow::MainWindow(bool _show_hidden, QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    gen_widg = new general_widget();
    if(is_app_started()) {
        gen_widg->show_message_box("", tr("Application already started"), QMessageBox::Warning, nullptr);
        delete gen_widg;
        delete ui;
        throw std::runtime_error("Process exists");
    }
    show_hidden = _show_hidden;
    this->setWindowFlags(this->windowFlags() | Qt::FramelessWindowHint);
    ui->frm_mouse_img_anim->setAutoFillBackground(true);
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
    connect(ui->pshBttn_close, &QPushButton::released, this, &MainWindow::hide);
    connect(ui->pshBttn_mnmz, &QPushButton::released, this, &MainWindow::showMinimized);
    show_action = new QAction(tr("Show"), this);
    connect(show_action, &QAction::triggered, this, &MainWindow::showNormal);
    startup_action = new QAction(tr("Startup"), this);
    connect(startup_action, &QAction::triggered, this, [=](){});
    exit_action = new QAction(tr("Exit"), this);
    connect(exit_action, &QAction::triggered, qApp, &QCoreApplication::quit);
    tray_icon_menu = new QMenu(this);
    tray_icon_menu->addAction(show_action);
    tray_icon_menu->addAction(startup_action);
    tray_icon_menu->addAction(exit_action);
    tray_icon = new QSystemTrayIcon(this);
    tray_icon->setContextMenu(tray_icon_menu);
    connect(tray_icon, &QSystemTrayIcon::activated, this, [=](QSystemTrayIcon::ActivationReason actvtn_rsn) {
        if(actvtn_rsn == QSystemTrayIcon::Trigger) {
            this->setVisible(!this->isVisible());
        }
    });
    chrctrstc_txt_lst = {tr("Welcome to Xiaomi Gaming Mouse!"),
                         tr("The 6 buttons can be set independently according to the player's needs using\n the software, and Mi Mouse can be made more convenient by setting the combination key."),
                         tr("Mi Mouse uses a foot pad made of teflon material, which is more durable and can greatly\n increase the life of the mouse."),
                         tr("Mi Mouse has two connection modes: wired and 2.4G wireless.\n The transmission speed using the wired connection is the fastest. Swift speed is a key to a prolific gaming\n"
                            "experience. When switching to wireless mode, Mi Mouse can be useful for fun and office work."),
                         tr("Mi Mouse is designed to fit really well in the hand, providing the player with a seamless gaming experience. Coupled with two rubber side skirts, it helps players get a "
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
    tray_icon->setIcon(ico);
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
            anim_1(anim_img_nam, 14, -1, -1);
        } else {
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
            if(button && !button->isChecked() && dynamic_cast<QWidget *>(button->parent())->isEnabled()) {
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
                        anim_2(14, -1, -1);
                    } else if((crrnt_page == LIGHTNING) && ui->pshBttn_lghtnng_tail->isChecked()) {
                        anim_1("trailToStrabismus_0", 14, -1, -1);
                    }
                } else if(prev_page == BUTTONS) {
                    if((crrnt_page == HOME) || (crrnt_page == SPEED)) {
                        anim_2(1, 16, 1);
                    } else if(crrnt_page == LIGHTNING) {
                        if(ui->pshBttn_lghtnng_head->isChecked()) {
                            anim_2(1, 16, 1);
                        } else {
                            anim_3(crrnt_page);
                        }
                    }
                } else if(prev_page == LIGHTNING) {
                    if((crrnt_page == HOME) || (crrnt_page == SPEED)) {
                        if(ui->pshBttn_lghtnng_tail->isChecked()) {
                            anim_1("trailToStrabismus_0", 1, 16, 1);
                        }
                    } else if(crrnt_page == BUTTONS) {
                        if(ui->pshBttn_lghtnng_head->isChecked()) {
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
        ui->pshBttn_lghtnng_tic_tac->setVisible(!ui->pshBttn_lghtnng_head->isChecked());
        ui->pshBttn_lghtnng_switching->setVisible(!ui->pshBttn_lghtnng_head->isChecked());
        ui->pshBttn_lghtnng_rgb->setVisible(!ui->pshBttn_lghtnng_head->isChecked());
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
    QVector<QVector<int>> anim_params_lst{{1, 16, 1}, {14, -1, -1}};
    for(uint8_t i = 0; i < bttns_lst.count(); i++) {
        connect(bttns_lst[i], &QPushButton::toggled, this, [=](bool tggld) {
            if(tggld) {
                if(crrnt_page == BUTTONS) {
                    ui->stckdWdgt_mouse_img_overlay->setCurrentWidget(ui->page_1_1_overlay_empty);
                    QVector<QRadioButton *> mouse_bttns{ui->rdBtn_lft_bttn, ui->rdBttn_rght_bttn, ui->rdBttn_scrll_bttn, ui->rdBttn_m5_bttn,
                                                        ui->rdBttn_m4_bttn, ui->rdBttn_rise_dpi, ui->rdBttn_lwr_dpi, ui->rdBttn_amng_bttn};
                    emit mouse_bttns[get_current_mouse_button()]->toggled(true);
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
    QVector<QRadioButton *> mouse_bttns{ui->rdBtn_lft_bttn, ui->rdBttn_rght_bttn, ui->rdBttn_scrll_bttn, ui->rdBttn_m5_bttn,
                                        ui->rdBttn_m4_bttn, ui->rdBttn_rise_dpi, ui->rdBttn_lwr_dpi, ui->rdBttn_amng_bttn};
    for(uint8_t i = 0; i < mouse_bttns.count(); i++) {
        connect(mouse_bttns[i], &QPushButton::toggled, this, [=](bool tggld) {
            if(tggld) {
                QByteArray tmp_in, tmp_out, header = "\x4d\xb0";
                header.append(get_current_mouse_button());
                prepare_data_for_mouse_read_write(&tmp_out, &tmp_in, header);
                mnl_chng = true;
                uint16_t mode = tmp_in.mid(4, 2).toHex().toUInt(nullptr, 16);
                QAbstractButton *checked = key_fnc_bttns_grp->checkedButton();
                if(checked) {
                    key_fnc_bttns_grp->setExclusive(false);
                    checked->setChecked(false);
                    key_fnc_bttns_grp->setExclusive(true);
                }
                on_pshBttn_clr_cstm_key_cmb_clicked();
                clear_list_widget(ui->lstWdgt_mcrs_evnts_lst);
                QVector<mouse_keys> mouse_keys_lst;
                QVector<QString> keys_lst;
                add_button_keys_to_list(&mouse_keys_lst, &keys_lst);
                QVector<QPushButton *> bttns_pages_lst{ui->pshBttn_bttns_key_fnctns, ui->pshBttn_bttns_key_cmbntns, ui->pshBttn_bttns_key_macros};
                if((mode == MACROS_NO_DELAY) || (mode == MACROS_WITH_DELAY)) {
                    mnl_chng = false;
                    int16_t size = tmp_in.mid(6, 2).toHex().toInt(nullptr, 16);
                    int16_t diff = 0;
                    int16_t pckts_size = 0;
                    uint8_t rqst_size = ceil((static_cast<double>(size) / MACROS_BUTTON_LENGHT) / ((KEY_MACRO_LENGHT - MACROS_HEADER_LENGHT) / MACROS_BUTTON_LENGHT));
                    QVector<QString> key_types{"pressed", "released"};
                    uint8_t pos, key_pckt_size;
                    for(uint8_t i = 0; i < rqst_size; i++) {
                        header = "\x57\xf0\x0d";
                        tmp_in.clear();
                        diff = size - (KEY_MACRO_LENGHT - MACROS_HEADER_LENGHT);
                        if(diff < 0) {
                            diff += (KEY_MACRO_LENGHT - MACROS_HEADER_LENGHT);
                        } else {
                            diff = (KEY_MACRO_LENGHT - MACROS_HEADER_LENGHT);
                        }
                        size -= diff;
                        header.append(diff & 0xFF);
                        header.append(2, '\x00');
                        header.append(pckts_size >> BYTE);
                        header.append(pckts_size & 0xFF);
                        pckts_size += diff;
                        prepare_data_for_mouse_read_write(&tmp_out, &tmp_in, header, KEY_MACRO_LENGHT);
                        key_pckt_size = tmp_in.mid(3, 1).toHex().toUInt(nullptr, 16);
                        if(key_pckt_size < MACROS_BUTTON_LENGHT) {
                            break;
                        }
                        for(uint8_t j = 0; j < (key_pckt_size / (MACROS_BUTTON_LENGHT / 2)); j++) {
                            pos = MACROS_HEADER_LENGHT + ((MACROS_BUTTON_LENGHT / 2) * j);
                            if((mode == MACROS_WITH_DELAY) && (j != 0)) {
                                ui->lstWdgt_mcrs_evnts_lst->addItem(new QListWidgetItem(QIcon(":/images/icons/buttons/icon_key_hold_delay.png"),
                                                                                       (QString::number(tmp_in.mid((pos + 2), 2).toHex().toUInt(nullptr, 16)) + " Milliseconds delay")));
                            }
                            ui->lstWdgt_mcrs_evnts_lst->addItem(new QListWidgetItem(QIcon(":/images/icons/buttons/icon_key_" + key_types[tmp_in.mid((pos + 1), 1).toHex().toUInt(nullptr, 16)] + ".png"),
                                                                                    keys_lst[mouse_keys_lst.indexOf(static_cast<mouse_keys>(tmp_in.mid(pos, 1).toHex().toUInt(nullptr, 16)))]));
                        }
                    }
                } else if(mode == CUSTOM_KEY_COMBO) {
                    QVector<mouse_key_modifiers> mouse_keys_mdfrs_lst{MOD_RIGHT_WIN, MOD_RIGHT_ALT, MOD_RIGHT_SHIFT, MOD_RIGHT_CTRL, MOD_LEFT_WIN, MOD_LEFT_ALT, MOD_LEFT_SHIFT, MOD_LEFT_CTRL};
                    QVector<QString> cmb_names{"R WIN", "R ALT", "R SHIFT", "R CTRL", "L WIN", "L ALT", "L SHIFT", "L CTRL"};
                    uint8_t modifiers = tmp_in.mid(6, 1).toHex().toUInt(nullptr, 16);
                    for(uint8_t j = 0; j < mouse_keys_mdfrs_lst.count(); j++) {
                        if(modifiers >= mouse_keys_mdfrs_lst[j]) {
                            modifiers -= mouse_keys_mdfrs_lst[j];
                            cmb_mdfrs_lst.prepend(cmb_names[j]);
                        }
                    }
                    cmb_key = keys_lst[mouse_keys_lst.indexOf(static_cast<mouse_keys>(tmp_in.mid(7, 1).toHex().toUInt(nullptr, 16)))];
                    form_keys_combination();
                } else {
                    QVector<QRadioButton *> key_fnc_bttns_lst{ui->rdBttn_key_func_lft_clck, ui->rdBttn_key_func_rght_clck, ui->rdBttn_key_func_mddl_clck, ui->rdBttn_key_func_mv_bck,
                                                              ui->rdBttn_key_func_mv_frwrd, ui->rdBttn_key_func_rise_dpi, ui->rdBttn_key_func_lwr_dpi, ui->rdBttn_key_func_trn_dpi,
                                                              ui->rdBttn_key_func_vlm_up, ui->rdBttn_key_func_vlm_dwn, ui->rdBttn_key_func_slnt_mod, ui->rdBttn_key_cmb_cls_wndw,
                                                              ui->rdBttn_key_cmb_prev_tab_in_brwsr, ui->rdBttn_key_cmb_cut, ui->rdBttn_key_cmb_shw_dsktp, ui->rdBttn_key_cmb_nxt_tab_in_brwsr,
                                                              ui->rdBttn_key_cmb_copy, ui->rdBttn_key_cmb_undo, ui->rdBttn_key_cmb_redo, ui->rdBttn_key_cmb_paste};
                    QVector<mouse_key_combos> mouse_key_cmbs{LEFT_CLICK, RIGHT_CLICK, MIDDLE_CLICK, MOVE_BACK, MOVE_FORWARD, RISE_DPI, LOWER_DPI, TURN_DPI, VOLUME_UP, VOLUME_DOWN, SILENT_MODE,
                                                             ALT_F4, CTRL_SHIFT_TAB, CTRL_X, WIN_D, CTRL_TAB, CTRL_C, CTRL_Z, CTRL_Y, CTRL_V};
                    for(uint8_t j = 0; j < mouse_key_cmbs.count(); j++) {
                        if(mode == mouse_key_cmbs[j]) {
                            mode = CUSTOM_KEY_COMBO * (j > mouse_key_cmbs.indexOf(SILENT_MODE));
                            key_fnc_bttns_lst[j]->setChecked(true);
                            break;
                        }
                    }
                }
                bttns_pages_lst[((mode == CUSTOM_KEY_COMBO) + (2 * ((mode == MACROS_NO_DELAY) || (mode == MACROS_WITH_DELAY))))]->click();
                emit bttns_pages_lst[((mode == CUSTOM_KEY_COMBO) + (2 * ((mode == MACROS_NO_DELAY) || (mode == MACROS_WITH_DELAY))))]->clicked();
                mnl_chng = false;
            }
        });
    }
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
        clear_list_widget(ui->lstWdgt_mcrs_evnts_lst, tggld);
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
    clear_list_widget(ui->lstWdgt_mcrs_evnts_lst);
    delete show_action;
    delete startup_action;
    delete exit_action;
    delete tray_icon_menu;
    delete tray_icon;
    delete settings;
    clear_vector(&bttns_wtchrs_lst);
    clear_vector(&chrctrstc_frms_wtchrs_lst);
    clear_vector(&chrctrstc_frms_tmrs_lst);
    delete key_fnc_bttns_grp;
    delete gen_widg;
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event) {
    event->ignore();
    this->hide();
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
        if(ui->rdBttn_dly_btwn_evnts->isChecked() && !pressed_keys_lst.count() && ui->lstWdgt_mcrs_evnts_lst->count()) {
            ui->lstWdgt_mcrs_evnts_lst->addItem(new QListWidgetItem(QIcon(":/images/icons/buttons/icon_key_hold_delay.png"), (QString::number(between_key_timer.elapsed()) + " Milliseconds delay")));
        }
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
                } else if(!pressed_keys_lst.count()) {
                    between_key_timer.restart();
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
        QByteArray dflt_sttgs_arr;
        for(uint8_t i = 0; i < mouse_dvcs.count(); i++) {
            dflt_sttgs_arr.clear();
            dflt_sttgs_arr.append("\x4d\xa1");
            dflt_sttgs_arr.append(mouse_dvcs[i]);
            dflt_sttgs_arr.append(mouse_dflt_effcts[i]);
            dflt_sttgs_arr.append(mouse_dflt_spd[i]);
            dflt_sttgs_arr.append("\x08\x08");
            dflt_sttgs_arr.append(mouse_dflt_clrs[i].red());
            dflt_sttgs_arr.append(mouse_dflt_clrs[i].green());
            dflt_sttgs_arr.append(mouse_dflt_clrs[i].blue());
            dflt_sttgs_arr.append((PACKET_SIZE - dflt_sttgs_arr.count()), '\x00');
            write_to_mouse_hid(dflt_sttgs_arr);
        }
        remove_color_buttons(gen_widg->get_setting(settings, "USER_COLOR/Num").toInt());
        QVector<uint16_t> crrnt_dpi{100, 800, 1600, 2400, 4800};
        QVector<QSlider *> crrnt_dpi_sldrs{ui->hrzntlSldr_rfrsh_rate_lvl_1, ui->hrzntlSldr_rfrsh_rate_lvl_2, ui->hrzntlSldr_rfrsh_rate_lvl_3, ui->hrzntlSldr_rfrsh_rate_lvl_4, ui->hrzntlSldr_rfrsh_rate_lvl_5};
        for(uint8_t i = 0; i < crrnt_dpi_sldrs.count(); i++) {
            crrnt_dpi_sldrs[i]->setValue(crrnt_dpi[i]);
        }
        dflt_sttgs_arr = "\x4d\xc3";
        dflt_sttgs_arr.append(1);
        dflt_sttgs_arr.append((PACKET_SIZE - dflt_sttgs_arr.count()), '\x00');
        write_to_mouse_hid(dflt_sttgs_arr);
        QVector<mouse_buttons> mouse_bttns_lst{LEFT_BUTTON, RIGHT_BUTTON, SCROLL_BUTTON, M5_BUTTON, M4_BUTTON, RISE_DPI_BUTTON, LOWER_DPI_BUTTON, AIMING_BUTTON};
        QVector<mouse_key_combos> dflt_mouse_key_cmbs_lst{LEFT_CLICK, RIGHT_CLICK, MIDDLE_CLICK, MOVE_FORWARD, MOVE_BACK, RISE_DPI, LOWER_DPI, TURN_DPI};
        for(uint8_t i = 0; i < mouse_bttns_lst.count(); i++) {
            dflt_sttgs_arr.clear();
            dflt_sttgs_arr.append("\x4d\xb1");
            dflt_sttgs_arr.append(mouse_bttns_lst[i]);
            dflt_sttgs_arr.append(dflt_mouse_key_cmbs_lst[i] >> BYTE);
            dflt_sttgs_arr.append(dflt_mouse_key_cmbs_lst[i] & 0xFF);
            dflt_sttgs_arr.append((PACKET_SIZE - dflt_sttgs_arr.count()), '\x00');
            write_to_mouse_hid(dflt_sttgs_arr);
        }
        ui->pshBttn_4_speed->setChecked(true);
        emit ui->pshBttn_4_speed->toggled(true);
        ui->pshBttn_speed_rfrsh_rate->setChecked(true);
        emit ui->pshBttn_speed_rfrsh_rate->toggled(true);
        ui->pshBttn_rfrsh_rate_1000->setChecked(true);
        emit ui->pshBttn_rfrsh_rate_1000->toggled(true);
        emit ui->hrzntlSldr_rfrsh_rate_lvl_3->sliderReleased();
        ui->pshBttn_3_lightning->setChecked(true);
        emit ui->pshBttn_3_lightning->toggled(true);
        ui->pshBttn_lghtnng_head->setChecked(true);
        emit ui->pshBttn_lghtnng_head->toggled(true);
        ui->pshBttn_2_buttons->setChecked(true);
        emit ui->pshBttn_2_buttons->toggled(true);
        ui->pshBttn_bttns_top->setChecked(true);
        emit ui->pshBttn_bttns_top->toggled(true);
        ui->rdBttn_scrll_bttn->setChecked(true);
        emit ui->rdBttn_scrll_bttn->toggled(true);
        ui->rdBttn_m5_bttn->setChecked(true);
        emit ui->rdBttn_m5_bttn->toggled(true);
        ui->pshBttn_1_home->setChecked(true);
        emit ui->pshBttn_1_home->toggled(true);
    }
}

void MainWindow::on_pshBttn_add_clr_clicked() {
    QVector<QString> devs_clrs_lst{crrnt_tail_clr, crrnt_wheel_clr};
    Dialog_Get_Color clr_dlg(this, QColor(devs_clrs_lst[ui->pshBttn_lghtnng_head->isChecked()]),
                             ui->pshBttn_add_clr->mapToGlobal(QPoint((ui->pshBttn_add_clr->pos().x() + ui->pshBttn_add_clr->width()), ui->pshBttn_add_clr->pos().y())));
    if(clr_dlg.exec() == QDialog::Accepted) {
        QColor color = clr_dlg.get_selected_color();
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
    ui->lbl_cstm_key_cmb->clear();
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
        QVector<mouse_keys> mouse_keys_lst;
        QVector<QString> keys_lst;
        add_button_keys_to_list(&mouse_keys_lst, &keys_lst);
        QVector<mouse_key_modifiers> mouse_keys_mdfrs_lst{MOD_LEFT_CTRL, MOD_LEFT_SHIFT, MOD_LEFT_ALT, MOD_LEFT_WIN, MOD_RIGHT_CTRL, MOD_RIGHT_SHIFT, MOD_RIGHT_ALT, MOD_RIGHT_WIN};
        QVector<QString> keys_mdfrs_lst{"L CTRL", "L SHIFT", "L ALT", "L WIN", "R CTRL", "R SHIFT", "R ALT", "R WIN"};
        uint8_t key = 0;
        if(cmb_key.count()) {
            key = mouse_keys_lst[keys_lst.indexOf(cmb_key)];
            cmb_keys_lst.removeAt(cmb_keys_lst.count() - 1);
        }
        uint8_t modifiers = 0;
        for(uint8_t i = 0; i < cmb_keys_lst.count(); i++) {
            for(uint8_t j = 0; j < keys_mdfrs_lst.count(); j++) {
                if(cmb_keys_lst[i].compare(keys_mdfrs_lst[j], Qt::CaseInsensitive) == 0) {
                    modifiers += mouse_keys_mdfrs_lst[j];
                    break;
                }
            }
        }
        bind_mouse_button(get_current_mouse_button(), (COMBOS_COUNT - 3), modifiers, key);
    }
}

void MainWindow::on_pshBttn_save_mcrs_clicked() {
    if(ui->lstWdgt_mcrs_evnts_lst->model()->rowCount() != 0) {
        QIcon prssd_icon_cache_icn(":/images/icons/buttons/icon_key_pressed.png");
        QIcon rlsd_icon_cache_icn(":/images/icons/buttons/icon_key_released.png");
        QIcon dly_icon_cache_icn(":/images/icons/buttons/icon_key_hold_delay.png");
        QImage prssd_icon_cache = prssd_icon_cache_icn.pixmap(prssd_icon_cache_icn.availableSizes().at(0)).toImage();
        QImage rlsd_icon_cache = rlsd_icon_cache_icn.pixmap(rlsd_icon_cache_icn.availableSizes().at(0)).toImage();
        QImage dly_icon_cache = dly_icon_cache_icn.pixmap(dly_icon_cache_icn.availableSizes().at(0)).toImage();
        QImage crrnt_icon_cache;
        uint16_t crrnt_dly = 0;
        uint8_t crrnt_bttn_stat = 0;
        uint8_t crrnt_bttn = 0;
        QVector<mouse_keys> mouse_keys_lst;
        QVector<QString> keys_lst;
        add_button_keys_to_list(&mouse_keys_lst, &keys_lst);
        QByteArray mcrs_arr = "\x4d\xb1";
        uint16_t bttns_cnt_size = ceil(static_cast<double>(ui->lstWdgt_mcrs_evnts_lst->model()->rowCount()) / (2 << ui->rdBttn_dly_btwn_evnts->isChecked())) * BYTE;
        QVector<mouse_key_combos> mcrs_typ_lst{MACROS_NO_DELAY, MACROS_WITH_DELAY};
        mcrs_arr.append(get_current_mouse_button());
        mcrs_arr.append(mcrs_typ_lst[ui->rdBttn_dly_btwn_evnts->isChecked()] >> BYTE);
        mcrs_arr.append(mcrs_typ_lst[ui->rdBttn_dly_btwn_evnts->isChecked()] & 0xFF);
        mcrs_arr.append(bttns_cnt_size >> BYTE);
        mcrs_arr.append(bttns_cnt_size & 0xFF);
        mcrs_arr.append((PACKET_SIZE - mcrs_arr.count()), '\x00');
        write_to_mouse_hid(mcrs_arr);
        mcrs_arr.clear();
        uint16_t prev_pckt_size = 0;
        uint8_t crrnt_pckt_size = 0;
        for(uint16_t i = 0; i < ui->lstWdgt_mcrs_evnts_lst->model()->rowCount(); i++) {
            crrnt_icon_cache = ui->lstWdgt_mcrs_evnts_lst->item(i)->icon().pixmap(ui->lstWdgt_mcrs_evnts_lst->item(i)->icon().availableSizes().at(0)).toImage();
            if(crrnt_icon_cache == dly_icon_cache) {
                crrnt_dly = ui->lstWdgt_mcrs_evnts_lst->item(i)->text().split(" ").first().toUInt(nullptr, 10);
            } else if((crrnt_icon_cache == rlsd_icon_cache) || (crrnt_icon_cache == prssd_icon_cache)) {
                crrnt_bttn_stat = static_cast<uint8_t>(crrnt_icon_cache == rlsd_icon_cache);
                crrnt_bttn = mouse_keys_lst[keys_lst.indexOf(ui->lstWdgt_mcrs_evnts_lst->item(i)->text())];
            }
            if(!ui->rdBttn_dly_btwn_evnts->isChecked() || (ui->rdBttn_dly_btwn_evnts->isChecked() && ((i % 2) == 0))) {
                mcrs_arr.append(crrnt_bttn);
                mcrs_arr.append(crrnt_bttn_stat);
                mcrs_arr.append(crrnt_dly >> BYTE);
                mcrs_arr.append(crrnt_dly & 0xFF);
                if((mcrs_arr.count() == (KEY_MACRO_LENGHT - MACROS_HEADER_LENGHT)) || (i == (ui->lstWdgt_mcrs_evnts_lst->model()->rowCount() - 1))) {
                    crrnt_pckt_size = mcrs_arr.count();
                    mcrs_arr.prepend(prev_pckt_size & 0xFF);
                    mcrs_arr.prepend(prev_pckt_size >> BYTE);
                    prev_pckt_size += crrnt_pckt_size;
                    mcrs_arr.prepend(2, '\x00');
                    mcrs_arr.prepend(crrnt_pckt_size);
                    mcrs_arr.prepend("\x57\xf1\x0d", 3);
                    mcrs_arr.append((KEY_MACRO_LENGHT - mcrs_arr.count()), '\x00');
                    write_to_mouse_hid(mcrs_arr);
                    mcrs_arr.clear();
                }
            }
        }
    }
}

bool MainWindow::is_app_started() {
    QProcess tsk_lst_prcss;
#ifdef __linux__
    tsk_lst_prcss.start("pidof \"" + QFileInfo(QCoreApplication::applicationFilePath()).fileName() + "\"");
    tsk_lst_prcss.waitForFinished();
    return (QString(tsk_lst_prcss.readAllStandardOutput()).split(" ").count() > 1);
#elif __WIN32__
    tsk_lst_prcss.start("tasklist", QStringList() << "/NH" << "/FO" << "CSV" << "/FI" << QString("IMAGENAME eq %1").arg(QFileInfo(QCoreApplication::applicationFilePath()).fileName()));
    tsk_lst_prcss.waitForFinished();
    return (QString(tsk_lst_prcss.readAllStandardOutput()).split(QFileInfo(QCoreApplication::applicationFilePath()).fileName()).count() > 2);
#endif
}

void MainWindow::finish_init() {
    read_mouse_parameters();
    anim_img_nam = ":/images/anim/positionToStrabismus_015.png";
    tray_icon->show();
    if(!show_hidden) {
        this->show();
    }
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
                QColor tmp_clr;
                if(crrnt_devs_clr_indxs_lst[ui->pshBttn_lghtnng_head->isChecked()] != -1) {
                    tmp_clr.setNamedColor("#" + clrs_bttns_lst[i]->styleSheet().split("#").last().split(",").first());
                } else {
                    tmp_clr.setNamedColor(devs_clrs_lst[ui->pshBttn_lghtnng_head->isChecked()]);
                }
                Dialog_Get_Color clr_dlg(this, tmp_clr, clrs_bttns_lst[0]->mapToGlobal(QPoint((clrs_bttns_lst[i]->pos().x() + clrs_bttns_lst[i]->width()), (clrs_bttns_lst[i]->pos().y() - 3))));
                if(clr_dlg.exec() == QDialog::Accepted) {
                    QColor color = clr_dlg.get_selected_color();
                    if(color.isValid()) {
                        tmp_clr.setRgb(color.rgb());
                        tmp_clr.setAlpha(128);
                        clrs_bttns_lst[i]->setStyleSheet(gen_widg->get_color_button_stylesheet(color.name(QColor::HexRgb), tmp_clr.name(QColor::HexArgb)));
                        gen_widg->save_setting(settings, "COLOR" + QString::number(i + 1) + "/Red", color.red());
                        gen_widg->save_setting(settings, "COLOR" + QString::number(i + 1) + "/Green", color.green());
                        gen_widg->save_setting(settings, "COLOR" + QString::number(i + 1) + "/Blue", color.blue());
                        if(crrnt_devs_clr_indxs_lst[ui->pshBttn_lghtnng_head->isChecked()] == i) {
                            mouse_set_color_for_device();
                        }
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
    while(clrs_dlt_bttns_lst.count()) {
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

void MainWindow::add_button_keys_to_list(QVector<mouse_keys> *mouse_keys_lst, QVector<QString> *keys_lst) {
    if(mouse_keys_lst) {
        mouse_keys_lst->append({KEY_CTRL_BREAK, KEY_A, KEY_B, KEY_C, KEY_D, KEY_E, KEY_F, KEY_G, KEY_H, KEY_I, KEY_J, KEY_K, KEY_L, KEY_M, KEY_N, KEY_O, KEY_P, KEY_Q, KEY_R, KEY_S, KEY_T, KEY_U, KEY_V,
                                KEY_W, KEY_X, KEY_Y, KEY_Z, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9, KEY_0, KEY_ENTER, KEY_ESCAPE, KEY_BACKSPACE, KEY_TAB, KEY_SPACE, KEY_SUB,
                                KEY_EQUAL, KEY_SQR_BRCKT_OPEN, KEY_SQR_BRCKT_CLOSE, KEY_BACKSLASH, KEY_SEMICOLON, KEY_APOSTROPHE, KEY_BACKTICK, KEY_COMMA, KEY_DOT, KEY_SLASH, KEY_CAPS_LOCK, KEY_F1,
                                KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_F10, KEY_F11, KEY_F12, KEY_PRINT_SCREEN, KEY_SCROLL_LOCK, KEY_PAUSE, KEY_INSERT, KEY_HOME,
                                KEY_PAGE_UP, KEY_DELETE, KEY_END, KEY_PAGE_DOWN, KEY_RIGHT_ARROW, KEY_LEFT_ARROW, KEY_DOWN_ARROW, KEY_UP_ARROW, KEY_NUM_LOCK, KEY_NUMPAD_DIV, KEY_NUMPAD_MULT,
                                KEY_NUMPAD_SUB, KEY_NUMPAD_ADD, KEY_NUMPAD_1, KEY_NUMPAD_2, KEY_NUMPAD_3, KEY_NUMPAD_4, KEY_NUMPAD_5, KEY_NUMPAD_6, KEY_NUMPAD_7, KEY_NUMPAD_8, KEY_NUMPAD_9, KEY_NUMPAD_0,
                                KEY_NUMPAD_DOT, KEY_LESS, KEY_APPS_MENU, KEY_LEFT_CTRL, KEY_LEFT_SHIFT, KEY_LEFT_ALT, KEY_LEFT_WIN, KEY_RIGHT_CTRL, KEY_RIGHT_SHIFT, KEY_RIGHT_ALT, KEY_RIGHT_WIN});
    }
    if(keys_lst) {
        keys_lst->append({"CTRLBREAK", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", "1", "2", "3", "4", "5", "6",
                          "7", "8", "9", "0", "RETURN", "ESC", "BACKSPACE", "TAB", "SPACE", "-", "=", "[", "]", "\\", ";", "'", "`", ",", ".", "/", "CAPSLOCK", "F1", "F2", "F3", "F4", "F5", "F6", "F7",
                          "F8", "F9", "F10", "F11", "F12", "PRINT", "SCROLLLOCK", "PAUSE", "INS", "HOME", "PGUP", "DEL", "END", "PGDOWN", "RIGHT", "LEFT", "DOWN", "UP", "NUMLOCK", "/", "*", "-", "+",
                          "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", ".", "<", "MENU", "L CTRL", "L SHIFT", "L ALT", "L WIN", "R CTRL", "R SHIFT", "R ALT", "R WIN"});
    }
}

template <typename T>
void MainWindow::clear_vector(QVector<T *> *vctr) {
    while(vctr->count()) {
        delete (*vctr)[0];
        vctr->removeAt(0);
    }
}

void MainWindow::clear_list_widget(QListWidget *widg, bool prcssng_flg) {
    while(prcssng_flg && widg->count()) {
        delete widg->takeItem(0);
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
        return key.toUpper();
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
    if(cmb_key.count()) {
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

int MainWindow::prepare_data_for_mouse_read_write(QByteArray *arr_out, QByteArray *arr_in, QByteArray header, uint8_t pckt_size) {
    arr_in->fill('\x00', INPUT_PACKET_SIZE);
    arr_out->clear();
    arr_out->append(header);
    arr_out->append((pckt_size - arr_out->count()), '\x00');
    return write_to_mouse_hid((*arr_out), true, arr_in);
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
    emit rfrsh_rate_bttns_lst[crrnt_rfrsh_rate]->clicked();
    pages tmp_crrnt_page = crrnt_page;
    crrnt_page = LIGHTNING;
    ui->pshBttn_lghtnng_head->setChecked(true);
    emit ui->pshBttn_lghtnng_head->toggled(true);
    crrnt_page = tmp_crrnt_page;
    ui->rdBttn_scrll_bttn->setChecked(true);
    emit ui->rdBttn_scrll_bttn->toggled(true);
    ui->rdBttn_m5_bttn->setChecked(true);
    emit ui->rdBttn_m5_bttn->toggled(true);
    mouse_non_sleep();
    mnl_chng = false;
}

int MainWindow::write_to_mouse_hid(QByteArray &data, bool read, QByteArray *output) {
    if(mnl_chng) {
        return 0;
    }
    int result = -1;
#ifdef __linux__
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
    if(prod_id_lst.count()) {
        hid_device *handle = hid_open_path(path_lst[prod_id_lst.indexOf(crrnt_prdct_id)].toLatin1().data());
        if(crrnt_ui_state != (crrnt_prdct_id == PRODUCT_ID_WIRE)) {
            crrnt_ui_state = (crrnt_prdct_id == PRODUCT_ID_WIRE);
            change_state_of_ui(crrnt_ui_state);
        }
        if(!handle) {
            return -1;
        }
        hid_set_nonblocking(handle, 1);
        result = /*hid_send_feature_report*/hid_write(handle, reinterpret_cast<unsigned char *>(data.data()), data.count());
        if((result == 0) || (result == PACKET_SIZE) || (result == KEY_MACRO_LENGHT)) {
            result = 0;
            if(read) {
                result = hid_read(handle, reinterpret_cast<unsigned char *>(output->data()), output->count());
            }
        } else {
            std::wcout << hid_error(handle) << std::endl;
//            const wchar_t *string = hid_error(handle);
//            qDebug() << "Failure: " << QString::fromWCharArray(string, (sizeof(string) / sizeof(const wchar_t *))) << ";  code:" << result;
        }
        hid_set_nonblocking(handle, 0);
        hid_close(handle);
    }
#elif __WIN32__
    HANDLE DeviceHandle = INVALID_HANDLE_VALUE;
    HDEVINFO DeviceInfoSet;
    GUID InterfaceClassGuid;
    SP_DEVICE_INTERFACE_DATA DeviceInterfaceData;
    PSP_DEVICE_INTERFACE_DETAIL_DATA pDeviceInterfaceDetailData;
    HIDD_ATTRIBUTES Attributes;
    DWORD Result;
    DWORD MemberIndex = 0;
    DWORD Required;
    HidD_GetHidGuid(&InterfaceClassGuid);
    DeviceInfoSet = SetupDiGetClassDevsW(&InterfaceClassGuid, NULL, NULL, (DIGCF_PRESENT | DIGCF_DEVICEINTERFACE));
    if(DeviceInfoSet == INVALID_HANDLE_VALUE) {
        return -1;
    }
    DeviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
    while(SetupDiEnumDeviceInterfaces(DeviceInfoSet, NULL, &InterfaceClassGuid, MemberIndex, &DeviceInterfaceData)) {
        Result = SetupDiGetDeviceInterfaceDetailW(DeviceInfoSet, &DeviceInterfaceData, NULL, 0, &Required, NULL);
        pDeviceInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(Required);
        if(!pDeviceInterfaceDetailData) {
            result = -1;
            break;
        }
        pDeviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
        Result = SetupDiGetDeviceInterfaceDetailW(DeviceInfoSet, &DeviceInterfaceData, pDeviceInterfaceDetailData, Required, NULL, NULL);
        if(!Result) {
            free(pDeviceInterfaceDetailData);
            result = -1;
            break;
        }
        DeviceHandle = CreateFileW(pDeviceInterfaceDetailData->DevicePath, (GENERIC_READ | GENERIC_WRITE), (FILE_SHARE_READ | FILE_SHARE_WRITE), NULL, OPEN_EXISTING, 0, NULL);
        if(DeviceHandle != INVALID_HANDLE_VALUE) {
            Attributes.Size = sizeof(Attributes);
            Result = HidD_GetAttributes(DeviceHandle, &Attributes);
            if(!Result) {
                result = -1;
                CloseHandle(DeviceHandle);
                free(pDeviceInterfaceDetailData);
                break;
            }
            if((Attributes.VendorID == VENDOR_ID) && (Attributes.ProductID == PRODUCT_ID_WIRE) && (((data.count() == PACKET_SIZE) && wcsstr(pDeviceInterfaceDetailData->DevicePath, L"&col03")) ||
                                                                                                   ((data.count() == KEY_MACRO_LENGHT) && wcsstr(pDeviceInterfaceDetailData->DevicePath, L"&col02")))) {
                result = HidD_SetOutputReport(DeviceHandle, reinterpret_cast<unsigned char *>(data.data()), data.count());
                if(result && read) {
                    OVERLAPPED overlapped;
                    overlapped.Offset = 0;
                    overlapped.OffsetHigh = 0;
                    overlapped.hEvent = 0;
                    DWORD bytes;
                    result = ReadFile(DeviceHandle, reinterpret_cast<unsigned char *>(output->data()), output->count(), &bytes, &overlapped);
//                    if(done == FALSE) {
//                        DWORD error = GetLastError();
//                        if (error != ERROR_IO_PENDING) {
//                            //dprintf(D_ALWAYS, "ReadFileError: %u\n", error);
//                            return -1;
//                        }
//                        if (GetOverlappedResult(DeviceHandle, &overlapped, &bytes, TRUE) == FALSE) {
//                            //dprintf(D_ALWAYS, "GetOverlappedResult error: %u\n", GetLastError());
//                            return -1;
//                        }
//                    }
                    if(!result) {
                        qDebug() << GetLastError();
                    }
                }
                CloseHandle(DeviceHandle);
                free(pDeviceInterfaceDetailData);
                DeviceHandle = NULL;
                break;
            } else {
                CloseHandle(DeviceHandle);
            }
        }
        MemberIndex++;
        free(pDeviceInterfaceDetailData);
    }
    SetupDiDestroyDeviceInfoList(DeviceInfoSet);
#endif
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
    if(mouse_key_cmbs[mouse_key_combo] != CUSTOM_KEY_COMBO) {
        on_pshBttn_clr_cstm_key_cmb_clicked();
    }
    QByteArray bnd_key_arr = "\x4d\xb1";
    bnd_key_arr.append(mouse_bttns[mouse_button]);
    bnd_key_arr.append(mouse_key_cmbs[mouse_key_combo] >> BYTE);
    bnd_key_arr.append(mouse_key_cmbs[mouse_key_combo] & 0xFF);
    bnd_key_arr.append(mouse_key_modifiers);
    bnd_key_arr.append(mouse_key);
    bnd_key_arr.append((PACKET_SIZE - bnd_key_arr.count()), '\x00');
    return write_to_mouse_hid(bnd_key_arr);
}

int MainWindow::mouse_non_sleep() {
    QByteArray tmp_in, tmp_out, chrg_header = "\x4d\x90";
    int res = prepare_data_for_mouse_read_write(&tmp_out, &tmp_in, chrg_header);
    if(tmp_in.mid((chrg_header.count() - 1), 1).compare(chrg_header.mid((chrg_header.count() - 1), 1), Qt::CaseInsensitive) == 0) {
        ui->lbl_chrg_lvl->setText(tr("Charge level ") + QString::number(tmp_in.mid(3, 1).toHex().toInt(nullptr, 16)) + "%");
    }
    return res;
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

////////////////////////////////////////////////////DIALOG GET COLOR//////////////////////////////////////////////////////
Dialog_Get_Color::Dialog_Get_Color(QWidget *parent, QColor _crrnt_clr, QPoint pos) : QDialog(parent), ui(new Ui::Dialog_Get_Color) {
    ui->setupUi(this);
    this->setWindowFlags(this->windowFlags() | Qt::FramelessWindowHint);
    this->setAttribute(Qt::WA_TranslucentBackground);
    ui->frm_hsv_clr->setAutoFillBackground(true);
    ui->frm_clr_slctr->setAutoFillBackground(true);
    QVector<QSpinBox *> clrs_spn_bxs_lst{ui->spnBx_clr_red, ui->spnBx_clr_green, ui->spnBx_clr_blue};
    for(uint8_t i = 0; i < clrs_spn_bxs_lst.count(); i++) {
        connect(clrs_spn_bxs_lst[i], QOverload<int>::of(&QSpinBox::valueChanged), this, [=]() {
            select_color_on_hsv_disk();
        });
    }
    connect(ui->pshBttn_close, &QPushButton::clicked, this, [=]() {
        QDialog::done(QDialog::Rejected);
    });
    connect(ui->pshBttn_accp_clr, &QPushButton::clicked, this, [=]() {
        QDialog::done(QDialog::Accepted);
    });
    QColor crrnt_clr(Qt::white);
    if(_crrnt_clr.isValid()) {
        crrnt_clr = _crrnt_clr;
    }
    if((pos.x() != -1) && (pos.y() != -1)) {
        this->move(pos.x(), (pos.y() - 48));
    }
    set_current_color(crrnt_clr);
    ui->hrzntlSldr_clr_brghtnss->setValue(crrnt_clr.toHsv().value());
}

Dialog_Get_Color::~Dialog_Get_Color() {
    delete ui;
}

void Dialog_Get_Color::showEvent(QShowEvent *) {
    if(is_frst_show) {
        is_frst_show = false;
        resizeEvent(nullptr);
    }
}

void Dialog_Get_Color::resizeEvent(QResizeEvent *) {
    if(!this->isHidden()) {
        QImage cmbn_img(ui->frm_hsv_clr->size(), QImage::Format_ARGB32);
        cmbn_img.fill(Qt::transparent);
        QPainter painter(&cmbn_img);
        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::HighQualityAntialiasing);
        uint16_t diameter = std::min(cmbn_img.width(), cmbn_img.height()) - 20;
        QPointF center((cmbn_img.width() / 2.0), (cmbn_img.height() / 2.0));
        QRectF rect((center.x() - (diameter / 2.0)), (center.y() - (diameter / 2.0)), diameter, diameter);
        QRadialGradient gradient(center, (diameter / 2.0));
        gradient.setColorAt(0, QColor(Qt::white));
        for(uint16_t angle = 0; angle < 360; angle++) {
            gradient.setColorAt(1, QColor::fromHsv(angle, 255, 255));
            QBrush brush(gradient);
            painter.setPen(QPen(brush, 1.0));
            painter.setBrush(brush);
            painter.drawPie(rect, (angle * 16), 16);
        }
        QPalette palette;
        palette.setBrush(ui->frm_hsv_clr->backgroundRole(), cmbn_img);
        ui->frm_hsv_clr->setPalette(palette);
        select_color_on_hsv_disk();
    }
}

void Dialog_Get_Color::mousePressEvent(QMouseEvent *event) {
    change_color(event);
}

void Dialog_Get_Color::mouseMoveEvent(QMouseEvent *event) {
    change_color(event);
}

void Dialog_Get_Color::mouseReleaseEvent(QMouseEvent *) {
    qApp->setOverrideCursor(QCursor(Qt::ArrowCursor));
    this->setCursor(Qt::ArrowCursor);
}

QColor Dialog_Get_Color::get_selected_color() {
    QColor clr(ui->spnBx_clr_red->value(), ui->spnBx_clr_green->value(), ui->spnBx_clr_blue->value());
    clr.setHsv(clr.hue(), clr.saturation(), ui->hrzntlSldr_clr_brghtnss->value());
    return clr;
}

void Dialog_Get_Color::change_color(QMouseEvent *event) {
    if(ui->frm_hsv_clr->contentsRect().contains(event->pos()) && QApplication::mouseButtons().testFlag(Qt::LeftButton)) {
        QImage cmbn_img(ui->frm_hsv_clr->size(), QImage::Format_ARGB32);
        cmbn_img.fill(Qt::transparent);
        QPainter pntr(&cmbn_img);
        pntr.setCompositionMode(QPainter::CompositionMode_SourceOver);
        ui->frm_hsv_clr->render(&pntr, ui->frm_hsv_clr->pos(), ui->frm_hsv_clr->rect(), QWidget::IgnoreMask);
        QColor clr = cmbn_img.pixelColor(ui->frm_hsv_clr->mapFromGlobal(this->mapToGlobal(event->pos())));
        if(clr.alpha()) {
            qApp->setOverrideCursor(QCursor(Qt::BlankCursor));
            this->setCursor(Qt::BlankCursor);
            set_current_color(clr);
        }
    }
}

void Dialog_Get_Color::select_color_on_hsv_disk() {
    QColor clr(ui->spnBx_clr_red->value(), ui->spnBx_clr_green->value(), ui->spnBx_clr_blue->value());
    int16_t angle = abs(((clr.hue() + 270) % 360) - 360) % 360;
    uint16_t diameter = std::min(ui->frm_hsv_clr->width(), ui->frm_hsv_clr->height()) - 20;
    double radius_part = (diameter / 2.0) * (clr.saturation() / 255.0);
    QPoint pnt((round(radius_part * sin(angle * (M_PI / 180.0))) + (ui->frm_hsv_clr->width() / 2.0)), abs(round(radius_part * cos(angle * (M_PI / 180.0))) - (ui->frm_hsv_clr->height() / 2.0)));
    QPalette palette;
    QImage src_img(":/images/icons/lightning/color_dialog/icon_select_color.png");
    QImage cmbn_img(ui->frm_clr_slctr->size(), QImage::Format_ARGB32);
    cmbn_img.fill(Qt::transparent);
    QPainter pntr(&cmbn_img);
    pntr.setCompositionMode(QPainter::CompositionMode_DestinationOver);
    pntr.drawImage((pnt.x() - (src_img.width() / 2)), (pnt.y() - (src_img.height() / 2)), src_img);
    palette.setBrush(ui->frm_clr_slctr->backgroundRole(), cmbn_img);
    ui->frm_clr_slctr->setPalette(palette);
}

void Dialog_Get_Color::set_current_color(QColor &clr) {
    ui->spnBx_clr_red->setValue(clr.red());
    ui->spnBx_clr_green->setValue(clr.green());
    ui->spnBx_clr_blue->setValue(clr.blue());
}
