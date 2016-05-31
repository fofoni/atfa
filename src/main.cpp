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

#include "utils.h"
#include "Stream.h" // TODO: deletar isso

#include "ATFA.h"

using namespace std;

/// `main()` function.
/**
 * No command-line parameters.
 *
 * This function runs the "ATFA" Qt app. ATFA stands for "Ambiente de testes
 * para filtros adaptativos".
 *
 * \param[in] argc      command line argument count
 * \param[in] argv      command line argument values
 * \returns 0 if no errors
 */
int main(int argc, char *argv[]) {

    cout << "ATFA " << ATFA_VERSION << "." << endl;
#ifdef ATFA_DEBUG
    cout << ">>> This is a Debug build" << endl;
#endif
    cout << "Using " << Pa_GetVersionText() << "." << endl;
    cout << endl;

    QApplication app(argc, argv);

    ATFA window;
    window.resize(window.sizeHint());

    QRect r = window.geometry();
    r.moveCenter(QApplication::desktop()->availableGeometry().center());
    window.setGeometry(r);

    window.show();

    return app.exec();

}
