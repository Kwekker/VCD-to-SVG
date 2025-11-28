#ifndef SVG_H_
#define SVG_H_

#include <stdio.h>
#include "vcd.h"


//               slope_width
//              |--|
// height ┬       /‾‾‾‾‾‾
//        ┴ _____/
// margin ┬
//        │
//        │
//        ┴
//                 /‾‾‾‾‾‾
//           _____/
//         |-------------|
//          waveform width


typedef struct {

    double height;
    double slope_width;
    double margin;
    double line_thickness;
    double text_margin;
    double font_size;

} signal_settings_t;

typedef struct {

    double waveform_width;
    size_t max_time;

    signal_settings_t global;
    signal_settings_t *signals;
    size_t signal_count;

} svg_settings_t;

#define IGNORED_SETTINGS (signal_settings_t){\
    .height         = 0,    \
    .slope_width    = 0,    \
    .margin         = -1,   \
    .line_thickness = 0,    \
    .text_margin    = -1,   \
    .font_size      = 0,    \
}

void writeSVG(FILE *file, vcd_t vcd, svg_settings_t settings);


#endif