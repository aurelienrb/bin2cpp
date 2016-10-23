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
 *	distribute this software, either in source code form or as a compiled
 *	binary, for any purpose, commercial or non-commercial, and by any means.
 *  - For more information, please refer to http://unlicense.org/
 *
 *  Author(s):
 *  - Aurelien Regat-Barrel (https://github.com/aurelienrb/bin2cpp)
 */

#include <string>
#include <vector>
#include <cassert>
#include <iostream>
#include <sstream>
#include <fstream>
#include <filesystem>
namespace fs = std::tr2::sys;

// Program options.
// We don't support Unicode (wide strings) but that's on purpose (given strings will appear in C++ source code)
struct Options {
	// list of files to embed
	std::vector<std::string> inputFiles;
	// outout directory for generated files
	fs::path outputDir;
	// output file names
	std::string headerFileName;
	std::string cppFileName;
	// C++ namespace to use (if any)
	std::string namespaceName;
};

const std::string s_defaultOutputBase = "bin2cpp";

// Display help message
void displayUsage() {
	std::cout << "bin2cpp: generates C++11 source code which embed several external (binary) files.\n";
	std::cout << "Supported options:\n";
	std::cout << " <input>	: path to an input file or directory to embed in C++ code.\n";
	std::cout << "			  If it's a directory, its content will be recursively iterated.\n";
	std::cout << "			  Note: several inputs can be passed on the command line.\n";
	std::cout << " -h		 : this help message.\n";
	std::cout << " -d <path>  : directory where to save the generated files.\n";
	std::cout << " -o <name>  : base name to be used for the generated .h/.cpp files.\n";
	std::cout << "			  => '-o generated' will produce 'generated.h' and 'generated.cpp' files.\n";
	std::cout << "			  Default value is '" << s_defaultOutputBase << "'.\n";
	std::cout << " -ns <name> : name of the namespace to be used in generated code (recommended).\n";
	std::cout << "			  Default is empty (no namespace).\n";
}

// Parse supported program options (-o, -ns, ...)
void parseNamedArgument(const std::string & argName, const std::string & argValue, Options & options) {
	assert(argName.front() == '-');
	assert(!argValue.empty());

	if (argName == "-d") {
		if (!fs::is_directory(argValue)) {
			throw std::runtime_error{ "Invalid output directory: " + argValue };
		}
		options.outputDir = argValue;
	}
	else if (argName == "-o") {
		options.headerFileName = argValue + ".h";
		options.cppFileName = argValue + ".cpp";
	}
	else if (argName == "-ns") {
		options.namespaceName = argValue;
	}
	else {
		throw std::runtime_error{ "Invalid option name: " + argName };
	}
}

// Parse one given input value (test if it's a file name or a directory to iterate for the files it contains)
void parsePositionalArgument(const std::string & value, Options & options) {
	if (fs::is_directory(value)) {
		// this syntax requires boost filesystem version >= 1.61
		for (auto path : fs::recursive_directory_iterator{ value }) {
			if (fs::is_regular_file(path)) {
				// use generic_string() to normalize the path on Windows platform
				options.inputFiles.push_back(path.path().generic_string());
			}
		}
	}
	else if (fs::is_regular_file(value)) {
		options.inputFiles.push_back(value);
	}
	else {
		throw std::runtime_error{ "Can't find file or directory named " + value };
	}
}

// Parse the given command line
Options parseCommandLine(int argc, char ** argv) {
	Options options;

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
			}
			else if (i == argc - 1) {
				throw std::runtime_error{ "Missing value for option " + arg };
			}
			else {
				parseNamedArgument(arg, argv[i + 1], options);
				i += 1;
			}
		}
		else {
			parsePositionalArgument(arg, options);
		}
	}

	if (options.cppFileName.empty()) {
		options.headerFileName = s_defaultOutputBase + ".h";
		options.cppFileName = s_defaultOutputBase + ".cpp";
	}

	return options;
}

