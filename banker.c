#include<stdio.h>
int alloc[10][10];
int max[10][10];
int cneed[10][10];
int a[10];
int total[10];
int main()
{
	int r,p,i,j,k,flag,f,count,x,flag1;
	count=0;
	flag1=0;
	printf("\nEnter number of resources:");
	scanf("%d",&r);
	f=r;
	printf("\nEnter total memory of given resources sequentially:");
	for(i=0;i<r;i++)
		scanf("%d",&total[i]);
	printf("\nEnter number of processes:");
	scanf("%d",&p);
	printf("\nEnter Allocated and Maxneed memory...press a key to continue");
	getchar();
	for(i=0;i<p;i++)
	{
		for(j=0;j<r;j++)
		{
			printf("\nFor Process %d resource %d =",i,j);
			scanf("%d%d",&alloc[i][j],&max[i][j]);
			//calculating current need
			cneed[i][j]=max[i][j]-alloc[i][j];
		}
	}
	printf("\nSequence of execution is...\n");
	k=0;
	for(j=0;j<r;j++)
	{
		for(i=0;i<p;i++)
		{
			a[k]+=alloc[i][j];
		}
		a[k]=total[k]-a[k];
		k++;
	}
	while(count!=p)
	{
		for(i=0;i<p;i++)
		{
			flag=0;
			if(cneed[i][f]!=1)
			{
				for(j=0;j<r;j++)
				{
					total[j]=a[j]-cneed[i][j];
				}
				for(k=0;k<r;k++)
				{
					if(total[k]<0)
					{
						flag=1;
					}
				}
			}
			if((flag==0)&&(cneed[i][f]!=1))
				break;
		}
		x=i;
		cneed[x][f]=1;
		printf("P%d->",x);
		count++;
		for(i=0;i<r;i++)
		{
			total[i]+=max[x][i];
			a[i]=total[i];
		}
		for(i=0;i<p;i++)
		{
			if((cneed[i][0]<a[0])&&(cneed[i][f]==0))
				flag1=1;
		}
		if(flag1==0)
			break;
	}
	if(flag1==0)
		printf("\nUnsafe...");
	else
		printf("\nSafe...");
	getchar();
	return 0;
}
