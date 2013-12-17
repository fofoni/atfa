#ifndef SIGNAL_H
#define SIGNAL_H

#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>

#include <cstdlib>

#include <opencv2/core/core.hpp>

#include <sndfile.hh>

#ifndef NULL
# define NULL ((void *) 0)
#endif

class Signal
{

public:
    Signal();
    Signal(std::string filename);
    ~Signal();

    float *data;
    unsigned long samples;

    int sample_rate;
    unsigned long counter;

    inline float& operator [](int index);

    void set_size(unsigned long n);

};

Signal::Signal()
    : data(NULL), sample_rate(0), counter(0)
{
}

Signal::~Signal()
{
    if (data != NULL)
        free(data);
}

inline float& Signal::operator [](int index) {
    // no checking that index is valid for performance
    return data[index];
}

void Signal::set_size(unsigned long n) {
    float *ptr = (float *)realloc(data, n*sizeof(float));
    if (ptr == NULL)
        throw std::runtime_error("Error trying to (re)allocate space.");
    data = ptr;
    samples = n;
}

Signal::Signal(std::string filename)
    : data(NULL), counter(0)
{

    // open file
    SNDFILE *file;
    SF_INFO info;
    if (!( file = sf_open(filename.c_str(), SFM_READ, &info) ))
        throw std::runtime_error(std::string("Cannot open file `") + filename +
                                 "' for reading.");

    // check number of channels
    unsigned chans = info.channels;
    if (chans > 1)
        std::cerr << "Warning: file " << filename << " has more than one" <<
                     " channel." << std::endl << "  We will use just the" <<
                     " first (which is the left channel on stereo WAV files).";

    // set properties
    set_size(info.frames);
    sample_rate = info.samplerate;

    // read file
    float *buf = (float *)malloc(samples*chans*sizeof(float));
    unsigned long items_read = sf_read_float(file, buf, samples*chans);
    if (items_read != samples*chans)
        throw std::runtime_error(std::string("Error reading file `") +
                                 filename + "'.");
    for (unsigned i = 0; i < samples; ++i)
        data[i] = buf[i*chans];
    free(buf);

}

#endif // SIGNAL_H
