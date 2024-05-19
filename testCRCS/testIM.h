#pragma once
#include <string>
#include <vector>

class testIM{
public:
    testIM(const std::vector<std::string>& inputFiles, const std::string& outputDirectory);

    void process();

private:
    //std::string inputDirectory;
    std::vector<std::string> inputFiles;
	std::string outputDirectory;
};

