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

/// \todo fazer o canal. meio que ta feito, mas nao funciona. Aproveitar, e
///       escrever um framework pra simular o portaudio, e ver o resultado.

class Stream
{

public:
    /// The type for holding each signal sample.
    typedef float sample_t;

    /// The type for holding each signal sample index.
    typedef unsigned long index_t;

    struct sample_wrapper_t {
//        mutable int count;
        sample_t sample;
        sample_wrapper_t() : sample(0) {}
    };

    /// The type for holding the whole vector of signal samples.
    typedef std::vector<sample_wrapper_t> container_t;

    static const size_t buf_size = 88200; // 20
    static const unsigned samplerate = 11025; // 1

    // TODO: could be an operator>>
    sample_t read() {
        sample_t s = read_ptr->sample;
        ++read_ptr;
        if (static_cast<size_t>(read_ptr - data.begin()) == buf_size)
            read_ptr = data.begin();
        return s;
    }

    container_t::const_iterator get_last_n(index_t n) {
        ++read_ptr;
        container_t::const_iterator p = read_ptr + buf_size - n;
        if (static_cast<size_t>(read_ptr - data.begin()) == buf_size)
            read_ptr = data.begin();
        return p;
    }

    // TODO: could be an operator<<
    void write(sample_t s) {
        write_ptr->sample = s;
        (write_ptr + buf_size)->sample = s;
        ++write_ptr;
        if (static_cast<size_t>(write_ptr - data.begin()) == buf_size)
            write_ptr = data.begin();
    }

    Stream()
        : delay_samples(0), data(2*buf_size),
          write_ptr(data.begin()), read_ptr(data.begin())
    {
#ifdef ATFA_DEBUG
        if (buf_size == 0) throw std::runtime_error("Stream: Bad buf_size");
        if (srate == 0) throw std::runtime_error("Stream: Bad srate");
#endif
    }

    sample_t& operator [](index_t index) {
#ifdef ATFA_DEBUG
        if (index >= samples()) throw std::runtime_error("Range error!!");
#endif
        return data[index].sample;
    }

    const sample_t& operator [](index_t index) const {
#ifdef ATFA_DEBUG
        if (index >= samples()) throw std::runtime_error("Range error!!");
#endif
        return data[index].sample;
    }

    void echo(unsigned delay = 0, unsigned sleep = 0);

    void set_delay(unsigned delay) {
        delay_samples = samplerate * delay/1000;
        if (delay_samples <= static_cast<size_t>(write_ptr - data.begin()))
            read_ptr = write_ptr - delay_samples;
        else
            // We won't ckeck, for performance, that delay_samples <= data.size
            // The application must enforce this.
            read_ptr = write_ptr + (buf_size - delay_samples);
    }

private:

    index_t delay_samples;

    container_t data;
    typename container_t::iterator write_ptr;
    typename container_t::iterator read_ptr;

};

#endif // STREAM_H
