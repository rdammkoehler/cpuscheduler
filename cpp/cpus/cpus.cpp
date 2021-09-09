#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define max(a,b) (a > b)?a:b

#define MAJICRATIO 0.30

#define CPU 1
#define IO 0

#define TRUE 0
#define FALSE 1

#define BUFFSIZE 256

//The only global var anywhere
FILE *log = stdout;
FILE *out = stdout;

enum stateT { loading, ready, execute, iocycle, complete };

/* jobT represents a cpu job
	All jobs begin w/ a CPU cycle, cpu cycles can be 0 in length to rep. a no op
	All jobs end w/ an IO cycle where next cpu is -1, 
		io cycles can be 0 in length to rep. a no op
*/
class jobT {
	public:
		long jno;		//job number
		stateT state;	//curent state of the job
		jobT *next;		//pointer to the next job

	/*constructors*/ 		
		jobT() { jno = 0; state = loading; cpu = NULL; io = NULL; next = NULL; cpuct = 0; ioct = 0; };
		jobT(long n) { jno = n; state = loading; cpu = NULL; io = NULL; next = NULL; cpuct = 0; ioct = 0; };
		jobT(long n, stateT s) { jno =n, state = s; cpu = NULL; io = NULL; next = NULL; cpuct = 0; ioct = 0; };
		jobT(long n, stateT s, long *c, long *i) { jno = n, state = s, cpu = c, io = i, next = NULL; cpuct = 0; ioct = 0; };

	/*destructors*/
		~jobT() { free(cpu); free(io); };

	/*methods*/
		long CpuCt() { return(cpuct); };
		long CpuCt(long x) { cpuct = x; return(cpuct); };
		long IoCt() { return(ioct); }
		long IoCt(long x) { ioct = x; return(ioct); };
		char *toString();
		long pop(long x);
		long push(long x,long val);
		long Cpu() 			{ return(cpu[0]); };
		long Cpu(long x) 	{ return(cpu[x]); };
		long Io() 			{ return(io[0]); };
		long Io(long x) 	{ return(io[x]); };
		jobT *AddEl(long btype, char *buffer);
		long AddBurst(long btype, long x);
		
	private:
		long ct(char *buffer); //count entries in buffer
		long *cpu;		//pointer to an array or dynamic list of cpu bursts
		long cpuct;
		long *io;		//pointer to an array or dynamic listof io bursts
		long ioct;
};

long jobT::pop(long x) 
{
	long r = (x == CPU)?cpu[0]:io[0];
	long i;
	for(i = 1;i < ((x == CPU)?cpuct+1:ioct);i++)
		if (x == CPU)
			cpu[i-1] = cpu[i];
		else
			io[i-1] = io[i];
	if (x == CPU) 
		CpuCt(cpuct-1);
	else
		IoCt(ioct-1);
	return(r);
}

long jobT::push(long x,long val)
{
	long i;
	for(i = (((x == CPU)?cpuct+1:ioct)+1);i > 0 ;i--)
		if (x == CPU)
			cpu[i] = cpu[i-1];
		else
			io[i] = io[i-1];

	if (x == CPU)
	{
		cpu[0] = val;
		CpuCt(cpuct+1);
	}
	else
	{
		io[0] = val;
		IoCt(ioct+1);
	}
	return(val);
}

long jobT::AddBurst(long btype, long x)
{ 
	if (btype == CPU)
	{
		++cpuct;
		if ((cpuct - 1) == 0)
			cpu = (long *) malloc(sizeof(long) * cpuct);
		else
			cpu = (long *) realloc(cpu,sizeof(long) * cpuct);
		cpu[cpuct-1] = x;
	}
	else
	{
		++ioct;
		if ((ioct - 1) == 0)
			io = (long *) malloc(sizeof(long) * ioct);
		else
			io = (long *) realloc(io,sizeof(long) * ioct);
		io[ioct-1] = x;
	}
	return((btype == CPU)?cpuct:ioct);
}

