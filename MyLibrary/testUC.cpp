#include "testUC.h"
#include <iostream>
#include <gdal_priv.h>
#include <vector>
#include <Eigen/Dense>
#include <algorithm>
#include <string>
#include "cpl_conv.h"
#include <cmath>
#include <filesystem>

void processImages(GDALDataset*& poRefDataset, GDALDataset* poDataset, const std::string& imagePath, const std::string& outputDirectory) {

	// 获取影像的宽度和高度
	int nXSize1 = poRefDataset->GetRasterXSize();
	int nYSize1 = poRefDataset->GetRasterYSize();
	int nXSize2 = poDataset->GetRasterXSize();
	int nYSize2 = poDataset->GetRasterYSize();

	// 获取影像的波段数
	int nBands1 = poRefDataset->GetRasterCount();
	int nBands2 = poDataset->GetRasterCount();

	// 获取影像的波段数据
	std::vector<GDALRasterBand*> poBands1(nBands1);
	std::vector<GDALRasterBand*> poBands2(nBands2);
	for (int i = 0; i < nBands1; ++i) {
		poBands1[i] = poRefDataset->GetRasterBand(i + 1);
	}
	for (int i = 0; i < nBands2; ++i) {
		poBands2[i] = poDataset->GetRasterBand(i + 1);
	}

	// 读取影像的地理范围
	double adfGeoTransform1[6];
	poRefDataset->GetGeoTransform(adfGeoTransform1);
	double adfGeoTransform2[6];
	poDataset->GetGeoTransform(adfGeoTransform2);

	// 判断分辨率是否相同
	if (fabs(adfGeoTransform1[1] - adfGeoTransform2[1]) > 1e-6 || fabs(adfGeoTransform1[5] - adfGeoTransform2[5]) > 1e-6) {
		//std::cout << "Resolution is different" << std::endl;
		return;
	}

	// 计算每幅影像的四个角的地理坐标
	double minX1 = adfGeoTransform1[0];
	double maxX1 = adfGeoTransform1[0] + nXSize1 * adfGeoTransform1[1];
	double minY1 = adfGeoTransform1[3] + nYSize1 * adfGeoTransform1[5];
	double maxY1 = adfGeoTransform1[3];

	double minX2 = adfGeoTransform2[0];
	double maxX2 = adfGeoTransform2[0] + nXSize2 * adfGeoTransform2[1];
	double minY2 = adfGeoTransform2[3] + nYSize2 * adfGeoTransform2[5];
	double maxY2 = adfGeoTransform2[3];

	// 找出重叠区域的边界
	double overlapMinX = std::max(minX1, minX2);
	double overlapMaxX = std::min(maxX1, maxX2);
	double overlapMinY = std::max(minY1, minY2);
	double overlapMaxY = std::min(maxY1, maxY2);

	if (overlapMinX >= overlapMaxX || overlapMinY >= overlapMaxY) {
		//std::cout << "No overlap" << std::endl;
		return;
	}
	else {
		//std::cout << "Overlap region: (" << overlapMinX << ", " << overlapMinY << ") to (" << overlapMaxX << ", " << overlapMaxY << ")" << std::endl;

		// 计算重叠区域的宽度和高度
		double width = overlapMaxX - overlapMinX;
		double height = overlapMaxY - overlapMinY;

		// 创建一个二维数组来存储每个影像的每个波段的像素值
		std::vector<std::vector<int>> pixelValues1(3), pixelValues2(3);

		// 创建一个数组来存储每个波段的像素值
		std::vector<int> pixelValue1(3), pixelValue2(3);

		// 创建一个vector来存储每个波段的拟合结果
		std::vector<Eigen::VectorXd> fitResults;

		// 按照10米的间隔生成点，第一个点偏移5米
		double offset = 5.0; // 偏移量
		for (double x = overlapMinX + offset; x < overlapMaxX; x += 10) {
			for (double y = overlapMinY + offset; y < overlapMaxY; y += 10) {
				// 计算在第一张影像中的行列号
				int col1 = std::round((x - adfGeoTransform1[0]) / adfGeoTransform1[1]);
				int row1 = std::round((y - adfGeoTransform1[3]) / adfGeoTransform1[5]);

				// 计算在第二张影像中的行列号
				int col2 = std::round((x - adfGeoTransform2[0]) / adfGeoTransform2[1]);
				int row2 = std::round((y - adfGeoTransform2[3]) / adfGeoTransform2[5]);

				// 对每个波段执行相同的操作
				for (int i = 0; i < 3; ++i) {
					// 取出每幅影像的当前波段的像素值
					poBands1[i]->RasterIO(GF_Read, col1, row1, 1, 1, &pixelValue1[i], 1, 1, GDT_Int32, 0, 0);
					poBands2[i]->RasterIO(GF_Read, col2, row2, 1, 1, &pixelValue2[i], 1, 1, GDT_Int32, 0, 0);
				}

				// 如果在任意一张影像的三个波段上取出的像素值都为0，那么就跳过这一对点
				if ((pixelValue1[0] == 0 && pixelValue1[1] == 0 && pixelValue1[2] == 0) ||
					(pixelValue2[0] == 0 && pixelValue2[1] == 0 && pixelValue2[2] == 0)) {
					continue;
				}

				// 将像素值添加到数组中
				for (int i = 0; i < 3; ++i) {
					pixelValues1[i].push_back(pixelValue1[i]);
					pixelValues2[i].push_back(pixelValue2[i]);
				}
			}
		}

		// 输出每个点的RGB值
		/*for (int j = 0; j < pixelValues1[0].size(); ++j) {
			std::cout << "Image 1 RGB: " << pixelValues1[0][j] << " " << pixelValues1[1][j] << " " << pixelValues1[2][j] << std::endl;
			std::cout << "Image 2 RGB: " << pixelValues2[0][j] << " " << pixelValues2[1][j] << " " << pixelValues2[2][j] << std::endl;
		}*/

		// 对每个波段执行相同的操作
		for (int i = 0; i < 3; ++i) {
			// 将像素值转换为Eigen向量
			Eigen::VectorXd y(pixelValues1[i].size()), x(pixelValues2[i].size());
			for (int j = 0; j < pixelValues1[i].size(); ++j) {
				y[j] = pixelValues1[i][j];
				x[j] = pixelValues2[i][j];
			}

			// 创建一个包含自变量和常数项的矩阵
			Eigen::MatrixXd A(x.size(), 2);
			A.col(0) = x;
			A.col(1).setOnes();

			// 使用BDCSVD进行最小二乘拟合
			Eigen::VectorXd result = A.bdcSvd(Eigen::ComputeThinU | Eigen::ComputeThinV).solve(y);

			// 将拟合结果添加到数组中
			fitResults.push_back(result);

			// 计算拟合误差
			Eigen::VectorXd error = y - A * result;

			// 计算误差的均值
			double mean_error = error.mean();

			// 计算误差的标准差
			double std_dev_error = std::sqrt((error.array() - mean_error).square().sum() / error.size());


			// 输出拟合结果
			/*std::cout << "Band " << i + 1 << " fit: y = " << result[0] << "x + " << result[1] << std::endl;
			std::cout << "Mean error: " << mean_error << std::endl;
			std::cout << "Standard deviation of error: " << std_dev_error << std::endl;*/
		}

		// 创建一个新tif影像保存校正后的影像
		GDALDriver* poDriver = GetGDALDriverManager()->GetDriverByName("GTiff");
		if (poDriver == NULL) {
			//std::cout << "Driver not found" << std::endl;
			return;
		}

		// 获取影像的文件名
		std::filesystem::path imagePathPath(imagePath);
		std::string filename = imagePathPath.stem().string();

		// 创建一个新的影像
		std::string outputImagePath = outputDirectory + "/" + filename + "_corrected.tif";
		GDALDataset* poDstDS = poDriver->Create(outputImagePath.c_str(), nXSize2, nYSize2, nBands2, GDT_Byte, NULL);
		if (poDstDS == NULL) {
			//std::cout << "Create failed" << std::endl;
			return;
		}

		// 对每个波段执行相同的操作
		for (int i = 0; i < nBands2; ++i) {
			// 获取新影像的当前波段
			GDALRasterBand* poBand = poDstDS->GetRasterBand(i + 1);

			// 创建一个数组来存储像素值
			std::vector<int> pixelValues(nXSize2 * nYSize2);

			// 读取原始影像的当前波段的像素值
			poBands2[i]->RasterIO(GF_Read, 0, 0, nXSize2, nYSize2, pixelValues.data(), nXSize2, nYSize2, GDT_Int32, 0, 0);

			// 对每个像素应用拟合结果
			for (int& pixelValue : pixelValues) {
				pixelValue = static_cast<int>(pixelValue * fitResults[i][0] + fitResults[i][1]);
				pixelValue = std::clamp(pixelValue, 0, 255);
			}
			// 将像素值转换为字节形式
			std::vector<unsigned char> bytePixelValues(pixelValues.begin(), pixelValues.end());

			// 将校正后的像素值写入新的tif文件
			poBand->RasterIO(GF_Write, 0, 0, nXSize2, nYSize2, bytePixelValues.data(), nXSize2, nYSize2, GDT_Byte, 0, 0);
		}

		// 设置新影像的地理变换参数和投影信息
		poDstDS->SetGeoTransform(adfGeoTransform2);
		poDstDS->SetProjection(poDataset->GetProjectionRef());
	}
}