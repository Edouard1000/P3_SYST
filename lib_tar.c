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
           
            if (header.typeflag == SYMTYPE) {
                return 1; // It is a symlink
            } else {
                return 0; // Not a symlink
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
        return -1; // Erreur critique
    }

    tar_header_t header;
    size_t entry_count = 0;

    while (read(tar_fd, &header, sizeof(header)) == sizeof(header)) {
        
        if (header.name[0] == '\0') {
            break;
        }

        // Vérifier si l'entrée commence par le chemin donné
        if (strncmp(header.name, path, strlen(path)) == 0) {
            
            const char *subpath = header.name + strlen(path);

            // Si c'est un symlink, récupérer sa destination
            if (header.typeflag == SYMTYPE) {
                char resolved_path[100];
                strncpy(resolved_path, header.linkname, sizeof(resolved_path) - 1);
                resolved_path[sizeof(resolved_path) - 1] = '\0'; // Assurez la terminaison nulle


                // Vérifier si le symlink pointe vers un répertoire
                if (resolved_path[strlen(resolved_path) - 1] != '/') {
                    strncat(resolved_path, "/", sizeof(resolved_path) - strlen(resolved_path) - 1);
                }

                printf("Le chemin %s est un symlink vers %s\n", header.name, resolved_path);

                // Ajuster pour lister les entrées dans la destination du symlink
                return list(tar_fd, resolved_path, entries, no_entries);
            }

            // Si subpath ne contient pas de '/' ou '/' est a la fin path
            if (strchr(subpath, '/') == NULL || strchr(subpath, '/') == subpath + strlen(subpath) - 1) {
                
                if (entry_count < *no_entries) {
                    strncpy(entries[entry_count], header.name, 100);
                    entry_count++;
                } else {
                    fprintf(stderr, "La liste des entrées est pleine. Augmentez `no_entries`\n");
                    break;
                }
            }
        }

        
        unsigned long file_size = strtol(header.size, NULL, 8);
        if (skip_file_data(tar_fd, file_size) == -1) {
            perror("Erreur lors du saut des blocs de données");
            return 0;
        }
    }

    // Mettre à jour le nombre d'entrées listées
    *no_entries = entry_count;

    if (entry_count > 0) {
        return 1;
    } else {
        return 0;
    }
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

    if (is_file(tar_fd, path) != 1) {
        return -1; 
    }

    
    if (lseek(tar_fd, 0, SEEK_SET) == -1) {
        perror("Erreur lors du repositionnement au début de l'archive");
        return -1;
    }

    
    while (read(tar_fd, &header, sizeof(header)) == sizeof(header)) {

        if (header.name[0] == '\0') {
            break;
        }

        
        if (strncmp(header.name, path, sizeof(header.name)) == 0) {
            
            unsigned long file_size = strtol(header.size, NULL, 8);

            
            if (offset >= file_size) {
                return -2; 
            }

            // data_offset = la position courante = juste après l'en-tête
            unsigned long data_offset = lseek(tar_fd, 0, SEEK_CUR);
            if (data_offset == (unsigned long)-1) {
                perror("Erreur lors du calcul de l'offset des données");
                return -1;
            }

            data_offset += BLOCK_SIZE; 
            if (lseek(tar_fd, data_offset + offset, SEEK_SET) == -1) {
                perror("Erreur lors du positionnement dans le fichier");
                return -1;
            }

            // Lire les données
            size_t bytes_to_read = file_size - offset;
            if (bytes_to_read > *len) {
                bytes_to_read = *len; // Ne pas dépasser la taille du tampon
            }

            ssize_t bytes_read = read(tar_fd, dest, bytes_to_read);
            if (bytes_read == -1) {
                perror("Erreur lors de la lecture des données");
                return -1;
            }

            *len = bytes_read;

            
            ssize_t remaining_bytes = file_size - offset - bytes_read;

            if (remaining_bytes > 0) {
                return remaining_bytes;
            } else {
                return 0;
            }
        }
        unsigned long file_size = strtol(header.size, NULL, 8);
        if (skip_file_data(tar_fd, file_size) == -1) {
            perror("Erreur lors du saut des blocs de données");
            return -1;
        }
    }

    return -1; 
}