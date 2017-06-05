/*
 * Universidade Federal do Rio de Janeiro
 * Escola Politécnica
 * Projeto Final de Graduação
 * Ambiente de Teste para Filtros Adaptativos
 * Pedro Angelo Medeiros Fonini
 * Orientador: Markus Lima
 */

#include <sstream>
#include <cmath>

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

/* TODO: fazer uma interface p mostrar informações disponibilizadas pelas
 *       funções de querying do PorAudio, sobre os devices disponíveis, etc.
 */

/* TODO: detectar quando o filtro explodiu, e parar a simulação imediatamente
 *       (usar a função de "abort stream" do portaudio, ao invés da função de
 *       "stop stream")
 */

// TODO: quando a RIR selecionada for o "None", o botão de "show rir
//       coefficients" tem que estar HABILITADO, e mostrar "rir = [ 1 ]".

// TODO: verificar que não tem nenhum qDebug solto por aí

// TODO: no Stream::echo(), enche o data_out com ruído antes de começar,
//       pq se não o ruído entra no meio da simulação

// TODO: impedir de clicar no botão de mudar o ruído durante a simulação
//       (o botão não tem efeito, por isso é bad deixar que o usuário clique
//       nele, por uma questão de UX)

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
#include "dialogs/ChooseNumberDialog.h"
#include "dialogs/BenchmarkAdapfDialog.h"
#include "utils.h"

ATFA::ATFA(QWidget *parent) :
    QMainWindow(parent), stream(), pastream(NULL),
    muted(false), scene_filename("")
{

    delay_min = stream.scene.system_latency + stream.min_delay;

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

    // edit system latency
    syslatency_act = new QAction("Change system &latency", this);
    connect(syslatency_act, SIGNAL(triggered()),
            this, SLOT(change_syslatency()));

    // benchmark DSO times
    benchmark_act = new QAction("&Benchmark DSO", this);
    connect(benchmark_act, SIGNAL(triggered()), this, SLOT(benchmark_dso()));

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

    tools_menu = menuBar()->addMenu("&Tools");
    tools_menu->addAction(syslatency_act);
    tools_menu->addAction(benchmark_act);

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
            flearn_on_radio->setChecked(true);
            flearn_layout->addWidget(flearn_on_radio);
            flearn_layout->addWidget(flearn_off_radio);
            flearn_layout->addWidget(flearn_vad_radio);
            flearn_layout->addStretch(1); // TODO: do we need this?
        flearn_group->setLayout(flearn_layout);
        left_layout->addWidget(flearn_group);

        zero_button = new QPushButton("&Reset filter state", main_widget);
        zero_button->setDisabled(true);
        left_layout->addWidget(zero_button);

        play_button = new QPushButton(main_widget);
        play_button->setMinimumHeight(80);
        play_button->setIcon(QIcon(QPixmap("../../imgs/play.png")));
        play_button->setIconSize(QSize(58, 58));
        left_layout->addWidget(play_button);

    layout->addLayout(left_layout);

    QVBoxLayout *right_layout = new QVBoxLayout;

        first_row_widget = new QWidget(main_widget);
        QHBoxLayout *first_row_layout = new QHBoxLayout;

            vad_indicator_label = new QLabel("Voice Activity:",
                                             first_row_widget);
            first_row_layout->addWidget(vad_indicator_label);

            vad_indicator_led = new LEDIndicatorWidget(first_row_widget);
            first_row_layout->addWidget(vad_indicator_led);
            stream.setLED(vad_indicator_led);

            vad_algorithm_combo = new QComboBox;
            vad_algorithm_combo->addItem("Hard");
            vad_algorithm_combo->addItem("Soft");
            first_row_layout->addWidget(vad_algorithm_combo);

            first_row_div = new QFrame(first_row_widget);
            first_row_div->setFrameShape(QFrame::VLine);
            first_row_div->setFrameShadow(QFrame::Sunken);
            first_row_layout->addWidget(first_row_div);

            noise_label = new QLabel("Noise level:", first_row_widget);
            first_row_layout->addWidget(noise_label);

            noise_spin = new QSpinBox(first_row_widget);
            noise_spin->setMinimum(-80);
            noise_spin->setMaximum(-20);
            noise_spin->setFixedWidth(60);
            noise_spin->setValue(Scene::DEFAULT_NOISE);
            first_row_layout->addWidget(noise_spin);

            noise_unit_label = new QLabel("dB", first_row_widget);
            first_row_layout->addWidget(noise_unit_label);

        first_row_widget->setLayout(first_row_layout);
        right_layout->addWidget(first_row_widget);

        delay_widget = new QWidget(main_widget);
        QHBoxLayout *delay_layout = new QHBoxLayout;

            delay_label = new QLabel("Round trip delay:", delay_widget);
            delay_layout->addWidget(delay_label);

            delay_slider = new QSlider(Qt::Horizontal, delay_widget);
            delay_slider->setMinimumWidth(200);
            delay_slider->setMinimum(delay_min);
            delay_slider->setMaximum(delay_max);
            delay_slider->setValue(Scene::DEFAULT_DELAY);
            delay_layout->addWidget(delay_slider);
            { int stream_delay = delay_slider->value() -
                                 stream.scene.system_latency;
              if (stream_delay < stream.min_delay)
                  throw std::out_of_range("[ATFA::ATFA] initial stream delay"
                                          " too low.");
              stream.set_delay(static_cast<unsigned>(stream_delay)); }

            delay_spin = new QSpinBox(delay_widget);
            delay_spin->setMinimum(delay_min);
            delay_spin->setMaximum(delay_max);
            delay_spin->setFixedWidth(60);
            delay_spin->setValue(Scene::DEFAULT_DELAY);
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

    // Sintaxe nova -> complicada porque QComboBox::currentIndexChanged é
    //                 um sinal com overload, aí a gente tem que especificar
    //                 qual deles a gente quer.
    connect(vad_algorithm_combo,
            static_cast<void (QComboBox::*)(int)>(
                &QComboBox::currentIndexChanged),
            [this](int idx){stream.setVADAlgorithm(idx);});

    connect(noise_spin,
            static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            [this](int db){stream.set_noise(db);});

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

    // TODO: com a janela já mostrada, fazer um teste de audio pra ver se
    //       rola 11025Hz, e se não, faz o fallback pra 44100.
    //       Esse teste deve setar a função de callback accordingly.

}

