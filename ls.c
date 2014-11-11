#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include <getopt.h>
#include <stdlib.h>
#include <libgen.h>
#include <stdbool.h>
#define MAXLEN 2048

/* Global Variables*/

bool showHiddenFile = false;
bool recursiveShow = false;

int sizeLowerBound = -1;
int sizeUpperBound = -1;
int lastModifyTime = -1;

char dirStack[256][MAXLEN]={{0}}; // Support 256 levels of nested directory
int stackDepth = 0; // current depth of directory

void printEntry(char *path) {
	struct stat st;
	time_t now;
	time(&now);
	if (stat(path,&st) < 0){
		printf("\e[31mOpen file\"%s\":%s(ERROR %d)\e[0m\n", path, strerror(errno), errno);
		return;
	}

	if (	(showHiddenFile == false && (basename(path)[0] == '.')) // it is hidden and no need to show it.
			|| (sizeLowerBound != -1 && st.st_size < sizeLowerBound)  // too small
			|| (sizeUpperBound != -1 && st.st_size > sizeUpperBound) // too large 
			|| (lastModifyTime != -1 && ((now - st.st_ctime) / (60*60*24) > lastModifyTime)) // too old
		) return;

	if ((st.st_mode & S_IXUSR) == S_IXUSR) {
		printf("\e[32m\e[1m"); // special color for excutable
	}
	if (S_ISDIR(st.st_mode)) {
		printf("\e[34m\e[1m"); // special color for directory
	}

	printf("%s\e[0m\n",basename(path));
}

void listDir(char *dir){
	struct dirent ** namelist;
	int i,n;
	char nextPath[MAXLEN], path[MAXLEN];
	strcpy(path, dir);
	n = scandir(path, &namelist, 0, alphasort);
	if (n < 0){
		printf("\e[31mOpen directory\"%s\":%s(ERROR %d)\e[0m\n",path,strerror(errno),errno);
		return;
	}
	if (recursiveShow == true) {
		printf("\n%s\n",dir);
	}
	for (i = 0; i < n; i++){
		memset(nextPath, 0, sizeof(char) * MAXLEN); // reset
		strcpy(nextPath, path);
		nextPath[strlen(nextPath)]='/';
		strcat(nextPath,namelist[i]->d_name);
		printEntry(nextPath);
		
		if (	recursiveShow == true // Show recursively
				&& (namelist[i]->d_type == DT_DIR) // It is a directory
				&& (strcmp(namelist[i]->d_name,".") != 0) // It is not current directory
				&& (strcmp(namelist[i]->d_name,"..") != 0) // It is not parent directory
				&& (namelist[i]->d_name[0] != '.' || showHiddenFile)// It is not hidden directory or need to show all directory
			) {
				stackDepth++;
				strcpy(dirStack[stackDepth],nextPath);
			}
	}
	// clean up
	while (n--) free(namelist[n]);
	free(namelist);
}

void explorePath(char * path){
	struct stat st;
	stat(path,&st);
	if (S_ISDIR(st.st_mode)) { // it is a directory
		strcpy(dirStack[++stackDepth], path);
	} else {
		printEntry(path); // single file
	}
	while (stackDepth) {
		listDir(dirStack[stackDepth]);
		--stackDepth;
	}
}

void init(int argc, char **argv) {
	char ch;
	while ((ch = getopt(argc, argv, "arm:h:l:")) != -1) {
		switch(ch){
			case 'a':
				showHiddenFile = true;
			break;
			case 'r':
				recursiveShow = true;
			break;
			case 'm':
				sscanf(optarg,"%d", &lastModifyTime);
			break;
			case 'l':
				sscanf(optarg,"%d", &sizeLowerBound);
			break;
			case 'h':
				sscanf(optarg,"%d", &sizeUpperBound);
			break;
		}
	}
}

int main(int argc, char **argv) {
	init(argc,argv);
	if (optind < argc){
		do {
			explorePath(argv[optind]);
		} while(++optind < argc);
	} else {
		explorePath(".");
	}
	return 0;
}
