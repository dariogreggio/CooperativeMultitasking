// ----------------------------------------------------------------------------
//https://github.com/Kraego/STM32-MiniOS/blob/main/Usercode/Concurrency/scheduler.c
// pic32-libs/libpic32/arch/mips/setjmp.S (col cazzo che c'è...)
static THREAD *gRunningThread;
static BOOL addThread(THREAD *thread) {
  THREAD *myThread;
  
  if(!rootThreads)
    rootThreads=thread;
  else {
    myThread=rootThreads;
    while(myThread && myThread->next)
      myThread=myThread->next;
    myThread->next=thread;
    }
// USARE MAX_THREADS!
  return TRUE;
  }
static BOOL removeThread(THREAD *thread) {
  THREAD *myThread,*myThread2;

  if(thread==rootThreads) {
    rootThreads=thread->next;
	  return TRUE;
		}

  myThread=rootThreads;
  while(myThread != thread) {
    myThread2=myThread;
    myThread=myThread->next;
		}
	if(myThread2) {
	  myThread2->next=myThread->next;
		myThread=myThread2->next;
	  return TRUE;
		}

  return FALSE;
  }
THREAD *BeginThread(void *function,BYTE state,DWORD parm) {
  THREAD *thread;
  
	ATOMIC_START();
  thread=malloc(sizeof(THREAD));
  thread->next=NULL;
  thread->context=function;
  thread->priority=THREAD_PTY_MIDDLE;
  thread->state=THREAD_NEW;
  thread->sleepCount=0;
  addThread(thread);
	ATOMIC_END();
  gRunningThread=thread;
  ((void (*)(DWORD))(thread->context))(parm);    // per primo preset
  switch(state) {   // FORZARE automaticamente se è il primo/root? avrebbe senso
    case THREAD_RUNNING:
      setActiveThread(thread);
      longjmp(thread->env,1);
      break;
    case THREAD_SLEEPING:
      break;
    case THREAD_BLOCKED:
      break;
    default:
      gRunningThread=NULL;
      break;
    }
  return thread;
  }
BOOL EndThread(THREAD *thread) {
  
	ATOMIC_START();
  removeThread(thread);
  free(thread);
	ATOMIC_END();
  }

BOOL ExitThread(DWORD n) {
  
	ATOMIC_START();
	//DEBUG_PRINTF("Finishing Thread with id: %d", gRunningThread);
	GetCurrentThread()->state = THREAD_DONE;
  GetCurrentThread()->env[ENV_POSITION_RETURNCODE]=n;
  EndThread(GetCurrentThread());
  Yield();
	ATOMIC_END();
// serve??      longjmp(GetCurrentThread()->env,1);
#warning si schianta sempre, a fine task...
  }

BOOL KillThread(THREAD *thread,DWORD ret) {
  
	ATOMIC_START();
	//DEBUG_PRINTF("Finishing Thread with id: %d", gRunningThread);
	thread->state = THREAD_DONE;
  thread->env[ENV_POSITION_RETURNCODE]=ret;
	ATOMIC_END();
  Yield();
  }

void SuspendThread(THREAD *thread) {
  
	thread->state = THREAD_BLOCKED;
  Yield();
  }

void ResumeThread(THREAD *thread) {
  
	thread->state = THREAD_READY;
  }

BOOL GetExitCodeThread(THREAD *t,DWORD *lpExitCode) {
  
  *lpExitCode=t->env[ENV_POSITION_RETURNCODE];      // pescato a caso, un posto dove mettere il valore! v. nelle routine
  }

THREAD *GetThreadFromContext(void *c) {
  THREAD *myThread=rootThreads;
  
  while(myThread) {
		if(myThread->context == c)
      return myThread;
    myThread=myThread->next;
    }
  return NULL;
  }

THREAD_PRIORITY GetThreadPriority(THREAD *t) {
  return t->priority;
  }
BOOL SetThreadPriority(THREAD *t,THREAD_PRIORITY p) {
  if(t) {
    t->priority=p;
    return TRUE;
    }
  else
    return FALSE;
  }
BYTE GetThreadID(THREAD *t) {
  THREAD *myThread=rootThreads;
  BYTE n=0;
  
  while(myThread) {
		if(myThread == t)
      return n+1;
    myThread=myThread->next;
    }
  return 0;
  }
BYTE GetCurrentThreadID() {
  return gRunningThread ? GetThreadID(gRunningThread) : 0;
  }
THREAD *GetThreadFromID(BYTE n) {
  THREAD *myThread=rootThreads;
  
  if(!n)
    return NULL;
  n--;
  while(n-- && myThread) {
    myThread=myThread->next;
    }
  return myThread;
  }
BOOL GetThreadIOPendingFlag(THREAD *t,BOOL *lpIOIsPending) {
  *lpIOIsPending=0;    //t->PendingIO fare...
  return FALSE;
  }
  
void updateThreads(void) {
  THREAD *myThread=rootThreads;
  
  __delay_ms(TIMESLICE_MS); 
  while(myThread) {
		switch(myThread->state) {
      case THREAD_SLEEPING:
        {uint16_t sleep = myThread->sleepCount;
        myThread->sleepCount = sleep >= TIMESLICE_MS ? (sleep-TIMESLICE_MS) : 0;
        if(myThread->sleepCount == 0)
          myThread->state = THREAD_READY;
        }
        break;
    	case THREAD_DONE:
        {THREAD *t=myThread->next;
        EndThread(myThread);    // mah mi sembra giusto
        myThread=t;
        continue;
        }
        break;
      }
    myThread=myThread->next;
    }
  }

THREAD *getActiveThread(void) {
  return gRunningThread;
  }
THREAD *setActiveThread(THREAD *thread) {
  gRunningThread=thread;
  gRunningThread->state = THREAD_RUNNING;
  }
THREAD *getNextThread(void) {
  THREAD *myThread;

  myThread=rootThreads;
  while(myThread) {
    if(myThread->state == THREAD_RUNNING) {   // diciamo inutile
      if(myThread == gRunningThread)
        break;
      }
    myThread=myThread->next;
    }

	if(myThread) {
    BYTE done1=FALSE;
    myThread=myThread->next;
    
    myThread->priority;   // GESTIRE!
    
    do {
      if(myThread == gRunningThread)    // se ho fatto il giro...
        done1=TRUE;
      if(myThread && myThread->state == THREAD_READY)
        return myThread;
      if(myThread)
        myThread=myThread->next;
      else
        myThread=rootThreads;
      } while(!done1);
    }
  else 
    return rootThreads /*NULL*/;    // NON deve accadere, diciamo! il manager deve sempre essere attivo (magari gestire in Kill/Suspend
  }

void ThreadSleep(uint16_t duration_ms) {  // OVVIAMENTE deve agire solo sul thread in corso!
  
  if(gRunningThread->state>THREAD_NEW && gRunningThread->state<THREAD_DONE) {
    ATOMIC_START();
    gRunningThread->sleepCount = duration_ms;
    gRunningThread->state = THREAD_SLEEPING;
  	ATOMIC_END();
    Yield();
    }
  }

BOOL SwitchToThread(THREAD *t) {
  THREAD *t2=gRunningThread;
// in un VERO multitasking, non si può scegliere a chi passare il controllo...  
// ..per cui sorvoliamo :)  gRunningThread->state=THREAD_READY;
//  getNextThread()->state=THREAD_RUNNING;
  Yield();
  return t2 != gRunningThread;
  }