jobT *jobT::AddEl(long btype, char *buffer)
{
	char *p, *r;
	long 	i = 0, stopat = ((btype == CPU)?CpuCt():IoCt()) + ct(buffer),
			listct = ((btype == CPU)?CpuCt():IoCt());
	
	p = r = buffer;
	for(i = listct; i < stopat;i++)
	{
		if((p = strstr(r,",")) != NULL)
		{
			p[0] = '\0';
			AddBurst((btype == CPU)?CPU:IO,atol(r));
			p[0] = ',';
			r = ++p;
		}
		else
		 AddBurst((btype == CPU)?CPU:IO,atol(r));
	}

	return(this);
}

long jobT::ct(char *buffer)
{
	char *q,*r;
	long elct = 0;

	r = q = buffer;
	elct = ((q = strstr(r,",")) != NULL)?1:0;
	while ( (q = strstr(r,",")) != NULL ) { elct++;	r = ++q; }

	return (elct);
}

char *jobT::toString()
{
	long i = 0;
	char *s = (char *) malloc(sizeof(char) * 8096);

	strcpy(s,"###########################|\t");
	for(i = 0; i < ((cpuct > ioct)?cpuct:ioct);i++) sprintf(s,"%s |%4d",s,i);
	sprintf(s,"%s |\n---------------------------|\t ",s);
	for(i = 0; i < (((cpuct > ioct)?cpuct:ioct)*6);i++) sprintf(s,"%s%c",s,'-');
	sprintf(s,"%s\nJob Number: <%8d>(%3d)|\t",s,jno,cpuct);
	for(i = 0; i < cpuct;i++) sprintf(s,"%s |%4ld",s,cpu[i]);
	sprintf(s,"%s |\nState:      <%8s>(%3d)|\t",s,(state == loading)?"loading":(state == ready)?"ready":(state == execute)?"execute":(state == iocycle)?"io":"complete",ioct);
	for(i = 0; i < ioct;i++) sprintf(s,"%s |%4ld",s,io[i]);
	sprintf(s,"%s |\n---------------------------|\t ",s);
	for(i = 0; i < (((cpuct > ioct)?cpuct:ioct)*6);i++) sprintf(s,"%s%c",s,'-');
	
	return(s);
}

jobT *loadjob(long jno, jobT **job, FILE *f);
jobT *getJobs(jobT *job, long i);
void showqueue(jobT *q);
void schedual(jobT *jobs);

long go(jobT *jq, long cycle, long maxcyc);

/*eventually these will be part of the cpuT class*/
jobT *dequeue(jobT **jobs);
jobT *enqueue(jobT *jobs, jobT *newJob);

long calcmajic(jobT *j);

void pp_header(const char *algo_name, FILE *file);
void pp_footer(const char *algo_name, FILE *file);

