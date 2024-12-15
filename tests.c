#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "lib_tar.h"

/**
 * You are free to use this file to write tests for your implementation
 */

void debug_dump(const uint8_t *bytes, size_t len) {
    for (int i = 0; i < len;) {
        printf("%04x:  ", (int) i);

        for (int j = 0; j < 16 && i + j < len; j++) {
            printf("%02x ", bytes[i + j]);
        }
        printf("\t");
        for (int j = 0; j < 16 && i < len; j++, i++) {
            printf("%c ", bytes[i]);
        }
        printf("\n");
    }
}
void test_exists(int fd, const char *path) {
    int result = exists(fd, (char *)path);
    if (result == 1) {
        printf("Le chemin '%s' existe dans l'archive.\n", path);
    } else if (result == 0) {
        printf("Le chemin '%s' n'existe pas dans l'archive.\n", path);
    } else {
        printf("Erreur lors de la vérification de l'existence de '%s' dans l'archive.\n", path);
    }
}
void test_is_dir(int fd, const char *path) {
    int result = is_dir(fd, (char *)path);
    if (result == 1) {
        printf("Le chemin '%s' est un répertoire dans l'archive.\n", path);
    } else if (result == 0) {
        printf("Le chemin '%s' n'est pas un répertoire ou n'existe pas dans l'archive.\n", path);
    } else {
        printf("Erreur lors de la vérification de '%s' comme répertoire dans l'archive.\n", path);
    }
}
void test_is_file(int fd, const char *path) {
    int result = is_file(fd, (char *)path);
    if (result == 1) {
        printf("Le chemin '%s' est un fichier dans l'archive.\n", path);
    } else if (result == 0) {
        printf("Le chemin '%s' n'est pas un fichier ou n'existe pas dans l'archive.\n", path);
    } else {
        printf("Erreur lors de la vérification de '%s' comme fichier dans l'archive.\n", path);
    }
}
void test_is_symlink(int fd, const char *path) {
    int result = is_symlink(fd, (char *)path);
    if (result == 1) {
        printf("Le chemin '%s' est un lien symbolique dans l'archive.\n", path);
    } else if (result == 0) {
        printf("Le chemin '%s' n'est pas un lien symbolique ou n'existe pas dans l'archive.\n", path);
    } else {
        printf("Erreur lors de la vérification de '%s' comme lien symbolique dans l'archive.\n", path);
    }
}
void test_list(int fd, const char *path) {
    size_t max_entries = 10; // Nombre maximum d'entrées à lister
    char **entries = malloc(max_entries * sizeof(char *));

    // Allouer de la mémoire pour chaque entrée
    for (size_t i = 0; i < max_entries; i++) {
        entries[i] = malloc(100 * sizeof(char));
    }

    size_t no_entries = max_entries;

    int result = list(fd, (char *)path, entries, &no_entries);

    if (result > 0) {
        printf("Liste des entrées pour '%s' :\n", path);
        for (size_t i = 0; i < no_entries; i++) {
            printf("  - %s\n", entries[i]);
        }
    } else if (result == 0) {
        printf("Aucune entrée trouvée pour '%s'.\n", path);
    } else {
        printf("Erreur lors de l'exécution de la fonction list pour '%s'.\n", path);
    }

    // Libérer la mémoire allouée
    for (size_t i = 0; i < max_entries; i++) {
        free(entries[i]);
    }
    free(entries);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: %s tar_file\n", argv[0]);
        return -1;
    }

    int fd = open(argv[1] , O_RDONLY);
    if (fd == -1) {
        perror("open(tar_file)");
        return -1;
    }

    int ret = check_archive(fd);
    printf("check_archive returned %d\n", ret);

    // Tester la fonction `exists`
    printf("Test de la fonction exists :\n");
    test_exists(fd, "file1.txt");     // Test d'un fichier existant
    test_exists(fd, "nonexistent");  // Test d'un chemin inexistant
    test_exists(fd, "dir/");         // Test d'un répertoire existant
    test_exists(fd, "link_to_file"); // Test d'un lien symbolique

    // Tester la fonction `is_dir`
    printf("\nTest de la fonction is_dir :\n");
    test_is_dir(fd, "dir/");         // Test d'un répertoire existant
    test_is_dir(fd, "file1.txt");    // Test d'un fichier (pas un répertoire)
    test_is_dir(fd, "nonexistent");  // Test d'un chemin inexistant

    // Tester la fonction `is_file`
    printf("\nTest de la fonction is_file :\n");
    test_is_file(fd, "file1.txt");    // Test d'un fichier existant
    test_is_file(fd, "dir/");         // Test d'un répertoire (pas un fichier)
    test_is_file(fd, "nonexistent");  // Test d'un chemin inexistant

     // Tester la fonction `is_symlink`
    printf("\nTest de la fonction is_symlink :\n");
    test_is_symlink(fd, "link_to_file"); // Test d'un lien symbolique existant
    test_is_symlink(fd, "file1.txt");   // Test d'un fichier (pas un lien symbolique)
    test_is_symlink(fd, "nonexistent"); // Test d'un chemin inexistant

    printf("Test de la fonction list :\n");
    test_list(fd, "dir/");   // Tester avec un répertoire
    test_list(fd, "dir/c/"); // Tester avec un sous-répertoire
    test_list(fd, "dir/a");  // Tester avec un fichier unique
    test_list(fd, "unknown"); // Tester avec un chemin inexistant

    // Fermer le descripteur de fichier
    close(fd);
    return 0;
}