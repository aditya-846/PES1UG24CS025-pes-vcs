#include "pes.h"
#include "index.h"
#include "commit.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

void cmd_init(void) {
    mkdir(PES_DIR, 0755);
    mkdir(OBJECTS_DIR, 0755);
    mkdir(".pes/refs", 0755);
    mkdir(REFS_DIR, 0755);
    FILE *f = fopen(HEAD_FILE, "w");
    if (f) { fprintf(f, "ref: refs/heads/main\n"); fclose(f); }
    printf("Initialized empty PES repository in %s/\n", PES_DIR);
}

void cmd_add(int argc, char *argv[]) {
    if (argc < 3) { fprintf(stderr, "Usage: pes add <file>...\n"); return; }
    static Index index;
    if (index_load(&index) != 0) { fprintf(stderr, "error: failed to load index\n"); return; }
    for (int i = 2; i < argc; i++) {
        if (index_add(&index, argv[i]) == 0)
            printf("staged: %s\n", argv[i]);
        else
            fprintf(stderr, "error: failed to add '%s'\n", argv[i]);
    }
}

void cmd_status(void) {
    static Index index;
    if (index_load(&index) != 0) { fprintf(stderr, "error: failed to load index\n"); return; }
    index_status(&index);
}

void cmd_commit(int argc, char *argv[]) {
    const char *message = NULL;
    for (int i = 2; i < argc - 1; i++) {
        if (strcmp(argv[i], "-m") == 0) { message = argv[i + 1]; break; }
    }
    if (!message) {
        fprintf(stderr, "error: commit requires a message (-m \"message\")\n");
        return;
    }
    ObjectID commit_id;
    if (commit_create(message, &commit_id) == 0) {
        char hex[HASH_HEX_SIZE + 1];
        hash_to_hex(&commit_id, hex);
        printf("Committed: %.12s... %s\n", hex, message);
    } else {
        fprintf(stderr, "error: commit failed\n");
    }
}

static void log_callback(const ObjectID *id, const Commit *commit, void *ctx) {
    (void)ctx;
    char hex[HASH_HEX_SIZE + 1];
    hash_to_hex(id, hex);
    time_t ts = (time_t)commit->timestamp;
    char time_str[64];
    struct tm *tm_info = localtime(&ts);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
    printf("commit %s\n", hex);
    printf("Author: %s\n", commit->author);
    printf("Date:   %s\n", time_str);
    printf("\n    %s\n\n", commit->message);
}

void cmd_log(void) {
    if (commit_walk(log_callback, NULL) != 0)
        fprintf(stderr, "No commits yet.\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: pes <command> [args]\n");
        return 1;
    }
    const char *cmd = argv[1];
    if      (strcmp(cmd, "init")   == 0) cmd_init();
    else if (strcmp(cmd, "add")    == 0) cmd_add(argc, argv);
    else if (strcmp(cmd, "status") == 0) cmd_status();
    else if (strcmp(cmd, "commit") == 0) cmd_commit(argc, argv);
    else if (strcmp(cmd, "log")    == 0) cmd_log();
    else { fprintf(stderr, "Unknown command: %s\n", cmd); return 1; }
    return 0;
}
