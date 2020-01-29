#include <stdlib.h>
/* autori 
	gazulli e karici 

 */
int main()
{
	system("clear");
	system("gcc -c funzioni.c");
	system("gcc gestore.c funzioni.o -o gestore");
	system("gcc student.c funzioni.o -o student");
}