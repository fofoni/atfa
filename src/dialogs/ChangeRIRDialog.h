/*
 * Universidade Federal do Rio de Janeiro
 * Escola Politécnica
 * Projeto Final de Graduação
 * Ambiente de Teste para Filtros Adaptativos
 * Pedro Angelo Medeiros Fonini
 * Orientador: Markus Lima
 */

#ifndef CHANGERIRDIALOG_H
#define CHANGERIRDIALOG_H

#include <QDialog>
#include <QLayout>
#include <QtGui>

class ChangeRIRDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ChangeRIRDialog(QWidget *parent = 0);
    bool run();

private:
    QComboBox *choose_combo;

    QTextEdit *literal_edit;

    QDialogButtonBox *button_box;

private slots:

};

#endif // CHANGERIRDIALOG_H
