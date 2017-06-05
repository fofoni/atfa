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
#include <random>
#include <utility>

#include "AdaptiveFilter.h"
#include "Signal.h"

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
                     const Signal& input, const Signal& imp_resp, int noise,
                     int pro=DEFAULT_PROLOGUE, int epi=DEFAULT_EPILOGUE)
        : adapf_(af), prologue_(pro), epilogue_(epi),
          input_(input), N(input_.samples()), output_{input_}
    {
        output_.filter(imp_resp);
        Signal::container_t awgn(output_.samples()); // deveria ser Signal
        std::mt19937 rng;
        std::normal_distribution<> gauss{0, std::pow(10,noise/20)};
        for (auto& x : awgn) x = gauss(rng);
        output_ = output_ + Signal{awgn}; // TODO operator+= para o Signal
        if (prologue_ > static_cast<int>(input_.samples()))
            prologue_ = static_cast<int>(input_.samples());
        if (epilogue_ > static_cast<int>(input_.samples()))
            epilogue_ = static_cast<int>(input_.samples());
    }

    template <int learn>
    std::pair<std::chrono::duration<double>, int> benchmark();

private:

    AdaptiveFilter<SAMPLE_T>& adapf_;
    int prologue_, epilogue_;

    Signal input_;

    int N;
    Signal output_;

};

template <typename SAMPLE_T>
template <int learn>
std::pair<std::chrono::duration<double>, int>
AdapfBenchmarker<SAMPLE_T>::benchmark() {

    adapf_.initialize_data_structures();

    auto input_ptr = input_.array();
    auto output_ptr = output_.array();

    for (int i = 0; i < prologue_; ++i)
        adapf_.get_sample(*input_ptr++, *output_ptr++, learn);

    input_ptr = input_.array();
    output_ptr = output_.array();
    adapf_.reset_nup();

    auto start_time = std::chrono::steady_clock::now();
    for (int i = 0; i < N; ++i)
        adapf_.get_sample(*input_ptr++, *output_ptr++, learn);
    auto end_time = std::chrono::steady_clock::now();

    input_ptr = input_.array();
    output_ptr = output_.array();

    for (int i = 0; i < epilogue_; ++i)
        adapf_.get_sample(*input_ptr++, *output_ptr++, learn);

    adapf_.destroy_data_structures();

    std::chrono::duration<double> total_time = end_time - start_time;

    return {total_time / double(N), adapf_.number_of_updates()};

}

#endif // ADAPFBENCHMARKER_H
