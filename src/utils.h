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
 * \file utils.h
 *
 * Holds convenient definitions and other utilities.
 *
 * \author Pedro Angelo Medeiros Fonini
 */

#ifndef UTILS_H
#define UTILS_H

#include <sstream>
#include <stdexcept>

#include <QtCore>

#ifndef ATFA_DIR
#   include <cstring>
    extern "C" {
#       include <libgen.h>
    }

    char static_filename[] = __FILE__;
    char static_dirname[] = __FILE__;
    char static_projdirname[] = __FILE__;

    // watch out, because dirname() may modify its argument,
    // and also, ATFA_DIR might get evaluated more than once

    /// Macro for getting the path to the project directory from cmake
    /** Should be passed from `CMakeLists.txt`, but if it's not, we try to
        deduce it from the `__FILE__` macro */
#   define ATFA_DIR (static_cast<const char *>( \
        std::strcpy(static_dirname, dirname(static_filename)), \
        std::strcpy(static_filename, __FILE__), \
        std::strcpy(static_projdirname, dirname(static_dirname)), \
        static_projdirname \
    ))
#endif

#define html_link(url) "<a href='" url "'>" url "</a>"

/// Shorthand for the number \f$2\pi\f$.
/**
  * Useful in the generation of the table of sines and cosines for the
  * Signal::DFTDriver class, for example.
  */
static constexpr double TAU = 6.283185307179586477;

/// Initialize PortAudio.
void portaudio_init(bool list_devices=false);

/// Close PortAudio.
void portaudio_end();

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
///     ...
///     if (error ocurred) throw FileError(filename);
///
class FileError : public std::runtime_error {

    /// The message that will be displayed if we don't catch the exception.
    /**
      * Must be static, so that we can modify it inside the
      * `what()` `const` function, and read it after the
      * temporary object has been destroyed.
      */
    static std::ostringstream msg;

    /// The name of the file that caused the error.
    const std::string filename;

public:
    /// Constructs the exception object from the filename.
    /**
      * \param[in] fn   A `std::string` that holds the filename.
      */
    FileError(const std::string& fn)
        : runtime_error("File I/O error"), filename(fn) {}

    /// Destructor that does nothing.
    /**
      * Needed to prevent the `looser throw specifier` error because,
      * `std::runtime_error::~runtime_error()` is declared as `throw()`
      */
    ~FileError() throw() {}

    /// Gives a description for the error.
    /**
      * Updates the \ref msg static member with the error
      * message, and returns it as a C string.
      */
    virtual const char *what() const throw() {
        msg.str(""); // static member `msg' can be modified by const methods
        msg << runtime_error::what() << ": Couldn't read file `" << filename
            << "'.";
        return msg.str().c_str();
    }

};

// Compile-time utils
namespace CTUtils {
// compile-time exponentiation (using exponentiation-by-squaring algorithm)
// Taken from:
// http://stackoverflow.com/a/16443849
template<class T>
inline constexpr T pow(const T base, unsigned const exponent) {
    return (exponent == 0)     ? 1 :
           (exponent % 2 == 0) ? pow(base, exponent/2)*pow(base, exponent/2) :
           base * pow(base, (exponent-1)/2) * pow(base, (exponent-1)/2);
}
}

QJsonObject read_json_file(const QString& filename);

#endif // UTILS_H
