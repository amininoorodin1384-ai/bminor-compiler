#include <iostream>
#include <string_view>

namespace {

void print_usage(std::string_view program_name) {
    std::cout << "B-Minor front-end (development scaffold)\n"
              << "Usage: " << program_name << " --help\n\n"
              << "The initial lexer library is available; parser and highlighter are not implemented yet.\n";
}

}

int main(int argc, char* argv[]) {
    if (argc == 2 && std::string_view(argv[1]) == "--help") {
        print_usage(argv[0]);
        return 0;
    }

    print_usage(argv[0]);
    return argc == 1 ? 0 : 2;
}
