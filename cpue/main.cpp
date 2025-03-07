#include <CLI/CLI.hpp>

#include "emulator.h"
#include "kernel.h"


using namespace CPUE;

[[noreturn]] static void die(char const* msg) {
    printf("%s\n", msg);
    exit(1);
}

int run(KernelType kernel_type, std::string const& kernel_img, std::string const& binary_path, size_t ram_mb, bool verbose, bool serial) {
    auto* kernel = [&]() -> Kernel* {
        switch (kernel_type) {
            case NONE: return new NoKernel();
            case EMULATE: return new EmulatedKernel();
            case CUSTOM: return new CustomKernel(kernel_img);
            default: die("Unknown kernel type");
        }
    }();
    Emulator e{*kernel, binary_path};
    e.set_verbosity(verbose);
    e.set_serial(serial);
    e.set_available_ram_in_mb(ram_mb);
    e.start();
    delete kernel;
    return 0;
}

int main(int argc, char** argv) {
    CLI::App app;

    bool verbose = false;
    app.add_flag("-v,--verbose", verbose, "Enable verbose output.");
    size_t ram_mb;
    app.add_option("--ram,-r", ram_mb, "Available RAM in MB.")->check(CLI::PositiveNumber)->default_val("4");
    KernelType kernel_type;
    app.add_option("--kernel", kernel_type, "Kernel type to use.")
        ->transform(CLI::CheckedTransformer(std::map<std::string, KernelType>({{"none", NONE}, {"emulate", EMULATE}, {"custom", CUSTOM}})))
        ->default_val("emulate");
    std::string kernel_img;
    auto* kernel_img_opt = app.add_option("--kernel-img", kernel_img, "Path to the kernel image to load if --kernel=custom.")->check(CLI::ExistingFile);
    bool no_serial = false;
    app.add_flag("--no-serial", no_serial, "Disable serial-console.");
    std::string binary_path;
    app.add_option("binary", binary_path, "Path to the binary to emulate.")->required()->check(CLI::ExistingFile);

    CLI11_PARSE(app, argc, argv);

    if (kernel_type == CUSTOM && !*kernel_img_opt) {
        die("custom kernel specified but no kernel image given. (Use --kernel-img=<kernel_img> to set one).");
    }
    if (kernel_type != CUSTOM && *kernel_img_opt) {
        die("non-custom kernel specified but kernel image given. (Either use --kernel=custom or remove --kernel-img).");
    }

    return run(kernel_type, kernel_img, binary_path, ram_mb, verbose, !no_serial);
}