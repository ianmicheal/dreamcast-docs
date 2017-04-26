/********************************************************************
 *  Shinobi Library Sample
 *  Copyright (c) 1998 SEGA
 *
 *  Library : Ninja
 *  Module  : Debug print
 *  File    : dbgcons.c
 *  Date    : 1999-05-27
 *  Version : 1.00
 *  Note    : This file provides a simple console interface with
 *            njDebugPrint().
 ********************************************************************/

#include <shinobi.h>
#include "dbgcons.h"
#include <stdarg.h>

/*-------------------------------------------------------------------------*/
typedef struct {
	char* buf;              /* �e�L�X�g�o�b�t�@�A�h���X                    */
	                        /* Text Buffer Address                         */
	Sint32 cx, cy;          /* �J�[�\���ʒu                                */
	                        /* Cursor Position                             */
	Sint32 x, y, w, h;      /* �R���\�[���̍��W(x,y)�ƃT�C�Y(w,h)          */
	                        /* Console position(x,y) and size(w,h)         */
	Uint32 count;           /* �T�[�o�[���Ă΂�邽�тɃJ�E���g�A�b�v      */
	                        /* Count up when server function is called     */
} CONSOLECTX;

static CONSOLECTX ConsoleCtx;
static CONSOLECTX* cn;

static void ResetCursorBlink(void);

/*-------------------------------------------------------------------------*/
/*===============================================================*/
/* �R���\�[���̏�����                                            */
/* Initialize console                                            */
/*===============================================================*/

void ConInit(Sint32 x, Sint32 y, Sint32 w, Sint32 h, void* buf)
{
	cn = &ConsoleCtx;
	cn->buf = (char*)buf;
	cn->cx = 0;
	cn->cy = 0;
	cn->x = x;
	cn->y = y;
	cn->w = w;
	cn->h = h;
	cn->count = 0;

	memset(buf, ' ', w * h);
}

/*-------------------------------------------------------------------------*/
/*===============================================================*/
/* �R���\�[���̏I��                                              */
/* Finalize console                                              */
/*===============================================================*/
void ConExit(void)
{
	memset(cn, 0, sizeof(*cn));
}

/*-------------------------------------------------------------------------*/
/*===============================================================*/
/* �J�[�\���ʒu�ւP�����\��                                      */
/* Put an character into cursor position                         */
/*===============================================================*/
void ConPutc(char ch)
{
	cn->buf[cn->cx + cn->cy * cn->w] = ch;
}

/*-------------------------------------------------------------------------*/
/*===============================================================*/
/* �P�s�X�N���[��                                                */
/* Scrolle one line                                              */
/*===============================================================*/
void ConRoll(void)
{
	Sint32 i, j;
	char* d;
	char* s;

	d = cn->buf;
	s = d + cn->w;
	for (i = 0; i < cn->h - 1; i++) {
		for (j = 0; j < cn->w; j++) {
			*d++ = *s++;
		}
	}

	d =cn->buf + cn->w * (cn->h - 1);
	for (j = 0; j < cn->w; j++) {
		*d++ = ' ';
	}
}

/*-------------------------------------------------------------------------*/
/*===============================================================*/
/* �J�[�\���ʒu�֕�����\��(�J�[�\���ʒu�X�V)                    */
/* Put an string into cusor position(w/Cursor position update)   */
/*===============================================================*/

void ConPrint(const char* str)
{
	char ch;

	while (ch = *str++) {
		if (ch == '\n') {
			cn->cy++;
			cn->cx = 0;
		}
		if (ch == 0x08) {	/* BS */
			if (cn->cx > 0) cn->cx--;
			ConPutc(' ');
		}
		if (ch == '\t') {	/* TAB */
			cn->cx += 4;
			cn->cx &= ~3;
			ConPutc(' ');
		}

		if (cn->cx >= cn->w) {
			cn->cx = 0;
			cn->cy++;
		}
		if (cn->cy >= cn->h) {
			cn->cy--;
			cn->cx = 0;
			ConRoll();
		}

		if (ch == '\n') continue;
		if (ch == '\t') continue;
		if (ch == 0x08) continue;
		if (ch == 0x7f) continue;

		ConPutc(ch);
		cn->cx++;
	}

	cn->count = 1 << 4;
}

/*-------------------------------------------------------------------------*/
/*===============================================================*/
/* �J�[�\���ʒu��printf()����������\��(�J�[�\���ʒu�X�V)        */
/* Put an printf() format string into cusor position             */
/* (w/Cursor position update)                                    */
/*===============================================================*/

void ConPrintf(const char* fmt, ...)
{
	static char buffer[512];
	va_list va;

	va_start(va, fmt);
	vsprintf(buffer, fmt, va);
	va_end(va);

	ConPrint(buffer);
}

/*-------------------------------------------------------------------------*/
/*===============================================================*/
/* �J�[�\���ʒu�ݒ�                                              */
/* Set cursor position                                           */
/*===============================================================*/

void ConLocate(Sint32 x, Sint32 y)
{
	if (x < 0) x = 0;
	if (y < 0) y = 0;
	if (x >= cn->w) x = cn->w;
	if (y >= cn->h) y = cn->h;

	cn->cx = x;
	cn->cy = y;

	ResetCursorBlink();
}

/*-------------------------------------------------------------------------*/
/*===============================================================*/
/* �J�[�\���ʒu�擾                                              */
/* Get cursor position                                           */
/*===============================================================*/

void ConGetPos(Sint32* x, Sint32* y)
{
	*x = cn->cx;
	*y = cn->cy;
}

/*-------------------------------------------------------------------------*/
/*===============================================================*/
/* �T�[�o�[�֐� �e�L�X�g�o�b�t�@�̓��e��`��                     */
/* Server function:Render the text buffer                        */
/*===============================================================*/

void ConMain(void)
{
	Sint32 i, j, x, y;
	char* p;
	char buf[2];

	buf[1] = '\0';

	p = cn->buf;
	for (i = 0; i < cn->h; i++) {
		for (j = 0; j < cn->w; j++, p++) {
			x = cn->x + j;
			y = cn->y + i;
			buf[0] = *p;
			njPrintC(NJM_LOCATION(x, y), buf);
		}
	}

	cn->count++;
}


/*-------------------------------------------------------------------------*/
/*===============================================================*/
/* �e�L�X�g�o�b�t�@�̃N���A�ƃz�[���ʒu�ւ̈ړ�                  */
/* Clear the text buffer and set cursor position to (0,0)        */
/*===============================================================*/

void ConCls(void)
{
	memset(cn->buf, ' ', cn->w * cn->h);
	cn->cx = 0;
	cn->cy = 0;
}

/*-------------------------------------------------------------------------*/
/*===============================================================*/
/* �J�[�\���̕\��                                                */
/* Put cursor                                                    */
/*===============================================================*/

void ConDispCursor(void)
{
	if ((cn->count >> 4) & 1) {
		njPrintC(NJM_LOCATION(cn->x + cn->cx, cn->y + cn->cy), "_");
	}
}

/*-------------------------------------------------------------------------*/
/*===============================================================*/
/* �J�[�\���̓_�ł�������                                        */
/* Initialize the blink count of the cursor                      */
/*===============================================================*/

static void ResetCursorBlink(void)
{
	cn->count = 1 << 4;
}

