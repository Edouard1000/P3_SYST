#include "lib_tar.h"

#define BLOCK_SIZE 512

tar_header_t header;

int move_toNext_file(int fd, tar_header_t* head){
    unsigned long fileSize = strtol(head->size,NULL,8);
    long total_ofset = BLOCK_SIZE + ((fileSize + BLOCK_SIZE - 1)/BLOCK_SIZE) * BLOCK_SIZE;
    if(lseek(fd,total_ofset,SEEK_SET)==-1){
        printf("Erreur dans Move_toNext_file\n");
        return 0;
    }
    return 1;
}

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
    
    while (read(tar_fd, &header, sizeof(header)) == sizeof(header)) {
        // Vérifiez si l'en-tête est vide (fin de l'archive)
        if (header.name[0] == '\0') {
            break;  // Fin des en-têtes
        }

        // Vérification du champ "magic" 
        if (strncmp(header.magic, TMAGIC, TMAGLEN) != 0) {
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

/**
 * Checks whether an entry exists in the archive.
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive.
 *
 * @return zero if no entry at the given path exists in the archive,
 *         any other value otherwise.
 */
int exists(int tar_fd, char *path) {
    int chekFd = check_archive(tar_fd);

    if (chekFd != 0){
        return chekFd;
    }
    while (read(tar_fd, (void *)&header, sizeof(tar_header_t)) == sizeof(tar_header_t)) {
        // Vérifie si l'en-tête est un fichier vide (fin de l'archive)
        if (memcmp(header.name, "\0", 100) == 0) {
            printf("la fin de l'archive\n");
                if ((lseek(tar_fd,0,SEEK_SET ))==-1){
                perror("erreur lors pour retourner l'offset au debut du fichier\n");
                return -1;
            }

            return -1;
        }
        if (memcmp(header.name,path,100)==0){
            printf("Le fichier est dans l'archive");
            
            /* je pense que cest pas une bonne idee de retourner au debut de l'archive apre avoir trouve le fichier sachant que par la suite on a besoin d'utiliser son header pour veifier si le fichier est un fichier ou repertoire ou symlink
            if ((lseek(tar_fd,0,SEEK_SET ))==-1){
                perror("erreur lors pour retourner l'offset au debut du fichier\n");
                return -1;
            }

            */
            return 0;
            

        }
       if(move_toNext_file(tar_fd,&header)==-1){
         printf("Erreur dans Move_toNext_file\n");
         return -1;
       }
        
        
    }
    if ((lseek(tar_fd,0,SEEK_SET ))==-1){
        perror("erreur lors pour retourner l'offset au debut du fichier\n");
        return -1;
    }
    
    
    return -1;
}

/**
 * Checks whether an entry exists in the archive and is a directory.
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive.
 *
 * @return zero if no entry at the given path exists in the archive or the entry is not a directory,
 *         any other value otherwise.
 */
int is_dir(int tar_fd, char *path) {

    int checkExitsFile=exists(tar_fd,path);

    if (checkExitsFile != 0){
        return checkExitsFile;
    }

    if ((memcmp(header.typeflag,NULL,156))){

        if ((lseek(tar_fd,0,SEEK_SET ))==-1){
        perror("erreur lors pour retourner l'offset au debut du fichier\n");
        return -1;
       
        }

        printf("le typeflag est nul \n");
        return -1;
    }

    long typeFlag = strtol(header.typeflag,NULL,8);

    if (typeFlag != 5){
        printf("Ce fichier n'est pas un répertoire (typeflag: %ld).\n", typeFlag);

        if ((lseek(tar_fd,0,SEEK_SET ))==-1){
        perror("erreur lors pour retourner l'offset au debut du fichier\n");
        return -1;
       
        }  

        return -1;      
        
    }
    


    
    
    return 0;
}

/**
 * Checks whether an entry exists in the archive and is a file.
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive.
 *
 * @return zero if no entry at the given path exists in the archive or the entry is not a file,
 *         any other value otherwise.
 */
int is_file(int tar_fd, char *path) {
    int checkExitsFile=exists(tar_fd,path);

    if (checkExitsFile != 0){
        return checkExitsFile;
    }

    if ((memcmp(header.typeflag,NULL,156))){

        if ((lseek(tar_fd,0,SEEK_SET ))==-1){
        perror("erreur lors pour retourner l'offset au debut du fichier\n");
        return -1;
       
        }

        printf("le typeflag est nul \n");
        return -1;
    }

    long typeFlag = strtol(header.typeflag,NULL,8);

    if (typeFlag == 0 || typeFlag==2){
        return 0;
   
        
    }
    printf("Ce fichier n'est pas un fichier (typeflag: %ld).\n", typeFlag);

    if ((lseek(tar_fd,0,SEEK_SET ))==-1){
        perror("erreur lors pour retourner l'offset au debut du fichier\n");
        return -1;
       
    }  

    return -1;   

}

/**
 * Checks whether an entry exists in the archive and is a symlink.
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive.
 * @return zero if no entry at the given path exists in the archive or the entry is not symlink,
 *         any other value otherwise.
 */
int is_symlink(int tar_fd, char *path) {
    int checkExitsFile=exists(tar_fd,path);

    if (checkExitsFile != 0){
        return checkExitsFile;
    }

    if ((memcmp(header.typeflag,NULL,156))){

        if ((lseek(tar_fd,0,SEEK_SET ))==-1){
        perror("erreur lors pour retourner l'offset au debut du fichier\n");
        return -1;
       
        }

        printf("le typeflag est nul \n");
        return -1;
    }

    long typeFlag = strtol(header.typeflag,NULL,8);

    if (typeFlag != 1 ){
         printf("Ce fichier n'est pas un symplink (typeflag: %ld).\n", typeFlag);

        if ((lseek(tar_fd,0,SEEK_SET ))==-1){
        perror("erreur lors pour retourner l'offset au debut du fichier\n");
        return -1;
       
        }  

        return -1;      
        
    }

    return 0;
}


/**
 * Lists the entries at a given path in the archive.
 * list() does not recurse into the directories listed at the given path.
 *
 * Example:
 *  dir/          list(..., "dir/", ...) lists "dir/a", "dir/b", "dir/c/" and "dir/e/"
 *   ├── a
 *   ├── b
 *   ├── c/
 *   │   └── d
 *   └── e/
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive. If the entry is a symlink, it must be resolved to its linked-to entry.
 * @param entries An array of char arrays, each one is long enough to contain a tar entry path.
 * @param no_entries An in-out argument.
 *                   The caller set it to the number of entries in `entries`.
 *                   The callee set it to the number of entries listed.
 *
 * @return zero if no directory at the given path exists in the archive,
 *         any other value otherwise.
 */
int list(int tar_fd, char *path, char **entries, size_t *no_entries) {
    return 0;
}

/**
 * Reads a file at a given path in the archive.
 *
 * @param tar_fd A file descriptor pointing to the start of a valid tar archive file.
 * @param path A path to an entry in the archive to read from.  If the entry is a symlink, it must be resolved to its linked-to entry.
 * @param offset An offset in the file from which to start reading from, zero indicates the start of the file.
 * @param dest A destination buffer to read the given file into.
 * @param len An in-out argument.
 *            The caller set it to the size of dest.
 *            The callee set it to the number of bytes written to dest.
 *
 * @return -1 if no entry at the given path exists in the archive or the entry is not a file,
 *         -2 if the offset is outside the file total length,
 *         zero if the file was read in its entirety into the destination buffer,
 *         a positive value if the file was partially read, representing the remaining bytes left to be read to reach
 *         the end of the file.
 *
 */
ssize_t read_file(int tar_fd, char *path, size_t offset, uint8_t *dest, size_t *len) {
    return 0;

    
}