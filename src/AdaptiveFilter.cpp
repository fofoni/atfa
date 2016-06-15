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
AdaptiveFilter<SAMPLE_T>::AdaptiveFilter(std::string dso_path)
  : path(dso_path), data(nullptr)
{

    lib = dlopen(path.c_str(), RTLD_NOW);
    if (!lib)
        throw AdapfException(
                "Could not open shared object file",
                path, dlerror());

    init = get_sym<afi_t>("init");
    close = get_sym<afc_t>("close");
    run = get_sym<afr_t>("run");
    restart = get_sym<aft_t>("restart");

    test();

}

template <typename SAMPLE_T>
AdaptiveFilter<SAMPLE_T>::~AdaptiveFilter()
{
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
    data = (*init)();
    if (!data)
        throw AdapfException(
                "Could not initialize adaptive filter data structures",
                path, dlerror());
}

// TODO: deve ser noexcept, ou throw(), etc, pq é chamado de dentro do destrutor
template <typename SAMPLE_T>
void AdaptiveFilter<SAMPLE_T>::destroy_data_structures() {
    if (!(*close)(data))
        std::cerr << "[Adaptive Filter] Warning: could not close adaptive "
                  << "filter data structures." << std::endl
                  << "[Adaptive Filter] DSO path: " << path << std::endl;
    data = nullptr;
}

template <typename SAMPLE_T>
void AdaptiveFilter<SAMPLE_T>::test() {
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

template class AdaptiveFilter<float>;
