#include "testIM.h"

#include <opencv2/opencv.hpp>
#include <vector>
#include <string>
#include <iostream>
#include <gdal_priv.h>
#include <sstream>
#include <filesystem>
#include <gdalwarper.h>


using namespace std;
using namespace cv;

testIM::testIM(const std::vector<std::string>& inputFiles, const std::string& outputDirectory)
	: inputFiles(inputFiles), outputDirectory(outputDirectory)
{
	// 构造函数
}


static void draw_point(cv::Mat& img, cv::Point2f fp, cv::Scalar color)
{
    cv::circle(img, fp, 2, color, cv::FILLED, cv::LINE_AA, 0);
}

// 用于表示地理坐标的结构体
struct GeoCoordinate {
    double latitude;
    double longitude;
};
struct VertexWithCenter {
    cv::Point2f vertex;
    cv::Point2f center;
};
// 用于表示遥感影像的类
class RemoteSensingImage {
public:
    GeoCoordinate minCoordinate{ 0, 0 };
    GeoCoordinate maxCoordinate{ 0, 0 };
    GeoCoordinate centerCoordinate{ 0, 0 };
    GDALDataset* poDataset;
    std::string filePath;
    double adfGeoTransform[6];
    cv::Mat image;

    RemoteSensingImage(const std::string& filePath) {
        // 从文件中读取影像，获取其地理坐标
        GDALAllRegister();
        poDataset = (GDALDataset*)GDALOpen(filePath.c_str(), GA_ReadOnly);
        if (poDataset == NULL)
        {
            std::cerr << "Error: file open failed!" << std::endl; // 文件打开失败，处理错误
        }

        // 读取影像的地理坐标
        if (poDataset->GetGeoTransform(adfGeoTransform) == CE_None)
        {
            // 获取影像的地理坐标
            double Xmin = minCoordinate.longitude = adfGeoTransform[0];
            double Ymax = minCoordinate.latitude = adfGeoTransform[3];
            double Xmax = maxCoordinate.longitude = adfGeoTransform[0] + adfGeoTransform[1] * poDataset->GetRasterXSize();
            double Ymin = maxCoordinate.latitude = adfGeoTransform[3] + adfGeoTransform[5] * poDataset->GetRasterYSize();
            // 计算影像的投影中心
            centerCoordinate.longitude = (Xmin + Xmax) / 2;
            centerCoordinate.latitude = (Ymin + Ymax) / 2;
        }
        else
        {
            std::cerr << "Error: get geo transform failed!" << std::endl; // 获取地理坐标失败，处理错误
        }

        // 读取图像数据
        GDALRasterBand* poBand = poDataset->GetRasterBand(1);
        int nXSize = poBand->GetXSize();
        int nYSize = poBand->GetYSize();

        image = cv::Mat(nYSize, nXSize, CV_8UC3);

        // Opencv读取影像数据
        image = imread(filePath, cv::IMREAD_COLOR);


        //poBand->RasterIO(GF_Read, 0, 0, nXSize, nYSize, image.data, nXSize, nYSize, GDT_Byte, 0, 0);

        GDALClose(poDataset);
    }
};

