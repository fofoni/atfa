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
 * Arquivo mêim ponto cê pê pê
 *
 * \author Pedro Angelo Medeiros Fonini
 */

/**
 * Version string of the project.
 */
static const char *version = "0.0.1";

#include <iostream>

using namespace std;

/**
 * Função irada que dá o sinal da variável.
 * \param[in] val       val se fuder
 * \param[in] zero      zero?
 * \returns to the caller
 */
template typename<T>
int sgn(T val, int zero=1) {
    return zero*(T(0) == val) + (T(0) < val) - (val < T(0));
}

namespace funcfunc {

    /**
     * função zoada um
     */
    int func1(float x, int y) {
        x *= sgn(x);
        return x + y;
    }

    /**
     * função muito zoada Dois
     */
    int func2(int x=4) {
        return 2*x;
    }

}

namespace foncfonc {

    /**
     * Classe zoada Um
     */
    class A {
      public:
        int x; ///< parâmetro escroto
        A() : x(5) {};
    }

}

/**
 * função main ué
 * \param[in] argc      qtde de paradas
 * \param[in] argv      paradas
 * \returns to the operational system
 */
int main(int argc, char *argv[]) {

    float x = 8.3;
    int y = 10;

    funcfunc::func1(x,y);

    return 0;

}
