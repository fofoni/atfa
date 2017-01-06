/*
 * Universidade Federal do Rio de Janeiro
 * Escola Politécnica
 * Projeto Final de Graduação
 * Ambiente de Teste para Filtros Adaptativos
 * Pedro Angelo Medeiros Fonini
 * Orientador: Markus Lima
 */

#include <sstream>

// TODO: fazer alguma coisa a respeito das imagens de play/pause/etc.
//       (quando a gente roda o atfa de outro lugar que não seja o
//        build/release/, o qt não acha os arquivos)

/* TODO: fazer adapfs parametrizáveis. Isto é, o adapf.so deve poder
 *       especificar uma lista de parâmetros modificáveis (e.g., mu e
 *       Delta para o NLMS), e a interface gráfica deve deixar o usuário
 *       modificar esses parâmetros. O adapf.so deve especificar, para
 *       cada parâmetro coisas como: tipo (int, float, enum, etc), faixa
 *       permitida, nome, valor default, etc, etc
 */

// TODO: quando a RIR selecionada for o "None", o botão de "show rir
//       coefficients" tem que estar HABILITADO, e mostrar "rir = [ 1 ]".

extern "C" {
# include <portaudio.h>
}

#include <QtGui>
#include <QRegExp>

#include "ATFA.h"
#include "Stream.h"
#include "dialogs/ShowTextDialog.h"
#include "dialogs/ChangeAlgorithmDialog.h"
#include "dialogs/ChangeRIRDialog.h"

