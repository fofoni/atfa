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
    cout << ">>> THIS IS A DEBUG BUILD" << endl;
#endif
    cout << "Using " << Pa_GetVersionText() << "." << endl;
    cout << endl;

    Stream<8,1000> s;

    s.write(0);
    s.write(1);
    s.write(1);
    cout << s.read() << endl;
    s.write(2);
    s.write(3);
    cout << s.read() << endl;
    s.write(5);
    cout << s.read() << endl;
    cout << s.read() << endl;
    s.write(8);
    cout << s.read() << endl;
    s.write(13);
    cout << s.read() << endl;
    cout << s.read() << endl;
    cout << s.read() << endl;
    cout << s.read() << endl;
    cout << s.read() << endl;
    cout << s.read() << endl;
    s.write(100);
    s.write(100);
    s.write(100);
    s.write(100);
    cout << s.read() << endl;
    cout << s.read() << endl;

    cout << "Finishing..." << endl;

    return 0;

}
