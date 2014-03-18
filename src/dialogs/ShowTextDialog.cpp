/*
 * Universidade Federal do Rio de Janeiro
 * Escola Politécnica
 * Projeto Final de Graduação
 * Ambiente de Teste para Filtros Adaptativos
 * Pedro Angelo Medeiros Fonini
 * Orientador: Markus Lima
 */

#include "ShowTextDialog.h"

ShowTextDialog::ShowTextDialog(QString title, QString t, QWidget *parent) :
    QDialog(parent), text(t), window_title(title)
{

    QVBoxLayout *layout = new QVBoxLayout(this);

    text_widget = new QTextBrowser(this);
    text_widget->setMinimumWidth(700);
    text_widget->setMinimumHeight(100);
    text_widget->setHtml(text);
    text_widget->setTextInteractionFlags(
        Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard |
        Qt::LinksAccessibleByMouse | Qt::LinksAccessibleByKeyboard
    );
    layout->addWidget(text_widget);

    button_box = new QDialogButtonBox(QDialogButtonBox::Close);
    layout->addWidget(button_box);
    connect(button_box, SIGNAL(accepted()), this, SLOT(accept()));
    connect(button_box, SIGNAL(rejected()), this, SLOT(reject()));

    setLayout(layout);
    setWindowTitle(window_title);

}
