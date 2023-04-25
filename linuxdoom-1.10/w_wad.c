/**
 * MOD
 * this file took a lot of refactoring, some of it is not strightfoward to explain
 * anyway, the main philosophy was to use the hardcoded wad compressed with brotli
 * so it fits in the flash memory, thent extrat it to the sdram and have all data
 * permanently loaded, this is the reaso why neither Z_Free nor Z_ChangeTag should
 * ever be applied to the loaded wad data
 */
// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// $Log:$
//
// DESCRIPTION:
//	Handles WAD file header, directory, lump I/O.
//
//-----------------------------------------------------------------------------

static const char
	rcsid[] = "$Id: w_wad.c,v 1.5 1997/02/03 16:47:57 b1 Exp $";

#ifdef NORMALUNIX
#include <ctype.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <fcntl.h>
#include <sys/stat.h>
//#include <alloca.h>
#define O_BINARY 0
#endif

#include "doomtype.h"
#include "m_swap.h"
#include "i_system.h"
#include "z_zone.h"

#ifdef __GNUG__
#pragma implementation "w_wad.h"
#endif
#include "w_wad.h"

//
// GLOBALS
//

// Location of each lump on disk.
lumpinfo_t *lumpinfo;
int numlumps;

void **lumpcache;

#define strcmpi strcasecmp

/*
void strupr(char *s)
{
	while (*s)
	{
		*s = toupper(*s);
		s++;
	}
}

int filelength(int handle)
{
	struct stat fileinfo;

	if (fstat(handle, &fileinfo) == -1)
		I_Error("Error fstating");

	return fileinfo.st_size;
}
*/

void ExtractFileBase(char *path,
					 char *dest)
{
	char *src;
	int length;

	src = path + strlen(path) - 1;

	// back up until a \ or the start
	while (src != path && *(src - 1) != '\\' && *(src - 1) != '/')
	{
		src--;
	}

	// copy up to eight characters
	memset(dest, 0, 8);
	length = 0;

	while (*src && *src != '.')
	{
		if (++length == 9)
			I_Error("Filename base of %s >8 chars", path);

		*dest++ = toupper((int)*src++);
	}
}

//
// LUMP BASED ROUTINES.
//

//
// W_AddFile
// All files are optional, but at least one file must be
//  found (PWAD, if all required lumps are present).
// Files with a .wad extension are wadlink files
//  with multiple lumps.
// Other files are single lumps with the base filename
//  for the lump name.
//
// If filename starts with a tilde, the file is handled
//  specially to allow map reloads.
// But: the reload feature is a fragile hack...

int reloadlump;
char *reloadname;
#ifdef USE_WAD_FILE 
unsigned char waddata_comp[25 * 1024 * 1024];
#else
#include "waddata_comp.h"
#endif

unsigned char* waddata;

#include <brotli/decode.h>
#define CHUNK_SIZE 4096

static void* owner;

void* BrotliCustomAllocFunc(void* opaque, size_t size) {

	//printf("--------------- Allocating %u bytes\n", size);
	return Z_Malloc(size, PU_PURGELEVEL, &owner);
}

void BrotliCustomFreeFunc(void* opaque, void* address) {
  
	if (address == NULL) return;
	//printf("--------------- Deallocating addr %u\n", address);
	Z_Free(address);
}

