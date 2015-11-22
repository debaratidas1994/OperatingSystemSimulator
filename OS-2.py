from multiprocessing import *
from threading import Thread
import random,signal
import os
num_requests=0
l=Lock()
page_fault=0
time=0
tlb_miss=0
end_for_app,end_for_mmu=Pipe()
end_for_mmuS,end_for_schedulerM=Pipe()
sem1=Semaphore(0)#for waking mmu from app
sem2=Semaphore(0)#for waking scheduler from mmu
end=Semaphore(0)#for indicating current process is over...
switch=Semaphore(0)#for invoking mmu switch
switch_over=Semaphore(0)
proc_name=Queue()
class  MMU(Process):
	app_pipe=end_for_mmu
	sem_scheduler=sem2
	sem_app=sem1
	scheduler_pipe=end_for_mmuS
	def __init__(self,t,p,taccess,phit,pmiss,n):
		self.tlb={}#page_no:num_accessed
		self.memory={}#page_no:num_accessed//pid:pageTable//page_no:num_accessed
		self.t=t
		self.p=p
		self.taccess=taccess
		self.phit=phit
		self.current_process=0
		self.pmiss=pmiss
		self.count=0
		self.finished=0
		Process.__init__(self)

		self.n=n
	def run(self):
		global num_requests,page_fault,time,tlb_miss
		t=Thread(target=self.mmu_switch)
		t.start()
		while True:
			MMU.sem_app.acquire()	
			page_num=MMU.app_pipe.recv()
			num_requests+=1
			if page_num==-1:
				num_requests-=1
				self.finished+=1
				if self.finished==self.n:
					print 'Page Fault Rate',page_fault/float(num_requests)
					print 'Effective Memory Access Time',time/float(num_requests)
					print 'Number of TLB Misses',tlb_miss
					os.kill(os.getppid(),signal.SIGTERM)
					os.kill(os.getpid(),signal.SIGTERM)
				end.release()
				continue
			elif page_num in self.tlb:
			#	print 'Request by ',proc_name.get()+'. Page num',page_num,'Time:',str(self.taccess+self.phit)
				
				for i in self.tlb:
					self.tlb[i]+=1
				self.tlb[page_num]=0
				time+=self.taccess+self.phit
			elif page_num in self.memory:
		#		print 'TLB MISS'
		#		print 'Request by ',proc_name.get()+'. Page num',page_num,'Time:',str(self.taccess+2*self.phit)
				tlb_miss+=1
				max_num=-1
				max_page=0
				for i in self.memory:
					self.memory[i]+=1
				self.memory[page_num]=0
				if len(self.tlb)==self.t:
					for i in self.tlb:
						if self.tlb[i]>max_num:
							max_num=self.tlb[i]
							max_page=i
					del self.tlb[max_page]
				for i in self.tlb:
					self.tlb[i]+=1
				self.tlb[page_num]=0
				time+=self.taccess+2*self.phit				
			else:

		#		print 'PAGE FAULT'
				page_fault+=1
				tlb_miss+=1
				max_num,max_page=-1,0
				if len(self.memory)==self.p:
					for i in self.memory:
						if self.memory[i]>max_num:
							max_num=self.memory[i]
							max_page=i
					del self.memory[max_page]
				#self.memory[self.current_process][page_num]=0#its below..
				for i in self.memory:
					self.memory[i]+=1
				self.memory[page_num]=0
				max_num=-1
				max_page=0
				if len(self.tlb)==self.t:
					for i in self.tlb:
						if self.tlb[i]>max_num:
							max_num=self.tlb[i]
							max_page=i
					del self.tlb[max_page]
				for i in self.tlb:
					self.tlb[i]+=1
				self.tlb[page_num]=0
		#		print 'Request by ',proc_name.get()+'. Page num',page_num,'Time:',str(self.taccess+self.phit+self.pmiss)
				time+=self.taccess+self.phit+self.pmiss
			MMU.sem_scheduler.release()

	def mmu_switch(self):
		while True:
			switch.acquire()
			self.count=0
			self.tlb={}
			self.memory={}			
			switch_over.release()

class  App(Process):
	mmu_pipe=end_for_app
	sem_mmu=sem1
	def __init__(self,name,max_req,total_page,sem):
		Process.__init__(self)
		self.name=name
		self.max_req=max_req
		self.total_page=total_page
		self.sem=sem
	def run(self):
		count=1
		while count<=self.max_req:
			self.sem.acquire()
			page_num=random.randint(1,self.total_page)
			proc_name.put(self.name)
			App.mmu_pipe.send(page_num)
			App.sem_mmu.release()
			count+=1
		self.sem.acquire()
		App.mmu_pipe.send(-1)	
		App.sem_mmu.release()
class Scheduler_details:
	pass
class Scheduler(Process):
	def __init__(self,C):
		Process.__init__(self)	
		self.process_list=[]
		self.semaphore_list=[]
		self.current_process=0
		self.count=0
	
	def remove_current_process(self):
		while True:
			end.acquire()
			del self.process_list[self.current_process]
			del self.semaphore_list[self.current_process]
			if len(self.process_list)==0:
			 	break
			switch.release()
			switch_over.acquire()
			self.count=0
			self.current_process=(self.current_process)%len(self.process_list)
			self.semaphore_list[self.current_process].release()
	def schedule(self):
		self.current_process=0
		self.count=0
		self.semaphore_list[self.current_process].release()
		while True:
			sem2.acquire()
			self.count+=1
			if self.count==self.C:
				switch.release()
				switch_over.acquire()
				self.current_process=(self.current_process+1)%len(self.process_list)
				self.count=0
			self.semaphore_list[self.current_process].release()
	def run(self):
		with open('proc_config.txt') as f:
			t=f.readlines()
			for i in t[1:]:
				i=i.split(',')
				self.semaphore_list.append(Semaphore(0))
				self.process_list.append(App(i[0],int(i[2].strip()),int(i[1].strip()),self.semaphore_list[-1]))
		with open('scheduler_config.txt') as f:
			self.C=int(f.readlines()[-1])
		with open('mmu_config.txt') as f:
			t=f.readlines()[-1]#P,Phit,Pmiss,T,Taccess..t,p,taccess,phit,pmiss,C
			k=map(int,t.split(','))
			mmu=MMU(k[3],k[0],k[-1],k[1],k[2],len(self.process_list))
		mmu.start()
		for proc in self.process_list:
			proc.start()
		t=Thread(target=Scheduler.remove_current_process,args=(self,))
		t.start()
		self.schedule()
if __name__=='__main__':
	Scheduler(None).start()
