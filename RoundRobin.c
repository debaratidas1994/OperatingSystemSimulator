/* Round Robin Algorithm PROGRAM: */
/* 

This is a simplistic simulation : it assumes that all procesees are there and just does the scheduling 
This simulation DOES NOT take care of process arrival time  


AIM

To write a c program to implement the round robin scheduling algorithm

ALGORITHM

1. Start the process

2. Declare the array size

3. Get the number of elements to be inserted

4. Get the value

5. Set the time sharing system with preemption

6. Define quantum

7. Declare the queue as a circular

8. Make the CPU scheduler goes around the ready queue allocating CPU to each process
    for the time interval specified

9. Make the CPU scheduler picks the first process and sets time to interrupt after quantum
    expired dispatches the process

9. If the process have burst less than the time quantum than the process release the CPU

10. If the process have bust greater then time quantum then time will go off and cause
      interrupt to OS and the process put into the tail of ready queue and the schedule select
      next process

11. Display the results

12. Stop the process



*/
#include<stdio.h>
int ttime,i,j,temp;
int main()
{
        int pname[10],btime[10],pname2[10],btime2[10];
        int n,x,z;
        printf("Enter the no. of process:");
        scanf("%d",&n);
        for(i=0;i<n;i++)
        {     
				printf("Enter the %dth process name:", i);
                scanf("%d",&pname2[i]);
                printf("Enter burst time for %d th process %d:",i, pname2[i]);
                scanf("%d",&btime2[i]);
        }
        printf("PROCESS NAME \t\t BURST TIME\n");
        for(i=0;i<n;i++)
            printf("%d\t\t\t %d\n",pname2[i],btime2[i]);
            z=1;
        while(z==1)
        {
ttime=0;
        for(i=0;i<n;i++)
        { 
pname[i]=pname2[i];
            btime[i]=btime2[i];
        }

            printf ("PRESS 1.ROUND ROBIN 2.EXIT\n");
            scanf("%d",&x);
        switch(x)
        {
        case 1:
                rrobin(pname,btime,n);
                break;
        case 2:
                exit(0);
                break;
        default:
                printf("Invalid option");
               break;
       }
       printf("\n\n If you want to continue press 1:");
       scanf("%d",&z);
       }
       return 0;
}
     
 rrobin(int pname[],int btime[],int n)
       {
            int tslice;
            j=0;
            printf("\n\t ROUND ROBIN SCHEDULING \n\n");
            printf("Enter the time slice:\n");
            scanf("%d",&tslice);
            printf("PROCESS NAME \t REMAINING TIME\t TOTAL TIME");
    

while(j<n)
            { 
      for(i=0;i<n;i++)
{
      if(btime[i]>0)
                        { 
if(btime[i]>=tslice)
                                    { 
ttime+=tslice;
                                                btime[i]=btime[i]-tslice;
                                                printf("\n%d\t\t %d \t\t %d",pname[i],btime[i],ttime);
                                                if(btime[i]==0)
                                                 j++;
                                    }
                                    else
                                    { 
ttime+=btime[i];
                                                btime[i]=0;
                                                printf("\n%d\t\t %d \t\t %d",pname[i],btime[i],ttime);
                                    }
}
 }
       }
}
