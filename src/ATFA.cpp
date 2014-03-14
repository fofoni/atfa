/*
 * Universidade Federal do Rio de Janeiro
 * Escola Politécnica
 * Projeto Final de Graduação
 * Ambiente de Teste para Filtros Adaptativos
 * Pedro Angelo Medeiros Fonini
 * Orientador: Markus Lima
 */

#include <sstream>

#include <QtGui>

#include "ATFA.h"
#include "Stream.h"

ATFA::ATFA(QWidget *parent) :
    QMainWindow(parent), scene(), rir_source(NoRIR),
    rir_filetype(None), rir_file("")
{

    /*
     * ACTIONS
     *
     */

    // new
    newscene_act = new QAction(QIcon::fromTheme("document-new"),
                               "&New Scenario", this);
    newscene_act->setShortcuts(QKeySequence::New);
    newscene_act->setStatusTip("Setup a new scenario.");
    connect(newscene_act, SIGNAL(triggered()), this, SLOT(newscene()));

    // open
    open_act = new QAction(QIcon::fromTheme("document-open"),
                           "&Open", this);
    open_act->setShortcuts(QKeySequence::Open);
    open_act->setStatusTip("Open saved scenario setup.");
    connect(open_act, SIGNAL(triggered()), this, SLOT(open()));

    // save
    save_act = new QAction(QIcon::fromTheme("document-save"),
                           "&Save", this);
    save_act->setShortcuts(QKeySequence::Save);
    save_act->setStatusTip("Save scenario setup.");
    connect(save_act, SIGNAL(triggered()), this, SLOT(save()));

    // save as
    save_as_act = new QAction(QIcon::fromTheme("document-save-as"),
                              "Save &As", this);
    save_as_act->setShortcuts(QKeySequence::SaveAs);
    save_as_act->setStatusTip(
      "Save scenario setup in a new file, without overwriting the existing one."
    );
    connect(save_as_act, SIGNAL(triggered()), this, SLOT(save_as()));

    // quit
    quit_act = new QAction(QIcon::fromTheme("application-exit"),
                           "Quit", this);
    quit_act->setShortcuts(QKeySequence::Quit);
    connect(quit_act, SIGNAL(triggered()), this, SLOT(quit()));

    // help
    show_help_act = new QAction(QIcon::fromTheme("help-contents"),
                                "&Manual", this);
    show_help_act->setShortcuts(QKeySequence::HelpContents);
    connect(show_help_act, SIGNAL(triggered()), this, SLOT(show_help()));

    // about
    about_atfa_act = new QAction(QIcon::fromTheme("help-about"),
                                 "&About ATFA", this);
    connect(about_atfa_act, SIGNAL(triggered()), this, SLOT(about_atfa()));

    // about qt
    about_qt_act = new QAction("About &Qt", this);
    connect(about_qt_act, SIGNAL(triggered()), this, SLOT(about_qt()));

    /*
     * MENU BAR
     *
     */

    file_menu = menuBar()->addMenu("&File");
    file_menu->addAction(newscene_act);
    file_menu->addAction(open_act);
    file_menu->addAction(save_act);
    file_menu->addAction(save_as_act);
    file_menu->addSeparator();
    file_menu->addAction(quit_act);

    help_menu = menuBar()->addMenu("&Help");
    help_menu->addAction(show_help_act);
    help_menu->addSeparator();
    help_menu->addAction(about_atfa_act);
    help_menu->addAction(about_qt_act);

    /*
     * TOOL BAR
     *
     */

    toolbar = addToolBar("Toolbar");
    toolbar->addAction(newscene_act);
    toolbar->addAction(open_act);
    toolbar->addAction(save_act);
    toolbar->addAction(save_as_act);
    toolbar->addSeparator();
    toolbar->addAction(show_help_act);
    toolbar->setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea |
                             Qt::LeftToolBarArea);

    /*
     * STATUS BAR
     *
     */

    statusBar()->showMessage(QString("ATFA"));

    /*
     * MAIN VIEW
     *
     */

    setWindowTitle("ATFA");

    QHBoxLayout *layout = new QHBoxLayout;

    QVBoxLayout *left_layout = new QVBoxLayout;

        vad_checkbox = new QCheckBox("VAD", this);
        vad_checkbox->setChecked(true);
        left_layout->addWidget(vad_checkbox);

        flearn_group = new QGroupBox("Filter learning", this);
        QVBoxLayout *flearn_layout = new QVBoxLayout;
            flearn_on_radio = new QRadioButton("Enabled (always)", this);
            flearn_off_radio = new QRadioButton("Disabled (always)", this);
            flearn_vad_radio = new QRadioButton("Enabled when VAD is active",
                                                this);
            flearn_vad_radio->setChecked(true);
            flearn_layout->addWidget(flearn_on_radio);
            flearn_layout->addWidget(flearn_off_radio);
            flearn_layout->addWidget(flearn_vad_radio);
            flearn_layout->addStretch(1); // TODO: do we need this?
        flearn_group->setLayout(flearn_layout);
        left_layout->addWidget(flearn_group);

        fout_group = new QGroupBox("Filter output", this);
        QVBoxLayout *fout_layout = new QVBoxLayout;
            fout_on_radio = new QRadioButton("Enabled (always)", this);
            fout_off_radio = new QRadioButton("Disabled (always)", this);
            fout_vad_radio = new QRadioButton("Enabled when VAD is active",
                                              this);
            fout_vad_radio->setChecked(true);
            fout_layout->addWidget(fout_on_radio);
            fout_layout->addWidget(fout_off_radio);
            fout_layout->addWidget(fout_vad_radio);
            fout_layout->addStretch(1); // TODO: do we need this?
        fout_group->setLayout(fout_layout);
        left_layout->addWidget(fout_group);

    layout->addLayout(left_layout);

    QVBoxLayout *right_layout = new QVBoxLayout;

        play_button = new QPushButton(this);
        play_button->setMinimumHeight(80);
        play_button->setIcon(QIcon(QPixmap("../../imgs/play2.png")));
        play_button->setIconSize(QSize(58, 58));
        play_button->setDisabled(true);
        right_layout->addWidget(play_button);

        delay_widget = new QWidget(this);
        QHBoxLayout *delay_layout = new QHBoxLayout;

            delay_label = new QLabel("Round trip delay:", this);
            delay_layout->addWidget(delay_label);

            delay_slider = new QSlider(Qt::Horizontal, this);
            delay_slider->setMinimumWidth(200);
            delay_slider->setMinimum(0);
            delay_slider->setMaximum(500);
            delay_slider->setValue(100);
            // TODO: if playback lags during sliding, disable tracking
            delay_layout->addWidget(delay_slider);

            delay_spin = new QSpinBox(this);
            delay_spin->setMinimum(0);
            delay_spin->setMaximum(500);
            delay_spin->setFixedWidth(60);
            delay_spin->setValue(100);
            delay_layout->addWidget(delay_spin);

            delay_units = new QLabel("ms", this);
            delay_layout->addWidget(delay_units);

        delay_widget->setLayout(delay_layout);
        right_layout->addWidget(delay_widget);

        zero_button = new QPushButton(this);
        zero_button->setText("Reset filter state");
        right_layout->addWidget(zero_button);

        vol_widget = new QWidget(this);
        QHBoxLayout *vol_layout = new QHBoxLayout;

            vol_label = new QLabel("Volume:", this);
            vol_layout->addWidget(vol_label);

            vol_mute_button = new QPushButton("Mute", this);
            vol_mute_button->setCheckable(true);
            vol_layout->addWidget(vol_mute_button);

            vol_slider = new QSlider(Qt::Horizontal, this);
            vol_slider->setMinimumWidth(200);
            vol_slider->setMinimum(0);
            vol_slider->setMaximum(100);
            vol_slider->setValue(50);
            // TODO: if playback lags during sliding, disable tracking
            vol_layout->addWidget(vol_slider);

            vol_spin = new QSpinBox(this);
            vol_spin->setMinimum(0);
            vol_spin->setMaximum(100);
            vol_spin->setFixedWidth(60);
            vol_spin->setValue(50);
            vol_layout->addWidget(vol_spin);

        vol_widget->setLayout(vol_layout);
        right_layout->addWidget(vol_widget);

        rir_widget = new QWidget(this);
        QHBoxLayout *rir_layout = new QHBoxLayout;

            rir_label = new QLabel("Room impulse response:", this);
            rir_layout->addWidget(rir_label);

            rir_type_label = new QLabel("None", this);
            rir_layout->addWidget(rir_type_label);

            rir_show_button = new QPushButton("Show filter coefficients", this);
            rir_layout->addWidget(rir_show_button);

            rir_change_button = new QPushButton("Change", this);
            rir_layout->addWidget(rir_change_button);

        rir_widget->setLayout(rir_layout);
        right_layout->addWidget(rir_widget);

        adapf_widget = new QWidget(this);
        QHBoxLayout *adapf_layout = new QHBoxLayout;

            adapf_label = new QLabel("Adaptative filtering algorithm:", this);
            adapf_layout->addWidget(adapf_label);

            adapf_file_label = new QLabel("None", this);
            adapf_layout->addWidget(adapf_file_label);

            adapf_show_button = new QPushButton("Show code", this);
            adapf_layout->addWidget(adapf_show_button);

            adapf_change_button = new QPushButton("Change", this);
            adapf_layout->addWidget(adapf_change_button);

        adapf_widget->setLayout(adapf_layout);
        right_layout->addWidget(adapf_widget);

    layout->addLayout(right_layout);

    main_widget = new QWidget(this);
    main_widget->setLayout(layout);


    /*
     * SIGNALING
     *
     */

    connect(vad_checkbox, SIGNAL(stateChanged(int)),
            this, SLOT(vad_checkbox_changed(int)));

    setCentralWidget(main_widget);

}

