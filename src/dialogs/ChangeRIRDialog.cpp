/*
 * Universidade Federal do Rio de Janeiro
 * Escola Politécnica
 * Projeto Final de Graduação
 * Ambiente de Teste para Filtros Adaptativos
 * Pedro Angelo Medeiros Fonini
 * Orientador: Markus Lima
 */

#include "ChangeRIRDialog.h"

ChangeRIRDialog::ChangeRIRDialog(QWidget *parent) :
    QDialog(parent)
{

    QVBoxLayout *layout = new QVBoxLayout(this);

    QHBoxLayout *choose_layout = new QHBoxLayout;

        QLabel *choose_label = new QLabel("New RIR:");
        choose_layout->addWidget(choose_label);

        choose_combo = new QComboBox;
        choose_combo->addItem("");
        choose_combo->addItem("None");
        choose_combo->addItem("Literal");
        choose_combo->addItem("Database file");
        choose_combo->addItem("RIR file");
        choose_layout->addWidget(choose_combo);

    layout->addLayout(choose_layout);

    QLabel *none_label = new QLabel(
                "There will be no room impulse response. This is the same "
                "as setting <span style='font-family: monospace'>imp_resp"
                "&nbsp;=&nbsp;[&nbsp;1&nbsp;];</span>", this);
    none_label->setWordWrap(true);
    layout->addWidget(none_label);
    none_label->hide();

    QWidget *literal_widget = new QWidget(this);
    QVBoxLayout *literal_layout = new QVBoxLayout(literal_widget);

        QLabel *literal_label = new QLabel(
            "Write down the coefficients for the new room impulse response. "
            "Type floating-point numbers separated by whitespace and/or "
            "commas.", literal_widget
        );
        literal_label->setWordWrap(true);
        literal_layout->addWidget(literal_label);

        literal_edit = new QTextEdit(literal_widget);
        literal_layout->addWidget(literal_edit);

    literal_widget->setLayout(literal_layout);
    layout->addWidget(literal_widget);
    literal_widget->hide();

    QWidget *database_widget = new QWidget(this);
    QHBoxLayout *database_layout = new QHBoxLayout(database_widget);

        QLabel *database_label = new QLabel("Choose a RIR-database file:",
                                            literal_widget);
        database_layout->addWidget(database_label);

        QLabel *database_button_placeholder = new QLabel("NOT IMPLEMENTED YET",
                                                         database_widget);
        database_layout->addWidget(database_button_placeholder);

    database_widget->setLayout(database_layout);
    layout->addWidget(database_widget);
//    database_widget->hide();

    button_box = new QDialogButtonBox(
        QDialogButtonBox::Ok |
        QDialogButtonBox::Cancel
    );
    button_box->buttons()[0]->setDisabled(true);
    layout->addWidget(button_box);
    connect(button_box, SIGNAL(accepted()), this, SLOT(accept()));
    connect(button_box, SIGNAL(rejected()), this, SLOT(reject()));

    setLayout(layout);
    setMinimumWidth(324);
    setWindowTitle("Change RIR");

}

bool ChangeRIRDialog::run() {
    if (exec() == QDialog::Rejected)
        return false;
    // ...
    return true;
}
