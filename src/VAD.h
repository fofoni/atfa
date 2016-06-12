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
 * \file Stream.h
 *
 * Holds the interfaces to VAD functions.
 *
 * \author Pedro Angelo Medeiros Fonini
 */

#ifndef VAD_H
#define VAD_H

#include <vector>

bool vad_hard(std::vector<float>::const_iterator first,
              std::vector<float>::const_iterator last);

bool vad_soft(std::vector<float>::const_iterator first,
              std::vector<float>::const_iterator last);

#endif // STREAM_H
