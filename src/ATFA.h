/*
 * Universidade Federal do Rio de Janeiro
 * Escola Politécnica
 * Projeto Final de Graduação
 * Ambiente de Teste para Filtros Adaptativos
 * Pedro Angelo Medeiros Fonini
 * Orientador: Markus Lima
 */

#ifndef ATFA_H
#define ATFA_H

#include <QtGui>
#include <QtWidgets>
#include <QMainWindow>

#include "Stream.h"
#include "widgets/LEDIndicatorWidget.h"

// TODO: limpar todos os new's (destruir todo mundo; membros no destrutor do
//       dono, e avulsos assim que possível)
//       alem disso, fechar o portaudio no destrutor do ATFA, caso necessario

class ATFA : public QMainWindow {

    Q_OBJECT

public:
    explicit ATFA(QWidget *parent = 0);

    Stream stream;
    PaStream *pastream;

    enum RIR_source_t {NoRIR, Literal, Database, File};
    enum RIR_filetype_t {None, MAT, WAV};

    RIR_source_t rir_source;
    RIR_filetype_t rir_filetype;
    QString rir_file;
    int database_index;
    bool muted;

private slots:
    // file
    void newscene();
    void open();
    void save();
    void save_as();
    void quit();

    // help
    void show_help();
    void about_atfa();
    void about_qt();

    // ui
    void flearn_on_toggled(bool t);
    void flearn_off_toggled(bool t);
    void flearn_vad_toggled(bool t);

    void zero_filter_clicked();

    void fout_on_toggled(bool t);
    void fout_off_toggled(bool t);
    void fout_vad_toggled(bool t);

    void play_clicked();

    void delay_changed(int v);

    void vol_mute_toggled(bool t);

    void vol_changed(int v);

    void show_rir();
    void change_rir();

    void show_adapf();
    void change_adapf();


private:
    QAction *newscene_act;
    QAction *open_act;
    QAction *save_act;
    QAction *save_as_act;
    QAction *quit_act;
    QAction *show_help_act;
    QAction *about_atfa_act;
    QAction *about_qt_act;

    QMenu *file_menu;
    QMenu *help_menu;

    QToolBar *toolbar;

    QWidget *main_widget;

    // left layout
    QGroupBox *flearn_group;
        QRadioButton *flearn_on_radio;
        QRadioButton *flearn_off_radio;
        QRadioButton *flearn_vad_radio;
    QPushButton *zero_button;
    QGroupBox *fout_group;
        QRadioButton *fout_on_radio;
        QRadioButton *fout_off_radio;
        QRadioButton *fout_vad_radio;
    // right layout
    QWidget *vad_indicator_widget;
        QLabel *vad_indicator_label;
        LEDIndicatorWidget *vad_indicator_led;
        QComboBox *vad_algorithm_combo;
    QPushButton *play_button;
    QWidget *delay_widget;
        QLabel *delay_label;
        QSlider *delay_slider;
        QSpinBox *delay_spin;
        QLabel *delay_units;
    QWidget *vol_widget;
        QLabel *vol_label;
        QPushButton *vol_mute_button;
        QSlider *vol_slider;
        QSpinBox *vol_spin;
    QWidget *rir_widget;
        QLabel *rir_label;
        QLabel *rir_type_label;
        QPushButton *rir_show_button;
        QPushButton *rir_change_button;
    QWidget *adapf_widget;
        QLabel *adapf_label;
        QLabel *adapf_file_label;
        QPushButton *adapf_show_button;
        QPushButton *adapf_change_button;

};

#endif // ATFA_H
