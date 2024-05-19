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

	// ��ȡӰ��Ŀ�Ⱥ͸߶�
	int nXSize1 = poRefDataset->GetRasterXSize();
	int nYSize1 = poRefDataset->GetRasterYSize();
	int nXSize2 = poDataset->GetRasterXSize();
	int nYSize2 = poDataset->GetRasterYSize();

	// ��ȡӰ��Ĳ�����
	int nBands1 = poRefDataset->GetRasterCount();
	int nBands2 = poDataset->GetRasterCount();

	// ��ȡӰ��Ĳ�������
	std::vector<GDALRasterBand*> poBands1(nBands1);
	std::vector<GDALRasterBand*> poBands2(nBands2);
	for (int i = 0; i < nBands1; ++i) {
		poBands1[i] = poRefDataset->GetRasterBand(i + 1);
	}
	for (int i = 0; i < nBands2; ++i) {
		poBands2[i] = poDataset->GetRasterBand(i + 1);
	}

	// ��ȡӰ��ĵ���Χ
	double adfGeoTransform1[6];
	poRefDataset->GetGeoTransform(adfGeoTransform1);
	double adfGeoTransform2[6];
	poDataset->GetGeoTransform(adfGeoTransform2);

	// �жϷֱ����Ƿ���ͬ
	if (fabs(adfGeoTransform1[1] - adfGeoTransform2[1]) > 1e-6 || fabs(adfGeoTransform1[5] - adfGeoTransform2[5]) > 1e-6) {
		//std::cout << "Resolution is different" << std::endl;
		return;
	}

	// ����ÿ��Ӱ����ĸ��ǵĵ�������
	double minX1 = adfGeoTransform1[0];
	double maxX1 = adfGeoTransform1[0] + nXSize1 * adfGeoTransform1[1];
	double minY1 = adfGeoTransform1[3] + nYSize1 * adfGeoTransform1[5];
	double maxY1 = adfGeoTransform1[3];

	double minX2 = adfGeoTransform2[0];
	double maxX2 = adfGeoTransform2[0] + nXSize2 * adfGeoTransform2[1];
	double minY2 = adfGeoTransform2[3] + nYSize2 * adfGeoTransform2[5];
	double maxY2 = adfGeoTransform2[3];

	// �ҳ��ص�����ı߽�
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

		// �����ص�����Ŀ�Ⱥ͸߶�
		double width = overlapMaxX - overlapMinX;
		double height = overlapMaxY - overlapMinY;

		// ����һ����ά�������洢ÿ��Ӱ���ÿ�����ε�����ֵ
		std::vector<std::vector<int>> pixelValues1(3), pixelValues2(3);

		// ����һ���������洢ÿ�����ε�����ֵ
		std::vector<int> pixelValue1(3), pixelValue2(3);

		// ����һ��vector���洢ÿ�����ε���Ͻ��
		std::vector<Eigen::VectorXd> fitResults;

		// ����10�׵ļ�����ɵ㣬��һ����ƫ��5��
		double offset = 5.0; // ƫ����
		for (double x = overlapMinX + offset; x < overlapMaxX; x += 10) {
			for (double y = overlapMinY + offset; y < overlapMaxY; y += 10) {
				// �����ڵ�һ��Ӱ���е����к�
				int col1 = std::round((x - adfGeoTransform1[0]) / adfGeoTransform1[1]);
				int row1 = std::round((y - adfGeoTransform1[3]) / adfGeoTransform1[5]);

				// �����ڵڶ���Ӱ���е����к�
				int col2 = std::round((x - adfGeoTransform2[0]) / adfGeoTransform2[1]);
				int row2 = std::round((y - adfGeoTransform2[3]) / adfGeoTransform2[5]);

				// ��ÿ������ִ����ͬ�Ĳ���
				for (int i = 0; i < 3; ++i) {
					// ȡ��ÿ��Ӱ��ĵ�ǰ���ε�����ֵ
					poBands1[i]->RasterIO(GF_Read, col1, row1, 1, 1, &pixelValue1[i], 1, 1, GDT_Int32, 0, 0);
					poBands2[i]->RasterIO(GF_Read, col2, row2, 1, 1, &pixelValue2[i], 1, 1, GDT_Int32, 0, 0);
				}

				// ���������һ��Ӱ�������������ȡ��������ֵ��Ϊ0����ô��������һ�Ե�
				if ((pixelValue1[0] == 0 && pixelValue1[1] == 0 && pixelValue1[2] == 0) ||
					(pixelValue2[0] == 0 && pixelValue2[1] == 0 && pixelValue2[2] == 0)) {
					continue;
				}

				// ������ֵ��ӵ�������
				for (int i = 0; i < 3; ++i) {
					pixelValues1[i].push_back(pixelValue1[i]);
					pixelValues2[i].push_back(pixelValue2[i]);
				}
			}
		}

		// ���ÿ�����RGBֵ
		/*for (int j = 0; j < pixelValues1[0].size(); ++j) {
			std::cout << "Image 1 RGB: " << pixelValues1[0][j] << " " << pixelValues1[1][j] << " " << pixelValues1[2][j] << std::endl;
			std::cout << "Image 2 RGB: " << pixelValues2[0][j] << " " << pixelValues2[1][j] << " " << pixelValues2[2][j] << std::endl;
		}*/

		// ��ÿ������ִ����ͬ�Ĳ���
		for (int i = 0; i < 3; ++i) {
			// ������ֵת��ΪEigen����
			Eigen::VectorXd y(pixelValues1[i].size()), x(pixelValues2[i].size());
			for (int j = 0; j < pixelValues1[i].size(); ++j) {
				y[j] = pixelValues1[i][j];
				x[j] = pixelValues2[i][j];
			}

			// ����һ�������Ա����ͳ�����ľ���
			Eigen::MatrixXd A(x.size(), 2);
			A.col(0) = x;
			A.col(1).setOnes();

			// ʹ��BDCSVD������С�������
			Eigen::VectorXd result = A.bdcSvd(Eigen::ComputeThinU | Eigen::ComputeThinV).solve(y);

			// ����Ͻ����ӵ�������
			fitResults.push_back(result);

			// ����������
			Eigen::VectorXd error = y - A * result;

			// �������ľ�ֵ
			double mean_error = error.mean();

			// �������ı�׼��
			double std_dev_error = std::sqrt((error.array() - mean_error).square().sum() / error.size());


			// �����Ͻ��
			/*std::cout << "Band " << i + 1 << " fit: y = " << result[0] << "x + " << result[1] << std::endl;
			std::cout << "Mean error: " << mean_error << std::endl;
			std::cout << "Standard deviation of error: " << std_dev_error << std::endl;*/
		}

		// ����һ����tifӰ�񱣴�У�����Ӱ��
		GDALDriver* poDriver = GetGDALDriverManager()->GetDriverByName("GTiff");
		if (poDriver == NULL) {
			//std::cout << "Driver not found" << std::endl;
			return;
		}

		// ��ȡӰ����ļ���
		std::filesystem::path imagePathPath(imagePath);
		std::string filename = imagePathPath.stem().string();

		// ����һ���µ�Ӱ��
		std::string outputImagePath = outputDirectory + "/" + filename + "_corrected.tif";
		GDALDataset* poDstDS = poDriver->Create(outputImagePath.c_str(), nXSize2, nYSize2, nBands2, GDT_Byte, NULL);
		if (poDstDS == NULL) {
			//std::cout << "Create failed" << std::endl;
			return;
		}

		// ��ÿ������ִ����ͬ�Ĳ���
		for (int i = 0; i < nBands2; ++i) {
			// ��ȡ��Ӱ��ĵ�ǰ����
			GDALRasterBand* poBand = poDstDS->GetRasterBand(i + 1);

			// ����һ���������洢����ֵ
			std::vector<int> pixelValues(nXSize2 * nYSize2);

			// ��ȡԭʼӰ��ĵ�ǰ���ε�����ֵ
			poBands2[i]->RasterIO(GF_Read, 0, 0, nXSize2, nYSize2, pixelValues.data(), nXSize2, nYSize2, GDT_Int32, 0, 0);

			// ��ÿ������Ӧ����Ͻ��
			for (int& pixelValue : pixelValues) {
				pixelValue = static_cast<int>(pixelValue * fitResults[i][0] + fitResults[i][1]);
				pixelValue = std::clamp(pixelValue, 0, 255);
			}
			// ������ֵת��Ϊ�ֽ���ʽ
			std::vector<unsigned char> bytePixelValues(pixelValues.begin(), pixelValues.end());

			// ��У���������ֵд���µ�tif�ļ�
			poBand->RasterIO(GF_Write, 0, 0, nXSize2, nYSize2, bytePixelValues.data(), nXSize2, nYSize2, GDT_Byte, 0, 0);
		}

		// ������Ӱ��ĵ���任������ͶӰ��Ϣ
		poDstDS->SetGeoTransform(adfGeoTransform2);
		poDstDS->SetProjection(poDataset->GetProjectionRef());
	}
}