int ATFA::get_delay() {
    return delay_spin->value();
}

void ATFA::update_widgets() {

    switch (stream.scene.filter_learning) {
    case Stream::Scenario::On:
        flearn_on_radio->setChecked(true);
        break;
    case Stream::Scenario::Off:
        flearn_off_radio->setChecked(true);
        break;
    case Stream::Scenario::VAD:
        flearn_vad_radio->setChecked(true);
        break;
    default:
        throw std::runtime_error(
                    "stream.scene.filter_learning has wrong value");
    }

    // TODO: o que fazer em relação ao filter_output?

    delay_min = stream.scene.system_latency + stream.min_delay;
    delay_slider->setMinimum(delay_min);
    delay_spin->setMinimum(delay_min);
    delay_slider->setValue(stream.scene.delay);
    {auto stream_delay = stream.scene.delay - stream.scene.system_latency;
    if (stream_delay < stream.min_delay)
        throw std::out_of_range("[ATFA::newscene] Stream delay (delay minus"
                                " system latency) cannot be less than the"
                                " duration of one block.");
    stream.set_delay(static_cast<unsigned>(stream_delay));}

    vol_slider->setValue(std::round(100*stream.scene.volume));

    switch (stream.scene.rir_source) {
    case Scene::NoRIR:
        rir_type_label->setText("None");
        rir_show_button->setDisabled(true);
        break;
    case Scene::Literal:
        rir_type_label->setText("Literal");
        rir_show_button->setDisabled(false);
        break;
    case Scene::File:
        {
            QString small_filename;
            QRegExp rx("^.*/([^/]*)$");
            if (rx.indexIn(stream.scene.rir_file) > -1)
                small_filename = rx.cap(1);
            else
                small_filename = stream.scene.rir_file;
            rir_type_label->setText(small_filename);
        }
        rir_show_button->setDisabled(false);
    }

    stream.setAdapfAlgorithm(
                new AdaptiveFilter<Stream::sample_t>(stream.scene.adapf_file));
    if (stream.adapf_is_dummy()) {
        adapf_file_label->setText("None");
        adapf_show_button->setDisabled(true);
    }
    else {
        adapf_file_label->setText(stream.get_adapf_title());
        adapf_show_button->setDisabled(false);
    }

}

