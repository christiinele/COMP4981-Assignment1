#include <dc_posix/dc_string.h>
#include <wordexp.h>
#include <dc_posix/dc_wordexp.h>
#include <dc_posix/dc_unistd.h>
#include <dc_posix/dc_stdlib.h>
#include <dc_posix/dc_stdio.h>
#include <dc_util/path.h>
#include "builtins.h"


/**
 * Change the working directory.
 * ~ is converted to the users home directory.
 * - no arguments is converted to the users home directory.
 * The command->exit_code is set to 0 on success or error->errno_code on failure.
 *
 * @param env the posix environment.
 * @param err the error object
 * @param command the command information
 * @param errstream the stream to print error messages to
 */
void builtin_cd(const struct dc_posix_env *env, struct dc_error *err,
                struct command *command, FILE *errstream) {
    char *path;
    int chdir_status;

    if (command->argv[1]) {
        path = dc_strdup(env, err, command->argv[1]);
    } else {
        path = dc_strdup(env, err, "~/");
    }

    if (dc_strstr(env, path, "~") != NULL) {
        dc_expand_path(env, err, &path, "~");
    }


    chdir_status = dc_chdir(env, err, path);
    if (chdir_status == -1) {

        char *message;
        char *dir = dc_strdup(env, err, path);

        switch (err->err_code) {
        case EACCES: case ELOOP: case ENAMETOOLONG:
                message = dc_strdup(env, err, err->message);
                dir = dc_realloc(env, err, dir, strlen(dir) + strlen(message));
                sprintf(dir, "%s: %s", dir, message);
                dc_free(env, message, strlen(message));
                fprintf(errstream, "%s\n", dir);
                break;
            case ENOENT:
                dir = dc_realloc(env, err, dir, strlen(dir) + 16);
                sprintf(dir, "%s: does not exist", dir);
                fprintf(errstream, "%s\n", dir);

                break;
            case ENOTDIR:
                dir = dc_realloc(env, err, dir, strlen(dir) + 20);
                sprintf(dir, "%s: is not a directory", dir);
                fprintf(errstream, "%s\n", dir);
                break;
        }

        free(dir);


        command->exit_code = 1;
    } else {
        command->exit_code = 0;
    }



    dc_free(env, path, strlen(path));
}
