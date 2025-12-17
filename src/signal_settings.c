#include <stdlib.h>
#include <stdio.h>

#include "signal_settings.h"
#include "vcd.h"

#include <cyaml/cyaml.h>
#include <string.h>

#define YAML_SCHEMA_LOCATION "~/.local/share/vcd2svg/yaml-schema.json"

// I hate that I have to do this but libcyaml doesn't have another option.
// And I'm not about to use libyaml for this.
static double default_slope_width = 0.1;
static double default_margin      = 1.5;
static double default_text_margin = 0.5;
static uint16_t default_fixed_point_shift = 0;

static uint8_t true_bool = 1;

const signal_settings_t default_sig_settings = {
    .show           = &true_bool,
    .height         = 1,
    .slope_width    = &default_slope_width,
    .margin         = &default_margin,
    .text_margin    = &default_text_margin,
    .line_thickness = 0.1,
    .font_size      = 1,
    .line_color     = "black",
    .text_color     = "black",

    .show_value         = &true_bool,
    .radix              = 16,
    .fixed_point_shift  = &default_fixed_point_shift,
    .value_font_size    = 0.8,
    .value_text_color   = "black"
};

const svg_settings_t default_settings = {
    .waveform_width = 100,
    .max_time       = 0,
    .global         = default_sig_settings,
    .signals        = NULL,
    .signal_count   = 0
};



static const cyaml_schema_field_t style_field[] = {
    CYAML_FIELD_STRING_PTR(
        "id", CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
        signal_settings_t, id, 0, CYAML_UNLIMITED
    ),
    CYAML_FIELD_STRING_PTR(
        "path", CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
        signal_settings_t, path, 0, CYAML_UNLIMITED
    ),

    CYAML_FIELD_BOOL_PTR(
        // Ptr because we need to differentiate between unset and 0
        "show", CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
        signal_settings_t, show
    ),

    CYAML_FIELD_FLOAT(
        "height", CYAML_FLAG_STRICT | CYAML_FLAG_OPTIONAL,
        signal_settings_t, height
    ),
    CYAML_FIELD_FLOAT_PTR(
        // Ptr because we need to differentiate between unset and 0
        "slope-width", CYAML_FLAG_OPTIONAL | CYAML_FLAG_POINTER,
        signal_settings_t, slope_width
    ),
    CYAML_FIELD_FLOAT_PTR(
        "margin", CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
        signal_settings_t, margin
    ),
    CYAML_FIELD_FLOAT(
        "line-thickness", CYAML_FLAG_STRICT | CYAML_FLAG_OPTIONAL,
        signal_settings_t, line_thickness
    ),
    CYAML_FIELD_FLOAT_PTR(
        "text-margin", CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
        signal_settings_t, text_margin
    ),
    CYAML_FIELD_FLOAT(
        "font-size", CYAML_FLAG_STRICT | CYAML_FLAG_OPTIONAL,
        signal_settings_t, font_size
    ),

    CYAML_FIELD_STRING_PTR(
        "line-color", CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
        signal_settings_t, line_color, 0, CYAML_UNLIMITED
    ),
    CYAML_FIELD_STRING_PTR(
        "text-color", CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
        signal_settings_t, text_color, 0, CYAML_UNLIMITED
    ),

    CYAML_FIELD_BOOL_PTR(
        "show-value", CYAML_FLAG_OPTIONAL | CYAML_FLAG_POINTER,
        signal_settings_t, show_value
    ),
    CYAML_FIELD_INT(
        "radix", CYAML_FLAG_OPTIONAL,
        signal_settings_t, radix
    ),
    CYAML_FIELD_INT_PTR(
        "fixed-point-shift", CYAML_FLAG_OPTIONAL | CYAML_FLAG_POINTER,
        signal_settings_t, fixed_point_shift
    ),
    CYAML_FIELD_FLOAT(
        "value-font-size", CYAML_FLAG_OPTIONAL,
        signal_settings_t, value_font_size
    ),
    CYAML_FIELD_STRING_PTR(
        "value-text-color", CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
        signal_settings_t, value_text_color, 0, CYAML_UNLIMITED
    ),

    CYAML_FIELD_END
};

