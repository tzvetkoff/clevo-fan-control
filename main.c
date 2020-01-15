#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/io.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <getopt.h>
#include <errno.h>

#define VERSION             "0.1.0"

// XXXXXX
#define EC_REG_PATH         "/sys/kernel/debug/ec/ec0/io"
#define EC_REG_SIZE         0x0100



#define IBF 1
#define OBF 0

#define EC_SC               0x66
#define EC_DATA             0x62

#define EC_SC_READ_CMD      0x80
#define EC_SC_WRITE_CMD     0x99

#define EC_REG_CPU_TEMP     0x07
#define EC_REG_GPU_TEMP     0xcd
#define EC_REG_FAN_DUTY     0xce
#define EC_REG_FAN_DUTY_W   0x01
#define EC_REG_FAN_RPMS_HI  0xd0
#define EC_REG_FAN_RPMS_LO  0xd1

struct ec_data_s {
    int cpu_temp;
    int gpu_temp;
    int fan_duty;
    int fan_rpms;
};
typedef struct ec_data_s ec_data_t;

static inline int
clevo_ec_init()
{
    int n = 0;

    if ((n = ioperm(EC_DATA, 1, 1)) != 0) {
        return n;
    }

    if ((n = ioperm(EC_SC, 1, 1) != 0) != 0) {
        return n;
    }

    return 0;
}

static inline int
clevo_ec_wait(const uint32_t port, const uint32_t flag, const char value)
{
    uint8_t data = 0;
    int n = 0;

    data = inb(port);
    while (((data >> flag) & 0x01) != value && n++ < 100) {
        usleep(1000);
        data = inb(port);
    }

    if (n >= 100) {
        return n;
    }

    return 0;
}

static inline int
clevo_ec_read(const uint32_t port, uint8_t *value)
{
    clevo_ec_wait(EC_SC, IBF, 0);
    outb(EC_SC_READ_CMD, EC_SC);

    clevo_ec_wait(EC_SC, IBF, 0);
    outb(port, EC_DATA);

    clevo_ec_wait(EC_SC, OBF, 1);
    *value = inb(EC_DATA);

    return 0;
}

static inline int
clevo_ec_write(const uint32_t port, const uint8_t value)
{
    clevo_ec_wait(EC_SC, IBF, 0);
    outb(EC_SC_WRITE_CMD, EC_SC);

    clevo_ec_wait(EC_SC, IBF, 0);
    outb(port, EC_DATA);

    clevo_ec_wait(EC_SC, IBF, 0);
    outb(value, EC_DATA);

    return clevo_ec_wait(EC_SC, IBF, 0);
}

static inline void
usage_get(const char *prog_name, FILE *out)
{
    fputs("\n", out);
    fputs("Usage:\n", out);
    fprintf(out, "  %s get [options]\n", prog_name);

    fputs("\n", out);
    fputs("Options:\n", out);
    fputs("  -h, --help          Show this message\n", out);
    fputs("  -v, --version       Show version information\n", out);
    fputs("  -b, --bare          Bare output\n", out);
    fputs("  -c, --cpu-temp      Only print CPU temperature\n", out);
    fputs("  -g, --cpu-temp      Only print GPU temperature\n", out);
    fputs("  -d, --fan-duty      Only print FAN duty\n", out);
    fputs("  -r, --fan-rpms      Only print FAN speed\n", out);

    fputs("\n", out);
    exit(out == stderr ? -1 : 0);
}

static inline int
cmd_get(const char *prog_name, int argc, const char **argv)
{
    int opt_bare = 0, opt_all = 1, opt_cpu_temp = 0, opt_gpu_temp = 0,
        opt_fan_duty = 0, opt_fan_rpms = 0, ch = 0, n = 0;
    uint8_t hi = 0, lo = 0;

    const struct option longopts[] = {
        {"bare", no_argument, NULL, 'b'},
        {"cpu-temp", no_argument, NULL, 'c'},
        {"gpu-temp", no_argument, NULL, 'g'},
        {"fan-duty", no_argument, NULL, 'd'},
        {"fan-rpms", no_argument, NULL, 'r'},
        {"help", no_argument, NULL, 'h'},
        {"version", no_argument, NULL, 'v'},
        {NULL, 0, NULL, 0}
    };

    while ((ch = getopt_long(argc, (char *const *)argv, "bcgdrhv", longopts,
        NULL)) != -1) {
        switch (ch) {
            case 'b':
                opt_bare = 1;
                break;
            case 'c':
                opt_all = 0;
                opt_cpu_temp = 1;
                break;
            case 'g':
                opt_all = 0;
                opt_gpu_temp = 1;
                break;
            case 'd':
                opt_all = 0;
                opt_fan_duty = 1;
                break;
            case 'r':
                opt_all = 0;
                opt_fan_rpms = 1;
                break;
            case 'h':
                usage_get(prog_name, stdout);
                break;
            case 'v':
                puts(VERSION);
                return 0;
            default:
                usage_get(prog_name, stderr);
        }
    }

    if (opt_all) {
        opt_cpu_temp = opt_gpu_temp = opt_fan_duty = opt_fan_rpms = 1;
    }

    if ((n = clevo_ec_init()) != 0) {
        fprintf(stderr, "%s: get: clevo_ec_init: %s\n",
            prog_name, strerror(errno));
        return n;
    }

    if (opt_cpu_temp) {
        if ((n = clevo_ec_read(EC_REG_CPU_TEMP, &lo) != 0)) {
            fprintf(stderr, "%s: get: clevo_ec_read: %s\n",
                prog_name, strerror(errno));
            return n;
        }
        if (!opt_bare) {
            fputs("cpu-temp: ", stdout);
        }
        printf("%d\n", lo);
    }

    if (opt_gpu_temp) {
        if ((n = clevo_ec_read(EC_REG_GPU_TEMP, &lo) != 0)) {
            fprintf(stderr, "%s: get: clevo_ec_read: %s\n",
                prog_name, strerror(errno));
            return n;
        }
        if (!opt_bare) {
            fputs("gpu-temp: ", stdout);
        }
        printf("%d\n", lo);
    }

    if (opt_fan_duty) {
        if ((n = clevo_ec_read(EC_REG_FAN_DUTY, &lo) != 0)) {
            fprintf(stderr, "%s: get: clevo_ec_read: %s\n",
                prog_name, strerror(errno));
            return n;
        }
        if (!opt_bare) {
            fputs("fan-duty: ", stdout);
        }
        lo = (uint8_t)(((double)lo * 100.0) / 255.0);
        printf("%d\n", lo);
    }

    if (opt_fan_rpms) {
        if ((n = clevo_ec_read(EC_REG_FAN_RPMS_HI, &hi)) != 0) {
            fprintf(stderr, "%s: get: clevo_ec_read: %s\n",
                prog_name, strerror(errno));
            return n;
        }
        if ((n = clevo_ec_read(EC_REG_FAN_RPMS_LO, &lo)) != 0) {
            fprintf(stderr, "%s: get: clevo_ec_read: %s\n",
                prog_name, strerror(errno));
            return n;
        }
        if (!opt_bare) {
            fputs("fan-rpms: ", stdout);
        }
        n = (hi << 8) | lo;
        n = n > 0 ? 2156220 / n : 0;
        printf("%d\n", n);
    }

    return 0;
}

