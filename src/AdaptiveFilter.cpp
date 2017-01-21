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

extern "C" {
# include <dlfcn.h>
}

#include <stdexcept>
#include <iostream>

#include "AdaptiveFilter.h"

template <typename SAMPLE_T>
SAMPLE_T AdaptiveFilter<SAMPLE_T>::placeholder = 0;

template <typename SAMPLE_T>
AdaptiveFilter<SAMPLE_T>::AdaptiveFilter(std::string dso_path)
  : dummy(false), path(dso_path), data(nullptr)
{

    lib = dlopen(path.c_str(), RTLD_NOW);
    if (!lib)
        throw AdapfException(
                "Could not open shared object file",
                path, dlerror());

    init = get_sym<afi_t>("init");
    close = get_sym<afc_t>("close");
    run = get_sym<afr_t>("run");
    restart = get_sym<afz_t>("restart");
    getw = get_sym<afw_t>("getw");
    title = get_sym<aft_t>("title");
    listing = get_sym<afl_t>("listing");

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
    void *dat = (*init)();
    if (!dat)
        throw AdapfException(
                "Could not initialize adaptive filter data structures,"
                " during testing",
                path, dlerror());
    (*run)(dat, SAMPLE_T(0.1), SAMPLE_T(0.2));
    (*run)(dat, SAMPLE_T(0.3), SAMPLE_T(0.4));
    (*run)(dat, SAMPLE_T(0.5), SAMPLE_T(0.6));
    dat = (*restart)(dat);
    if (!dat)
        throw AdapfException(
                "Could not restart adaptive filter data structures"
                " during testing",
                path, dlerror());
    (*run)(dat, SAMPLE_T(0.7), SAMPLE_T(0.8));
    if (!(*close)(dat))
        throw AdapfException(
                "Error while closing adaptive filter data structures"
                " during testing",
                path, dlerror());
}

/* DUMMY (NO OP) FILTER */

void *dummy_init() { return nullptr; }
int dummy_close(void *) { return 1; }
void *dummy_restart(void *) { return nullptr; }
template <typename SAMPLE_T>
SAMPLE_T dummy_run(void *, SAMPLE_T, SAMPLE_T y) { return y; }
template <typename SAMPLE_T>
void dummy_getw(void *, SAMPLE_T **begin, unsigned *n) {
    // *begin will be used as source in a call to std::memcpy.
    // Apparently, memcpy invokes undefined behaviour when it gets
    // called with invalid pointer (e.g. nullptr) arguments, EVEN
    // if n==0.
    // This means that *begin should point to somewhere ok -- it
    // just doesn't matter where.
    *begin = &AdaptiveFilter<SAMPLE_T>::placeholder;
    *n = 0;
}

template <typename SAMPLE_T>
AdaptiveFilter<SAMPLE_T>::AdaptiveFilter()
  : dummy(true), data(nullptr)
{

    lib = nullptr;

    init = &dummy_init;
    close = &dummy_close;
    run = &dummy_run<SAMPLE_T>;
    restart = &dummy_restart;
    getw = &dummy_getw<SAMPLE_T>;

}

/* Explicit template instantiation for use with `Stream` */

template class AdaptiveFilter<float>;
