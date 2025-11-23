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
    double waveform_width;
    double image_width;
    double margin;

    size_t max_time;

} svg_settings_t;

void writeSVG(FILE *file, vcd_t vcd, svg_settings_t settings);


#endif