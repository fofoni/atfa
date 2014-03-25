/*
 * Universidade Federal do Rio de Janeiro
 * Escola Politécnica
 * Projeto Final de Graduação
 * Ambiente de Teste para Filtros Adaptativos
 * Pedro Angelo Medeiros Fonini
 * Orientador: Markus Lima
 */

#include "ChangeAlgorithmDialog.h"

ChangeAlgorithmDialog::ChangeAlgorithmDialog(QWidget *parent) :
    QDialog(parent)
{

    QVBoxLayout *layout = new QVBoxLayout(this);

    QLabel *nimp_label = new QLabel("NOT IMPLEMENTED YET", this);
    layout->addWidget(nimp_label);

    button_box = new QDialogButtonBox(
        QDialogButtonBox::Ok |
        QDialogButtonBox::Cancel
    );
    layout->addWidget(button_box);
    connect(button_box, SIGNAL(accepted()), this, SLOT(accept()));
    connect(button_box, SIGNAL(rejected()), this, SLOT(reject()));

    setLayout(layout);
    setWindowTitle("Change adaptative filter algorithm");

}

bool ChangeAlgorithmDialog::run() {
    if (exec() == QDialog::Rejected)
        return false;
    // ...
    return true;
}
