#include <pthread.h>                    /* pthread_t C structure            */

    friend void * input_thread_func (void * myperf);
    friend void * output_thread_func (void * myperf);

    extern void * output_thread_func (void * p);
    extern void * input_thread_func (void * p);

    pthread_t m_out_thread;
    pthread_t m_in_thread;

/*
 * -------------------------------------------------------------------------
 *  I/O threads
 * -------------------------------------------------------------------------
 */

#if ! defined SEQ66_USE_STD_THREADING

void *
output_thread_func (void * myperf)
{
    performer * p = reinterpret_cast<performer *>(myperf);

#if defined PLATFORM_WINDOWS
    timeBeginPeriod(1);                         /* WinMM.dll function   */
    p->output_func();
    timeEndPeriod(1);
#else
    if (rc().priority())                        /* Not in MinGW RCB     */
    {
        struct sched_param schp;
        std::memset(&schp, 0, sizeof(sched_param));
        schp.sched_priority = 1;
        if (sched_setscheduler(0, SCHED_FIFO, &schp) != 0)
        {
            errprint
            (
                "output_thread_func: couldn't sched_setscheduler(FIFO), "
                "need root priviledges."
            );
            pthread_exit(0);
        }
        else
        {
            infoprint("[Output priority set to 1]");
        }
    }
    p->output_func();
#endif

    return nullptr;
}

void *
input_thread_func (void * myperf)
{
    performer * p = reinterpret_cast<performer *>(myperf);

#if defined PLATFORM_WINDOWS
    timeBeginPeriod(1);
    p->input_func();
    timeEndPeriod(1);
#else                                   // MinGW RCB
    if (rc().priority())
    {
        struct sched_param schp;
        std::memset(&schp, 0, sizeof(sched_param));
        schp.sched_priority = 1;
        if (sched_setscheduler(0, SCHED_FIFO, &schp) != 0)
        {
            printf
            (
                "input_thread_func: couldn't sched_setscheduler"
               "(FIFO), need root priviledges."
            );
            pthread_exit(0);
        }
        else
        {
            infoprint("[Input priority set to 1]");
        }
    }
    p->input_func();
#endif

    return nullptr;
}

#endif  // defined SEQ66_USE_STD_THREADING



    if (m_out_thread_launched)
        pthread_join(m_out_thread, NULL);

    if (m_in_thread_launched)
        pthread_join(m_in_thread, NULL);



    int err = pthread_create(&m_out_thread, NULL, output_thread_func, this);
    if (err != 0)
    {
        errprint("failed to launch output thread");
    }
    else
        m_out_thread_launched = true;




    int err = pthread_create(&m_in_thread, NULL, input_thread_func, this);
    if (err != 0)
    {
        errprint("failed to launch input thread");
    }
    else
        m_in_thread_launched = true;



