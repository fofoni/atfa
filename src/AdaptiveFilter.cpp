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
 * \file AdaptiveFilter.cpp
 *
 * Holds the AdaptiveFilter class, which is an interface to dynamic
 * shared object files (*.so) implementing adaptive filtering
 * algorithms.
 *
 * \author Pedro Angelo Medeiros Fonini
 */

#include <stdexcept>
#include <iostream>

#include "AdaptiveFilter.h"

#ifdef ATFA_LOG_MATLAB
template <typename SAMPLE_T>
SAMPLE_T AdaptiveFilter<SAMPLE_T>::placeholder = 0;
#endif

template <typename SAMPLE_T>
AdaptiveFilter<SAMPLE_T>::AdaptiveFilter(std::string dso_path)
  : path(dso_path), data(nullptr), num_of_updates{}
{

    dummy = dso_path.length()==0;

    if (dummy) {
        make_dummy();
        return;
    }

    lib = dlopen(path.c_str(), RTLD_NOW);
    if (!lib)
        throw AdapfException(
                "Could not open shared object file",
                path, dlerror());

    ATFA_API_table_t *api = get_sym<ATFA_API_table_t>("adapf_api");

    init = api->init;
    close = api->close;
    run = api->run;
    restart = api->restart;
#ifdef ATFA_LOG_MATLAB
    getw = api->getw;
#endif
    title = api->title;
    listing = api->listing;

    test();

    title_str = (*title)();
    listing_str = (*listing)();

}

template <typename SAMPLE_T>
AdaptiveFilter<SAMPLE_T>::~AdaptiveFilter()
{
    if (dummy)
        return;
    if (dlclose(lib) != 0)
        std::cerr << "[Adaptive Filter DSO] Warning: could not close dynamic "
                  << "shared object." << std::endl
                  << "[Adaptive Filter DSO] DSO path: " << path << std::endl
                  << "[Adaptive Filter DSO] DL error: " << dlerror()
                  << std::endl;
    if (data)
        destroy_data_structures();
}

template <typename SAMPLE_T>
void AdaptiveFilter<SAMPLE_T>::initialize_data_structures() {
    if (dummy)
        return;
    data = (*init)();
    if (!data)
        throw AdapfException(
                "Could not initialize adaptive filter data structures",
                path, dlerror());
}

// TODO: deve ser noexcept, ou throw(), etc, pq é chamado de dentro do destrutor
//       UPDATE: mentira. Deve jogar uma exceção se der merda sim. MAS essa
//               exceção deve ser pega dentro do construtor
template <typename SAMPLE_T>
void AdaptiveFilter<SAMPLE_T>::destroy_data_structures() {
    if (dummy)
        return;
    if (!(*close)(data))
        std::cerr << "[Adaptive Filter] Warning: could not close adaptive "
                  << "filter data structures." << std::endl
                  << "[Adaptive Filter] DSO path: " << path << std::endl;
    data = nullptr;
}

template <typename SAMPLE_T>
void AdaptiveFilter<SAMPLE_T>::test() {
    if (dummy)
        return;
    AdapfData *dat = (*init)();
    if (!dat)
        throw AdapfException(
                "Could not initialize adaptive filter data structures,"
                " during testing",
                path, dlerror());
    int placeholder;
    (*run)(dat, SAMPLE_T(0.1), SAMPLE_T(0.2), 0, &placeholder);
    (*run)(dat, SAMPLE_T(0.3), SAMPLE_T(0.4), 0, &placeholder);
    (*run)(dat, SAMPLE_T(0.5), SAMPLE_T(0.6), 1, &placeholder);
    dat = (*restart)(dat);
    if (!dat)
        throw AdapfException(
                "Could not restart adaptive filter data structures"
                " during testing",
                path, dlerror());
    (*run)(dat, SAMPLE_T(0.7), SAMPLE_T(0.8), 1, &placeholder);
    if (!(*close)(dat))
        throw AdapfException(
                "Error while closing adaptive filter data structures"
                " during testing",
                path, dlerror());
}

/* DUMMY (NO OP) FILTER */

// TODO: essas funções aqui deveriam ter linkage interno (anonymous namespace)
AdapfData *dummy_init() { return nullptr; }
int dummy_close(AdapfData *) { return 1; }
AdapfData *dummy_restart(AdapfData *) { return nullptr; }
template <typename SAMPLE_T>
SAMPLE_T dummy_run(AdapfData *, SAMPLE_T, SAMPLE_T y, int, int* updated)
{ *updated = 0; return y; }
#ifdef ATFA_LOG_MATLAB
template <typename SAMPLE_T>
void dummy_getw(const AdapfData *, const SAMPLE_T **begin, unsigned *n) {
    // *begin will be used as source in a call to std::memcpy.
    // Apparently, memcpy invokes undefined behaviour when it gets
    // called with invalid pointer (e.g. nullptr) arguments, EVEN
    // if n==0.
    // This means that *begin should point to somewhere ok -- it
    // just doesn't matter where.
    *begin = &AdaptiveFilter<SAMPLE_T>::placeholder;
    *n = 0;
}
#endif

template <typename SAMPLE_T>
void AdaptiveFilter<SAMPLE_T>::make_dummy() {
    lib = nullptr;
    init = &dummy_init;
    close = &dummy_close;
    run = &dummy_run<SAMPLE_T>;
    restart = &dummy_restart;
#ifdef ATFA_LOG_MATLAB
    getw = &dummy_getw<SAMPLE_T>;
#endif
}

template <typename SAMPLE_T>
AdaptiveFilter<SAMPLE_T>::AdaptiveFilter()
  : dummy(true), path(""), data(nullptr), num_of_updates{}
{
    make_dummy();
}

/* Explicit template instantiation for use with `Stream` */

template class AdaptiveFilter<float>;
