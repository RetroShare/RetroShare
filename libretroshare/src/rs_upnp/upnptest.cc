//this file use miniupnp

#include <upnp/upnphandler.h>


int main(int argc, char **argv)
{

	int id = argc % 3;

	/*********
	char *fhash1 = "3509426505463458576487";
	char *hash2 = "1549879882341985914515";
	char *hash3 = "8743598543269526505434";

	int port1 = 8754;
	int port2 = 2355;
	int port3 = 6621;
	**********/

	std::cerr << "Starting dhttest Id: " << id <<  std::endl;
/******************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#else
        // Windows Networking Init.
        WORD wVerReq = MAKEWORD(2,2);
        WSADATA wsaData;

        if (0 != WSAStartup(wVerReq, &wsaData))
        {
                std::cerr << "Failed to Startup Windows Networking";
                std::cerr << std::endl;
        }
        else
        {
                std::cerr << "Started Windows Networking";
                std::cerr << std::endl;
        }

#endif
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/

  #ifdef PTW32_STATIC_LIB
         pthread_win32_process_attach_np();
  #endif


	upnphandler upnp;

	upnp.setInternalPort(12122);

	for(int i = 0; 1; i++)
	{

/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
#ifndef WINDOWS_SYS
                        sleep(1);
#else

                        Sleep(1000);
#endif
/********************************** WINDOWS/UNIX SPECIFIC PART ******************/
		
	if (i % 120 == 10)
	{
		/* start up a forward */
		upnp.enable(true);

	}

	if (i % 120 == 60)
	{
		/* shutdown a forward */
		upnp.restart();
	}

	if (i % 120 == 100)
	{
		/* shutdown a forward */
		upnp.shutdown();
	}

	}
}

