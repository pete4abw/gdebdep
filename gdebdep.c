/* gdebdep.c
 * Get Debian Dependencies from deb package for non-Debiam systems
 * (c) Peter Hyman
 * October 2020
 * GPL
 * Free for all for any purpose
 * No warranties
 * Some code and inspiration from strtok man page
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>

char lineinput[2048];
struct Package {
	struct Package *nextNode;
	char *package;
	char *dllocation;
} *packageDependencies = NULL, *packageList = NULL;

struct Ftp {
	char ftpmirror[64];
	char ftpdebiandir[64];
	char ftppkglistdir[64];
	char ftppkgname[16];
	char ftparch[16];
} ftpvars;

/* Retrieve configuration variables
 * Must be set in gdebdep.conf
*/
int getconf() {
	FILE *conffile;
	char *conffilename="gdebdep.conf";
	char *ret, *value;

	conffile=fopen(conffilename,"r");
	while(fgets(lineinput, sizeof(lineinput), conffile)) {
		if (lineinput[0] == '#' || lineinput[0] == ' ' || lineinput[0] == '\0')
			continue;
		lineinput[strlen(lineinput)-1]='\0';
		ret=strtok(lineinput," =");
		if (ret == NULL)
			continue;
		value=strtok(NULL, " =");
		if (!strcasecmp(ret, "MIRROR"))
			strcpy(ftpvars.ftpmirror, value);
		else if (!strcasecmp(ret, "DEBIANDIR"))
			strcpy(ftpvars.ftpdebiandir, value);
		else if (!strcasecmp(ret, "PACKAGELISTDIR"))
			strcpy(ftpvars.ftppkglistdir, value);
		else if (!strcasecmp(ret, "PACKAGEFILENAME"))
			strcpy(ftpvars.ftppkgname, value);
		else if (!strcasecmp(ret, "ARCH"))
			strcpy(ftpvars.ftparch, value);
		else {
			fprintf(stderr, "Bad Parameter, %s, in gdebdep.conf file\n", ret);
			return 1;
		}

	}
	strcat(ftpvars.ftppkglistdir, ftpvars.ftparch);
	fclose(conffile);
	return 0;
}

/* Load all Packages into packageList structure */
void load_packages() {
	FILE *packages;
	struct Package *curNode = NULL, *nextNode = NULL, *lastNode = NULL;
	char *buffer;
	int count = 0;

	packages=fopen("Packages","r");
	if (!packages) {
		fprintf(stderr, "Error: %s packages file not found\n");
		exit(EXIT_FAILURE);
	}
	while (fgets(lineinput, sizeof(lineinput), packages)) {
		if (strncmp("Package:", lineinput, 8))
			continue;
		if (lineinput[8] == ' ')
			buffer = &lineinput[9];
		else
			buffer = &lineinput[8];
		buffer[strlen(buffer)-1] = '\0';
		if (packageList == NULL) {
			packageList = malloc(sizeof(struct Package));
			curNode = packageList;
		} else {
			lastNode = curNode;
			lastNode->nextNode = malloc(sizeof(struct Package));
			curNode = lastNode->nextNode;
		}
		curNode->package = malloc(strlen(buffer)+1);
		curNode->nextNode = NULL;
		strcpy(curNode->package, buffer);
		// Get download URL
		while (fgets(lineinput, sizeof(lineinput), packages)) {
			if (strncmp("Filename:", lineinput, 9))
				continue;
			if (lineinput[9] == ' ')
				buffer = &lineinput[10];
			else
				buffer = &lineinput[9];
			buffer[strlen(buffer)-1] = '\0';
			curNode->dllocation = malloc(strlen(buffer)+1);
			// error check
			strcpy(curNode->dllocation, buffer);
			break;
		}
		count++;
	}
	fclose(packages);
	fprintf(stdout, "%d elements loaded from %s\n", count, packages);
}

