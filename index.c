#include "index.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <time.h>

extern int object_write(ObjectType type, const void *data, size_t len, ObjectID *id_out);

IndexEntry* index_find(Index *index, const char *path) {
    for (int i = 0; i < index->count; i++) {
        if (strcmp(index->entries[i].path, path) == 0)
            return &index->entries[i];
    }
    return NULL;
}

int index_save(const Index *index) {
    char tmp[256];
    snprintf(tmp, sizeof(tmp), "%s.tmp", INDEX_FILE);
    FILE *f = fopen(tmp, "w");
    if (!f) return -1;
    char hex[HASH_HEX_SIZE + 1];
    for (int i = 0; i < index->count; i++) {
        hash_to_hex(&index->entries[i].hash, hex);
        fprintf(f, "%o %s %llu %u %s\n",
                index->entries[i].mode, hex,
                (unsigned long long)index->entries[i].mtime_sec,
                index->entries[i].size,
                index->entries[i].path);
    }
    fflush(f);
    fsync(fileno(f));
    fclose(f);
    rename(tmp, INDEX_FILE);
    return 0;
}

int index_remove(Index *index, const char *path) {
    for (int i = 0; i < index->count; i++) {
        if (strcmp(index->entries[i].path, path) == 0) {
            int remaining = index->count - i - 1;
            if (remaining > 0)
                memmove(&index->entries[i], &index->entries[i + 1],
                        remaining * sizeof(IndexEntry));
            index->count--;
            return index_save(index);
        }
    }
    fprintf(stderr, "error: '%s' is not in the index\n", path);
    return -1;
}

int index_status(const Index *index) {
    printf("Staged changes:\n");
    int staged_count = 0;
    for (int i = 0; i < index->count; i++) {
        printf("  staged:     %s\n", index->entries[i].path);
        staged_count++;
    }
    if (staged_count == 0) printf("  (nothing to show)\n");
    printf("\n");
    printf("Unstaged changes:\n");
    int unstaged_count = 0;
    for (int i = 0; i < index->count; i++) {
        struct stat st;
        if (stat(index->entries[i].path, &st) != 0) {
            printf("  deleted:    %s\n", index->entries[i].path);
            unstaged_count++;
        } else {
            if (st.st_mtime != (time_t)index->entries[i].mtime_sec ||
                st.st_size != (off_t)index->entries[i].size) {
                printf("  modified:   %s\n", index->entries[i].path);
                unstaged_count++;
            }
        }
    }
    if (unstaged_count == 0) printf("  (nothing to show)\n");
    printf("\n");
    printf("Untracked files:\n");
    int untracked_count = 0;
    DIR *dir = opendir(".");
    if (dir) {
        struct dirent *ent;
        while ((ent = readdir(dir)) != NULL) {
            if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;
            if (strcmp(ent->d_name, ".pes") == 0) continue;
            if (strcmp(ent->d_name, "pes") == 0) continue;
            if (strstr(ent->d_name, ".o") != NULL) continue;
            int is_tracked = 0;
            for (int i = 0; i < index->count; i++) {
                if (strcmp(index->entries[i].path, ent->d_name) == 0) {
                    is_tracked = 1;
                    break;
                }
            }
            if (!is_tracked) {
                struct stat st;
                stat(ent->d_name, &st);
                if (S_ISREG(st.st_mode)) {
                    printf("  untracked:  %s\n", ent->d_name);
                    untracked_count++;
                }
            }
        }
        closedir(dir);
    }
    if (untracked_count == 0) printf("  (nothing to show)\n");
    printf("\n");
    return 0;
}

int index_load(Index *index) {
    index->count = 0;
    FILE *f = fopen(INDEX_FILE, "r");
    if (!f) return 0;
    char hex[HASH_HEX_SIZE + 1];
    while (fscanf(f, "%o %64s %llu %u %511s",
                  &index->entries[index->count].mode,
                  hex,
                  (unsigned long long*)&index->entries[index->count].mtime_sec,
                  &index->entries[index->count].size,
                  index->entries[index->count].path) == 5) {
        hex_to_hash(hex, &index->entries[index->count].hash);
        index->count++;
    }
    fclose(f);
    return 0;
}

int index_add(Index *index, const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "error: cannot open '%s'\n", path); return -1; }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);
    void *data = malloc(size);
    if (!data) { fclose(f); return -1; }
    if (fread(data, 1, size, f) != (size_t)size) { free(data); fclose(f); return -1; }
    fclose(f);
    ObjectID id;
    if (object_write(OBJ_BLOB, data, size, &id) != 0) { free(data); return -1; }
    free(data);
    struct stat st;
    if (stat(path, &st) != 0) return -1;
    IndexEntry *entry = index_find(index, path);
    if (!entry) entry = &index->entries[index->count++];
    strncpy(entry->path, path, sizeof(entry->path) - 1);
    entry->hash = id;
    entry->mtime_sec = st.st_mtime;
    entry->size = st.st_size;
    entry->mode = (st.st_mode & S_IXUSR) ? 0100755 : 0100644;
    return index_save(index);
}
