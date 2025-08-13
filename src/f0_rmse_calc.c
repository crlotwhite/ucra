/**
 * @file f0_rmse_calc.c
 * @brief F0 Root Mean Square Error (RMSE) Calculation Utility
 * 
 * This utility calculates the F0 RMSE between a ground truth F0 curve
 * and an estimated F0 curve, performing time alignment via linear interpolation.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>

#ifdef _WIN32
#include <io.h>
#define access(path, mode) _access(path, mode)
#define F_OK 0
#else
#include <unistd.h>
#endif

/**
 * @brief F0 curve structure
 */
typedef struct {
    double* time_sec;           // Time values in seconds
    double* f0_hz;              // F0 values in Hz
    size_t length;              // Number of points
} F0Curve;

/**
 * @brief F0 RMSE calculation result
 */
typedef struct {
    double rmse_hz;             // RMSE in Hz
    double rmse_cents;          // RMSE in cents
    double mean_error_hz;       // Mean absolute error in Hz
    double max_error_hz;        // Maximum absolute error in Hz
    size_t num_points;          // Number of comparison points
    size_t voiced_points;       // Number of voiced comparison points
    char* error_message;        // Error message if calculation failed
} F0RMSEResult;

/* Forward declarations */
static int file_exists(const char* path);
static int load_f0_curve(const char* filename, F0Curve* curve);
static void cleanup_f0_curve(F0Curve* curve);
static double interpolate_f0(const F0Curve* curve, double time);
static int calculate_f0_rmse(const F0Curve* ground_truth, const F0Curve* estimated, 
                            F0RMSEResult* result);
static void print_usage(const char* program_name);
static double hz_to_cents(double f0_hz, double reference_hz);
static int is_voiced(double f0_hz);

/**
 * @brief Print usage information
 */
static void print_usage(const char* program_name) {
    printf("UCRA F0 RMSE Calculation Utility\n");
    printf("Usage: %s <ground_truth_f0> <estimated_f0> [options]\n", program_name);
    printf("\nArguments:\n");
    printf("  ground_truth_f0         Path to ground truth F0 curve file\n");
    printf("  estimated_f0           Path to estimated F0 curve file\n");
    printf("\nOptions:\n");
    printf("  --min-time TIME        Start time for comparison (default: 0.0)\n");
    printf("  --max-time TIME        End time for comparison (default: auto)\n");
    printf("  --step-size SIZE       Time step for comparison (default: 0.01s)\n");
    printf("  --voiced-only          Only compare voiced frames (F0 > 0)\n");
    printf("  --verbose              Enable verbose output\n");
    printf("  -h, --help             Show this help message\n");
    printf("\nF0 File Format:\n");
    printf("  Each line: <time_seconds> <f0_hz>\n");
    printf("  Lines starting with # are ignored\n");
    printf("  F0 = 0 indicates unvoiced frame\n");
    printf("\nReturn codes:\n");
    printf("  0: Calculation successful\n");
    printf("  1: Error occurred during calculation\n");
}

/**
 * @brief Check if a file exists
 */
static int file_exists(const char* path) {
    return access(path, F_OK) == 0;
}

/**
 * @brief Check if F0 value indicates voiced frame
 */
static int is_voiced(double f0_hz) {
    return f0_hz > 0.0;
}

/**
 * @brief Convert Hz to cents relative to reference frequency
 */
static double hz_to_cents(double f0_hz, double reference_hz) {
    if (f0_hz <= 0.0 || reference_hz <= 0.0) {
        return 0.0;
    }
    return 1200.0 * log2(f0_hz / reference_hz);
}

/**
 * @brief Load F0 curve from file
 */