// Subtly different in that there's no ID or Path
static const cyaml_schema_field_t global_style_field[] = {
    CYAML_FIELD_BOOL_PTR(
        "show", CYAML_FLAG_POINTER,
        signal_settings_t, show
    ),

    CYAML_FIELD_FLOAT(
        "height", CYAML_FLAG_STRICT,
        signal_settings_t, height
    ),
    CYAML_FIELD_FLOAT_PTR(
        "slope-width", CYAML_FLAG_POINTER,
        signal_settings_t, slope_width
    ),
    CYAML_FIELD_FLOAT_PTR(
        "margin", CYAML_FLAG_POINTER,
        signal_settings_t, margin
    ),
    CYAML_FIELD_FLOAT(
        "line-thickness", CYAML_FLAG_STRICT,
        signal_settings_t, line_thickness
    ),
    CYAML_FIELD_FLOAT_PTR(
        "text-margin", CYAML_FLAG_POINTER,
        signal_settings_t, text_margin
    ),
    CYAML_FIELD_FLOAT(
        "font-size", CYAML_FLAG_STRICT,
        signal_settings_t, font_size
    ),

    CYAML_FIELD_STRING_PTR(
        "line-color", CYAML_FLAG_POINTER,
        signal_settings_t, line_color, 0, CYAML_UNLIMITED
    ),
    CYAML_FIELD_STRING_PTR(
        "text-color", CYAML_FLAG_POINTER,
        signal_settings_t, text_color, 0, CYAML_UNLIMITED
    ),


    CYAML_FIELD_BOOL_PTR(
        "show-value", CYAML_FLAG_POINTER,
        signal_settings_t, show_value
    ),
    CYAML_FIELD_INT(
        "radix", CYAML_FLAG_DEFAULT,
        signal_settings_t, radix
    ),
    CYAML_FIELD_INT_PTR(
        "fixed-point-shift", CYAML_FLAG_POINTER,
        signal_settings_t, fixed_point_shift
    ),
    CYAML_FIELD_FLOAT(
        "value-font-size", CYAML_FLAG_DEFAULT,
        signal_settings_t, value_font_size
    ),
    CYAML_FIELD_STRING_PTR(
        "value-text-color", CYAML_FLAG_POINTER,
        signal_settings_t, value_text_color, 0, CYAML_UNLIMITED
    ),

    CYAML_FIELD_END
};



/* CYAML value schema for the top level mapping. */
static const cyaml_schema_value_t style_schema = {
    CYAML_VALUE_MAPPING(CYAML_FLAG_DEFAULT,
        signal_settings_t, style_field),
};



static const cyaml_schema_field_t top_field[] = {
    CYAML_FIELD_SEQUENCE_COUNT(
        "signals", CYAML_FLAG_POINTER, svg_settings_t, signals, signal_count,
        &style_schema, 0, CYAML_UNLIMITED
    ),
    CYAML_FIELD_MAPPING(
        "global", CYAML_FLAG_STRICT, svg_settings_t, global, global_style_field
    ),
    CYAML_FIELD_FLOAT(
        "waveform-width", CYAML_FLAG_STRICT, svg_settings_t, waveform_width
    ),
    CYAML_FIELD_INT(
        "max-time", CYAML_FLAG_STRICT, svg_settings_t, max_time
    ),
    CYAML_FIELD_FLOAT(
        "time-unit-width", CYAML_FLAG_STRICT, svg_settings_t, time_unit_width
    ),
    CYAML_FIELD_END
};

/* CYAML value schema for the top level mapping. */
static const cyaml_schema_value_t top_schema = {
    CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
        svg_settings_t, top_field),
};


static const cyaml_config_t config = {
    .log_fn = cyaml_log,            /* Use the default logging function. */
    .mem_fn = cyaml_mem,            /* Use the default memory allocator. */
    .log_level = CYAML_LOG_WARNING,
    // .log_level = CYAML_LOG_DEBUG,

    .flags = CYAML_CFG_STYLE_BLOCK
};



