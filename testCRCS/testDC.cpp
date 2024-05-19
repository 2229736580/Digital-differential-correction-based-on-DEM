#include "testDC.h"

#include <fstream>
#include <sstream>

using namespace cv;
using namespace std;

testDC::testDC(const std::string& imagePath, const std::string& csvPath, const std::string& demPath, const std::string& outputpath)
    : imagePath(imagePath), csvPath(csvPath), demPath(demPath), outputpath(outputpath) 
{
    // ���캯��
}

void testDC::process(double re) {
	std::vector<std::string> imagePaths = {imagePath};

	std::map<std::string, ImageData> data = readCSV(csvPath);

	// ��ȡDEM����
	GDALDataset* poDataset;
	GDALAllRegister();

	// ֧������·��
	CPLSetConfigOption("GDAL_FILENAME_IS_UTF8", "YES");

	// �����ݼ�
	poDataset = (GDALDataset*)GDALOpen(demPath.c_str(), GA_ReadOnly);
	if (poDataset == NULL) {
		cout << "Error: Failed to open DEM file." << endl;
	}

	// ��ȡDEM���ݼ��Ļ�����Ϣ
	double adfGeoTransform[6];
	poDataset->GetGeoTransform(adfGeoTransform);

	std::unique_ptr<GDALDataset> dataset(poDataset); //�Զ��ͷ���Դ

	// ����ڲ�
	double f = 25.1150;
	double pixel_size = 0.0039;

	// ���÷ֱ���
	double resolution = re;

	for (const auto& imagePath : imagePaths) {

		// ��ȡԭʼͼ��
		Mat src = imread(imagePath);

		// ���ͼ���Ƿ���ȷ����
		if (src.empty()) {
			cout << "Error: Failed to open image file." << endl;
		}

		std::string imageName = getFilenameWithoutExtension(imagePath);

		// ��ȡ��Ӧ����
		const auto& item = data[imageName];

		double Xs = item.Xs;
		double Ys = item.Ys;
		double Zs = item.Zs;
		double a1 = item.R[0], a2 = item.R[3], a3 = item.R[6];
		double b1 = item.R[1], b2 = item.R[4], b3 = item.R[7];
		double c1 = item.R[2], c2 = item.R[5], c3 = item.R[8];

		// ��ȡͼ��ߴ�
		int width = src.cols;
		int height = src.rows;

		float Z = 638.0;

		double x_values[] = { 11.70, -11.70 };
		double y_values[] = { 7.80, -7.80 };

		std::vector<Point3D> points;

		// ȷ��ͶӰ��Χ
		for (int i = 0; i < 2; i++) {
			for (int j = 0; j < 2; j++) {

				double x = x_values[i];
				double y = y_values[j];

				double A = a1 * x + a2 * y - a3 * f;
				double B = b1 * x + b2 * y - b3 * f;
				double C = c1 * x + c2 * y - c3 * f;

				double X, Y;

				// ����Z��ֵΪ638
				Z = 638.0;
				// ��������µ�X��Yֵ
				bool shouldBreak = false;
				for (int k = 0; k < 50; k++) {

					X = (Z - Zs) * A / C + Xs;
					Y = (Z - Zs) * B / C + Ys;

					// ��DEM��˫���Բ�ֵ��ȡ�µ�Zֵ
					double Xoff = (X - adfGeoTransform[0]) / adfGeoTransform[1];
					double Yoff = (Y - adfGeoTransform[3]) / adfGeoTransform[5];

					// ����Ƿ���DEM��Χ��
					if (Xoff >= 0 && Xoff < poDataset->GetRasterXSize() && Yoff >= 0 && Yoff < poDataset->GetRasterYSize()) {
						// ˫���Բ�ֵ
						int xoff = (int)Xoff;
						int yoff = (int)Yoff;
						float u = Xoff - xoff;
						float v = Yoff - yoff;

						float z[4];
						int m[4] = { 0, 1, 0, 1 };
						int n[4] = { 0, 0, 1, 1 };

						for (int i = 0; i < 4; i++) {
							poDataset->GetRasterBand(1)->RasterIO(GF_Read, xoff + m[i], yoff + n[i], 1, 1, &z[i], 1, 1, GDT_Float32, 0, 0);
							// ���ȡ��һ��ֵΪ-32767����һ����Чֵ������shouldBreakΪtrue��ֹͣ��ǰѭ��
							if (z[i] == -32767) {
								shouldBreak = true;
								break;
							}
						}

						if (shouldBreak) {
							break;
						}

						float Z1 = (1 - u) * (1 - v) * z[0];
						float Z2 = u * (1 - v) * z[1];
						float Z3 = (1 - u) * v * z[2];
						float Z4 = u * v * z[3];

						float newZ = Z1 + Z2 + Z3 + Z4;

						float deltaZ = newZ - Z;

						Z = newZ;
						// ����µ�Zֵ��ԭ����Zֵ�Ĳ�С��0.00001����ֹͣѭ��
						if (abs(deltaZ) < 0.00001) {
							break;
						}
					}
					else {
						break;
					}
				}
				Point3D point;
				point.X = X;
				point.Y = Y;
				point.Z = Z;
				points.push_back(point);
			}
		}

		// �ҵ���һ��Zֵ������638�ĵ�
		double lastValidZ = 638.0;
		for (const auto& point : points) {
			if (point.Z != 638.0) {
				lastValidZ = point.Z;
				break;
			}
		}

		// ����points��������Zֵ����638�ĵ��Zֵ����ΪlastValidZ
		for (auto& point : points) {
			if (point.Z == 638.0) {
				point.Z = lastValidZ;
			}
			else {
				lastValidZ = point.Z;
			}
		}

		// ������������6λС��
		/*for (int i = 0; i < points.size(); i++) {
			cout << "X: " << fixed << setprecision(6) << points[i].X << ", Y: " << fixed << setprecision(6) << points[i].Y << ", Z: " << fixed << setprecision(6) << points[i].Z << endl;
		}*/

		auto xMinMax = std::minmax_element(points.begin(), points.end(), [](const Point3D& a, const Point3D& b) {return a.X < b.X; });
		auto yMinMax = std::minmax_element(points.begin(), points.end(), [](const Point3D& a, const Point3D& b) {return a.Y < b.Y; });

		double Xmin = xMinMax.first->X;
		double Xmax = xMinMax.second->X;
		double Ymin = yMinMax.first->Y;
		double Ymax = yMinMax.second->Y;

		//cout << "Xmin: " << fixed << setprecision(6) << Xmin << ", Xmax: " << fixed << setprecision(6) << Xmax << ", Ymin: " << fixed << setprecision(6) << Ymin << ", Ymax: " << fixed << setprecision(6) << Ymax << endl;

		// ���У����Ӱ��ķ�Χ
		double deltaX = Xmax - Xmin;
		double deltaY = Ymax - Ymin;

		int cols = (int)(deltaX / resolution) + 1; //����
		int rows = (int)(deltaY / resolution) + 1; //����

		//cout << "Cols: " << cols << ", Rows: " << rows << endl;

		// ����У�����Ӱ��
		Mat dst(rows, cols, CV_8UC3, Scalar(0, 0, 0));

		// �����i�е�j�е�����
		for (int i = 0; i < cols; i++) {
			for (int j = 0; j < rows; j++) {

				double X_dst = Xmin - resolution / 2 + i * resolution;
				double Y_dst = Ymin - resolution / 2 + j * resolution;

				float Z_dst = 0.0;

				// ��DEM��˫���Բ�ֵ��ȡ�߳�
				double Xoff_dst = (X_dst - adfGeoTransform[0]) / adfGeoTransform[1];
				double Yoff_dst = (Y_dst - adfGeoTransform[3]) / adfGeoTransform[5];

				// ����Ƿ���DEM��Χ��
				if (Xoff_dst < 0 || Xoff_dst >= poDataset->GetRasterXSize() - 1 || Yoff_dst < 0 || Yoff_dst >= poDataset->GetRasterYSize() - 1) {
					dst.at<Vec3b>(rows - 1 - j, i) = Vec3b(0, 0, 0);
					continue;
				}
				else {
					// ˫���Բ�ֵ
					int xoff_dst = (int)Xoff_dst;
					int yoff_dst = (int)Yoff_dst;
					float u_dst = Xoff_dst - xoff_dst;
					float v_dst = Yoff_dst - yoff_dst;

					float z_dst[4];
					int m_dst[4] = { 0, 1, 0, 1 };
					int n_dst[4] = { 0, 0, 1, 1 };

					bool allNan = false;

					for (int i = 0; i < 4; i++) {
						poDataset->GetRasterBand(1)->RasterIO(GF_Read, xoff_dst + m_dst[i], yoff_dst + n_dst[i], 1, 1, &z_dst[i], 1, 1, GDT_Float32, 0, 0);
						if (z_dst[i] == -32767) {
							allNan = true; 
							break;
						}
					}


					if (allNan) {
						dst.at<Vec3b>(rows - 1 - j, i) = Vec3b(0, 0, 0);
						continue;
					}

					float Z1_dst = (1 - u_dst) * (1 - v_dst) * z_dst[0];
					float Z2_dst = u_dst * (1 - v_dst) * z_dst[1];
					float Z3_dst = (1 - u_dst) * v_dst * z_dst[2];
					float Z4_dst = u_dst * v_dst * z_dst[3];

					Z_dst = Z1_dst + Z2_dst + Z3_dst + Z4_dst;
				}

				// ������������
				double A_dst = a1 * (X_dst - Xs) + b1 * (Y_dst - Ys) + c1 * (Z_dst - Zs);
				double B_dst = a2 * (X_dst - Xs) + b2 * (Y_dst - Ys) + c2 * (Z_dst - Zs);
				double C_dst = a3 * (X_dst - Xs) + b3 * (Y_dst - Ys) + c3 * (Z_dst - Zs);

				double x_dst = -f * A_dst / C_dst;
				double y_dst = -f * B_dst / C_dst;

				// ������ת��Ϊͼ������
				double I = (x_dst + 11.70) / pixel_size;
				double J = (7.80 - y_dst) / pixel_size;

				// �ж��Ƿ���ͼ��Χ��
				if (I < 0 || I > width - 1 || J < 0 || J > height - 1) {
					dst.at<Vec3b>(rows - 1 - j, i) = Vec3b(0, 0, 0);
					continue;
				}

				// ˫���Բ�ֵ
				int ioff = (int)I;
				int joff = (int)J;
				float u1 = I - ioff;
				float v1 = J - joff;

				Vec3b p1 = src.at<Vec3b>(joff, ioff);
				Vec3b p2 = src.at<Vec3b>(joff, ioff + 1);
				Vec3b p3 = src.at<Vec3b>(joff + 1, ioff);
				Vec3b p4 = src.at<Vec3b>(joff + 1, ioff + 1);

				Vec3b p;
				for (int k = 0; k < 3; k++) {
					p[k] = (1 - u1) * (1 - v1) * p1[k] + u1 * (1 - v1) * p2[k] + (1 - u1) * v1 * p3[k] + u1 * v1 * p4[k];
				}

				dst.at<Vec3b>(rows - 1 - j, i) = p;
			}
		}

		// ��ͼ��ת��Ϊ�Ҷ�ͼ��
		Mat gray;
		cvtColor(dst, gray, COLOR_BGR2GRAY);

		// ��ֵ��ͼ��
		Mat binary;
		threshold(gray, binary, 1, 255, THRESH_BINARY);

		// �ҵ�ͼ���е�����
		vector<vector<Point>> contours;
		findContours(binary, contours, RETR_EXTERNAL, CHAIN_APPROX_SIMPLE);

		// �ҵ���������
		int maxArea = 0;
		Rect maxRect;
		for (const auto& contour : contours) {
			Rect rect = boundingRect(contour);
			int area = rect.width * rect.height;
			if (area > maxArea) {
				maxArea = area;
				maxRect = rect;
			}
		}

		// �ü�ͼ��
		Mat result = dst(maxRect);

		// ��BGRת��ΪRGB
		Mat rgbResult;
		cvtColor(result, rgbResult, COLOR_BGR2RGB);

		// ����GDAL���ݼ�
		GDALDriver* poDriver = GetGDALDriverManager()->GetDriverByName("GTiff");
		std::string outputPath = outputpath + "\\" + imageName + ".tif";

		GDALDataset* poDstDS = poDriver->Create(outputPath.c_str(), result.cols, result.rows, 3, GDT_Byte, NULL);




		// ���õ���������Ϣ
		double newGeoTransform[6];
		newGeoTransform[0] = Xmin;
		newGeoTransform[1] = resolution;
		newGeoTransform[2] = 0;
		newGeoTransform[3] = Ymax;
		newGeoTransform[4] = 0;
		newGeoTransform[5] = -resolution;

		poDstDS->SetGeoTransform(newGeoTransform);

		// ����ͶӰ��Ϣ����DEM�ļ��л�ȡ��
		const char* pszSRS_WKT = poDataset->GetProjectionRef();
		if (pszSRS_WKT != NULL && strlen(pszSRS_WKT) > 0)
		{
			poDstDS->SetProjection(pszSRS_WKT);
		}

		// ��OpenCV��Mat����д��GDAL���ݼ�
		for (int i = 0; i < 3; i++)
		{
			GDALRasterBand* poBand = poDstDS->GetRasterBand(i + 1);
			poBand->RasterIO(GF_Write, 0, 0, rgbResult.cols, rgbResult.rows, rgbResult.data + i, rgbResult.cols, rgbResult.rows, GDT_Byte, 3, rgbResult.step);
		}

		// ����NoDataֵ
		for (int i = 1; i <= poDstDS->GetRasterCount(); i++)
		{
			GDALRasterBand* poBand = poDstDS->GetRasterBand(i);
			poBand->SetNoDataValue(0);
		}

		// ���һ��tfw�ļ�
		std::string tfwFileName = outputpath + "\\" + imageName + ".tfw";

		ofstream tfwFile(tfwFileName); //����һ��tfw�ļ�
		if (!tfwFile.is_open()) {
			cout << "Error: Failed to create tfw file." << endl;
		}

		tfwFile << std::fixed << std::setprecision(2) << resolution << std::endl;
		tfwFile << 0.0 << std::endl;
		tfwFile << 0.0 << std::endl;
		tfwFile << std::fixed << std::setprecision(2) << -resolution << std::endl;

		tfwFile << std::fixed << std::setprecision(12) << Xmin << endl;
		tfwFile << std::fixed << std::setprecision(12) << Ymax << endl;

		tfwFile.close();

		// �ر�GDAL���ݼ�
		GDALClose(poDstDS);

		// ��ʾ���
		/*namedWindow("Result", WINDOW_NORMAL);
		imshow("Result", result);

		waitKey(0);*/

		// �ͷ�mat��Դ
		src.release();
		dst.release();
		result.release();
		gray.release();
		binary.release();
		rgbResult.release();
	}
}

