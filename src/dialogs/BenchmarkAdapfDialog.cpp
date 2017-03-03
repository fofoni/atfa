/*
 * Universidade Federal do Rio de Janeiro
 * Escola Politécnica
 * Projeto Final de Graduação
 * Ambiente de Teste para Filtros Adaptativos
 * Pedro Angelo Medeiros Fonini
 * Orientador: Markus Lima
 */

#include "BenchmarkAdapfDialog.h"
#include "../AdapfBenchmarker.h"

BenchmarkAdapfDialog::BenchmarkAdapfDialog(ATFA *parent)
    : QDialog(parent), atfa(parent)
{

    QVBoxLayout *layout = new QVBoxLayout(this);

    size_label = new QLabel("Set the size of Monte-Carlo loop, and hit Run:",
                            this);
    layout->addWidget(size_label);

    QHBoxLayout *run_layout = new QHBoxLayout();

    size_spin = new QSpinBox(this);
    size_spin->setMinimum(1);
    size_spin->setMaximum(MCLOOP_MAXIMUM);
    size_spin->setValue(MCLOOP_DEFAULT);
    size_spin->setMaximumWidth(100);
    run_layout->addWidget(size_spin);

    run_button = new QPushButton("Run", this);
    run_button->setMaximumWidth(100);
    connect(run_button, &QPushButton::clicked,
            this, &BenchmarkAdapfDialog::run_and_show);
    run_layout->addWidget(run_button);

    layout->addLayout(run_layout);

    result_label = new QLabel(this);
    result_label->setWordWrap(true);
    result_label->hide();
    layout->addWidget(result_label);

    button_box = new QDialogButtonBox(QDialogButtonBox::Close);
    connect(button_box->buttons()[0], &QPushButton::clicked,
            this, &QDialog::close);
    layout->addWidget(button_box);

    setLayout(layout);
    setWindowTitle("Benchmark adaptive filter DSO");

    setMinimumWidth(600);

}

void BenchmarkAdapfDialog::run_and_show() {

    result_label->setDisabled(true);
    run_button->setDisabled(true);
    size_spin->setDisabled(true);

    button_box->buttons()[0]->clearFocus();

    repaint();

    int N = size_spin->value();

    AdapfBenchmarker<Stream::sample_t> bm(*atfa->stream.adapf);
    double duration_us_true  = bm.benchmark<true >(N).count() * 1e6;
    double duration_us_false = bm.benchmark<false>(N).count() * 1e6;

    QString dso_filename = atfa->stream.adapf->get_path().c_str();
    if (dso_filename == "")
        dso_filename = "(no adaptive filter loaded)";

    QString dso_title = atfa->stream.get_adapf_title();
    if (dso_title == "")
        dso_title = "(None)";

    result_label->setText(QString{} +
                "<br /><ul>\
                    <li>DSO: <b>" + qt_html_tt(dso_filename) +"</b></li>\
                    <li>Algorithm: <b>" + dso_title + "</b><br /></li>\
                    <li>When the DSO is asked to <b>update</b> the\
                        coefficient vector:<br />\
                        <ul>\
                            <li>" + qt_html_tt("adapf_run") + " function called\
                                <b>" + QString::number(N) + "</b> times, with\
                                mean call duration of <b>" +
                                QString::number(duration_us_true, 'f', 3) +
                                " " + QString(QChar(0x03BC)) + "s</b></li>\
                            <li>Disregarding any overhead, this algorithm could\
                                run at <b>" +
                                QString::number(1e3/duration_us_true, 'f', 3) +
                                " kHz</b><br /></li>\
                        </ul></li>\
                    <li>When the DSO is asked to <b>not update</b> the\
                        coefficient vector:\
                        <ul>\
                            <li>" + qt_html_tt("adapf_run") + " function called\
                                <b>" + QString::number(N) + "</b> times, with\
                                mean call duration of <b>" +
                                QString::number(duration_us_false, 'f', 3) +
                                " " + QString(QChar(0x03BC)) + "s</b></li>\
                            <li>Disregarding any overhead, this algorithm could\
                                run at <b>" +
                                QString::number(1e3/duration_us_false, 'f', 3) +
                                " kHz</b></li>\
                        </ul></li>\
                </ul>");
    result_label->show();

    result_label->setDisabled(false);
    run_button->setDisabled(false);
    size_spin->setDisabled(false);

    run_button->setFocus();

    adjustSize();

}
