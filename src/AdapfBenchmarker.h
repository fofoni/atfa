/*
 * Universidade Federal do Rio de Janeiro
 * Escola Politécnica
 * Projeto Final de Graduação
 * Ambiente de Teste para Filtros Adaptativos
 * Pedro Angelo Medeiros Fonini
 * Orientador: Markus Lima
 */

#ifndef ADAPFBENCHMARKER_H
#define ADAPFBENCHMARKER_H

#include <chrono>

#include "AdaptiveFilter.h"

template <typename SAMPLE_T>
class AdapfBenchmarker
{
public:

    constexpr static int DEFAULT_PROLOGUE = 50;
    int prologue() const { return prologue_; }
    void set_prologue(int pro) { prologue_ = pro; }

    constexpr static int DEFAULT_EPILOGUE = DEFAULT_PROLOGUE;
    int epilogue() const { return epilogue_; }
    void set_epilogue(int epi) { epilogue_ = epi; }

    void set_margins(int margins) { prologue_ = epilogue_ = margins; }

    AdapfBenchmarker(AdaptiveFilter<SAMPLE_T>& af,
                     int pro=DEFAULT_PROLOGUE, int epi=DEFAULT_EPILOGUE)
        : adapf_(af), prologue_(pro), epilogue_(epi) {}

    template <bool learn>
    std::chrono::duration<double> benchmark(int N);

private:

    AdaptiveFilter<SAMPLE_T>& adapf_;
    int prologue_, epilogue_;

};

template <typename SAMPLE_T>
template <bool learn>
std::chrono::duration<double> AdapfBenchmarker<SAMPLE_T>::benchmark(int N) {

    adapf_.initialize_data_structures();

    for (int i = 0; i < prologue_; ++i)
        adapf_.get_sample(SAMPLE_T{}, SAMPLE_T{}, learn);

    auto start_time = std::chrono::steady_clock::now();
    for (int i = 0; i < N; ++i)
        adapf_.get_sample(SAMPLE_T{}, SAMPLE_T{}, learn);
    auto end_time = std::chrono::steady_clock::now();

    for (int i = 0; i < epilogue_; ++i)
        adapf_.get_sample(SAMPLE_T{}, SAMPLE_T{}, learn);

    adapf_.destroy_data_structures();

    std::chrono::duration<double> total_time = end_time - start_time;

    return total_time / double(N);

}

#endif // ADAPFBENCHMARKER_H
