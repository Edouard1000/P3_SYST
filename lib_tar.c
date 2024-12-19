#include "lib_tar.h"
#include "string.h"
#include "stdio.h"
#include <sys/types.h>

#define BLOCK_SIZE 512

tar_header_t header;
int skip_file_data(int tar_fd, unsigned long file_size) {
    // Calculer le nombre de blocs nécessaires pour stocker les données du fichier
    unsigned long num_blocks = (file_size + BLOCK_SIZE - 1) / BLOCK_SIZE;

    if (lseek(tar_fd, num_blocks * BLOCK_SIZE, SEEK_CUR) == -1) {
        perror("Error while skipping file data");
        return -1; 
    }

    return 0; 
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
            break;  
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

        
        unsigned long file_size = strtol(header.size, NULL, 8);

        // Sauter les blocs de données
        if (skip_file_data(tar_fd, file_size) == -1) {
            return -4;  
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
    // Remettre le descripteur de fichier au début de l'archive
    if (lseek(tar_fd, 0, SEEK_SET) == -1) {
        perror("Erreur lors du repositionnement au début de l'archive");
        return -1;
    }

    tar_header_t header; 

    
    while (read(tar_fd, &header, sizeof(header)) == sizeof(header)) {
        // Vérifier si l'en-tête est vide (fin de l'archive)
        if (header.name[0] == '\0') {
            break; 
        }

        
        if (strncmp(header.name, path, sizeof(header.name)) == 0) {
            return 1; 
        }
        printf("%s\n", header.name);

        
        unsigned long file_size = strtol(header.size, NULL, 8); 
        
        if (skip_file_data(tar_fd, file_size) == -1) {
            return -4;  
        }
        
    }

    // Aucune correspondance trouvée
    return 0;
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
    
    if (lseek(tar_fd, 0, SEEK_SET) == -1) {
        perror("Erreur lors du repositionnement au début de l'archive");
        return -1;
    }

    while (read(tar_fd, &header, sizeof(header)) == sizeof(header)) {
        
        if (header.name[0] == '\0') {
            break;
        }

        
        if (strncmp(header.name, path, sizeof(header.name)) == 0) {
            
            if (header.typeflag == DIRTYPE) {
                return 1; 
            } else {
                return 0; 
            }
        }

       
        unsigned long file_size = strtol(header.size, NULL, 8);
        if (skip_file_data(tar_fd, file_size) == -1) {
            return -1;
        }
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
    
    if (lseek(tar_fd, 0, SEEK_SET) == -1) {
        perror("Erreur lors du repositionnement au début de l'archive");
        return -1;
    }

    while (read(tar_fd, &header, sizeof(header)) == sizeof(header)) {
        
        if (header.name[0] == '\0') {
            break;
        }

        
        if (strncmp(header.name, path, sizeof(header.name)) == 0) {
            
            if (header.typeflag == REGTYPE || header.typeflag == AREGTYPE) {
                return 1; 
            } else {
                return 0;
            }
        }

        
        unsigned long file_size = strtol(header.size, NULL, 8);
        if (skip_file_data(tar_fd, file_size) == -1) {
            return -1;
        }
    }

    return 0; 
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
    
    if (lseek(tar_fd, 0, SEEK_SET) == -1) {
        perror("Erreur lors du repositionnement au début de l'archive");
        return -1;
    }

    while (read(tar_fd, &header, sizeof(header)) == sizeof(header)) {
        
        if (header.name[0] == '\0') {
            break;
        }

        
        if (strncmp(header.name, path, sizeof(header.name)) == 0) {
           
            if (header.typeflag == SYMTYPE || header.typeflag == LNKTYPE) {
                return 1; 
            } else {
                return 0; 
            }
        }

        
        unsigned long file_size = strtol(header.size, NULL, 8);
        if (skip_file_data(tar_fd, file_size) == -1) {
            return -1;
        }
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

    if (lseek(tar_fd, 0, SEEK_SET) == -1) {
        perror("Erreur lors du repositionnement au début de l'archive");
        
        return 0;
    }
        
    if (is_dir(tar_fd, path) != 1 ){
        
        return 0;
    }



    tar_header_t header2;

    int countEntry = 0;
    char *pathcpy = malloc(100 * sizeof(char));
    


    while (read(tar_fd, &header2, BLOCK_SIZE) == BLOCK_SIZE){

        if (header2.name[0] == '\0'){
            break;
        }

        if (strncmp(header2.name, path,strlen(path))==0){
            const char *remainPath = header2.name + strlen(path);
            if(strchr(remainPath, '/')==NULL || remainPath[strlen(remainPath)-1]=='/'){
                
                if (header2.typeflag == LNKTYPE || header2.typeflag == SYMTYPE){
                    
                    strcpy(pathcpy,header2.linkname);


                    if (exists(tar_fd, pathcpy) !=1){
                        free(pathcpy);
                        return 0;
                    }
                    



                    return list(tar_fd, pathcpy,entries, no_entries);
                }
            

                
                if(countEntry < *no_entries){
                    strncpy(entries[countEntry],header2.name,100);
                    countEntry++;
                }
                
            }                

        }
        unsigned long file_size = strtol(header2.size, NULL, 8);
        
        skip_file_data(tar_fd,file_size);
    }
        
        
        

    free(pathcpy);

    *no_entries = countEntry;
    return (countEntry > 0) ? 1 : 0;

    
    
   
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



    tar_header_t header2;

    char *pathcpy = malloc(100*sizeof(char));
    strcpy(pathcpy,path);

    // Réinitialiser la position de lecture au début de l'archive
    if (lseek(tar_fd, 0, SEEK_SET) == -1) {
        perror("Erreur lors du repositionnement au début de l'archive");
        free(pathcpy);
        return -1;
    }


    // Parcourir les entrées de l'archive
    while (read(tar_fd, &header2, BLOCK_SIZE) == BLOCK_SIZE) {
        if (header2.name[0] == '\0') {
            break;  // Fin de l'archive
        }

        if (strncmp(header2.name, pathcpy, sizeof(header2.name)) == 0) {
            // Gestion des symlinks
            if (header2.typeflag == '2' || header2.typeflag == '1' ) {

                strcpy(pathcpy,header2.linkname);

                return read_file(tar_fd, pathcpy, offset, dest, len);
            }

            else{

                if (is_file(tar_fd, pathcpy) != 1 || exists(tar_fd, pathcpy) != 1) {
                    free(pathcpy);
                    return -1; 
                }

                unsigned long file_size = strtol(header2.size, NULL, 8);

                // Vérification de l'offset
                if (offset >= file_size || offset<0) {
                    free(pathcpy);
                    *len = 0;
                    return -2;  // Offset trop grand
                }


                size_t bytes_to_read = file_size - offset;


                // Lire les données

                if (bytes_to_read > *len) {
                    bytes_to_read = *len;  // Ne pas dépasser la taille du tampon
                }

                if (lseek(tar_fd, offset, SEEK_CUR) == -1) {
                    perror("Erreur lors du positionnement dans le fichier");
                    free(pathcpy);
                    return -1;
                }

                ssize_t bytes_read = read(tar_fd, dest, bytes_to_read);
                if (bytes_read == -1) {
                    perror("Erreur lors de la lecture des données");
                    free(pathcpy);
                    *len = 0;
                    return -1;
                }
            
                *len = bytes_read;

                // Retourner le nombre d'octets restants à lire
                ssize_t bytesRestant = file_size - offset - bytes_read;

                if(bytesRestant == 0){
                    free(pathcpy);
                    return 0;
                }

                free(pathcpy);
                return bytesRestant;

                   
            }


        }

        unsigned long file_size = strtol(header2.size, NULL, 8);
        if (skip_file_data(tar_fd, file_size) == -1) {
            perror("Erreur lors du saut des blocs de données");
            free(pathcpy);
            return -1;
        }
    }
    free(pathcpy);

    return -1;  
}