#define _POSIX_C_SOURCE 200809L
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <errno.h>

#define   TIMEOUTS       16
#define   TIMEOUT_SIGNAL (SIGRTMIN+0)

#define   TIMEOUT_USED   1
#define   TIMEOUT_ARMED  2
#define   TIMEOUT_PASSED 4

static timer_t timeout_timer;
static volatile sig_atomic_t timeout_state[TIMEOUTS] = { 0 };

static struct timespec timeout_time[TIMEOUTS];

/* Return the number of seconds between before and after, (after - before).
 * This must be async-signal safe, so it cannot use difftime().
*/
static inline double timespec_diff(const struct timespec after, const struct timespec before)
{
    return (double)(after.tv_sec - before.tv_sec)
        + (double)(after.tv_nsec - before.tv_nsec) / 1000000000.0;
}

/* Add positive seconds to a timespec, nothing if seconds is negative.
 * This must be async-signal safe.
*/
static inline void timespec_add(struct timespec *const to, const double seconds)
{
    if (to && seconds > 0.0) {
        long s = (long)seconds;
        long ns = (long)(0.5 + 1000000000.0 * (seconds - (double)s));

        /* Adjust for rounding errors. */
        if (ns < 0L)
            ns = 0L;
        else if (ns > 999999999L)
            ns = 999999999L;

        to->tv_sec += (time_t) s;
        to->tv_nsec += ns;

        if (to->tv_nsec >= 1000000000L) {
            to->tv_nsec -= 1000000000L;
            to->tv_sec++;
        }
    }
}

/* Set the timespec to the specified number of seconds, or zero if negative seconds.
*/
static inline void timespec_set(struct timespec *const to, const double seconds)
{
    if (to) {
        if (seconds > 0.0) {
            const long s = (long)seconds;
            long ns = (long)(0.5 + 1000000000.0 * (seconds - (double)s));

            if (ns < 0L)
                ns = 0L;
            else if (ns > 999999999L)
                ns = 999999999L;

            to->tv_sec = (time_t) s;
            to->tv_nsec = ns;

        } else {
            to->tv_sec = (time_t) 0;
            to->tv_nsec = 0L;
        }
    }
}

/* Return nonzero if the timeout has occurred.
*/
static inline int timeout_passed(const int timeout)
{
    if (timeout >= 0 && timeout < TIMEOUTS) {
        const int state = __sync_or_and_fetch(&timeout_state[timeout], 0);

        /* Refers to an unused timeout? */
        if (!(state & TIMEOUT_USED))
            return -1;

        /* Not armed? */
        if (!(state & TIMEOUT_ARMED))
            return -1;

        /* Return 1 if timeout passed, 0 otherwise. */
        return (state & TIMEOUT_PASSED) ? 1 : 0;

    } else {
        /* Invalid timeout number. */
        return -1;
    }
}

/* Release the timeout.
 * Returns 0 if the timeout had not fired yet, 1 if it had.
*/
static inline int timeout_unset(const int timeout)
{
    if (timeout >= 0 && timeout < TIMEOUTS) {
        /* Obtain the current timeout state to 'state',
         * then clear all but the TIMEOUT_PASSED flag
         * for the specified timeout.
         * Thanks to Bylos for catching this bug. */
        const int state = __sync_fetch_and_and(&timeout_state[timeout], TIMEOUT_PASSED);

        /* Invalid timeout? */
        if (!(state & TIMEOUT_USED))
            return -1;

        /* Not armed? */
        if (!(state & TIMEOUT_ARMED))
            return -1;

        /* Return 1 if passed, 0 otherwise. */
        return (state & TIMEOUT_PASSED) ? 1 : 0;

    } else {
        /* Invalid timeout number. */
        return -1;
    }
}

int timeout_set(const double seconds)
{
    struct timespec now, then;
    struct itimerspec when;
    double next;
    int timeout, i;

    /* Timeout must be in the future. */
    if (seconds <= 0.0)
        return -1;

    /* Get current time, */
    if (clock_gettime(CLOCK_REALTIME, &now))
        return -1;

    /* and calculate when the timeout should fire. */
    then = now;
    timespec_add(&then, seconds);

    /* Find an unused timeout. */
    for (timeout = 0; timeout < TIMEOUTS; timeout++)
        if (!(__sync_fetch_and_or(&timeout_state[timeout], TIMEOUT_USED) & TIMEOUT_USED))
            break;

    /* No unused timeouts? */
    if (timeout >= TIMEOUTS)
        return -1;

    /* Clear all but TIMEOUT_USED from the state, */
    __sync_and_and_fetch(&timeout_state[timeout], TIMEOUT_USED);

    /* update the timeout details, */
    timeout_time[timeout] = then;

    /* and mark the timeout armable. */
    __sync_or_and_fetch(&timeout_state[timeout], TIMEOUT_ARMED);

    /* How long till the next timeout? */
    next = seconds;
    for (i = 0; i < TIMEOUTS; i++)
        if ((__sync_fetch_and_or(&timeout_state[i], 0) &
             (TIMEOUT_USED | TIMEOUT_ARMED | TIMEOUT_PASSED)) == (TIMEOUT_USED | TIMEOUT_ARMED)) {
            const double secs = timespec_diff(timeout_time[i], now);
            if (secs >= 0.0 && secs < next)
                next = secs;
        }

    /* Calculate duration when to fire the timeout next, */
    timespec_set(&when.it_value, next);
    when.it_interval.tv_sec = 0;
    when.it_interval.tv_nsec = 0L;

    /* and arm the timer. */
    if (timer_settime(timeout_timer, 0, &when, NULL)) {
        /* Failed. */
        __sync_and_and_fetch(&timeout_state[timeout], 0);
        return -1;
    }

    /* Return the timeout number. */
    return timeout;
}

