
#include "fonts.h"

#include <stdint.h>
#include <string.h>
#include <stdio.h>


// This is a really, really stupid meathod of doing this.
// A good method, however, requires me to read font files,
// which I'm not going to do (yet).
static double font[] = {
    15.258,19.242,22.078,40.219,30.539,45.609,37.43, 13.195,
    18.727,18.727,24,    40.219,15.258,17.32, 15.258,16.172,
    30.539,30.539,30.539,30.539,30.539,30.539,30.539,30.539,
    30.539,30.539,16.172,16.172,40.219,40.219,40.219,25.477,
    48,    32.836,32.93, 33.516,36.961,30.328,27.609,37.195,
    36.094,14.156,14.156,31.477,26.742,41.414,35.906,37.781,
    28.945,37.781,33.351,30.469,29.32, 35.133,32.836,47.461,
    32.883,29.32, 32.883,18.727,16.172,18.727,40.219,24,
    24,    29.414,30.469,26.391,30.469,29.531,16.898,30.469,
    30.422,13.336,13.336,27.797,13.336,46.758,30.422,29.367,
    30.469,30.469,19.734,25.008,18.82, 30.422,28.406,39.258,
    28.406,28.406,25.195,30.539,16.172,30.539,40.219
};

static const double multiplier = 49.8217868;
static const double avg_char_width = 28.66235789 / multiplier;
static const double max_char_width = 48 / multiplier;
static const double min_char_width = 13.195 / multiplier;

// TODO: Look at this page:
// https://www.balisage.net/Proceedings/vol26/html/Birnbaum01/BalisageVol26-Birnbaum01.html


double approximateTextWidth(char *text, double font_height) {
    char *c = text;
    double width = 0;

    while(*c) {
        // Check for invalid characters
        if (*c < 32 || *c > 126) return -1;

        width += font[*c - 32];
        c++;
    }

    return font_height * width / multiplier;
}


int limitTextWidth(char *text, double font_height, double max_width) {

    printf("Limiting text width of \"%s\" to %f\n", text, max_width);
    double text_width = approximateTextWidth(text, font_height);
    if (text_width < 0) return -1;
    if (text_width < max_width) return 0;

    printf("\t%f is too long. Making a copy.\n", text_width);


    // Make a copy of the text.
    size_t text_length = strlen(text);
    size_t copy_length = text_length;
     // + 4  for possible ellipsis (...) that's added,
     // as well as the terminating '\0'
    char copy[text_length + 4];
    strcpy(copy, text);

    printf("\tCopy is %s\n", copy);


    // Overestimate the amount of characters we need to shave off.
    double difference = text_width - max_width;
    int32_t excess_chars = difference / (font_height * avg_char_width);

    strcpy(copy + copy_length - excess_chars - 2, "..");
    copy_length -= excess_chars;

    double copy_width = approximateTextWidth(copy, font_height);
    printf("\tEstimated %d excess characters. It's now %f.\n", excess_chars, copy_width);
    printf("\tCopy is %s\n", copy);

    // Keep adding characters until we exceed the maximum length.
    while(copy_width < max_width) {
        // Make the first '.' a letter from text
        // Make the terminating '\0' a '.'
        // Make the character after the '\0' the new terminating '\0'
        copy[copy_length - 2] = text[copy_length - 2];
        copy[copy_length] = '.';
        copy[copy_length + 1] = '\0';
        copy_length++;
        copy_width = approximateTextWidth(copy, font_height);
        printf("\tMade it longer to %s\n", copy);
        printf("\tWidth is now %f\n", copy_width);
    }

    while(copy_width > max_width) {
        copy[copy_length - 3] = '.';
        copy[copy_length - 1] = '\0';
        copy_length--;
        copy_width = approximateTextWidth(copy, font_height);
        printf("\tMade it shorter to %s\n", copy);
        printf("\tWidth is now %f\n", copy_width);
    }

    // The string should now be the longest it can be within the bounds of
    // the max_width.
    strcpy(text, copy);

    return 0;
}


#ifdef TEST_FONTS
#include <stdio.h>

int main(int argc, char *argv[]) {

    if (argc < 2) {
        printf("Gonna need an argument\n");
        return -1;
    }

    double width = approximateTextWidth(argv[1], 1);
    if (width < 0) printf("Something went wrong with the text\n");
    printf("%f\n", width);

    limitTextWidth(argv[1], 1, width * 0.75);

    printf("Limited text: %s\n", argv[1]);

    return 0;
}
#endif