#include <unistd.h>
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
    if (dc_error_has_error(err)) {
        state_arg->fatal_error = true;
    }
    state_arg->out_redirect_regex = dc_malloc(env, err, sizeof (regex_t));
    if (dc_error_has_error(err)) {
        state_arg->fatal_error = true;
    }
    state_arg->err_redirect_regex = dc_malloc(env, err, sizeof (regex_t));
    if (dc_error_has_error(err)) {
        state_arg->fatal_error = true;
    }
    dc_regcomp(env, err, state_arg->in_redirect_regex, "[ \\t\\f\\v]<.*", REG_EXTENDED);
    if (dc_error_has_error(err)) {
        state_arg->fatal_error = true;
    }
    dc_regcomp(env, err, state_arg->out_redirect_regex, "[ \\t\\f\\v][1^2]?>[>]?.*", REG_EXTENDED);
    if (dc_error_has_error(err)) {
        state_arg->fatal_error = true;
    }
    dc_regcomp(env, err, state_arg->err_redirect_regex, "[ \\t\\f\\v]2>[>]?.*", REG_EXTENDED);
    if (dc_error_has_error(err)) {
        state_arg->fatal_error = true;
    }

    path = get_path(env, err);
    if (dc_error_has_error(err)) {
        state_arg->fatal_error = true;
    }
    path_array = parse_path(env, err, path);
    if (dc_error_has_error(err)) {
        state_arg->fatal_error = true;
    }
    state_arg->path = path_array;

    ps1_env_var = get_prompt(env, err);
    if (dc_error_has_error(err)) {
        state_arg->fatal_error = true;
    }
    state_arg->prompt = ps1_env_var;

    state_arg->current_line_length = 0;
    state_arg->current_line = NULL;
    state_arg->command = NULL;
    state_arg->fatal_error = false;

    dc_free(env, path, strlen(path));

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
    struct command *command;
    size_t pos;

    state_arg = (struct state *) arg;

    state_arg->fatal_error = false;
    //dc_free(env, state_arg->command, sizeof(struct command));
    state_arg->current_line_length = 0;
    if (state_arg->current_line != NULL) {
        dc_free(env, state_arg->current_line, strlen(state_arg->current_line));
    }
    state_arg->max_line_length = 0;
    if (state_arg->prompt != NULL) {
        dc_free(env, state_arg->prompt, strlen(state_arg->prompt));
    }
    pos = 0;
    if (state_arg->path != NULL) {
        while (state_arg->path[pos] != NULL) {
            pos++;
        }
        dc_strs_destroy_array(env, pos, state_arg->path);
        dc_free(env, state_arg->path, sizeof(char **));
    }

//    dc_free(env, state_arg->err_redirect_regex, sizeof(regex_t));
//    dc_free(env, state_arg->in_redirect_regex, sizeof(regex_t));
//    dc_free(env, state_arg->out_redirect_regex, sizeof(regex_t));
    free(state_arg->err_redirect_regex);
    free(state_arg->in_redirect_regex);
    free(state_arg->out_redirect_regex);

    command = state_arg->command;
    if (command != NULL) {
        if (command->line != NULL){
            dc_free(env, command->line, strlen(command->line));
        }
        if (command->command != NULL){
            dc_free(env, command->command, strlen(command->command));
        }
        for (size_t i = 0; i < command->argc; ++i) {
            if (command->argv[i] != NULL) {
                dc_free(env, command->argv[i], strlen(command->argv[i]));
            }
        }

        if (command->stdin_file != NULL){
            dc_free(env, command->stdin_file, strlen(command->stdin_file));
        }
        if (command->stdout_file != NULL){
            dc_free(env, command->stdout_file, strlen(command->stdout_file));
        }

        if (command->stderr_file != NULL){
            dc_free(env, command->stderr_file, strlen(command->stderr_file));
        }

        dc_free(env, command, sizeof(struct command));
    }


    state_arg->command = NULL;
    state_arg->current_line = NULL;
    state_arg->prompt = NULL;
    state_arg->err_redirect_regex = NULL;
    state_arg->in_redirect_regex = NULL;
    state_arg->out_redirect_regex = NULL;
    state_arg->path = NULL;





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
    char *cwd;
    struct state *state_arg;
    size_t line_length;
    char *line;
    char *prompt;
    size_t *line_length_pointer;
    size_t line_length_of_prompt;

    state_arg = (struct state *) arg;

    line_length_pointer = &line_length;

