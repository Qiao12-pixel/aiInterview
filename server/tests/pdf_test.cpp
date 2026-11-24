#include "interview/pdf_parser.h"
#include <fstream>
#include <iterator>
#include <iostream>
#include <stdexcept>
int main(int argc, char **argv) {
    if (!backend::ParseResumePdf("").empty())
        return 1;
    try {
        backend::ParseResumePdf("invalid");
        return 2;
    } catch (const std::invalid_argument &) {
    }
    if (argc > 1) {
        std::ifstream in(argv[1], std::ios::binary);
        std::string data(std::istreambuf_iterator<char>(in), {});
        if (backend::ParseResumePdf(data).empty())
            return 3;
    }
    std::cout << "PDF validation/extraction passed\n";
}