int main(long argc, char argv[], char envp[])
{
	jobT *job;

	log = fopen("jjjob.log","w");
	out = fopen("jjjob.rpt","w");
	time_t start, finish;

	pp_header("Cooperative Multi-Tasking Example",log);
	pp_header("Cooperative Multi-Tasking Example",out);
	job = getJobs(job, 0);
	schedual(job);
	showqueue(job);
	fprintf(stdout,"Begining Cooperative Multi-Tasking Simulation\n");
	start = time(&start);
	fprintf(stdout,"Report: (%ld) cycles were executed.\n",go(job,0,0));
	finish = time(&finish);
	fprintf(stdout,"Report: Elapsed Time: (%.4f) seconds\n",difftime(finish,start));
	fprintf(stdout,"Ending Cooperative Multi-Tasking Simulation\n");
	pp_footer("Cooperative Multi-Tasking Example",out);
	pp_footer("Cooperative Multi-Tasking Example",log);

	pp_header("Pre-Emptive (50) Multi-Tasking Example",log);
	pp_header("Pre-Emptive (50) Multi-Tasking Example",out);
	job = getJobs(job, 0);
	schedual(job);
	showqueue(job);
	fprintf(stdout,"Begining Pre-Emptive (50 cycle) Multi-Tasking Simulation\n");
	start = time(&start);
	fprintf(stdout,"Report: (%ld) cycles were executed.\n",go(job,0,50));
	finish = time(&finish);
	fprintf(stdout,"Report: Elapsed Time: (%.4f) seconds\n",difftime(finish,start));
	fprintf(stdout,"Ending Pre-Emptive (50 cycle) Multi-Tasking Simulation\n");
	pp_footer("Pre-Emptive (50) Multi-Tasking Example",out);
	pp_footer("Pre-Emptive (50) Multi-Tasking Example",log);

	pp_header("Pre-Emptive (1000) Multi-Tasking Example",log);
	pp_header("Pre-Emptive (1000) Multi-Tasking Example",out);
	job = getJobs(job, 0);
	schedual(job);
	showqueue(job);
	fprintf(stdout,"Begining Pre-Emptive (1000 cycle) Multi-Tasking Simulation\n");
	start = time(&start);
	fprintf(stdout,"Report: (%ld) cycles were executed.\n",go(job,0,1000));
	finish = time(&finish);
	fprintf(stdout,"Report: Elapsed Time: (%.4f) seconds\n",difftime(finish,start));
	fprintf(stdout,"Ending Pre-Emptive (1000 cycle) Multi-Tasking Simulation\n");
	pp_footer("Pre-Emptive (1000) Multi-Tasking Example",out);
	pp_footer("Pre-Emptive (1000) Multi-Tasking Example",log);

	pp_header("ADAPTIVE Multi-Tasking Example",log);
	pp_header("ADAPTIVE Multi-Tasking Example",out);
	job = getJobs(job, 0);
	schedual(job);
	showqueue(job);
	fprintf(stdout,"Begining Adaptive Multi-Tasking Simulation\n");
	start = time(&start);
	{
		long majic = calcmajic(job);
		fprintf(stdout,"Report: (%ld) cycles were executed, using an adaptive step of %ld.\n",go(job,0,majic),majic);
		finish = time(&finish);
	}
	fprintf(stdout,"Report: Elapsed Time: (%.4f) seconds\n",difftime(finish,start));
	fprintf(stdout,"Ending Adaptive Multi-Tasking Simulation\n");
	pp_footer("ADAPTIVE Multi-Tasking Example",out);
	pp_footer("ADAPTIVE Multi-Tasking Example",log);
	
	fclose(out);
	fclose(log);

	while(getchar() != '\n');
	
	return 0;
}

void pp_header(const char *algo_name, FILE *file)
{
	long i;
	fprintf(file,"################################################################################\n");
	for(i = 0; i < (78 - ((long) strlen(algo_name)));i++)
		fprintf(file,"#");
	fprintf(file," %s ",algo_name);
	for(i+=(long) strlen(algo_name);i < 78;i++)
		fprintf(file,"#");
	fprintf(file,"\n################################## Begins ######################################\n");
	fprintf(file,"################################################################################\n\n");
	return;
}

void pp_footer(const char *algo_name, FILE *file)
{
	long i;
	fprintf(file,"################################################################################\n");
	for(i = 0; i < (78 - ((long) strlen(algo_name)));i++)
		fprintf(file,"#");
	fprintf(file," %s ",algo_name);
	for(i+=(long) strlen(algo_name);i < 78;i++)
		fprintf(file,"#");
	fprintf(file,"\n################################## Ends ########################################\n");
	fprintf(file,"################################################################################\n\n");
	return;
}

long calcmajic(jobT *j)
/*The Idea here is to optimize for the nastyest job*/
{
	long i = 0,stop = 0,maxcpu = 0, maxio = 0, avgcpu = 0, avgio = 0, majic = 0;
	if (j != NULL)
	{
		stop = j->CpuCt();
		for(i=0;i<stop;i++)
		{
			avgcpu = (long) (((avgcpu * (i-1)) + j->Cpu(i))/(i+1));
			maxcpu = (long) max(j->Cpu(i),maxcpu);
		}
		stop = j->IoCt();
		for(i=0;i<stop;i++)
		{
			avgio = (long) (((avgio * (i-1)) + j->Io(i))/(i+1));
			maxio =  (long) max(j->Io(i),maxio);
		}
		majic = (long) ((maxcpu + maxio)/2) + (MAJICRATIO * (maxcpu > maxio)?maxcpu:maxio);
		return( (long) max(calcmajic(j->next),majic) );
	}
	else
		return(0);
}

