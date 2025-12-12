#ifndef SIGNAL_SETTINGS_H_
#define SIGNAL_SETTINGS_H_


#include <stddef.h>
#include <stdint.h>

struct vcd_s;


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
    char *id;
    char *path;
    // We need to be able to differentiate between 0 and unset for this
    // value. The only way libcyaml can do this (afaik) is by using pointers
    uint8_t *show;
    double height;
    double *slope_width; // Same story here
    double *margin;      // Same story here
    double *text_margin; // Same story here
    double line_thickness;
    double font_size;
    char *line_color; // These are just strings
    char *text_color; // These are just strings

} signal_settings_t;


typedef struct {

    double waveform_width;
    double time_unit_width;
    size_t max_time;

    signal_settings_t global;
    signal_settings_t *signals;
    size_t signal_count;

} svg_settings_t;


svg_settings_t initSvgSettings(size_t count);

//! DO NOT RUN DIRECTLY ON THE STRUCTS FROM A YAML FILE, ONLY ON COPIES.
void mergeStyles(signal_settings_t *dest, signal_settings_t *from);

svg_settings_t *loadSettingsFromFile(char *file_name);
void freeSettings(svg_settings_t *settings);

void writeTemplate(char *file_name, struct vcd_s vcd);

#endif