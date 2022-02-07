#include <dc_posix/dc_stdlib.h>
#include <dc_posix/dc_string.h>
#include "util.h"
#include "command.h"

static size_t count(const char *str, int c);



/**
 * Get the prompt to use.
 *
 * @param env the posix environment.
 * @param err the error object
 * @return value of the PS1 environ var or "$ " if PS1 not set.
 */
char *get_prompt(const struct dc_posix_env *env, struct dc_error *err)
{
    char * env_var = dc_getenv(env, "PS1");

    if (env_var == NULL) {
        char *env_var_null_case = dc_strdup(env, err, "$ ");
        return env_var_null_case;
    }

    char * env_var_dup = dc_strdup(env, err, env_var);

    return env_var_dup;
}

/**
 * Get the PATH environ var.
 *
 * @param env the posix environment.
 * @param err the error object
 * @return the PATH environ var
 */
char *get_path(const struct dc_posix_env *env, struct dc_error *err)
{
    char * env_var = dc_getenv(env, "PATH");

    if (env_var == NULL) {
        return NULL;
    }

    char * env_var_dup = dc_strdup(env, err, env_var);
    return env_var_dup;
}

/**
 * Separate a path (eg. PATH environ var) into separate directories.
 * Directories are separated with a ':' character.
 * Any directories with ~ are converted to the users home directory.
 *
 * @param env the posix environment.
 * @param err the error object.
 * @param path_str the string to separate.
 * @return The directories that make up the path.
 */
char **parse_path(const struct dc_posix_env *env, struct dc_error *err,
                  const char *path_str) {
    char *str = dc_strdup(env, err, path_str);
    char *state;
    char *token;
    size_t num;
    char **list;
    size_t i;


    state = str;
    num = count(str, ':') + 1;
    list = malloc((num + 1) * sizeof(char *));

    i = 0;

    while((token = dc_strtok_r(env,state, ":", &state)) != NULL)
    {

        list[i] = dc_strdup(env, err, token);
        i++;
    }

    list[i] = NULL;
    dc_free(env, str, strlen(str));

    //added free(state);
    return list;
}

/**
 * Reset the state for the next read, freeing any dynamically allocated memory.
 *
 * @param env the posix environment.
 * @param err the error object
 */
void do_reset_state(const struct dc_posix_env *env, struct dc_error *err, struct state *state) {
    int pos;
    struct command *command;


    state->stdin = stdin;
    state->stdout = stdout;
    state->stderr = stderr;
    command = state->command;

    if (state->in_redirect_regex != NULL) {
        dc_free(env, state->in_redirect_regex, sizeof(regex_t));
        state->in_redirect_regex = NULL;
    }

    if (state->out_redirect_regex != NULL) {
        dc_free(env, state->out_redirect_regex, sizeof(regex_t));
        state->out_redirect_regex = NULL;
    }

    if (state->err_redirect_regex != NULL) {
        dc_free(env, state->err_redirect_regex, sizeof(regex_t));
        state->err_redirect_regex = NULL;
    }

    if (state->prompt != NULL) {
        dc_free(env, state->prompt, strlen(state->prompt));
        state->prompt = NULL;
    }

    if (state->current_line != NULL) {
        dc_free(env, state->current_line, strlen(state->current_line));
        state->current_line = NULL;
    }
//    if (command != NULL) {
//
//        dc_free(env, command, sizeof(&state->command));
//        state->command = NULL;
//    }

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
    state->command = NULL;

    state->max_line_length = 0;
    state->current_line_length = 0;
    state->fatal_error = false;

    if (state->path != NULL) {
        pos = 0;
        while (state->path[pos] != NULL) {
            dc_free(env, state->path[pos], strlen(state->path[pos]));
            ++pos;
        }

        dc_free(env, state->path, sizeof(char **));
        state->path = NULL;
    }


    if (err->message != NULL) {
        dc_free(env, err->message, strlen(err->message));
        err->message = NULL;
    }
    if (err->file_name != NULL) {
//        dc_free(env, *err->file_name, strlen(err->file_name));
        err->file_name = NULL;
    }
    if (err->function_name != NULL) {
//        dc_free(env, &err->function_name, strlen(err->function_name));
        err->function_name = NULL;
    }
    err->line_number = 0;
    err->type = 0;
    err->err_code = 0;



//    err->err_code = 0;
//    err->type = 0;
//    err->line_number = 0;
//    err->file_name = NULL;
//    err->function_name = NULL;




}

/**
 * Display the state values to the given stream.
 *
 * @param env the posix environment.
 * @param state the state to display.
 * @param stream the stream to display the state on,
 */
void display_state(const struct dc_posix_env *env, const struct state *state, FILE *stream){
    char *str;

    str = state_to_string(env, NULL, state);
    fprintf(stream, "%s\n", str);
    free(str);
}

/**
 * Display the state values to the given stream.
 *
 * @param env the posix environment.
 * @param state the state to display.
 * @param stream the stream to display the state on,
 */
char *state_to_string(const struct dc_posix_env *env,  struct dc_error *err, const struct state *state){
    size_t len;
    char *line;

    if(state->current_line == NULL) {
        len = dc_strlen(env, "current_line = NULL");
    }
    else
    {
        len = dc_strlen(env, "current_line = \"\"");
        len += state->current_line_length;
    }

    len += dc_strlen(env, ", fatal_error = ");
    len += sizeof(bool);

//    line = malloc(len + 1 + 1);
    if (state->current_line == NULL) {
        line = malloc(len+ 1);
        sprintf(line, "current_line = NULL, fatal_error = %d", state->fatal_error);
    } else {
        len += dc_strlen(env, state->current_line);
        line = malloc(len + 1);
        sprintf(line, "current_line = \"%s\", fatal_error = %d", state->current_line, state->fatal_error);
    }

    line[len] = '\0';

    return line;
}


static size_t count(const char *str, int c){
    size_t num;

    num = 0;
    for (const char *tmp = str; *tmp; tmp++)
    {
        if(*tmp == c){
            num++;
        }
    }

    return num;

}