jobT *enqueue(jobT *jobs, jobT *newJob)
{
	jobT *p = jobs;

	if (p != NULL)
	{
		while(p->next != NULL) p = p->next;
		p->next = newJob;
		return( jobs );
	}
	else
		return( newJob );
}

jobT *dequeue(jobT **jobs)
{
	jobT *p = *jobs;
	
	if (p != NULL)
		*jobs = p->next;

	p->next = NULL;

	return(p);
}

long go(jobT *jq, long cycle, long maxcyc)
{
	jobT *p = jq, *q = NULL;
	long thistime = 0, val = 0;
	long allcomplete = FALSE,
		eburstcompleted = FALSE;

	fprintf(log,"################################################################################\n");
	fprintf(log,"######################## Starting Scheduler Simulation! ########################\n");
	fprintf(log,"################################################################################\n\n");
	if (p != NULL)
	{
		while(allcomplete == FALSE)
		{
			p = jq;
			fprintf(log,"################################################################################\n");
			fprintf(log,"Cycles Before <%5ld>\n",cycle);
			while ((p != NULL) && (p->CpuCt() < 1) && (p->IoCt() < 1))
			{
				p = dequeue(&jq);
				p->state = complete;
				fprintf(log,"!!!JOB COMPLETED!!!\n%s\n",p->toString());
				p = jq;
			}
			if ((p != NULL) && (p->state != ready))
			{
				q = p = dequeue(&jq);
				p = jq = enqueue(jq,p);
				while ((p->state != ready) && (p != q))
				{
					p = dequeue(&jq);
					p = jq = enqueue(jq,p);
				}
			}
			if ((p != NULL) && (p->state == ready))
			{	
				p->state = execute;
				fprintf(log,"%s\n",p->toString());
				if (maxcyc == 0)
				{
					cycle += thistime = p->pop(CPU);
					eburstcompleted = TRUE;
				}
				else
				{
					if ((val = p->pop(CPU)) > maxcyc)
					{
						p->push(CPU,val - maxcyc);
						cycle += thistime = val;
						eburstcompleted = FALSE;
					}
					else
					{
						cycle += thistime = val;
						eburstcompleted = TRUE;
					}
				}
				fprintf(out,"cycle: %5ld job %5d cpu %5ld\n",cycle, p->jno, thistime);
				p = p->next;
				while(p != NULL)
				{
					switch(p->state) {
					case ready :	//no op
						
						break;
					case execute : 
							
						break;
					case iocycle :	//all jobs goto complete here!
							if (( val = p->pop(IO) - thistime ) <= 0)
								p->state = ready;
							else
								p->push(IO,val);
							fprintf(out,"cycle: %5ld job %5d io  %5ld\n",cycle, p->jno, val + thistime);
							if ((p->CpuCt() < 1) && (p->IoCt() < 1))
								p->state = complete;
						break;
					case complete :	
						break;
					default : 
							fprintf(log,"Error: Job (%d) is not ready, willing or able.\n",p->jno);
						break;
					}
					fprintf(log,"%s\n",p->toString());
					p = p->next;
				}
				p = dequeue(&jq);
				if (eburstcompleted == TRUE)
					p->state = iocycle;
				else
					p->state = execute;
				jq = enqueue(jq,p);
			}
			else
			{
				if ((p != NULL) && (p->state = iocycle))
				{
					fprintf(log,"%s\n",p->toString());
					/***/
					if (maxcyc == 0)
					{
						cycle += thistime = p->pop(IO);
						eburstcompleted = TRUE;
					}
					else
					{
						if ((val = p->pop(IO)) > maxcyc)
						{
							p->push(IO,val - maxcyc);
							cycle += thistime = val;
							eburstcompleted = FALSE;
						}
						else
						{
							cycle += thistime = val;
							eburstcompleted = TRUE;
						}
					}
					/***/
					fprintf(out,"cycle: %5ld job %5d io %5ld\n",cycle, p->jno, thistime);
					p = p->next;
					while(p != NULL)
					{
						switch(p->state) {
						case ready :	//no op
							break;
						case execute : 
								
							break;
						case iocycle :	//all jobs goto complete here!
								if (( val = p->pop(IO) - thistime ) <= 0)
									p->state = ready;
								else
									p->push(IO,val);
								fprintf(out,"cycle: %5ld job %5d io  %5ld\n",cycle, p->jno, val + thistime);
								if ((p->CpuCt() < 1) && (p->IoCt() < 1))
									p->state = complete;
							break;
						case complete :	
							break;
						default : 
								fprintf(log,"Error: Job (%d) is not ready, willing or able.\n",p->jno);
							break;
						}
						fprintf(log,"%s\n",p->toString());
						p = p->next;
					}
					p = dequeue(&jq);
					if (eburstcompleted == TRUE)
						p->state = ready;
					else
						p->state = iocycle;
					jq = enqueue(jq,p);
				}
				else
				{
					allcomplete = TRUE;
					showqueue(jq);
				}
			}
			fprintf(log,"Cycles After <%5ld>\n\n",cycle);
			fprintf(log,"################################################################################\n");
			fflush(log);
		}													 
	}
	fprintf(log,"################################################################################\n");
	fprintf(log,"######################### Ending Scheduler Simulation! #########################\n");
	fprintf(log,"################################################################################\n\n");

	return (cycle);
}

