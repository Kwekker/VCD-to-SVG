/*
 * SPDX-License-Identifier: ISC
 *
 * Copyright (C) 2018 Michael Drake <tlsa@netsurf-browser.org>
 */

#include <stdlib.h>
#include <stdio.h>

#include <cyaml/cyaml.h>

/******************************************************************************
 * C data structure for storing a project plan.
 *
 * This is what we want to load the YAML into.
 ******************************************************************************/



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
    // CYAML_FIELD_INT(
    //     "id", CYAML_FLAG_DEFAULT, var_style_t, id
    // ),
    // CYAML_FIELD_INT(
    //     "thickness", CYAML_FLAG_DEFAULT, var_style_t, thickness
    // ),
    CYAML_FIELD_END
};

// static const cyaml_schema_field_t style_field[] = {
//     CYAML_FIELD_INT(
//         "hoeveelheid", CYAML_FLAG_STRICT, struct flinder, hoeveelheid
//     ),
//     CYAML_FIELD_INT(
//         "miauw", CYAML_FLAG_STRICT, struct flinder, miauw
//     ),
//     CYAML_FIELD_END
// };


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
int main(int argc, char *argv[])
{
    cyaml_err_t err;
    styles_t *n;
    enum {
        ARG_PROG_NAME,
        ARG_PATH_IN,
        ARG__COUNT,
    };

    /* Handle args */
    if (argc != ARG__COUNT) {
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "  %s <INPUT>\n", argv[ARG_PROG_NAME]);
        return EXIT_FAILURE;
    }

    /* Load input file. */
    err = cyaml_load_file(argv[ARG_PATH_IN], &config,
            &top_schema, (cyaml_data_t **)&n, NULL);
    if (err != CYAML_OK) {
        fprintf(stderr, "ERROR: %s\n", cyaml_strerror(err));
        return EXIT_FAILURE;
    }

    /* Use the data. */
    printf("katten: %f\n", n->katten);
    printf("count: %zu\n", n->styles_count);
    for (size_t i = 0; i < n->styles_count; i++) {
        var_style_t style = n->styles[i];
        // printf("Id: %s\n\tthickness= %f\n", style.id, style.thickness);
        printf("Id: %s\n\tthickness= %f\n", style.id, style.thickness);
        printf("\tcolor: %s\n\tslope_width: %f\n", style.color, style.slope_width);
    }

    /* Free the data */
    cyaml_free(&config, &top_schema, n, 0);

    return EXIT_SUCCESS;
}