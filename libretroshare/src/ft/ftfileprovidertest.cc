#include "ftfileprovider.h"

int main(){
	ftFileProvider fp("dummy.txt",1,"ABCDEF");
	char data[2];
	long offset = 0;
	for (int i=0;i<10;i++) {
		
		if (fp.getFileData(offset,2,&data)){
			std::cout <<"Recv data " << data[0] <<  std::endl;	
		}
		else {
			std::cout <<"Recv no data." << std::endl;
		}
		offset+=2;
	}
	
	
	ftFileProvider fp1("dummy1.txt",3,"ABCDEF");
	char data1[3];
	offset = 0;
	for (int i=0;i<10;i++) {
		
		if (fp1.getFileData(offset,2,&data1)){
			std::cout <<"Recv data " << data1[0] <<  std::endl;	
		}
		else {
			std::cout <<"Revc no data" << std::endl;
		}
		offset+=2;
	}
	return 1;
}
