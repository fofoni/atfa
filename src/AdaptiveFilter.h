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

#include <string>
#include <sstream>
#include <stdexcept>

template <typename SAMPLE_T>
class AdaptiveFilter
{

public:

    // adaptive filter {init, close, run, restart,
    //                  get w, get title, get listing}
    typedef void *(*afi_t)(void);
    typedef int (*afc_t)(void *);
    typedef SAMPLE_T (*afr_t)(void *, SAMPLE_T, SAMPLE_T, int);
    typedef void *(*afz_t)(void *);
#ifdef ATFA_LOG_MATLAB
    typedef void (*afw_t)(void *, SAMPLE_T **, unsigned *);
#endif
    typedef const char *(*aft_t)(void);
    typedef const char *(*afl_t)(void);

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

    bool is_dummy() {
        return dummy;
    }

    std::string get_path() {
        return path;
    }

    const char *get_title() {
        return title_str.c_str();
    }

    const char *get_listing() {
        return listing_str.c_str();
    }

    static SAMPLE_T placeholder;

    void reset_state() {
        data = (*restart)(data);
    }

private:

    bool dummy;

    template <typename SYM_T>
    SYM_T get_sym(std::string sym_name);

    // TODO: fazer função raise(string) para embutir o path e o dlerror
    // no AdapfException (com opção p não mostrar dlerror)

    std::string path;
    std::string title_str;
    std::string listing_str;

    void *lib;

    afi_t init;
    afc_t close;
    afr_t run;
    afz_t restart;
#ifdef ATFA_LOG_MATLAB
    afw_t getw;
#endif
    aft_t title;
    afl_t listing;

    void *data;

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
SYM_T AdaptiveFilter<SAMPLE_T>::get_sym(std::string sym_name) {
    std::string full_sym_name = std::string("adapf_") + sym_name;
    SYM_T sym = reinterpret_cast<SYM_T>(dlsym(lib, full_sym_name.c_str()));
    if (!sym)
        throw AdapfException(
                std::string("Could not open ") + full_sym_name
                + " symbol from shared object file",
                path, dlerror());
    return sym;
}

#endif // ADAPTIVEFILTER_H
