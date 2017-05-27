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

#include <vector>

#include "utils.h"

/// A time- or frequency-domain signal
/**
  * Holds data and provides routines for dealing with time-domain and
  * frequency-domain signals. Currently, all Signals are an array of
  * single-precision floating-point samples. Signals are aware of their sample
  * rates.
  */
class Signal
{

public:
    /// The type for holding each signal sample.
    typedef float sample_t;

    /// The type for holding each signal sample index.
    typedef unsigned long index_t;

    /// The type for holding the whole vector of signal samples.
    typedef std::vector<sample_t> container_t;

    /// \brief This is a type for specifying whether a time interval is given in
    ///        milliseconds or in samples.
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
      * not check that the given index is valid. (Except in debug releases, in
      * which it _does_ check.)
      *
      * \param[in] index    The index of the desired sample. %Signal indexes are
      *                     zero-based.
      *
      * \returns a reference to the sample.
      */
    sample_t& operator [](index_t index) {
#ifdef ATFA_DEBUG
        if (index >= samples()) throw std::runtime_error("Range error!!");
#endif
        return data[index];
    }

    /// Returns a "read-only" sample.
    /**
      * Just like the "read-write" version, but returns a const reference to a
      * sample.
      *
      * \param[in] index    The index of the desired sample. %Signal indexes are
      *                     zero-based.
      *
      * \returns a const reference to the sample.
      */
    const sample_t& operator [](index_t index) const {
#ifdef ATFA_DEBUG
        if (index >= samples()) throw std::runtime_error("Range error!!");
#endif
        return data[index];
    }

    /// Changes the number of samples.
    /**
      * Changes the signal length. Allocates more space if we are growing the
      * signal, and deletes the last samples if we are shrinking it. Also
      * initializes any new samples to zero.
      *
      * \param[in] n        The desired signal length.
      *
      * \see container_t::resize()
      */
    void set_size(index_t n) { data.resize(n); }

    void set_samplerate(int sr); ///< Re-samples the signal.
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

    /// Makes PortAudio playback the audio signal.
    void play(bool sleep=true);

    //       Do jeito que está, a DFT faz transformada de vetores de qualquer
    //       um dos tamanhos 2^0, 2^1, 2^2, ..., 2^tblbits. Para isso, o código
    //       é genérico no comprimento do vetor.
    // TODO: isso não deveria ser assim. O tamanho dos vetores nos quais a
    //       DFT vai operar é conhecido em tempo de compilação! (a única
    //       exceção é o Signal::filter, que usar dft de tamanho variável;
    //       dar um jeito nisso) Na pior das hipóteses, fazemos duas DFTs:
    //       uma genérica e uma especializada

    /// \brief A class for providing discrete Fourier transform capabilities.
    ///
    /// This class implements the radix-2 FFT algorithm used in the
    /// Signal::filter() method.
    ///
    /// Usage:
    ///
    ///     Signal::DFTDriver dft;
    ///     Signal::container_t real, imag;
    ///     // initialize the real and imaginary parts of a complex time-domain
    ///     // signal
    ///     dft(real, imag); // performs in-place FFT
    ///     // now, work with the real and imaginary parts of the
    ///     // frequency-domain version of the signal
    ///     dft(real, imag, Signal::DFTDriver::INVERSE); // inverse in-place fft
    ///     // now, we can work again with the time-domain complex signal
    ///
    template<int TABLE_BITS=14>
    class DFTDriver {

