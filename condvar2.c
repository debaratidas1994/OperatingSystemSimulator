/* 
 * SIMULATOR FRAMEWORK implementing synchronization between scheduler + running processes making page request/response 
 * 
 * What we have used ?  pthread + signal + condition variable (mutex + condition variable) + pipe   
 * 
 * WORK REMAINING :-
 * 
 * (1) Instead of having these quanta (slot size) and count1_max etc defined in the main thread, we have to read these from a config file 
 * 
 * (2) Right now, a Random Number generator is used to  generate the roundRobinChoice. I have to plug in the 
 *     RoundRobin Scheduler function there 
 * 
 * (3) In this case, if Random Number generator generates 0, we set the Quit Flag and come out. Ultimately that functionality
 *     has to be commented as we want the whole thing to run till all other threads (simulating processes) are done with their share 
 *     of page requests
 * 
 * (4) When plugging in the Round Robin code (in the signal handler), we have to print out the Round Robin Report while coming out
 * 
 * (5)  We have just incremented the page request count and printed out ...  
 *     We have to plug in the MMU+LRU  code in the MMU thread and this needs to print out the final report  before coming out 
 *  
 *  EXPLANATION:- 
 * 
 * (6) How many threads we have now ? We have 6 i.e. main thread + scheduler thread + MMU thread + 3 threads simulating processes.  
 * 
 * (7) What IPC we have used so far ? Conditional synchronization (alomng with Boolean falgs)  between scheduler threads + threads 
 *     simulating processes ( 3 of them) 
 *     
 *     When the thread (simulating process) reaches quanta (slot in terms of number of page requests), it sends a signal to the 
 *     scheduler thread and schedulee thread run the  round Robin ( in this case random generator) and sets the boolean 
 *     variable signifying which thread (simulating proces) got the next slot ...
 * 
 * (8) What is the clock here ? a page request count is the clock... So, after every quanta ( slot size), a signal is used to tell the
 *     scheduler that the quanta got over and scheduler needs to run ( note that we have not made the scheduler observe every page request) 
 * 
 * (9) Why are we using conditional variable ? This is a case where we have used one mutex with 3 condition variables. We use this set up 
 *     to selectively wake up waiting processes to resume the page requests ! 
 * 
 * (10)Actual page request has to have a page number ( that has a logic involving random()  as per specification provided ) and this needs to be passed on to the MMU thread
 * 
 * (11)Bug or incomplete feature now : If a thread(simulating processes) exhausts all the page requests, then it kills itself. In the current code, the scheduler ( random generator) does NOT know 
 *     that. It can still allocate the quanta to the exited thread and in this case it will hang. This needs a logic of not generating the exited thread number (should not be the case with actual scheduler)...  
 * 
 * (12)How does the 3 threads simulating the processes (making page requests) communicate with MMU ?  They do this using pipe IPC . Note that 
 *     there are two pipes here i.e. one pipe for writing the page request to the MMU and MMU is reading from this. The other pipe is for 
 *     MMU to write the page response and for the 3 threads (simulating processes) read the page response from.
 */ 

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>

#define TRUE    1
#define FALSE   0

char roundRobinChoice;
int r;

// Boolean flags
short tflag_one_on, tflag_two_on, tflag_three_on, quit_flag = FALSE;

// Counters for threads 1,2,3 ( each count is a page request/response) 

int count_one, count_two, count_three = 0;

int quanta = 5; //hypothetical but later on needs to be read from file 

//total number of page requests to be made by each process (simulated by thread) 
//hypothetical but later on needs to be read from file 

int count1_max = 40;
int count2_max = 80;
int count3_max = 90;

// Threads 1, 2 and 3 declared as global
pthread_t func_one_thread, func_two_thread, func_three_thread;

//main scheduler thread ( to replace the keypress ) 
pthread_t scheduler_thread;

//main MMU thread 
pthread_t  mmu_thread;
 
pthread_attr_t attr;
   
// Thread conditions
pthread_cond_t thread_cond_one, thread_cond_two, thread_cond_three;

// Thread mutex to go with thread conditions
pthread_mutex_t mutex_flag;


//pipe related stuff 

int 	fd[2];//File descriptor for creating a pipe  for passing the page request tick
int 	fd1[2] ; //File descriptor for creating a pipe  for passing the the scheduling info 
int    page_request;
int    page_resp;
int    result;

//========================================================================