svg_settings_t *loadSettingsFromFile(char *file_name) {
    cyaml_err_t err;
    svg_settings_t *settings;


    err = cyaml_load_file(
        file_name, &config, &top_schema, (void **)&settings, NULL
    );

    if (err != CYAML_OK) {
        fprintf(stderr, "ERROR: %s\n", cyaml_strerror(err));
        return NULL;
    }

    return settings;
}

void freeSettings(svg_settings_t *settings) {
    cyaml_free(&config, &top_schema, settings, 0);

}


svg_settings_t initSvgSettings(size_t count) {
    svg_settings_t ret = default_settings;

    ret.signals = calloc(count, sizeof(signal_settings_t));
    ret.signal_count = count;

    return ret;
}


void mergeStyles(signal_settings_t *dest, signal_settings_t *from) {
    // Please tell me if you know a better way to do this.

    printf("%s:%s  %s\t<-\t", dest->id, dest->path, dest->line_color);
    printf("%s:%s  %s\n", from->id, from->path, from->line_color);

    if (dest->show == NULL) dest->show = from->show;
    if (dest->height == 0) dest->height = from->height;
    if (dest->slope_width == NULL) dest->slope_width = from->slope_width;
    if (dest->margin == NULL) dest->margin = from->margin;
    if (dest->text_margin == NULL) dest->text_margin = from->text_margin;
    if (dest->line_thickness == 0) dest->line_thickness = from->line_thickness;
    if (dest->font_size == 0) dest->font_size = from->font_size;
    if (dest->line_color == NULL) dest->line_color = from->line_color;
    if (dest->text_color == NULL) dest->text_color = from->text_color;

    if (dest->show_value == NULL) dest->show_value = from->show_value;
    if (dest->radix == 0) dest->radix = from->radix;
    if (dest->fixed_point_shift == NULL)
        dest->fixed_point_shift = from->fixed_point_shift;
    if (dest->value_font_size == 0)
        dest->value_font_size = from->value_font_size;
    if (dest->value_text_color == NULL)
        dest->value_text_color = from->value_text_color;

}


static void sanitize(char *to, char *from) {
    while(*from) {
        if (*from == '"' || *from == '\\') {
            *to = '\\';
            *(to + 1) = *from;
            to += 2;
        }
        else {
            *to = *from;
            to++;
        }
        from++;
    }
    *to = '\0';
}


// I know libcyaml supports outputting YAML files but:
// - It doesn't support outputting comments
// - It doesn't seem to support NOT outputting optional values
// - It's a very simple yaml file
void writeTemplate(char *file_name, vcd_t vcd) {
    FILE* file = fopen(file_name, "w");

    fprintf(file,
        "# yaml-language-server: $schema=" YAML_SCHEMA_LOCATION "\n"
        "# Template generated and used by vcd-to-svg\n"
        "# https://github.com/kwekker/vcd-to-svg\n"
        "\n"
        "waveform-width: 0     # Use time-unit-width\n"
        "max-time: 0           # Output everything\n"
        "time-unit-width: 0.5  # Time units have a width of 0.1\n"
        "\n"
        "# The style to fall back to if a signal doesn't have one specified:\n"
        "global:\n"
        "  show: true\n"
        "  height: 1\n"
        "  slope-width: 0.1\n"
        "  margin: 1.5\n"
        "  text-margin: 0.5\n"
        "  text-color: black\n"
        "  font-size: 1\n"
        "  line-thickness: 0.1\n"
        "  line-color: black\n"
        "\n"
        "  show-value: true\n"
        "  radix: 16\n"
        "  fixed-point-shift: 0\n"
        "  value-font-size: 0.8\n"
        "  value-text-color: black\n"
        "\n"
        "# Here you can specify the style of each individual signal:\n"
        "signals:\n"
    );

    for (size_t i = 0; i < vcd.var_count; i++) {
        var_t *var = &vcd.vars[i];

        char id[MAX_ID_LENGTH * 2 + 1];
        char name[strlen(var->name) * 2 + 1];

        sanitize(id, var->id);
        sanitize(name, var->name);


        fprintf(file,
            "  - path: \"%s\"\n"
            "    id: \"%s\"\n\n", name, id

        );
    }


}