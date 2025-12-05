/*
 * SPDX-License-Identifier: ISC
 *
 * Copyright (C) 2018 Michael Drake <tlsa@netsurf-browser.org>
 */

#include <stdlib.h>
#include <stdio.h>

#include "signal_settings.h"

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



static const cyaml_schema_field_t style_field[] = {
    CYAML_FIELD_STRING_PTR(
        "id", CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
        signal_settings_t, id, 0, CYAML_UNLIMITED
    ),
    CYAML_FIELD_STRING_PTR(
        "path", CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
        signal_settings_t, path, 0, CYAML_UNLIMITED
    ),

    CYAML_FIELD_BOOL(
        "show", CYAML_FLAG_STRICT | CYAML_FLAG_OPTIONAL,
        signal_settings_t, show
    ),

    CYAML_FIELD_FLOAT(
        "height", CYAML_FLAG_STRICT | CYAML_FLAG_OPTIONAL,
        signal_settings_t, height
    ),
    CYAML_FIELD_FLOAT(
        "slope-width", CYAML_FLAG_STRICT | CYAML_FLAG_OPTIONAL,
        signal_settings_t, slope_width
    ),
    CYAML_FIELD_FLOAT(
        "margin", CYAML_FLAG_STRICT | CYAML_FLAG_OPTIONAL,
        signal_settings_t, margin
    ),
    CYAML_FIELD_FLOAT(
        "line-thickness", CYAML_FLAG_STRICT | CYAML_FLAG_OPTIONAL,
        signal_settings_t, line_thickness
    ),
    CYAML_FIELD_FLOAT(
        "text-margin", CYAML_FLAG_STRICT | CYAML_FLAG_OPTIONAL,
        signal_settings_t, text_margin
    ),
    CYAML_FIELD_FLOAT(
        "font-size", CYAML_FLAG_STRICT | CYAML_FLAG_OPTIONAL,
        signal_settings_t, font_size
    ),

    CYAML_FIELD_STRING_PTR(
        "line-flolor", CYAML_FLAG_POINTER,
        signal_settings_t, line_color, 0, CYAML_UNLIMITED
    ),
    CYAML_FIELD_STRING_PTR(
        "text-color", CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
        signal_settings_t, text_color, 0, CYAML_UNLIMITED
    ),

    CYAML_FIELD_END
};

// Subtly different in that there's no ID or Path
static const cyaml_schema_field_t global_style_field[] = {
    CYAML_FIELD_BOOL(
        "show", CYAML_FLAG_STRICT,
        signal_settings_t, show
    ),

    CYAML_FIELD_FLOAT(
        "height", CYAML_FLAG_STRICT,
        signal_settings_t, height
    ),
    CYAML_FIELD_FLOAT(
        "slope-width", CYAML_FLAG_STRICT,
        signal_settings_t, slope_width
    ),
    CYAML_FIELD_FLOAT(
        "margin", CYAML_FLAG_STRICT,
        signal_settings_t, margin
    ),
    CYAML_FIELD_FLOAT(
        "line-thickness", CYAML_FLAG_STRICT,
        signal_settings_t, line_thickness
    ),
    CYAML_FIELD_FLOAT(
        "text-margin", CYAML_FLAG_STRICT,
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
};



svg_settings_t *loadSettingsFromFile(char *file_name, vcd_t vcd) {
    cyaml_err_t err;
    svg_settings_t *settings;


    err = cyaml_load_file(
        file_name, &config, &top_schema, (void **)&settings, NULL
    );

    if (err != CYAML_OK) {
        fprintf(stderr, "ERROR: %s\n", cyaml_strerror(err));
        return NULL;
    }

    // These are not guaranteed to be in the correct order at all
    // so we need to put them in the correct order
    for (size_t i = 0; i < settings->signal_count; i++) {
        signal_settings_t *sig = settings->signals + i;
        var_t *var = NULL;
        if (sig->id != NULL) var = getVarById(&vcd, sig->id);
        else if (sig->path != NULL) var = getVarByPath(&vcd, sig->path);
        if (var == NULL) {
            printf("Error: Could not find signal %zu "
                "of yaml file by path or id.\n", i);
            return NULL;
        }
        var->style = sig;
    }

    // Set the style for all variables that weren't mentioned to the global one.
    for (size_t i = 0; i < vcd.var_count; i++) {
        if (vcd.vars[i].style == NULL) {
            vcd.vars[i].style = &settings->global;
        }
    }

    return settings;
}

void freeSettings(svg_settings_t *settings) {
    cyaml_free(&config, &top_schema, settings, 0);

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

//TODO: You fucked this up. Think of a way to do this better.
//TODO: Remember: var_t now holds a pointer to a signal_settigns_t.
//TODO: svg_settings_t should basically only be used for storage;
//TODO: settings.signals should basically never be accessed outside of this file.
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