void ATFA::newscene() {
    if (stream.running()) {
        QMessageBox msg_box;
        msg_box.setText("Can't change scenario while simulation is running.");
        msg_box.setWindowTitle("ATFA [info]");
        msg_box.setIcon(QMessageBox::Information);
        msg_box.exec();
        return;
    }
    stream.set_scene(Stream::Scenario());
    scene_filename = "";
    update_widgets();
    statusBar()->showMessage("New scenario with default parameters set.");
}

void ATFA::open() {
    QString filename = QFileDialog::getOpenFileName(
                this, "Open scenario file", QDir::currentPath(),
                "ATFA scenario files (*.atfascene)");
    if (filename == "") return;
    stream.set_scene(Scene(filename, delay_max));
    update_widgets();
    scene_filename = filename;
    // TODO: no utils.h, fazer rotina pra pegar o basename do arquivo
    statusBar()->showMessage("Scenario configuration loaded from file.");
}

void ATFA::save() {
    QString filename = scene_filename;
    if (filename == "") {
        filename = QFileDialog::getSaveFileName(
                    this, "Save File", QDir::currentPath(),
                    "ATFA scenario files (*.atfascene)");
    }
    if (filename == "")
        return;
    save_to_file(filename);
    statusBar()->showMessage("Scenario configuration successfully saved.");
}

void ATFA::save_as() {
    QString filename = QFileDialog::getSaveFileName(
                this, "Save File", QDir::currentPath(),
                "ATFA scenario files (*.atfascene)");
    if (filename == "")
        return;
    save_to_file(filename);
    statusBar()->showMessage("Scenario configuration successfully saved"
                             " to new file.");
}

void ATFA::save_to_file(QString filename) {
    if (filename == "")
        return;
    QFile save_file {filename};
    if (QFileInfo{save_file}.isDir()) {
        QMessageBox msg_box;
        msg_box.setText("You can't choose a directory.");
        msg_box.setWindowTitle("ATFA [error]");
        msg_box.setIcon(QMessageBox::Warning);
        msg_box.exec();
        return;
    }
    if (!save_file.open(QIODevice::WriteOnly)) {
        QMessageBox msg_box;
        msg_box.setText(QString{"Error opening file "} + filename + " .");
        msg_box.setWindowTitle("ATFA [error]");
        msg_box.setIcon(QMessageBox::Warning);
        msg_box.exec();
        return;
    }
    save_file.write(stream.scene.to_json().toJson());
    scene_filename = filename;
}

void ATFA::quit() {
    qApp->quit();
}


void ATFA::change_syslatency() {

    ChooseNumberDialog *choose_dialog = new ChooseNumberDialog(this,
                "The latency of your system is the portion of the round-trip"
                " delay that's outside of the control of this simulator. The"
                " delay set in the main window is the real round-trip delay,"
                " provided the system latency has been configured correctly in"
                " this dialog. You should measure your system's latency and set"
                " it accordingly. Please"
                " download a copy of the manual from " html_link(ATFA_BITLY)
                " for more detailed instructions.",
//                "The delay introduced"
//                " in a controlled manner by the simulator (refered to as the"
//                " 'stream delay') is the delay set in the main window"
//                " (reffered to as the 'scenario delay') minus the system"
//                " latency."
                "Change System Latency", "New system latency:",
                0, delay_max - stream.min_delay - 1,
                stream.scene.system_latency,
                "", "ms");
    if (!choose_dialog->run())
        return;

    stream.scene.system_latency = choose_dialog->chosen_num;
    int stream_delay = stream.scene.delay - stream.scene.system_latency;
    if (stream_delay < stream.min_delay) {
        stream.scene.delay = stream.scene.system_latency + stream.min_delay;
        stream_delay = stream.min_delay;
        QMessageBox msg_box(this);
        msg_box.setText("Impossible to achieve current delay with new system"
                        " latency value. Resetting delay to " +
                        QString::number(stream.scene.delay));
        msg_box.setWindowTitle("ATFA - New system latency [info]");
        msg_box.setIcon(QMessageBox::Critical);
        msg_box.exec();
    }

    delay_min = stream.scene.system_latency + stream.min_delay;
    delay_slider->setMinimum(delay_min);
    delay_spin->setMinimum(delay_min);

    statusBar()->showMessage("System latency set.");

}

