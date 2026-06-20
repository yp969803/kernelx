#include "shell.h"
#include "../drivers/keyboard.h"
#include "../drivers/vga.h"
#include "../fs/fat.h"
#include "../stdlib/stdio.h"
#include "../stdlib/string.h"
#include "kmalloc.h"
#include "mem.h"
#include "task.h"
#include "utils.h"
#include <stddef.h>

typedef struct {
    char *cmd;
    char *arg1;
    char *rest;
} shell_line;

typedef void (*shell_command_fn)(shell_line *line);

typedef struct {
    const char *name;
    const char *usage;
    shell_command_fn fn;
} shell_command;

static void cmd_help(shell_line *line);
static void cmd_clear(shell_line *line);
static void cmd_mkdir(shell_line *line);
static void cmd_touch(shell_line *line);
static void cmd_rm(shell_line *line);
static void cmd_write(shell_line *line);
static void cmd_cat(shell_line *line);
static void cmd_ls(shell_line *line);

static shell_command commands[] = {
    {"help", "help", cmd_help},
    {"clear", "clear", cmd_clear},
    {"mkdir", "mkdir <path>", cmd_mkdir},
    {"touch", "touch <path>", cmd_touch},
    {"rm", "rm <path>", cmd_rm},
    {"write", "write <path> <text>", cmd_write},
    {"cat", "cat <path>", cmd_cat},
    {"ls", "ls <path>", cmd_ls},
};

#define COMMAND_COUNT (sizeof(commands) / sizeof(commands[0]))

static char *skip_spaces(char *s)
{
    while (*s == ' ') {
        s++;
    }
    return s;
}

static char *next_token(char **cursor)
{
    char *s = skip_spaces(*cursor);
    if (*s == '\0') {
        *cursor = s;
        return NULL;
    }

    char *start = s;
    while (*s != '\0' && *s != ' ') {
        s++;
    }

    if (*s == ' ') {
        *s = '\0';
        s++;
    }

    *cursor = s;
    return start;
}

static shell_line parse_line(char *line)
{
    shell_line parsed;
    char *cursor = line;

    parsed.cmd  = next_token(&cursor);
    parsed.arg1 = next_token(&cursor);
    parsed.rest = skip_spaces(cursor);

    if (parsed.rest && parsed.rest[0] == '\0') {
        parsed.rest = NULL;
    }

    return parsed;
}

static bool path_valid(const char *path)
{
    return path && path[0] == '/' && path[1] != '\0';
}

static fat_directory_entry_t *lookup_entry(const char *path, char dir_name[11],
                                           uint16_t *parent_cluster)
{
    mem_set(dir_name, ' ', 11);
    *parent_cluster = ROOT_CLUSTER;
    return fat_lookup(path, dir_name, parent_cluster);
}

static bool path_exists(const char *path)
{
    char dir_name[11];
    uint16_t parent_cluster;
    fat_directory_entry_t *entry = lookup_entry(path, dir_name, &parent_cluster);
    if (!entry) {
        return false;
    }

    kfree(entry);
    return true;
}

static int shell_write_file(const char *path, const uint8_t *data, uint32_t size)
{
    fat_rm_entry(path);

    if (mkfile_fat(path) != OK) {
        return ERR;
    }

    char dir_name[11];
    uint16_t parent_cluster;
    fat_directory_entry_t *entry = lookup_entry(path, dir_name, &parent_cluster);
    if (!entry) {
        return ERR;
    }

    uint16_t file_cluster = entry->first_cluster_low;
    if (size > 0 && fat_write_file(&file_cluster, data, size) != OK) {
        kfree(entry);
        return ERR;
    }

    entry->first_cluster_low = file_cluster;
    entry->file_size         = size;
    if (fat_update_dir_entry(parent_cluster, dir_name, entry) != OK) {
        kfree(entry);
        return ERR;
    }

    kfree(entry);
    return OK;
}

static int shell_cat_file(const char *path)
{
    char dir_name[11];
    uint16_t parent_cluster;
    fat_directory_entry_t *entry = lookup_entry(path, dir_name, &parent_cluster);
    if (!entry || entry->attr == ATTR_DIRECTORY) {
        if (entry) {
            kfree(entry);
        }
        return ERR;
    }

    uint8_t *buffer = kmalloc(entry->file_size + 1);
    if (!buffer) {
        kfree(entry);
        return ERR;
    }

    if (entry->file_size > 0) {
        uint16_t cluster = entry->first_cluster_low;
        if (fat_read_file(&cluster, buffer, entry->file_size) != OK) {
            kfree(buffer);
            kfree(entry);
            return ERR;
        }
    }

    buffer[entry->file_size] = '\0';
    kprintf("%s\n", buffer);

    kfree(buffer);
    kfree(entry);
    return OK;
}

