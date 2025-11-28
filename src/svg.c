#include "svg.h"
#include "vcd.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>



#define DEFAULT_STROKE_STYLE "stroke-width:%f;fill:none;stroke:#000000;stroke-linecap:round;stroke-linejoin:round;stroke-dasharray:none;stroke-opacity:1"

void outputBitSignal(FILE* file, var_t var, svg_settings_t settings, uint32_t y_index);
static void outputSignal(FILE* file, var_t var, svg_settings_t sett, uint32_t y_index);
static inline uint8_t isZero(char *val);

// These are inline because they're only run once.
static inline void outputVectorSignal(
    FILE* file, svg_settings_t sett, value_pair_t val,
    double start_pos_y, size_t prev_time, uint8_t was_zero
);
static inline void outputBitFlip(FILE* file, svg_settings_t sett, double start_pos_y,
    uint8_t new_state, size_t time
);


void writeSVG(FILE *file, vcd_t vcd, svg_settings_t settings) {

    // Make sure we don't draw a bunch of empty space.
    if (settings.max_time > vcd.max_time) settings.max_time = vcd.max_time;
    if (settings.max_time == 0) settings.max_time = vcd.max_time;

    // Calculate viewbox
    if (settings.waveform_width == 0) settings.waveform_width = settings.max_time;
    double width = settings.waveform_width;
    double height = // No margin below the bottom signal.
        vcd.var_count * (settings.height + settings.margin) - settings.margin;


    fprintf(file,
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n"
        "<!-- Created with VCD-to-SVG converter (https://www.github.com/kwekker/vcd-to-svg/) -->\n"
        "<svg width=\"%fmm\" height=\"%fmm\" viewBox=\"0 0 %f %f\" version=\"1.1\" "
        "xmlns:inkscape=\"http://www.inkscape.org/namespaces/inkscape\" "
        "xmlns=\"http://www.w3.org/2000/svg\" "
        "xmlns:svg=\"http://www.w3.org/2000/svg\">\n\t",
        width, height, width, height
    );

    fprintf(file, "<g id=\"layer1\">\n");
    for (size_t i = 0; i < vcd.var_count; i++) {
        var_t var = vcd.vars[i];
        if (var.copy_of != NULL) {
            var.value_count = var.copy_of->value_count;
            var.values = var.copy_of->values;
        }
        printf("Var %zu: %s\n", i, var.name);
        outputSignal(file, var, settings, i);

    }
    printf("Done!\n");

    fprintf(file, "</g>\n");
    fprintf(file, "</svg>");

}

void outputSignal(FILE* file, var_t var, svg_settings_t sett, uint32_t y_index) {
    if (var.size == 0) return;

    // Make the SVG nicer to work with in inkscape by setting a label.
    fprintf(file, "<g id=\"var_%ld\" inkscape:label=\"%s\">\n",
        ftell(file), var.name
    );

    printf("%s has %zu values\n", var.name, var.value_count);

    fprintf(file, "<path style=\"" DEFAULT_STROKE_STYLE "\"\n",
        sett.line_thickness
    );
    fprintf(file, "id=\"waveform_%ld\"\n", ftell(file));
    fprintf(file, "inkscape:label=\"vector waveform\"\n");


    double start_pos_y = y_index * (sett.height + sett.margin);
    double start_pos_x = -sett.slope_width / 2;
    double xs = sett.waveform_width / sett.max_time;

    // current_state is 0 if all values are 0, otherwise it is 1.
    // It's used for drawing the waveform,
    // since it should be ___ when all bits are 0
    uint8_t was_zero = 0;
    if (var.size == 1) was_zero = var.values[0].value_char == '1';
    else was_zero = isZero(var.values[0].value_string);

    size_t prev_time = 0;

    // Always start the svg 'curcor' at the bottom of the signal.
    fprintf(file, "d=\"M %f %f ",
        start_pos_x, start_pos_y + sett.height
    );

    size_t line_count = 0;


    for (size_t j = 1; j < var.value_count; j++) {
        value_pair_t val = var.values[j];
        if (val.time > sett.max_time) break;


        uint8_t is_zero = 0;
        if (var.size == 1) is_zero = val.value_char == '1';
        else is_zero = isZero(val.value_string);

        // Continue if the value didn't change.
        if (is_zero && was_zero) continue;
        char* prev = var.values[j-1].value_string;
        if (var.size > 1 && (
            !is_zero && memcmp(val.value_string, prev, var.size) == 0
        )) continue;

        // Keep track of line count to handle an edge case that's described a
        // bit further.
        line_count++;
        if (var.size > 1) {
            outputVectorSignal(
                file, sett, val, start_pos_y, prev_time, was_zero
            );
        }
        else {
            outputBitFlip(file, sett, start_pos_y, !is_zero, val.time);
        }

    }

    // Draw a final line if the signal got cut off,
    // or if we haven't drawn anything yet.
    if (prev_time != sett.max_time || line_count == 0) {
        double final_time = sett.max_time;
        fprintf(file, "H %f", xs*final_time);
        if (!was_zero && var.size > 1) {
            fprintf(file, "M %f %f H %f ",
                xs*prev_time + sett.slope_width / 2.0, start_pos_y,
                xs*final_time
            );
        }
    }

    fprintf(file, "\"/>\n");
    fprintf(file, "</g>\n");
}


