/*
 * Universidade Federal do Rio de Janeiro
 * Escola Politécnica
 * Projeto Final de Graduação
 * Ambiente de Teste para Filtros Adaptativos
 * Pedro Angelo Medeiros Fonini
 * Orientador: Markus Lima
 */

#include <QDir>
#include <QFile>
#include <QRegExp>

#include "ChooseNumberDialog.h"
#include "../Stream.h"
#include "../AdaptiveFilter.h"
#include "../ATFA.h"
#include "../widgets/FileSelectWidget.h"

ChooseNumberDialog::ChooseNumberDialog(ATFA *parent, QString directions,
                                       QString title, QString number_name,
                                       int min, int max, int def,
                                       QString pref, QString suff) :
    QDialog(parent), atfa(parent), directions_str(directions),
    title_str(title), name_str(number_name), min_number(min), max_number(max)
{

    if (def < min  ||  def > max)
        throw std::out_of_range("In ChooseNumberDialog, you need"
                                " min<=def<=max.");

    QVBoxLayout *layout = new QVBoxLayout(this);

    directions_label = new QLabel(directions_str, this);
    directions_label->setWordWrap(true);
    layout->addWidget(directions_label);

    QHBoxLayout *choose_layout = new QHBoxLayout();

    spin_label = new QLabel(name_str, this);
    choose_layout->addWidget(spin_label);

    number_spin = new QSpinBox(this);
    number_spin->setRange(min_number, max_number);
    number_spin->setValue(def);
    number_spin->setPrefix(pref);
    number_spin->setSuffix(suff);
    choose_layout->addWidget(number_spin);

    layout->addLayout(choose_layout);

    button_box = new QDialogButtonBox(
        QDialogButtonBox::Ok |
        QDialogButtonBox::Cancel
    );
    layout->addWidget(button_box);
    connect(number_spin, SIGNAL(valueChanged(int)),
            this, SLOT(update_status()));
    connect(button_box, SIGNAL(accepted()), this, SLOT(accept()));
    connect(button_box, SIGNAL(rejected()), this, SLOT(reject()));

    setLayout(layout);
    setWindowTitle(title_str);

}

void ChooseNumberDialog::err_dialog(const QString& err_msg) {
    QMessageBox msg_box(parentWidget());
    msg_box.setText(err_msg);
    msg_box.setWindowTitle("ATFA - " + title_str + " [info]");
    msg_box.setIcon(QMessageBox::Critical);
    msg_box.exec();
}

bool ChooseNumberDialog::run() {

    if (exec() == QDialog::Rejected)
        return false;

    chosen_num = number_spin->value();
    if (chosen_num < min_number  ||  chosen_num > max_number) {
        err_dialog("Chosen number must respect min<=chosen<=max.");
        return false;
    }

    return true;

}

bool ChooseNumberDialog::validate_everything() {
    int v = number_spin->value();
    return min_number <= v  &&  v <= max_number;
}

void ChooseNumberDialog::update_status() {
    button_box->buttons()[0]->setDisabled(!validate_everything());
}

void ChooseNumberDialog::accept_if_validated() {
    if (validate_everything())
        accept();
}