static void DecompressWad() {

	// Inicializar el estado de descompresión
	BrotliDecoderState *state = BrotliDecoderCreateInstance(BrotliCustomAllocFunc, BrotliCustomFreeFunc, NULL); 
	//BrotliDecoderState *state = BrotliDecoderCreateInstance(NULL, NULL, NULL); 
	if (!state)
	{
		I_Error("No se pudo crear el estado de descompresión de Brotli");
	}
	size_t available_out = 4 * 1024 * 1024 + 5 * 1024;

	// Crear un búfer de entrada y uno de salida
	uint8_t *in = waddata_comp;
	uint8_t *out = waddata = Z_Malloc(available_out, PU_STATIC, 0);

	// Descomprimir los datos del archivo de entrada y escribirlos en el archivo de salida
	size_t available_in = sizeof(waddata_comp);
	//available_in = fread(in, 1, CHUNK_SIZE, input);
	//printf("Leidos %u bytes\n", available_in);
	uint8_t *next_in = in;
	uint8_t *next_out = out;
	size_t total_out = 0;
	while (1)
	{
		BrotliDecoderResult res;
		res = BrotliDecoderDecompressStream(state, &available_in, &next_in, &available_out, &next_out, &total_out);
		if (res == BROTLI_DECODER_RESULT_ERROR)
		{
			I_Error("Error al descomprimir el archivo");
		}
		else if (res == BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT)
		{
			
			I_Error("Error BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT");
			size_t out_size = CHUNK_SIZE - available_out;
			//fwrite(out, 1, out_size, output);
			next_out = out;
			available_out = CHUNK_SIZE;
		}
		else if (res == BROTLI_DECODER_RESULT_SUCCESS)
		{
			
			//size_t out_size = CHUNK_SIZE - available_out;
			printf("decompress_wad: Success! %u bytes\n", total_out);
			//fwrite(out, 1, out_size, output);
			
			break;
		}
		else if (res == BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT)
		{
			I_Error("Error BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT");
			//available_in = fread(in, 1, CHUNK_SIZE, input);
			//next_in = in;
		}
	}

	// Liberar la memoria utilizada por el estado de descompresión
	BrotliDecoderDestroyInstance(state);

}

void W_AddFile(char *filename)
{
	wadinfo_t header;
	lumpinfo_t *lump_p;
	unsigned i;
	int handle;
	int length;
	int startlump;
	filelump_t *fileinfo;
	filelump_t singleinfo;
	int storehandle;

	// open the file and add to directory

	// handle reload indicator.
	if (filename[0] == '~')
	{
		filename++;
		reloadname = filename;
		reloadlump = numlumps;
	}

#ifdef USE_WAD_FILE
	if ((handle = open(filename, O_RDONLY | O_BINARY)) == -1)
	{
		printf(" couldn't open %s\n", filename);
		return;
	}

	fread(waddata_comp, 1, sizeof(waddata_comp), handle);
#endif
	DecompressWad();
	printf(" adding %s\n", filename);
	startlump = numlumps;

	if (strcmpi(filename + strlen(filename) - 3, "wad"))
	{
		/*
		// single lump file
		fileinfo = &singleinfo;
		singleinfo.filepos = 0;
		singleinfo.size = LONG(filelength(handle));
		ExtractFileBase(filename, singleinfo.name);
		numlumps++;
		*/
	}
	else
	{
		// WAD file
		//read(handle, &header, sizeof(header));
		header = *((wadinfo_t*) waddata);
		if (strncmp(header.identification, "IWAD", 4))
		{
			// Homebrew levels?
			if (strncmp(header.identification, "PWAD", 4))
			{
				I_Error("Wad file %s doesn't have IWAD "
						"or PWAD id\n",
						filename);
			}

			// ???modifiedgame = true;
		}
		header.numlumps = LONG(header.numlumps);
		header.infotableofs = LONG(header.infotableofs);
		length = header.numlumps * sizeof(filelump_t);
		//fileinfo = alloca(length);
		//lseek(handle, header.infotableofs, SEEK_SET);
		//read(handle, fileinfo, length);
		lumpinfo = fileinfo = &(waddata[header.infotableofs]);
		numlumps += header.numlumps;
	}
#if 0
	// Fill in lumpinfo
	lumpinfo = realloc(lumpinfo, numlumps * sizeof(lumpinfo_t));

	if (!lumpinfo)
		I_Error("Couldn't realloc lumpinfo");

	lump_p = &lumpinfo[startlump];

	storehandle = reloadname ? -1 : handle;

	for (i = startlump; i < numlumps; i++, lump_p++, fileinfo++)
	{
		lump_p->handle = storehandle;
		lump_p->position = LONG(fileinfo->filepos);
		lump_p->size = LONG(fileinfo->size);
		strncpy(lump_p->name, fileinfo->name, 8);
	}

	if (reloadname)
		close(handle);
#endif
}