static inline void outputBitFlip(
    FILE* file, svg_settings_t sett, double start_pos_y,
    uint8_t new_state, size_t time
) {
    double xs = sett.waveform_width / sett.max_time;

    fprintf(file, "H %f L %f %f ",
        xs*time - sett.slope_width / 2.0,      // x1
        xs*time + sett.slope_width / 2.0,      // x2
        start_pos_y + sett.height * !new_state // y2
    );
}



// This function with an unreasonably large amount of arguments
// was the best thing I could come up with here, unfortunately.
static inline void outputVectorSignal(
    FILE* file, svg_settings_t sett, value_pair_t val,
    double start_pos_y, size_t prev_time, uint8_t was_zero
) {
    double xs = sett.waveform_width / sett.max_time;
    uint8_t is_zero = isZero(val.value_string);

    // There are 3 possibilities:
    //   1. Transition from a value to 0
    //   2. Transition from a value to another value
    //   3. Transition from 0 to a value

    if (is_zero) { // From a value to zero
        printf("Drawing the is zero case!!\n");
        // Bottom line that ends  in the middle of /
        fprintf(file, "H %f L %f %f ",
            xs*val.time - sett.slope_width / 2.0,
            xs*val.time, start_pos_y + sett.height / 2.0
        );

        // Top line that goes down like ‾‾‾‾\_
        fprintf(file, "M %f %f H %f L %f %f ",
            xs*prev_time + sett.slope_width / 2.0, start_pos_y,
            xs*val.time - sett.slope_width / 2.0,
            xs*val.time + sett.slope_width / 2.0, start_pos_y + sett.height
        );
    }
    else if (!is_zero && !was_zero) { // From a value to another value
        // Bottom line that goes up like ____/‾
        fprintf(file, "H %f L %f %f",
            xs*val.time - sett.slope_width / 2.0,
            xs*val.time + sett.slope_width / 2.0,
            start_pos_y
        );
        // Top line that goes down like  ‾‾‾‾\_
        fprintf(file, "M %f %f H %f L %f %f ",
            xs*prev_time + sett.slope_width / 2.0, start_pos_y,
            xs*val.time - sett.slope_width / 2.0,
            xs*val.time + sett.slope_width / 2.0,
            start_pos_y + sett.height
        );
    }
    else if (!is_zero && was_zero) { // From zero to a value
        // Bottom line that goes up like ____/‾
        fprintf(file, "H %f L %f %f ",
            xs*val.time - sett.slope_width / 2.0,
            xs*val.time + sett.slope_width / 2.0, start_pos_y
        );
        // Bottom line that goes down like    \_
        // It starts at the midpoint of the diagonal line.
        fprintf(file, "M %f %f L %f %f ",
            xs*val.time, start_pos_y + sett.height / 2.0,
            xs*val.time + sett.slope_width / 2.0, start_pos_y + sett.height
        );
    }
}



static inline uint8_t isZero(char *val) {
    while(*val) {
        if (*val != '0') return 0;
        val++;
    }
    return 1;
}



// "font-style:normal;
// font-variant:normal;
// font-weight:normal;
// font-stretch:normal;
// font-size:10px;
// line-height:normal;
// font-family:sans-serif;
// -inkscape-font-specification:'sans-serif, Normal';
// font-variant-ligatures:no-common-ligatures;
// font-variant-position:normal;
// font-variant-caps:normal;
// font-variant-numeric:normal;
// font-variant-alternates:normal;
// font-variant-east-asian:normal;
// font-feature-settings:normal;
// font-variation-settings:normal;
// text-indent:0;
// text-align:end;
// text-decoration-line:none;
// text-decoration-style:solid;
// text-decoration-color:#000000;
// letter-spacing:normal;
// word-spacing:normal;
// text-transform:none;
// writing-mode:lr-tb;
// direction:ltr;
// text-orientation:mixed;
// dominant-baseline:auto;
// baseline-shift:baseline;
// text-anchor:end;
// white-space:normal;
// shape-padding:0;
// shape-margin:0;
// inline-size:0;
// opacity:1;
// vector-effect:none;
// fill:#000000;
// fill-opacity:1;
// stroke-width:79.0001;
// stroke-linecap:square;
// stroke-linejoin:miter;
// stroke-miterlimit:4;
// stroke-dasharray:none;
// stroke-dashoffset:0;
// stroke-opacity:1;
// -inkscape-stroke:none;
// stop-color:#000000;
// stop-opacity:1"
