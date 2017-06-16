#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <upng.h>

extern char *
load_png(const char * filename, unsigned * width, unsigned * height) {
    upng_t * upng = upng_new_from_file(filename);
    if (!upng) goto error;
    upng_decode(upng);
    if (upng_get_error(upng) != UPNG_EOK) goto error;
    *width = upng_get_width(upng);
    *height = upng_get_height(upng);
    size_t buffer_size = *width * *height * 3;
    char * buffer = (char *)malloc(buffer_size);
    if (!buffer) exit(-1);
    const unsigned char * image_data = upng_get_buffer(upng);
    enum upng_format format = upng_get_format(upng);
    if (UPNG_RGBA8 != format) goto error;
    memcpy(buffer, image_data, buffer_size);
    upng_free(upng);
    return buffer;
error:
    printf("Can't read png file: %s\n", filename);
    exit(-1);
}