static void timeout_signal_handler(int signum
                                   __attribute__ ((unused)), siginfo_t * info, void *context
                                   __attribute__ ((unused)))
{
    struct timespec now;
    struct itimerspec when;
    int saved_errno, i;
    double next;

    /* Not a timer signal? */
    if (!info || info->si_code != SI_TIMER)
        return;

    /* Save errno; some of the functions used may modify errno. */
    saved_errno = errno;

    if (clock_gettime(CLOCK_REALTIME, &now)) {
        errno = saved_errno;
        return;
    }

    /* Assume no next timeout. */
    next = -1.0;

    /* Check all timeouts that are used and armed, but not passed yet. */
    for (i = 0; i < TIMEOUTS; i++)
        if ((__sync_or_and_fetch(&timeout_state[i], 0) &
             (TIMEOUT_USED | TIMEOUT_ARMED | TIMEOUT_PASSED)) == (TIMEOUT_USED | TIMEOUT_ARMED)) {
            const double seconds = timespec_diff(timeout_time[i], now);
            if (seconds <= 0.0) {
                /* timeout [i] fires! */
                __sync_or_and_fetch(&timeout_state[i], TIMEOUT_PASSED);

            } else if (next <= 0.0 || seconds < next) {
                /* This is the soonest timeout in the future. */
                next = seconds;
            }
        }

    /* Note: timespec_set() will set the time to zero if next <= 0.0,
     *       which in turn will disarm the timer.
     * The timer is one-shot; it_interval == 0.
     */
    timespec_set(&when.it_value, next);
    when.it_interval.tv_sec = 0;
    when.it_interval.tv_nsec = 0L;
    timer_settime(timeout_timer, 0, &when, NULL);

    /* Restore errno. */
    errno = saved_errno;
}

int timeout_init(void)
{
    struct sigaction act;
    struct sigevent evt;
    struct itimerspec arm;

    /* Install timeout_signal_handler. */
    sigemptyset(&act.sa_mask);
    act.sa_sigaction = timeout_signal_handler;
    act.sa_flags = SA_SIGINFO;
    if (sigaction(TIMEOUT_SIGNAL, &act, NULL))
        return errno;

    /* Create a timer that will signal to timeout_signal_handler. */
    evt.sigev_notify = SIGEV_SIGNAL;
    evt.sigev_signo = TIMEOUT_SIGNAL;
    evt.sigev_value.sival_ptr = NULL;
    if (timer_create(CLOCK_REALTIME, &evt, &timeout_timer))
        return errno;

    /* Disarm the timeout timer (for now). */
    arm.it_value.tv_sec = 0;
    arm.it_value.tv_nsec = 0L;
    arm.it_interval.tv_sec = 0;
    arm.it_interval.tv_nsec = 0L;
    if (timer_settime(timeout_timer, 0, &arm, NULL))
        return errno;

    return 0;
}

int timeout_done(void)
{
    struct sigaction act;
    struct itimerspec arm;
    int errors = 0;

    /* Ignore the timeout signals. */
    sigemptyset(&act.sa_mask);
    act.sa_handler = SIG_IGN;
    if (sigaction(TIMEOUT_SIGNAL, &act, NULL))
        if (!errors)
            errors = errno;

    /* Disarm any current timeouts. */
    arm.it_value.tv_sec = 0;
    arm.it_value.tv_nsec = 0L;
    arm.it_interval.tv_sec = 0;
    arm.it_interval.tv_nsec = 0;
    if (timer_settime(timeout_timer, 0, &arm, NULL))
        if (!errors)
            errors = errno;

    /* Destroy the timer itself. */
    if (timer_delete(timeout_timer))
        if (!errors)
            errors = errno;

    /* If any errors occurred, set errno. */
    if (errors)
        errno = errors;

    /* Return 0 if success, errno otherwise. */
    return errors;
}
