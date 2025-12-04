/*
 * SPDX-License-Identifier: ISC
 *
 * Copyright (C) 2018 Michael Drake <tlsa@netsurf-browser.org>
 */

#include <stdlib.h>
#include <stdio.h>

#include "svg.h"

#include <cyaml/cyaml.h>

const signal_settings_t default_sig_settings = {
    .height         = 1,
    .slope_width    = 0.1,
    .margin         = 1.5,
    .line_thickness = 0.1,
    .text_margin    = 0.5,
    .font_size      = 1,
};

const svg_settings_t default_settings = {
    .waveform_width = 100,
    .max_time       = 0,
    .global         = default_sig_settings,
    .signals        = NULL,
    .signal_count   = 0
};


typedef struct {
    char *id;
    float thickness;
    float slope_width;
    char *color;
} var_style_t;


typedef struct {
    size_t styles_count;
    var_style_t *styles;
    float katten;
} styles_t;


/******************************************************************************
 * CYAML schema to tell libcyaml about both expected YAML and data structure.
 *
 * (Our CYAML schema is just a bunch of static const data.)
 ******************************************************************************/

static const cyaml_schema_field_t style_field[] = {
    CYAML_FIELD_STRING_PTR(
        "id", CYAML_FLAG_POINTER, var_style_t, id, 0, CYAML_UNLIMITED
    ),
    CYAML_FIELD_FLOAT(
        "thickness", CYAML_FLAG_STRICT | CYAML_FLAG_OPTIONAL,
        var_style_t, thickness
    ),
    CYAML_FIELD_FLOAT(
        "slope width", CYAML_FLAG_STRICT | CYAML_FLAG_OPTIONAL,
        var_style_t, slope_width
    ),
    CYAML_FIELD_STRING_PTR(
        "color", CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
        var_style_t, color, 0, CYAML_UNLIMITED
    ),

    CYAML_FIELD_END
};



/* CYAML value schema for the top level mapping. */
static const cyaml_schema_value_t style_schema = {
    CYAML_VALUE_MAPPING(CYAML_FLAG_DEFAULT,
        var_style_t, style_field),
};



static const cyaml_schema_field_t top_field[] = {
    CYAML_FIELD_SEQUENCE(
        "styles", CYAML_FLAG_POINTER,
        styles_t, styles,
        &style_schema, 0, CYAML_UNLIMITED),
    CYAML_FIELD_FLOAT("katten", CYAML_FLAG_DEFAULT, styles_t, katten),
    CYAML_FIELD_END
};

/* CYAML value schema for the top level mapping. */
static const cyaml_schema_value_t top_schema = {
    CYAML_VALUE_MAPPING(CYAML_FLAG_POINTER,
        styles_t, top_field),
};


static const cyaml_config_t config = {
    .log_fn = cyaml_log,            /* Use the default logging function. */
    .mem_fn = cyaml_mem,            /* Use the default memory allocator. */
    .log_level = CYAML_LOG_WARNING, /* Logging errors and warnings only. */
};


/* Main entry point from OS. */
int loadYaml(char *file_name)
{
    cyaml_err_t err;
    styles_t *n;



    /* Load input file. */
    err = cyaml_load_file(file_name, &config,
            &top_schema, (cyaml_data_t **)&n, NULL);
    if (err != CYAML_OK) {
        fprintf(stderr, "ERROR: %s\n", cyaml_strerror(err));
        return EXIT_FAILURE;
    }

    /* Free the data */
    cyaml_free(&config, &top_schema, n, 0);

    return EXIT_SUCCESS;
}


svg_settings_t initSvgSettings(size_t count) {
    svg_settings_t ret = default_settings;
    ret.signals = malloc(count * sizeof(signal_settings_t));
    for (size_t i = 0; i < count; i++) {
        ret.signals[i] = IGNORED_SETTINGS;
    }
    ret.signal_count = count;

    return ret;
}


void mergeSettings(svg_settings_t *sett) {
    signal_settings_t *sigs = sett->signals;
    for (size_t i = 0; i < sett->signal_count; i++) {
        sigs[i] = getSettings(sett, i);
    }
}


signal_settings_t getSettings(svg_settings_t *sett, size_t index) {
    signal_settings_t ret = sett->global;
    if (sett->signals == NULL) return ret;

    signal_settings_t s = sett->signals[index];
    const signal_settings_t ignore = IGNORED_SETTINGS;


    if (s.show != ignore.show) ret.show = s.show;
    if (s.height != ignore.height) ret.height = s.height;
    if (s.slope_width != ignore.slope_width) ret.slope_width = s.slope_width;
    if (s.margin != ignore.margin) ret.margin = s.margin;
    if (s.line_thickness != ignore.line_thickness)
        ret.line_thickness = s.line_thickness;
    if (s.text_margin != ignore.text_margin) ret.text_margin = s.text_margin;
    if (s.font_size != ignore.font_size) ret.font_size = s.font_size;

    return ret;
}
