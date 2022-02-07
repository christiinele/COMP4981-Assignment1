#include <dc_posix/dc_stdlib.h>
#include <dc_posix/dc_regex.h>
#include <dc_posix/dc_string.h>
#include <dc_posix/dc_wordexp.h>
#include <dc_util/strings.h>
#include "command.h"


char *trim_string_left_arrow(const struct dc_posix_env *env, char *str);

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

    regex_t *in_regex;
    regex_t *out_regex;
    regex_t *err_regex;
    regmatch_t match;
    int matched;
    char* command_line;
    size_t original_argc;

    err_regex = state->err_redirect_regex;
    command_line = dc_strdup(env, err, command->line);
    if (dc_error_has_error(err)) {
        state->fatal_error = true;
    }
    matched = dc_regexec(env, err_regex, command_line, 1, &match, 0);
    if (matched == 0)
    {
        char *str;
        regoff_t length;

        length = match.rm_eo - match.rm_so;
        str = dc_malloc(env , err, (size_t) length + 1);
        if (dc_error_has_error(err)) {
            state->fatal_error = true;
        }
        dc_strncpy(env, str, &command_line[match.rm_so], (size_t) length);
        str[length] = '\0';
        command_line[match.rm_so] = '\0';

        if (dc_strstr(env, str, ">>") != NULL) {
            command->stderr_overwrite = true;
        }

        str = trim_string_left_arrow(env, str);

        if (dc_strstr(env, str, "~") != NULL) {
            wordexp_t exp;
            dc_wordexp(env, err, str, &exp, 0);
            if (dc_error_has_error(err)) {
                state->fatal_error = true;
            }
            command->stderr_file = dc_strdup(env, err, exp.we_wordv[0]);
            if (dc_error_has_error(err)) {
                state->fatal_error = true;
            }

//            wordfree(&exp);
        } else {
            command->stderr_file = dc_strdup(env, err, str);
            if (dc_error_has_error(err)) {
                state->fatal_error = true;
            }
        }

        dc_free(env, str, strlen(str));
    }

    out_regex = state->out_redirect_regex;
    matched = dc_regexec(env, out_regex, command_line, 1, &match, 0);
    if (matched == 0)
    {
        char *str;
        regoff_t length;

        length = match.rm_eo - match.rm_so;
        str = dc_malloc(env, err, (size_t) length + 1);
        if (dc_error_has_error(err)) {
            state->fatal_error = true;
        }
        dc_strncpy(env, str, &command_line[match.rm_so], (size_t) length);
        command_line[match.rm_so] = '\0';
        str[length] = '\0';

        if (dc_strstr(env, str, ">>") != NULL) {
            command->stdout_overwrite = true;
        }

        str = trim_string_left_arrow(env, str);


        if (dc_strstr(env, str, "~") != NULL) {
            wordexp_t exp;
            dc_wordexp(env, err, str, &exp, 0);
            if (dc_error_has_error(err)) {
                state->fatal_error = true;
            }
            command->stdout_file = dc_strdup(env, err, exp.we_wordv[0]);
            if (dc_error_has_error(err)) {
                state->fatal_error = true;
            }
//            wordfree(&exp);
        } else {
            command->stdout_file = dc_strdup(env, err, str);
            if (dc_error_has_error(err)) {
                state->fatal_error = true;
            }
        }

        dc_free(env, str, strlen(str));
    }

    in_regex = state->in_redirect_regex;
    matched = dc_regexec(env, in_regex, command_line, 1, &match, 0);
    if (matched == 0)
    {
        char *str;
        regoff_t length;

        length = match.rm_eo - match.rm_so;
        str = malloc((size_t) length + 1);
        dc_strncpy(env, str, &command_line[match.rm_so], (size_t) length);
        str[length] = '\0';
        command_line[match.rm_so] = '\0';

        str = trim_string_left_arrow(env, str);
        str = dc_str_left_trim(env, str);

        if (dc_strstr(env, str, "~") != NULL) {
            wordexp_t exp;
            dc_wordexp(env, err, str, &exp, 0);
            if (dc_error_has_error(err)) {
                state->fatal_error = true;
            }
            command->stdin_file = dc_strdup(env, err, exp.we_wordv[0]);
//            wordfree(&exp);
        } else {
            command->stdin_file = dc_strdup(env, err, str);
        }

        dc_free(env, str, strlen(str));
    }

    wordexp_t exp;
    dc_wordexp(env, err, command_line, &exp, 0);
    if (dc_error_has_error(err)) {
        state->fatal_error = true;
    }

    original_argc = command->argc;
    command->argc = exp.we_wordc;
    if (original_argc > 0) {
        for (int i = 0; command->argv[i]; ++i) {
            dc_free(env, command->argv[i], strlen(command->argv[i]));
        }
        dc_free(env, command->argv, original_argc);
    }
    command->argv = dc_malloc(env, err, exp.we_wordc + 2);
    if (dc_error_has_error(err)) {
        state->fatal_error = true;
    }
    command->argv[0] = NULL;
    if (command->argc > 1) {
        for (int i = 1; i < (int) exp.we_wordc; ++i) {
            command->argv[i] = dc_strdup(env, err, exp.we_wordv[i]);
            if (dc_error_has_error(err)) {
                state->fatal_error = true;
            }
        }

    }
    command->argv[exp.we_wordc] = NULL;

    if (exp.we_wordv[0] != NULL) {
        if (command->command != NULL) {
            dc_free(env, command->command, strlen(command->command));
        }
        command->command = dc_strdup(env, err, exp.we_wordv[0]);
        if (dc_error_has_error(err)) {
            state->fatal_error = true;
        }
    }

