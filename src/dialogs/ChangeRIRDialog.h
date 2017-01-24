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
#include "../Signal.h"

class ChangeRIRDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ChangeRIRDialog(ATFA *parent);
    bool run();

    bool validate_everything();

    static Stream::container_t parse_txt(const QString &txt);

    static void err_dialog(const QString &err_msg, QWidget *p = 0);

    ATFA::RIR_source_t get_source() { return final_source; }
    QString get_filename() { return final_filename; }
    QString get_literal() { return final_literal; }

private:
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

    ATFA::RIR_source_t final_source;
    QString final_filename;
    QString final_literal;

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
