#include <string>
#include <vector>
#include <map>
#include "opencv2/opencv.hpp"
#include "gdal_priv.h"

struct Point3D {
    double X;
    double Y;
    double Z;
};

struct ImageData {
    std::string imageName;
    double Xs;
    double Ys;
    double Zs;
    double R[9];
};

class testDC {
public:
    testDC(const std::string& imagePath, const std::string& csvPath, const std::string& demPath, const std::string& outputpath);
    void process(double re);

private:
    std::string imagePath;
    std::string csvPath;
    std::string demPath;
    std::string outputpath;

    std::map<std::string, ImageData> readCSV(const std::string& filename);
    std::string getFilenameWithoutExtension(const std::string& filepath);
    // 其他私有方法...
};