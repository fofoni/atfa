/*
 * Universidade Federal do Rio de Janeiro
 * Escola Politécnica
 * Projeto Final de Graduação
 * Ambiente de Teste para Filtros Adaptativos
 * Pedro Angelo Medeiros Fonini
 * Orientador: Markus Lima
 */

#include <QFileDialog>

#include "FileSelectWidget.h"

FileSelectWidget::FileSelectWidget(
    const QString& cpt, const QString& d,
    const QString& flt, QWidget *parent, QWidget *parentWindow
)
    : QWidget(parent), caption(cpt), dir(d), filter(flt), pWin(parentWindow)
{

    QHBoxLayout *layout = new QHBoxLayout(this);

        file_path_edit = new QLineEdit(this);
        layout->addWidget(file_path_edit);

        choose_button = new QPushButton("Choose...", this);
        layout->addWidget(choose_button);

    connect(file_path_edit, SIGNAL(returnPressed()),
            this, SIGNAL(returnPressed()));
    connect(file_path_edit, SIGNAL(textChanged(const QString&)),
            this, SIGNAL(textChanged(const QString&)));

    connect(choose_button, SIGNAL(clicked(bool)),
            this, SLOT(showDialog()));

    setLayout(layout);

}

QString FileSelectWidget::text() {
    return file_path_edit->text();
}

void FileSelectWidget::showDialog() {
    QString filename = QFileDialog::getOpenFileName(pWin, caption, dir, filter);
    if (!filename.isEmpty())
        file_path_edit->setText(filename);
}
