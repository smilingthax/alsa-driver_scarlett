/*
 *  Utility to find soundcard dependencies from modules.config
 *  Copyright (c) by Anders Semb Hermansen <ahermans@vf.telia.no>,
 *		     Martin Dahl <dahlm@vf.telia.no>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include "snd-deps.h"


// Globals
dep *Cards;	// All cards
dep *Deps;	// All other modules 


int read_file(char *filename)
{
	char buffer[255];
	FILE *file;

	if((file=fopen(filename,"r"))==NULL)
		return -errno;

	while(!feof(file))
	{
		bzero(buffer,255);
		fscanf(file,"%[^\n]\n",buffer);
		if(buffer[0]=='|')
			add_dep(&buffer[1], Cards, TYPE_CARDS); // Card modules (skip |)
		else if(isalpha(buffer[0]))
			add_dep(buffer, Deps, TYPE_DEPS); // Other modules
	}
	return 0;
}

// Add a new dependency or soundcard to the list
void add_dep(char *line, dep *firstdep, short type)
{
	dep *new_dep;
	char *word=NULL;
	int numdeps=0;

	new_dep=alloc_mem_for_dep(firstdep, type);
	word=malloc(strlen(line)+1);
	if(word==NULL)
	{
		fprintf(stderr, "Not enough memory\n");
		exit(EXIT_FAILURE);
	}
	get_word(line, word);
	strcpy(new_dep->name, word); // Fill inn name of dependency

	new_dep->deps=malloc(1);
	if(new_dep->deps==NULL)
	{
		fprintf(stderr, "Not enough memory\n");
		exit(EXIT_FAILURE);
	}
	while(get_word(line, word))
	{
		new_dep->deps=realloc(new_dep->deps, sizeof(new_dep)*(numdeps+1));
		if(new_dep->deps==NULL)
		{
			fprintf(stderr, "Not enough memory\n");
			exit(EXIT_FAILURE);
		}
		new_dep->deps[numdeps]=find_dep(new_dep->name, word);
		if(new_dep->deps[numdeps])
			numdeps++;
	}
	new_dep->numdeps=numdeps;
	free(word);
	return;
}

dep *alloc_mem_for_dep(dep *firstdep, short type)
{
	if(!firstdep)
	{
		if(type==TYPE_CARDS)
			Cards=(dep *)malloc(sizeof(dep));
		else
			Deps=(dep *)malloc(sizeof(dep));
		firstdep=(type==TYPE_CARDS) ? Cards : Deps;
		if(firstdep==NULL)
		{
			fprintf(stderr, "Not enough memory\n");
			exit(EXIT_FAILURE);
		}
		firstdep->link=NULL;
		return firstdep;
	}
	else
	{
		while(firstdep->link)
			firstdep=firstdep->link;
		firstdep->link=(dep *)malloc(sizeof(dep));
		if(firstdep->link==NULL)
		{
			fprintf(stderr, "Not enough memory\n");
			exit(EXIT_FAILURE);
		}
		firstdep=firstdep->link;
		firstdep->link=NULL;
		return firstdep;
	}
}

// Put the first word in "line" in "word". Put the rest back in "line"
char *get_word(char *line, char *word)
{
	int i,j;
	char *full_line;

	if(strlen(line)==0)
		return NULL;

	full_line=malloc(strlen(line)+1);
	if(full_line==NULL)
	{
		fprintf(stderr, "Not enough memory\n");
		exit(EXIT_FAILURE);
	}
	strcpy(full_line, line);

	for(i=0;i<strlen(full_line);i++)
	{
		if(full_line[i]!=' ')
			word[i]=full_line[i];
		else
		{
			// We got the whole word
			word[i++]='\0';
			for(j=0;i<strlen(full_line);i++,j++)
				line[j]=full_line[i];
			line[j]='\0';
			free(full_line);
			return word;
		}
	}
	// This was the last word
	word[i]='\0';
	line[0]='\0';
	free(full_line);
	return word;
}

// Find the dependency named "depname"
dep *find_dep(char *parent, char *depname)
{
	dep *temp_dep;

	temp_dep=Deps;
	while(temp_dep)
	{
		if(!strcmp(depname, temp_dep->name))
			return temp_dep;
		temp_dep=temp_dep->link;
	}
#ifdef WARNINGS
	fprintf(stderr, "Warning: Unsatisfied dep for %s: %s\n", parent, depname);
#endif
	return NULL;
}

// Fill list[] with all deps for dependency
int make_list_of_deps_for_dep(dep *dependency, depname list[], int num)
{
	int i,j;
	int add;

	for(i=0;i<dependency->numdeps;i++)
		if(dependency->deps[i])
		{
			add=1;
			for(j=0;j<num;j++)
				if(!strcmp(list[j], dependency->deps[i]->name))
					add=0;
			if(add)
			{
				strcpy(list[num++], dependency->deps[i]->name);
				num=make_list_of_deps_for_dep(dependency->deps[i], list, num);
			}
		}
	return num;
}

// Free memory for all deps in Cards and Deps
void del_all_from_list(void)
{
	dep *temp=Cards;
	dep *next;

	while(temp)
	{
		next=temp->link;
		free(temp->deps);
		free(temp);
		temp=next;
	}
	temp=Deps;
	while(temp)
	{
		next=temp->link;
		free(temp->deps);
		free(temp);
		temp=next;
	}
	return;
}
