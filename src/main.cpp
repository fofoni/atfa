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
 * \file main.cpp
 *
 * Holds the `main()` function and other routines.
 *
 * \author Pedro Angelo Medeiros Fonini
 */

#include <iostream>

extern "C" {
#   include <portaudio.h>
}

#include "Stream.h"
#include "utils.h"

using namespace std;

/// `main()` function.
/**
 * No command-line parameters.
 *
 * This function:
 * 1. Prints version info
 *
 * \param[in] argc      argument count (unused)
 * \param[in] argv      argument values (unused)
 * \returns 0 if no errors
 */
int main(int argc, char *argv[]) {

    cout << "ATFA " << ATFA_VERSION << "." << endl;
#ifdef ATFA_DEBUG
    cout << ">>> This is a Debug build" << endl;
#endif
    cout << "Using " << Pa_GetVersionText() << "." << endl;
    cout << endl;

    Stream s;

    Stream::container_t h(16, 1.0);
    for (Stream::index_t k=0; k!=16; k+=2)
        h[k].sample *= 1;
    s.set_filter(h);

#ifndef ATFA_DEBUG
    portaudio_init();
    s.echo(300);
    portaudio_end();
#else
    s.simulate();
#endif

    cout << "Finishing..." << endl;

    return 0;

}