//
// W_Reload
// Flushes any of the reloadable lumps in memory
//  and reloads the directory.
//
void W_Reload(void)
{
#ifdef USE_PC_PORT
	wadinfo_t header;
	int lumpcount;
	lumpinfo_t *lump_p;
	unsigned i;
	int handle;
	int length;
	filelump_t *fileinfo;

	if (!reloadname)
		return;//this function will always return since reloadname is almos harcoded to NULL

	if ((handle = open(reloadname, O_RDONLY | O_BINARY)) == -1)
		I_Error("W_Reload: couldn't open %s", reloadname);

	read(handle, &header, sizeof(header));
	lumpcount = LONG(header.numlumps);
	header.infotableofs = LONG(header.infotableofs);
	length = lumpcount * sizeof(filelump_t);
	fileinfo = alloca(length);
	lseek(handle, header.infotableofs, SEEK_SET);
	read(handle, fileinfo, length);

	// Fill in lumpinfo
	lump_p = &lumpinfo[reloadlump];

	for (i = reloadlump;
		 i < reloadlump + lumpcount;
		 i++, lump_p++, fileinfo++)
	{
		//if (lumpcache[i])
		//	Z_Free(lumpcache[i]);

		lump_p->position = LONG(fileinfo->filepos);
		lump_p->size = LONG(fileinfo->size);
	}

	close(handle);
#endif
}

//
// W_InitMultipleFiles
// Pass a null terminated list of files to use.
// All files are optional, but at least one file
//  must be found.
// Files with a .wad extension are idlink files
//  with multiple lumps.
// Other files are single lumps with the base filename
//  for the lump name.
// Lump names can appear multiple times.
// The name searcher looks backwards, so a later file
//  does override all earlier ones.
//
void W_InitMultipleFiles(char **filenames)
{
	int size;

	// open all the files, load headers, and count lumps
	numlumps = 0;

	// will be realloced as lumps are added
	//lumpinfo = malloc(1);

	for (; *filenames; filenames++)
		W_AddFile(*filenames);

	if (!numlumps)
		I_Error("W_InitFiles: no files found");

	// set up caching
	size = numlumps * sizeof(*lumpcache);
	lumpcache = malloc(size);

	if (!lumpcache)
		I_Error("Couldn't allocate lumpcache");

	memset(lumpcache, 0, size);

	for (int i = 0; i < numlumps; i++) {

		lumpinfo_t *lump_p;
		lump_p = lumpinfo + i;
		lumpcache[i] = &waddata[lump_p->position];

	}
}

//
// W_InitFile
// Just initialize from a single file.
//
void W_InitFile(char *filename)
{
	char *names[2];

	names[0] = filename;
	names[1] = NULL;
	W_InitMultipleFiles(names);
}

//
// W_NumLumps
//
int W_NumLumps(void)
{
	return numlumps;
}

//
// W_CheckNumForName
// Returns -1 if name not found.
//

int W_CheckNumForName(char *name)
{
	union
	{
		char s[9];
		int x[2];

	} name8;

	int v1;
	int v2;
	lumpinfo_t *lump_p;

	// make the name into two integers for easy compares
	strncpy(name8.s, name, 8);

	// in case the name was a fill 8 chars
	name8.s[8] = 0;

	// case insensitive
	strupr(name8.s);

	v1 = name8.x[0];
	v2 = name8.x[1];

	// scan backwards so patch lump files take precedence
	lump_p = lumpinfo + numlumps;

	while (lump_p-- != lumpinfo)
	{
		if (*(int *)lump_p->name == v1 && *(int *)&lump_p->name[4] == v2)
		{
			return lump_p - lumpinfo;
		}
	}

	// TFB. Not found.
	return -1;
}

