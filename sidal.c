#include "utils.h"

#define DELAY 1<<0
#define START 1<<1
#define STOP 1<<2
#define AVAIL 1<<3
#define UNAVAIL 1<<4
#define WAIT 1<<5
#define MARK 1<<6

static short int mode=0; //consists of flags above
static int delay=1;

static int doservice(const char *path);
static char* findpath(char *name);
static int runservice(const char *target);
static void usage(char *name);

int
main(int argc, char *argv[])
{
	if (argc<2)
		usage(argv[0]);
	if (strchr(argv[1],'c')){
		system("mkdir /etc/sidal/avail");
		system("mkdir /etc/sidal/default");
		system("mkdir /etc/sidal/run");
		system("mkdir /run/sidal");
		exit(0);
		//usage(argv[0]);
	}
	if (strchr(argv[1],'l')){
		system("ls /etc/sidal/run");
		exit(0);
		//usage(argv[0]);
	}
	if (strchr(argv[1],'a'))
		mode=mode|AVAIL;
	else if (strchr(argv[1],'u'))
		mode=mode|UNAVAIL;
	else if (strchr(argv[1],'s'))
		mode=mode|START;
	else if (strchr(argv[1],'k'))
		mode=mode|STOP;
	else if (strchr(argv[1],'r'))
		mode=mode|START|STOP;
	if (strchr(argv[1],'w'))
		mode=mode|WAIT;
	if (strchr(argv[1],'m'))
		mode=mode|MARK;
	if (strchr(argv[1],'d'))
		{
			mode=mode|DELAY;
			sscanf(argv[2],"%d",&delay);		
		}
	if (!(argc-2-(mode&DELAY)))
		if (mode&AVAIL)
			system("ls /etc/sidal/avail");
		else if (mode & START)
			system("ls /run/sidal");
		else usage(argv[0]);
	for (int i=0;i<argc-2-(mode&DELAY);i+=1)
		runservice(smprintf("%s",argv[2+(mode&DELAY)+i]));
		
	return 0;
}

int
doservice(const char *path)
{
	char **cmd=(char **)malloc(3*sizeof(char*));
	cmd[0]=path;
	cmd[2]=NULL;
	if ((mode & START) && (mode & STOP) )
		cmd[1]="restart";
	else if (mode & START)
		cmd[1]="start";
	else if (mode & STOP)
		cmd[1]="stop";
	else cmd[1]=NULL;
	pid_t service=spawn(cmd);
	free(cmd);
	int ret=1;
	if (mode & WAIT){
		int wstatus;
		waitpid(service,&wstatus,0);
		ret=WIFEXITED(wstatus);
	}
	return ret;
}

char*
findpath(char *name)
{
	char **paths;
	FILE *fp;
	if ( (mode & START) && (mode & STOP) ){
		paths=(char**)malloc(3*sizeof(char*));
		paths[0]="/run/sidal/";
		paths[1]="/etc/sidal/run/";
		paths[2]=NULL;
	}
	else if (mode & STOP){
		paths=(char**)malloc(2*sizeof(char*));
		paths[0]="/run/sidal/";
		paths[1]=NULL;
	}
	else if (mode & START){
		paths=(char**)malloc(10*sizeof(char*));
		paths[0]="/etc/sidal/run/";
		paths[1]="/bin/";
		paths[2]="/sbin/";
		paths[3]="/usr/bin/";
		paths[4]="/usr/sbin/";
		paths[5]="/usr/local/bin/";
		paths[6]="/usr/local/sbin/";
		paths[7]="";
		paths[8]="./";
		paths[9]=NULL;
	}
	else if (mode & AVAIL){
		paths=(char**)malloc(2*sizeof(char*));
		paths[0]="/etc/sidal/avail/";
		paths[1]=NULL;
	}
	else if (mode & UNAVAIL){
		paths=(char**)malloc(2*sizeof(char*));
		paths[0]="/etc/sidal/run/";
		paths[1]=NULL;
	}
	for(int i=0;paths[i];i+=1){
		if ((fp=fopen(smprintf("%s%s",paths[i],name),"r"))){
			const char *path=paths[i];
			free(paths);
			fclose(fp);
			return smprintf("%s%s",path,name);
		}
	}
	return 0;
}

int
runservice(const char *target)
{
	char *path=findpath(target);
	if (!path){
		warn("cannot find service: %s",target);
		return -1;
	}
	if (mode&START | mode&STOP){
		printf("%s service: %s\n",((mode & START) && (mode & STOP)) ? "restarting" : (mode & START) ? "starting" : "stopping",target);
		int res=doservice(path);
		if (mode&WAIT){
			if (res)
				printf("service %s successfully: %s\n",((mode & START) && (mode & STOP)) ? "restarted" : (mode & START) ? "started" : "stopped",target);
			else printf("errors occured while %s service: %s\n",((mode & START) && (mode & STOP)) ? "restarting" : (mode & START) ? "starting" : "stopping",target);
		}
		if (mode&MARK && res){
			if (mode & STOP)
				system(smprintf("rm %s",path));
			if (mode & START)
				system(smprintf("ln -sf %s /run/sidal",path));
		}
		if (mode & DELAY)
			sleep(delay);
	}
	else if (mode & AVAIL)
		system(smprintf("ln -sf %s /etc/sidal/run",path));
	else if (mode & UNAVAIL)
		system(smprintf("rm %s",path));
	return 0;
}

void
usage(char *name)
{
	printf("%s [-skrwmdfh] [--delay d] [--linkdir dir]\n",name);
	exit(0);
}