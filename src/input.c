#include <dc_util/strings.h>
#include "input.h"
#include "dc_posix/dc_stdio.h"
#include <string.h>

/**
 * Read the command line from the user.
 *
 * @param env the posix environment.
 * @param err the error object
 * @param stream The stream to read from (eg. stdin)
 * @param line_size the maximum characters to read.
 * @return The command line that the user entered.
 */
char *read_command_line(const struct dc_posix_env *env, struct dc_error *err, FILE *stream, size_t *line_size) {
    char *line;
    ssize_t read;

    if (stream == NULL || line_size == 0) {
        return NULL;
    }

//    while ((read = dc_getline(env, err, &line, line_size, stream)) != -1) {
//    printf("Retrieved line of length %zu :\n", read);
//    printf("%s\n", line);
//    }

    dc_getline(env, err, &line, line_size, stream);
    printf("Message: %s\n", line);
    dc_str_trim(env, line);
    printf("Line size: %zu\n", strlen(line));
    *line_size = strlen(line);
    return line;

}
