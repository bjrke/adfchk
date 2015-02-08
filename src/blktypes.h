/* blktypes.h */

#ifndef BLKTYPES_H
#define BLKTYPES_H

/* Generic block data */

#define ROOTBASE 0x06E000
#define ROOTBLOCK 880

#define BLOCKSPERDISK 1760
#define BYTESPERBLK    512

#define HASHTBLSTART 77
#define HASHTBLEND    6

/* Block & checksum types */

/* these #defines are actually offsets for the memset routine which
   clears the old checksum; this way we can use the same routine for 
   both checksum types! */
#define CHKSUM_TYPE_BOOT_B 4
#define CHKSUM_TYPE_OTHER_B 20

#define CHKSUM_TYPE_BOOT_L 1
#define CHKSUM_TYPE_OTHER_L 5

/* Amiga original primary block type IDs, first longword  */

#define BLKTYPE_PRI_DOSBASE 0x444F5300
#define BLKTYPE_PRI_RNCOPYLK 0x524E4300   /* RNC          */
#define BLKTYPE_PRI_A1K_KICK   0x4B49434B /* KICK (A1000) */
#define BLKTYPE_PRI_QBACKDISK1 0x51620000 /* Qb??         */
#define BLKTYPE_PRI_QBACKDISK2 0x51420000 /* QB??  (rare) */
#define BLKTYPE_QBACKDISK      0x08
#define BLKTYPE_A1KICKDSK      0x09
#define BLKTYPE_RNCOPYLOCK     0x0A
#define BLKTYPE_UNKNOWN        0x0B

#define BLKTYPE_PRI_UNUSED     0x00000000
#define BLKTYPE_PRI_STRUCTINIT 0x00000002
#define BLKTYPE_PRI_DATA       0x00000008
#define BLKTYPE_PRI_FILELIST   0x00000010

/* Amiga original secondary block type IDs, last longword */
#define BLKTYPE_SEC_ROOT        0x000000001
#define BLKTYPE_SEC_USERDIR     0x00000002
#define BLKTYPE_SEC_FILEHDRLIST 0xFFFFFFFD


/* these are my custom defines, 100% linear, for program-internal
   use only to make life easier in coding, as the Amiga IDs are
   very chaotic and sometimes depend on the value of the secondary
   type found at the last long word in the sector */

#define BLKTYPE_CUST_DATABLK_ALL_OK         0x00  /* O O O O */

/* hdr ok / BAD SEQ NUM / val data ok / next data ok / ??? */
#define BLKTYPE_CUST_DATABLK_BAD_SEQNUM     0x01  /* O O O I */

/* hdr ok / seq num ok / BAD VAL DATA / next data ok / ??? */
#define BLKTYPE_CUST_DATABLK_BAD_VALIDD     0x02  /* O O I O */

/* hdr ok / seq num ok / val data ok / BAD NEXT DATA / ??? */
#define BLKTYPE_CUST_DATABLK_BAD_NEXTDT     0x04  /* O I O O */

/* hdr ok / seq num ok / val data ok / next data ok; 
   next data block is IDENTICAL to sequence number (DANGER!) */
#define BLKTYPE_CUST_DATABLK_BAD_SELFPT     0x08  /* I O O O */

#define BLKTYPE_CUST_FILEHDR                0x09
#define BLKTYPE_CUST_FILELST                0x0A
#define BLKTYPE_CUST_ROOTBLK                0x0B
#define BLKTYPE_CUST_USERDIR                0x0C
#define BLKTYPE_CUST_FORMATT                0x0D
#define BLKTYPE_CUST_INVALID1               0x0E
#define BLKTYPE_CUST_INVALID2               0x0F
#define BLKTYPE_CUST_UNKNOWN                0x10

/*both of these are for displaying verbosely on screen and in the log file 
  and hence not abbreviated;
  BTW those two identical INTL. values @ bit 4/6 resp. bit 5/7 are no cut-and-paste errors,
  but refer to the weirdness of the Amiga architecture: once bit 2 (DIRC) is active, the 
  INTL mode is ALWAYS switched on too, *regardless* of the 1st bit; however, in reality,
  a LSB with value 0x06 or 0x07 is practically impossible to find.
*/                    

#endif