void ATFA::benchmark_dso() {
    auto *bm_dialog = new BenchmarkAdapfDialog{this};
    bm_dialog->exec();
}

void ATFA::show_help() {
    QDialog *help_dialog = new QDialog{this};
    QVBoxLayout *layout = new QVBoxLayout{help_dialog};
                // TODO: filter_output ???
    QLabel *txt = new QLabel{
               "<h1>ATFA - Help</h1>\
                <p>This is a very short introduction to ATFA. For a more\
                detailed manual, please visit " html_link(ATFA_BITLY) ".</p> \
                <p>ATFA is the Ambiente para Testes de Filtros Adaptativos\
                (Testing environment for adaptive filters). To use it, adjust\
                the settings in the main window to setup your simulation, and\
                then click the \"play\" button.</p>\
                <p>The many settings and buttons presented in the main window\
                are briefly explained below.</p>\
                <ul>\
                    <li><b>Filter learning:</b> this box controls the learning\
                        of the adaptive filter. When set to 'Enabled', the\
                        adaptive filter will update its coefficients at every\
                        input sample. When set to 'Disabled', the coefficients\
                        will never be updated. When set to 'VAD', the\
                        coefficients will be updated only on the frames for\
                        which the Voice Activity Detector triggers. This\
                        setting can be changed online (while the simulation is\
                        running), and its effect is instantaneous.</li>\
                    <li><b>Reset filter state:</b> While the simulation is\
                        running, you can click this button to reset all filter\
                        coefficients back to their initial value (usually\
                        zero).</li>\
                    <li><b>Voice activity:</b> during simulation, this will\
                        glow in green when a voice signal has been detected in\
                        the input. You can choose between the <i>Soft</i> and\
                        the <i>Hard</i> detectors (the <i>Hard</i> detector is\
                        suggested if you're working in a noisy\
                        environment).</li>\
                    <li><b>Play button:</b> Click to start the simulation.</li>\
                    <li><b>Round trip delay:</b> The delay introduced by the\
                        simulated communications network. This includes all\
                        layers of the communications system, from the\
                        sound-capture hardware, all the way to the remote\
                        simulated echoing room, and back to the sound\
                        reproducing hardware. This value, given in miliseconds,\
                        depends on the latency of your audio system, over which\
                        the simulator has no control. For this value to be\
                        accurate, the system latency must be calibrated in\
                        Tools &gt; Change system latency.</li>\
                    <li><b>Volume:</b> The playback volume. You can use the\
                        \"Mute\" button to temporarily stop playback without\
                        stopping the simulation.</li>\
                    <li><b>Room impulse response:</b> When you start the\
                        simulator, the RIR is set to a kronecker delta (as\
                        if the audio output and input in the remote room\
                        were short-circuited). You can click the \"Change\"\
                        button to set a custom RIR. You can: 1) write down\
                        the exact coefficients you want; 2) choose a WAV file;\
                        or 3) go back to the default kronecker delta.</li>\
                    <li><b>Adaptive filtering algorithm:</b> When ATFA starts,\
                        no adaptive filter is loaded, and if you proceed to\
                        run the simulation without loading one, you will hear\
                        the convolution of the RIR with the sound activity in\
                        your room (e.g. your voice). Click \"Choose\" to load a\
                        custom adaptive filter.</li>\
                </ul>", help_dialog};
    txt->setWordWrap(true);
    QScrollArea *scroll = new QScrollArea;
    scroll->setMinimumWidth(600);
    scroll->setWidget(txt);
    layout->addWidget(scroll);
    help_dialog->setLayout(layout);
    help_dialog->setWindowTitle("ATFA - help");
    help_dialog->exec();
}