ATFA::ATFA(QWidget *parent) :
    QMainWindow(parent), stream(), pastream(NULL),
    rir_source(NoRIR), rir_filetype(None), rir_file(""), database_index(-1),
    adapf_is_dummy(true), adapf_file(""), muted(false)
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
                           "&Quit", this);
    quit_act->setShortcuts(QKeySequence::Quit);
    connect(quit_act, SIGNAL(triggered()), this, SLOT(quit()));

    // help
    show_help_act = new QAction(QIcon::fromTheme("help-contents"),
                                "&Manual", this);
    show_help_act->setShortcuts(QKeySequence::HelpContents);
    connect(show_help_act, SIGNAL(triggered()), this, SLOT(show_help()));

    // about
    about_atfa_act = new QAction(QIcon::fromTheme("help-about"),
                                 "A&bout ATFA", this);
    connect(about_atfa_act, SIGNAL(triggered()), this, SLOT(about_atfa()));

    // about qt
    about_qt_act = new QAction("Abo&ut Qt", this);
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

    main_widget = new QWidget(this);
    QHBoxLayout *layout = new QHBoxLayout;

    QVBoxLayout *left_layout = new QVBoxLayout;

        /* TODO: esse QGroupBox devia ser um QButtonGroup. Ver:
         * http://stackoverflow.com/questions/25280146/do-i-need-to-check-one-by-one-to-know-which-radiobutton-is-checked-in-a-group-in
         * http://stackoverflow.com/questions/33809177/get-index-of-checked-radio-button-in-group
         * http://stackoverflow.com/questions/26319977/how-to-find-out-which-radio-button-chosen-by-the-user
         */
        flearn_group = new QGroupBox("Filter &learning", main_widget);
        QVBoxLayout *flearn_layout = new QVBoxLayout;
            flearn_on_radio = new QRadioButton(
                "&Enabled (always)", flearn_group);
            flearn_off_radio = new QRadioButton(
                "&Disabled (always)", flearn_group);
            flearn_vad_radio = new QRadioButton(
                "Enabled when &VAD detects action", flearn_group);
            flearn_vad_radio->setChecked(true);
            flearn_layout->addWidget(flearn_on_radio);
            flearn_layout->addWidget(flearn_off_radio);
            flearn_layout->addWidget(flearn_vad_radio);
            flearn_layout->addStretch(1); // TODO: do we need this?
        flearn_group->setLayout(flearn_layout);
        left_layout->addWidget(flearn_group);

        zero_button = new QPushButton("&Reset filter state", main_widget);
        left_layout->addWidget(zero_button);

        fout_group = new QGroupBox("Filter ou&tput", main_widget);
        QVBoxLayout *fout_layout = new QVBoxLayout;
            fout_on_radio = new QRadioButton("Enabled (al&ways)", fout_group);
            fout_off_radio = new QRadioButton("Disabled (alwa&ys)", fout_group);
            fout_vad_radio = new QRadioButton(
                "Enabled w&hen VAD detects action", fout_group);
            fout_vad_radio->setChecked(true);
            fout_layout->addWidget(fout_on_radio);
            fout_layout->addWidget(fout_off_radio);
            fout_layout->addWidget(fout_vad_radio);
            fout_layout->addStretch(1); // TODO: do we need this?
        fout_group->setLayout(fout_layout);
        left_layout->addWidget(fout_group);

    layout->addLayout(left_layout);

    QVBoxLayout *right_layout = new QVBoxLayout;

        vad_indicator_widget = new QWidget(main_widget);
        QHBoxLayout *vad_indicator_layout = new QHBoxLayout;

            vad_indicator_label = new QLabel("Voice Activity:",
                                             vad_indicator_widget);
            vad_indicator_layout->addWidget(vad_indicator_label);

            vad_indicator_led = new LEDIndicatorWidget(vad_indicator_widget);
            vad_indicator_layout->addWidget(vad_indicator_led);
            stream.setLED(vad_indicator_led);

            vad_algorithm_combo = new QComboBox;
            vad_algorithm_combo->addItem("Hard");
            vad_algorithm_combo->addItem("Soft");
            vad_indicator_layout->addWidget(vad_algorithm_combo);

        vad_indicator_widget->setLayout(vad_indicator_layout);
        right_layout->addWidget(vad_indicator_widget);

        play_button = new QPushButton(main_widget);
        play_button->setMinimumHeight(80);
        play_button->setIcon(QIcon(QPixmap("../../imgs/play.png")));
        play_button->setIconSize(QSize(58, 58));
        right_layout->addWidget(play_button);

        delay_widget = new QWidget(main_widget);
        QHBoxLayout *delay_layout = new QHBoxLayout;

            delay_label = new QLabel("Round trip delay:", delay_widget);
            delay_layout->addWidget(delay_label);

            delay_slider = new QSlider(Qt::Horizontal, delay_widget);
            delay_slider->setMinimumWidth(200);
            delay_slider->setMinimum(0);
            delay_slider->setMaximum(500);
            delay_slider->setValue(30);
            // TODO: if playback lags during sliding, disable tracking
            delay_layout->addWidget(delay_slider);
            stream.set_delay(delay_slider->value());

            delay_spin = new QSpinBox(delay_widget);
            delay_spin->setMinimum(0);
            delay_spin->setMaximum(500);
            delay_spin->setFixedWidth(60);
            delay_spin->setValue(30);
            delay_layout->addWidget(delay_spin);

            delay_units = new QLabel("ms", delay_widget);
            delay_layout->addWidget(delay_units);

        delay_widget->setLayout(delay_layout);
        right_layout->addWidget(delay_widget);

        vol_widget = new QWidget(main_widget);
        QHBoxLayout *vol_layout = new QHBoxLayout;

            vol_label = new QLabel("Volume:", vol_widget);
            vol_layout->addWidget(vol_label);

            vol_mute_button = new QPushButton("Mute", vol_widget);
            vol_mute_button->setCheckable(true);
            vol_layout->addWidget(vol_mute_button);

            vol_slider = new QSlider(Qt::Horizontal, vol_widget);
            vol_slider->setMinimumWidth(200);
            vol_slider->setMinimum(0);
            vol_slider->setMaximum(100);
            vol_slider->setValue(50);
            // TODO: if playback lags during sliding, disable tracking
            vol_layout->addWidget(vol_slider);

            vol_spin = new QSpinBox(vol_widget);
            vol_spin->setMinimum(0);
            vol_spin->setMaximum(100);
            vol_spin->setFixedWidth(60);
            vol_spin->setValue(50);
            vol_layout->addWidget(vol_spin);

        vol_widget->setLayout(vol_layout);
        right_layout->addWidget(vol_widget);

        rir_widget = new QWidget(main_widget);
        QHBoxLayout *rir_layout = new QHBoxLayout;

            rir_label = new QLabel("Room impulse response:", rir_widget);
            rir_layout->addWidget(rir_label);

            rir_type_label = new QLabel("None", rir_widget);
            rir_layout->addWidget(rir_type_label);

            rir_show_button = new QPushButton("Show f&ilter coefficients",
                                              rir_widget);
            rir_show_button->setDisabled(true);
            rir_layout->addWidget(rir_show_button);

            rir_change_button = new QPushButton("Change", rir_widget);
            rir_layout->addWidget(rir_change_button);

        rir_widget->setLayout(rir_layout);
        right_layout->addWidget(rir_widget);

        adapf_widget = new QWidget(main_widget);
        QHBoxLayout *adapf_layout = new QHBoxLayout;

            adapf_label = new QLabel("Adaptative filtering algorithm:",
                                     adapf_widget);
            adapf_layout->addWidget(adapf_label);

            adapf_file_label = new QLabel("None", adapf_widget);
            adapf_layout->addWidget(adapf_file_label);

            adapf_show_button = new QPushButton("Show &code", adapf_widget);
            adapf_show_button->setDisabled(true);
            adapf_layout->addWidget(adapf_show_button);

            adapf_change_button = new QPushButton("Chan&ge", adapf_widget);
            adapf_layout->addWidget(adapf_change_button);

        adapf_widget->setLayout(adapf_layout);
        right_layout->addWidget(adapf_widget);

    layout->addLayout(right_layout);
    main_widget->setLayout(layout);

    /*
     * SIGNALING
     *
     */

    connect(flearn_on_radio, SIGNAL(toggled(bool)),
            this, SLOT(flearn_on_toggled(bool)));
    connect(flearn_off_radio, SIGNAL(toggled(bool)),
            this, SLOT(flearn_off_toggled(bool)));
    connect(flearn_vad_radio, SIGNAL(toggled(bool)),
            this, SLOT(flearn_vad_toggled(bool)));

    connect(zero_button, SIGNAL(clicked()), this, SLOT(zero_filter_clicked()));

    connect(fout_on_radio, SIGNAL(toggled(bool)),
            this, SLOT(fout_on_toggled(bool)));
    connect(fout_off_radio, SIGNAL(toggled(bool)),
            this, SLOT(fout_off_toggled(bool)));
    connect(fout_vad_radio, SIGNAL(toggled(bool)),
            this, SLOT(fout_vad_toggled(bool)));

    // Sintaxe nova -> complicada porque QComboBox::currentIndexChanged é
    //                 um sinal com overload, aí a gente tem que especificar
    //                 qual deles a gente quer.
    connect(vad_algorithm_combo,
            static_cast<void (QComboBox::*)(int)>(
                &QComboBox::currentIndexChanged),
            [this](int idx){stream.setVADAlgorithm(idx);});

    connect(play_button, SIGNAL(clicked()), this, SLOT(play_clicked()));

    connect(delay_slider, SIGNAL(valueChanged(int)),
            this, SLOT(delay_changed(int)));
    connect(delay_slider, SIGNAL(valueChanged(int)),
            delay_spin, SLOT(setValue(int)));
    connect(delay_spin, SIGNAL(valueChanged(int)),
            delay_slider, SLOT(setValue(int)));

    connect(vol_mute_button, SIGNAL(toggled(bool)),
            this, SLOT(vol_mute_toggled(bool)));

    connect(vol_slider, SIGNAL(valueChanged(int)),
            this, SLOT(vol_changed(int)));
    connect(vol_slider, SIGNAL(valueChanged(int)),
            vol_spin, SLOT(setValue(int)));
    connect(vol_spin, SIGNAL(valueChanged(int)),
            vol_slider, SLOT(setValue(int)));

    connect(rir_change_button, SIGNAL(clicked()), this, SLOT(change_rir()));
    connect(rir_show_button, SIGNAL(clicked()), this, SLOT(show_rir()));

    connect(adapf_change_button, SIGNAL(clicked()), this, SLOT(change_adapf()));
    connect(adapf_show_button, SIGNAL(clicked()), this, SLOT(show_adapf()));

    /*
     * SHOW ON SCREEN
     *
     */

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