//    char* working_dir = dc_get_working_dir(env, err);
//
//    fprintf(state_arg->stdout, "[%s] %s", working_dir, state_arg->prompt);
//    dc_free(env, working_dir, strlen(working_dir));
    cwd = dc_get_working_dir(env, err);
    if (dc_error_has_error(err)) {
        state_arg->fatal_error = true;
    }
    // [current working directory] state.prompt
    line_length_of_prompt = 1 + strlen(cwd) + 2 + 1 + strlen(state_arg->prompt) + 1;
    prompt = dc_malloc(env, err, line_length_of_prompt);
    if (dc_error_has_error(err)) {
        state_arg->fatal_error = true;
    }
    sprintf(prompt, "[%s] %s", cwd, state_arg->prompt);

    fprintf(state_arg->stdout, "%s", prompt);
    dc_free(env, prompt, strlen(prompt));
    dc_free(env, cwd, strlen(cwd));

    fflush(state_arg->stdout);

    if (dc_error_has_error(err))
    {
        state_arg->fatal_error = true;

        return ERROR;
    }

    line = read_command_line(env, err, state_arg->stdin, line_length_pointer);
    if (dc_error_has_error(err))
    {
        state_arg->fatal_error = true;
//        dc_free(env, line, sizeof(line));
        return ERROR;
    }

    if (state_arg->current_line != NULL) {
//        dc_free(env, state_arg->current_line, sizeof(state_arg->current_line));
        state_arg->current_line = NULL;
    }

    state_arg->current_line = dc_strdup(env, err, line);

    if (dc_strlen(env, line) == 0) {
        return RESET_STATE;
    }

    state_arg->current_line_length = dc_strlen(env, line);

    dc_free(env, line, line_length_of_prompt);
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
    struct command *command;
    struct command *new_command;


    state_arg = (struct state *) arg;

    command = state_arg->command;

    if (command != NULL) {
        if (command->line != NULL){
            dc_free(env, command->line, strlen(command->line));
        }
        if (command->command != NULL){
            dc_free(env, command->command, strlen(command->command));
        }
        if (command->argv != NULL) {
            for (int i = 0; command->argv[i]; ++i) {
                dc_free(env, command->argv[i], strlen(command->argv[i]));
            }
            dc_free(env, command->argv, sizeof(char **));
        }

        if (command->stdin_file != NULL){
            dc_free(env, command->stdin_file, strlen(command->stdin_file));
        }
        if (command->stdout_file != NULL){
            dc_free(env, command->stdout_file, strlen(command->stdout_file));
        }

        if (command->stderr_file != NULL){
            dc_free(env, command->stderr_file, strlen(command->stderr_file));
        }
        dc_free(env, command, sizeof(struct command));

    }
    command = NULL;


    new_command = dc_malloc(env, err, sizeof(struct command));

    if (dc_error_has_error(err))
    {
        state_arg->fatal_error = true;
        return ERROR;
    }

    state_arg->command = new_command;

    new_command->line = dc_strdup(env, err, state_arg->current_line);
//    new_command->line = state_arg->current_line;

//    new_command->command = NULL;

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
        execute(env, err, state_arg->command, state_arg->path);

        if (dc_error_has_error(err))
        {
            state_arg->fatal_error = true;
        }
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


    if (state_arg->fatal_error) {
        return DESTROY_STATE;
    }


    return RESET_STATE;
}