void ATFA::newscene() {



}

void ATFA::open() {
    QMessageBox msg_box;
    msg_box.setText("Not implemented yet");
    msg_box.setWindowTitle("ATFA [info]");
    msg_box.setIcon(QMessageBox::Information);
    msg_box.exec();
}

void ATFA::save() {
    QMessageBox msg_box;
    msg_box.setText("Not implemented yet");
    msg_box.setWindowTitle("ATFA [info]");
    msg_box.setIcon(QMessageBox::Information);
    msg_box.exec();
}

void ATFA::save_as() {
    QMessageBox msg_box;
    msg_box.setText("Not implemented yet");
    msg_box.setWindowTitle("ATFA [info]");
    msg_box.setIcon(QMessageBox::Information);
    msg_box.exec();
}

void ATFA::quit() {
    qApp->quit();
}

void ATFA::show_help() {
    QMessageBox msg_box;
    msg_box.setText("Not implemented yet");
    msg_box.setWindowTitle("ATFA [info]");
    msg_box.setIcon(QMessageBox::Information);
    msg_box.exec();
}

void ATFA::about_atfa() {
    QMessageBox msg_box;
    msg_box.setText("Not implemented yet");
    msg_box.setWindowTitle("ATFA [info]");
    msg_box.setIcon(QMessageBox::Information);
    msg_box.exec();
}

void ATFA::about_qt() {
    QMessageBox::aboutQt(this);
}

void ATFA::vad_checkbox_changed(int state) {
    switch (state) {
    case Qt::Checked:
        scene.is_VAD_active = true;
        statusBar()->showMessage(QString("VAD is now present"));
        break;
    case Qt::Unchecked:
        scene.is_VAD_active = false;
        statusBar()->showMessage(QString("VAD is now turned off"));
        break;
    }
}

void ATFA::flearn_on_toggled(bool t) {

}