//    wordfree(&exp);
    if (command_line != NULL) {
        dc_free(env, command_line, strlen(command_line));
    }
}

char *trim_string_left_arrow(const struct dc_posix_env *env, char *str) {
    str = dc_str_left_trim(env, str);
    int counter;

    counter = 0;

    while (counter < 1) {
        if (str[0] == '>' || str[0] == '<') {
            counter++;
        }
//        if (dc_strcmp(env, &str[0], "<") == 0 || dc_strcmp(env, &str[0], ">") == 0) {
//            counter++;
//        }
        dc_memmove(env, str, str+1, strlen(str));
    }

    if (str[0] == '>' || str[0] == '<') {
        dc_memmove(env, str, str+1, strlen(str));
    }
//    if (dc_strcmp(env, &str[0], "<") == 0 || dc_strcmp(env, &str[0], ">") == 0) {
//        dc_memmove(env, str, str+1, strlen(str));
//    }
    str = dc_str_left_trim(env, str);


    return str;
}




/**
 *
 * @param env
 * @param command
 */
void destroy_command(const struct dc_posix_env *env, struct command *command) {
    if (command->line != NULL) {
        dc_free(env, command->line, strlen(command->line));
        command->line = NULL;
    }

    if (command->command != NULL) {
        dc_free(env, command->command, strlen(command->command));
        command->command = NULL;
    }


    if (command->argv != NULL) {
        int pos = 0;
        while (command->argv[pos] != NULL) {
            dc_free(env, command->argv[pos], strlen(command->argv[pos]));
            pos++;
        }
        dc_free(env, command->argv, command->argc);
        command->argv = NULL;
    }
    command->argc = 0;

    if (command->stdin_file != NULL) {
        dc_free(env, command->stdin_file, strlen(command->stdin_file));
        command->stdin_file = NULL;
    }

    if (command->stdout_file != NULL) {
        dc_free(env, command->stdout_file, strlen(command->stdout_file));
        command->stdout_file = NULL;
    }

    command->stdout_overwrite = false;

    if (command->stderr_file != NULL) {
        dc_free(env, command->stderr_file, strlen(command->stderr_file));
        command->stderr_file = NULL;
    }

    command->stderr_overwrite = false;
    command->exit_code = 0;


}

