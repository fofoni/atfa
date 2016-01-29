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

    /*

    Stream::container_t h(1024, 0);
    for (Stream::index_t k=0; k!=64; k+=2)
        h[k].sample = 1;
    for (Stream::index_t k=960; k!=1024; k+=2)
        h[k].sample = (k%2) ? .25 : -.25;

    Stream::Scenario scene(h);
    scene.delay = 300;

    Stream s(scene);

#ifndef ATFA_DEBUG
    portaudio_init();
    s.echo();
    portaudio_end();
#else
    s.simulate();
#endif

//    */

    return 0;

}
