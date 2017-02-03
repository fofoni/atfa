/*
 * Universidade Federal do Rio de Janeiro
 * Escola Politécnica
 * Projeto Final de Graduação
 * Ambiente de Teste para Filtros Adaptativos
 * Pedro Angelo Medeiros Fonini
 * Orientador: Markus Lima
 */

/**
 *
 * \file VAD.cpp
 *
 * Holds some VAD (Voice Activity Detector) implementations.
 * \author Pedro Angelo Medeiros Fonini
 */

#include <cmath>
#include <vector>

#include "VAD.h"

constexpr double VAD_COEF_1 = -1.005079894781262e-4;
constexpr double VAD_COEF_0 =  1.182502528634821e-2;

bool vad_hard(std::vector<float>::const_iterator first,
              std::vector<float>::const_iterator last) {
    unsigned zero_crossing = 0;
    double power = 0;
    if (first == last)
        return false;
    auto sample = *first;
    int state = (sample > 0)*2 - 1;
    power = sample*sample;
    unsigned num_samples = 1;
    for (; first != last; ++first) {
        sample = *first;
        if (sample * state < 0) {
            state *= -1;
            ++zero_crossing;
        }
        power += sample*sample;
        ++num_samples;
    }
    power = std::sqrt(power/num_samples);
    return (power > VAD_COEF_1*zero_crossing + VAD_COEF_0 +.002)
           && ((zero_crossing <= 20) || (zero_crossing >= 60));
}

bool vad_soft(std::vector<float>::const_iterator first,
              std::vector<float>::const_iterator last) {
    unsigned zero_crossing = 0;
    double power = 0;
    if (first == last)
        return false;
    auto sample = *first;
    int state = (sample > 0)*2 - 1;
    power = sample*sample;
    unsigned num_samples = 1;
    for (; first != last; ++first) {
        sample = *first;
        if (sample * state < 0) {
            state *= -1;
            ++zero_crossing;
        }
        power += sample*sample;
        ++num_samples;
    }
    power = std::sqrt(power/num_samples);
    return (power > VAD_COEF_1*zero_crossing + VAD_COEF_0-0.002)
           && ((zero_crossing <= 25) || (zero_crossing >= 55));
}
