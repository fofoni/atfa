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

#include <cmath>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <stdexcept>

#include <portaudio.h>
#include <sndfile.hh>

#ifndef NULL
/// Null pointer.
/**
  * Replaces the standard null pointer in case it's not defined.
  */
static const void * const NULL = ((void *)0)
#endif

/// \brief A runtime exception while trying to process a file.
///
/// Thrown when we cannot read a file, for some reason.
///
/// Usage:
///
///     if (error ocurred) throw FileError("badfile.wav");
///
/// Or:
///
///     std::string filename;
///     std::cin >> filename;
///     // ...
///     if (error ocurred) throw FileError(filename);
///
class FileError : public std::runtime_error {

    /// The message that will be displayed if we don't catch the exception.
    /**
      * Must be static, so that we can modify it inside the
      * \ref FileError::what [`what()`] `const` function, and read it after the
      * temporary object has been destroyed.
      */
    static std::ostringstream msg;

    /// The name of the file that caused the error.
    const std::string filename;

public:
    /// Constructs the exception object from the filename.
    /**
      * \param[in] fn   A \ref string [`string`] that holds the filename.
      */
    FileError(const std::string& fn)
        : runtime_error("File I/O error"), filename(fn) {}

    /// Destructor that does nothing.
    /**
      * Needed to prevent the `looser throw specifier` error because,
      * \ref std::runtime_error::~runtime_error [`~runtime_errorr()`] is
      * declared as throw()
      */
    ~FileError() throw() {}

    /// Gives a description for the error.
    /**
      * Updates the \ref FileError::msg [`msg`]  static member with the error
      * message, and returns it as a C string.
      */
    virtual const char *what() const throw() {
        msg.str(""); // static member `msg' can be modified by const methods
        msg << runtime_error::what() << ": Couldn't read file `" << filename
            << "'.";
        return msg.str().c_str();
    }

};

/// A time- or frequency-domain signal
/**
  * Holds data and provides routines for dealing with time-domain and
  * frequency-domain signals. Currently, all Signals are an array of
  * single-precision floating-point samples. Signals are aware of their sample
  * rates.
  *
  * \todo Implement "stream" signals, to provide real-time processing.
  *
  * \todo documentar tudo o que ainda não está documentado e botar a
  *       documentacao no gh-pages
  *
  */
class Signal
{

public:
    /// Shorthand for the number \f$2\pi\f$.
    /**
      * Useful in the generation of the table of sines and cosines for the DFTDriver
      * class, for example.
      */
    static const double TAU = 6.283185307179586477;

    /// The type for holding each signal sample.
    typedef float sample_t;

    /// The type for holding each signal sample index.
    typedef unsigned long index_t;

    /// The type for holding the whole vector of signal samples.
    typedef std::vector<sample_t> container_t;

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
    Signal() : counter(0), data(), srate(0) {}

    /// Constructs a signal from an audio file.
    Signal(const std::string &filename);

    /// Copy-constructor. Constructs a signal as a copy of another.
    /**
      * Constructs a signal as a copy of another one. If this signal is not
      * empty, we destroy it.
      *
      * \param[in] other    The signal to be copied from.
      */
    Signal(const Signal &other)
        : counter(0), data(other.data), srate(other.srate) {}

    /// Frees memory used
    /**
      * Free the pointer to the array of samples.
      */
    ~Signal() {}

    /// Returns a pointer to the first sample.
    /**
      * Sometimes needed for performance reasons. Shouldn't be used to modify
      * the samples.
      *
      * \returns a pointer to the first element of a contiguous region of memory
      * that holds the samples.
      */
    const sample_t *array() const { return &data[0]; }

    /// Number of samples.
    /**
      * \returns the number of elements inside the vector of samples.
      */
    index_t samples() const { return data.size(); }

    /// Sample rate in samples per second.
    /**
      * \returns the number of samples per second that should be used when
      * playing back the signal.
      */
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

    /// Changes the number of samples.
    /**
      * Changes the signal length. Allocates more space if we are growing the
      * signal, and deletes the last samples if we are shrinking it. Also
      * initializes any new samples to zero.
      *
      * \param[in] n        The desired signal length.
      */
    void set_size(index_t n) { data.resize(n); }

