#include <unistd.h>
#include <stdlib.h>
#include <dc_posix/dc_stdlib.h>
#include <dc_posix/dc_regex.h>
#include <dc_util/filesystem.h>
#include <dc_posix/dc_stdio.h>
#include <dc_posix/dc_string.h>
#include <dc_util/strings.h>
#include "shell_impl.h"
#include "util.h"
#include "input.h"
#include "builtins.h"

char *remove_both_end_char(const struct dc_posix_env *env, struct dc_error *err, char* message);

/**
 * Set up the initial state:
 *  - in_redirect_regex  "[ \t\f\v]<.*"
 *  - out_redirect_regex "[ \t\f\v][1^2]?>[>]?.*"
 *  - err_redirect_regex "[ \t\f\v]2>[>]?.*"
 *  - path the PATH environ var separated into directories
 *  - prompt the PS1 environ var or "$" if PS1 not set
 *  - max_line_length the value of _SC_ARG_MAX (see sysconf)
 *
 * @param env the posix environment.
 * @param err the error object
 * @param arg the current struct state
 * @return READ_COMMANDS or INIT_ERROR
 */
int init_state(const struct dc_posix_env *env, struct dc_error *err, void *arg){

    struct state *state_arg;
    char * path;
    char **path_array;
    char *ps1_env_var;

    state_arg = (struct state *) arg;

    state_arg->max_line_length = (size_t) sysconf(_SC_ARG_MAX);

    state_arg->in_redirect_regex = dc_malloc(env, err, sizeof (regex_t));
    state_arg->out_redirect_regex = dc_malloc(env, err, sizeof (regex_t));
    state_arg->err_redirect_regex = dc_malloc(env, err, sizeof (regex_t));
    dc_regcomp(env, err, state_arg->in_redirect_regex, "[ \\t\\f\\v]<.*", REG_EXTENDED);
    dc_regcomp(env, err, state_arg->out_redirect_regex, "[ \\t\\f\\v][1^2]?>[>]?.*", REG_EXTENDED);
    dc_regcomp(env, err, state_arg->err_redirect_regex, "[ \\t\\f\\v]2>[>]?.*", REG_EXTENDED);

    path = get_path(env, err);
    path_array = parse_path(env, err, path);
    state_arg->path = path_array;

    ps1_env_var = get_prompt(env, err);
    state_arg->prompt = ps1_env_var;

    state_arg->current_line_length = 0;
    state_arg->current_line = NULL;
    state_arg->command = NULL;
    state_arg->fatal_error = false;


    return READ_COMMANDS;
}

/**
 * Free any dynamically allocated memory in the state and sets variables to NULL, 0 or false.
 *
 * @param env the posix environment.
 * @param err the error object
 * @param arg the current struct state
 * @return DC_FSM_EXIT
 */
int destroy_state(const struct dc_posix_env *env, struct dc_error *err,
                  void *arg) {

    struct state *state_arg;
    size_t pos;

    state_arg = (struct state *) arg;

    state_arg->fatal_error = false;
    dc_free(env, state_arg->command, sizeof(struct command));
    state_arg->current_line_length = 0;
    dc_free(env, state_arg->current_line, sizeof(char *));
    state_arg->max_line_length = 0;
    dc_free(env, state_arg->prompt, sizeof(char *));
    dc_free(env, state_arg->err_redirect_regex, sizeof(regex_t));
    dc_free(env, state_arg->in_redirect_regex, sizeof(regex_t));
    dc_free(env, state_arg->out_redirect_regex, sizeof(regex_t));

    state_arg->command = NULL;
    state_arg->current_line = NULL;
    state_arg->prompt = NULL;
    state_arg->err_redirect_regex = NULL;
    state_arg->in_redirect_regex = NULL;
    state_arg->out_redirect_regex = NULL;

    pos = 0;
    if (state_arg->path != NULL) {
        while (state_arg->path[pos] != NULL) {
            pos++;
        }
    }


    dc_strs_destroy_array(env, pos, state_arg->path);


    dc_free(env, state_arg->path, sizeof(char **));
    state_arg->path = NULL;

    state_arg->stdin = stdin;
    state_arg->stderr = stderr;
    state_arg->stdout = stdout;



    return DC_FSM_EXIT;
}

