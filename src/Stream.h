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
 * Holds the interface to the `Stream` class.
 *
 * \author Pedro Angelo Medeiros Fonini
 */

#include <vector>
#include <stdexcept>

#ifndef STREAM_H
#define STREAM_H

template <size_t buf_size, unsigned srate>
class Stream
{

public:
    /// The type for holding each signal sample.
    typedef float sample_t;

    /// The type for holding each signal sample index.
    typedef unsigned long index_t;

    /// The type for holding the whole vector of signal samples.
    typedef std::vector<sample_t> container_t;

    typedef std::vector<bool> flag_cter_t;

    Stream()
        : data(buf_size), flags(buf_size), write_ptr(&data[0]),
          read_ptr(&data[0])
    {
#ifdef ATFA_DEBUG
        if (buf_size == 0) throw std::runtime_error("Stream: Bad buf_size");
        if (srate == 0) throw std::runtime_error("Stream: Bad srate");
#endif
    }

private:
    container_t data;
    flag_cter_t flags;
    sample_t *write_ptr;
    const sample_t *read_ptr;

};

#endif // STREAM_H
