/*
 * Universidade Federal do Rio de Janeiro
 * Escola Politécnica
 * Projeto Final de Graduação
 * Ambiente de Teste para Filtros Adaptativos
 * Pedro Angelo Medeiros Fonini
 * Orientador: Markus Lima
 */

#include "dialogs/ChangeRIRDialog.h"
#include "Stream.h"
#include "utils.h"

template<>
void Scene::set_rir<Scene::NoRIR>(
    RIR_filetype_t filetype, const QString& file
) {
    if (filetype != None  ||  file.length() != 0) // should never happen
        throw std::runtime_error("Unknown error: Scene::set_rir<Scene::NoRIR>"
                                 " called with more than zero arguments."
                                 " Scene::NoRIR impulse responses should not"
                                 " specify a file or filetype.");
    rir_source = NoRIR;
    rir_filetype = None;
    rir_file = "";
}

template<>
void Scene::set_rir<Scene::Literal>(
    RIR_filetype_t filetype, const QString& file
) {
    if (filetype != None  ||  file.length() != 0) // should never happen
        throw std::runtime_error("Unknown error: Scene::set_rir<Scene::Literal>"
                                 " called with more than zero arguments."
                                 " Scene::Literal impulse responses should not"
                                 " specify a file or filetype.");
    rir_source = Literal;
    rir_filetype = None;
    rir_file = "";
}

template<>
void Scene::set_rir<Scene::File>(
    RIR_filetype_t filetype, const QString& file
) {
    if (filetype == None  ||  file.length() == 0) // should never happen
        throw std::runtime_error("Unknown error: Scene::set_rir<Scene::File>"
                                 " called with no arguments."
                                 " Scene::File impulse responses should"
                                 " specify a file and a filetype.");
    rir_source = File;
    rir_filetype = filetype;
    rir_file = file;
}