void ATFA::about_atfa() {
    QMessageBox msg_box;
    msg_box.setText(
                "<h1>Ambiente para Testes de Filtros Adaptativos</h1>\
                <p>You are using\
                <center><b>ATFA " ATFA_VERSION "</b></center><br />\
                You can find this software, along with a detailed manual,\
                in<br />\
                <center>" html_link(ATFA_URL) "</center><br />\
                Please, send comments and suggestions to the author<br />\
                <center>Pedro Angelo Medeiros Fonini ("
                html_email("pedro.fonini@smt.ufrj.br") ")</center><br />\
                This software was developed as part of the requirements for\
                obtaining the degree of electronics and computer engineer at\
                Universidade Federal do Rio de Janeiro (UFRJ). I am immensely\
                grateful to the help of my advisors Markus Lima and Paulo\
                Diniz, and to the Signals, Multimedia and Telecommunications\
                group (SMT/COPPE/UFRJ).</p>\
                <p>ATFA (in english, testing environment for adaptive filters)\
                is a software to aid the testing, teaching and\
                development of different types of adaptive filtering algorithms\
                for echo suppression. The software simulates, in real-time, an\
                L.E.M. (loudspeaker-enclosure-microphone) system, to which the\
                user's voice is subject, and then applies a given adaptive\
                filtering algorithm. You will know that your algorithm is\
                working as desired when you speak while the simulation is\
                running and do not hear the echo of your voices. You can\
                write your algorithms in C or C++ and feed them, compiled, to\
                the testing environment.</p>");
    msg_box.setWindowTitle("ATFA - about");
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
    stream.reset_adapf_state();
    statusBar()->showMessage("Adaptive filter memory zeroed.");
}

void ATFA::play_clicked() {
    if (stream.running()) {

        stream.stop(pastream);
        pastream = NULL;

        statusBar()->showMessage("Simulation stopped.");
        play_button->setIcon(QIcon(QPixmap("../../imgs/play.png")));

        rir_change_button->setDisabled(false);
        adapf_change_button->setDisabled(false);
        zero_button->setDisabled(true);
        newscene_act->setDisabled(false);
        save_act->setDisabled(false);
        open_act->setDisabled(false);
        save_as_act->setDisabled(false);
        benchmark_act->setDisabled(false);
        syslatency_act->setDisabled(false);

        vad_indicator_led->setLEDStatus(false);

    }
    else {

        rir_change_button->setDisabled(true);
        adapf_change_button->setDisabled(true);
        zero_button->setDisabled(false);
        newscene_act->setDisabled(true);
        save_act->setDisabled(true);
        open_act->setDisabled(true);
        save_as_act->setDisabled(true);
        benchmark_act->setDisabled(true);
        syslatency_act->setDisabled(true);

        pastream = stream.echo();

        statusBar()->showMessage("Simulation running...");
        play_button->setIcon(QIcon(QPixmap("../../imgs/pause.png")));

    }
}

void ATFA::delay_changed(int v) {
    stream.set_delay(static_cast<unsigned>(v - stream.scene.system_latency));
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
        stream.scene.volume = vol_slider->value()/100.0f;
        statusBar()->showMessage("Local speaker unmuted.");
    }
}
void ATFA::vol_changed(int v) {
    if (muted) return;
    stream.scene.volume = v/100.0f;
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
                         "You will hear nothing.<br />";
    }
    imp_resp_html << "</span>";
    ShowTextDialog *showrir_dialog = new ShowTextDialog(
                "RIR coefficients", imp_resp_html.str().c_str(), this);
    showrir_dialog->exec();

}

void ATFA::set_stream_rir(const Stream::container_t &h) {
    try {
        stream.set_filter(h);
    }
    catch (const std::length_error& e) {
        throw RIRSizeException(e.what());
    }
}

void ATFA::set_stream_rir(Signal h) {
    h.set_samplerate(stream.samplerate);
    try {
        stream.set_filter(h.array(), h.array() + h.samples());
    }
    catch (const std::length_error& e) {
        throw RIRSizeException(e.what());
    }
}

