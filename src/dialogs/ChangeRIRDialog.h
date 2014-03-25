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

    QLabel *none_label;
    QWidget *literal_widget;
    QWidget *database_widget;
    QWidget *file_widget;

    QTextEdit *literal_edit;

    QDialogButtonBox *button_box;

    bool check_literal();

private slots:
    void set_rir_source(int n);
    void update_status();

};

class FloatStream {
public:
    FloatStream(std::istream *p) : err_flag(0), pos(start), ip(p), curr(0) {}
    ~FloatStream() { delete ip; }
    double get();
    double& current();
    int err_flag;
private:
    enum State {start, name, equals, open, end};
    State pos;
    std::istream *ip;
    double curr;
};

#endif // CHANGERIRDIALOG_H
