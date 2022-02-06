#include "execute.h"
#include <dc_posix/dc_unistd.h>
#include <dc_posix/dc_stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <dc_posix/dc_string.h>
#include <sys/wait.h>
#include <dc_posix/dc_stdlib.h>

void redirect(const struct dc_posix_env *env, struct dc_error *err, struct command *command);
void run(const struct dc_posix_env *env, struct dc_error *err, struct command *command, char **path);
int handle_run_error(struct dc_error *err);
bool is_path_empty(char **path);

/**
 * Create a child process, exec the command with any redirection, set the exit code.
 * If there is an err executing the command print an err message.
 * If the command cannot be found set the command->exit_code to 127.
 *
 * @param env the posix environment.
 * @param err the err object
 * @param command the command to execute
 * @param path the directories to search for the command
 */
void execute(const struct dc_posix_env *env, struct dc_error *err, struct command *command, char **path)
{
    pid_t child;
    int status;

    child = dc_fork(env, err);
    if (child == 0) {
        redirect(env, err, command);

        if (dc_error_has_error(err)) {
            exit(126);
        }

        run(env, err, command, path);

        status = handle_run_error(err);
        exit(status);
    } else {
        waitpid(child, &status, WUNTRACED);
        command->exit_code = WEXITSTATUS(status);
    }

}

int handle_run_error(struct dc_error *err) {
    int to_return;
    switch(err->err_code) {
        case (E2BIG):
            to_return = 1;
            break;
        case (EACCES):
            to_return = 2;
            break;
        case (EINVAL):
            to_return = 3;
            break;
        case (ELOOP):
            to_return = 4;
            break;
        case (ENAMETOOLONG):
            to_return = 5;
            break;
        case (ENOENT):
            to_return = 127;
            break;
        case (ENOTDIR):
            to_return = 6;
            break;
        case(ENOEXEC):
            to_return = 7;
            break;
        case(ENOMEM):
            to_return = 8;
            break;
        case(ETXTBSY):
            to_return = 9;
            break;
        default:
            to_return = 125;
            break;
    }
    return to_return;
}



bool is_path_empty(char **path) {
    bool to_return;
    if (path == NULL || path[0] == NULL) {
        to_return = true;
    } else {
        to_return = false;
    }
    return to_return;


}


void run(const struct dc_posix_env *env, struct dc_error *err, struct command *command, char **path) {
    char *cmd;

    if (dc_strstr(env, command->command, "/") != NULL){
        /*
         * perhaps stdup?
         */
        command->argv[0] = command->command;
        dc_execv(env, err, command->command, &command->argv[0]);
    } else {
        if (is_path_empty(path)) {
            err->err_code = ENOENT;
        } else {
            for (size_t i = 0; path[i]; i++) {
                int execv_val;
                cmd = malloc(strlen(path[i]) + strlen(command->command) + 1);
                sprintf(cmd, "%s/%s", path[i], command->command);

                command->argv[0] = dc_strdup(env, err, cmd);
                execv_val = dc_execv(env, err, cmd, &command->argv[0]);

                if(execv_val != ENOENT) {
                    dc_free(env, cmd, strlen(cmd));
                    break;
                }
                dc_free(env, cmd, strlen(cmd));
            }

        }
    }
}

void redirect(const struct dc_posix_env *env, struct dc_error *err, struct command *command) {
    int fd;

    if (command->stdin_file != NULL) {

        fd = open(command->stdin_file, O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

        if (dc_error_has_error(err))
        {
            dc_close(env, err, fd);
            return;
        }

        dc_dup2(env, err, fd, 0);
    }

    if (command->stdout_file != NULL) {
        if (command->stdout_overwrite) {
            fd = open(command->stdout_file, O_WRONLY | O_CREAT|O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

        } else {
            fd = open(command->stdout_file, O_WRONLY | O_CREAT| O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        }

        if (dc_error_has_error(err))
        {
            dc_close(env, err, fd);
            return;
        }

        dc_dup2(env, err, fd, 1);
    }

    if (command->stderr_file != NULL) {
        if (command->stderr_overwrite) {
            fd = open(command->stderr_file, O_WRONLY | O_CREAT|O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

        } else {
            fd = open(command->stderr_file, O_WRONLY | O_CREAT|O_APPEND, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        }
        if (dc_error_has_error(err))
        {
            dc_close(env, err, fd);
            return;
        }

        dc_dup2(env, err, fd, 2);
    }

}
