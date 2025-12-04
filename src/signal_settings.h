#ifndef SIGNAL_SETTINGS_H_
#define SIGNAL_SETTINGS_H_


#include <stddef.h>
#include <stdint.h>

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
    uint8_t show;
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
    .show           = 0xff,    \
    .height         = 0,    \
    .slope_width    = 0,    \
    .margin         = -1,   \
    .line_thickness = 0,    \
    .text_margin    = -1,   \
    .font_size      = 0,    \
}


svg_settings_t initSvgSettings(size_t count);
signal_settings_t getSettings(svg_settings_t *sett, size_t index);
void mergeSettings(svg_settings_t *sett);


#endif