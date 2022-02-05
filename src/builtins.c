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
    char *string;
    char *message;
    int chdir_status;

    if (command->argv[1] == NULL) {
        path = dc_strdup(env, err, "~/");
    } else {
        path = dc_strdup(env, err, command->argv[1]);
    }

    if (dc_strstr(env, path, "~") != NULL) {
        dc_expand_path(env, err, &path, "~");
    }


    chdir_status = dc_chdir(env, err, path);

    if (chdir_status == -1) {

        switch(err->err_code) {
            case (EACCES): case (ELOOP): case (ENAMETOOLONG):
                message = dc_strdup(env, err, err->message);
                break;
            case (ENOENT):
                message = dc_strdup(env, err, "does not exist");
                break;
            case (ENOTDIR):
                message = dc_strdup(env, err, "is not a directory");
                break;
        }
        string = dc_malloc(env, err, strlen(path) + strlen(message) + 2);
        sprintf(string, "%s: %s", path, message);
        fprintf(errstream, "%s\n", string);
        command->exit_code = 1;

        dc_free(env, string, strlen(string));
        dc_free(env, message   , strlen(message));
    } else {
        command->exit_code = 0;
    }

    dc_free(env, path, strlen(path));


//    char *path;
//    int chdir_status;
//
//    if (command->argv[1] == NULL) {
//        path = dc_strdup(env, err, "~/");
//    } else {
//        path = dc_strdup(env, err, command->argv[1]);
//    }
//
//    if (dc_strstr(env, path, "~") != NULL) {
//        dc_expand_path(env, err, &path, "~");
//    }

//    char *path;
//    char *expandedPath;
//    char *string;
//
//    if (command->argv[1] == NULL) {
//        path = dc_strdup(env, err, "~/");
//    } else {
//        path = dc_strdup(env, err, command->argv[1]);
//    }
//
//    dc_expand_path(env, err, &expandedPath, path);
//
//
//    chdir_status = dc_chdir(env, err, expandedPath);
//    if (chdir_status == -1) {
//        char *message;
//        char *dir = expandedPath;
//
//        switch (err->err_code) {
//        case EACCES: case ELOOP: case ENAMETOOLONG:
//                message = dc_strdup(env, err, err->message);
//                string = dc_realloc(env, err, dir, strlen(dir) + strlen(message));
//                sprintf(string, "%s: %s", dir, message);
//                fprintf(errstream, "%s\n", string);
//                break;
//            case ENOENT:
//                string = dc_realloc(env, err, dir, strlen(dir) + 16);
//                sprintf(string, "%s: does not exist", dir);
//                fprintf(errstream, "%s\n", string);
//
//                break;
//            case ENOTDIR:
//                string = dc_realloc(env, err, dir, strlen(dir) + 20);
//                sprintf(string, "%s: is not a directory", dir);
//                fprintf(errstream, "%s\n", string);
//                break;
//        }
//
//
//
//        command->exit_code = 1;
//    } else {
//        command->exit_code = 0;
//    }
    //dc_free(env, path, strlen(path));




}
