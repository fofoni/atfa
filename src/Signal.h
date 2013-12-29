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
 * \file Signal.h
 *
 * Holds the interface to the `Signal` class.
 *
 * \author Pedro Angelo Medeiros Fonini
 */

#ifndef SIGNAL_H
#define SIGNAL_H

#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>

#include <cmath>

#include <portaudio.h>
#include <sndfile.hh>

#ifndef NULL
/// null pointer
static const void *NULL = ((void *)0)
#endif

/// A time- or frequency-domain signal
/**
  * Holds data and provides routines for dealing with time-domain and
  * frequency-domain signals. Currently, all Signals are an array of
  * single-precision floating-point samples. Signals know their sample rate.
  *
  * \todo Implement "stream" signals, to provide real-time processing.
  *
  * \todo documentar tudo o que ainda não está documentado
  *
  */
class Signal
{

public:
    /// The type for holding each signal sample.
    typedef float sample_t;

    /// The type for holding each signal sample index.
    typedef unsigned long index_t;

    /// This is a type for specifying whether a time interval is given in
    /// milliseconds or in samples.
    enum delay_t {
        MS,     ///< Time interval given in milliseconds.
        SAMPLE  ///< Time interval given in samples.
    };

    /// Constructs an empty signal.
    /**
      * Initializes the signal with no meta-data and no samples. The user needs
      * to specify the sample rate and create samples before using the signal.
      */
    Signal() : counter(0), srate(0) {}

    /// Constructs a signal from an audio file.
    Signal(const std::string filename);

    /// Copy-constructor. Constructs a signal as a copy of another.
    /**
      * Constructs a signal as a copy of another one. If this signal is not empty,
      * we destroy it.
      *
      * \param[in] other    The signal to be copied from.
      */
    Signal(const Signal &other)
        : counter(0), data(other.data), srate(other.srate) {}

    /// Frees memory used
    /**
      * If the signal is not empty, free the pointer to the array of samples.
      */
    ~Signal() {}

    // todo: isso devia retornar pointer pra `_const_ sample_t`
    // needed for performance inside the callback function
    sample_t *array() { return &data[0]; }

    index_t samples() const { return data.size(); }

    int samplerate() const { return srate; }

    index_t counter; ///< general-purpose variable for external use.

    /// Returns a sample.
    /**
      * Gets a sample of the signal. For performance reasons, this method does
      * not check that the given index is valid.
      *
      * \param[in] index    The index of the desired sample. Signal indexes are
      *                     zero-based.
      *
      * \returns a reference to the sample.
      */
    sample_t& operator [](index_t index) { return data[index]; }

    /// Returns a "read-only" sample.
    /**
      * Just like the "read-write" version, but returns a const reference to a
      * sample.
      *
      * \param[in] index    The index of the desired sample. Signal indexes are
      *                     zero-based.
      *
      * \returns a const reference to the sample.
      */
    const sample_t& operator [](index_t index) const { return data[index]; }

    void set_size(index_t n); ///< Changes the number of samples.

    void set_samplerate(int sr); ///< Changes the signal sample rate.

    void delay(delay_t t, unsigned long d); ///< Delays the signal in time.
    void add(const Signal &other); ///< Adds the `other` signal to the caller.
    void gain(double g); ///< Applies gain `g` to the signal.

    /// Convolves the sinal with an impulse_response.
    void filter(const Signal &imp_resp, Signal& conv) const;

    void play(bool sleep=true); ///< Makes PortAudio playback the audio signal.

private:
    /// Pointer to the array of samples.
    /**
      * This member is public only because we need to pass it to the libsndfile
      * read audio file function.
      */
    std::vector<sample_t> data;

    /// %Signal sample rate in hertz
    int srate;

};

#endif // SIGNAL_H
