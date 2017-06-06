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

/* TODO: devemos mostrar como "taxa máxima" a taxa atingida quando o período
 *       é o *dobro* da duração da DSO. Fala que "assuming 50% of CPU
 *       time lost to scheduling jitter, processing overhead, and other
 *       processes in the system..."
 */

BenchmarkAdapfDialog::BenchmarkAdapfDialog(ATFA *parent)
    : QDialog(parent), atfa(parent)
{

    QVBoxLayout *layout = new QVBoxLayout(this);

    choosefile_label = new QLabel("Choose an input WAV file, and hit Run:",
                                  this);
    layout->addWidget(choosefile_label);

    QHBoxLayout *run_layout = new QHBoxLayout();

        file_select = new FileSelectWidget(
                    "Open WAV file", QDir::currentPath(),
                    "WAV files (*.wav)", this, this);
        run_layout->addWidget(file_select);

        // TODO: impedir de clicar em "Run" se o file_select estiver vazio
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
    file_select->setDisabled(true);

    button_box->buttons()[0]->clearFocus();

    repaint();

    Signal input_signal{file_select->text().toUtf8().constData()};
    int N = input_signal.samples();

    // TODO: o benchmark deveria ser feito num thread separado, pra manter a
    //       interface responsiva
    //       Idéia: criar uma função AdapfBenchmarker::estimate_time que rode
    //       o adapf poucas vezes (quantas? vai rodando e vendo quantas vezes
    //       vale a pena rodar, à la python's timeit) e retorne uma estimação
    //       do tempo total que vai levar o benchmark de verdade; usar o
    //       resultado dessa estimação pra decidir se vale a pena botar o
    //       bagulho num thread separado.
    AdapfBenchmarker<Stream::sample_t> bm{
        *atfa->stream.adapf, input_signal, Signal{atfa->stream.scene.imp_resp},
        atfa->stream.scene.noise_vol};
    auto pair = bm.benchmark<0>();
    double duration_us_0 = pair.first.count() * 1e6;
    int nup_0 = pair.second;
    pair = bm.benchmark<1>();
    double duration_us_1 = pair.first.count() * 1e6;
    int nup_1 = pair.second;
    pair = bm.benchmark<2>();
    double duration_us_2 = pair.first.count() * 1e6;
    int nup_2 = pair.second;

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
                    <li>When the DSO is asked to <b>never update</b> the\
                        coefficient vector:<br />\
                        <ul>\
                            <li>" + qt_html_tt("adapf_run") + " function called\
                                <b>" + QString::number(N) + "</b> times, with\
                                mean call duration of <b>" +
                                QString::number(duration_us_0, 'f', 3) +
                                " " + QString(QChar(0x03BC)) + "s</b></li>\
                            <li>Disregarding any overhead, this algorithm could\
                                run at <b>" +
                                QString::number(1e3/duration_us_0, 'f', 3) +
                                " kHz</b><br /></li>\
                            <li>The " + qt_html_tt("adapf_run") + " function\
                                performed an update <b>" + QString::number(
                                static_cast<double>(nup_0)/N*100, 'f', 1) +
                                "%</b> of the time<br /></li>\
                        </ul></li>\
                    <li>When the DSO is asked to <b>update normally</b> the\
                        coefficient vector:<br />\
                        <ul>\
                            <li>" + qt_html_tt("adapf_run") + " function called\
                                <b>" + QString::number(N) + "</b> times, with\
                                mean call duration of <b>" +
                                QString::number(duration_us_1, 'f', 3) +
                                " " + QString(QChar(0x03BC)) + "s</b></li>\
                            <li>Disregarding any overhead, this algorithm could\
                                run at <b>" +
                                QString::number(1e3/duration_us_1, 'f', 3) +
                                " kHz</b><br /></li>\
                            <li>The " + qt_html_tt("adapf_run") + " function\
                                performed an update <b>" + QString::number(
                                static_cast<double>(nup_1)/N*100, 'f', 1) +
                                "%</b> of the time<br /></li>\
                        </ul></li>\
                    <li>When the DSO is asked to <b>always update</b> the\
                        coefficient vector:\
                        <ul>\
                            <li>" + qt_html_tt("adapf_run") + " function called\
                                <b>" + QString::number(N) + "</b> times, with\
                                mean call duration of <b>" +
                                QString::number(duration_us_2, 'f', 3) +
                                " " + QString(QChar(0x03BC)) + "s</b></li>\
                            <li>Disregarding any overhead, this algorithm could\
                                run at <b>" +
                                QString::number(1e3/duration_us_2, 'f', 3) +
                                " kHz</b></li>\
                            <li>The " + qt_html_tt("adapf_run") + " function\
                                performed an update <b>" + QString::number(
                                static_cast<double>(nup_2)/N*100, 'f', 1) +
                                "%</b> of the time<br /></li>\
                        </ul></li>\
                </ul>");
    result_label->show();

    result_label->setDisabled(false);
    run_button->setDisabled(false);
    file_select->setDisabled(false);

    run_button->setFocus();

    adjustSize();

}
