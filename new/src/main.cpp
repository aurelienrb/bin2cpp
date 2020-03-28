/*
 *  bin2cpp:
 *  - generates C++11 source code which embed several external (binary) files.
 *
 *  Features:
 *  - can wrap the generated code into a namespace
 *  - can iterate (recursively) over the files of a given folder
 *  - name of the original input file is also embedded with its data
 *  - provides a C++11 interface compatible with range-based for loops
 *
 *  License:
 *  - This is free and unencumbered software released into the public domain.
 *  - Anyone is free to copy, modify, publish, use, compile, sell, or
 *    distribute this software, either in source code form or as a compiled
 *    binary, for any purpose, commercial or non-commercial, and by any means.
 *  - For more information, please refer to http://unlicense.org/
 *
 *  Author(s):
 *  - Aurelien Regat-Barrel (https://github.com/aurelienrb/bin2cpp)
 */

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
namespace fs = std::filesystem;

// creates a valid C++ variable name from a file name
static std::string makeFileCppVarName(const fs::path & filePath) {
    std::string result;

    result = "file_";
    for (char c : filePath.filename().generic_string()) {
        if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) {
            result += c;
        }
        else {
            result += '_';
        }
    }

    return result;
}

struct InputFile {
    fs::path filePath;
    std::string cppVarName;
};

static InputFile makeInputFile(const fs::path & filePath) {
    return InputFile{ filePath, makeFileCppVarName(filePath) };
}

// Program options.
// We don't support Unicode (wide strings) but that's on purpose (given strings will appear in C++ source code)
struct Options {
    // list of files to embed
    std::vector<InputFile> inputFiles;
    // outout directory for generated files
    fs::path outputDir;
    // output file names
    std::string outputBaseName;
    // C++ namespace to use (if any)
    std::string namespaceName;
};

const std::string s_defaultOutputBase = "embedded_files";

// Display help message
void displayUsage() {
    std::cout << "bin2cpp: generates C++11 source code which embed several external (binary) files.\n";
    std::cout << "Supported options:\n";
    std::cout << " <input>    : path to an input file or directory to embed in C++ code.\n";
    std::cout << "              If it's a directory, its content will be recursively iterated.\n";
    std::cout << "              Note: several inputs can be passed on the command line.\n";
    std::cout << " -h         : this help message.\n";
    std::cout << " -d <path>  : directory where to save the generated files.\n";
    std::cout << " -o <name>  : base name to be used for the generated .h/.cpp files.\n";
    std::cout << "              => '-o generated' will produce 'generated.h' and 'generated.cpp' files.\n";
    std::cout << "              Default value is '" << s_defaultOutputBase << "'.\n";
    std::cout << " -ns <name> : name of the namespace to be used in generated code (recommended).\n";
    std::cout << "              Default is empty (no namespace).\n";
}

// Parse supported program options (-o, -ns, ...)
void parseNamedArgument(const std::string & argName, const std::string & argValue, Options & options) {
    assert(argName.front() == '-');
    assert(!argValue.empty());

    if (argName == "-d") {
        options.outputDir = argValue;
    } else if (argName == "-o") {
        options.outputBaseName = argValue;
    } else if (argName == "-ns") {
        options.namespaceName = argValue;
    } else {
        throw std::runtime_error{ "invalid option name: " + argName };
    }
}

// Parse one given input value (test if it's a file name or a directory to iterate for the files it contains)
void parsePositionalArgument(const std::string & value, Options & options) {
    if (fs::is_directory(value)) {
        // this syntax requires boost filesystem version >= 1.61
        for (const fs::path & path : fs::recursive_directory_iterator{ value }) {
            if (fs::is_regular_file(path)) {
                // use generic_string() to normalize the path on Windows platform
                options.inputFiles.push_back(makeInputFile(path));
            }
        }
    } else if (fs::is_regular_file(value)) {
        options.inputFiles.push_back(makeInputFile(value));
    } else {
        throw std::runtime_error{ "can't find file or directory '" + value + "'" };
    }
}

