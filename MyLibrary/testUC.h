// testUC.h
#ifndef testUC_H
#define testUC_H

#include <gdal_priv.h>
#include <string>

_declspec(dllexport) void processImages(GDALDataset*& poRefDataset, GDALDataset* poDataset, const std::string& imagePath, const std::string& outputDirectory);

#endif // testUC_H