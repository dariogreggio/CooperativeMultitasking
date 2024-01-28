// threads
#define THREAD_ID_INVALID	(-1)
#define MAX_THREADS			(20)
#define TIMESLICE_MS		(1)
#define ENV_POSITION_RETURNADDRESS (11*2)      // sperimentale (non ci sono i sorgenti e il debugger mostra int64!! @#£$% #cancrokubler
#define ENV_POSITION_RETURNCODE (23)      // a caso, v. .. oppure si potrebbero usare 8-9 dove c'è lo stack, vicinanze
typedef enum __attribute__ ((__packed__)) {
	THREAD_NEW = 0, 
    THREAD_READY,
    THREAD_RUNNING,
    THREAD_SLEEPING,
    THREAD_BLOCKED,
    THREAD_DONE
    } THREAD_STATE;
typedef enum __attribute__ ((__packed__)) {
	THREAD_PTY_IDLE = 0, 
	THREAD_PTY_VERYLOW,
    THREAD_PTY_LOW, 
    THREAD_PTY_MIDDLE,
    THREAD_PTY_HIGH, 
    THREAD_PTY_VERYHIGH, 
    THREAD_PTY_REALTIME
    } THREAD_PRIORITY;
typedef struct __attribute((packed)) _THREAD {
	void *context;
    struct _THREAD *next;
	jmp_buf env;
    THREAD_PRIORITY priority;
    THREAD_STATE state;
  	uint16_t sleepCount;		// sleeptime in ms
//    unsigned char ID; mettere... ovvero usiamo ordine prog. getThreadID
    // BYTE PendingIO;
    } THREAD;
STATIC_ASSERT((sizeof(struct _THREAD) ==12+sizeof(jmp_buf)),0);
#define	ATOMIC_START()
#define	ATOMIC_END()
THREAD *BeginThread(void *function,BYTE state,DWORD parm);      // SIZE_T  dwStackSize
BOOL EndThread(THREAD *);
BOOL ExitThread(DWORD);
BOOL KillThread(THREAD *,DWORD);
void SuspendThread(THREAD *);
void ResumeThread(THREAD *);
BOOL GetExitCodeThread(THREAD *,DWORD *lpExitCode);
void ThreadSleep(uint16_t);
BOOL SwitchToThread(THREAD *);
BYTE GetThreadID(THREAD *);
THREAD_PRIORITY GetThreadPriority(THREAD *);
BOOL SetThreadPriority(THREAD *,THREAD_PRIORITY);
BOOL GetThreadPriorityBoost(THREAD *,BOOL *pDisablePriorityBoost);
BOOL SetThreadPriorityBoost(THREAD *,BOOL pDisablePriorityBoost);
int GetThreadDescription(THREAD *,char *ppszThreadDescription);
int SetThreadDescription(THREAD *,char *lpThreadDescription);
//BOOL GetThreadInformation(THREAD *,THREAD_INFORMATION_CLASS ThreadInformationClass,
//  void *ThreadInformation,DWORD ThreadInformationSize);
//BOOL SetThreadInformation(THREAD *,THREAD_INFORMATION_CLASS ThreadInformationClass,
//       void *ThreadInformation,DWORD ThreadInformationSize);
BOOL GetThreadTimes(THREAD *,FILETIME *lpCreationTime,FILETIME *lpExitTime,
  FILETIME *lpKernelTime,FILETIME *lpUserTime);
BOOL GetThreadIOPendingFlag(THREAD *,BOOL *lpIOIsPending);
BYTE GetCurrentThreadID(void);
#define GetCurrentThread() getActiveThread()        // per forza...
THREAD *GetThreadFromID(BYTE );
THREAD *GetThreadFromContext(void *);
THREAD *getNextThread(void);
THREAD *getActiveThread(void);
THREAD *setActiveThread(THREAD *);
//extern THREAD *rootThreads,*winManagerThreadID;
//https://stackoverflow.com/questions/14685406/practical-usage-of-setjmp-and-longjmp-in-c/14685524
//https://en.wikipedia.org/wiki/Setjmp.h
// DEVO spacioccare con l'active Thread perché non c'è modo di ricavare il thread da dentro la funzione...
//  IDEM deve essere una Macro per salvare/ricevere la posizione attuale
//void Yield(void *);
#ifdef USA_BREAKTHROUGH
#define Yield() \
      {\
      ATOMIC_START();\
      THREAD *nextThread=getNextThread();\
      THREAD *myThread=getActiveThread();\
      switch(myThread->state) {\
        case THREAD_NEW:\
          if(setjmp(myThread->env) == 0) {\
            myThread->state = THREAD_READY;\
            return;\
            }\
          else\
            myThread->state = THREAD_RUNNING;\
          break;\
        case THREAD_READY:\
          Nop();\
          break;\
        case THREAD_RUNNING:\
          if(setjmp(myThread->env) == 0) {\
            if(myThread->state==THREAD_RUNNING) myThread->state = THREAD_READY;\
            setActiveThread(nextThread);\
            longjmp(nextThread->env, GetThreadID(nextThread));\
            return;\
            }\
          else\
            setActiveThread(myThread);\
          break;\
        default:\
          Nop();\
          break;\
        }\
      ATOMIC_END();\
      }
#else
#define Yield()
#endif
//https://www.chiark.greenend.org.uk/~sgtatham/coroutines.html