/* Search through Packages and return download locatrion */
char *returndllocation(char package[]) {
	struct Package *pcurNode = NULL, *pnextNode = NULL;
	char *dllocation = NULL;
	for (pcurNode = packageList; pcurNode != NULL; pcurNode = pcurNode->nextNode) {
		if (strcmp(package, pcurNode->package))
			continue;
		// Package Name matched
		// Get download URL
		dllocation = malloc(strlen(pcurNode->dllocation)+1);
		// error check
		strcpy(dllocation, pcurNode->dllocation);
		break;
	}
	return dllocation;
}

int main(int argc, char *argv[])
{
	char command[256];
	char packageroot[64];
	char *debianpackagename;
		
	char *ret, *value;
	FILE *control;
	struct stat filestat;

        if (argc != 2) {
        	fprintf(stderr, "Usage: %s Debian package file\n",
			argv[0]);
 		exit(EXIT_FAILURE);
	}

	/* check conf file */
	if (getconf()) {
		fprintf(stderr, "Configuration file %s not found\n");
		exit(EXIT_FAILURE);
	}


	/* check Debian deb file */
	if (stat(argv[1], &filestat) == -1) {
		fprintf(stderr, "Debian package file %s not found\n", argv[1]);
		exit(EXIT_FAILURE);
	}

	// save full name
	debianpackagename = malloc(strlen(argv[1])+1);
	strcpy(debianpackagename, argv[1]);

	// strip any leading /
	ret = strrchr(argv[1], '/');
	if (ret == NULL)
		ret=strtok(argv[1], "_");
	else
		ret=strtok(++ret, "_");

	if (ret == NULL) {
		fprintf(stderr, "Malformed package name %s\n", argv[1]);
		exit(EXIT_FAILURE);
	}

	strcpy(packageroot, ret);

	if (mkdir(packageroot, 0755) == -1) {
		if (errno != EEXIST) {
			fprintf(stderr, "Error creating directory %s\n", packageroot);
			exit(EXIT_FAILURE);
		}
	}

	// Build system command
	// to get and extract control file
	// wget mirror/debiandir/packagelistdir/packafilename
	// ar p filename control.tar.xz | tar -xJf - ./control
	// get Packages file if not here
	if (stat(ftpvars.ftppkgname, &filestat) == -1) {
		sprintf(command, "wget %s/%s/%s/%s", ftpvars.ftpmirror,ftpvars.ftpdebiandir,ftpvars.ftppkglistdir,ftpvars.ftppkgname);
		system(command);
	}
	// extract Packages
	sprintf(command, "xz -dk %s", ftpvars.ftppkgname);
	system(command);

	sprintf(command, "ar p %s control.tar.xz | tar -xJf - ./control", debianpackagename);
	system(command);

	/* Open Control File */	
	control=fopen("control", "r");
	if (!control) {
		fprintf(stderr, "Error: %s control file not found\n");
		exit(EXIT_FAILURE);
	}

	/* get all Dependencies */
	char *buffer, *str1;
	while (fgets(lineinput, sizeof(lineinput), control)) {
		if (strncmp("Depends:", lineinput, 8))
			continue;
		lineinput[strlen(lineinput)-1] = '\0';
		if (lineinput[8] == ' ')
			buffer = &lineinput[9];
		else
			buffer = &lineinput[8];
		break;
	}
	str1 = malloc(strlen(buffer)+1);
	strcpy(str1, buffer);

	fclose(control);

	/* Get URL for download of each dependency */
	// load entire Package list
	load_packages();

	/* sift through and add each depend to packagename structure */

	int i,j,k,dependLen,dependCount = 0;
	char *token,*subtoken,*str2,*saveptr1,*saveptr2;
	char *dependSeparator = ",";
	char *dependOrSeparator = "|";
	char *dependVersionSeparator = "(";
	struct Package *lastNode = NULL, *curNode = NULL;
	
	for ( ; ; str1 = NULL) {
		token = strtok_r(str1, dependSeparator, &saveptr1);
		if (token == NULL)
			break;
		/* if we get here, we have a dependency */
		/* now get any ORs */
		// leading space?
		if (isspace(*token))
			++token;
		for (str2 = token; ; str2 = NULL) {
			subtoken = strtok_r(str2, dependOrSeparator, &saveptr2);
			if (subtoken == NULL)
				break;
			/* we're done */
			/* strip any vertsion info  */
			// leading space?
			if (isspace(*subtoken))
				++subtoken;
			for (k = 0; subtoken[k] != '\0' ; k++) {
				if (subtoken[k] == *dependVersionSeparator)
					subtoken[k] = '\0';
			}
			// strip trailing space if any
			if (isspace(subtoken[strlen(subtoken)-1]))
				subtoken[strlen(subtoken)-1] = '\0';
			/* we have a dependency */
			dependCount++;

			// setup structure
			if (packageDependencies == NULL) {
				packageDependencies = malloc(sizeof(struct Package));
				curNode = packageDependencies;
			} else {
				lastNode = curNode;
				lastNode->nextNode = malloc(sizeof(struct Package));
				curNode = lastNode->nextNode;
			}
			curNode->package = malloc(strlen(subtoken)+1);
			// error check
			strcpy(curNode->package, subtoken);
			/* get download location */
			curNode->dllocation = returndllocation(subtoken);
			// set end of list
			curNode->nextNode = NULL;
			// fprintf(stdout," --> %s -- %s\n", curNode->package, curNode->dllocation);
		}
	}
	// now sort it
	// remove duplicates
	int compare;
	struct Package *nextNode, *saveptr;
	char *savePackage, *saveDllocation;
	for (curNode = packageDependencies; curNode->nextNode != NULL; curNode = curNode->nextNode) {
		for (nextNode = curNode->nextNode; nextNode != NULL; nextNode = nextNode->nextNode) {
			compare=strcmp(curNode->package, nextNode->package);
			if (compare > 0) {
				savePackage = nextNode->package;
				saveDllocation = nextNode->dllocation;
				nextNode->package = curNode->package;
				nextNode->dllocation = curNode->dllocation;
				curNode->package = savePackage;
				curNode->dllocation = saveDllocation;
			}
		}
	}

	for (curNode = packageDependencies; curNode->nextNode != NULL; curNode = curNode->nextNode) {
		for (nextNode = curNode->nextNode; nextNode != NULL; nextNode = nextNode->nextNode) {
			compare=strcmp(curNode->package, nextNode->package);
			// fprintf(stdout,"%s == %s\n", curNode->package, nextNode->package);
			if (compare == 0) {		// packages are the same
				saveptr = nextNode->nextNode;
				//if (nextNode->package) free(nextNode->package);
				//if (nextNode->dllocation) free(nextNode->dllocation);
				//free (nextNode);
				curNode->nextNode = saveptr;
			}
		}
	}


	FILE *lftp;
	if (chdir(packageroot) == -1) {
		fprintf(stderr, "Cannot chdir to %s\n", packageroot);
		exit(EXIT_FAILURE);
	}

	lftp = fopen("lftp","w");
	if (!lftp) {
		fprintf(stderr, "Cannot open lftp file");
		exit(EXIT_FAILURE);
	}
	fprintf(lftp,"open %s\n", ftpvars.ftpmirror);
	fprintf(lftp,"cd %s\n", ftpvars.ftpdebiandir);

	for (curNode = packageDependencies; curNode != NULL; curNode = curNode->nextNode) {
		// if no dllocation found, continue
		if (curNode->dllocation == NULL) {
			fprintf(stderr, "%-25s !! not found in Packages\n", curNode->package);
			continue;
		}
		ret=strrchr(curNode->dllocation, '/');
		// check if already downloaded
		if (stat(++ret, &filestat) == -1) {
			fprintf(stdout,"%-25s -- %s\n", curNode->package, curNode->dllocation);
			// write to lfpt file
			if (curNode->dllocation)
				fprintf(lftp, "get %s\n", curNode->dllocation);
		}
	}
	fclose(lftp);
	chdir("..");

	exit(EXIT_SUCCESS);
}
