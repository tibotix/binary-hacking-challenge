#include <CLI/CLI.hpp>

#include "emulator.h"
#include "kernel.h"


using namespace CPUE;

[[noreturn]] static void die(char const* msg) {
    printf("%s\n", msg);
    exit(1);
}

int run(KernelType kernel_type, std::string const& kernel_img, std::string const& binary, bool verbose) {
    auto* kernel = [&]() -> Kernel* {
        switch (kernel_type) {
            case NONE: return new NoKernel();
            case EMULATE: return new EmulatedKernel();
            case CUSTOM: return new CustomKernel(kernel_img);
            default: die("Unknown kernel type");
        }
    }();
    Emulator e{*kernel, binary, verbose};
    e.start();
    delete kernel;
    return 0;
}

int main(int argc, char** argv) {
    CLI::App app;

    bool verbose = false;
    app.add_flag("-v,--verbose", verbose, "Enable verbose output.");
    KernelType kernel_type;
    app.add_option("--kernel", kernel_type, "Kernel type to use.")
        ->transform(CLI::CheckedTransformer(std::map<std::string, KernelType>({{"none", NONE}, {"emulate", EMULATE}, {"custom", CUSTOM}})))
        ->default_str("emulate");
    std::string kernel_img;
    auto* kernel_img_opt = app.add_option("--kernel-img", kernel_img, "Path to the kernel image to load if --kernel=custom.")->check(CLI::ExistingFile);
    std::string binary;
    app.add_option("binary", binary, "Path to the binary to emulate.")->required()->check(CLI::ExistingFile);

    CLI11_PARSE(app, argc, argv);

    if (kernel_type == CUSTOM && !*kernel_img_opt) {
        die("custom kernel specified but no kernel image given. (Use --kernel-img=<kernel_img> to set one).");
    }
    if (kernel_type != CUSTOM && *kernel_img_opt) {
        die("non-custom kernel specified but kernel image given. (Either use --kernel=custom or remove --kernel-img).");
    }

    return run(kernel_type, kernel_img, binary, verbose);
}