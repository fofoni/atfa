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
#include "../Signal.h"
#include "../Stream.h"

ChangeRIRDialog::ChangeRIRDialog(ATFA *parent) :
    QDialog(parent), atfa(parent)
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

    database_widget = new QWidget(this);
    QHBoxLayout *database_layout = new QHBoxLayout(database_widget);

        database_label = new QLabel("Choose a RIR-database file:",
                                    database_widget);
        database_layout->addWidget(database_label);

        QLabel *database_button_placeholder = new QLabel("NOT IMPLEMENTED YET",
                                                         database_widget);
        database_layout->addWidget(database_button_placeholder);

    database_widget->setLayout(database_layout);
    layout->addWidget(database_widget);
    database_widget->hide();

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

void ChangeRIRDialog::err_dialog(const QString& err_msg) {
    QMessageBox msg_box(parentWidget());
    msg_box.setText(err_msg);
    msg_box.setWindowTitle("ATFA - Change RIR [info]");
    msg_box.setIcon(QMessageBox::Critical);
    msg_box.exec();
}

bool ChangeRIRDialog::run() {
    /// TODO: fazer um backup do atfa->stream.scene antes de sair modificando
    /// tudo, pq várias vezes a gente tá modificando pra depois parar no meio
    /// dizendo que deu erro, e deixando o estado sujo.
    if (exec() == QDialog::Rejected)
        return false;
    switch (choose_combo->currentIndex()) {
    case 0: // Nothing chosen
        return false;
    case 1: // No RIR
        atfa->stream.set_filter(std::vector<Stream::sample_t>(1,1));
        atfa->rir_source = ATFA::NoRIR;
        atfa->rir_filetype = ATFA::None;
        atfa->rir_file = "";
        atfa->database_index = -1;
        return true;
    case 2: // Literal
        {
            FloatStream fs = new std::istringstream(
                literal_edit->toPlainText().toUtf8().constData());
            Stream::container_t h;
            while (fs.err_flag == 0)
                h.push_back(fs.get());
            if (fs.err_flag == 1) {
                err_dialog("Error parsing vector input. Please try again.");
                return false;
            }
            h.pop_back(); // remove trailing zero
            if (h.size() >= Stream::fft_size - Stream::blk_size)
                err_dialog(
                    (std::string("RIR deve ter no máximo ") +
                    std::to_string(Stream::fft_size - Stream::blk_size) +
                    std::string(" samples.")).c_str()
                );
            atfa->stream.set_filter(h);
        }
        atfa->rir_source = ATFA::Literal;
        atfa->rir_filetype = ATFA::None;
        atfa->rir_file = "";
        atfa->database_index = -1;
        return true;
    case 3: // database
        return false;
    case 4: // file
        {
            /// TODO: a libsndfile aceita outros tipos, além de WAV.
            /// (olhar documentação do Signal::Signal(const std::string&)
            /// TODO: deixar as err_dialog's mais descritivas.
            ATFA::RIR_filetype_t filetype;
            QString filename = file_select->text();

            QRegExp rx_m  ("*.m",   Qt::CaseInsensitive, QRegExp::Wildcard);
            QRegExp rx_wav("*.wav", Qt::CaseInsensitive, QRegExp::Wildcard);
            if (rx_m.exactMatch(filename))
                filetype = ATFA::MAT;
            else if (rx_wav.exactMatch(filename))
                filetype = ATFA::WAV;
            else {
                err_dialog("Please, choose a *.m or *.wav file.");
                return false;
            }

            if (filetype == ATFA::MAT) {
                QFile file(filename);
                if (!file.open(QIODevice::ReadOnly)) {
                    err_dialog("Error opening file.");
                    return false;
                }
                FloatStream fs = new std::istringstream(
                            file.readAll().constData());
                Stream::container_t h;
                while (fs.err_flag == 0)
                    h.push_back(fs.get());
                if (fs.err_flag == 1) {
                    err_dialog("Error parsing file contents."
                               " Please try again.");
                    return false;
                }
                h.pop_back(); // remove trailing zero
                if (h.size() >= Stream::fft_size - Stream::blk_size)
                    err_dialog(
                        (std::string("RIR deve ter no máximo ") +
                        std::to_string(Stream::fft_size - Stream::blk_size) +
                        std::string(" samples.")).c_str()
                    );
                atfa->stream.set_filter(h);
            }
            else {
                Signal s;
                try {
                    Signal sig_from_file(filename.toUtf8().constData());
                    s = sig_from_file;
                }
                catch (const FileError&) {
                    err_dialog("Error opening file.");
                    return false;
                }
                s.set_samplerate(atfa->stream.samplerate);
                if (s.samples() >= Stream::fft_size - Stream::blk_size)
                    err_dialog(
                        (std::string("RIR deve ter no máximo ") +
                        std::to_string(Stream::fft_size - Stream::blk_size) +
                        std::string(" samples.")).c_str()
                    );
                atfa->stream.set_filter(
                            s.array(), s.array() + s.samples());
            }
            atfa->rir_filetype = filetype;
            atfa->rir_source = ATFA::File;
            atfa->rir_file = filename;
            atfa->database_index = -1;
        }
        return true;
    }
    // should never be reached
    /// TODO: if DEGUB, stderr << "deu ruim";
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
    case 3: // database
        return false;
    case 4: // file
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
