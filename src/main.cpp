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
 * Holds the `main()` function.
 *
 * \author Pedro Angelo Medeiros Fonini
 */

#include <iostream>

#include <QtGui>

extern "C" {
#   include <portaudio.h>
}

#include "utils.h" // TODO: deletar isso
#include "Stream.h" // TODO: deletar isso

#include "ATFA.h"

using namespace std;

/// `main()` function.
/**
 * No command-line parameters.
 *
 * This function:
 * 1. Prints version info
 * 2. Creates an i/o stream to represent the communication channel with echo
 * 3. Creates a room impulse response
 * 4. Assigns the created impulse response to the stream
 * 5. Assigns a value of 300ms to the stream's delay echo
 * 6. Runs the stream
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


#ifndef ATFA_DEBUG
    QApplication app(argc, argv);

    ATFA window;
    window.resize(window.sizeHint());

    QRect r = window.geometry();
    r.moveCenter(QApplication::desktop()->availableGeometry().center());
    window.setGeometry(r);

    window.show();

    return app.exec();
#else
    Stream s;
    s.simulate();

    return 0;
#endif

}
