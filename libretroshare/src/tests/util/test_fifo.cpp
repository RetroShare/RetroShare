// COMPILE_LINE: g++ test_fifo.cpp -o test_fifo -lstdc++ -I..
//
// This is a test program for fifo previewing of files.
// Usage:
// 	test_fifo -i [movie file]
//
// The program creates a named pipe /tmp/fifo.avi on Linux, \\.\pipe\fifo.avi on Windows to be played with e.g. mplayer
// On Windows the mplayer needs the argument -noidx, because the pipe is not seekable
//
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <pgp/argstream.h>
#include <string.h>
#include <string>
#include <stdexcept>
#include <stdio.h>
#include <errno.h>

#ifdef WIN32
#include <windows.h>
#endif

#ifdef WIN32
#define myFIFO TEXT("\\\\.\\pipe\\fifo.avi")
#else
#define myFIFO "/tmp/fifo.avi"
#endif

int main(int argc,char *argv[])
{
	try
	{
		std::string movie_name ;

		argstream as(argc,argv) ;

		as >> parameter('i',"input",movie_name,"file name of the movie to preview",true) 
			>> help() ;

		as.defaultErrorHandling() ;


#ifdef WIN32
		HANDLE fifo = CreateNamedPipe(myFIFO, PIPE_ACCESS_OUTBOUND, PIPE_TYPE_BYTE | PIPE_WAIT , 2, 1024, 1024, 2000, NULL);
#else
		int status, num, fifo;
		umask(0);
		if (mkfifo(myFIFO, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP))
			throw std::runtime_error("Cannot create fifo") ;

		fifo = open(myFIFO, O_WRONLY);

		std::cerr << "Opening fifo = " << fifo << " with file name " << myFIFO << std::endl;
#endif

#ifdef WIN32
		if(fifo == INVALID_HANDLE_VALUE)
#else
		if(fifo < 0) 
#endif
		{ 
			printf("\nCan't open fifo: %s \n", strerror(errno));
			return 0;
		}

#ifdef WIN32
		printf("pipe=%p\n", fifo);

		printf("connect...\n");
		bool connected = ConnectNamedPipe(fifo, NULL) ?  TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
		if (!connected) {
			printf("ConnectNamedPipe failed. error=%ld\n", GetLastError());
			return -1;
		}
		printf("...connected\n");
#endif

		static const int SIZE = 1024*1024 ;	// 1 MB

		void *membuf = malloc(SIZE) ;
		FILE *f = fopen(movie_name.c_str(),"rb") ;
		int chunk = 0;

		if(f == NULL)
			throw std::runtime_error("Cannot open movie to stream into fifo") ;

		while(true) 
		{
			int n = fread(membuf, 1, SIZE, f) ;

			if(n == 0)
				break ;

#ifdef WIN32
			DWORD dwNumberOfBytesWritten;
			if (!WriteFile(fifo, membuf, n, &dwNumberOfBytesWritten, NULL) || dwNumberOfBytesWritten != (DWORD) n) {
				printf("WriteFile failed. error=%ld\n", GetLastError());
#else
			if(write(fifo,membuf,SIZE) != n) {
#endif
				throw std::runtime_error("Could not write chunk to fifo") ;
			}

			std::cerr << "Streamed chunk " << chunk << std::endl;
			if(n < SIZE)
				break ;

			chunk++ ;
		}

		fclose(f) ;
		free(membuf) ;

#ifdef WIN32
//		FlushFileBuffers(fifo);
		DisconnectNamedPipe(fifo);
#endif

		std::cerr << "Sleeping 60 sec." << std::endl;
#ifdef WIN32
		Sleep(60000) ;
		CloseHandle(fifo);
#else
		sleep(60) ;
		close(fifo) ;
#endif

		return 0 ;
	}
	catch(std::exception& e)
	{
		std::cerr << "Exception caught: " << e.what() << std::endl;
	}
}


