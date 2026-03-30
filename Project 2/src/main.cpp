#include "csv_io.hpp"
#include "apsc.hpp"
#include "geometry.hpp"
#include <iostream>
#include <cstdlib>
#include <stdexcept>
#include <fstream>
#include <chrono>
#include <string>

// Read peak RSS (VmHWM) from /proc/self/status on Linux/WSL.
// Returns peak memory in KB, or -1 if unavailable.
static long get_peak_rss_kb() {
    std::ifstream f("/proc/self/status");
    if (!f.is_open()) return -1;
    std::string line;
    while (std::getline(f, line)) {
        if (line.rfind("VmHWM:", 0) == 0) {
            long kb = 0;
            std::sscanf(line.c_str(), "VmHWM: %ld", &kb);
            return kb;
        }
    }
    return -1;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <input_file.csv> <target_vertices>\n";
        return EXIT_FAILURE;
    }

    const std::string filepath      = argv[1];
    const int         target_n_raw  = std::atoi(argv[2]);

    if (target_n_raw < 3) {
        std::cerr << "Error: target_vertices must be >= 3\n";
        return EXIT_FAILURE;
    }
    const size_t target_n = static_cast<size_t>(target_n_raw);

    Polygon poly;
    try {
        poly = read_polygon(filepath);
    } catch (const std::exception& e) {
        std::cerr << "Error reading input: " << e.what() << '\n';
        return EXIT_FAILURE;
    }

    if (poly.ring_count() == 0) {
        std::cerr << "Error: input polygon has no rings\n";
        return EXIT_FAILURE;
    }

    // Compute input area (sum of signed areas across all rings)
    double area_in = 0.0;
    for (const auto& ring : poly.rings)
        area_in += signed_area(*ring);

    // No simplification needed if already at or below target
    double total_displacement = 0.0;
    auto t_start = std::chrono::high_resolution_clock::now();
    if (poly.total_vertices() > target_n)
        total_displacement = simplify(poly, target_n);
    auto t_end = std::chrono::high_resolution_clock::now();
    double elapsed_ms = std::chrono::duration<double, std::milli>(t_end - t_start).count();
    long peak_kb = get_peak_rss_kb();
    std::cerr << "Simplification: " << poly.total_vertices() << " vertices, "
              << elapsed_ms << " ms";
    if (peak_kb > 0) std::cerr << ", peak RSS: " << peak_kb << " KB";
    std::cerr << "\n";

    // Compute output area
    double area_out = 0.0;
    for (const auto& ring : poly.rings)
        area_out += signed_area(*ring);

    write_polygon(poly, area_in, area_out, total_displacement);
    return EXIT_SUCCESS;
}