void ATFA::flearn_on_toggled(bool t) {
    if (!t) return;
    stream.scene.filter_learning = Stream::Scenario::On;
    statusBar()->showMessage("Filter learning is enabled even during silence.");
}
void ATFA::flearn_off_toggled(bool t) {
    if (!t) return;
    stream.scene.filter_learning = Stream::Scenario::Off;
    statusBar()->showMessage("Filter learning is disabled.");
}
void ATFA::flearn_vad_toggled(bool t) {
    if (!t) return;
    stream.scene.filter_learning = Stream::Scenario::VAD;
    statusBar()->showMessage("Filter learning is controlled by the VAD.");
}

void ATFA::zero_filter_clicked() {
    statusBar()->showMessage("Adaptive filter memory zeroed.");
}

void ATFA::fout_on_toggled(bool t) {
    if (!t) return;
    stream.scene.filter_output = Stream::Scenario::On;
    statusBar()->showMessage("Filter output always enabled.");
}
void ATFA::fout_off_toggled(bool t) {
    if (!t) return;
    stream.scene.filter_learning = Stream::Scenario::Off;
    statusBar()->showMessage("Filter output always disabled.");
}
void ATFA::fout_vad_toggled(bool t) {
    if (!t) return;
    stream.scene.filter_learning = Stream::Scenario::VAD;
    statusBar()->showMessage("Filter output is controlled by the VAD.");
}

