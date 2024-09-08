static THREAD *rootThreads=NULL,*winManagerThreadID;

// ----------------------------------------------------------------------------
//https://github.com/Kraego/STM32-MiniOS/blob/main/Usercode/Concurrency/scheduler.c
// pic32-libs/libpic32/arch/mips/setjmp.S (col cazzo che c'è...)
static THREAD *gRunningThread;
static BYTE addThread(THREAD *thread) {
  THREAD *myThread;
  BYTE n=0;
  
  if(!rootThreads)
    rootThreads=thread;
  else {
    myThread=rootThreads;
    n++;
    while(myThread && myThread->next) {
      myThread=myThread->next;
      n++;
      }
    myThread->next=thread;
    }
// USARE MAX_THREADS!
  return n;
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
THREAD *BeginThread(void *function,THREAD_STATE state,DWORD parm1,DWORD parm2,DWORD parm3,DWORD parm4) {
  THREAD *thread,*oldthread;
  
	ATOMIC_START();
  thread=malloc(sizeof(THREAD));
  thread->next=NULL;
  thread->context=function;
  thread->priority=THREAD_PTY_MIDDLE;
  thread->state=THREAD_NEW;
  thread->sleepCount=0;
  thread->signals=0;
  thread->pendingIO=0;
  thread->ID=addThread(thread);
  if(thread->ID>=MAX_THREADS) {
    removeThread(thread);
    return NULL;
    }
	ATOMIC_END();
  oldthread=gRunningThread;   // patcho per consentire il primo yield alla funzione
  gRunningThread=thread;
  
  thread->state=state;
  ((void (*)(DWORD,DWORD,DWORD,DWORD))(thread->context))(parm1,parm2,parm3,parm4);    // per primo preset, di là farà il primo setjmp
  gRunningThread=oldthread;
 
// NON SI PUò FARE LONGJMP A UNA FUNZIONE TERMINATA... per cui va usato ExitThread() e il return non viene mai eseguito
//  https://web.eecs.utk.edu/~mbeck/classes/cs560/360/notes/Setjmp/lecture.html
  // v. sotto: potrebbero esserci dei leak di memoria...
  state=thread->state;
  switch(state) {   // FORZARE automaticamente se è il primo/root? avrebbe senso
    case THREAD_RUNNING:
      setActiveThread(thread);
//  ((void (*)(DWORD))(thread->context))(parm);    // per primo preset
      longjmp(thread->env,1);
      break;
    case THREAD_SLEEPING:
      break;
    case THREAD_BLOCKED:
      break;
    default:
 //     gRunningThread=NULL;
      break;
    }
  return thread;
  }
BOOL EndThread(THREAD *thread) {
  
	ATOMIC_START();
  removeThread(thread);
  free(thread);
	ATOMIC_END();
  return TRUE;
  }

void ExitThread(DWORD n) {
  THREAD *thread=GetCurrentThread();
  
	ATOMIC_START();
	//DEBUG_PRINTF("Exiting Thread with id: %d", thread);
    
//  GetCurrentThread()->env=rootThreads->env;
//  gRunningThread->env[11*2]=rootThreads->env[11*2];
//  gRunningThread->env[22*2]=rootThreads->env[22*2];
  
  thread->env[ENV_POSITION_RETURNCODE]=n;   // NATURALMENTE diventa difficile accedere al thread dopo che è stato deallocato :D ! vedere...
  setActiveThread(getNextThread());
	thread->state = THREAD_DONE;
  EndThread(thread);
  longjmp(gRunningThread->env, -1);
  // ora va , ma potremmo avere dei leak di memoria..
	ATOMIC_END();
  }

BOOL TerminateThread(THREAD *thread,DWORD ret) {
  
	ATOMIC_START();
	//DEBUG_PRINTF("Finishing Thread with id: %d", gRunningThread);
	thread->state = THREAD_DONE;
  thread->env[ENV_POSITION_RETURNCODE]=ret;
	ATOMIC_END();
  Yield();
  }

void SuspendThread(THREAD *thread) {    // leggermente diversa da Windows...
  
	thread->state = THREAD_BLOCKED;
// mah...  Yield(); direi che serve solo se agisco sul thread in corso
  }

void ResumeThread(THREAD *thread) {    // leggermente diversa da Windows...
  
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
      return n;
    myThread=myThread->next;
    }
  return THREAD_ID_INVALID;
  }
