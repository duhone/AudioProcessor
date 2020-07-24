#include "Platform/MemoryMappedFile.h"
#include "Platform/PathUtils.h"
#include "core/Log.h"

#include <3rdParty/cli11.h>
#include <3rdParty/fmt.h>

#define DOCTEST_CONFIG_IMPLEMENT
#include <3rdParty/doctest.h>

#include <chrono>
#include <cstdio>

using namespace std;
using namespace std::chrono_literals;
namespace fs = std::filesystem;
using namespace CR;
using namespace CR::Core;

int main(int argc, char** argv) {
	CLI::App app{"AudioProcessor"};
	string inputFileName  = "";
	string outputFileName = "";
	app.add_option("-i,--input", inputFileName, "Input file to store in code")->required();
	app.add_option("-o,--output", outputFileName,
	               "Output file and path. filename only, both a .h and a .cpp will be generated at the output location")
	    ->required();

	CLI11_PARSE(app, argc, argv);

	fs::path inputPath{inputFileName};
	fs::path outputPath{outputFileName};

	filesystem::current_path(Platform::GetCurrentProcessPath());

	if(!fs::exists(inputPath)) {
		CLI::Error error{"input file", "Input file doesn't exist", CLI::ExitCodes::FileError};
		app.exit(error);
	}
	if(outputPath.has_extension()) {
		CLI::Error error{"extension",
		                 "do not add an extension to the output file name, .h and .cpp will be appended automaticly",
		                 CLI::ExitCodes::FileError};
		app.exit(error);
	}

	fs::path outputHeader = outputPath.replace_extension(".h");
	fs::path outputSrc    = outputPath.replace_extension(".cpp");
	string varName        = outputPath.filename().replace_extension("").string();

	Platform::MemoryMappedFile inputData(inputPath);

	bool needsUpdating = false;
	if(!fs::exists(outputHeader) || (fs::last_write_time(outputHeader) <= fs::last_write_time(inputPath))) {
		needsUpdating = true;
	}
	if(!fs::exists(outputSrc) || (fs::last_write_time(outputSrc) <= fs::last_write_time(inputPath))) {
		needsUpdating = true;
	}
	if(!needsUpdating) {
		return 0;    // nothing to do;
	}

	{
		fs::path outputFolder = outputHeader;
		outputFolder.remove_filename();
		fs::create_directories(outputFolder);
	}

	{
		fs::path outputFolder = outputSrc;
		outputFolder.remove_filename();
		fs::create_directories(outputFolder);
	}

	return 0;
}
