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

#ifndef STREAM_H
#define STREAM_H

#include <vector>
#include <stdexcept>
#include <numeric>

/// Represents an input/output stream of audio samples
/**
  * Holds data and provides routines for dealing with streams that represent
  * communication systems with echo. Currently, all Streams are implemented as a
  * circular array of single-precision floating-point samples. Streams are aware
  * of their sample rates.
  */
class Stream
{

public:
    /// The type for holding each signal sample.
    typedef float sample_t;

    /// The type for holding each signal sample index.
    typedef unsigned long index_t;

    /// A structured type for holding a single sample. Will be simplified later.
    struct sample_wrapper_t {
//        mutable int count;
        sample_t sample;
        sample_wrapper_t() : sample(0) {}
        sample_wrapper_t(sample_t s) : sample(s) {}
    };

    /// The type for holding the whole vector of signal samples.
    typedef std::vector<sample_wrapper_t> container_t;

#ifndef ATFA_DEBUG
    /// The stream's rate in samples per second
    static const unsigned samplerate = 11025;

    /// The number of data samples held internally be the stream structure.
    /**
      * The actual size of the vector used to hold is `2*samplerate`, in order
      * to make the data structure "look" circular.
      */
    static const size_t buf_size = 8*samplerate;
#else
    /// The stream's rate in samples per second
    static const size_t buf_size = 24;

    /// The number of data samples held internally be the stream structure.
    /**
      * The actual size of the vector used to hold is `2*samplerate`, in order
      * to make the data structure "look" circular.
      */
    static const unsigned samplerate = 1;
#endif

    /// Returns the next audio sample
    /**
      * Reads the audio sample pointed to by `read_ptr` and makes the pointer
      * indicate the next sample. Handles the case in which the pointer must be
      * rewinded, because the data structure is circular.
      *
      * \returns the next audio sample in line
      *
      * \see data
      * \see write
      */
    sample_t read() {
        sample_t s = read_ptr->sample;
        ++read_ptr;
        if (read_ptr == semantic_end)
            read_ptr = data.begin();
        return s;
    }

    /// Returns an "array" with the last \a n samples
    /**
      * Makes a pointer (actually, an iterator) to the n-th pointer behind
      * `read_ptr`. Handles the case in which the range
      * \f$\left\[\texttt read_ptr -\texttt n,\texttt read_ptr\right\[\f$
      * crosses the end of the circular structure. That is, for any valid value
      * of `read_ptr`, this function can be called with any
      * \f$\texttt n\in\left\[0, \texttt buf_size\right\]\f$. A valid `read_ptr`
      * is one such that `read_ptr - data.begin()` is in the range
      * \f$\left\[0, \texttt buf_size\right\[\f$.
      *
      * \returns an iterator pointing to the last \a n samples.
      *
      * \see data
      */
    container_t::const_iterator get_last_n(index_t n) {
        ++read_ptr;
        // think of the following summation as being "modulo buf_size",
        // because the data structure is to be circular.
        container_t::const_iterator p = read_ptr + buf_size - n;
        if (read_ptr == semantic_end)
            read_ptr = data.begin();
        return p;
    }

    /// Writes an audio sample to the stream
    /**
      * Writes a sample to the data structure so that it can be later read by
      * `read()` or `get_last_n()`. Writes it two times, so that the list is
      * always "doubled" to make it look "circular". Handles the case in which
      * the pointer must be rewinded.
      *
      * \see data
      * \see read
      */
    void write(sample_t s) {
        write_ptr->sample = s;
        (write_ptr + buf_size)->sample = s;
        ++write_ptr;
        if (write_ptr == semantic_end)
            write_ptr = data.begin();
    }

    Stream()
        : delay_samples(0), data(2*buf_size),
          write_ptr(data.begin()), read_ptr(data.begin()),
          semantic_end(data.begin() + buf_size), imp_resp(1, 1)
    {
#ifdef ATFA_DEBUG
        if (buf_size == 0) throw std::runtime_error("Stream: Bad buf_size");
        if (samplerate == 0) throw std::runtime_error("Stream: Bad srate");
#endif
    }

    sample_t& operator [](index_t index) {
#ifdef ATFA_DEBUG
        if (index >= data.size()) throw std::runtime_error("Range error!!");
#endif
        return data[index].sample;
    }

    const sample_t& operator [](index_t index) const {
#ifdef ATFA_DEBUG
        if (index >= data.size()) throw std::runtime_error("Range error!!");
#endif
        return data[index].sample;
    }

    void echo(unsigned sleep = 0);

    void set_delay(unsigned delay) {
        delay_samples = samplerate * delay/1000;
        if (delay_samples <= static_cast<size_t>(write_ptr - data.begin()))
            read_ptr = write_ptr - delay_samples;
        else
            // We won't ckeck, for performance, that delay_samples <= data.size
            // The application must enforce this.
            read_ptr = write_ptr + (buf_size - delay_samples);
    }

    sample_t get_filtered_sample() {
        container_t::const_iterator p = get_last_n(imp_resp.size());
        sample_t accum = 0;
        for (index_t k = imp_resp.size(); k != 0; --k, ++p)
            accum += imp_resp[k-1].sample * p->sample;
        return accum;
    }

    void set_filter(container_t h);

    void dump_state(const container_t speaker_buf) const;
    void simulate();

private:

    index_t delay_samples;

    container_t data;
    typename container_t::iterator write_ptr;
    typename container_t::const_iterator read_ptr;
    const typename container_t::const_iterator semantic_end;

    container_t imp_resp;

};

#endif // STREAM_H
