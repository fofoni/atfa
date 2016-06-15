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

#include "ChangeAlgorithmDialog.h"
#include "../Stream.h"
#include "../AdaptiveFilter.h"
#include "../ATFA.h"
#include "../widgets/FileSelectWidget.h"

ChangeAlgorithmDialog::ChangeAlgorithmDialog(ATFA *parent) :
    QDialog(parent), atfa(parent)
{

    QVBoxLayout *layout = new QVBoxLayout(this);

    file_directions_label = new QLabel(
                "Choose a shared object file (*.so) containing the"
                " implementation of the adaptive filtering algorithm.",
                this);
    file_directions_label->setWordWrap(true);
    layout->addWidget(file_directions_label);

    QHBoxLayout *file_choose_layout = new QHBoxLayout();

    file_label = new QLabel("Choose a DSO file:", this);
    file_choose_layout->addWidget(file_label);

    file_select = new FileSelectWidget(
                "Open DSO file", QDir::currentPath(),
                "Dynamic Shared Object files (*.so)",
                this);
    file_choose_layout->addWidget(file_select);

    layout->addLayout(file_choose_layout);

    button_box = new QDialogButtonBox(
        QDialogButtonBox::Ok |
        QDialogButtonBox::Cancel
    );
    button_box->buttons()[0]->setDisabled(true);
    layout->addWidget(button_box);
    connect(file_select, SIGNAL(textChanged(const QString&)),
            this, SLOT(update_status()));
    connect(button_box, SIGNAL(accepted()), this, SLOT(accept()));
    connect(button_box, SIGNAL(rejected()), this, SLOT(reject()));
    connect(file_select, SIGNAL(returnPressed()),
            this, SLOT(accept_if_validated()));

    setLayout(layout);
    setWindowTitle("Change adaptative filter algorithm");

}

void ChangeAlgorithmDialog::err_dialog(const QString& err_msg) {
    QMessageBox msg_box(parentWidget());
    msg_box.setText(err_msg);
    msg_box.setWindowTitle("ATFA - Change Algorithm [info]");
    msg_box.setIcon(QMessageBox::Critical);
    msg_box.exec();
}

bool ChangeAlgorithmDialog::run() {

    if (exec() == QDialog::Rejected)
        return false;

    QString filename = file_select->text();
    QRegExp rx_so("*.so", Qt::CaseInsensitive, QRegExp::Wildcard);
    if (!rx_so.exactMatch(filename)) {
        err_dialog("Please, choose a *.so file.");
        return false;
    }

    atfa->stream.setAdapfAlgorithm(
        new AdaptiveFilter<Stream::sample_t>(filename.toUtf8().constData())
    );

    return true;

}

bool ChangeAlgorithmDialog::validate_everything() {
    return !file_select->text().isEmpty();
}

void ChangeAlgorithmDialog::update_status() {
    button_box->buttons()[0]->setDisabled(!validate_everything());
}

void ChangeAlgorithmDialog::accept_if_validated() {
    if (validate_everything())
        accept();
}
