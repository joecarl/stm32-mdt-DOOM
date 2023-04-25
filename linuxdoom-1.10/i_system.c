/**
 * MOD
 * include sdram dirver
 * implement I_ZoneBase mod to allocate memory in the sdram
 * implement I_GetTime mod to use HAL_GetTick
 * implement MDT_Init to initialize the video driver at startup
 * implement I_WaitVBL mod to use HAL_Delay
 * implement I_Error mod to show error on screen
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
//
//-----------------------------------------------------------------------------

static const char
	rcsid[] = "$Id: m_bbox.c,v 1.1 1997/02/03 22:45:10 b1 Exp $";

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <stdarg.h>
#include <sys/time.h>
#include <unistd.h>


#include "doomdef.h"
#include "m_misc.h"
#include "i_video.h"
#include "i_sound.h"

#include "d_net.h"
#include "g_game.h"

#ifdef __GNUG__
#pragma implementation "i_system.h"
#endif
#include "i_system.h"

#ifndef USE_PC_PORT
#include "mdt/drivers/sdram_driver.h"
#endif

int mb_used = 7;

void I_Tactile(int on,
			   int off,
			   int total)
{
	// UNUSED.
	on = off = total = 0;
}

ticcmd_t emptycmd;
ticcmd_t *I_BaseTiccmd(void)
{
	return &emptycmd;
}

int I_GetHeapSize(void)
{
	return mb_used * 1024 * 1024;
}

byte *I_ZoneBase(int *size)
{
	*size = mb_used * 1024 * 1024;
	#ifdef USE_PC_PORT
	return (byte *)malloc(*size);
	#else
	MDT_SDRAM_Init();	
	byte* ptr = (byte*) MDT_SDRAM_malloc(*size);
	memset(ptr, 0, *size);
	return ptr;
	#endif
}

//
// I_GetTime
// returns time in 1/70th second tics
//
int I_GetTime(void)
{
#ifdef USE_PC_PORT
	struct timeval tp;
	struct timezone tzp;
	int newtics;
	static int basetime = 0;

	gettimeofday(&tp, &tzp);
	if (!basetime)
		basetime = tp.tv_sec;
	newtics = (tp.tv_sec - basetime) * TICRATE + tp.tv_usec * TICRATE / 1000000;
	return newtics;
#else
	return HAL_GetTick() * TICRATE / 1000;
#endif
}

#ifndef USE_PC_PORT
#include "mdt/graphics.h"
#include "mdt/text.h"
void MDT_Init() {
	
	MDT_GRAPHICS_InitTypeDef graphicsCfg = {
		.useHardwareAcceleration = true,
		.useSDRAM = false,
		.mainCtxHeight = SCREENHEIGHT,
		.mainCtxWidth = SCREENWIDTH,
		.videoDriver = VIDEO_DRIVER_VGA,
	};
	MDT_GRAPHICS_Init(&graphicsCfg);
	MDT_Clear(0x00);
	MDT_DrawText(
		"STM32 DOOM.\n"
		"Please wait while WAD extracts...\n",
		10, 10, 0b0111001
	);
	MDT_SwapBuffers();

}
#endif

//
// I_Init
//
void I_Init(void)
{
	I_InitSound();
	//  I_InitGraphics();
}

//
// I_Quit
//
void I_Quit(void)
{
	D_QuitNetGame();
	I_ShutdownSound();
	I_ShutdownMusic();
	M_SaveDefaults();
	I_ShutdownGraphics();
	exit(0);
}

void I_WaitVBL(int count)
{
#ifndef USE_PC_PORT
	HAL_Delay(count * (1000 / 70));
#else
#ifdef SGI
	sginap(1);
#else
#ifdef SUN
	sleep(0);
#else
	usleep(count * (1000000 / 70));
#endif
#endif
#endif
}

void I_BeginRead(void)
{
}

void I_EndRead(void)
{
}

byte *I_AllocLow(int length)
{
	byte *mem;

	mem = (byte *)malloc(length);
	memset(mem, 0, length);
	return mem;
}

//
// I_Error
//
extern boolean demorecording;

void I_Error(char *error, ...)
{
	char errmsg[128];
	va_list argptr;

	// Message first.
	va_start(argptr, error);
	vsprintf(errmsg, error, argptr);
	fprintf(stderr, "Error: ");
	vfprintf(stderr, error, argptr);
	fprintf(stderr, "\n");
	va_end(argptr);

	fflush(stderr);
#ifndef USE_PC_PORT
	MDT_Clear(0x00);
	MDT_DrawText("ERROR", 5, 5, 0b0111001);
	MDT_DrawText(errmsg, 5, 15, 0b0111001);
	MDT_SwapBuffers();
#endif
	// Shutdown. Here might be other errors.
	if (demorecording)
		G_CheckDemoStatus();

	D_QuitNetGame();
	I_ShutdownGraphics();

	exit(-1);
}
