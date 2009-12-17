#ifndef DERVISH_MSG_C_H
#define DERVISH_MSG_C_H

#define dervish_facnum 0x7
typedef unsigned int RET_CODE;

/*  */
/*  DERVISH SUCCESS MESSAGE */
/*  */
#define   SH_SUCCESS		0x8001c009
/*  */
/*  COMMAND PROGRAM LIBRARY MESSAGES */
/*  */
#define SH_LOCK_FILE_ERR	0x8001c014
#define SH_LOCK_SEM_ERR		0x8001c01c
#define SH_SHARED_MEM_ERR	0x8001c024
#define SH_PIPE_WRITE_MAX	0x8001c02c
#define SH_PIPE_WRITEERR	0x8001c034
#define SH_PIPE_OPENERR		0x8001c03c
#define SH_PIPE_READERR		0x8001c044
#define SH_SELECT_ERR		0x8001c04c
#define SH_CMD_NOT_INIT		0x8001c054
#define SH_SAO_SYNC_ERR		0x8001c05c
#define SH_CMD_ABORT		0x8001c063
#define SH_RANGE_ERR		0x8001c06c
#define SH_SAO_NUM_INV		0x8001c074
#define SH_DRAW_OPT_INV		0x8001c07c
#define SH_MALLOC_ERR		0x8001c084
#define SH_NO_FSAO		0x8001c08c
#define SH_PIPE_CLOSE		0x8001c092
/*
 * Add your own codes here. 
 * Note that we will get cvs merge errors if two people add
 * a new code but that it will be trivial to resolve.
 * Make sure that you increment the value added to SHERRORBASE
 * when you construct your new error value.
 */
#define SHERRORBASE		0x4000
#define SH_GENERIC_ERROR		SHERRORBASE	/* a generic error */
#define SH_FREE_NONVIRTUAL_REG		SHERRORBASE +  1 /* attempt to free non-virtual REGION*/
#define SH_HEADER_IS_NULL		SHERRORBASE +  2
#define SH_HEADER_INSERTION_ERROR	SHERRORBASE +  3
#define SH_UNK_PIX_TYPE			SHERRORBASE +  4
#define SH_IS_PHYS_REG			SHERRORBASE +  5
#define SH_IS_SUB_REG			SHERRORBASE +  6
#define SH_HAS_SUB_REG			SHERRORBASE +  7
#define SH_REGINFO_ERR			SHERRORBASE +  8
#define SH_PIX_TYP_NOMATCH		SHERRORBASE +  9
#define SH_ENVSCAN_ERR			SHERRORBASE + 10
#define SH_FITS_OPEN_ERR		SHERRORBASE + 11
#define SH_FITS_PIX_READ_ERR		SHERRORBASE + 12
#define SH_FITS_HDR_WRITE_ERR		SHERRORBASE + 13
#define SH_FITS_PIX_WRITE_ERR		SHERRORBASE + 14
#define SH_NO_FITS_KWRD			SHERRORBASE + 15
#define SH_INV_FITS_FILE		SHERRORBASE + 16
#define SH_INV_BSCALE_KWRD		SHERRORBASE + 17
#define SH_INV_BZERO_KWRD		SHERRORBASE + 18
#define SH_NO_BSCALE_KWRD		SHERRORBASE + 19
#define SH_HEADER_FULL			SHERRORBASE + 20
#define SH_NO_BZERO_KWRD		SHERRORBASE + 21
#define SH_NO_PHYS_REG			SHERRORBASE + 22
#define SH_PHYS_FUNC_ERR		SHERRORBASE + 23
#define SH_PHYS_HDR_INV			SHERRORBASE + 24
#define SH_PHYS_HDR_NOEND		SHERRORBASE + 25
#define SH_PHYS_REG_REQD		SHERRORBASE + 26
#define SH_DIR_TOO_LONG			SHERRORBASE + 27
#define SH_EXT_TOO_LONG			SHERRORBASE + 28
#define SH_TYPE_MISMATCH		SHERRORBASE + 29
#define SH_CANT_SPLIT_LIST		SHERRORBASE + 30
#define SH_LIST_ELEM_MISMATCH		SHERRORBASE + 31
#define SH_CANT_SWITCH_TYPE		SHERRORBASE + 32
#define SH_MISMATCH_NAXIS		SHERRORBASE + 33 /* NAXIS mismatch    */
#define SH_NOT_SUPPORTED		SHERRORBASE + 34 /* Unsupported featur*/
#define SH_INTERNAL_ERR			SHERRORBASE + 35 /* Internal error    */
#define SH_CONFLICT_ARG			SHERRORBASE + 36 /* Conflicting args  */
#define SH_FITS_NOEXTEND		SHERRORBASE + 37 /* No FITS extensions*/
#define	SH_LIST_END			SHERRORBASE + 38 /* End of list       */
#define	SH_NOEXIST			SHERRORBASE + 39 /* Item doesn't exist*/
#define	SH_MISARG			SHERRORBASE + 40 /* Missing argument  */
#define	SH_INVARG			SHERRORBASE + 41 /* Invalid argument  */
#define	SH_INVOBJ			SHERRORBASE + 42 /* Invalid object    */
#define	SH_TRUNC			SHERRORBASE + 43 /* Generic truncation*/
#define SH_HAS_SUB_MASK                 SHERRORBASE + 44 /* Mask has a submask*/
#define SH_MASKINFO_ERR                 SHERRORBASE + 45 /* Error getting mask info structure */
#define SH_SIZE_MISMATCH                SHERRORBASE + 46 /* region and FITS file data sizes do not match */
#define SH_IS_SUB_MASK                  SHERRORBASE + 47 /* Mask is a submask*/
#define SH_NO_NAXIS_KWRD                SHERRORBASE + 48
#define SH_NO_NAXIS1_KWRD               SHERRORBASE + 49
#define SH_NO_NAXIS2_KWRD               SHERRORBASE + 50
/*
 *   #define SH_BAD_NAXIS3_KWRD               SHERRORBASE + 92
 */
