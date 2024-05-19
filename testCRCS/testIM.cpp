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
	// ���캯��
}


static void draw_point(cv::Mat& img, cv::Point2f fp, cv::Scalar color)
{
    cv::circle(img, fp, 2, color, cv::FILLED, cv::LINE_AA, 0);
}

// ���ڱ�ʾ��������Ľṹ��
struct GeoCoordinate {
    double latitude;
    double longitude;
};
struct VertexWithCenter {
    cv::Point2f vertex;
    cv::Point2f center;
};
// ���ڱ�ʾң��Ӱ�����
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
        // ���ļ��ж�ȡӰ�񣬻�ȡ���������
        GDALAllRegister();
        poDataset = (GDALDataset*)GDALOpen(filePath.c_str(), GA_ReadOnly);
        if (poDataset == NULL)
        {
            std::cerr << "Error: file open failed!" << std::endl; // �ļ���ʧ�ܣ��������
        }

        // ��ȡӰ��ĵ�������
        if (poDataset->GetGeoTransform(adfGeoTransform) == CE_None)
        {
            // ��ȡӰ��ĵ�������
            double Xmin = minCoordinate.longitude = adfGeoTransform[0];
            double Ymax = minCoordinate.latitude = adfGeoTransform[3];
            double Xmax = maxCoordinate.longitude = adfGeoTransform[0] + adfGeoTransform[1] * poDataset->GetRasterXSize();
            double Ymin = maxCoordinate.latitude = adfGeoTransform[3] + adfGeoTransform[5] * poDataset->GetRasterYSize();
            // ����Ӱ���ͶӰ����
            centerCoordinate.longitude = (Xmin + Xmax) / 2;
            centerCoordinate.latitude = (Ymin + Ymax) / 2;
        }
        else
        {
            std::cerr << "Error: get geo transform failed!" << std::endl; // ��ȡ��������ʧ�ܣ��������
        }

        // ��ȡͼ������
        GDALRasterBand* poBand = poDataset->GetRasterBand(1);
        int nXSize = poBand->GetXSize();
        int nYSize = poBand->GetYSize();

        image = cv::Mat(nYSize, nXSize, CV_8UC3);

        // Opencv��ȡӰ������
        image = imread(filePath, cv::IMREAD_COLOR);


        //poBand->RasterIO(GF_Read, 0, 0, nXSize, nYSize, image.data, nXSize, nYSize, GDT_Byte, 0, 0);

        GDALClose(poDataset);
    }
};

// ��ȡ����Ӱ��ĺ���
std::vector<RemoteSensingImage> readImages(const std::vector<std::string>& filePaths) {
    std::vector<RemoteSensingImage> images;
    for (const auto& filePath : filePaths) {
        RemoteSensingImage image(filePath);
        images.push_back(image); // ����ȡ��Ӱ����ӵ�images��
    }
    return images;
}
// ����������
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

        // ����������
        if (rect.contains(pt[0]) && rect.contains(pt[1]) && rect.contains(pt[2]))
        {
            cv::line(img, pt[0], pt[1], delaunay_color, 1, cv::LINE_AA, 0);
            cv::line(img, pt[1], pt[2], delaunay_color, 1, cv::LINE_AA, 0);
            cv::line(img, pt[2], pt[0], delaunay_color, 1, cv::LINE_AA, 0);
        }
    }
}
// ����άŵͼ
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

    return facets; // ���ض���εĶ�������
}