void ATFA::set_new_rir(Scene::RIR_source_t source, QString txt,
                       QString filename) {
    switch (source) {
    case Scene::NoRIR:
        set_stream_rir(Stream::container_t(1,1)); // set RIR samples
        stream.scene.set_rir<Scene::NoRIR>(); // set RIR metadata
        break;
    case Scene::Literal:
        set_stream_rir(ChangeRIRDialog::parse_txt(txt)); // set RIR samples
        stream.scene.set_rir<Scene::Literal>(); // set RIR metadata
        break;
    case Scene::File:
        {
            /// TODO: a libsndfile aceita outros tipos, além de WAV.
            /// (olhar documentação do Signal::Signal(const std::string&)
            /// TODO: deixar as err_dialog's mais descritivas.
            /// TODO: o código a seguir dever ser uma função separada
            ///       (get_container_from_file(std::string) ou algo assim),
            ///       e essa função deveria morar no utils (ou então, melhor
            ///       ainda: essa função pode ser um método estático estilo
            ///       factory da classe Signal)
            Scene::RIR_filetype_t filetype;
            QRegExp rx_m  ("*.m",   Qt::CaseInsensitive, QRegExp::Wildcard);
            QRegExp rx_wav("*.wav", Qt::CaseInsensitive, QRegExp::Wildcard);
            if (rx_m.exactMatch(filename))
                filetype = Scene::MAT;
            else if (rx_wav.exactMatch(filename))
                filetype = Scene::WAV;
            else
                throw RIRInvalidException("RIR file must be *.m or *.wav.");
            if (filetype == Scene::MAT) {
                QFile file(filename);
                if (!file.open(QIODevice::ReadOnly))
                    throw RIRInvalidException("Error opening RIR file.");
                set_stream_rir(ChangeRIRDialog::parse_txt( // set RIR samples
                                   file.readAll().constData()));
            }
            else {
                Signal s;
                try {
                    Signal sig_from_file(filename.toUtf8().constData());
                    s = sig_from_file;
                }
                catch (const FileError&) {
                    throw RIRInvalidException("Error opening file.");
                }
                set_stream_rir(s); // set RIR samples
            }
            stream.scene.set_rir<Scene::File>(filetype, filename);
        }
        break;
    default:
        throw std::runtime_error("Invalid source.");
    }
}

void ATFA::change_rir() {

    ChangeRIRDialog *chrir_dialog = new ChangeRIRDialog(this);
    if (!chrir_dialog->run())
        return;

    set_new_rir(chrir_dialog->get_source(), chrir_dialog->get_literal(),
                chrir_dialog->get_filename());

    switch (stream.scene.rir_source) {
    case Scene::NoRIR:
        rir_type_label->setText("None");
        rir_show_button->setDisabled(true);
        break;
    case Scene::Literal:
        rir_type_label->setText("Literal");
        rir_show_button->setDisabled(false);
        break;
    case Scene::File:
        {
            QString small_filename;
            QRegExp rx("^.*/([^/]*)$");
            if (rx.indexIn(stream.scene.rir_file) > -1)
                small_filename = rx.cap(1);
            else
                small_filename = stream.scene.rir_file;
            rir_type_label->setText(small_filename);
        }
        rir_show_button->setDisabled(false);
    }

    statusBar()->showMessage("Room impulse response updated.");

}

void ATFA::show_adapf() {

    std::stringstream adapf_html;
    adapf_html << "<span style='font-family: monospace'>";
    QString listing = stream.get_adapf_listing();
    QStringList ll = listing.split(" ");
    listing = ll.join(QString("&nbsp;"));
    ll = listing.split("\n");
    adapf_html << ll.join(QString("<br />")).toUtf8().constData();
    adapf_html << "</span>";
    ShowTextDialog *showrir_dialog = new ShowTextDialog(
                "Adaptative filtering code", adapf_html.str().c_str(), this);
    showrir_dialog->exec();

}


void ATFA::change_adapf() {

    ChangeAlgorithmDialog *chapf_dialog = new ChangeAlgorithmDialog(this);
    if (!chapf_dialog->run())
        return;

    if (stream.adapf_is_dummy()) {
        adapf_file_label->setText("None");
        adapf_show_button->setDisabled(true);
    }
    else {
        adapf_file_label->setText(stream.get_adapf_title());
        adapf_show_button->setDisabled(false);
    }

    statusBar()->showMessage("Adaptive filter algorithm updated.");

}
