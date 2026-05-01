#include "pes.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <openssl/sha.h>

void compute_hash(const void *data, size_t len, ObjectID *id_out);
void object_path(const ObjectID *id, char *path_out, size_t path_size);
int object_exists(const ObjectID *id);

int main(void) {
    system("rm -rf .pes");
    system("mkdir -p .pes/objects .pes/refs/heads");

    const char *type_str = "blob";
    const char *data = "Hello, PES-VCS!\n";
    size_t len = strlen(data);

    char header[64];
    int header_len = snprintf(header, sizeof(header), "%s %zu", type_str, len);
    printf("header: '%s' (len=%d)\n", header, header_len);

    size_t full_len = header_len + 1 + len;
    uint8_t *full = malloc(full_len);
    memcpy(full, header, header_len);
    full[header_len] = '\0';
    memcpy(full + header_len + 1, data, len);

    ObjectID id;
    compute_hash(full, full_len, &id);

    char hex[65];
    hash_to_hex(&id, hex);
    printf("hash: %s\n", hex);

    printf("exists: %d\n", object_exists(&id));

    char shard_dir[256];
    snprintf(shard_dir, sizeof(shard_dir), "%s/%.2s", OBJECTS_DIR, hex);
    printf("shard_dir: %s\n", shard_dir);
    int r = mkdir(shard_dir, 0755);
    printf("mkdir returned: %d\n", r);

    char final_path[512];
    object_path(&id, final_path, sizeof(final_path));
    printf("final_path: %s\n", final_path);

    char tmp_path[560];
    snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", final_path);
    printf("tmp_path: %s\n", tmp_path);

    int fd = open(tmp_path, O_CREAT | O_WRONLY | O_TRUNC, 0444);
    printf("open fd: %d\n", fd);
    if (fd < 0) { perror("open failed"); free(full); return 1; }

    ssize_t written = write(fd, full, full_len);
    printf("written: %zd / %zu\n", written, full_len);
    free(full);

    fsync(fd);
    close(fd);

    r = rename(tmp_path, final_path);
    printf("rename returned: %d\n", r);

    return 0;
}