void ATFA::play_clicked() {
    if (stream.running()) {

        stream.stop(pastream);
        pastream = NULL;

        statusBar()->showMessage("Simulation stopped.");
        play_button->setIcon(QIcon(QPixmap("../../imgs/play.png")));

        rir_change_button->setDisabled(false);
        adapf_change_button->setDisabled(false);

        vad_indicator_led->setLEDStatus(false);

    }
    else {

        rir_change_button->setDisabled(true);
        adapf_change_button->setDisabled(true);

        pastream = stream.echo();

        statusBar()->showMessage("Simulation running...");
        play_button->setIcon(QIcon(QPixmap("../../imgs/pause.png")));

    }
}

void ATFA::delay_changed(int v) {
    stream.set_delay(v);
}

void ATFA::vol_mute_toggled(bool t) {
    if (t) {
        muted = true;
        stream.scene.volume = 0;
        statusBar()->showMessage( stream.running() ?
            "Local speaker muted. Simulation still running." :
            "Local speaker muted."
        );
    }
    else {
        muted = false;
        stream.scene.volume = float(vol_slider->value())/100.0;
        statusBar()->showMessage("Local speaker unmuted.");
    }
}
void ATFA::vol_changed(int v) {
    if (muted) return;
    stream.scene.volume = float(v)/100.0;
}

void ATFA::show_rir() {

    /// TODO: mostrar também a resp ao impulso na frequência!

    std::stringstream imp_resp_html;
    imp_resp_html << "<span style='font-family: monospace'>";
    if (stream.scene.imp_resp.size() > 0) {
        imp_resp_html << "room_impulse_resp = <b>[</b><br /><br />"
                      << stream.scene.imp_resp[0];
        for (Stream::container_t::const_iterator it =
             stream.scene.imp_resp.begin() + 1;
             it != stream.scene.imp_resp.end(); ++it)
            imp_resp_html << ", " << *it;
        imp_resp_html << "<br /><br /><b>]</b><br />";
    }
    else {
        imp_resp_html << "The room impulse response is empty!<br />"
                         "This means there is no echo in the room. "
                         "You will hear nothing.";
    }
    imp_resp_html << "</span>";
    ShowTextDialog *showrir_dialog = new ShowTextDialog(
                "RIR coefficients", imp_resp_html.str().c_str(), this);
    showrir_dialog->exec();

}

void ATFA::change_rir() {

    ChangeRIRDialog *chrir_dialog = new ChangeRIRDialog(this);
    if (!chrir_dialog->run())
        return;

    switch (rir_source) {
    case NoRIR:
        rir_type_label->setText("None");
        rir_show_button->setDisabled(true);
        break;
    case Literal:
        rir_type_label->setText("Literal");
        rir_show_button->setDisabled(false);
        break;
    case Database:
        // rir_type_label->setText( database file-name and database index );
        rir_type_label->setText("NOT IMPLEMENTED YET");
        rir_show_button->setDisabled(true);
        break;
    case File:
        {
            QString small_filename;
            QRegExp rx("^.*/([^/]*)$");
            if (rx.indexIn(rir_file) > -1)
                small_filename = rx.cap(1);
            else
                small_filename = rir_file;
            rir_type_label->setText(small_filename);
        }
        rir_show_button->setDisabled(false);
    }

    statusBar()->showMessage("Room impulse response updated.");

}

void ATFA::show_adapf() {

    std::stringstream adapf_html;
    adapf_html << "<span style='font-family: monospace'>";
    adapf_html << "NOT IMPLEMENTED YET";
    adapf_html << "</span>";
    ShowTextDialog *showrir_dialog = new ShowTextDialog(
                "Adaptative filtering code", adapf_html.str().c_str(), this);
    showrir_dialog->exec();

}


void ATFA::change_adapf() {

    ChangeAlgorithmDialog *chapf_dialog = new ChangeAlgorithmDialog(this);
    if (!chapf_dialog->run())
        return;

    if (adapf_is_dummy) {
        adapf_file_label->setText("None");
        adapf_show_button->setDisabled(true);
    }
    else {
        QString small_filename;
        QRegExp rx("^.*/([^/]*)$");
        if (rx.indexIn(adapf_file) > -1)
            small_filename = rx.cap(1);
        else
            small_filename = adapf_file;
        adapf_file_label->setText(small_filename);
        adapf_show_button->setDisabled(false);
    }

    statusBar()->showMessage("Adaptive filter algorithm updated.");

}