// Parse the given command line
Options parseCommandLine(int argc, char ** argv) {
    Options options{};

    if (argc == 1) {
        displayUsage();
        std::exit(0);
    }

    for (int i = 1; i < argc; ++i) {
        const std::string arg{ argv[i] };

        if (arg.front() == '-') {
            if (arg == "-h") {
                displayUsage();
                std::exit(0);
            } else if (i == argc - 1) {
                throw std::runtime_error{ "missing value for option " + arg };
            } else {
                parseNamedArgument(arg, argv[i + 1], options);
                i += 1;
            }
        } else {
            parsePositionalArgument(arg, options);
        }
    }

    // make sure to always have an output dir to simplify the code
    if (options.outputDir.empty()) {
        options.outputDir = fs::current_path();
        std::cout << "Using " << options.outputDir << " as output dir" << std::endl;
    } else {
        // create output dir if missing
        if (!fs::exists(options.outputDir)) {
            std::cout << "Creating output dir " << options.outputDir << std::endl;
            fs::create_directories(options.outputDir);
        }
    }

    if (options.outputBaseName.empty()) {
        options.outputBaseName = s_defaultOutputBase;
    }

    return options;
}

void convertFileDataToCppSource(const fs::path & filePath, const std::string & cppVarName, std::ostream & stream) {
    assert(fs::is_regular_file(filePath));

    std::ifstream inputFile{ filePath, std::ios_base::in | std::ios_base::binary };
    if (!inputFile) {
        throw std::runtime_error{ std::string("failed to open file ") + filePath.generic_string() };
    }

    // save formatting flags of the given stream
    std::ios::fmtflags flags(stream.flags());

    const auto fileLen = static_cast<size_t>(fs::file_size(filePath));
    stream << "static const char * name_" << cppVarName << " = \"" << filePath.filename().generic_string() << "\";\n";
    stream << "static const char * data_" << cppVarName << " = \n";

    size_t current_line_width = 0;
    char c;
    while (inputFile.get(c)) {
        if (current_line_width == 0) {
            stream << "\"";
            current_line_width = 1;
        }

        // handle special chars
        if (c == '"') {
            stream << "\\\"";
            current_line_width += 2;
        }
        else if (c == '\n') {
            // got to new line when we find one
            stream << "\\n\"\n";
            current_line_width = 0;
        }
        else if (c == '\r') {
            stream << "\\r";
            current_line_width += 2;
        }
        else if (c == '\t') {
            stream << "\\t";
            current_line_width += 2;
        }
        else if (c >= 0x20 && c <= 0x7e) { // ASCII chars [0x20, 0x7e]
            stream << c;
            current_line_width++;
        }
        else {
            const auto x = static_cast<unsigned int>(c);
            stream << "\\x";
            if (x < 16) {
                stream << '0';
            }
            stream << std::hex << x;
            current_line_width += 4;
        }

        if (current_line_width >= 120) {
            stream << "\"\n\n";
            current_line_width = 0;
        }
    }

    if (current_line_width > 0) {
        stream << "\"\n\n";
    }
    stream << ";\n";
    stream << "\n";

    // restore save formatting flags
    stream.flags(flags);
}

static const char * warningBanner = "// This file was generated by bin2cpp\n"
                                    "// WARNING: any change you make will be lost!\n";

