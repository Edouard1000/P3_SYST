#include "lib_tar.h"
#include "string.h"
#include "stdio.h"
#define BLOCK_SIZE 512

tar_header_t header;
int skip_file_data(int tar_fd, unsigned long file_size) {
    // Calculer le nombre de blocs nécessaires pour stocker les données du fichier
    unsigned long num_blocks = (file_size + BLOCK_SIZE - 1) / BLOCK_SIZE;

    // Utiliser lseek pour sauter ces blocs
    if (lseek(tar_fd, num_blocks * BLOCK_SIZE, SEEK_CUR) == -1) {
        perror("Error while skipping file data");
        return -1;  // Erreur lors du saut
    }

    return 0;  // Succès
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

        
        unsigned long file_size = strtol(header.size, NULL, 8);

        // Sauter les blocs de données
        if (skip_file_data(tar_fd, file_size) == -1) {
            return -4;  
        }
        
        num_headers++;
    }

    return num_headers; 
}


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

        
        unsigned long file_size = strtol(header.size, NULL, 8); 
        
        if (skip_file_data(tar_fd, file_size) == -1) {
            return -4;  
        }
        
    }

    // Aucune correspondance trouvée
    return 0;
}


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
                return 1; // It is a directory
            } else {
                return 0; // Not a directory
            }
        }

       
        unsigned long file_size = strtol(header.size, NULL, 8);
        if (skip_file_data(tar_fd, file_size) == -1) {
            return -1;
        }
    }

    return 0; 
}

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
                return 1; // It is a file
            } else {
                return 0; // Not a file
            }
        }

        
        unsigned long file_size = strtol(header.size, NULL, 8);
        if (skip_file_data(tar_fd, file_size) == -1) {
            return -1;
        }
    }

    return 0; 
}