// COMPILE_LINE: g++ test_fifo.cpp -o test_fifo -lstdc++ -I..
//
// This is a test program for fifo previewing of files.
// Usage:
// 	test_fifo -i [movie file]
//
// The program creates a named pipe /tmp/fifo.avi, to be played with e.g. mplayer
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
#define myFIFO "/tmp/fifo.avi"

int main(int argc,char *argv[])
{
	try
	{
		int status, num, fifo;
		std::string movie_name ;

		argstream as(argc,argv) ;

		as >> parameter('i',"input",movie_name,"file name of the movie to preview",true) 
			>> help() ;

		as.defaultErrorHandling() ;

		umask(0);
		if (mkfifo(myFIFO, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP))
			throw std::runtime_error("Cannot create fifo") ;

		fifo = open(myFIFO, O_WRONLY);

		std::cerr << "Opening fifo = " << fifo << " with file name " << myFIFO << std::endl;

		if(fifo < 0) 
		{ 
			printf("\nCan't open fifo: %s \n", strerror(errno));
			return 0;
		}

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

			if(write(fifo,membuf,SIZE) != n)
				throw std::runtime_error("Could not write chunk to fifo") ;

			std::cerr << "Streamed chunk " << chunk << std::endl;
			if(n < SIZE)
				break ;

			chunk++ ;
		}

		fclose(f) ;
		free(membuf) ;

		std::cerr << "Sleeping 60 sec." << std::endl;
		sleep(60) ;

		close(fifo) ;
		return 0 ;
	}
	catch(std::exception& e)
	{
		std::cerr << "Exception caught: " << e.what() << std::endl;
	}
}