Scene::Scenario(const QJsonObject &json, int delay_max)
{

    // filter_learning
    if (json.contains(QStringLiteral("filter_learning"))) {
        const QString &flearn_str = json["filter_learning"].toString();
        if (flearn_str == QStringLiteral("On"))
            filter_learning = On;
        else if (flearn_str == QStringLiteral("Off"))
            filter_learning = Off;
        else if (flearn_str == QStringLiteral("VAD"))
            filter_learning = VAD;
        else
            throw SceneJsonInvtokenException(
                    "in filter_learning",
                    "one of {On,Off,VAD}",
                    flearn_str.toUtf8().constData());
    }
    else
        filter_learning = DEFAULT_FLEARN;

    // filter_output
    if (json.contains(QStringLiteral("filter_output"))) {
        const QString &fout_str = json["filter_output"].toString();
        if (fout_str == QStringLiteral("On"))
            filter_output = On;
        else if (fout_str == QStringLiteral("Off"))
            filter_output = Off;
        else if (fout_str == QStringLiteral("VAD"))
            filter_output = VAD;
        else
            throw SceneJsonInvtokenException(
                    "in filter_output",
                    "one of {On,Off,VAD}",
                    fout_str.toUtf8().constData());
    }
    else
        filter_output = DEFAULT_FOUTPUT;

    // rir_source
    if (json.contains(QStringLiteral("rir_source"))) {
        const QString &rirsource_str = json["rir_source"].toString();
        if (rirsource_str == QStringLiteral("NoRIR"))
            rir_source = NoRIR;
        else if (rirsource_str == QStringLiteral("Literal"))
            rir_source = Literal;
        else if (rirsource_str == QStringLiteral("File"))
            rir_source = File;
        else
            throw SceneJsonInvtokenException(
                    "in rir_source",
                    "one of {None,MAT,WAV}",
                    rirsource_str.toUtf8().constData());
    }
    else
        rir_source = DEFAULT_SOURCE;

    // rir_file
    if (json.contains(QStringLiteral("rir_file")))
        rir_file = json["rir_file"].toString();
    else
        rir_file = QStringLiteral("");

    // system_latency
    if (json.contains(QStringLiteral("system_latency"))) {
        int json_syslatency = json["system_latency"].toInt();
        if (json_syslatency < 0 || json_syslatency > delay_max - min_delay)
            throw SceneJsonOOBException("system_latency", json_syslatency,
                                        0, delay_max - min_delay);
        system_latency = json_syslatency;
    }
    else
        system_latency = DEFAULT_SYSLATENCY;

    // delay
    if (json.contains(QStringLiteral("delay"))) {
        int json_delay = json["delay"].toInt();
        int delay_min = system_latency + min_delay;
        if (json_delay < delay_min || json_delay > delay_max)
            throw SceneJsonOOBException("delay", json_delay,
                                        delay_min, delay_max);
        delay = json_delay;
    }
    else
        delay = DEFAULT_DELAY;

    // volume
    if (json.contains(QStringLiteral("volume"))) {
        int json_vol = json["volume"].toInt();
        if (json_vol < 0 || json_vol > 100)
            throw SceneJsonOOBException("volume", json_vol,
                                        0, 100);
        volume = json_vol;
    }
    else
        volume = DEFAULT_VOLUME;

    // rir_filetype & imp_resp
    switch (rir_source) {
    case NoRIR:
        if (json.contains(QStringLiteral("imp_resp"))) {
            QJsonArray json_impresp = json["imp_resp"].toArray();
            if (json_impresp.size() != 1 || json_impresp[0].toDouble() != 1.0)
                throw SceneJsonInvfieldException(
                        "imp_resp", "this field should not exist, or else "
                                    " should be the list [1].");
        }
        rir_filetype = None;
        imp_resp = container_t(1,1);
        break;
    case Literal:
        if (json.contains(QStringLiteral("imp_resp"))) {
            QJsonArray json_impresp = json["imp_resp"].toArray();
            imp_resp = container_t();
            for (auto json_sample : json_impresp) {
                if (!json_sample.isDouble())
                    throw SceneJsonInvfieldException(
                            "imp_resp", "this field should be a list of"
                                        " doubles.");
                imp_resp.push_back(json_sample.toDouble());
            }
            rir_filetype = None;
        }
        else
            throw SceneJsonInvfieldException(
                    "imp_resp", "this field should exist.");
        break;
    case File:
        if (json.contains(QStringLiteral("imp_resp")))
            throw SceneJsonInvfieldException(
                    "imp_resp", "this field should not exist.");
        else {
            QRegExp rx_m  ("*.m",   Qt::CaseInsensitive, QRegExp::Wildcard);
            QRegExp rx_wav("*.wav", Qt::CaseInsensitive, QRegExp::Wildcard);
            if (rx_m.exactMatch(rir_file))
                rir_filetype = MAT;
            else if (rx_wav.exactMatch(rir_file))
                rir_filetype = WAV;
            else
                throw RIRInvalidException("RIR file must be *.m or *.wav.");
            if (rir_filetype == MAT) {
                QFile file(rir_file);
                if (!file.open(QIODevice::ReadOnly))
                    throw RIRInvalidException("Error opening RIR file.");
                imp_resp = ChangeRIRDialog::parse_txt(
                            file.readAll().constData());
            }
            else {
                try {
                    Signal s(rir_file.toUtf8().constData());
                    imp_resp = container_t(s.array(), s.array()+s.samples());
                }
                catch (const FileError&) {
                    throw RIRInvalidException("Error opening file.");
                }
            }
            if (imp_resp.size() >= fft_size - blk_size)
                // A convolução de 'h' com um bloco deve caber em uma fft
                throw RIRInvalidException(
                        std::string("RIR pode ter no máximo ") +
                        std::to_string(fft_size - blk_size) +
                        std::string(" amostras."));
        }
        break;
    default:
        throw std::runtime_error("Unknown error!!!!");
    }

    // adapf_file
    if (json.contains(QStringLiteral("adapf_file")))
        adapf_file = json["adapf_file"].toString().toUtf8().constData();
    else
        adapf_file = AdaptiveFilter<sample_t>().get_path();

}

void Scene::save_to_file() const {
    throw std::runtime_error("Not implemented.");
}

void Scene::write_to_json(QJsonObject &json) {
    throw std::runtime_error("Not implemented.");
}