static int load_f0_curve(const char* filename, F0Curve* curve) {
    if (!filename || !curve) {
        return -1;
    }

    FILE* file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Error: Cannot open F0 file '%s': %s\n", 
                filename, strerror(errno));
        return -1;
    }

    memset(curve, 0, sizeof(F0Curve));

    // First pass: count valid lines
    char line[256];
    size_t line_count = 0;
    
    while (fgets(line, sizeof(line), file)) {
        // Skip comments and empty lines
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\0') {
            continue;
        }
        
        double time, f0;
        if (sscanf(line, "%lf %lf", &time, &f0) == 2) {
            line_count++;
        }
    }

    if (line_count == 0) {
        fprintf(stderr, "Error: No valid F0 data found in '%s'\n", filename);
        fclose(file);
        return -1;
    }

    // Allocate arrays
    curve->time_sec = malloc(line_count * sizeof(double));
    curve->f0_hz = malloc(line_count * sizeof(double));
    
    if (!curve->time_sec || !curve->f0_hz) {
        free(curve->time_sec);
        free(curve->f0_hz);
        fclose(file);
        return -1;
    }

    // Second pass: read data
    rewind(file);
    curve->length = 0;
    
    while (fgets(line, sizeof(line), file) && curve->length < line_count) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\0') {
            continue;
        }
        
        double time, f0;
        if (sscanf(line, "%lf %lf", &time, &f0) == 2) {
            curve->time_sec[curve->length] = time;
            curve->f0_hz[curve->length] = f0;
            curve->length++;
        }
    }

    fclose(file);

    printf("Loaded F0 curve from '%s': %zu points\n", filename, curve->length);
    if (curve->length > 0) {
        printf("  Time range: %.3f - %.3f seconds\n", 
               curve->time_sec[0], curve->time_sec[curve->length - 1]);
    }

    return 0;
}

/**
 * @brief Cleanup F0 curve resources
 */
static void cleanup_f0_curve(F0Curve* curve) {
    if (!curve) return;
    
    free(curve->time_sec);
    free(curve->f0_hz);
    memset(curve, 0, sizeof(F0Curve));
}

/**
 * @brief Interpolate F0 value at given time using linear interpolation
 */
static double interpolate_f0(const F0Curve* curve, double time) {
    if (!curve || curve->length == 0) {
        return 0.0;
    }

    // If time is before the first point, return first F0 value
    if (time <= curve->time_sec[0]) {
        return curve->f0_hz[0];
    }

    // If time is after the last point, return last F0 value
    if (time >= curve->time_sec[curve->length - 1]) {
        return curve->f0_hz[curve->length - 1];
    }

    // Find the two points to interpolate between
    for (size_t i = 0; i < curve->length - 1; i++) {
        if (time >= curve->time_sec[i] && time <= curve->time_sec[i + 1]) {
            double t1 = curve->time_sec[i];
            double t2 = curve->time_sec[i + 1];
            double f0_1 = curve->f0_hz[i];
            double f0_2 = curve->f0_hz[i + 1];

            // Linear interpolation
            double alpha = (time - t1) / (t2 - t1);
            
            // If either point is unvoiced (0), interpolate as unvoiced
            if (!is_voiced(f0_1) || !is_voiced(f0_2)) {
                return 0.0;
            }
            
            return f0_1 + alpha * (f0_2 - f0_1);
        }
    }

    return 0.0;
}

/**
 * @brief Calculate F0 RMSE between ground truth and estimated curves
 */
static int calculate_f0_rmse(const F0Curve* ground_truth, const F0Curve* estimated, 
                            F0RMSEResult* result) {
    if (!ground_truth || !estimated || !result) {
        return -1;
    }

    memset(result, 0, sizeof(F0RMSEResult));

    // Determine time range for comparison
    double min_time = fmax(ground_truth->time_sec[0], estimated->time_sec[0]);
    double max_time = fmin(ground_truth->time_sec[ground_truth->length - 1],
                          estimated->time_sec[estimated->length - 1]);

    if (min_time >= max_time) {
        result->error_message = strdup("No overlapping time range between curves");
        return -1;
    }

    // Use 10ms time step for comparison
    const double time_step = 0.01;
    size_t num_steps = (size_t)((max_time - min_time) / time_step) + 1;

    double sum_squared_error = 0.0;
    double sum_absolute_error = 0.0;
    double max_error = 0.0;
    size_t valid_points = 0;
    size_t voiced_points = 0;

    for (size_t i = 0; i < num_steps; i++) {
        double time = min_time + i * time_step;
        
        double gt_f0 = interpolate_f0(ground_truth, time);
        double est_f0 = interpolate_f0(estimated, time);

        // Only compare if both are voiced
        if (is_voiced(gt_f0) && is_voiced(est_f0)) {
            double error = gt_f0 - est_f0;
            double abs_error = fabs(error);

            sum_squared_error += error * error;
            sum_absolute_error += abs_error;
            
            if (abs_error > max_error) {
                max_error = abs_error;
            }

            valid_points++;
            voiced_points++;
        }
        
        // Count total comparison points (including unvoiced)
        if (i == 0 || is_voiced(gt_f0) || is_voiced(est_f0)) {
            // Count if it's the first point or if either curve is voiced
        }
    }

    if (valid_points == 0) {
        result->error_message = strdup("No valid comparison points found (both curves voiced)");
        return -1;
    }

    // Calculate metrics
    result->rmse_hz = sqrt(sum_squared_error / valid_points);
    result->mean_error_hz = sum_absolute_error / valid_points;
    result->max_error_hz = max_error;
    result->num_points = num_steps;
    result->voiced_points = voiced_points;

    // Calculate RMSE in cents (using 440Hz as reference)
    // For RMSE in cents, we need to compute the RMS of cent differences
    double sum_squared_cent_error = 0.0;
    const double reference_f0 = 440.0; // A4

    // Second pass for cents calculation
    for (size_t i = 0; i < num_steps; i++) {
        double time = min_time + i * time_step;
        
        double gt_f0 = interpolate_f0(ground_truth, time);
        double est_f0 = interpolate_f0(estimated, time);

        if (is_voiced(gt_f0) && is_voiced(est_f0)) {
            double gt_cents = hz_to_cents(gt_f0, reference_f0);
            double est_cents = hz_to_cents(est_f0, reference_f0);
            double cent_error = gt_cents - est_cents;
            
            sum_squared_cent_error += cent_error * cent_error;
        }
    }

    result->rmse_cents = sqrt(sum_squared_cent_error / voiced_points);

    return 0;
}