    void set_samplerate(int sr); ///< Changes the signal sample rate.
    void delay(delay_t t, unsigned long d); ///< Delays the signal in time.
    void gain(double g); ///< Applies gain \a g to the signal.
    sample_t l_inf_norm(); ///< Gets the \f$\ell^\infty\f$-norm of the signal.

    /// Normalize the signal according to its \f$\ell^\infty\f$-norm.
    /**
      * Divide the signal by a constant so that the maximum absolute value of
      *  its samples is \f$1\f$.
      */
    void normalize() { gain(1.0/l_inf_norm()); }

    /// Adds the \a other signal to the caller.
    Signal& operator +=(Signal other);

    /// Convolves the sinal with an impulse response.
    void filter(Signal imp_resp);

    /// Makes PortAudio playback the audio signal.
    void play(bool sleep=true);

    /// \brief A class for providing discrete Fourier transform capabilities.
    ///
    /// This class implements the radix-2 FFT algorithm used in the
    /// Signal::filter() method.
    ///
    /// Usage:
    ///
    ///     Signal::DFTDriver dft;
    ///     Signal real, imag;
    ///     // initialize the real and imaginary parts of a complex time-domain
    ///     // signal
    ///     dft(real, imag); // performs in-place FFT
    ///     // now, work with the real and imaginary parts of the
    ///     // frequency-domain version of the signal
    ///     dft(real, imag, Signal::DFTDriver::INVERSE); // inverse in-place fft
    ///     // now, we can work again with the time-domain complex signal
    ///
    class DFTDriver {

        /// Number of bits for an actuall fft computation.
        /**
          * Always assume this is uninitialized, and all methods that use it
          * should initialize it themselves.
          */
        unsigned bits;

        /// Bit-reverse.
        /**
          * Returns the bit-reversed version of the parameter \a x. Assumes \a x
          * is _bits_-bit wide, and ignore any bits with more significance than
          * that.
          *
          * This function assumes that the number of bits in one `char` is 8,
          * and that bitshifting is zero-padded, and not circular.
          *
          * \tparam     T       The type of the parameter \a x. It __must__ be
          *                     an unsigned integer type.
          * \param[in]  x       The _bits_-bit unsigned integer to be
          *                     bit-reversed.
          * \param[in]  bits    The number of bits of the integer \a x.
          * \returns the unsigned integer \a x, bit-reversed.
          */
        template <typename T>
        static T br(T x, int bits) {

            static const unsigned char BitReverseTable256[] = {

              0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0,
              0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0,

              0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8,
              0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8,

              0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4,
              0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4,

              0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC,
              0x1C, 0x9C, 0x5C, 0xDC, 0x3C, 0xBC, 0x7C, 0xFC,

              0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2,
              0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2,

              0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA,
              0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA,

              0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6,
              0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6, 0x76, 0xF6,

              0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE,
              0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE,

              0x01, 0x81, 0x41, 0xC1, 0x21, 0xA1, 0x61, 0xE1,
              0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1,

              0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9,
              0x19, 0x99, 0x59, 0xD9, 0x39, 0xB9, 0x79, 0xF9,

              0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5,
              0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5,

              0x0D, 0x8D, 0x4D, 0xCD, 0x2D, 0xAD, 0x6D, 0xED,
              0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD,

              0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3,
              0x13, 0x93, 0x53, 0xD3, 0x33, 0xB3, 0x73, 0xF3,

              0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB,
              0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB,

              0x07, 0x87, 0x47, 0xC7, 0x27, 0xA7, 0x67, 0xE7,
              0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7,

              0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF,
              0x1F, 0x9F, 0x5F, 0xDF, 0x3F, 0xBF, 0x7F, 0xFF

            };

            T result = 0;

            // reverse inside each byte, and then reverse order of bytes
            for (unsigned i = 0; i < sizeof(T); ++i)
                result |= BitReverseTable256[
                              ( x >> (8*(sizeof(T)-1-i)) ) & 0xFF
                          ] << (8*i);

            return result >> ( 8*sizeof(T) - bits );

        }

