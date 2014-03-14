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
#include <QWidget>

#include "Stream.h"

class ATFA : public QMainWindow {

    Q_OBJECT

public:
    explicit ATFA(QWidget *parent = 0);

    Stream::Scenario scene;

    enum RIR_source_t {NoRIR, Literal, Database, File};
    enum RIR_filetype_t {None, MAT, WAV};

    RIR_source_t rir_source;
    RIR_filetype_t rir_filetype;
    QString rir_file;
    int database_index;

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
    void vad_checkbox_changed(int state);

    void flearn_on_toggled(bool t);

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
    QCheckBox *vad_checkbox;
    QGroupBox *flearn_group;
        QRadioButton *flearn_on_radio;
        QRadioButton *flearn_off_radio;
        QRadioButton *flearn_vad_radio;
    QGroupBox *fout_group;
        QRadioButton *fout_on_radio;
        QRadioButton *fout_off_radio;
        QRadioButton *fout_vad_radio;
    // right layout
    QPushButton *play_button;
    QWidget *delay_widget;
        QLabel *delay_label;
        QSlider *delay_slider;
        QSpinBox *delay_spin;
        QLabel *delay_units;
    QPushButton *zero_button;
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
