/*
 * F0 RMSE Calculation Utility
 *
 * This utility calculates the Root Mean Square Error (RMSE) between
 * a ground truth F0 curve and an estimated F0 curve.
 *
 * Usage: f0_rmse_calc <ground_truth_file> <estimated_file>
 *
 * File format: Two columns - time(sec) frequency(Hz)
 * Comments starting with # are ignored
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>

#define MAX_LINE_LENGTH 256
#define MAX_POINTS 10000

typedef struct {
    double time;
    double f0;
} F0Point;

typedef struct {
    F0Point* points;
    int count;
    int capacity;
} F0Curve;

/* Initialize an F0 curve */
static int f0_curve_init(F0Curve* curve) {
    curve->capacity = 1000;
    curve->points = malloc(curve->capacity * sizeof(F0Point));
    curve->count = 0;
    return curve->points ? 0 : -1;
}

/* Add a point to the F0 curve */
static int f0_curve_add_point(F0Curve* curve, double time, double f0) {
    if (curve->count >= curve->capacity) {
        curve->capacity *= 2;
        F0Point* new_points = realloc(curve->points, curve->capacity * sizeof(F0Point));
        if (!new_points) {
            return -1;
        }
        curve->points = new_points;
    }

    curve->points[curve->count].time = time;
    curve->points[curve->count].f0 = f0;
    curve->count++;
    return 0;
}

/* Free F0 curve memory */
static void f0_curve_free(F0Curve* curve) {
    if (curve->points) {
        free(curve->points);
        curve->points = NULL;
    }
    curve->count = 0;
    curve->capacity = 0;
}

/* Load F0 curve from file */
static int load_f0_curve(const char* filename, F0Curve* curve) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Error: Cannot open file '%s': %s\n", filename, strerror(errno));
        return -1;
    }

    char line[MAX_LINE_LENGTH];
    int line_number = 0;

    while (fgets(line, sizeof(line), file)) {
        line_number++;

        /* Skip comments and empty lines */
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') {
            continue;
        }

        double time, f0;
        if (sscanf(line, "%lf %lf", &time, &f0) == 2) {
            if (f0_curve_add_point(curve, time, f0) < 0) {
                fprintf(stderr, "Error: Memory allocation failed at line %d\n", line_number);
                fclose(file);
                return -1;
            }
        } else {
            fprintf(stderr, "Warning: Invalid line format at line %d: %s", line_number, line);
        }
    }

    fclose(file);

    if (curve->count == 0) {
        fprintf(stderr, "Error: No valid data points found in file '%s'\n", filename);
        return -1;
    }

    printf("Loaded %d F0 points from '%s'\n", curve->count, filename);
    return 0;
}

/* Linear interpolation to get F0 value at specific time */
static double interpolate_f0(const F0Curve* curve, double target_time) {
    if (curve->count == 0) {
        return 0.0;
    }

    /* Handle edge cases */
    if (target_time <= curve->points[0].time) {
        return curve->points[0].f0;
    }
    if (target_time >= curve->points[curve->count - 1].time) {
        return curve->points[curve->count - 1].f0;
    }

    /* Find surrounding points and interpolate */
    for (int i = 0; i < curve->count - 1; i++) {
        if (target_time >= curve->points[i].time && target_time <= curve->points[i + 1].time) {
            double t1 = curve->points[i].time;
            double t2 = curve->points[i + 1].time;
            double f1 = curve->points[i].f0;
            double f2 = curve->points[i + 1].f0;

            /* Linear interpolation */
            double alpha = (target_time - t1) / (t2 - t1);
            return f1 + alpha * (f2 - f1);
        }
    }

    return 0.0; /* Should not reach here */
}

/* Calculate F0 RMSE between two curves */
static double calculate_f0_rmse(const F0Curve* ground_truth, const F0Curve* estimated) {
    /* Determine time range */
    double min_time = fmax(ground_truth->points[0].time, estimated->points[0].time);
    double max_time = fmin(ground_truth->points[ground_truth->count - 1].time,
                          estimated->points[estimated->count - 1].time);

    if (min_time >= max_time) {
        fprintf(stderr, "Error: No overlapping time range between curves\n");
        return -1.0;
    }

    /* Calculate RMSE using fixed time step */
    double time_step = 0.01; /* 10ms intervals */
    double sum_squared_error = 0.0;
    int sample_count = 0;

    for (double t = min_time; t <= max_time; t += time_step) {
        double gt_f0 = interpolate_f0(ground_truth, t);
        double est_f0 = interpolate_f0(estimated, t);

        /* Skip unvoiced regions (F0 = 0) */
        if (gt_f0 > 0.0 && est_f0 > 0.0) {
            double error = gt_f0 - est_f0;
            sum_squared_error += error * error;
            sample_count++;
        }
    }

    if (sample_count == 0) {
        fprintf(stderr, "Error: No voiced regions found for comparison\n");
        return -1.0;
    }

    double mse = sum_squared_error / sample_count;
    double rmse = sqrt(mse);

    printf("Compared %d samples over %.3f seconds\n", sample_count, max_time - min_time);
    printf("Mean Squared Error: %.6f Hz²\n", mse);

    return rmse;
}

static void print_usage(const char* program_name) {
    printf("Usage: %s <ground_truth_file> <estimated_file>\n", program_name);
    printf("\n");
    printf("Calculate F0 Root Mean Square Error (RMSE) between two F0 curves.\n");
    printf("\n");
    printf("File format:\n");
    printf("  Each line: <time_seconds> <frequency_hz>\n");
    printf("  Comments starting with # are ignored\n");
    printf("  Example:\n");
    printf("    # Time(sec) F0(Hz)\n");
    printf("    0.0 261.63\n");
    printf("    0.1 262.45\n");
    printf("\n");
    printf("Output:\n");
    printf("  F0 RMSE value in Hz\n");
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        print_usage(argv[0]);
        return 1;
    }

    const char* ground_truth_file = argv[1];
    const char* estimated_file = argv[2];

    /* Load F0 curves */
    F0Curve ground_truth, estimated;

    if (f0_curve_init(&ground_truth) < 0) {
        fprintf(stderr, "Error: Memory allocation failed for ground truth curve\n");
        return 1;
    }

    if (f0_curve_init(&estimated) < 0) {
        fprintf(stderr, "Error: Memory allocation failed for estimated curve\n");
        f0_curve_free(&ground_truth);
        return 1;
    }

    /* Load data */
    printf("Loading ground truth F0 curve from '%s'...\n", ground_truth_file);
    if (load_f0_curve(ground_truth_file, &ground_truth) < 0) {
        f0_curve_free(&ground_truth);
        f0_curve_free(&estimated);
        return 1;
    }

    printf("Loading estimated F0 curve from '%s'...\n", estimated_file);
    if (load_f0_curve(estimated_file, &estimated) < 0) {
        f0_curve_free(&ground_truth);
        f0_curve_free(&estimated);
        return 1;
    }

    /* Calculate RMSE */
    printf("\nCalculating F0 RMSE...\n");
    double rmse = calculate_f0_rmse(&ground_truth, &estimated);

    /* Cleanup */
    f0_curve_free(&ground_truth);
    f0_curve_free(&estimated);

    if (rmse < 0.0) {
        return 1;
    }

    printf("\nF0 RMSE: %.6f Hz\n", rmse);
    return 0;
}
