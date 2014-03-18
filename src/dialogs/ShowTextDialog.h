/*
 * Universidade Federal do Rio de Janeiro
 * Escola Politécnica
 * Projeto Final de Graduação
 * Ambiente de Teste para Filtros Adaptativos
 * Pedro Angelo Medeiros Fonini
 * Orientador: Markus Lima
 */

#ifndef SHOWTEXTDIALOG_H
#define SHOWTEXTDIALOG_H

#include <QDialog>
#include <QLayout>
#include <QtGui>

class ShowTextDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ShowTextDialog(QString title, QString t = "", QWidget *parent = 0);

    QString text;
    QString window_title;

private:
    QTextBrowser *text_widget;
    QDialogButtonBox *button_box;

private slots:

};

#endif // SHOWTEXTDIALOG_H
