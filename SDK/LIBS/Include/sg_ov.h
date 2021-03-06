/*
 *  Overlay Library
 *  Copyright (c) 1998 SEGA
 *  
 *      Overlay
 *
 *  Module  : Library Header File
 *  File    : sg_ov.h
 *  Create  : 1998-02-20
 *  Modify  : 1998-05-12
 *  Version : 0.41
 *  Note    :
 */
#ifndef _SG_OV_H_
#define _SG_OV_H_

#include	<sg_xpt.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*	Version No.	*/
#define	OVD_VER					"1.00"

/*  ライブラリのバージョ�"(Ver.1.00)		*/
#define	OVD_VERSION_NO			(0x01)		/*  バージョ�""ﾔ号				*/
#define	OVD_REVISION_NO			(0x00)		/*  リビジョ�""ﾔ号				*/

/*	Universally Unique Identifier	*/
#ifndef UUID_DEFINED
#define UUID_DEFINED
typedef struct _UUID
{
    Uint32	Data1;
    Uint16	Data2;
    Uint16	Data3;
    Uint8	Data4[8];
} UUID;
typedef UUID Uuid;
#endif

/*	ライブラリIDの'闍`		*/
#define OVD_LID { \
	0xba7e0a00, 0xbda9, 0x11d1, 0x8a, 0xeb, 0x00, 0x60, 0x08, 0x95, 0x62, 0x5f\
}\

/*	エラーコード	*/
typedef enum {
	OVE_ERR_OK				= 0,		/*  正常終了					*/
	OVE_ERR_INVALID_FORMAT	= -1,		/*  不正なフォーマット			*/
	OVE_ERR_INVALID_LIB_ID	= -2,		/*  不正なライブラリID			*/
	OVE_ERR_INVALID_LIB_HDL	= -3		/*  不正なライブラリハ�"ドル	*/
} OVE_ERR;


/*	ライブラリハ�"ドル	*/
typedef struct {
	struct _ov_lif	*lif;			/*	ライブラリイ�"タフェース			*/
	Uint8			ver;			/*	バージョ�""ﾔ号						*/
	Uint8			revi;			/*	リビジョ�""ﾔ号						*/
	Sint16			rsv1;			/*	予約 1								*/
	Sint32			rsv2;			/*	予約 2								*/
	Sint32			rsv3;			/*	予約 3								*/
	UUID    		lid;			/*	ライブラリID						*/
} OVS_LIB_OBJ;
typedef OVS_LIB_OBJ *OVLIB;

/*	各ライブラリが必ず作成するライブラリイ�"タフェース関�"  				*/
typedef struct _ov_lif {
	void (*QueryInterface)();		/*		for COM compatibility			*/
	void (*AddRef)();				/*		for COM compatibility			*/
	void (*Release)();				/*		for COM compatibility			*/
	void (*Init)(OVLIB ovl);		/*		初期化							*/
	void (*Finish)(OVLIB ovl);		/*		終了処理						*/
	void (*Exec)(OVLIB ovl);		/*		実行							*/
} OVS_LIB_IF;
typedef OVS_LIB_IF *OVLIB_IF;

/***
*		実行モジュールのための関�"
***/

/*	実行モジュールハ�"ドル	*/
typedef struct {
	Sint32	rsv[8];
	OVS_LIB_OBJ libobj[1];
} OVS_MDL_OBJ;
typedef OVS_MDL_OBJ *OVMDL;

/***
*			ライブラリ"熾"のための'闍`
***/

/*	クリティカルセクショ�"	*/
typedef Sint32 OV_CS;

/*	ライブラリ管理テーブルへのポイ�"タのアドレス	*/
#define	OVD_LIBTBL_ADR_PTR	(0x8C00FFF0)
/*	ライブラリ管理テーブルの最'�"o録�"へのアドレス	*/
#define	OVD_LIBTBL_ADR_MAX	(0x8C00FFF4)
/*	ライブラリ管理テーブルのアドレス	*/
#define	OVD_DEF_LIBTBL_ADR	(0x8C00FC00)

/*	ライブラリ管理テーブルの最'�"o録�"	*/
#define	OVD_DEF_LIBTBL_MAX	(120)

/*	ライブラリ管理テーブル	*/
typedef struct {
	Sint32	nlib;				/* "o録されているライブラリ�"				*/
	Sint32	rsv1;				/* 予約１									*/
	struct {
		OVLIB	lib;			/* ライブラリハ�"ドル						*/
		Sint32	ref;			/* 参照カウ�"タ								*/
	} entry[1];
} OVS_LIBTBL;

/***
*		関�"宣言
***/

/*	ライブラリハ�"ドルの取"ｾ												*/
OVLIB ovLibOpen(UUID *lid);
/*	ライブラリハ�"ドルの開放												*/
void ovLibClose(UUID *lid);
/*	モジュールファイルの"ﾇみ込み											*/
OVE_ERR ovMdlEntryTbl(OVMDL ovmdl);
/*	モジュールファイルの開放												*/
OVE_ERR ovMdlDeleteTbl(OVMDL ovmdl);
/*  実行モジュールの実行													*/
Sint32 ovMdlExec(OVMDL ovmdl);

/*  ライブラリイ�"タフェースの取"ｾ											*/
OVLIB_IF ovLibGetLibIf(OVLIB lib);
/*  ライブラリのバージョ�"の取"ｾ											*/
void ovLibGetVersion(OVLIB lib, Uint8 *ver, Uint8 *revi);
/*  ライブラリIDの取"ｾ														*/
UUID *ovLibGetUuid(OVLIB lib);

/*  ライブラリ管理ライブラリの初期化 ('ﾊ常使用しない）						*/
void ovInit(void *ovltbl, Sint32 nlib);
/*  ライブラリ管理ライブラリの終了処理 ('ﾊ常使用しない）					*/
void ovFinish(void);
/*	ライブラリハ�"ドルの"o録												*/
OVE_ERR ovLibEntryTbl(OVLIB lib);
/*	ライブラリハ�"ドルの削除												*/
OVE_ERR ovLibDeleteTbl(OVLIB lib);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif     /*  _SG_OV_H_	*/