// 读取多张影像的函数
std::vector<RemoteSensingImage> readImages(const std::vector<std::string>& filePaths) {
    std::vector<RemoteSensingImage> images;
    for (const auto& filePath : filePaths) {
        RemoteSensingImage image(filePath);
        images.push_back(image); // 将读取的影像添加到images中
    }
    return images;
}
// 绘制三角网
static void draw_delaunay(cv::Mat& img, cv::Subdiv2D& subdiv, cv::Scalar delaunay_color)
{
    std::vector<cv::Vec6f> triangleList;

    subdiv.getTriangleList(triangleList);
    std::vector<cv::Point> pt(3);

    cv::Size size = img.size();
    cv::Rect rect(0, 0, size.width, size.height);

    for (size_t i = 0; i < triangleList.size(); i++)
    {
        cv::Vec6f t = triangleList[i];
        pt[0] = cv::Point(cvRound(t[0]), cvRound(t[1]));
        pt[1] = cv::Point(cvRound(t[2]), cvRound(t[3]));
        pt[2] = cv::Point(cvRound(t[4]), cvRound(t[5]));

        // 绘制三角形
        if (rect.contains(pt[0]) && rect.contains(pt[1]) && rect.contains(pt[2]))
        {
            cv::line(img, pt[0], pt[1], delaunay_color, 1, cv::LINE_AA, 0);
            cv::line(img, pt[1], pt[2], delaunay_color, 1, cv::LINE_AA, 0);
            cv::line(img, pt[2], pt[0], delaunay_color, 1, cv::LINE_AA, 0);
        }
    }
}
// 绘制维诺图
std::vector<std::vector<cv::Point2f>> draw_voronoi(cv::Mat& img, cv::Subdiv2D& subdiv)
{
    std::vector<std::vector<cv::Point2f>> facets;
    std::vector<cv::Point2f> centers;

    subdiv.getVoronoiFacetList(std::vector<int>(), facets, centers);

    std::vector<cv::Point> ifacet;
    std::vector<std::vector<cv::Point>> ifacets(1);
    for (size_t i = 0; i < facets.size(); i++)
    {
        ifacet.resize(facets[i].size());

        for (size_t j = 0; j < facets[i].size(); j++)
        {
            ifacet[j] = facets[i][j];
        }

        cv::Scalar color;
        color[0] = rand() & 255;
        color[1] = rand() & 255;
        color[2] = rand() & 255;

        fillConvexPoly(img, ifacet, color, 8, 0);
    }

    return facets; // 返回多边形的顶点坐标
}