//
// W_GetNumForName
// Calls W_CheckNumForName, but bombs out if not found.
//
int W_GetNumForName(char *name)
{
	int i;

	i = W_CheckNumForName(name);

	if (i == -1)
		I_Error("W_GetNumForName: %s not found!", name);

	return i;
}

//
// W_LumpLength
// Returns the buffer size needed to load the given lump.
//
int W_LumpLength(int lump)
{
	if (lump >= numlumps)
		I_Error("W_LumpLength: %i >= numlumps", lump);

	return lumpinfo[lump].size;
}

//
// W_ReadLump
// Loads the lump into the given buffer,
//  which must be >= W_LumpLength().
//
void W_ReadLump(int lump,
				void *dest)
{
	int c;
	lumpinfo_t *l;
	int handle;

	if (lump >= numlumps)
		I_Error("W_ReadLump: %i >= numlumps", lump);

	l = lumpinfo + lump;

	memcpy(dest, &waddata[l->position], l->size);
	return;
/*
	// ??? I_BeginRead ();

	if (l->handle == -1)
	{
		// reloadable file, so use open / read / close
		if ((handle = open(reloadname, O_RDONLY | O_BINARY)) == -1)
			I_Error("W_ReadLump: couldn't open %s", reloadname);
	}
	else
		handle = l->handle;

	lseek(handle, l->position, SEEK_SET);
	c = read(handle, dest, l->size);

	if (c < l->size)
		I_Error("W_ReadLump: only read %i of %i on lump %i",
				c, l->size, lump);

	if (l->handle == -1)
		close(handle);

	// ??? I_EndRead ();
*/
}

//
// W_CacheLumpNum
//
void *
W_CacheLumpNum(int lump,
			   int tag)
{
	byte *ptr;

	if ((unsigned)lump >= numlumps)
		I_Error("W_CacheLumpNum: %i >= numlumps", lump);

	return lumpcache[lump];
	/*
	if (!lumpcache[lump])
	{
		// read the lump in

		// printf ("cache miss on lump %i\n",lump);
		ptr = Z_Malloc(W_LumpLength(lump), tag, &lumpcache[lump]);
		W_ReadLump(lump, lumpcache[lump]);
	}
	else
	{
		// printf ("cache hit on lump %i\n",lump);
		Z_ChangeTag(lumpcache[lump], tag);
	}

	return lumpcache[lump];
	*/
}

//
// W_CacheLumpName
//
void *
W_CacheLumpName(char *name,
				int tag)
{
	return W_CacheLumpNum(W_GetNumForName(name), tag);
}

//
// W_Profile
//
int info[2500][10];
int profilecount;

void W_Profile(void)
{
	int i;
	memblock_t *block;
	void *ptr;
	char ch;
	FILE *f;
	int j;
	char name[9];

	for (i = 0; i < numlumps; i++)
	{
		ptr = lumpcache[i];
		if (!ptr)
		{
			ch = ' ';
			continue;
		}
		else
		{
			block = (memblock_t *)((byte *)ptr - sizeof(memblock_t));
			if (block->tag < PU_PURGELEVEL)
				ch = 'S';
			else
				ch = 'P';
		}
		info[i][profilecount] = ch;
	}
	profilecount++;

	f = fopen("waddump.txt", "w");
	name[8] = 0;

	for (i = 0; i < numlumps; i++)
	{
		memcpy(name, lumpinfo[i].name, 8);

		for (j = 0; j < 8; j++)
			if (!name[j])
				break;

		for (; j < 8; j++)
			name[j] = ' ';

		fprintf(f, "%s ", name);

		for (j = 0; j < profilecount; j++)
			fprintf(f, "    %c", info[i][j]);

		fprintf(f, "\n");
	}
	fclose(f);
}