static void format_fat_name(const fat_directory_entry_t *entry, char *buffer, uint32_t max)
{
    uint32_t pos = 0;
    uint32_t name_end = 8;
    uint32_t ext_end  = 11;

    while (name_end > 0 && entry->name[name_end - 1] == ' ') {
        name_end--;
    }
    while (ext_end > 8 && entry->name[ext_end - 1] == ' ') {
        ext_end--;
    }

    for (uint32_t i = 0; i < name_end && pos < max - 1; i++) {
        buffer[pos++] = entry->name[i];
    }

    if (ext_end > 8 && pos < max - 1) {
        buffer[pos++] = '.';
        for (uint32_t i = 8; i < ext_end && pos < max - 1; i++) {
            buffer[pos++] = entry->name[i];
        }
    }

    if (entry->attr == ATTR_DIRECTORY && pos < max - 1) {
        buffer[pos++] = '/';
    }

    buffer[pos] = '\0';
}

static void cmd_help(shell_line *line)
{
    (void)line;

    for (uint32_t i = 0; i < COMMAND_COUNT; i++) {
        kprintf("%s\n", commands[i].usage);
    }
}

static void cmd_clear(shell_line *line)
{
    (void)line;
    clear_screen();
}

static void cmd_mkdir(shell_line *line)
{
    if (!path_valid(line->arg1)) {
        kprintf("mkdir: missing operand\n");
        return;
    }

    if (mkdir_fat(line->arg1) != OK) {
        kprintf("mkdir: %s: failed\n", line->arg1);
        return;
    }

    kprintf("ok\n");
}

static void cmd_touch(shell_line *line)
{
    if (!path_valid(line->arg1)) {
        kprintf("touch: missing file operand\n");
        return;
    }

    if (path_exists(line->arg1)) {
        kprintf("ok\n");
        return;
    }

    if (mkfile_fat(line->arg1) != OK) {
        kprintf("touch: %s: failed\n", line->arg1);
        return;
    }

    kprintf("ok\n");
}

static void cmd_rm(shell_line *line)
{
    if (!path_valid(line->arg1)) {
        kprintf("rm: missing operand\n");
        return;
    }

    if (fat_rm_entry(line->arg1) != OK) {
        kprintf("rm: %s: failed\n", line->arg1);
        return;
    }

    kprintf("ok\n");
}

static void cmd_write(shell_line *line)
{
    if (!path_valid(line->arg1)) {
        kprintf("write: missing file operand\n");
        return;
    }

    if (!line->rest) {
        kprintf("write: missing text operand\n");
        return;
    }

    if (shell_write_file(line->arg1, (const uint8_t *)line->rest, strlen(line->rest)) != OK) {
        kprintf("write: %s: failed\n", line->arg1);
        return;
    }

    kprintf("ok\n");
}

static void cmd_cat(shell_line *line)
{
    if (!path_valid(line->arg1)) {
        kprintf("cat: missing file operand\n");
        return;
    }

    if (shell_cat_file(line->arg1) != OK) {
        kprintf("cat: %s: failed\n", line->arg1);
    }
}

static void cmd_ls(shell_line *line)
{
    const char *path = line->arg1 ? line->arg1 : "/";
    if (!path_valid(path) && !(path[0] == '/' && path[1] == '\0')) {
        kprintf("ls: missing operand\n");
        return;
    }

    fat_directory_entry_t entries[64];
    uint32_t count = 0;

    if (fat_list_dir(path, entries, 64, &count) != OK) {
        kprintf("ls: %s: failed\n", path);
        return;
    }

    for (uint32_t i = 0; i < count; i++) {
        char name[16];
        format_fat_name(&entries[i], name, sizeof(name));
        kprintf("%s\n", name);
    }
}

static void shell_execute(char *line)
{
    shell_line parsed = parse_line(line);
    if (!parsed.cmd) {
        return;
    }

    for (uint32_t i = 0; i < COMMAND_COUNT; i++) {
        if (strcmp(parsed.cmd, commands[i].name) == 0) {
            commands[i].fn(&parsed);
            return;
        }
    }

    kprintf("%s: command not found\n", parsed.cmd);
}

void *shell_task(void *args)
{
    (void)args;

    char line[KEYBOARD_LINE_MAX];

    while (1) {
        kprintf("kernelx> ");
        if (keyboard_read_line(line, sizeof(line)) >= 0) {
            shell_execute(line);
        }
    }

    return NULL;
}
