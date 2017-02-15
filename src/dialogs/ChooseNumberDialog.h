/*
 * Universidade Federal do Rio de Janeiro
 * Escola Politécnica
 * Projeto Final de Graduação
 * Ambiente de Teste para Filtros Adaptativos
 * Pedro Angelo Medeiros Fonini
 * Orientador: Markus Lima
 */

#ifndef CHOOSENUMBERDIALOG_H
#define CHOOSENUMBERDIALOG_H

#include <QtWidgets>
#include <QtGui>
#include <QDialog>
#include <QLayout>

#include "../widgets/FileSelectWidget.h"

class ATFA;

class ChooseNumberDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ChooseNumberDialog(ATFA *parent, QString directions, QString title,
                                QString number_name, int min, int max, int def,
                                QString pref, QString suff);
    bool run();

    bool validate_everything();

    int chosen_num;

private:
    ATFA *atfa;

    QLabel *directions_label;
    QLabel *spin_label;
    QSpinBox *number_spin;

    QDialogButtonBox *button_box;

    QString directions_str;
    QString title_str;
    QString name_str;
    int min_number, max_number;

    void err_dialog(const QString &err_msg);

private slots:
    void update_status();
    void accept_if_validated();

};

#endif // CHOOSENUMBERDIALOG_H