void testIM::process()
{
    std::vector<RemoteSensingImage> images;
    double minX = std::numeric_limits<double>::max();
    double maxX = std::numeric_limits<double>::min();
    double minY = std::numeric_limits<double>::max();
    double maxY = std::numeric_limits<double>::min();
    std::vector<cv::Mat> results;

    // ���������������洢ÿ����Ĥ������������������minGeoX��maxGeoY
    std::vector<double> minGeoXs;
    std::vector<double> maxGeoYs;

    int W = 0;
    int H = 0;
    // ���������������ŵĴ���
    //std::string win_delaunay = "Delaunay";
    //std::string win_voronoi = "Voronoi";
    //std::string win_mask = "Mask";
    //std::string win_transition = "Transition";
    //cv::namedWindow(win_delaunay, cv::WINDOW_NORMAL);
    //cv::namedWindow(win_voronoi, cv::WINDOW_NORMAL);
    //cv::namedWindow(win_mask, cv::WINDOW_NORMAL);
    //cv::namedWindow(win_transition, cv::WINDOW_NORMAL);
    //cv::namedWindow("Result", cv::WINDOW_NORMAL);
    //��ȡӰ���ļ�����������Ҫ�ӿڣ������ļ���ַ
    std::vector<std::string> filePaths;
    //std::string directoryPath = inputDirectory; // �ļ���·��
    //for (const auto& entry : std::filesystem::directory_iterator(directoryPath)) {
    //    if (entry.path().extension() == ".tif") { // ֻ����.tif�ļ�
    //        filePaths.push_back(entry.path().string());
    //    }
    //}
    images = readImages(inputFiles);
    filePaths = inputFiles;


    // �򿪵�һ��Ӱ��,��ȡ�ֱ���
    GDALDataset* poSrcDS = (GDALDataset*)GDALOpen(filePaths[0].c_str(), GA_ReadOnly);

    // ��ȡ����任��ͶӰ����
    double adfGeoTransform[6];
    poSrcDS->GetGeoTransform(adfGeoTransform);
    const char* pszSRS_WKT = poSrcDS->GetProjectionRef();
    // �������ս��Ӱ��ĵ���Χ
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
    // ������Ӱ��ķ�Χ����͸�
    int Width = static_cast<int>(std::round(maxX - minX) / adfGeoTransform[1]);
    int Height = static_cast<int>(std::round(maxY - minY) / -adfGeoTransform[5]);

    // ����άŵͼ�����ķֱ���
    int width = 2000;
    int height = 2000;
    double xsize = (maxX - minX) / 2000;
    double ysize = -(maxY - minY) / 2000;
    // ���ú���
    bool animate = true;
    cv::Scalar delaunay_color(255, 255, 255), points_color(0, 0, 255);
    cv::Mat img = cv::Mat::zeros(height, width, CV_8UC3);
    cv::Mat img_ori = img.clone();
    cv::Size size = img.size();
    cv::Rect rect(0, 0, size.width, size.height);
    cv::Subdiv2D subdiv(rect);

    // �������ӵ�
    std::vector<cv::Point2f> points;
    for (const auto& image : images)
    {
        cv::Point2f pt;
        pt.x = static_cast<float>((image.centerCoordinate.longitude - minX) / xsize);
        pt.y = static_cast<float>((image.centerCoordinate.latitude - maxY) / ysize);
        points.push_back(pt);
    }
    // ��ʾ��������ӵ�����
    //for (size_t i = 0; i < points.size(); i++)
    //{
    //    std::cout << "Point " << i << ": " << points[i] << std::endl;
    //}
    // �����ӵ����ָ����
    for (int i = 0; i < points.size(); i++)
    {
        subdiv.insert(points[i]);

        if (animate)
        {
            cv::Mat img_copy = img_ori.clone();

            // ����������
            draw_delaunay(img_copy, subdiv, delaunay_color);
            // cv::imshow(win_delaunay, img_copy);
            waitKey(100);
        }
    }
    draw_delaunay(img, subdiv, delaunay_color);

    // ���Ƶ�
    for (size_t i = 0; i < points.size(); i++)
    {
        draw_point(img, points[i], points_color);
    }
    // ���� Voronoi ͼ����ȡ����εĶ�������
    cv::Mat img_voronoi = img.clone();
    std::vector<std::vector<cv::Point2f>> voronoiVertices = draw_voronoi(img_voronoi, subdiv);
    // cv::imshow(win_voronoi, img_voronoi);

    // emit progressChanged(10);

         // ��������Ӱ��
    for (size_t i = 0; i < images.size(); i++) {
        // ת����������Ϊ���������Ӱ��ƽ���ϵ�����
        std::vector<cv::Point> vertices;
        double minGeoX = std::numeric_limits<double>::max();
        double maxGeoX = std::numeric_limits<double>::min();
        double minGeoY = std::numeric_limits<double>::max();
        double maxGeoY = std::numeric_limits<double>::min();

        // ������ǰ����εĶ���    
        for (const auto& vertex : voronoiVertices[i]) {

            // ����άŵͼ�������С����
            double VnuoXmin = 0 * xsize + minX;
            double VnuoYmax = maxY - 0 * ysize;
            double VnuoXmax = 2000 * xsize + minX;
            double VnuoYmin = maxY - 2000 * ysize;
            //cout << "VnuoXmin: " << VnuoXmin << ", VnuoYmax: " << VnuoYmax << ", VnuoXmax: " << VnuoXmax << ", VnuoYmin: " << VnuoYmin << endl;

            double geoX = vertex.x * xsize + minX;//�����������
            double geoY = maxY - vertex.y * ysize;
            // ������geo���滻�����
            double GeoX = geoX;
            double GeoY = geoY;
            //cout << "geo0X: " << geoX << ", geo0Y: " << geoY << endl;
            // �������εĶ������곬����άŵͼ�ķ�Χ������������άŵͼ�ķ�Χ��
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

            // Ѱ��������С�Ķ���ε�geoX��geoY
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
            // �Ƚϵ�i��Ӱ��������С�ĵ�������
            double imageMinGeoX = images[i].minCoordinate.longitude;
            double imageMaxGeoX = images[i].maxCoordinate.longitude;
            double imageMinGeoY = images[i].maxCoordinate.latitude;
            double imageMaxGeoY = images[i].minCoordinate.latitude;
            // �ҳ���Ĥ�ļ��ķ�Χ
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
            // �������ε���ƽ�涥������
            double DimgX = (GeoX - minGeoX) / adfGeoTransform[1];
            double DimgY = (maxGeoY - GeoY) / adfGeoTransform[5];
            //cout<< "geoX: " << GeoX << ", geoY: " << GeoY << endl;
            //cout << "DimgX: " << DimgX << ", DimgY: " << DimgY << endl;
            vertices.push_back(cv::Point(static_cast<int>(DimgX), static_cast<int>(DimgY)));
        }
        // �ڴ���ÿ������κ󣬽�minGeoX��maxGeoY��ӵ�minGeoXs��maxGeoYs��
        minGeoXs.push_back(minGeoX);
        maxGeoYs.push_back(maxGeoY);

        // ������Ĥ�����Ŀ�Ⱥ͸߶�
        W = static_cast<int>(std::round(maxGeoX - minGeoX) / adfGeoTransform[1]);
        H = static_cast<int>(std::round(maxGeoY - minGeoY) / -adfGeoTransform[5]);
        // ������Ĥ����
        cv::Mat mask = cv::Mat::zeros(H, W, CV_8UC1);
        // ����Ĥ�����ϸ��ݶ���εĶ������괴����Ĥ
        cv::fillPoly(mask, std::vector<std::vector<cv::Point>>{vertices}, cv::Scalar(255));
        // ��ʾ��Ĥ����
        //cv::imshow(win_mask, mask);
        // ������������
        cv::Mat transition = cv::Mat::zeros(H, W, CV_8UC3);
        // ����i��Ӱ�������ؼ��ص�����������
        for (int row = 0; row < images[i].image.rows; row++) {
            for (int col = 0; col < images[i].image.cols; col++) {
                // ����ͶӰ�����ڲ������������к�
                int x = static_cast<int>((images[i].centerCoordinate.longitude - minGeoX) / adfGeoTransform[1]);
                int y = static_cast<int>((images[i].centerCoordinate.latitude - maxGeoY) / adfGeoTransform[5]);
                // �����������ڲ������������к�
                int N = static_cast<int>(x + col - images[i].image.cols / 2);
                int M = static_cast<int>(y + row - images[i].image.rows / 2);
                if (N >= 0 && N < transition.cols && M >= 0 && M < transition.rows) {
                    // ��ȡԭʼͼ�������ֵ
                    if (row < images[i].image.rows && col < images[i].image.cols) {
                        cv::Vec3b pixel_value = images[i].image.at<cv::Vec3b>(row, col);
                        // ��ԭʼͼ�������ֵ�������ɻ����Ķ�Ӧ����
                        transition.at<cv::Vec3b>(M, N) = pixel_value;
                    }
                }
            }
        }

        // ������Ĥ����       
        cv::Mat result;
        cv::Mat mask_three_channels;// ����Ĥ�ļ��ĳ���ͨ��
        cv::cvtColor(mask, mask_three_channels, cv::COLOR_GRAY2BGR);
        cv::bitwise_and(transition, mask_three_channels, result);
        // ����ƴ����
        // �ҵ���Ĥ�ı߽�ɫ
        std::vector<std::vector<cv::Point>> contours;
        std::vector<cv::Vec4i> hierarchy;
        cv::findContours(mask, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
        // ����һ��ȫ�ڵ�ͼ�����ڻ���ƴ����
        cv::Mat stitchLine = cv::Mat::zeros(mask.size(), CV_8UC3);
        // ���ư�ɫ��ƴ����
        cv::drawContours(stitchLine, contours, -1, cv::Scalar(255, 255, 255), 1, cv::LINE_8, hierarchy, 0);
        // ��ƴ����Ӧ�õ����ͼ����
        cv::bitwise_or(result, stitchLine, result);

        // ��ʾת���Ĳ�������
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

    // ���õ���������Ϣ
    double newGeoTransform[6];
    newGeoTransform[0] = minX;
    newGeoTransform[1] = adfGeoTransform[1];
    newGeoTransform[2] = 0;
    newGeoTransform[3] = maxY;
    newGeoTransform[4] = 0;
    newGeoTransform[5] = adfGeoTransform[5];

    // ����һ����ͨ����GeoTiff�ļ� 
    poDstDS = poDriver->Create(outputDirectory.c_str(), Width, Height, 3, GDT_Byte, NULL);
    poDstDS->SetGeoTransform(newGeoTransform);
    poDstDS->SetProjection(pszSRS_WKT);
    // �����Ӱ��д�뵽GeoTiff�ļ���
    for (size_t i = 0; i < results.size(); i++) {
        // ��BGRӰ��ת��ΪRGBӰ��
        cv::Mat rgbImage;
        cv::cvtColor(results[i], rgbImage, cv::COLOR_BGR2RGB);
        // ��ʾת�����Ӱ��
        //cv::imshow("Result", rgbImage);
        //cv::waitKey(0);
        // ����ÿ������
        for (int row = 0; row < rgbImage.rows; row++) {
            for (int col = 0; col < rgbImage.cols; col++) {
                cv::Vec3b pixel = rgbImage.at<cv::Vec3b>(row, col);

                // ������ص��������ε�ֵ��Ϊ0��������
                if (pixel[0] == 0 && pixel[1] == 0 && pixel[2] == 0) {
                    continue;
                } 

                // ����Ҫʹ��minGeoX��maxGeoY�ĵط�
                double GeoX_jg = minGeoXs[i] + col * adfGeoTransform[1];
                double GeoY_jg = maxGeoYs[i] - row * -adfGeoTransform[5];
                // result�е����������ڽ�������ϵ�����
                int dstCol = static_cast<int>(std::round((GeoX_jg - minX) / adfGeoTransform[1]));
                int dstRow = static_cast<int>(std::round((maxY - GeoY_jg) / -adfGeoTransform[5]));
                // cout<< "dstCol: " << dstCol << ", dstRow: " << dstRow << endl;
                // ������ص����кų����˽��Ӱ��ķ�Χ��������
                if (dstCol < 0 || dstCol >= Width || dstRow < 0 || dstRow >= Height) {
                    continue;
                }
                // ���졢�̡����������ε����ݷֱ�д�뵽GeoTiff�ļ���
                for (int band = 1; band <= 3; band++) {
                    GDALRasterBand* poDstBand = poDstDS->GetRasterBand(band);
                    uchar value = pixel[band - 1]; // ���ڵ�ͨ��˳��ΪRGB
                    // д������ֵ
                    poDstBand->RasterIO(GF_Write, dstCol, dstRow, 1, 1, &value, 1, 1, GDT_Byte, 0, 0);
                }
            }
        }

        /*double progress = 70 + static_cast<double>(i + 1) / results.size() * 29;
        emit progressChanged(std::round(progress));*/
    }
    // ����NoDataֵ
    for (int i = 1; i <= poDstDS->GetRasterCount(); i++)
    {
        GDALRasterBand* poBand = poDstDS->GetRasterBand(i);
        poBand->SetNoDataValue(0);
    }

    // emit progressChanged(100);

    // �ͷ���Դ
    GDALClose(poDstDS);
}