void convertFileDataToCppSource(const std::string & fileName, const std::string & fileId, std::ostream & stream) {
	assert(fs::is_regular_file(fileName));

	std::ifstream inputFile{ fileName, std::ios_base::in | std::ios_base::binary };
	if (!inputFile) {
		throw std::runtime_error{std::string("Failed to open file ") + fileName};
	}

	// save formatting flags of the given stream
	std::ios::fmtflags flags(stream.flags());
	
	const auto fileLen = static_cast<unsigned int>(fs::file_size(fileName));
	stream << "\tconst char * " << fileId << "_name = \"" << fileName << "\";\n";
	stream << "\tconst unsigned int " << fileId << "_data_size = " << fileLen << ";\n";
	stream << "\tconst unsigned char " << fileId << "_data[" << fileId << "_data_size] = {";

	size_t char_count{ 0 };
	char c;
	while (inputFile.get(c)) {
		if (char_count % 20 == 0) {
			stream << "\n\t\t";
		}
		char_count += 1;

		stream << "0x" << std::hex << (static_cast<int>(c) & 0xFF) << ",";
	}
	assert(char_count == fileLen);

	stream << "\n\t};\n";

	// restore save formatting flags
	stream.flags(flags);
}

void generateHeaderFile(const Options & options) {
	static const char * s_headerContent = R"raw(
	struct FileInfo {
		const char * fileName;
		const char * fileData;
		const unsigned int fileDataSize;

		std::string name() const {
			return fileName;
		}

		std::string content() const {
			return std::string{ fileData, fileDataSize };
		}
	};

	extern const unsigned int fileInfoListSize;
	extern const FileInfo fileInfoList[];

	struct FileInfoRange {
		const FileInfo * begin() const {
			return &fileInfoList[0];
		}
		const FileInfo * end() const {
			return begin() + size();
		}
		const size_t size() const {
			return fileInfoListSize;
		}
	};

	inline FileInfoRange fileList() {
		return FileInfoRange{};
	}
)raw";

	const fs::path fileName = options.outputDir.empty() ?
		options.headerFileName :
		options.outputDir / options.headerFileName;
	std::cout << "Generating " << fileName.generic_string() << "...\n";
	std::ofstream stream{ fileName };
	if (stream) {
		stream << "#pragma once\n";
		stream << "\n";
		stream << "#include <string>\n";

		if (!options.namespaceName.empty()) {
			stream << "\n";
			stream << "namespace " << options.namespaceName << " {";
		}
		stream << s_headerContent;
		if (!options.namespaceName.empty()) {
			stream << "}\n";
		}
	}
	else {
		throw std::runtime_error{ "Failed to create header file!" };
	}
}

void generateBodyFile(const Options & options) {
	const fs::path fileName = options.outputDir.empty() ?
		options.cppFileName :
		options.outputDir / options.cppFileName;

	std::cout << "Generating " << fileName.generic_string() << "...\n";
	std::ofstream stream{ fileName };
	if (stream) {
		stream << "#include \"" << options.headerFileName << "\"\n";
		stream << "\n";

		stream << "namespace /* anonymous */ {\n";

		// process the given files
		std::vector<std::string> fileIds;
		for (auto path : options.inputFiles) {
			// increment the file id
			const std::string fileId = "file" + std::to_string(fileIds.size());
			// read the file
			std::cout << "  " << path << "\n";
			convertFileDataToCppSource(path, fileId, stream);
			fileIds.emplace_back(fileId);
		}

		stream << "}\n";
		stream << "\n";

		if (!options.namespaceName.empty()) {
			stream << "namespace " << options.namespaceName << " {\n";
		}
		stream << "\tconst unsigned int fileInfoListSize = " << fileIds.size() << ";\n";
		stream << "\tconst FileInfo fileInfoList[fileInfoListSize] = {\n";
		for (auto id : fileIds) {
			stream << "\t\t{ " << id << "_name, reinterpret_cast<const char*>(" << id << "_data), " << id << "_data_size },\n";
		}
		stream << "\t};\n";
		if (!options.namespaceName.empty()) {
			stream << "}\n";
		}
	}
	else {
		throw std::runtime_error{ "Failed to create cpp file!" };
	}
}

int main(int argc, char ** argv) {
	try {
		const auto options = parseCommandLine(argc, argv);
		if (options.inputFiles.empty()) {
			std::cerr << "Warning: no input file to process, will generate empty C++ output!\n";
		}
		else {
			std::cout << "Ready to process " << options.inputFiles.size() << " file(s).\n";
		}

		generateHeaderFile(options);
		generateBodyFile(options);
	}
	catch (const std::exception & e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}
	return 0;
}