/**
 * Reset the state for the next read (see do_reset_state).
 *
 * @param env the posix environment.
 * @param err the error object
 * @param arg the current struct state
 * @return READ_COMMANDS
 */
int reset_state(const struct dc_posix_env *env, struct dc_error *err,
                void *arg) {
    struct state *state_arg;

    state_arg = (struct state *) arg;

    do_reset_state(env, err, state_arg);

    init_state(env, err, state_arg);

    return READ_COMMANDS;
}

/**
 * Prompt the user and read the command line (see read_command_line).
 * Sets the state->current_line and current_line_length.
 *
 * @param env the posix environment.
 * @param err the error object
 * @param arg the current struct state
 * @return SEPARATE_COMMANDS
 */
int read_commands(const struct dc_posix_env *env, struct dc_error *err,
                  void *arg) {
    struct state *state_arg;
    size_t line_length = 255;
    char *line;

    state_arg = (struct state *) arg;


    fprintf(state_arg->stdout, "[%s] %s", dc_get_working_dir(env, err), state_arg->prompt);

    if (dc_error_has_error(err))
    {
        state_arg->fatal_error = true;

        return ERROR;
    }

    line = read_command_line(env, err, state_arg->stdin, &line_length);
    if (dc_error_has_error(err))
    {
        state_arg->fatal_error = true;
        dc_free(env, line, sizeof(line));
        return ERROR;
    }

    if (state_arg->current_line != NULL) {
        dc_free(env, state_arg->current_line, sizeof(state_arg->current_line));
        state_arg->current_line = NULL;
    }

    state_arg->current_line = dc_strdup(env, err, line);

    if (dc_strlen(env, line) == 0) {
        dc_free(env, line, sizeof(line));
        return RESET_STATE;
    }

    state_arg->current_line_length = dc_strlen(env, line);
    dc_free(env, line, sizeof(line));


    return SEPARATE_COMMANDS;
}

/**
 * Separate the commands. In the current implementation there is only one command.
 * Sets the state->command.
 *
 * @param env the posix environment.
 * @param err the error object
 * @param arg the current struct state
 * @return PARSE_COMMANDS or SEPARATE_ERROR
 */
int separate_commands(const struct dc_posix_env *env, struct dc_error *err,
                      void *arg) {
    struct state *state_arg;
    struct command *state_command;
    struct command *new_command;

    int pos;

    state_arg = (struct state *) arg;

    state_command = state_arg->command;


//    if (state_command->line != NULL) {
//        dc_free(env, state_command->line, sizeof(state_command->line));
//    }
//
//    if (state_command->command != NULL) {
//        dc_free(env, state_command->command, sizeof(state_command->command));
//    }
//
//    if (state_command->stdin_file != NULL) {
//        dc_free(env, state_command->stdin_file, sizeof(state_command->stdin_file));
//    }
//
//    if (state_command->stdout_file != NULL) {
//        dc_free(env, state_command->stdout_file, sizeof(state_command->stdout_file));
//    }
//
//    if (state_command->stderr_file != NULL) {
//        dc_free(env, state_command->stderr_file, sizeof(state_command->stderr_file));
//    }
//
//
//    if (state_command->argv != NULL) {
//        pos = 0;
//        while (state_arg->path[pos] != NULL) {
//            dc_free(env, state_arg->path[pos++], sizeof(char *));
//        }
//        dc_free(env, state_command->argv, sizeof(char*));
//    }

//    destroy_command(env, state_command);

    dc_free(env, state_command, sizeof(struct command));

    new_command = dc_malloc(env, err, sizeof(struct command));

    if (dc_error_has_error(err))
    {
        state_arg->fatal_error = true;

        return ERROR;
    }

    state_arg->command = new_command;

    new_command->line = dc_strdup(env, err, state_arg->current_line);
    new_command->command = NULL;
    new_command->argc = 0;
    new_command->argv = NULL;
    new_command->stdin_file = NULL;
    new_command->stdout_file = NULL;
    new_command->stdout_overwrite = false;
    new_command->stderr_file = NULL;
    new_command->stderr_overwrite = false;
    new_command->exit_code = 0;




    return PARSE_COMMANDS;
}

