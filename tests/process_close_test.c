#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdlib.h>
#include <sys/wait.h>
#include "process_close.h"
static int pipe_fd;
static void card_close(void) { assert(write(pipe_fd, "C", 1) == 1); }
static void destructor(void) { (void)write(pipe_fd, "D", 1); }
int main(void) {
    int fds[2]; assert(pipe(fds) == 0);
    pid_t child = fork(); assert(child >= 0);
    if (!child) {
        close(fds[0]); pipe_fd = fds[1];
        assert(atexit(destructor) == 0);
        bw_process_close(card_close, 0);
    }
    close(fds[1]); char bytes[2] = {0};
    assert(read(fds[0], bytes, 2) == 1 && bytes[0] == 'C');
    assert(read(fds[0], bytes, 2) == 0);
    close(fds[0]); int status;
    assert(waitpid(child, &status, 0) == child && WIFEXITED(status) && WEXITSTATUS(status) == 0);
}
