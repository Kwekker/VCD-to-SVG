#include "svg.h"
#include "vcd.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>




#define DEFAULT_STROKE_STYLE "stroke-width:%f;fill:none;stroke:%s;stroke-linecap:round;stroke-linejoin:round;stroke-dasharray:none;stroke-opacity:1"

static void outputSignal(
FILE* file, var_t var, svg_settings_t sett, double start_pos_y
);
static inline uint8_t isZero(char *val);

// These are inline because they're only run once.
static inline void outputVectorSignal(
    FILE* file, svg_settings_t sett, signal_settings_t sig, value_pair_t val,
    double start_pos_y, double prev_time, uint8_t was_zero
);
static inline void outputBitFlip(
    FILE* file, svg_settings_t sett, signal_settings_t sig,
    double start_pos_y, uint8_t new_state, size_t time
);

static inline void outputSignalTitle(
    FILE *file, var_t var, double start_pos_y
);

static inline void outputValueText(
    FILE *file, svg_settings_t sett, signal_settings_t sig, value_pair_t val,
    double end_time, double start_pos_y
);



// This function assumes settings.signals
// is completely allocated and initialized.
void writeSVG(FILE *file, vcd_t vcd, svg_settings_t settings) {

    //* Setting the viewbox
    // There are 3 values that help set the width of the viewbox.
    // We only need 2, sometimes only one. Here are all combinations:
    // Waveform-width = ww; max-time = mt; time-unit-width = tuw.
    // In this table, 1 means that the value is any nonzero positive number.
    //    ww mt tuw
    // 0: 0  0  0:   Error, not enough information
    // 1: 0  0  1:   Entire waveform is outputted, scaled by using tuw.
    // 2: 0  1  0:   Error, not enough information
    // 3: 0  1  1:   Waveform is outputted up until mt, scaled by using tuw.
    // 4: 1  0  0:   Entire waveform is outputted, scaled by using ww.
    // 5: 1  0  1:   Error: This is a weird one and not yet supported.
    // 6: 1  1  0:   Waveform is outputted up until mt,
    //               scaled by using a combination of ww and mt.
    // 7: 1  1  1:   Same as above, the tuw is redundant. A warning is emitted.


    // Cases 0 and 1
    if (settings.max_time == 0 && settings.waveform_width == 0) {
        // Case 0
        if (settings.time_unit_width == 0) {
            printf(
                "Error: At least one of "
                "waveform-width, max-time, and time-unit-width "
                "should be more than 0. Default is:\n"
                "waveform-width: 0\nmax-time: 0\ntime-unit-width: 0.1\n"
            );
            return;
        }

        settings.max_time = vcd.max_time;
        settings.waveform_width = vcd.max_time * settings.time_unit_width;
    }
    // Case 7 warning. Will now be handled as case 6.
    else if (settings.time_unit_width != 0 && settings.waveform_width != 0) {
        printf("Warning: time-unit-width is ignored "
            "since waveform-width is nonzero.\n");
    }

    // Make sure we don't draw a bunch of empty space.
    if (settings.max_time > vcd.max_time) settings.max_time = vcd.max_time;

    // Cases 4 and 5
    if (settings.max_time == 0) {
        // Prevent case 5
        if (settings.time_unit_width) {
            printf("Error: waveform-width and time-unit-width "
                "set exclusively. This is not yet supported.\n");
            return;
        }

        // Case 4
        settings.max_time = vcd.max_time;
    }

    // Cases 2 and 3
    if (settings.waveform_width == 0){
        if (settings.time_unit_width == 0)
            settings.waveform_width = settings.max_time;
        else {
            settings.waveform_width = settings.time_unit_width * vcd.max_time;
            settings.max_time = vcd.max_time;
        }
    }


    // Calculate viewbox
    double width = settings.waveform_width;
    double height = 0;

    double last_margin = 0;
    for (size_t i = 0; i < vcd.var_count; i++) {
        signal_settings_t sig = vcd.vars[i].style;
        if (sig.show) {
            printf("adding %f and %f to height\n", sig.height, *sig.margin);
            height += sig.height;
            height += *sig.margin;
            last_margin = *sig.margin;
        }
    }
    height -= last_margin;
    if (height == 0) {
        printf("No signals to draw!\n");
        return;
    }



    fprintf(file,
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n"
        "<!-- Created with VCD-to-SVG converter "
        "(https://www.github.com/kwekker/vcd-to-svg/) -->\n"
        "<svg width=\"%fmm\" height=\"%fmm\" viewBox=\"0 0 %f %f\"\n"
        "version=\"1.1\"\n"
        "xmlns:inkscape=\"http://www.inkscape.org/namespaces/inkscape\"\n"
        "xmlns=\"http://www.w3.org/2000/svg\"\n"
        "xmlns:svg=\"http://www.w3.org/2000/svg\">\n\t",
        width, height, width, height
    );

    fprintf(file, "<g id=\"layer1\">\n");
    double y_pos = 0;
    for (size_t i = 0; i < vcd.var_count; i++) {

        var_t var = vcd.vars[i];
        signal_settings_t sig = var.style;
        if (!sig.show) continue;
        if (var.copy_of != NULL) {
            var.value_count = var.copy_of->value_count;
            var.values = var.copy_of->values;
        }
        printf("Var %zu: %s\n", i, var.name);
        outputSignal(file, var, settings, y_pos);
        y_pos += sig.height + *sig.margin;
    }
    printf("Done!\n");

    fprintf(file, "</g>\n");
    fprintf(file, "</svg>");

}

