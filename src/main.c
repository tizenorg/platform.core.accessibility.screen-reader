#include <time.h>
#include <signal.h>
#include <err.h>
#include <execinfo.h>
#include <appcore-efl.h>
#include <Elementary.h>
#include <eldbus-1/Eldbus.h>
#include "navigator.h"
#include "window_tracker.h"
#include "logger.h"
#include "screen_reader.h"
#include "screen_reader_gestures.h"

#define MAX_STACK_FRAMES 64
static void *stack_traces[MAX_STACK_FRAMES];

void posix_print_stack_trace(FILE *log_file)
{
    int i, trace_size = 0;
    char **messages = (char **)NULL;

    trace_size = backtrace(stack_traces, MAX_STACK_FRAMES);
    messages = backtrace_symbols(stack_traces, trace_size);

    /* skip the first couple stack frames (as they are this function and
     our handler) and also skip the last frame as it's (always?) junk. */
    // for (i = 3; i < (trace_size - 1); ++i)
    for (i = 0; i < trace_size; ++i) // we'll use this for now so you can see what's going on
    {
        fprintf(log_file, "  BACKTRACE LINE %i: %s\n",i, messages[i]);
    }
    if (messages) { free(messages); }
}

void print_warning(int sig, siginfo_t *siginfo, FILE *log_file)
{
    switch(sig)
    {
    case SIGSEGV:
        fputs("Caught SIGSEGV: Segmentation Fault\n", log_file);
        break;
    case SIGINT:
        fputs("Caught SIGINT: Interactive attention signal, (usually ctrl+c)\n", log_file);
        break;
    case SIGFPE:
        switch(siginfo->si_code)
        {
        case FPE_INTDIV:
            fputs("Caught SIGFPE: (integer divide by zero)\n", log_file);
            break;
        case FPE_INTOVF:
            fputs("Caught SIGFPE: (integer overflow)\n", log_file);
            break;
        case FPE_FLTDIV:
            fputs("Caught SIGFPE: (floating-point divide by zero)\n", log_file);
            break;
        case FPE_FLTOVF:
            fputs("Caught SIGFPE: (floating-point overflow)\n", log_file);
            break;
        case FPE_FLTUND:
            fputs("Caught SIGFPE: (floating-point underflow)\n", log_file);
            break;
        case FPE_FLTRES:
            fputs("Caught SIGFPE: (floating-point inexact result)\n", log_file);
            break;
        case FPE_FLTINV:
            fputs("Caught SIGFPE: (floating-point invalid operation)\n", log_file);
            break;
        case FPE_FLTSUB:
            fputs("Caught SIGFPE: (subscript out of range)\n", log_file);
            break;
        default:
            fputs("Caught SIGFPE: Arithmetic Exception\n", log_file);
            break;
        }
    case SIGILL:
        switch(siginfo->si_code)
        {
        case ILL_ILLOPC:
            fputs("Caught SIGILL: (illegal opcode)\n", log_file);
            break;
        case ILL_ILLOPN:
            fputs("Caught SIGILL: (illegal operand)\n", log_file);
            break;
        case ILL_ILLADR:
            fputs("Caught SIGILL: (illegal addressing mode)\n", log_file);
            break;
        case ILL_ILLTRP:
            fputs("Caught SIGILL: (illegal trap)\n", log_file);
            break;
        case ILL_PRVOPC:
            fputs("Caught SIGILL: (privileged opcode)\n", log_file);
            break;
        case ILL_PRVREG:
            fputs("Caught SIGILL: (privileged register)\n", log_file);
            break;
        case ILL_COPROC:
            fputs("Caught SIGILL: (coprocessor error)\n", log_file);
            break;
        case ILL_BADSTK:
            fputs("Caught SIGILL: (internal stack error)\n", log_file);
            break;
        default:
            fputs("Caught SIGILL: Illegal Instruction\n", log_file);
            break;
        }
        break;
    case SIGTERM:
        fputs("Caught SIGTERM: a termination request was sent to the program\n", log_file);
        break;
    case SIGABRT:
        fputs("Caught SIGABRT: usually caused by an abort() or assert()\n", log_file);
        break;
    default:
        break;
    }
}


void posix_signal_handler(int sig, siginfo_t *siginfo, void *context)
{
    char file_name[256] = {0};
    struct tm * timeinfo;
    time_t rawtime = time(NULL);
    timeinfo = localtime(&rawtime);
    sprintf(file_name, "/tmp/screen_reader_crash_stacktrace_%i%i%i_%i:%i:%i_pid_%i.log", timeinfo->tm_year,
            timeinfo->tm_mon, timeinfo->tm_mday,timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, getpid());
    FILE * log_file = fopen(file_name, "w");
    (void)context;
    print_warning(sig, siginfo, stderr);
    print_warning(sig, siginfo, log_file);
    posix_print_stack_trace(log_file);
    fclose(log_file);
    log_file = NULL;
    _Exit(1);
}
static uint8_t alternate_stack[SIGSTKSZ];
void set_signal_handler()
{
    /* setup alternate stack */
    {
        stack_t ss = {};
        /* malloc is usually used here, I'm not 100% sure my static allocation
         is valid but it seems to work just fine. */
        ss.ss_sp = (void*)alternate_stack;
        ss.ss_size = SIGSTKSZ;
        ss.ss_flags = 0;

        if (sigaltstack(&ss, NULL) != 0) { err(1, "sigaltstack"); }
    }

    /* register our signal handlers */
    {
        struct sigaction sig_action = {};
        sig_action.sa_sigaction = posix_signal_handler;
        sigemptyset(&sig_action.sa_mask);

        sig_action.sa_flags = SA_SIGINFO | SA_ONSTACK;

        if (sigaction(SIGSEGV, &sig_action, NULL) != 0) { err(1, "sigaction"); }
        if (sigaction(SIGFPE,  &sig_action, NULL) != 0) { err(1, "sigaction"); }
        if (sigaction(SIGINT,  &sig_action, NULL) != 0) { err(1, "sigaction"); }
        if (sigaction(SIGILL,  &sig_action, NULL) != 0) { err(1, "sigaction"); }
        if (sigaction(SIGTERM, &sig_action, NULL) != 0) { err(1, "sigaction"); }
        if (sigaction(SIGABRT, &sig_action, NULL) != 0) { err(1, "sigaction"); }
    }
}

static int app_create(void *data)
{
   eldbus_init();
   elm_init(0, NULL);

   logger_init();
   screen_reader_gestures_init();
   screen_reader_create_service(data);
   navigator_init();

   return 0;
}

static int app_terminate(void *data)
{
   screen_reader_terminate_service(data);

   navigator_shutdown();
   screen_reader_gestures_shutdown();
   eldbus_shutdown();
   logger_shutdown();
   return 0;
}

int main(int argc, char **argv)
{
   set_signal_handler();
   unsetenv("ELM_ATSPI_MODE");

   struct appcore_ops ops =
   {
      .create = app_create,
      .terminate = app_terminate,
      .pause = NULL,
      .resume = NULL,
      .reset = NULL
   };
   ops.data = get_pointer_to_service_data_struct();

   return appcore_efl_main("Smart Navigation", &argc, &argv, &ops);
}
