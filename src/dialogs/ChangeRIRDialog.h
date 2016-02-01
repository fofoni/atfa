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

#include <vector>

#include <QDialog>
#include <QLayout>
#include <QtGui>

#include "../ATFA.h"
#include "../widgets/FileSelectWidget.h"

class ChangeRIRDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ChangeRIRDialog(ATFA *parent);
    bool run();

    bool validate_everything();

private:
    ATFA *atfa;

    QComboBox *choose_combo;

    QLabel *none_label;
    QWidget *literal_widget;
        QLabel *literal_label;
        QTextEdit *literal_edit;
    QWidget *database_widget;
        QLabel *database_label;
    QWidget *file_widget;
        QLabel *file_directions_label;
        QLabel *file_label;
        FileSelectWidget *file_select;

    QDialogButtonBox *button_box;

    void err_dialog(const QString &err_msg);

private slots:
    void set_rir_source(int n);
    void update_status();
    void accept_if_validated();

};

class FloatStream {
public:
    FloatStream(std::istream *p) : err_flag(0), pos(start), ip(p), curr(0) {}
    ~FloatStream() { delete ip; }
    double get(bool neg = false);
    double& current();
    int err_flag;
private:
    enum State {start, name, equals, open, end};
    State pos;
    std::istream *ip;
    double curr;
};

#endif // CHANGERIRDIALOG_H
