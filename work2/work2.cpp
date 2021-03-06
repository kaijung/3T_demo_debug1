// work2.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <iostream>
#include <fstream>
#include "FileIO.h"
#include <io.h>
#include "FeatureMatch.h"
#include <opencv2\xfeatures2d\nonfree.hpp>
#include <iostream>
#include <vector>
#include <opencv2\features2d\features2d.hpp>
#include <opencv2/opencv.hpp>
#include "Structure.h"
#include "base.h"


using namespace cv;
using namespace std;

void main()
{
	FilesIO files("src/", ".png");
	vector<Mat> image_for_all = files.getImages();

	vector<vector<KeyPoint>> key_points_for_all;
	vector<Mat> descriptor_for_all;//用来匹配特征点的图像描述子
	vector<vector<Vec3b>> colors_for_all;
	vector<vector<DMatch>> matches_for_all;
	//提取所有图像的特征
	//对所有图像进行顺次的特征匹配
	FeatureMatch::FeatureObtatinAndMatchForAll(image_for_all, key_points_for_all, descriptor_for_all, colors_for_all, matches_for_all);
	for (int i = 0; i < key_points_for_all.size(); i++) {
		cout << key_points_for_all[i].size() << endl;
	}
	for (int i = 0; i < matches_for_all.size(); i++) {
		cout << matches_for_all[i].size() << endl;
	}
	//保存所有相机的齐次坐标
	vector<Point3f> structure;
	//保存第i副图像中第j个特征点对应的structure中点的索引
	vector<vector<int>> correspond_struct_idx;
	//匹配点颜色存储向量
	vector<Vec3b> colors;
	//旋转矩阵向量
	vector<Mat> rotations;
	//平移矩阵向量
	vector<Mat> motions;

	//初始化结构（三维点云）
	Structure::init_structure(
		key_points_for_all,
		colors_for_all,
		matches_for_all,

		structure,
		correspond_struct_idx,
		colors,//筛选颜色
		rotations,
		motions
	);

	//增量方式重建剩余的图像
	for (int i = 1; i < matches_for_all.size(); ++i)
	{
		vector<Point3f> object_points;
		vector<Point2f> image_points;
		Mat r, R, T;
		//Mat mask;

		//获取第i幅图像中匹配点对应的三维点，以及在第i+1幅图像中对应的像素点
		Transform::get_objpoints_and_imgpoints(
			matches_for_all[i],
			correspond_struct_idx[i],
			structure,
			key_points_for_all[i + 1],

			object_points,
			image_points
		);


		//求解变换矩阵
		solvePnPRansac(object_points, image_points, K, noArray(),
			r, T);
		//将旋转向量转换为旋转矩阵
		Rodrigues(r, R);

		//保存变换矩阵
		rotations.push_back(R);
		motions.push_back(T);

		vector<Point2f> p1, p2;
		vector<Vec3b> c1, c2;
		Transform::get_matched_points(key_points_for_all[i], key_points_for_all[i + 1], matches_for_all[i], p1, p2);
		//Transform::get_matched_colors(colors_for_all[i], colors_for_all[i + 1], matches_for_all[i], c1, c2);

		//根据之前求得的R，T进行三维重建
		vector<Point3f> next_structure;
		Reconstruct::reconstruct(rotations[i], motions[i], R, T, p1, p2, next_structure);

		//将新的重建结果与之前的融合
		Reconstruct::fusion_structure(
			matches_for_all[i],
			correspond_struct_idx[i],
			correspond_struct_idx[i + 1],
			structure,
			next_structure,
			colors,
			c1
		);
	}

	//保存
	Reconstruct::save_structure(".\\Viewer\\structure.yml", rotations, motions, structure, colors);
}