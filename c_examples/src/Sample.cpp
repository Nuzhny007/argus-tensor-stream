#include "WrapperC.h"
#include <experimental/filesystem> //C++17 

TensorStream reader;

void get_cycle(FrameParameters frameParameters, std::map<std::string, std::string> executionParameters) {
	try {
		int frames = std::atoi(executionParameters["frames"].c_str());
		if (!frames)
			return;

		std::shared_ptr<FILE> dumpFile;
		std::string fileName = executionParameters["dumpName"];
		if (!fileName.empty()) {
			remove(fileName.c_str());

			dumpFile = std::shared_ptr<FILE>(fopen(fileName.c_str(), "ab"), std::fclose);
		}
		for (int i = 0; i < frames; i++) {
			if (frameParameters.color.normalization) {
				auto result = reader.getFrame<float>(executionParameters["name"], std::atoi(executionParameters["delay"].c_str()), frameParameters);
				if (!fileName.empty()) {
					int status = reader.dumpFrame<float>((float*)std::get<0>(result), frameParameters, dumpFile);
					if (status < 0)
						return;
				}
				cudaFree(std::get<0>(result));
			}
			else {
				auto result = reader.getFrame<unsigned char>(executionParameters["name"], std::atoi(executionParameters["delay"].c_str()), frameParameters);
				if (!fileName.empty()) {
					int status = reader.dumpFrame<unsigned char>((unsigned char*)std::get<0>(result), frameParameters, dumpFile);
					if (status < 0)
						return;
				}
				cudaFree(std::get<0>(result));
			}
		}
	}
	catch (std::runtime_error e) {
		return;
	}

}

int main()
{
	//reader.enableLogs(-HIGH);
	reader.enableNVTX();
	int sts = VREADER_OK;
	int initNumber = 10;

	while (initNumber--) {
		sts = reader.initPipeline("rtmp://37.228.119.44:1935/vod/big_buck_bunny.mp4", 5, 0, 5);
		if (sts != VREADER_OK)
			reader.endProcessing();
		else
			break;
	}

	reader.skipAnalyzeStage();
	CHECK_STATUS(sts);
	std::thread pipeline([] { reader.startProcessing(); });
	int dstWidth = 1920;
	int dstHeight = 1080;
	ColorOptions colorOptions = { FourCC::NV12 };
	colorOptions.planesPos = Planes::PLANAR;
	colorOptions.normalization = false;
	ResizeOptions resizeOptions = { dstWidth, dstHeight };
	resizeOptions.type = ResizeType::BICUBIC;
	FrameParameters frameParameters = {resizeOptions, colorOptions};

	std::map<std::string, std::string> executionParameters = { {"name", "first"}, {"delay", "0"}, {"frames", "20"}, {"dumpName", std::to_string(dstWidth) + "x" + std::to_string(dstHeight) + ".yuv"} };
	std::thread get(get_cycle, frameParameters, executionParameters);
	get.join();
	reader.endProcessing();
	pipeline.join();
	return 0;
}