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

#include <QDialog>
#include <QLayout>
#include <QtGui>

class ChangeAlgorithmDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ChangeAlgorithmDialog(QWidget *parent = 0);
    bool run();

private:
    QDialogButtonBox *button_box;

private slots:

};

#endif // CHANGEALGORITHMDIALOG_H
