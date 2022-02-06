#include <dc_posix/dc_stdlib.h>
#include <dc_posix/dc_regex.h>
#include <dc_posix/dc_string.h>
#include <dc_posix/dc_wordexp.h>
#include <dc_util/strings.h>
#include "command.h"

// 126 tests

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

    err_regex = state->err_redirect_regex;
    command_line = dc_strdup(env, err, command->line);
    matched = dc_regexec(env, err_regex, command_line, 1, &match, 0);
    if (matched == 0)
    {
        char *str;
        regoff_t length;

        length = match.rm_eo - match.rm_so;
        str = dc_malloc(env , err, (size_t) length + 1);
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
            printf("we_wordv[0] at err: %s\n", exp.we_wordv[0]);
            command->stderr_file = dc_strdup(env, err, exp.we_wordv[0]);

            wordfree(&exp);
        } else {
            command->stderr_file = dc_strdup(env, err, str);
        }

        dc_free(env, str, sizeof(str));
    }

    out_regex = state->out_redirect_regex;
    matched = dc_regexec(env, out_regex, command_line, 1, &match, 0);
    if (matched == 0)
    {
        char *str;
        regoff_t length;

        length = match.rm_eo - match.rm_so;
        str = malloc((size_t) length + 1);
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
            command->stdout_file = dc_strdup(env, err, exp.we_wordv[0]);
            wordfree(&exp);
        } else {
            command->stdout_file = dc_strdup(env, err, str);
        }

        dc_free(env, str, sizeof(str));
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
            command->stdin_file = dc_strdup(env, err, exp.we_wordv[0]);
            wordfree(&exp);
        } else {
            command->stdin_file = dc_strdup(env, err, str);
        }

        dc_free(env, str, sizeof(str));
    }

    wordexp_t exp;
    dc_wordexp(env, err, command_line, &exp, 0);

    command->argc = exp.we_wordc;
    command->argv = dc_malloc(env, err, sizeof(char *) * (exp.we_wordc + 2));
    command->argv[0] = NULL;
    if (command->argc > 1) {
        for (int i = 1; i < (int) exp.we_wordc; ++i) {
            command->argv[i] = dc_strdup(env, err, exp.we_wordv[i]);
        }

    }
    command->argv[exp.we_wordc] = NULL;

    if (exp.we_wordv[0] != NULL) {
        command->command = dc_strdup(env, err, exp.we_wordv[0]);
    }

    wordfree(&exp);
    dc_free(env, command_line, strlen(command_line) + 1);
}

char *trim_string_left_arrow(const struct dc_posix_env *env, char *str) {
    str = dc_str_left_trim(env, str);
    int counter;

    counter = 0;

    while (counter < 1) {
        if (*str == 62 || *str == 60) {
            counter++;
        }
        dc_memmove(env, str, str+1, strlen(str));
    }

    if (*str == 62 || *str == 60) {
        dc_memmove(env, str, str+1, strlen(str));
    }
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