void outputSignal(
    FILE* file, var_t var, svg_settings_t sett, double start_pos_y
) {
    if (var.size == 0) return;
    if (var.value_count == 0) {
        printf("Warning: signal %s has 0 values!\n", var.name);
        return;
    }

    signal_settings_t sig = var.style;

    // Make the SVG nicer to work with in inkscape by setting a label.
    fprintf(file, "<g id=\"var_%ld\" inkscape:label=\"%s\">\n",
        ftell(file), var.name
    );
    printf("Drawing %s\n", var.name);
    printf("\tline color is %s\n", sig.line_color);
    fprintf(file, "<path style=\"" DEFAULT_STROKE_STYLE "\"\n",
        sig.line_thickness, sig.line_color
    );
    fprintf(file, "id=\"waveform_%ld\"\n", ftell(file));
    fprintf(file, "inkscape:label=\"vector waveform\"\n");


    double xs = sett.waveform_width / sett.max_time;

    // current_state is 0 if all values are 0, otherwise it is 1.
    // It's used for drawing the waveform,
    // since it should be ___ when all bits are 0
    uint8_t was_zero = 0;
    if (var.size == 1) was_zero = var.values[0].value_char == '1';
    else was_zero = isZero(var.values[0].value_string);

    printf("\tWas zero: %d\n", was_zero);

    size_t prev_time = 0;

    // Always start the svg 'cursor' at the bottom of the signal.
    fprintf(file, "d=\"M 0 %f ",
        start_pos_y + sig.height
    );

    size_t line_count = 0;


    for (size_t j = 1; j < var.value_count; j++) {
        value_pair_t *val = &var.values[j];
        if (val->time > sett.max_time) break;


        uint8_t is_zero = 0;
        if (var.size == 1) is_zero = val->value_char == '0';
        else is_zero = isZero(val->value_string);

        // Continue if the value didn't change.
        if (is_zero && was_zero) {
            printf("\tContinuing because was and is zero\n");
            continue;
        }
        char* prev = var.values[j-1].value_string;
        if (var.size > 1 && (
            !is_zero && memcmp(val->value_string, prev, var.size) == 0
        )) {
            // Mark value as duplicate for later when we're emitting text.
            val->duplicate = 1;
            printf("\tContinuing because it's a duplicate\n");
            continue;
        }

        // Keep track of line count to handle an edge case that's described a
        // bit further.
        line_count++;
        if (var.size > 1) {
            outputVectorSignal(
                file, sett, sig, *val, start_pos_y, prev_time, was_zero
            );
        }
        else {
            outputBitFlip(file, sett, sig, start_pos_y, !is_zero, val->time);
        }
        prev_time = val->time;
        was_zero = is_zero;
    }

    // Draw a final line if the signal got cut off,
    // or if we haven't drawn anything yet.
    if (prev_time != sett.max_time || line_count == 0) {
        printf("Drawing final part because %zu != %zu or %zu == 0\n",
            prev_time, sett.max_time, line_count
        );
        double final_time = sett.max_time;
        fprintf(file, "H %f", xs*final_time);
        if (!was_zero && var.size > 1) {
            fprintf(file, "M %f %f H %f ",
                xs*prev_time + *sig.slope_width / 2.0, start_pos_y,
                xs*final_time
            );
        }
    }

    fprintf(file, "\"/>\n");

    if (sig.show_value && var.size > 1) {
        for (size_t j = 0; j < var.value_count; j++) {
            value_pair_t val = var.values[j];
            // Skip duplicate values here as well.
            if (val.duplicate) continue;

            // Measure the end time of this value so that we don't put text
            // over other values.
            double end_time;
            if (j == var.value_count - 1) end_time = sett.max_time;
            else {
                // Find next non-duplicate value to figure out the end-time.
                size_t next = j + 1;
                while (var.values[next].duplicate) next++;
                end_time = var.values[next].time;
            }

            // Finally, end-time could be beyond the image border, so:
            if (end_time > sett.max_time) end_time = sett.max_time;


            outputValueText(
                file, sett, sig, val, end_time, start_pos_y
            );
        }
    }


    outputSignalTitle(file, var, start_pos_y);

    fprintf(file, "</g>\n");
}


static inline void outputBitFlip(
    FILE* file, svg_settings_t sett, signal_settings_t sig,
    double start_pos_y, uint8_t new_state, size_t time
) {
    double xs = sett.waveform_width / sett.max_time;

    fprintf(file, "H %f L %f %f ",
        xs*time - *sig.slope_width / 2.0,      // x1
        xs*time + *sig.slope_width / 2.0,      // x2
        start_pos_y + sig.height * !new_state // y2
    );
}



