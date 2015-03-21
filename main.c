#include <stdio.h>
#include <fcntl.h>

main ( )
{
    int i,fd;
    char ch;
    char write_buf[100];
    char read_buf[100];

    fd = open( "/dev/test", O_RDWR );

    if ( fd == -1 ) {

        printf( "Error in opening file\n" );
        return -1;
    }

    read( fd, read_buf, sizeof( read_buf ) );
    printf ( "The data in the device is: %s\n", read_buf );
    close( fd );

}