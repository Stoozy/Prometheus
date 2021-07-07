#include <string.h>
 
// Function to implement strcat() function in C
char* strcat(char* destination, const char* source)
{
    // make ptr point to the end of destination string
    char* ptr = destination + strlen(destination);
 
    // Appends characters of source to the destination string
    while (*source != '\0')
        *ptr++ = *source++;
 
    // null terminate destination string
    *ptr = '\0';
 
    // destination is returned by standard strcat()
    return destination;
}
 
