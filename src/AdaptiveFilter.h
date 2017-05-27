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
 * \file AdaptiveFilter.h
 *
 * Holds the interface to the `AdaptiveFilter` class.
 *
 * \author Pedro Angelo Medeiros Fonini
 */

#ifndef ADAPTIVEFILTER_H
#define ADAPTIVEFILTER_H

extern "C" {
#include <dlfcn.h>
}

#include <string>
#include <sstream>
#include <stdexcept>

#include "atfa_api.h"

template <typename SAMPLE_T>
class AdaptiveFilter
{

public:

    AdaptiveFilter(std::string dso_path);
    AdaptiveFilter();
    ~AdaptiveFilter();

    void test();

    void initialize_data_structures();
    void destroy_data_structures();

    SAMPLE_T get_sample(SAMPLE_T x, SAMPLE_T y, int learn) {
        return (*run)(data, x, y, learn);
    }

#ifdef ATFA_LOG_MATLAB
    void get_impresp(SAMPLE_T **begin, unsigned *n) {
        (*getw)(data, begin, n);
    }
#endif

    bool is_dummy() const {
        return dummy;
    }

    std::string get_path() const {
        return path;
    }

    const char *get_title() const {
        return title_str.c_str();
    }

    const char *get_listing() const {
        return listing_str.c_str();
    }

    static SAMPLE_T placeholder;

    void reset_state() {
        data = (*restart)(data);
    }

private:

    bool dummy;

    template <typename SYM_T>
    SYM_T *get_sym(const char *sym_name);

    // TODO: fazer função raise(string) para embutir o path e o dlerror
    // no AdapfException (com opção p não mostrar dlerror)

    std::string path;
    std::string title_str;
    std::string listing_str;

    void *lib;

    adapf_init_t *init;
    adapf_restart_t *restart;
    adapf_close_t *close;
    adapf_run_t *run;
#ifdef ATFA_LOG_MATLAB
    adapf_getw_t *getw;
#endif
    adapf_title_t *title;
    adapf_listing_t *listing;

    AdapfData *data;

    void make_dummy();

};


class AdapfException: public std::runtime_error {

public:

    AdapfException(const std::string& desc,
                   const std::string& dso_path,
                   const std::string& dlerr)
      : runtime_error(desc), description(desc), dlerror(dlerr), path(dso_path),
        what_msg(
            std::string(runtime_error::what()) + ". DSO: " + path
            + " . DL error: " + dlerror
        )
    {
    }

    AdapfException(const std::string& desc,
                   const std::string& dso_path)
      : runtime_error(desc), description(desc), dlerror(""), path(dso_path),
        what_msg(
            std::string(runtime_error::what()) + ". DSO: " + path + " ."
        )
    {
    }

    virtual const char* what() const throw() {
        return what_msg.c_str();
    }

private:
    std::string description;
    std::string dlerror;
    std::string path;
    std::string what_msg;

};


template <typename SAMPLE_T>
template <typename SYM_T>
SYM_T *
AdaptiveFilter<SAMPLE_T>::get_sym(const char *sym_name) {
    SYM_T *sym = static_cast<SYM_T *>(dlsym(lib, sym_name));
    if (!sym)
        throw AdapfException(
                std::string("Could not open ") + sym_name
                + " symbol from shared object file",
                path, dlerror());
    return sym;
}

#endif // ADAPTIVEFILTER_H