void *func_one() 
   { 
	   
  //waiting on condition 1 on mutex 
  pthread_mutex_lock(&mutex_flag);
  pthread_cond_wait(&thread_cond_one, &mutex_flag);
  pthread_mutex_unlock(&mutex_flag);
  
  // before going further I check the quit_flag
  if(quit_flag) 
       {
       printf("Thread 1 : quit flag set \n");
       pthread_exit(NULL);
	   }
  
  while(TRUE) { //otherwise keep doing 
  if(!tflag_one_on) { // If it is my turn 
      pthread_mutex_lock(&mutex_flag);
      pthread_cond_wait(&thread_cond_one, &mutex_flag);
      pthread_mutex_unlock(&mutex_flag);
			 } 
    if(quit_flag) 
        break; 
    
      //here the page request / response through pipe with the MMU thread should happen 
      result = write (fd[1], &page_request,1);
      printf ("running (thread) process 1: One page request made \n"); 
      if (result != 1){
            perror ("write");
            exit (2);
						}
						
	  result = read (fd1[0],&page_resp,1);	
	  printf ("running (thread) process 1 : One page response received \n"); 
	  
    fprintf(stderr, "<<Thread 1 Page Req/resp count :--%d-->>\n ", ++count_one);
    if (count_one==quanta)
		pthread_kill(scheduler_thread,SIGSEGV);
	if (count_one==count1_max)
	        {
			printf ("Simulation over for thread 1 \n");	
			pthread_kill(scheduler_thread,SIGSEGV);
			pthread_exit(NULL);
			}
    sleep(1);
  }
  printf("Thread 1 : quit flag set \n");
  
  pthread_exit(NULL);
}
//========================================================================
 
void *func_two() {
  pthread_mutex_lock(&mutex_flag);
  pthread_cond_wait(&thread_cond_two, &mutex_flag);
  pthread_mutex_unlock(&mutex_flag);
 
  if(quit_flag) 
		{
       printf("Thread 2 : quit flag set \n");
       pthread_exit(NULL);
	   }
  while(TRUE) {
    if(!tflag_two_on) {
      pthread_mutex_lock(&mutex_flag);
      pthread_cond_wait(&thread_cond_two, &mutex_flag);
      pthread_mutex_unlock(&mutex_flag);
    }
    if(quit_flag) 
          break;
        //here the page request / response through pipe with the MMU thread should happen 
      result = write (fd[1], &page_request,1);
      printf ("running (thread) process 2: One page request made \n"); 
      if (result != 1){
            perror ("write");
            exit (2);
						}
						
	  result = read (fd1[0],&page_resp,1);	
	  printf ("running (thread) process 2: One page response received \n"); 
    
    fprintf(stderr, "<<Thread 2 Page Req/resp count: %d >> \n", ++count_two);
    if (count_two==quanta)
		pthread_kill(scheduler_thread,SIGSEGV);
	if (count_two==count2_max)
	        {
			printf ("Simulation over for thread 2 \n");	
			pthread_kill(scheduler_thread,SIGSEGV);
			pthread_exit(NULL);
			}	
    sleep(1);
  }
  printf("Thread 2 : quit flag set \n");
  pthread_exit(NULL);
}
//========================================================================
 
void *func_three() {
	
  pthread_mutex_lock(&mutex_flag);
  pthread_cond_wait(&thread_cond_three, &mutex_flag);
  pthread_mutex_unlock(&mutex_flag);
 
  if(quit_flag) 
		{
       printf("Thread 3 : quit flag set \n");
       pthread_exit(NULL);
	   }
  while(TRUE) {
    if(!tflag_three_on) {
      pthread_mutex_lock(&mutex_flag);
      pthread_cond_wait(&thread_cond_three, &mutex_flag);
      pthread_mutex_unlock(&mutex_flag);
    }
    if(quit_flag) 
           break;
    //here the page request / response through pipe with the MMU thread should happen 
      result = write (fd[1], &page_request,1);
      printf ("running (thread) process 3: One page request made \n"); 
      if (result != 1){
            perror ("write");
            exit (2);
						}
						
	  result = read (fd1[0],&page_resp,1);	
	  printf ("running (thread) process 3: One page response received \n"); 	
    
    fprintf(stderr, "<<<<Thread 3 Page Req/resp count: %d  >> >> \n ", ++count_three);
    if (count_three==quanta)
		pthread_kill(scheduler_thread,SIGSEGV);
	if (count_three==count3_max)
	        {
			printf ("Simulation over for thread 3 \n");	
			pthread_kill(scheduler_thread,SIGSEGV);
			pthread_exit(NULL);
			}	
    sleep(1);
  }
   printf("Thread 3 : quit flag set \n");
   pthread_exit(NULL);
}

//------------------------------------------------------------------------------------

// Watch count or scheduler function 

