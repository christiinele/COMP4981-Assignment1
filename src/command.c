#include <dc_posix/dc_stdlib.h>
#include <dc_posix/dc_regex.h>
#include <dc_posix/dc_string.h>
#include <dc_posix/dc_wordexp.h>
#include "command.h"

// 126 tests

int redirect_err(const struct dc_posix_env *env, struct dc_error *err,
                  struct state *state, struct command *command, char *command_line);

int redirect_out(const struct dc_posix_env *env, struct dc_error *err,
                 struct state *state, struct command *command, char *command_line);

int redirect_in(const struct dc_posix_env *env, struct dc_error *err,
                 struct state *state, struct command *command, char *command_line);

/**
 * Parse the command. Take the command->line and use it to fill in all of the fields.
 *
 * @param env the posix environment.
 * @param err the error object.
 * @param state the current state, to set the fatal_error and access the command line and regex for redirection.
 * @param command the command to parse.
 */
void parse_command(const struct dc_posix_env *env, struct dc_error *err,
                   struct state *state, struct command *command) {
    char *command_line = NULL;

    redirect_err(env, err, state, command, command_line);
    redirect_out(env, err, state, command, command_line);
    redirect_in(env, err, state, command, command_line);

//    wordexp_t exp;
//    int status;
//    status = wordexp(command_line, &exp, 0);
//
//    command->argc = exp.we_wordc;
//    command->argv = dc_malloc(env, err, sizeof(char *) * (exp.we_wordc + 2));
//    for (int i = 1; i < (int) exp.we_wordc; ++i) {
//        command->argv[i] = dc_strdup(env, err, exp.we_wordv[i]);
//    }
//    command->command = dc_strdup(env, err, exp.we_wordv[0]);
//    free(command_line);
//
//    wordfree(&exp);


//    regex_t *stderr_redirect_regex;
//    char *command_line;
//    regmatch_t match;
//    int matched;
//
//    command_line = dc_strdup(env, err, command->line);
//
//    matched = dc_regexec(env, state->err_redirect_regex, command_line, 1, &match, 0);
//    if (matched == 0) {
//        if (dc_strstr(env, command_line, ">>") != NULL) {
//            command->stderr_overwrite = true;
//            wordexp_t exp;
//            int status;
//            status = wordexp("~", &exp, 0);
////            if (command->stderr_file != NULL) {
////                dc_free(env, command->stderr_file, sizeof(command->stderr_file));
////            }
//            command->stderr_file = dc_strdup(env, err, exp.we_wordv[0]);
//            wordfree(&exp);
//
//        }
//    }
//
//    matched = dc_regexec(env, state->out_redirect_regex, command_line, 1, &match, 0);
//    if (matched == 0) {
//        if (dc_strstr(env, command_line, ">>") != NULL) {
//            command->stderr_overwrite = true;
//            wordexp_t exp;
//            int status;
//            status = wordexp("~", &exp, 0);
////            if (command->stderr_file != NULL) {
////                dc_free(env, command->stderr_file, sizeof(command->stderr_file));
////            }
//            command->stdout_file = dc_strdup(env, err, exp.we_wordv[0]);
//            wordfree(&exp);
//
//        }
//    }
//
//    matched = dc_regexec(env, state->in_redirect_regex, command_line, 1, &match, 0);
//    if (matched == 0) {
////        if (dc_strstr(env, command_line, ">>") != NULL) {
////            command->stderr_overwrite = true;
////            wordexp_t exp;
////            int status;
////            status = wordexp("~", &exp, 0);
////            command->stderr_file = dc_strdup(env, err, exp.we_wordv[0]);
////        }
//            wordexp_t exp;
//            int status;
//            status = wordexp("~", &exp, 0);
//            command->stdin_file = dc_strdup(env, err, exp.we_wordv[0]);
//        wordfree(&exp);
//
//    }
//
//    wordexp_t exp;
//    int status;
//    status = wordexp(command_line, &exp, 0);
//
//    command->argc = exp.we_wordc;
//    command->argv = dc_malloc(env, err, sizeof(char *) * (exp.we_wordc + 2));
//    for (int i = 1; i < (int) exp.we_wordc; ++i) {
//        command->argv[i] = dc_strdup(env, err, exp.we_wordv[i]);
//    }
//    command->command = dc_strdup(env, err, exp.we_wordv[0]);
//    free(command_line);

//    wordfree(&exp);
}