static inline void
usage_set(const char *prog_name, FILE *out)
{
    fputs("\n", out);
    fputs("Usage:\n", out);
    fprintf(out, "  %s set [options]\n", prog_name);

    fputs("\n", out);
    fputs("Options:\n", out);
    fputs("  -h, --help          Show this message\n", out);
    fputs("  -v, --version       Show version information\n", out);
    fputs("  -d, --fan-duty=N    Set FAN duty\n", out);

    fputs("\n", out);
    exit(out == stderr ? -1 : 0);
}

static inline int
cmd_set(const char *prog_name, int argc, const char **argv)
{
    int opt_fan_duty = 0, val_fan_duty = 0, ch = 0, n = 0;
    uint8_t lo = 0;
    char *p = NULL;

    const struct option longopts[] = {
        /* {"cpu-temp", required_argument, NULL, 'c'}, */
        /* {"gpu-temp", required_argument, NULL, 'g'}, */
        {"fan-duty", required_argument, NULL, 'd'},
        /* {"fan-rpms", required_argument, NULL, 'r'}, */
        {"help", no_argument, NULL, 'h'},
        {"version", no_argument, NULL, 'v'},
        {NULL, 0, NULL, 0}
    };

    while ((ch = getopt_long(argc, (char *const *)argv, "d:hv", longopts,
        NULL)) != -1) {
        switch (ch) {
            case 'd':
                val_fan_duty = (int)strtol(optarg, &p, 10);
                if (p == optarg) {
                    fprintf(stderr, "%s: set: Invalid duty: %s\n",
                        prog_name, optarg);
                    return -1;
                }
                if (val_fan_duty < 0 || val_fan_duty > 100) {
                    fprintf(stderr, "%s: set: Invalid duty: %s. "
                        "Must be between 0 and 100\n",
                        prog_name, optarg);
                    return -1;
                }
                opt_fan_duty = 1;
                break;
            case 'h':
                usage_set(prog_name, stdout);
                break;
            case 'v':
                puts(VERSION);
                return 0;
            default:
                usage_set(prog_name, stderr);
        }
    }

    if ((n = clevo_ec_init()) != 0) {
        fprintf(stderr, "%s: get: clevo_ec_init: %s\n",
            prog_name, strerror(errno));
        return n;
    }

    if (opt_fan_duty) {
        lo = (uint8_t)((val_fan_duty * 255.0) / 100.0);
        if ((n = clevo_ec_write(EC_REG_FAN_DUTY_W, lo)) != 0) {
            fprintf(stderr, "%s: get: clevo_ec_write: %s\n",
                prog_name, strerror(errno));
            return n;
        }
    }

    return 0;
}

static inline void
usage_main(const char *prog_name, FILE *out)
{
    fputs("\n", out);
    fputs("Usage:\n", out);
    fprintf(out, "  %s <command> [options]\n", prog_name);

    fputs("\n", out);
    fputs("Commands:\n", out);
    fputs("  get                 Get fan info\n", out);
    fputs("  set                 Set fan info\n", out);
    fputs("  help                Show this message\n", out);
    fputs("  version             Show version information\n", out);

    fputs("\n", out);
    exit(out == stderr ? -1 : 0);
}

int
main(int argc, const char **argv)
{
    if (argc == 1 ||
        strcmp(argv[1], "-h") == 0 ||\
        strcmp(argv[1], "--help") == 0 ||
        strcmp(argv[1], "help") == 0) {
        usage_main(argv[0], stdout);
    }
    if (strcmp(argv[1], "-v") == 0 ||\
        strcmp(argv[1], "--version") == 0 ||
        strcmp(argv[1], "version") == 0) {
        puts(VERSION);
        return 0;
    }

    if (strcmp(argv[1], "get") == 0) {
        return cmd_get(argv[0], argc - 1, argv + 1);
    }
    if (strcmp(argv[1], "set") == 0) {
        return cmd_set(argv[0], argc - 1, argv + 1);
    }

    fprintf(stderr, "%s: unknown command: %s\n", argv[0], argv[1]);
    usage_main(argv[0], stderr);
    return -1;
}
