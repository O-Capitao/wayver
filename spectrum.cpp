#include "audio.hpp"

using namespace Wayver;

SpectrumSlice::SpectrumSlice(
    int n_bands,
    float *input_amps,
    float *input_freqs,
    int input_size,
    float amp_zero,
    bool useLogScale
){

    const float max_freq = 20e3;
    const float max_exp = log10(max_freq);
    const float freq_exp_step = max_exp / n_bands;

    float freq_exp_cursor = freq_exp_step;

    // http://astro.wku.edu/labs/m100/logs.html
    _bands.reserve(n_bands);

    // fill out Band data
    for ( int i = 0; i < n_bands; i++ ){

        SpectrumSliceBand b;

        b.log10_freq = freq_exp_cursor;
        b.freq = powf( 10, freq_exp_cursor );   
        

        freq_exp_cursor += freq_exp_step;
    }
}

int SpectrumSlice::_findBeforeFreqIndex( float *f_arr, int arr_size, float f, int strt_cur  ){

    int cursor = strt_cur;

    while ( cursor < arr_size ){

        if ( f_arr[cursor] > f ){
            return cursor - 1;
        }

        cursor ++;
    }

    return -1;
}

float SpectrumSlice::_calcInterpolatedValueForFrequency( float tgt_f, int size, float *f_arr, float *amp_arr ){

    int indexBefore = _findBeforeFreqIndex(f_arr, size, tgt_f);
    assert(indexBefore > -1);

    float tgt_amp = amp_arr[indexBefore] 
        + ( tgt_f - f_arr[indexBefore] ) * ( amp_arr[indexBefore + 1] 
        - amp_arr[ indexBefore] ) / ( f_arr[indexBefore + 1] - f_arr[indexBefore] );

    return tgt_amp;
}