#define SH_UNSUP_NAXIS_KWRD             SHERRORBASE + 51
#define SH_FITS_HDR_READ_ERR            SHERRORBASE + 52
#define SH_NO_MASK_DATA                 SHERRORBASE + 53
#define SH_CANT_MAKE_HDR                SHERRORBASE + 54
#define SH_WAITPID_ERR                  SHERRORBASE + 55
#define SH_FORK_ERR                     SHERRORBASE + 56
#define SH_NO_END_QUOTE                 SHERRORBASE + 57
#define SH_TOO_MANY_ARGS                SHERRORBASE + 58
#define	SH_ALREXIST			SHERRORBASE + 59 /* Obj alread exists */

#define SH_TABLE_FULL			SHERRORBASE + 60 /* all entries used */
#define SH_TABLE_SYNTAX_ERR		SHERRORBASE + 61
#define SH_ARRAY_CHECK_ERR		SHERRORBASE + 62
#define SH_TOO_MANY_INDIRECTIONS	SHERRORBASE + 63
#define SH_ARRAY_SYNTAX_ERR		SHERRORBASE + 64
#define SH_OBJNUM_INVALID		SHERRORBASE + 65
#define SH_MULTI_D_ERR			SHERRORBASE + 66
#define SH_BAD_CONTAINER		SHERRORBASE + 67
#define SH_BAD_SCHEMA			SHERRORBASE + 68
#define SH_FLD_SRCH_ERR			SHERRORBASE + 69
#define SH_BAD_INDEX			SHERRORBASE + 70
#define SH_INV_DATA_PTR			SHERRORBASE + 71
#define SH_SIZE_UNMATCH			SHERRORBASE + 72
#define SH_CONTAINER_ADD_ERR		SHERRORBASE + 73
#define SH_BAD_FLD_NUM			SHERRORBASE + 74
#define SH_OBJ_CREATE_ERR		SHERRORBASE + 75

/*
 * Chain error codes
 */
#define SH_CHAIN_NULL           SHERRORBASE + 76  /* Chain pointer is NULL */
#define SH_CHAIN_EMPTY          SHERRORBASE + 77  /* Chain size is 0 */
#define SH_CHAIN_INVALID_CURSOR SHERRORBASE + 78
#define SH_CHAIN_INVALID_ELEM   SHERRORBASE + 79
					/* SHERRORBASE + 80 is unused */
/*
 * And now back to our regularly scheduled program 
 */
#define SH_NAMES_TOO_LONG               SHERRORBASE + 81
#define SH_NO_COLOR_ENTRIES             SHERRORBASE + 82
#define SH_HASH_KEY_TOO_BIG             SHERRORBASE + 83
#define SH_HASH_TAB_IS_FULL             SHERRORBASE + 84
#define SH_HASH_ENTRY_NOT_FOUND         SHERRORBASE + 85
#define SH_FITS_HDR_SAVE_ERR            SHERRORBASE + 86

/*
 * Tape error codes
 */
#define SH_TAPE_BAD_FILE_PTR            SHERRORBASE + 87
#define SH_TAPE_DD_FAIL                 SHERRORBASE + 88
#define SH_TAPE_DD_ERR                  SHERRORBASE + 89

/*
 * Schema error codes
 */
#define SH_SCHEMA_LOCKED                SHERRORBASE + 90

/*
 * Error code for fsao zoom factor
 */
#define SH_BAD_ZOOM                     SHERRORBASE + 91

#define SH_BAD_NAXIS3_KWRD              SHERRORBASE + 92

#endif	/* ifndef DERVISH_MSG_C_H */