/**
 * @brief Print calculation results
 */
static void print_results(const F0RMSEResult* result, int verbose) {
    if (!result) return;

    if (result->error_message) {
        printf("Error: %s\n", result->error_message);
        return;
    }

    printf("F0 RMSE Calculation Results:\n");
    printf("  RMSE (Hz):           %.4f\n", result->rmse_hz);
    printf("  RMSE (cents):        %.4f\n", result->rmse_cents);
    printf("  Mean Absolute Error: %.4f Hz\n", result->mean_error_hz);
    printf("  Maximum Error:       %.4f Hz\n", result->max_error_hz);
    printf("  Comparison Points:   %zu total, %zu voiced\n", 
           result->num_points, result->voiced_points);

    if (verbose) {
        printf("  Voiced Frame Ratio:  %.1f%%\n", 
               (double)result->voiced_points / result->num_points * 100.0);
    }
}

/**
 * @brief Main function for F0 RMSE calculation
 */
int main(int argc, char* argv[]) {
    if (argc < 3) {
        print_usage(argv[0]);
        return 1;
    }

    const char* ground_truth_file = argv[1];
    const char* estimated_file = argv[2];
    int verbose = 0;
    int voiced_only = 0;

    // Parse options
    for (int i = 3; i < argc; i++) {
        if (strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
        } else if (strcmp(argv[i], "--voiced-only") == 0) {
            voiced_only = 1;
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        }
    }

    if (verbose) {
        printf("F0 RMSE Calculation:\n");
        printf("  Ground Truth: %s\n", ground_truth_file);
        printf("  Estimated:    %s\n", estimated_file);
        printf("  Voiced Only:  %s\n\n", voiced_only ? "Yes" : "No");
    }

    // Check if files exist
    if (!file_exists(ground_truth_file)) {
        fprintf(stderr, "Error: Ground truth file '%s' not found\n", ground_truth_file);
        return 1;
    }

    if (!file_exists(estimated_file)) {
        fprintf(stderr, "Error: Estimated file '%s' not found\n", estimated_file);
        return 1;
    }

    // Load F0 curves
    F0Curve ground_truth, estimated;
    
    if (load_f0_curve(ground_truth_file, &ground_truth) != 0) {
        return 1;
    }

    if (load_f0_curve(estimated_file, &estimated) != 0) {
        cleanup_f0_curve(&ground_truth);
        return 1;
    }

    // Calculate RMSE
    F0RMSEResult result;
    int calc_result = calculate_f0_rmse(&ground_truth, &estimated, &result);

    if (calc_result != 0) {
        print_results(&result, verbose);
        free(result.error_message);
        cleanup_f0_curve(&ground_truth);
        cleanup_f0_curve(&estimated);
        return 1;
    }

    // Print results
    print_results(&result, verbose);

    // Cleanup
    free(result.error_message);
    cleanup_f0_curve(&ground_truth);
    cleanup_f0_curve(&estimated);

    return 0;
}
