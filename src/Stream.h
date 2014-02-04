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

    struct sample_wrapper_t {
        mutable int count;
        sample_t sample;
        sample_wrapper_t() : count(0), sample(0) {}
    };

    /// The type for holding the whole vector of signal samples.
    typedef std::vector<sample_wrapper_t> container_t;

    /* TODO: testar a classe do jeito que está, no portaudio. Tentar dar um
       jeito de monitorara corrida entre o write e o read (grava a distancia
       em funcao do tempo num array, e depois bota num arquivo de maneira que
       seja facil de plotar no matlab). Se estiver ruim de performance, tenta
       fazer funcoes pra ler e escrever blocos inteiros de uma vez. */

    // TODO: could be an operator>>
    sample_t read() {
        read_ptr->count -= 1;
        sample_t s = read_ptr->count ? 0 : read_ptr->sample;
        ++read_ptr;
        if (read_ptr == data.end()) read_ptr = data.begin();
        return s;
    }

    // TODO: could be an operator<<
    void write(sample_t s) {
        write_ptr->count += 1;
        write_ptr->sample = s;
        write_ptr++;
        if (write_ptr == data.end()) write_ptr = data.begin();
    }

    int bad_to_read() {
        return read_ptr->count - 1;
    }

    int bad_to_write() {
        return write_ptr->count;
    }

    Stream()
        : data(buf_size), write_ptr(data.begin()), read_ptr(data.begin())
    {
#ifdef ATFA_DEBUG
        if (buf_size == 0) throw std::runtime_error("Stream: Bad buf_size");
        if (srate == 0) throw std::runtime_error("Stream: Bad srate");
#endif
    }

private:

    container_t data;
    typename container_t::iterator write_ptr;
    typename container_t::const_iterator read_ptr;

};

#endif // STREAM_H
