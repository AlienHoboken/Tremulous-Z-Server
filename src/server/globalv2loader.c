#include <stdio.h>
#include <stdlib.h>

int xglobal_mask(unsigned char in,unsigned char * out);
int xglobal_load_c(char * path);
int xglobal_parse_file_c(char * data,unsigned long size);
int xglobal_insert_c(char * data);
int xglobal_match(unsigned char * ip1,unsigned char * ip2,unsigned char * mask);
unsigned char xglobal_flags(char * ip);
void xglobal_free(void);

typedef struct
{
	unsigned char ip[4];
	unsigned char mask[4];
	unsigned char flags;
} xglobal;

xglobal ** globals = NULL;
unsigned long global_count = 0;

char * global_error = NULL;

void xglobal_seterror(char *es)
{
	global_error = es;
}

char * xglobal_geterror(void)
{
	char * errorm = global_error;
	global_error = NULL;
	return errorm;
}

int xglobal_mask(unsigned char in,unsigned char * out)
{
	if(out == NULL || in > 32){xglobal_seterror("Invalid arguments\n"); return 1;}
	out[0] = out[1] = out[2] = out[3] = 0;
	while(in > 0)
	{
		out[(in - 1) / 8] |= (1 << ((in - 1) % 8));
		in--;
	}
	return 0;
}

int xglobal_load_c(char * path)
{
	FILE *fp;
	if(path == NULL){xglobal_seterror("Null file path.\n");return 1;}
	fp = fopen(path,"rb");
	if(fp == NULL){xglobal_seterror("Error opening file for reading.\n");return 1;}
	{
		char * buffer;
		unsigned long size;
		fseek(fp,0,SEEK_END);
		size = ftell(fp);
		rewind(fp);
		buffer = malloc(sizeof(char) * size);
		if(buffer == NULL){xglobal_seterror("Malloc failure\n");return 1;}
		if(fread(buffer,1,size,fp) != size){xglobal_seterror("fread couldn't read entire file\n");return 1;}
		fclose(fp);
		xglobal_free();
		if(xglobal_parse_file_c(buffer,size) != 0){return 1;}
		free(buffer);
	}
	return 0;
}

#define GLOBAL_SIZE 6
#define HEADER_SIZE 9
int xglobal_parse_file_c(char * data,unsigned long size)
{
	unsigned long i;
	if(data == NULL){xglobal_seterror("Parse argument is null\n");return 1;}
	if(size < (GLOBAL_SIZE + HEADER_SIZE) || ((size - HEADER_SIZE) % GLOBAL_SIZE) != 0){xglobal_seterror("File size is invalid. Possibly corrupt.\n");return 1;}
	if(	data[0] != 0 || 
		data[1] != 'G' || 
		data[2] != 'l' || 
		data[3] != 'o' || 
		data[4] != 'b' || 
		data[5] != 'a' || 
		data[6] != 'l' || 
		data[7] != 0x02 || 
		data[8] != 0x04){xglobal_seterror("Header is invalid\n");return 1;}
	for(i = 0;i < (size - HEADER_SIZE) / GLOBAL_SIZE;i++)
	{
		if(xglobal_insert_c(data + (i * GLOBAL_SIZE) + HEADER_SIZE) != 0){return 1;}
	}
	return 0;
}

int xglobal_insert_c(char * data)
{
	if(data == NULL){xglobal_seterror("Invalid argument(insert)");return 1;}
	globals = realloc(globals,sizeof(xglobal *) * (global_count + 1));
	if(globals == NULL){xglobal_seterror("Realloc failed\n");return 1;}
	globals[global_count] = malloc(sizeof(xglobal));
	if(globals[global_count] == NULL){xglobal_seterror("Malloc globals[] failed\n");free(globals);return 1;}
	if(xglobal_mask(data[4],globals[global_count]->mask) != 0){free(globals[global_count]);free(globals);return 1;}
	globals[global_count]->ip[0] = data[0];
	globals[global_count]->ip[1] = data[1];
	globals[global_count]->ip[2] = data[2];
	globals[global_count]->ip[3] = data[3];
	globals[global_count]->flags = data[5];
	global_count++;
	return 0;
}

int xglobal_match(unsigned char * ip1,unsigned char * ip2,unsigned char * mask)
{
	if(ip1 == NULL || ip2 == NULL || mask == NULL){return 1;}
	if(	((ip1[0] & mask[0]) == (ip2[0] & mask[0])) && 
		((ip1[1] & mask[1]) == (ip2[1] & mask[1])) &&
		((ip1[2] & mask[2]) == (ip2[2] & mask[2])) &&
		((ip1[3] & mask[3]) == (ip2[3] & mask[3]))  )
	{
		return 0;
	}
	return 1;
}

#define XGLOBAL_CLEAR (1 << 5)

unsigned char xglobal_flags(char * ip)
{
	if(ip == NULL){xglobal_seterror("Null argument for xglobal_flags\n");return 0;}
	unsigned long start = 0;
	unsigned char value = 1;
	unsigned long i;
	long j;
	char hold;
	int k = 0;
	unsigned char ip1[4];
	for(i = 0;;i++)
	{
		if((ip[i] < '0' || ip[i] > '9' || ip[i] == 0) && value == 1)
		{
			if(ip[i] == 0 && k != 3){return 0;}
			hold = ip[i];
			ip[i] = 0;
			j = atol(ip + start);
			ip[i] = hold;
			if(j < 0 || j > 255)
			{
				return 0;
			}
			ip1[k] = j;
			k++;
			if(k > 3){break;}
			value = 0;
		}
		else if(value == 0)
		{
			start = i;
			value = 1;
		}
	}
	if(k != 4){return 0;}
	value = 0;
	for(i = 0;i < global_count;i++)
	{
		if(xglobal_match(ip1,globals[i]->ip,globals[i]->mask) == 0)
		{
			if(globals[i]->flags & XGLOBAL_CLEAR){return globals[i]->flags;}
			value |= globals[i]->flags;
		}
	}
	return value;
}

void xglobal_free(void)
{
	unsigned long i;
	for(i = 0;i < global_count;i++)
	{
		if(globals[i] != NULL){free(globals[i]);}
		globals[i] = NULL;
	}
	global_count = 0;
	if(globals != NULL){free(globals);}
	globals = NULL;
	return;
}
