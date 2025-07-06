#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#define BLOCK_SIZE 4096

#ifdef _WIN32
    #include <windows.h>
    #include <direct.h>
    #define PATH_MAX MAX_PATH
    #define realpath(N, R) _fullpath((R), (N), PATH_MAX)
#else
    #include <limits.h>
    #include <unistd.h>
#endif

typedef struct {
    int start;
    int end;
} BlockRange;

typedef struct {
    BlockRange *ranges;
    int count;
} CommandSet;

void parse_range_string(const char *input, BlockRange **ranges_out, int *count_out) {
    char *copy = strdup(input);
    char *token = strtok(copy, ",");
    int entries[2048];  // More than enough for most transfer lists
    int count = 0;

    while (token) {
        entries[count++] = atoi(token);
        token = strtok(NULL, ",");
    }

    if (count < 1 || (count - 1) % 2 != 0 || entries[0] != (count - 1)) {
        fprintf(stderr, "Invalid range string: %s\n", input);
        free(copy);
        exit(EXIT_FAILURE);
    }

    int pairs = entries[0] / 2;
    BlockRange *ranges = malloc(sizeof(BlockRange) * pairs);
    for (int i = 0; i < pairs; i++) {
        ranges[i].start = entries[1 + 2 * i];
        ranges[i].end = entries[1 + 2 * i + 1];
    }

    *ranges_out = ranges;
    *count_out = pairs;
    free(copy);
}

void parse_transfer_list(const char *filename, int *version, int *new_blocks, CommandSet *new_data_cmds) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open transfer list");
        exit(EXIT_FAILURE);
    }

    fscanf(file, "%d", version);
    fscanf(file, "%d", new_blocks);

    if (*version >= 2) {
        char buf[256];
        fgets(buf, sizeof(buf), file); // Skip rest of line
        fgets(buf, sizeof(buf), file); // Skip another line
    }

    char line[1024];
    new_data_cmds->ranges = NULL;
    new_data_cmds->count = 0;

    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "new", 3) == 0) {
            BlockRange *ranges = NULL;
            int count = 0;
            parse_range_string(line + 4, &ranges, &count);

            new_data_cmds->ranges = realloc(new_data_cmds->ranges, sizeof(BlockRange) * (new_data_cmds->count + count));
            memcpy(&new_data_cmds->ranges[new_data_cmds->count], ranges, sizeof(BlockRange) * count);
            new_data_cmds->count += count;
            free(ranges);
        }
        // You can extend support here for "erase", "zero", etc.
    }

    fclose(file);
}

void write_output_image(const char *data_file, const char *output_file, CommandSet *commands) {
    FILE *data = fopen(data_file, "rb");
    if (!data) {
        perror("Failed to open new data file");
        exit(EXIT_FAILURE);
    }

    FILE *out = fopen(output_file, "wb");
    if (!out) {
        perror("Failed to open output image file");
        fclose(data);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < commands->count; i++) {
        int start = commands->ranges[i].start;
        int end = commands->ranges[i].end;
        int block_count = end - start;

        printf("Writing %d blocks to block %d...\n", block_count, start);
        fseek(out, (long)start * BLOCK_SIZE, SEEK_SET);

        for (int j = 0; j < block_count; j++) {
            char buffer[BLOCK_SIZE];
            size_t bytes_read = fread(buffer, 1, BLOCK_SIZE, data);
            if (bytes_read != BLOCK_SIZE) {
                fprintf(stderr, "Unexpected end of new data file.\n");
                fclose(data);
                fclose(out);
                exit(EXIT_FAILURE);
            }
            fwrite(buffer, 1, BLOCK_SIZE, out);
        }
    }

    fclose(data);
    fclose(out);
}

int main(int argc, char *argv[]) {
    if (argc < 3 || argc > 4) {
        fprintf(stderr, "Usage: %s <transfer_list> <system.new.dat> [output_image]\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *transfer_list = argv[1];
    const char *data_file = argv[2];
    const char *output_file = (argc == 4) ? argv[3] : "system.img";

    int version = 0, new_blocks = 0;
    CommandSet new_data_cmds = {0};

    parse_transfer_list(transfer_list, &version, &new_blocks, &new_data_cmds);

    printf("Transfer list version: %d\n", version);
    printf("Total new blocks: %d\n", new_blocks);

    write_output_image(data_file, output_file, &new_data_cmds);

    char resolved_path[PATH_MAX];
    if (realpath(output_file, resolved_path)) {
        printf("Image successfully written to: %s\n", resolved_path);
    } else {
        printf("Image successfully written to: %s\n", output_file);
    }

    free(new_data_cmds.ranges);
    return EXIT_SUCCESS;
}