        /// Number of bits for the current FFT computation.
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
        static T br(T x, unsigned bits) {

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
                result |= static_cast<T>(BitReverseTable256[
                              ( x >> (8*(sizeof(T)-1-i)) ) & 0xFF
                          ]) << (8*i);

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
            { return costbl[(k & ((1u<<bits)-1u)) << (tblbits - bits)]; }

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
            { return sintbl[(k & ((1u<<bits)-1u)) << (tblbits - bits)]; }

    public:
        /// Number of bits for the index of the table of sines and cosines
        /**
          * We won't be able to perform an \f$N\f$-bit dft if
          * \f$N > \texttt{tblbits}\f$, so this should be big.
          *
          * \see tblsize
          */
        static constexpr unsigned tblbits = TABLE_BITS;

        /// Number of entries in the tables of sines and cosines
        /**
          * \see tblbits
          */
        static constexpr size_t tblsize = CTUtils::pow(2, tblbits);

        /// This is a type for specifying whether we should perform a direct or
        /// inverse FFT.
        enum dir_t {
            DIRECT,  ///< Perform direct FFT.
            INVERSE  ///< Perform inverse FFT.
        };

        /// Initializes the table of cosines
        /**
          * Computes a table of cosines that will be handed to the `costbl`
          * member.
          *
          * \see initialize_sintbl
          * \see costbl
          */
        static std::vector<double> initialize_costbl() {
            std::vector<double> ct(tblsize);
            for (unsigned i = 0; i != tblsize; ++i)
                ct[i] = std::cos(i * TAU / tblsize);
            return ct;
        }

        /// Initializes the table of sines
        /**
          * \see initialize_costbl
          * \see sintbl
          */
        static std::vector<double> initialize_sintbl() {
            std::vector<double> st(tblsize);
            for (unsigned i = 0; i != tblsize; ++i)
                st[i] = std::sin(i * TAU / tblsize);
            return st;
        }

        /// Constructor for an object that computes DFTs.
        /**
          * Does nothing at all.
          */
        DFTDriver() {}

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
        static const std::vector<double> sintbl;

        /// Table of cosines.
        /**
          * \see sintbl
          */
        static const std::vector<double> costbl;

    };

    static DFTDriver<> dft; ///< Single instance of the DFTDriver class.