BYTE GetCurrentThreadID() {
  return gRunningThread ? GetThreadID(gRunningThread) : THREAD_ID_INVALID;
  }
THREAD *GetThreadFromID(BYTE n) {
  THREAD *myThread=rootThreads;
  
  if(n==THREAD_ID_INVALID)
    return NULL;
  while(n-- && myThread) {
    myThread=myThread->next;
    }
  return myThread;
  }
BOOL GetThreadIOPendingFlag(THREAD *t,BOOL *lpIOIsPending) {
  *lpIOIsPending=t->pendingIO;
  return FALSE;
  }
  
void updateThreads(void) {
  THREAD *myThread=rootThreads;
  
  __delay_ms(TIMESLICE_MS); 
  ClrWdt();
  handleWinTimers();
  
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
// serve qua? come fare?			  longjmp(gRunningThread->env, -1);
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
#if 0 // questa versione "alternava", invece preferisco che TUTTI i thread di un livello vengano eseguiti più volte come dice la priorità
THREAD *getNextThreadPty(THREAD *oldThread,THREAD_PRIORITY myPty) {
  THREAD *myThread;
  BOOL rootPassed=FALSE;

  myThread=oldThread;
  do {
    if(!myThread) {   // se ho fatto il giro... o sono entrato come "primo giro" o non ce n'era cmq nessuno a questo livello
      myThread=rootThreads;
      rootPassed=TRUE;
      }
    else
      myThread=myThread->next;
    if(!myThread) {
      if(rootPassed) 
        return NULL;
      myThread=rootThreads;
      rootPassed=TRUE;
      }
//    if(myThread==oldThread && oldThread!=rootThreads /*caso particolare*/)   // [se torno a quello di partenza, restituisco NON TROVATO NESSuN ALTRO THREAD con questa pty
//      return NULL;
    if(myThread->priority==myPty && (myThread->state==THREAD_READY || myThread->state==THREAD_RUNNING))
      break;
    } while(myThread);

  return myThread;
  }
THREAD *getNextThread(void) {
  THREAD *myThread;
  static THREAD *oldThreadLow,*oldThreadMid,*oldThreadHigh;
  static BYTE ptyCnt=0;

rifo:  
  if(!(ptyCnt & 1)) {
    ptyCnt++;
    ptyCnt %= 7;
    myThread=getNextThreadPty(oldThreadHigh,THREAD_PTY_HIGH);
    if(myThread)
      oldThreadHigh=myThread;
    else
      goto rifo;
    }
  else if(!(ptyCnt & 2)) {
    ptyCnt++;
    ptyCnt %= 7;
    myThread=getNextThreadPty(oldThreadMid,THREAD_PTY_MIDDLE);
    if(myThread)
      oldThreadMid=myThread;
    else
      goto rifo;
    }
  else if(!(ptyCnt & 4)) {
    ptyCnt++;
    ptyCnt %= 7;
    myThread=getNextThreadPty(oldThreadLow,THREAD_PTY_LOW);
    if(myThread)
      oldThreadLow=myThread;
    else
      goto rifo;
    }
  
  return myThread;
  }
#endif
THREAD *getNextThreadPty(THREAD *oldThread,THREAD_PRIORITY myPty) {
  THREAD *myThread;

  myThread=oldThread;
  do {
    if(!myThread) {   // se ho fatto il giro... o sono entrato come "primo giro" o non ce n'era cmq nessuno a questo livello
      myThread=rootThreads;
      }
    else
      myThread=myThread->next;
    if(!myThread) {
      return NULL;
      }
//    if(myThread==oldThread && oldThread!=rootThreads /*caso particolare*/)   // [se torno a quello di partenza, restituisco NON TROVATO NESSuN ALTRO THREAD con questa pty
//      return NULL;
    if((myThread->priority==myPty) && (myThread->state==THREAD_READY || myThread->state==THREAD_RUNNING))
      break;
    } while(myThread);

  return myThread;
  }
THREAD *getNextThread(void) {
  THREAD *myThread;
  static THREAD *oldThread;
  static BYTE ptyCnt=0;

rifo:  
  if(!(ptyCnt & 1)) {
/*    ptyCnt++;
    ptyCnt %= 7;
    myThread=getNextThreadPty(oldThreadHigh,THREAD_PTY_HIGH);
    if(myThread)
      oldThreadHigh=myThread;
    else
      goto rifo;*/
    myThread=getNextThreadPty(oldThread,THREAD_PTY_HIGH);
    if(myThread) {
        if(!oldThread)
            oldThread=myThread;
        }
    else {
        oldThread=NULL;
        ptyCnt++;
        ptyCnt %= 7;
        goto rifo;
        }
    }
  else if(!(ptyCnt & 2)) {
/*    ptyCnt++;
    ptyCnt %= 7;
    myThread=getNextThreadPty(oldThreadMid,THREAD_PTY_MIDDLE);
    if(myThread)
      oldThreadMid=myThread;
    else
      goto rifo;*/
    myThread=getNextThreadPty(oldThread,THREAD_PTY_MIDDLE);
    if(myThread) {
        oldThread=myThread;
        }
    else {
        oldThread=NULL;
        ptyCnt++;
        ptyCnt %= 7;
        goto rifo;
        }
    }
  else if(!(ptyCnt & 4)) {
/*    ptyCnt++;
    ptyCnt %= 7;
    myThread=getNextThreadPty(oldThreadLow,THREAD_PTY_LOW);
    if(myThread)
      oldThreadLow=myThread;
    else
      goto rifo;*/
    myThread=getNextThreadPty(oldThread,THREAD_PTY_LOW);
    if(myThread) {
        oldThread=myThread;
        }
    else {
        oldThread=NULL;
        ptyCnt++;
        ptyCnt %= 7;
        goto rifo;
        }
    }
  
  return myThread;
  }

THREAD *getNextThreadSeq(void) {    // questa cerca semplicemente il thread READY successivo
  THREAD *myThread;

  myThread=rootThreads;
  while(myThread) {
    if(myThread->state == THREAD_RUNNING) {   // diciamo inutile, tutto questo...
      if(myThread == gRunningThread)
        break;
      }
    myThread=myThread->next;
    }

	if(myThread) {
    BYTE done1=FALSE;
    myThread=myThread->next;
    
    do {
      if(myThread == gRunningThread)    // se ho fatto il giro...
        done1=TRUE;
      if(myThread && myThread->state == THREAD_READY) {
        break;
        }
      if(myThread)
        myThread=myThread->next;
      else
        myThread=rootThreads;
      } while(!done1);
    }
  else {
    myThread=rootThreads /*NULL*/;    // NON deve accadere, diciamo! il manager deve sempre essere attivo (magari gestire in Kill/Suspend
    }
  
  return myThread;
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

BOOL SetEvent(THREAD *t,BYTE hEvent) {
  t->signals |= (1 << hEvent);
  return TRUE;
  }
BOOL ResetEvent(THREAD *t,BYTE hEvent) {
  t->signals &= ~(1 << hEvent);
  return TRUE;
  }
DWORD WaitForSingleObject(THREAD *t,BYTE hEvent,DWORD dwMilliseconds) {
  while(dwMilliseconds--) {   // INFINITE Ä ok :)
    if(t->signals & (1 << hEvent))
      return WAIT_OBJECT_0;
    Yield();
    }
  return WAIT_TIMEOUT;
  }
DWORD WaitForSingleObjectEx(THREAD *t,BYTE hEvent,DWORD dwMilliseconds,BOOL bAutoReset) {
  while(dwMilliseconds--) {   // INFINITE Ä ok :)
    if(t->signals & (1 << hEvent)) {
      if(bAutoReset)
        t->signals &= ~(1 << hEvent);
      return WAIT_OBJECT_0;
      }
    Yield();
    }
  return WAIT_TIMEOUT;
  }
DWORD WaitForMultipleObjects(THREAD *t,BYTE eventMask,BOOL bWaitAll,DWORD dwMilliseconds) {
  while(dwMilliseconds--) {   // INFINITE Ä ok :)
    if(bWaitAll) {
      if((t->signals & eventMask) == eventMask)
        return WAIT_OBJECT_0;
        break;
      }
    else {
      if(t->signals & eventMask)
        return WAIT_OBJECT_0 | eventMask;     // boh ok
      }
    Yield();
    }
  return WAIT_TIMEOUT;
  }