void showqueue(jobT *q)
{
	if (q != NULL)
	{
		fprintf(log,"%s\n",q->toString());
		if (q->next != NULL) showqueue(q->next);
		fprintf(log,"\n");
	}
	else
		fprintf(log,"The Queue is Empty!\n");
	return;
}

void schedual(jobT *jobs)
{
	jobs->state = ready;
	if (jobs->next != NULL) schedual(jobs->next);

	return;
}


jobT *getJobs(jobT *job, long i)
{
	FILE *in;
	char fname[13];

	sprintf(fname,"%d.job",i++);
	in = fopen(fname,"r");
	if ((job = loadjob(i,&job, in)) != NULL)
	{
		fclose(in);
		job->next = getJobs(job->next,i);
	}	

 	return(job);
}

jobT *loadjob(long jno, jobT **job, FILE *f)
{
	char buffer[BUFFSIZE];
	long i = 0;

	if (f != NULL)
	{
		while (!feof(f))
		{	
			fgets(buffer,BUFFSIZE,f);
			//cpu
			*job = new jobT(jno,loading);
			if (strstr(buffer,"ENDCPU") != NULL)
				(*job)->AddEl(CPU,buffer);
			else
			{
				(*job)->AddEl(CPU,buffer);
				fgets(buffer,BUFFSIZE,f);
				while (strstr(buffer,"ENDCPU") == NULL)
				{												 
					(*job)->AddEl(CPU,buffer);
					fgets(buffer,BUFFSIZE,f);
				}
				(*job)->AddEl(CPU,buffer);
			}
			//io
			fgets(buffer,BUFFSIZE,f);
			if (strstr(buffer,"ENDIO") != NULL)
				(*job)->AddEl(IO,buffer);
			else
			{
				(*job)->AddEl(IO,buffer);
				fgets(buffer,BUFFSIZE,f);
				while (strstr(buffer,"ENDIO") == NULL)
				{
					(*job)->AddEl(IO,buffer);
					fgets(buffer,BUFFSIZE,f);
				}
				(*job)->AddEl(IO,buffer);
			}
			fgets(buffer,BUFFSIZE,f); // should clear the buffer
		}
		(*job)->AddBurst(CPU,-1); //mark the end of all cpu bursts
		return( *job );
	}
	else
		return( NULL );
}
