#include "lib_tar.h"
#include "string.h"
#include "stdio.h"
#define BLOCK_SIZE 512

tar_header_t header;

/**
 * Checks whether the archive is valid.
 *
 * Each non-null header of a valid archive has:
 *  - a magic value of "ustar" and a null,
 *  - a version value of "00" and no null,
 *  - a correct checksum
 *
 * @param tar_fd A file descriptor pointing to the start of a file supposed to contain a tar archive.
 *
 * @return a zero or positive value if the archive is valid, representing the number of non-null headers in the archive,
 *         -1 if the archive contains a header with an invalid magic value,
 *         -2 if the archive contains a header with an invalid version value,
 *         -3 if the archive contains a header with an invalid checksum value
 */
int check_archive(int tar_fd) {
    tar_header_t header;
    int num_headers = 0; 
    unsigned int computed_chksum = 0; 

    if (read(tar_fd, &header, sizeof(header)) != sizeof(header)){

        printf("Un header doit faire 512 bytes\n");
        return -5;
    }

    if (memcmp(header.magic,TMAGIC,TMAGLEN) !=0 || memcmp(header.magic,NULL,TMAGLEN) == 0){

        printf("tu n'as pas le bon format\n");
        return -1;
    }

    if (memcmp(header.version,TVERSION,TVERSLEN) !=0 || memcmp(header.version,NULL,TVERSLEN) == 0){

        printf("tu n'as pas le bon format\n");
        return -2;
    }

    unsigned int stored_chksum = TAR_INT(header.chksum);  // Somme attendue

        char *raw_header = (char *)&header;


    for (size_t i = 0; i < sizeof(header); i++) {
        if (i >= 148 && i < 156) {
            computed_chksum += ' '; 
        } else {
            computed_chksum += (unsigned char)raw_header[i];
        }
    }

    if (computed_chksum != stored_chksum){
        
        return -3;
    }

    return 0;

}
    
    
    /*
    while (read(tar_fd, &header, sizeof(header)) == sizeof(header)) {
        // Vérifiez si l'en-tête est vide (fin de l'archive)
        if (header.name[0] == '\0') {
            break;  // Fin des en-têtes
        }

        // Vérification du champ "magic" 
        if (strncmp(header.magic, TMAGIC, TMAGLEN) != 0) {
            
            printf("tu n'as pas le bon format\n");
            return -1;  
        }

        // Vérification du champ "version"
        if (strncmp(header.version, TVERSION, TVERSLEN) != 0) {
            return -2; 
        }

        // Vérification de la somme de contrôle
        unsigned int stored_chksum = TAR_INT(header.chksum);  // Somme attendue
        unsigned int computed_chksum = 0;

        
        char *raw_header = (char *)&header;
        for (size_t i = 0; i < sizeof(header); i++) {
            if (i >= 148 && i < 156) {
                computed_chksum += ' ';  // Champ 'chksum' rempli d'espaces
            } else {
                computed_chksum += (unsigned char)raw_header[i];
            }
        }

        if (stored_chksum != computed_chksum) {
            return -3;  
        }

        
        num_headers++;
    }

    return num_headers; 
}

*/