void testIM::process()
{
    std::vector<RemoteSensingImage> images;
    double minX = std::numeric_limits<double>::max();
    double maxX = std::numeric_limits<double>::min();
    double minY = std::numeric_limits<double>::max();
    double maxY = std::numeric_limits<double>::min();
    std::vector<cv::Mat> results;

    // 创建两个向量来存储每张掩膜画布（操作画布）的minGeoX和maxGeoY
    std::vector<double> minGeoXs;
    std::vector<double> maxGeoYs;

    int W = 0;
    int H = 0;
    // 创建可以自由缩放的窗口
    //std::string win_delaunay = "Delaunay";
    //std::string win_voronoi = "Voronoi";
    //std::string win_mask = "Mask";
    //std::string win_transition = "Transition";
    //cv::namedWindow(win_delaunay, cv::WINDOW_NORMAL);
    //cv::namedWindow(win_voronoi, cv::WINDOW_NORMAL);
    //cv::namedWindow(win_mask, cv::WINDOW_NORMAL);
    //cv::namedWindow(win_transition, cv::WINDOW_NORMAL);
    //cv::namedWindow("Result", cv::WINDOW_NORMAL);
    //读取影像文件————需要接口，输入文件地址
    std::vector<std::string> filePaths;
    //std::string directoryPath = inputDirectory; // 文件夹路径
    //for (const auto& entry : std::filesystem::directory_iterator(directoryPath)) {
    //    if (entry.path().extension() == ".tif") { // 只考虑.tif文件
    //        filePaths.push_back(entry.path().string());
    //    }
    //}
    images = readImages(inputFiles);
    filePaths = inputFiles;


    // 打开第一张影像,获取分辨率
    GDALDataset* poSrcDS = (GDALDataset*)GDALOpen(filePaths[0].c_str(), GA_ReadOnly);

    // 获取地理变换和投影参数
    double adfGeoTransform[6];
    poSrcDS->GetGeoTransform(adfGeoTransform);
    const char* pszSRS_WKT = poSrcDS->GetProjectionRef();
    // 计算最终结果影像的地理范围
    for (const auto& image : images) {
        if (image.minCoordinate.longitude < minX) {
            minX = image.minCoordinate.longitude;
        }
        if (image.maxCoordinate.longitude > maxX) {
            maxX = image.maxCoordinate.longitude;
        }
        if (image.minCoordinate.latitude > maxY) {
            maxY = image.minCoordinate.latitude;
        }
        if (image.maxCoordinate.latitude < minY) {
            minY = image.maxCoordinate.latitude;
        }
    }
    //std::cout << "Min X: " << minX << ", Max X: " << maxX << ", Min Y: " << minY << ", Max Y: " << maxY << std::endl;
    // 计算结果影像的范围，宽和高
    int Width = static_cast<int>(std::round(maxX - minX) / adfGeoTransform[1]);
    int Height = static_cast<int>(std::round(maxY - minY) / -adfGeoTransform[5]);

    // 计算维诺图画布的分辨率
    int width = 2000;
    int height = 2000;
    double xsize = (maxX - minX) / 2000;
    double ysize = -(maxY - minY) / 2000;
    // 调用函数
    bool animate = true;
    cv::Scalar delaunay_color(255, 255, 255), points_color(0, 0, 255);
    cv::Mat img = cv::Mat::zeros(height, width, CV_8UC3);
    cv::Mat img_ori = img.clone();
    cv::Size size = img.size();
    cv::Rect rect(0, 0, size.width, size.height);
    cv::Subdiv2D subdiv(rect);

    // 导入种子点
    std::vector<cv::Point2f> points;
    for (const auto& image : images)
    {
        cv::Point2f pt;
        pt.x = static_cast<float>((image.centerCoordinate.longitude - minX) / xsize);
        pt.y = static_cast<float>((image.centerCoordinate.latitude - maxY) / ysize);
        points.push_back(pt);
    }
    // 显示导入的种子点坐标
    //for (size_t i = 0; i < points.size(); i++)
    //{
    //    std::cout << "Point " << i << ": " << points[i] << std::endl;
    //}
    // 将种子点加入分割对象
    for (int i = 0; i < points.size(); i++)
    {
        subdiv.insert(points[i]);

        if (animate)
        {
            cv::Mat img_copy = img_ori.clone();

            // 绘制三角网
            draw_delaunay(img_copy, subdiv, delaunay_color);
            // cv::imshow(win_delaunay, img_copy);
            waitKey(100);
        }
    }
    draw_delaunay(img, subdiv, delaunay_color);

    // 绘制点
    for (size_t i = 0; i < points.size(); i++)
    {
        draw_point(img, points[i], points_color);
    }
    // 绘制 Voronoi 图并获取多边形的顶点坐标
    cv::Mat img_voronoi = img.clone();
    std::vector<std::vector<cv::Point2f>> voronoiVertices = draw_voronoi(img_voronoi, subdiv);
    // cv::imshow(win_voronoi, img_voronoi);

    // emit progressChanged(10);

         // 遍历所有影像
    for (size_t i = 0; i < images.size(); i++) {
        // 转换顶点坐标为地理坐标和影像平面上的坐标
        std::vector<cv::Point> vertices;
        double minGeoX = std::numeric_limits<double>::max();
        double maxGeoX = std::numeric_limits<double>::min();
        double minGeoY = std::numeric_limits<double>::max();
        double maxGeoY = std::numeric_limits<double>::min();

        // 遍历当前多边形的顶点    
        for (const auto& vertex : voronoiVertices[i]) {

            // 计算维诺图的最大最小坐标
            double VnuoXmin = 0 * xsize + minX;
            double VnuoYmax = maxY - 0 * ysize;
            double VnuoXmax = 2000 * xsize + minX;
            double VnuoYmin = maxY - 2000 * ysize;
            //cout << "VnuoXmin: " << VnuoXmin << ", VnuoYmax: " << VnuoYmax << ", VnuoXmax: " << VnuoXmax << ", VnuoYmin: " << VnuoYmin << endl;

            double geoX = vertex.x * xsize + minX;//计算地理坐标
            double geoY = maxY - vertex.y * ysize;
            // 出现了geo被替换的情况
            double GeoX = geoX;
            double GeoY = geoY;
            //cout << "geo0X: " << geoX << ", geo0Y: " << geoY << endl;
            // 如果多边形的顶点坐标超出了维诺图的范围，则将其限制在维诺图的范围内
            if (geoX < VnuoXmin) {
                geoX = VnuoXmin;
            }
            if (geoX > VnuoXmax) {
                geoX = VnuoXmax;
            }
            if (geoY < VnuoYmin) {
                geoY = VnuoYmin;
            }
            if (geoY > VnuoYmax) {
                geoY = VnuoYmax;
            }

            // 寻找最大和最小的多边形的geoX和geoY
            if (geoX < minGeoX) {
                minGeoX = geoX;
            }
            if (geoX > maxGeoX) {
                maxGeoX = geoX;
            }
            if (geoY < minGeoY) {
                minGeoY = geoY;
            }
            if (geoY > maxGeoY) {
                maxGeoY = geoY;
            }
            // 比较第i张影像的最大最小的地理坐标
            double imageMinGeoX = images[i].minCoordinate.longitude;
            double imageMaxGeoX = images[i].maxCoordinate.longitude;
            double imageMinGeoY = images[i].maxCoordinate.latitude;
            double imageMaxGeoY = images[i].minCoordinate.latitude;
            // 找出掩膜文件的范围
            if (imageMinGeoX < minGeoX) {
                minGeoX = imageMinGeoX;
            }
            if (imageMaxGeoX > maxGeoX) {
                maxGeoX = imageMaxGeoX;
            }
            if (imageMinGeoY < minGeoY) {
                minGeoY = imageMinGeoY;
            }
            if (imageMaxGeoY > maxGeoY) {
                maxGeoY = imageMaxGeoY;
            }

            //std::cout << "Min GeoX: " << minGeoX << ", Max GeoX: " << maxGeoX << ", Min GeoY: " << minGeoY << ", Max GeoY: " << maxGeoY << std::endl;
            // 计算多边形的相平面顶点坐标
            double DimgX = (GeoX - minGeoX) / adfGeoTransform[1];
            double DimgY = (maxGeoY - GeoY) / adfGeoTransform[5];
            //cout<< "geoX: " << GeoX << ", geoY: " << GeoY << endl;
            //cout << "DimgX: " << DimgX << ", DimgY: " << DimgY << endl;
            vertices.push_back(cv::Point(static_cast<int>(DimgX), static_cast<int>(DimgY)));
        }
        // 在处理每个多边形后，将minGeoX和maxGeoY添加到minGeoXs和maxGeoYs中
        minGeoXs.push_back(minGeoX);
        maxGeoYs.push_back(maxGeoY);

        // 计算掩膜画布的宽度和高度
        W = static_cast<int>(std::round(maxGeoX - minGeoX) / adfGeoTransform[1]);
        H = static_cast<int>(std::round(maxGeoY - minGeoY) / -adfGeoTransform[5]);
        // 创建掩膜画布
        cv::Mat mask = cv::Mat::zeros(H, W, CV_8UC1);
        // 在掩膜画布上根据多边形的顶点坐标创建掩膜
        cv::fillPoly(mask, std::vector<std::vector<cv::Point>>{vertices}, cv::Scalar(255));
        // 显示掩膜画布
        //cv::imshow(win_mask, mask);
        // 创建操作画布
        cv::Mat transition = cv::Mat::zeros(H, W, CV_8UC3);
        // 将第i张影像逐像素加载到操作画布上
        for (int row = 0; row < images[i].image.rows; row++) {
            for (int col = 0; col < images[i].image.cols; col++) {
                // 计算投影中心在操作画布的行列号
                int x = static_cast<int>((images[i].centerCoordinate.longitude - minGeoX) / adfGeoTransform[1]);
                int y = static_cast<int>((images[i].centerCoordinate.latitude - maxGeoY) / adfGeoTransform[5]);
                // 待计算像素在操作画布的行列号
                int N = static_cast<int>(x + col - images[i].image.cols / 2);
                int M = static_cast<int>(y + row - images[i].image.rows / 2);
                if (N >= 0 && N < transition.cols && M >= 0 && M < transition.rows) {
                    // 获取原始图像的像素值
                    if (row < images[i].image.rows && col < images[i].image.cols) {
                        cv::Vec3b pixel_value = images[i].image.at<cv::Vec3b>(row, col);
                        // 将原始图像的像素值赋给过渡画布的对应像素
                        transition.at<cv::Vec3b>(M, N) = pixel_value;
                    }
                }
            }
        }

        // 进行掩膜操作       
        cv::Mat result;
        cv::Mat mask_three_channels;// 将掩膜文件改成三通道
        cv::cvtColor(mask, mask_three_channels, cv::COLOR_GRAY2BGR);
        cv::bitwise_and(transition, mask_three_channels, result);
        // 生成拼接线
        // 找到掩膜的边界色
        std::vector<std::vector<cv::Point>> contours;
        std::vector<cv::Vec4i> hierarchy;
        cv::findContours(mask, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
        // 创建一个全黑的图像，用于绘制拼接线
        cv::Mat stitchLine = cv::Mat::zeros(mask.size(), CV_8UC3);
        // 绘制白色的拼接线
        cv::drawContours(stitchLine, contours, -1, cv::Scalar(255, 255, 255), 1, cv::LINE_8, hierarchy, 0);
        // 将拼接线应用到结果图像上
        cv::bitwise_or(result, stitchLine, result);

        // 显示转化的操作画布
      /*cv::imshow(win_transition, transition);
        cv::namedWindow("Result", cv::WINDOW_NORMAL);
        cv::imshow("Result", result);
        cv::waitKey(0);*/
        results.push_back(result);
        mask.release();
        mask_three_channels.release();
        transition.release();

        /*double progress = 10 + static_cast<double>(i + 1) / images.size() * 60;
        emit progressChanged(std::round(progress));*/
    }
    GDALDriver* poDriver = GetGDALDriverManager()->GetDriverByName("GTiff");
    GDALDataset* poDstDS;

    // 设置地理坐标信息
    double newGeoTransform[6];
    newGeoTransform[0] = minX;
    newGeoTransform[1] = adfGeoTransform[1];
    newGeoTransform[2] = 0;
    newGeoTransform[3] = maxY;
    newGeoTransform[4] = 0;
    newGeoTransform[5] = adfGeoTransform[5];

    // 创建一个三通道的GeoTiff文件 
    poDstDS = poDriver->Create(outputDirectory.c_str(), Width, Height, 3, GDT_Byte, NULL);
    poDstDS->SetGeoTransform(newGeoTransform);
    poDstDS->SetProjection(pszSRS_WKT);
    // 将结果影像写入到GeoTiff文件中
    for (size_t i = 0; i < results.size(); i++) {
        // 将BGR影像转换为RGB影像
        cv::Mat rgbImage;
        cv::cvtColor(results[i], rgbImage, cv::COLOR_BGR2RGB);
        // 显示转换后的影像
        //cv::imshow("Result", rgbImage);
        //cv::waitKey(0);
        // 遍历每个像素
        for (int row = 0; row < rgbImage.rows; row++) {
            for (int col = 0; col < rgbImage.cols; col++) {
                cv::Vec3b pixel = rgbImage.at<cv::Vec3b>(row, col);

                // 如果像素的三个波段的值都为0，则跳过
                if (pixel[0] == 0 && pixel[1] == 0 && pixel[2] == 0) {
                    continue;
                } 

                // 在需要使用minGeoX和maxGeoY的地方
                double GeoX_jg = minGeoXs[i] + col * adfGeoTransform[1];
                double GeoY_jg = maxGeoYs[i] - row * -adfGeoTransform[5];
                // result中的像素坐标在结果画布上的坐标
                int dstCol = static_cast<int>(std::round((GeoX_jg - minX) / adfGeoTransform[1]));
                int dstRow = static_cast<int>(std::round((maxY - GeoY_jg) / -adfGeoTransform[5]));
                // cout<< "dstCol: " << dstCol << ", dstRow: " << dstRow << endl;
                // 如果像素的行列号超出了结果影像的范围，则跳过
                if (dstCol < 0 || dstCol >= Width || dstRow < 0 || dstRow >= Height) {
                    continue;
                }
                // 将红、绿、蓝三个波段的数据分别写入到GeoTiff文件中
                for (int band = 1; band <= 3; band++) {
                    GDALRasterBand* poDstBand = poDstDS->GetRasterBand(band);
                    uchar value = pixel[band - 1]; // 现在的通道顺序为RGB
                    // 写入像素值
                    poDstBand->RasterIO(GF_Write, dstCol, dstRow, 1, 1, &value, 1, 1, GDT_Byte, 0, 0);
                }
            }
        }

        /*double progress = 70 + static_cast<double>(i + 1) / results.size() * 29;
        emit progressChanged(std::round(progress));*/
    }
    // 设置NoData值
    for (int i = 1; i <= poDstDS->GetRasterCount(); i++)
    {
        GDALRasterBand* poBand = poDstDS->GetRasterBand(i);
        poBand->SetNoDataValue(0);
    }

    // emit progressChanged(100);

    // 释放资源
    GDALClose(poDstDS);
}

