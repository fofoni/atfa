/*
 * Universidade Federal do Rio de Janeiro
 * Escola Politécnica
 * Projeto Final de Graduação
 * Ambiente de Teste para Filtros Adaptativos
 * Pedro Angelo Medeiros Fonini
 * Orientador: Markus Lima
 */

#ifndef CHANGEALGORITHMDIALOG_H
#define CHANGEALGORITHMDIALOG_H

#include <QtWidgets>
#include <QtGui>
#include <QDialog>
#include <QLayout>

#include "../widgets/FileSelectWidget.h"

class ATFA;

class ChangeAlgorithmDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ChangeAlgorithmDialog(ATFA *parent);
    bool run();

    bool validate_everything();

private:
    ATFA *atfa;

    QLabel *file_directions_label;
    QLabel *file_label;
    FileSelectWidget *file_select;

    QDialogButtonBox *button_box;

    void err_dialog(const QString &err_msg);

    bool discard;

private slots:
    void update_status();
    void accept_if_validated();

};

#endif // CHANGEALGORITHMDIALOG_H
