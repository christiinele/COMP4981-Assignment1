#include <unistd.h>
#include <stdlib.h>
#include <dc_posix/dc_stdlib.h>
#include <dc_posix/dc_regex.h>
#include "shell_impl.h"
#include "util.h"

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
    int pos;

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

    while (state_arg->path[pos] != NULL) {
        dc_free(env, state_arg->path[pos++], sizeof(char *));
    }

    dc_free(env, state_arg->path, sizeof(char **));
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
    return 0;
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
    return 0;
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
    return 0;
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
    return 0;
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
    return 0;
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
    return 0;
}
