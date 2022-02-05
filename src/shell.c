#include "shell.h"
#include "dc_fsm/fsm.h"
#include "shell_impl.h"

/**
 * Run the shell FSM.
 *
 * @param env the posix environment.
 * @param error the error object
 * @param in the keyboard (stdin) file
 * @param out the keyboard (stdout) file
 * @param err the keyboard (stderr) file
 *
 * @return the exit code from the shell.
 */
int run_shell(const struct dc_posix_env *env, struct dc_error *error, FILE *in, FILE *out, FILE *err) {
    static struct dc_fsm_transition transition[] = {
            {DC_FSM_USER_START, INIT_STATE, init_state},
            {INIT_STATE, READ_COMMANDS, read_commands},
            {INIT_STATE, ERROR, handle_error},
            {READ_COMMANDS, RESET_STATE, reset_state},
            {READ_COMMANDS, SEPARATE_COMMANDS, separate_commands},
            {READ_COMMANDS, ERROR, handle_error},
            {SEPARATE_COMMANDS, PARSE_COMMANDS, parse_commands},
            {SEPARATE_COMMANDS, ERROR, handle_error},
            {PARSE_COMMANDS, EXECUTE_COMMANDS, execute_commands},
            {PARSE_COMMANDS, ERROR, handle_error},


    };
}