std::map<std::string, ImageData> testDC::readCSV(const std::string& filename) {
	std::ifstream file(filename);
	std::string line;
	std::map<std::string, ImageData> data;

	// ������һ��
	std::getline(file, line);

	while (std::getline(file, line)) {
		std::stringstream lineStream(line);
		std::string cell;
		ImageData imageData;

		std::getline(lineStream, imageData.imageName, ',');
		std::getline(lineStream, cell, ','); imageData.Xs = std::stod(cell);
		std::getline(lineStream, cell, ','); imageData.Ys = std::stod(cell);
		std::getline(lineStream, cell, ','); imageData.Zs = std::stod(cell);

		// ����3���հ���
		for (int i = 0; i < 3; i++) {
			std::getline(lineStream, cell, ',');
		}

		for (int i = 0; i < 9; i++) {
			std::getline(lineStream, cell, ',');
			imageData.R[i] = std::stod(cell);
		}

		data[imageData.imageName] = imageData;
	}

	return data;
}

std::string testDC::getFilenameWithoutExtension(const std::string& filepath) {
	// ��ȡ������չ�����ļ���
	const size_t last_slash_idx = filepath.find_last_of("\\/");
	std::string filename = filepath.substr(last_slash_idx + 1);

	// ɾ����չ��
	const size_t period_idx = filename.rfind('.');
	if (period_idx != std::string::npos) {
		filename.erase(period_idx);
	}

	return filename;
}