    /// Convolves the sinal with an impulse response.
    template<class DFT=DFTDriver<> >
    void filter(Signal imp_resp);

private:
    container_t data; ///< Holds the signal samples.
    int srate; ///< %Signal sample rate in Hertz.

};

using DefaultDFT = Signal::DFTDriver<>;

/// Adds two signals
/**
  * \see Signal::operator+=
  */
inline Signal operator +(Signal lhs, const Signal& rhs)
    { return lhs += rhs; }

/**
  * Convolves the signal with the given finite impulse response (FIR).
  *
  * The algorthm used is the "overlap-and-add", and we use the FFT implemented
  * in the DFTDriver class to compute each step. We try to do it using the least
  * possible number of DFTs.
  *
  * \param[in]  imp_resp    The filter impulse response to be convolved with.
  *
  * \see DFTDriver::operator()
  */
template<class DFT=Signal::DFTDriver<>>
void Signal::filter(Signal imp_resp) {
    imp_resp.set_samplerate(srate);
    index_t final_size = samples() + imp_resp.samples() - 1;
    if (final_size > DFT::tblsize) {
        // divide the signal in chunks of size N such that N+K-1==tblsize
        container_t *big, *small;
        if (samples() > imp_resp.samples())
            big = &data, small = &imp_resp.data;
        else
            big = &imp_resp.data, small = &data;
        long N = DFT::tblsize - small->size() + 1;
        container_t conv(final_size);
        if (N <= 0) {
            // fall back to time-domain filtering
            for (index_t i = 0; i < imp_resp.samples(); i++)
                for (index_t j = 0; j <= i; j++)
                    conv[i] += data[i-j] * imp_resp[j];
            for (index_t i = imp_resp.samples(); i < samples(); i++)
                for (index_t j = 0; j < imp_resp.samples(); j++)
                    conv[i] += data[i-j] * imp_resp[j];
            for (index_t i = 0; i < conv.size()-samples(); i++)
                for (index_t j = i+1; j < imp_resp.samples(); j++)
                    conv[samples()+i] += data[samples()+i-j] * imp_resp[j];
            data = conv; // destroy old data, and copy new from `conv'
            return;
        }
        container_t h_re(DFT::tblsize);
        container_t h_im(DFT::tblsize);
        container_t x1(DFT::tblsize);
        container_t x2(DFT::tblsize);
        container_t y1(DFT::tblsize);
        container_t y2(DFT::tblsize);
        std::copy(small->begin(), small->end(), h_re.begin());
        dft(h_re, h_im);
        index_t i;
        // TODO: ao invés de incrementar o 'i' e calcular i1 e i2 a cada
        // iteração, a gente podia calcular diretamente o i1 e i2, e.g.:
        // for (i1=0, i2=N ; i1 < big->size() ; i1+=2*N, i2+=2*N)
        for (i = 0; i != big->size()/(2*N); ++i) {
            index_t i1 = i*2*N;
            index_t i2 = i1 + N;
            std::copy(big->begin() + i1, big->begin() + i1 + N, x1.begin());
            std::fill(x1.begin() + N, x1.end(), 0);
            std::copy(big->begin() + i2, big->begin() + i2 + N, x2.begin());
            std::fill(x2.begin() + N, x2.end(), 0);
            dft(x1, x2);
            for (index_t j = 0; j != DFT::tblsize; ++j) {
                y1[j] = x1[j]*h_re[j] - x2[j]*h_im[j];
                y2[j] = x1[j]*h_im[j] + x2[j]*h_re[j];
            }
            dft(y1, y2, DFT::INVERSE);
            for (index_t j = 0; j != DFT::tblsize; ++j) {
                conv[i1 + j] += y1[j];
                conv[i2 + j] += y2[j];
            }
        }
        {
            index_t i1 = i*2*N;
            index_t i2 = i1 + N;
            long L = big->size() - i1;
            if (L > N) {
                std::copy(big->begin() + i1, big->begin() + i1 + N, x1.begin());
                std::fill(x1.begin() + N, x1.end(), 0);
                std::copy(big->begin() + i2, big->end(), x2.begin());
                std::fill(x2.begin() + (L-N), x2.end(), 0);
                dft(x1, x2);
                for (index_t j = 0; j != DFT::tblsize; ++j) {
                    y1[j] = x1[j]*h_re[j] - x2[j]*h_im[j];
                    y2[j] = x1[j]*h_im[j] + x2[j]*h_re[j];
                }
                dft(y1, y2, DFT::INVERSE);
                for (index_t j = 0; j != DFT::tblsize; ++j)
                    conv[i1 + j] += y1[j];
                for (index_t j = 0; j != L-N+small->size()-1; ++j)
                    conv[i2 + j] += y2[j];
            }
            else {
                std::copy(big->begin() + i1, big->end(), x1.begin());
                std::fill(x1.begin() + L, x1.end(), 0);
                std::fill(x2.begin(), x2.end(), 0);
                dft(x1, x2);
                for (index_t j = 0; j != DFT::tblsize; ++j) {
                    y1[j] = x1[j]*h_re[j] - x2[j]*h_im[j];
                    y2[j] = x1[j]*h_im[j] + x2[j]*h_re[j];
                }
                dft(y1, y2, DFT::INVERSE);
                for (index_t j = 0; j != L+small->size()-1; ++j)
                    conv[i1 + j] += y1[j];
            }
        }
        data = conv; // destroy old data, and copy new from `conv'
    }
    else {
        // fit the whole signal in only one fft
        index_t L;
        for (L = DFT::tblsize; L/2 >= final_size; L /= 2) ;
        // here, final_size fits in L
        container_t h_re(L);
        container_t h_im(L);
        container_t x1(L);
        container_t x2(L);
        container_t y1(L);
        container_t y2(L);
        std::copy(imp_resp.data.begin(), imp_resp.data.end(), h_re.begin());
        dft(h_re, h_im);
        std::copy(data.begin(), data.end(), x1.begin());
        dft(x1, x2);
        for (index_t j = 0; j != DFT::tblsize; ++j) {
            y1[j] = x1[j]*h_re[j] - x2[j]*h_im[j];
            y2[j] = x1[j]*h_im[j] + x2[j]*h_re[j];
        }
        dft(y1, y2, DFT::INVERSE);
        set_size(final_size);
        std::copy(y1.begin(), y1.begin() + final_size, data.begin());
    }
}

#endif // SIGNAL_H