void *watch_count(void *t) 
{
  
  printf("starting the simulation with 1 .. \n");
  printf("The simulation will exit when the random number generator (in place of scheduler) generates 0 .. \n");
  printf("quanta in the simulation is :%d.. \n",quanta);
  printf("Thread 1 simulating process 1 will make total page requests =%d.. \n",count1_max);
  printf("Thread 2 simulating process 1 will make total page requests =%d.. \n",count2_max);
  printf("Thread 3 simulating process 1 will make total page requests =%d.. \n",count3_max);
  
  printf("Scheduler thread is UP  .. \n");
   
  roundRobinChoice =1;
  
  do {
  
   //roundRobinChoice = getchar();
   switch (roundRobinChoice) {
      case 1:
        if(!tflag_one_on) {
		  printf("Scheduled 1...\n");	
          tflag_one_on = TRUE;
          tflag_two_on = FALSE;
          tflag_three_on = FALSE;
          pthread_mutex_lock(&mutex_flag);
          pthread_cond_signal(&thread_cond_one);
          pthread_mutex_unlock(&mutex_flag);
        } 
        break;
      case 2:
        if(!tflag_two_on) {
		  printf("Scheduled  2...\n");
          tflag_two_on = TRUE;
          tflag_one_on = FALSE;
          tflag_three_on = FALSE;
          pthread_mutex_lock(&mutex_flag);
          pthread_cond_signal(&thread_cond_two);
          pthread_mutex_unlock(&mutex_flag);
        } 
        break;
      case 3:
        if(!tflag_three_on) {
          printf("Scheduled  3...\n");
          tflag_two_on = FALSE;
          tflag_one_on = FALSE;
          tflag_three_on = TRUE;
          pthread_mutex_lock(&mutex_flag);
          pthread_cond_signal(&thread_cond_three);
          pthread_mutex_unlock(&mutex_flag);
					}  
		} //switch 
    }while(roundRobinChoice != 0);
   
  printf("Generated 0..for exit settin the Quit_flag for all threads simulating processes.\n");
  quit_flag = TRUE;
 
  pthread_cond_broadcast(&thread_cond_one);
  pthread_cond_broadcast(&thread_cond_two);
  pthread_cond_broadcast(&thread_cond_three);
 
  pthread_exit(NULL);
			 
}
//========================================================================

//signal handler 

void sig_func(int sig)
{
 printf("\n Quanta reached  + Caught signal for scheduling.request....\n");
 count_two=count_one=count_three=0;
 r = rand() % 4;  //generates random function between 0 to 3. Here the scheduler should come in
 printf ("Random number generated =%d Scheduled thread is %d \n",r,r);
 roundRobinChoice= r;
 signal(SIGSEGV,sig_func);
}

//========================================================================
void *MMU()
{
	   printf("MMU thread is UP  .. \n");
	   while(!quit_flag)
	     {  // it keeps reading the ticks from  writers till quanta elapses 
	   
	   
 	
       result = read (fd[0],&page_request,1);
       if (result != 1) {
            perror("read");
            exit(3);
						}  
		printf ("(thread) MMU : One page request received \n"); 
		//do the MMU stuff here 
		sleep(1);
		//now respond back with a result through the other pipe
		
		printf ("MMU : resolved page request  \n");  
		page_resp=1 ;//this is hypothetical . It should be replaced by some physical page number etc.
		result = write (fd1[1], &page_resp,1); //writing page_resp into this pipe means 
		printf ("(thread) MMU : One page response sent \n"); 
		 	
 	        
				} //while 
		
				
		printf("MMU Thread : quit flag set \n");
	    
	    //Before coming out, please print the LRU + MMU report here ( hit ratio, miss ratio etc)
		
		pthread_exit(NULL);		
				
}




//=======================================================================
// main function 

int main(int argc, char *argv[]) // main thread

{
  // Thread mutex initialization
  
  pthread_mutex_init(&mutex_flag, NULL); // One mutex with (n) number of flags depending on n number of threads
 
  // Thread condtions initialization
  
  pthread_cond_init(&thread_cond_one, NULL);
  pthread_cond_init(&thread_cond_two, NULL);
  pthread_cond_init(&thread_cond_three, NULL);
  
  //register signal handler
  
  signal(SIGSEGV,sig_func); // Register signal handler before going multithread
  
  // Thread creation
  
  pthread_attr_init(&attr);
  pthread_create(&scheduler_thread, &attr, watch_count, NULL); // This is the scheduler thread
  pthread_create(&mmu_thread,NULL,MMU,NULL); // this is the MMU thread 
  pthread_create(&func_one_thread, &attr, func_one, NULL); // this is worker thread doing page request
  pthread_create(&func_two_thread, &attr, func_two, NULL);  // this is worker thread doing page request
  pthread_create(&func_three_thread, &attr, func_three, NULL);  // this is worker thread doing page request
 
 //create the pipes PIPES are here because all threads should be able to use same pipe else we would use n number of pipes 
 
 result = pipe (fd);
   if (result < 0){
       perror("pipe ");
       exit(1);
				}
				
	result = pipe (fd1);
   if (result < 0){
       perror("pipe ");
       exit(1);
   }
 
  // main() launches these four threads and starts waiting for their completion
  
  pthread_join(mmu_thread, NULL);
  pthread_join(scheduler_thread, NULL);
  pthread_join(func_one_thread, NULL);
  pthread_join(func_two_thread, NULL);
  pthread_join(func_three_thread, NULL);
 
  // All threads exited normally
  
  printf("Exiting all threads normally \n");
 
  return 0;
}
