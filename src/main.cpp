#include <3rdParty/cli11.h>

#include "Platform/MemoryMappedFile.h"
#include "Platform/PathUtils.h"
#include "core/BinaryStream.h"
#include "core/Guid.h"
#include "core/Log.h"

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

struct ChunkHeader {
	uint32_t ChunkID;
	uint32_t ChunkSize;
};

struct WavChunk {
	const static uint32_t c_ChunkID = 'FFIR';

	const static uint32_t c_WavID = 'EVAW';

	uint32_t WavID{0};
};

struct FmtChunk {
	const static uint32_t c_ChunkID = ' tmf';

	uint16_t FormatCode{0};
	uint16_t NChannels{0};
	uint32_t SampleRate{0};
	uint32_t DataRate{0};
	uint16_t DataBlockSize{0};
	uint16_t BitsPerSample{0};
};

static_assert(sizeof(FmtChunk) == 16);

struct WavFile {
	uint32_t NChannels{0};
	vector<int16_t> Data;
};

WavFile ReadWaveFile(const fs::path& a_inputPath) {
	WavFile result;

	Platform::MemoryMappedFile inputData(a_inputPath);

	BinaryReader reader;
	reader.Data = inputData.data();
	reader.Size = (uint32_t)inputData.size();

	WavChunk wavChunk;
	FmtChunk fmtChunk;

	ChunkHeader header;
	while(Read(reader, header)) {
		if(header.ChunkID == WavChunk::c_ChunkID) {
			Read(reader, wavChunk);    // ignore chunk size for main header
		} else if(header.ChunkID == FmtChunk::c_ChunkID) {
			Read(reader, fmtChunk);
			reader.Offset += header.ChunkSize - sizeof(FmtChunk);    // skip extended fmts
		} else if(header.ChunkID == 'atad') {
			// handle data chunk specially
			result.Data.resize(header.ChunkSize / 2);
			memcpy(result.Data.data(), reader.Data + reader.Offset, header.ChunkSize / 2);
			reader.Offset += header.ChunkSize;
		} else {
			reader.Offset += header.ChunkSize;
		}
	}

	if(wavChunk.WavID != WavChunk::c_WavID) {
		Core::Log::Warn("Input wave file {} did not have expected riff chunk, invalid wave file", a_inputPath.string());
		result.Data.clear();
		return result;
	}

	if(fmtChunk.FormatCode != 1) {
		Core::Log::Warn("Input wave file {} was not in raw pcm format", a_inputPath.string());
		result.Data.clear();
		return result;
	}

	if(!(fmtChunk.NChannels == 1 || fmtChunk.NChannels == 2)) {
		Core::Log::Warn("Input wave file {} not either mono or stereo", a_inputPath.string());
		result.Data.clear();
		return result;
	}

	if(fmtChunk.BitsPerSample != 16) {
		Core::Log::Warn("Input wave file {} not 16 bits per sample", a_inputPath.string());
		result.Data.clear();
		return result;
	}

	if(fmtChunk.SampleRate != 48000) {
		Core::Log::Warn("Input wave file {} not 48Khz", a_inputPath.string());
		result.Data.clear();
		return result;
	}

	result.NChannels = fmtChunk.NChannels;

	return result;
}

int main(int argc, char** argv) {
	CLI::App app{"AudioProcessor"};
	string inputFileName  = "";
	string outputFileName = "";
	app.add_option("-i,--input", inputFileName, "Input wav file, must be wav 48khz 16bit, stereo or mono supported.")
	    ->required();
	app.add_option("-o,--output", outputFileName, "Output craud file and path.")->required();

	CLI11_PARSE(app, argc, argv);

	fs::path inputPath{inputFileName};
	fs::path outputPath{outputFileName};

	filesystem::current_path(Platform::GetCurrentProcessPath());

	if(!fs::exists(inputPath)) {
		CLI::Error error{"input file", "Input file doesn't exist", CLI::ExitCodes::FileError};
		app.exit(error);
	}
	if(outputPath.has_extension() && outputPath.extension() != ".craud") {
		CLI::Error error{"extension", "extension must be craud, or optionaly don't specify an extension",
		                 CLI::ExitCodes::FileError};
		app.exit(error);
	}

	outputPath.replace_extension(".craud");

	bool needsUpdating = false;
	if(!fs::exists(outputPath) || (fs::last_write_time(outputPath) <= fs::last_write_time(inputPath))) {
		needsUpdating = true;
	}
	if(!needsUpdating) {
		return 0;    // nothing to do;
	}

	{
		fs::path outputFolder = outputPath;
		outputFolder.remove_filename();
		fs::create_directories(outputFolder);
	}

	WavFile pcmData = ReadWaveFile(inputPath);

	if(pcmData.Data.empty()) {
		CLI::Error error{"input file", "Failed to read input file, or input file was a wrong format",
		                 CLI::ExitCodes::FileError};
		app.exit(error);
	}

	return 0;
}
