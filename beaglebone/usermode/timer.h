#ifndef __TIMER_H
#define __TIMER_H

#ifdef __cplusplus
extern "C" {
#endif

int timeout_set(const double seconds);
void timeout_signal_handler(int signum __attribute__((unused)), siginfo_t *info, void *context __attribute__((unused)));
int timeout_init(void);
int timeout_done(void);

#ifdef __cplusplus
}
#endif

#endif