int redirect_in(const struct dc_posix_env *env, struct dc_error *err,
                 struct state *state, struct command *command, char *command_line) {
    regmatch_t match;
    int matched;

//    command_line = dc_strdup(env, err, command->line);

    matched = dc_regexec(env, state->err_redirect_regex, command_line, 1, &match, 0);
    if (matched == 0) {
        char *str;
        regoff_t length;

        length = match.rm_eo - match.rm_so;
        str = dc_malloc(env, err, ((size_t) (length + 1)));
        strncpy(str, &command_line[match.rm_so], (unsigned long) length);
        str[length] = '\0';
        printf("%s\n", str);

        wordexp_t exp;
        int status;
        status = wordexp("~", &exp, 0);
        command->stderr_file = dc_strdup(env, err, exp.we_wordv[0]);

//        if (dc_strstr(env, str, ">>") != NULL) {
//            command->stderr_overwrite = true;
//            wordexp_t exp;
//            int status;
//            status = wordexp("~", &exp, 0);
//            command->stderr_file = dc_strdup(env, err, exp.we_wordv[0]);
//        }
        free(str);



    }
    free(command_line);

}


int redirect_err(const struct dc_posix_env *env, struct dc_error *err,
                  struct state *state, struct command *command, char *command_line) {
    regmatch_t match;
    int matched;

    command_line = dc_strdup(env, err, command->line);

    matched = dc_regexec(env, state->err_redirect_regex, command_line, 1, &match, 0);
    if (matched == 0) {
        char *str;
        regoff_t length;

        length = match.rm_eo - match.rm_so;
        str = dc_malloc(env, err, ((size_t) (length + 1)));
        strncpy(str, &command_line[match.rm_so], (unsigned long) length);
        str[length] = '\0';
        printf("%s\n", str);


        if (dc_strstr(env, str, ">>") != NULL) {
            command->stderr_overwrite = true;
            wordexp_t exp;
            int status;
            status = wordexp("~", &exp, 0);
            command->stderr_file = dc_strdup(env, err, exp.we_wordv[0]);
        }
        free(str);

//            command->stderr_overwrite = true;
//            wordexp_t exp;
//            int status;
//            status = wordexp("~", &exp, 0);
////            if (command->stderr_file != NULL) {
////                dc_free(env, command->stderr_file, sizeof(command->stderr_file));
////            }
//            command->stderr_file = dc_strdup(env, err, exp.we_wordv[0]);
//            wordfree(&exp);


    }
    free(command_line);

}

int redirect_out(const struct dc_posix_env *env, struct dc_error *err,
                 struct state *state, struct command *command, char *command_line) {
    regmatch_t match;
    int matched;

//    command_line = dc_strdup(env, err, command->line);

    matched = dc_regexec(env, state->out_redirect_regex, command_line, 1, &match, 0);
    if (matched == 0) {
        char *str;
        regoff_t length;

        length = match.rm_eo - match.rm_so;
        str = dc_malloc(env, err, ((size_t) (length + 1)));
        strncpy(str, &command_line[match.rm_so], (unsigned long) length);
        str[length] = '\0';
        printf("%s\n", str);


        if (dc_strstr(env, str, ">>") != NULL) {
            command->stdout_overwrite = true;
            wordexp_t exp;
            int status;
            status = wordexp("~", &exp, 0);
            command->stdout_file = dc_strdup(env, err, exp.we_wordv[0]);
        }
        free(str);

//            command->stderr_overwrite = true;
//            wordexp_t exp;
//            int status;
//            status = wordexp("~", &exp, 0);
////            if (command->stderr_file != NULL) {
////                dc_free(env, command->stderr_file, sizeof(command->stderr_file));
////            }
//            command->stderr_file = dc_strdup(env, err, exp.we_wordv[0]);
//            wordfree(&exp);


    }
    free(command_line);

}


/**
 *
 * @param env
 * @param command
 */
void destroy_command(const struct dc_posix_env *env, struct command *command) {
    if (command->line != NULL) {
        dc_free(env, command->line, sizeof(char *));
        command->line = NULL;
    }

    if (command->command != NULL) {
        dc_free(env, command->command, sizeof(char *));
        command->command = NULL;
    }

    command->argc = 0;

    if (command->argv != NULL) {
        int pos = 0;
        while (command->argv[pos] != NULL) {
            dc_free(env, command->argv[pos++], sizeof(char *));
        }
        dc_free(env, command->argv, sizeof(char**));
        command->argv = NULL;
    }

    if (command->stdin_file != NULL) {
        dc_free(env, command->stdin_file, sizeof(char *));
        command->stdin_file = NULL;
    }

    if (command->stdout_file != NULL) {
        dc_free(env, command->stdout_file, sizeof(char *));
        command->stdout_file = NULL;
    }

    command->stdout_overwrite = false;

    if (command->stderr_file != NULL) {
        dc_free(env, command->stderr_file, sizeof(char *));
        command->stderr_file = NULL;
    }

    command->stderr_overwrite = false;
    command->exit_code = 0;


}