/**
 * Parse the commands (see parse_command)
 *
 * @param env the posix environment.
 * @param err the error object
 * @param arg the current struct state
 * @return EXECUTE_COMMANDS or PARSE_ERROR
 */
int parse_commands(const struct dc_posix_env *env, struct dc_error *err,
                   void *arg) {
    struct state *state_arg;

    state_arg = (struct state *) arg;
    parse_command(env, err, state_arg, state_arg->command);

    if (dc_error_has_error(err))
    {
        state_arg->fatal_error = true;

        return ERROR;
    }

    return EXECUTE_COMMANDS;
}


/**
 * Run the command (see execute).
 * If the command->command is cd run builtin_cd
 *
 * @param env the posix environment.
 * @param err the error object
 * @param arg the current struct state
 * @return EXIT (if command->command is exit), RESET_STATE or EXECUTE_ERROR
 */
int execute_commands(const struct dc_posix_env *env, struct dc_error *err,
                     void *arg) {
    struct state *state_arg;

    state_arg = (struct state *) arg;


    if (dc_strstr(env, state_arg->command->command, "cd") != NULL) {
        builtin_cd(env, err, state_arg->command, state_arg->stderr);
    } else if (dc_strstr(env, state_arg->command->command, "exit") != NULL) {
        return EXIT;
    } else {
//        execute(env, err, state_arg->command, state_arg->path);
//
//        if (dc_error_has_error(err))
//        {
//            state_arg->fatal_error = true;
//        }
    }

    fprintf(state_arg->stdout, "%d\n", state_arg->command->exit_code);

    if (state_arg->fatal_error) {
        return ERROR;
    }

    return RESET_STATE;
}


/**
 * Handle the exit command (see do_reset_state)
 *
 * @param env the posix environment.
 * @param err the error object
 * @param arg the current struct state
 * @return DESTROY_STATE
 */
int do_exit(const struct dc_posix_env *env, struct dc_error *err, void *arg) {
    struct state *state_arg;

    state_arg = (struct state *) arg;
    do_reset_state(env, err, state_arg);
    return DESTROY_STATE;
}

/**
 * Print the error->message to stderr and reset the error (see dc_err_reset).
 *
 * @param env the posix environment.
 * @param err the error object
 * @param arg the current struct state
 * @return RESET_STATE or DESTROY_STATE (if state->fatal_error is true)
 */
int handle_error(const struct dc_posix_env *env, struct dc_error *err,
                 void *arg) {
    struct state *state_arg;

    state_arg = (struct state *) arg;


    if (state_arg->current_line == NULL) {
        fprintf(state_arg->stderr, "internal error (%d) %s\n", err->err_code, err->message);

    } else {

        fprintf(state_arg->stderr, "internal error (%d) %s: \"%s\"\n", err->err_code, err->message, state_arg->current_line);

    }

//    dc_free(env, trimmed, strlen(trimmed));

    if (state_arg->fatal_error) {
        return DESTROY_STATE;
    }


    return RESET_STATE;
}


char *remove_both_end_char(const struct dc_posix_env *env, struct dc_error *err, char* message) {

    char *trimmed;

    trimmed = dc_strndup(env, err, message, strlen(message) - 1);
    dc_memmove(env, trimmed, trimmed+1, strlen(trimmed));
    return trimmed;


}