#ifndef _UNIT_TEST_MACROS_H__
#define _UNIT_TEST_MACROS_H__

#include <stdio.h>

#define TFAILURE( s ) printf( "FAILURE: " __FILE__ ":%-4d %s\n",  __LINE__, s )
#define TSUCCESS( s ) printf( "SUCCESS: " __FILE__ ":%-4d %s\n",  __LINE__, s )

/* careful with this line (no protection) */
#define INITTEST()  int ok = 1; int gok = 1; 

/* declare the variables */
extern int ok;
extern int gok;

#define CHECK( b )  do { if ( ! (b) ) { ok = 0; TFAILURE( #b ); } } while(0)
#define FAILED( s ) do { ok = 0; TFAILURE( s ); } while(0)
#define REPORT( s ) do { if ( ! (ok) ) { ok = 0; TFAILURE( s ); } else { TSUCCESS( s );} gok &= ok; ok = 1; } while(0)
#define REPORT2( b, s  ) do { if ( ! (b) ) { ok = 0; TFAILURE( s ); } else { TSUCCESS( s );} gok &= ok; ok = 1; } while(0)
#define FINALREPORT(  s  ) do { gok &= ok; ok = 1; if ( ! (gok) ) { TFAILURE( s ); } else { TSUCCESS( s );} } while(0)
#define TESTRESULT() (!gok)

#endif