// This function with an unreasonably large amount of arguments
// was the best thing I could come up with here, unfortunately.
static inline void outputVectorSignal(
    FILE* file, svg_settings_t sett, signal_settings_t sig, value_pair_t val,
    double start_pos_y, double prev_time, uint8_t was_zero
) {
    double xs = sett.waveform_width / sett.max_time;
    uint8_t is_zero = isZero(val.value_string);

    // There are 3 possibilities:
    //   1. Transition from a value to 0
    //   2. Transition from a value to another value
    //   3. Transition from 0 to a value

    if (prev_time == 0) prev_time = -*sig.slope_width / 2;

    if (is_zero && was_zero) {
        printf("\tIs zero and was zero\n");
        return;
    }
    printf("\tValue: %s\n", val.value_string);


    if (is_zero) { // From a value to zero
        printf("\tValue to zero\n");
        // Bottom line that ends  in the middle of /
        fprintf(file, "H %f L %f %f ",
            xs*val.time - *sig.slope_width / 2.0,
            xs*val.time, start_pos_y + sig.height / 2.0
        );

        // Top line that goes down like ‾‾‾‾\_
        fprintf(file, "M %f %f H %f L %f %f ",
            xs*prev_time + *sig.slope_width / 2.0, start_pos_y,
            xs*val.time - *sig.slope_width / 2.0,
            xs*val.time + *sig.slope_width / 2.0, start_pos_y + sig.height
        );
    }
    else if (!is_zero && !was_zero) { // From a value to another value
        printf("\tValue to value\n");
        // Bottom line that goes up like ____/‾
        fprintf(file, "H %f L %f %f",
            xs*val.time - *sig.slope_width / 2.0,
            xs*val.time + *sig.slope_width / 2.0,
            start_pos_y
        );
        // Top line that goes down like  ‾‾‾‾\_
        fprintf(file, "M %f %f H %f L %f %f ",
            xs*prev_time + *sig.slope_width / 2.0, start_pos_y,
            xs*val.time - *sig.slope_width / 2.0,
            xs*val.time + *sig.slope_width / 2.0,
            start_pos_y + sig.height
        );
    }
    else if (!is_zero && was_zero) { // From zero to a value
        printf("\tZero to value\n");
        // Bottom line that goes up like ____/‾
        fprintf(file, "H %f L %f %f ",
            xs*val.time - *sig.slope_width / 2.0,
            xs*val.time + *sig.slope_width / 2.0, start_pos_y
        );
        // Bottom line that goes down like    \_
        // It starts at the midpoint of the diagonal line.
        fprintf(file, "M %f %f L %f %f ",
            xs*val.time, start_pos_y + sig.height / 2.0,
            xs*val.time + *sig.slope_width / 2.0, start_pos_y + sig.height
        );
    }
    else {
        printf("\tExcuse me\n");
    }
}



static inline void outputSignalTitle(
    FILE *file, var_t var, double start_pos_y
) {
    signal_settings_t sig = var.style;

    fprintf(file, "<text style=\"font-size:%f;", sig.font_size);
    fprintf(file, "color:%s;text-anchor:end;\" ", sig.text_color);
    fprintf(file, "dominant-baseline=\"central\" ");

    double y_pos = start_pos_y + sig.height / 2;// + sig.font_size / 2;

    fprintf(file, "x=\"%f\" y=\"%f\" id=\"text%ld\">",
        - *sig.text_margin, y_pos,
        ftell(file)
    );

    char *name = strrchr(var.name, '/') + 1;
    if (name == NULL) name = var.name;
    fprintf(file, "%s", name);

    fprintf(file, "</text>");
}

// TODO: Look at this page:
// https://www.balisage.net/Proceedings/vol26/html/Birnbaum01/BalisageVol26-Birnbaum01.html

// This function assumes sig.show_value is true.
static inline void outputValueText(
    FILE *file, svg_settings_t sett, signal_settings_t sig, value_pair_t val,
    double end_time, double start_pos_y
) {
    double xs = sett.waveform_width / sett.max_time;

    fprintf(file, "<text style=\"font-size:%f;", sig.value_font_size);
    fprintf(file, "color:%s;text-anchor:start;\" ", sig.value_text_color);
    fprintf(file, "dominant-baseline=\"central\" ");

    double y_pos = start_pos_y + sig.height / 2; // + sig.value_font_size / 2;
    fprintf(file, "x=\"%f\" y=\"%f\" id=\"value%ld\">",
        xs * val.time + *sig.slope_width, y_pos,
        ftell(file)
    );


    uint64_t number = strtol(val.value_string, NULL, 2);

    switch(sig.radix) {
        case 2:
            fprintf(file, "%s", val.value_string);
            break;
        case 8:
            fprintf(file, "%lo", number);
            break;
        case 10:
            fprintf(file, "%ld", number);
            break;
        case 16:
            fprintf(file, "%lx", number);
            break;
        default:
            fprintf(file, "Error base");
            break;

    }
    fprintf(file, "</text>");

}



static inline uint8_t isZero(char *val) {
    while(*val) {
        if (*val != '0') return 0;
        val++;
    }
    return 1;
}