void generateHeaderFile(const Options & options, const fs::path & headerFilePath) {
    std::cout << "Generating " << headerFilePath.generic_string() << "...\n";
    std::ofstream stream{ headerFilePath };
    if (stream) {
        const std::string includeGuard = "GENERATED_BIN2CPP_" + options.namespaceName + "_H";

        stream << warningBanner;
        stream << "#ifndef " << includeGuard << "\n";
        stream << "#define " << includeGuard << "\n";
        stream << "\n";
        stream << "#include <map>\n";
        stream << "#include <string>\n";
        stream << "\n";

        if (!options.namespaceName.empty()) {
            stream << "namespace " << options.namespaceName << " {\n";
            stream << "\n";
        }

        stream << "// total number of embedded files\n";
        stream << "constexpr size_t embeddedFileCount = " << options.inputFiles.size() << ";\n";

        for (const InputFile & f : options.inputFiles) {
            stream << "\n";
            stream << "// file \"" << f.filePath.filename().generic_string() << "\"\n";
            stream << "const std::string & get_" << f.cppVarName << "();\n";
        }

        stream << R"(
// returns all the embedded files indexed by their name
const std::map<std::string, std::string> & allEmbeddedFiles();

// returns the content of an embedded file (throws an exception if not found)
const std::string & mustGetFile(const std::string & fileName);
)";

        if (!options.namespaceName.empty()) {
            stream << "\n";
            stream << "} // " << options.namespaceName << "\n";
        }

        stream << "\n";
        stream << "#endif // " << includeGuard << "\n";
    } else {
        throw std::runtime_error{ "failed to create header file!" };
    }
}

void generateBodyFile(const Options & options, const fs::path & headerFilePath, const fs::path & bodyFilePath) {
    std::cout << "Generating " << bodyFilePath.generic_string() << "...\n";
    std::ofstream stream{ bodyFilePath };
    if (stream) {
        stream << warningBanner;
        stream << "#include \"" << headerFilePath.filename().generic_string() << "\"\n";
        stream << "\n";

        // process the given files
        for (const InputFile & f : options.inputFiles) {
            // read the file
            std::cout << "  " << f.filePath.filename() << "\n";
            convertFileDataToCppSource(f.filePath, f.cppVarName, stream);
        }

        // function to build the map
        stream << "static std::map<std::string, std::string> buildEmbeddedFileMap() {\n";
        stream << "    std::map<std::string, std::string> result;\n";
        stream << "\n";
        for (const InputFile & f : options.inputFiles) {
            stream << "    result[name_" << f.cppVarName << "] = data_" << f.cppVarName << ";\n";
        }
        stream << "\n";
        stream << "    return result;\n";
        stream << "}\n";

        if (!options.namespaceName.empty()) {
            stream << "\n";
            stream << "namespace " << options.namespaceName << " {\n";
        }

        for (const InputFile & f : options.inputFiles) {
            stream << "\n";
            stream << "const std::string & get_" << f.cppVarName << "() {\n";
            stream << "    static const std::string s_data = data_" << f.cppVarName << ";\n";
            stream << "    return s_data;\n";
            stream << "}\n";
        }

        stream << R"raw(
const std::map<std::string, std::string> & allEmbeddedFiles() {
    static const std::map<std::string, std::string> s_map = buildEmbeddedFileMap();
    return s_map;
}

const std::string & mustGetFile(const std::string & fileName) {
    const auto & files = allEmbeddedFiles();
    const auto it = files.find(fileName);
    if (it != files.end()) {
        return it->second;
    }
    throw std::runtime_error{ "embedded file not found: " + fileName };
}
)raw";

        stream << "\n";
        if (!options.namespaceName.empty()) {
            stream << "}\n";
            stream << "\n";
        }
    } else {
        throw std::runtime_error{ "failed to create cpp file!" };
    }
}

int main(int argc, char ** argv) {
    try {
        const auto options = parseCommandLine(argc, argv);
        if (options.inputFiles.empty()) {
            std::cerr << "Warning: no input file to process, will generate empty C++ output!\n";
        } else {
            std::cout << "Ready to process " << options.inputFiles.size() << " file(s).\n";
        }

        const fs::path headerFilePath = options.outputDir / (options.outputBaseName + ".h");
        const fs::path bodyFilePath = options.outputDir / (options.outputBaseName + ".cpp");

        generateHeaderFile(options, headerFilePath);
        generateBodyFile(options, headerFilePath, bodyFilePath);
    } catch (const std::exception & e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
