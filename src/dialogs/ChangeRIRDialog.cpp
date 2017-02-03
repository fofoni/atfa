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
#include <algorithm>

#include <QDir>
#include <QFile>
#include <QRegExp>

#include "ChangeRIRDialog.h"
#include "../widgets/FileSelectWidget.h"
#include "../Stream.h"

ChangeRIRDialog::ChangeRIRDialog(ATFA *parent) :
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

        literal_label = new QLabel(
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

    file_widget = new QWidget(this);
    QVBoxLayout *file_layout = new QVBoxLayout(file_widget);

        file_directions_label = new QLabel(
                    "You may choose either a MATLAB script file whose content"
                    " is only a declaration of a row vector; or a WAV file.",
                    file_widget);
        file_directions_label->setWordWrap(true);
        file_layout->addWidget(file_directions_label);

        QHBoxLayout *file_choose_layout = new QHBoxLayout();

        file_label = new QLabel("Choose a RIR file:", file_widget);
        file_choose_layout->addWidget(file_label);

        file_select = new FileSelectWidget(
                    "Open RIR file", QDir::currentPath(),
                    "MATLAB script files (*.m);;WAV files (*.wav)",
                    file_widget);
        file_choose_layout->addWidget(file_select);

        file_layout->addLayout(file_choose_layout);

    file_widget->setLayout(file_layout);
    layout->addWidget(file_widget);
    file_widget->hide();

    connect(choose_combo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(set_rir_source(int)));

    connect(literal_edit, SIGNAL(textChanged()), this, SLOT(update_status()));
    connect(file_select, SIGNAL(textChanged(const QString&)),
            this, SLOT(update_status()));

    button_box = new QDialogButtonBox(
        QDialogButtonBox::Ok |
        QDialogButtonBox::Cancel
    );
    button_box->buttons()[0]->setDisabled(true);
    layout->addWidget(button_box);
    connect(button_box, SIGNAL(accepted()), this, SLOT(accept()));
    connect(button_box, SIGNAL(rejected()), this, SLOT(reject()));
    connect(file_select, SIGNAL(returnPressed()),
            this, SLOT(accept_if_validated()));

    setLayout(layout);
    setMinimumWidth(324);
    setWindowTitle("Change RIR");

    choose_combo->setFocus(Qt::PopupFocusReason);

}

void ChangeRIRDialog::err_dialog(const QString& err_msg, QWidget *p) {
    QMessageBox msg_box(p);
    msg_box.setText(err_msg);
    msg_box.setWindowTitle("ATFA - Change RIR [info]");
    msg_box.setIcon(QMessageBox::Critical);
    msg_box.exec();
}

Stream::container_t ChangeRIRDialog::parse_txt(const QString& txt) {
    FloatStream fs = new std::istringstream(txt.toUtf8().constData());
    Stream::container_t h;
    while (fs.err_flag == 0)
        h.push_back(float(fs.get()));
    if (fs.err_flag == 1)
        throw RIRParseException("Non-conforming input data.");
    h.pop_back(); // remove trailing zero
    return h;
}

bool ChangeRIRDialog::run() {
    if (exec() == QDialog::Rejected)
        return false;
    int ccci = choose_combo->currentIndex();
    if (ccci == 0)
        return false;
    final_source = Scene::RIR_source_t(ccci - 1);
    final_filename = file_select->text();
    final_literal = literal_edit->toPlainText();
    return true;
}

void ChangeRIRDialog::set_rir_source(int n) {
    switch (n) {
    case 0:
        none_label->hide();
        literal_widget->hide();
        file_widget->hide();
        break;
    case 1:
        literal_widget->hide();
        file_widget->hide();
        none_label->show();
        break;
    case 2:
        none_label->hide();
        file_widget->hide();
        literal_widget->show();
        break;
    case 3:
        none_label->hide();
        literal_widget->hide();
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

bool ChangeRIRDialog::validate_everything() {
    /// TODO: usar funcionalidades de "validate" do Qt
    switch (choose_combo->currentIndex()) {
    case 0: // Nothing chosen
        return false;
    case 1: // No RIR
        return true;
    case 2: // Literal
    {
        FloatStream fs = new std::istringstream(
            literal_edit->toPlainText().toUtf8().constData());
        while (fs.err_flag == 0)
            fs.get();
        return (fs.err_flag != 1);
    }
    case 3: // file
        return !file_select->text().isEmpty();
    }
    // should never be reached
    /// TODO: if DEBUG, stderr << deu ruim.
    return false;
}

void ChangeRIRDialog::update_status() {
    button_box->buttons()[0]->setDisabled(!validate_everything());
}

void ChangeRIRDialog::accept_if_validated() {
    if (validate_everything())
        accept();
}
