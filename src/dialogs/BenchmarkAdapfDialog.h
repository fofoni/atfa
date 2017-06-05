/*
 * Universidade Federal do Rio de Janeiro
 * Escola Politécnica
 * Projeto Final de Graduação
 * Ambiente de Teste para Filtros Adaptativos
 * Pedro Angelo Medeiros Fonini
 * Orientador: Markus Lima
 */

#ifndef BENCHMARKADAPFDIALOG_H
#define BENCHMARKADAPFDIALOG_H

#include <QtGui>

#include "../ATFA.h"

class BenchmarkAdapfDialog : public QDialog
{
    Q_OBJECT

public:
    constexpr static int MCLOOP_MAXIMUM = 10000000; // 10e6
    constexpr static int MCLOOP_DEFAULT = MCLOOP_MAXIMUM/100;

    explicit BenchmarkAdapfDialog(ATFA *);

private:
    ATFA *atfa;

    QLabel *choosefile_label;
    FileSelectWidget *file_select;
    QPushButton *run_button;
    QLabel *result_label;

    QDialogButtonBox *button_box;

private slots:
    void run_and_show();

};

#endif // BENCHMARKADAPFDIALOG_H