        /// Easy access to the table of cosines.
        /**
          * This function is aware of the number of bits of the current FFT,
          * and makes it easy to get the cosine of
          * \f$\tau\cdot \texttt{k}/2^\texttt{bits}\f$, using the pre-computed
          * table of cosines.
          *
          * \param[in]  k   An integer in the range
          *                 \f$\left[0,2^\texttt{bits}\right[\f$.
          * \returns \f$\cos\left(
          *              \tau\cdot \texttt{k}/2^\texttt{bits}
          *          \right)\f$, where \f$\tau\f$ is shorthand for \f$2\pi\f$.
          * \see costbl
          * \see Wim
          */
        double Wre(unsigned k)
            { return costbl[(k & ((1<<bits)-1)) << (tblbits - bits)]; }

        /// Easy access to the table of sines.
        /**
          * \param[in]  k   Same as in \ref Wre.
          * \returns \f$\sin\left(
          *              \tau\cdot \texttt{k}/2^\texttt{bits}
          *          \right)\f$
          * \see sintbl
          * \see Wre
          */
        double Wim(unsigned k)
            { return sintbl[(k & ((1<<bits)-1)) << (tblbits - bits)]; }

    public:
        /// Number of bits for the index of the table of sines and cosines
        /**
          * We won't be able to perform an \f$N\f$-bit dft if
          * \f$N > \texttt{tblsize}\f$, so this should be big. Also, this
          * __must__ be equal to \f$\log_2\left(\texttt{tblsize}\right)\f$, but
          * there's nothing in the source code that enforces it.
          *
          * \see tblsize
          */
        static const unsigned tblbits = 14;

        /// Number of entries in the tables of sines and cosines
        /**
          * This __must__ be equal to \f$2^\texttt{tblbits}\f$, but there's
          * nothing in the source code that enforces it.
          *
          * \see tblbits
          */
        static const size_t tblsize = 16384;

        /// This is a type for specifying whether we should perform a direct or
        /// inverse FFT.
        enum dir_t {
            DIRECT,  ///< Perform direct FFT.
            INVERSE  ///< Perform inverse FFT.
        };

        /// Constructor for an object that computes DFTs.
        /**
          * Computes and initializes the table of sines and cosines. After this
          * initialization, the entries of the table shouldn't be modified.
          *
          * \todo make the table be inside a std::vector, instead of a built-in
          *       array, so that we can use the solution provided [here][1].
          *       An alternative is documented [here][2].
          *       [1]: stackoverflow.com/questions/16113551/c-const-static-member-array-initialization
          *       [2]: stackoverflow.com/questions/21248187/a-const-big-array-needs-to-be-initialized
          *
          * \todo this static member should be initialized only once, even
          *       though multiple instances of the DFTDriver might be created
          *       (that is, static members shouldn't be initialized in the
          *       constructor).
          *       (perhaps we could have a private bool telling wether it's been
          *       already initialized)
          *
          * \see sintbl
          * \see costbl
          */
        DFTDriver() {
            for (unsigned i = 0; i != tblsize; ++i) {
                sintbl[i] = std::sin(i * TAU / tblsize);
                costbl[i] = std::cos(i * TAU / tblsize);
            }
        }

        /// Used to perform the actual computation of the DFT
        void operator ()(container_t& re, container_t& im,
                         dir_t direction = DIRECT);

    private:
        /// Table of sines.
        /**
          * Holds the sines of \f$\tau\cdot k/\texttt{tblsize}\f$, for \f$k\f$
          * in the range \f$\left[0,\texttt{tblsize}\right[\f$. Here, \f$\tau\f$
          * is shorthand for \f$2\pi\f$.
          *
          * \see costbl
          */
        static double sintbl[tblsize];

        /// Table of cosines.
        /**
          * \see sintbl
          */
        static double costbl[tblsize];

    };

    static DFTDriver dft; ///< single instance of the DFTDriver class.

private:
    container_t data; ///< Holds the signal samples.
    int srate; ///< %Signal sample rate in hertz

};

/// Adds two signals
/**
  * \see Signal::operator+=
  */
inline Signal operator +(Signal lhs, const Signal& rhs)
    { return lhs += rhs; }

#endif // SIGNAL_H
