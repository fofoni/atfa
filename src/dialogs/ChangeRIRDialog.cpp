/*
 * Universidade Federal do Rio de Janeiro
 * Escola Politécnica
 * Projeto Final de Graduação
 * Ambiente de Teste para Filtros Adaptativos
 * Pedro Angelo Medeiros Fonini
 * Orientador: Markus Lima
 */

#include <vector>
#include <sstream>

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

    none_label = new QLabel(
        "There will be no room impulse response. This is the same as setting "
        "<span style='font-family: monospace'>imp_resp&nbsp;=&nbsp;[&nbsp;1"
        "&nbsp;];</span>", this);
    none_label->setWordWrap(true);
    layout->addWidget(none_label);
    none_label->hide();

    literal_widget = new QWidget(this);
    QVBoxLayout *literal_layout = new QVBoxLayout(literal_widget);

        QLabel *literal_label = new QLabel(
            "Write down the coefficients for the new room impulse response. "
            "Type floating-point numbers separated by whitespace and/or "
            "commas.", literal_widget
        );
        literal_label->setWordWrap(true);
        literal_layout->addWidget(literal_label);

        literal_edit = new QTextEdit(literal_widget);
        literal_edit->setStyleSheet("font-family: monospace");
        literal_edit->setLineWrapMode(QTextEdit::NoWrap);
        literal_layout->addWidget(literal_edit);

    literal_widget->setLayout(literal_layout);
    layout->addWidget(literal_widget);
    literal_widget->hide();

    database_widget = new QWidget(this);
    QHBoxLayout *database_layout = new QHBoxLayout(database_widget);

        QLabel *database_label = new QLabel("Choose a RIR-database file:",
                                            database_widget);
        database_layout->addWidget(database_label);

        QLabel *database_button_placeholder = new QLabel("NOT IMPLEMENTED YET",
                                                         database_widget);
        database_layout->addWidget(database_button_placeholder);

    database_widget->setLayout(database_layout);
    layout->addWidget(database_widget);
    database_widget->hide();

    file_widget = new QWidget(this);
    QHBoxLayout *file_layout = new QHBoxLayout(file_widget);

        QLabel *file_label = new QLabel("Choose a RIR file:", file_widget);
        file_layout->addWidget(file_label);

        QLabel *file_button_placeholder = new QLabel("NOT IMPLEMENTED YET",
                                                     file_widget);
        file_layout->addWidget(file_button_placeholder);

    file_widget->setLayout(file_layout);
    layout->addWidget(file_widget);
    file_widget->hide();

    connect(choose_combo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(set_rir_source(int)));

    connect(literal_edit, SIGNAL(textChanged()), this, SLOT(update_status()));

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

    choose_combo->setFocus(Qt::PopupFocusReason);

}

bool ChangeRIRDialog::run(ATFA *w) {
    if (exec() == QDialog::Rejected)
        return false;
    FloatStream fs = new std::istringstream(
        literal_edit->toPlainText().toUtf8().constData());
    switch (choose_combo->currentIndex()) {
    case 0: // Nothing chosen
        return false;
    case 1: // No RIR
        w->scene.imp_resp.resize(1);
        w->scene.imp_resp[0] = 1;
        w->rir_source = ATFA::NoRIR;
        w->rir_filetype = ATFA::None;
        w->rir_file = "";
        w->database_index = -1;
        return true;
    case 2: // Literal
        w->scene.imp_resp.resize(0);
        while (fs.err_flag == 0)
            w->scene.imp_resp.push_back(fs.get());
        if (fs.err_flag == 1) {
            QMessageBox msg_box(parentWidget());
            msg_box.setText("Error parsing vector input. Please try again.");
            msg_box.setWindowTitle("ATFA - Change RIR [info]");
            msg_box.setIcon(QMessageBox::Critical);
            msg_box.exec();
            return false;
        }
        w->scene.imp_resp.pop_back(); // remove trailing zero
        w->rir_source = ATFA::Literal;
        w->rir_filetype = ATFA::None;
        w->rir_file = "";
        w->database_index = -1;
        return true;
    case 3: // database
    case 4: // file
        return false;
    }
    // should never be reached
    return false;
}

void ChangeRIRDialog::set_rir_source(int n) {
    switch (n) {
    case 0:
        none_label->hide();
        literal_widget->hide();
        database_widget->hide();
        file_widget->hide();
        break;
    case 1:
        literal_widget->hide();
        database_widget->hide();
        file_widget->hide();
        none_label->show();
        break;
    case 2:
        none_label->hide();
        database_widget->hide();
        file_widget->hide();
        literal_widget->show();
        break;
    case 3:
        none_label->hide();
        literal_widget->hide();
        file_widget->hide();
        database_widget->show();
        break;
    case 4:
        none_label->hide();
        literal_widget->hide();
        database_widget->hide();
        file_widget->show();
        break;
    }
    update_status();
    adjustSize();
}

double FloatStream::get(bool neg) {
    char ch = 0;
    do { // skip whitespace
        if (!ip->get(ch)) {
            err_flag = -1;
            pos = end;
            return 0;
        }
    } while (isspace(ch));
    if (pos == end) {
        err_flag = -1;
        return 0;
    }
    switch (ch) {
    case ',':
        pos = open;
        return get();
    case '[':
        if (pos == open) {
            err_flag = 1;
            return 0;
        }
        pos = open;
        return get();
    case '=':
        if (pos == open || pos == equals) {
            err_flag = 1;
            return 0;
        }
        pos = equals;
        return get();
    case ']':
        err_flag = -1;
        pos = end;
        return 0;
    case '1': case '2': case '3': case '4': case '5': case '6':
    case '7': case '8': case '9': case '0': case '.':
        ip->putback(ch);
        *ip >> curr;
        curr *= (neg ? -1 : 1);
        pos = open;
        return curr;
    case '-':
        if (neg) {
            err_flag = 1;
            return 0;
        }
        pos = open;
        return get(true);
    default:
        if (isalpha(ch)) {
            if (pos != start) {
                err_flag = 1;
                return 0;
            }
            while (ip->get(ch) && (isalnum(ch) || ch=='_')) /* pass */;
            ip->putback(ch);
            pos = name;
            return get();
        }
        err_flag = 1;
        return 0;
    }
}

bool ChangeRIRDialog::check_literal() {
    FloatStream fs = new std::istringstream(
        literal_edit->toPlainText().toUtf8().constData());
    while (fs.err_flag == 0)
        fs.get();
    return (fs.err_flag != 1);
}

void ChangeRIRDialog::update_status() {
    switch (choose_combo->currentIndex()) {
    case 0: // Nothing chosen
        button_box->buttons()[0]->setDisabled(true);
        return;
    case 1: // No RIR
        button_box->buttons()[0]->setDisabled(false);
        return;
    case 2: // Literal
        button_box->buttons()[0]->setDisabled(!check_literal());
        return;
    case 3: // database
    case 4: // file
        button_box->buttons()[0]->setDisabled(true);
        return;
    }
}
