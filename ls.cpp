#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include <getopt.h>
#include <stdlib.h>
#include <libgen.h>
#define MAXLEN 2048
int all = 0;
int low = -1;
int high = -1;
int modify = -1;
int recursive = 0;
char dirStack[100][MAXLEN]={{0}}; // Stores all directory
int stackCount = 0; // count all directories

void printEntry(char *path){
	struct stat st;
	time_t now;
	time(&now);
	if (stat(path,&st) < 0){
		printf("\e[31mOpen file\"%s\":%s(ERROR %d)\e[0m\n",path,strerror(errno),errno);
		return;
	}
	if (all == 0 && (basename(path)[0] == '.')) return;//判断是否全部显示
	if (low !=-1 && st.st_size < low) return; //判断文件大小下界
	if (high !=-1 && st.st_size > high) return; //判断文件大小上界
	if (modify !=-1 && ((now -st.st_ctime)/(60*60*24) > modify)) return;//判断文件修改时间
	if ((st.st_mode & S_IXUSR) == S_IXUSR) printf("\e[32m\e[1m");//判断是否可执行
	if (S_ISDIR(st.st_mode)) printf("\e[34m\e[1m");
	printf("%s\e[0m\n",basename(path));
}

void listDir(char *dir){
	struct dirent ** namelist;
	int i,n;
	char newp[MAXLEN],path[MAXLEN];
	strcpy(path,dir);
	n = scandir(path,&namelist,0,alphasort);
	if (n < 0){
		printf("\e[31mOpen directory\"%s\":%s(ERROR %d)\e[0m\n",path,strerror(errno),errno);
		return;
	}
	if (recursive) printf("\n%s\n",dir);
	for (i = 0;i < n;i++){
		memset(newp,0,sizeof(char)*MAXLEN);
		strcpy(newp,path);
		newp[strlen(newp)]='/';
		strcat(newp,namelist[i]->d_name);
		printEntry(newp);
		
		if (	recursive // Show recursively
				&& (namelist[i]->d_type == DT_DIR) // It is a directory
				&& (strcmp(namelist[i]->d_name,".") != 0) // It is not current directory
				&& (strcmp(namelist[i]->d_name,"..")!= 0) // It is not parent directory
				&& (namelist[i]->d_name[0] != '.' || all)// It is not hidden directory or need to show all directory
			) {
				stackCount++;
				strcpy(dirStack[stackCount],newp);
				//printf("add to dirStack%s\n",newp);
			}
	}
	while (n--) free(namelist[n]);
	free(namelist);
	//printf("listDir:%s end\n",path);
}

void dealPath(char * path){
	struct stat st;
	stat(path,&st);
	if (S_ISDIR(st.st_mode))
		strcpy(dirStack[++stackCount],path);
	else
		printEntry(path);
	while (stackCount){
		listDir(dirStack[stackCount--]);
	}
}

void init(int argc, char **argv){
	char ch;
	while ((ch = getopt(argc,argv,"arm:h:l:"))!=-1){
		switch(ch){
			case 'a':
				all = 1;
			break;
			case 'r':
				recursive = 1;
			break;
			case 'm':
				sscanf(optarg,"%d",&modify);
			break;
			case 'l':
				sscanf(optarg,"%d",&low);
			break;
			case 'h':
				sscanf(optarg,"%d",&high);
			break;
		}
	}
}

int main(int argc, char **argv) {
	init(argc,argv);
	if (optind < argc){
		do{
			dealPath(argv[optind]);
		}while (++optind < argc);
	} else {
		dealPath(".");
	}
	return 0;
}
