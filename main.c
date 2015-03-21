#include <stdio.h>
#include <fcntl.h>

main ( )
{
    int i,fd;
    char ch;
    char read_buf[100];

    fd = open( "/dev/test", O_RDONLY );

    if ( fd == -1 ) {

        printf( "Error in opening file\n" );
        return -1;
    }

    read( fd, read_buf, sizeof( read_buf ) );
    printf ( "The data in the device is: %s\n", read_buf );
    read( fd, read_buf, sizeof( read_buf ) );
    close( fd );

}