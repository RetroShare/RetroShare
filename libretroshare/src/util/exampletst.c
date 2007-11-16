
#include "utest.h"

#include <string.h>

/* must define the global variables */
INITTEST();  

int main(int argc, char **argv)
{
	int a = 2;
	int b = 3;
	int c = 2;

	CHECK( a == c );

	REPORT( "Initial Tests");

	CHECK( (0 == strcmp("123", "123")) );

	REPORT( "Successful Tests");

	CHECK( a == b );
	CHECK( (0 == strcmp("123", "12345")) );

	REPORT( "Failed Tests" );

	CHECK( 1 );
	CHECK( a == c );

	REPORT( "Later Successful Tests");


	FINALREPORT( "Example Tests" );

	return TESTRESULT();
}

	
	
