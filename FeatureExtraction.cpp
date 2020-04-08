#include "stdafx.h"


#include "FeatureExtraction.h"

Identify::Identify()
{
	const char *argv[3];
	argv[0] = "F:/DACHUANG/player2/release/player.exe";
	argv[1] = "- device";
	argv[2] = "0";
	//构造函数载如数据模板（脸和眼睛）
	Identify::arguments = Identify::get_arguments(Identify::argc, argv);
	det_parameters = new LandmarkDetector::FaceModelParameters(Identify::arguments);
	face_model = new LandmarkDetector::CLNF(det_parameters->model_location);
	face_analysis_params = new FaceAnalysis::FaceAnalyserParameters(Identify::arguments);
	face_analyser = new FaceAnalysis::FaceAnalyser(*face_analysis_params);
	visualizer = new Utilities::Visualizer(Identify::arguments);

	if (!face_model->loaded_successfully)
	{
		/*ocout << "ERROR: Could not load the landmark detector\n";*/
	}
}
Identify::~Identify() {

}
void Identify::waring_out(int key, double data)
{
	switch (key)
	{
	case 0:
		std::cout << "head_erro,head is in x " << "   " << data << std::endl;
		break;
	case 1:
		std::cout << "gaze_erro,please correct your eyes! " << "   " << data << std::endl;
		break;
	case 2:
		std::cout << "head_erro,head is in y " << "   " << data << std::endl;
		break;
	case 3:
		std::cout << "head_erro,head is in z " << "   " << data << std::endl;
		break;
	default:
		break;
	}
}
void Identify::printErrorAndAbort(const std::string & error)
{
	std::cout << error << std::endl;
}

bool Identify::CheckCamera()
{
	if (Identify::sequence_reader.IsWebcam())
	{
		return true;
	}
	return false;
}


#define FATAL_STREAM( stream ) \
printErrorAndAbort( std::string( "Fatal error: " ) + stream )

std::vector<std::string> Identify::get_arguments(int argc, const char **argv)
{

	std::vector<std::string> arguments;

	// First argument is reserved for the name of the executable
	for (int i = 0; i < argc; ++i)
	{
		arguments.push_back(std::string(argv[i]));
	}
	return arguments;
}

void Identify::face_identify()
{
	//此处是解析参数 可以用字符串作为命令写入
//	//在特征提取中始终跟踪注视
	ofstream ocout;
	//	//可视化结果的实用程序  此段程序可以去掉 不用可视化
	//Utilities::Visualizer visualizer(Identify::arguments);

	//	//跟踪FPS以进行可视化   可以去掉
	Utilities::FpsTracker fps_tracker;
	fps_tracker.AddFrame();
	//
	if (Identify::contro_Identify) // this is not a for loop as we might also be reading from a webcam
	{
		//序列读取器根据提供的命令行参数选择要打开的内容

		if (Identify::sequence_reader.IsWebcam())
		{
			visualizer->vis_track = true;
		}
		cv::Mat captured_image;
		// 第二步 获取图像
		captured_image = Identify::sequence_reader.GetNextFrame();
		//用于报告进度
		double reported_completion = 0;

		if (!captured_image.empty())
		{
			//转换为灰度
			cv::Mat_<uchar> grayscale_image = Identify::sequence_reader.GetGrayFrame();
			//  实际面部界标检测 / 跟踪
			// 看来 face_model 、 det_parameter 不可或缺
			 detection_success = LandmarkDetector::DetectLandmarksInVideo(captured_image, *face_model,*det_parameters, grayscale_image);

			// Gaze tracking, absolute gaze direction
			cv::Point3f gazeDirection0(0, 0, 0); cv::Point3f gazeDirection1(0, 0, 0); cv::Vec2d gazeAngle(0, 0);

			if (detection_success && face_model->eye_model)
			{
				GazeAnalysis::EstimateGaze(*face_model, gazeDirection0, Identify::sequence_reader.fx, Identify::sequence_reader.fy, Identify::sequence_reader.cx, Identify::sequence_reader.cy, true);
				GazeAnalysis::EstimateGaze(*face_model, gazeDirection1, Identify::sequence_reader.fx, Identify::sequence_reader.fy, Identify::sequence_reader.cx, Identify::sequence_reader.cy, false);
				gazeAngle = GazeAnalysis::GetGazeAngle(gazeDirection0, gazeDirection1);
				//获取眼睛凝视方向			
				Identify::gaze_angle = ((gazeAngle[0] + gazeAngle[1]) / 2) * 180 / M_PI;//gaze_dect(gaze_angle);// 眼神方向
				ocout.open("test.txt");
				ocout << gaze_angle << "\n";
				ocout.close();
																			  //std::cout << "************************************" << gaze_angle<< std::endl;

			}

			// Do face alignment
			//进行人脸对齐
			cv::Mat sim_warped_img;
			cv::Mat_<double> hog_descriptor; int num_hog_rows = 0, num_hog_cols = 0;
			// Work out the pose of the head from the tracked model
			//头颅方向
			cv::Vec6d pose_estimate = LandmarkDetector::GetPose(*face_model, Identify::sequence_reader.fx, Identify::sequence_reader.fy, Identify::sequence_reader.cx, Identify::sequence_reader.cy);
			pose_angle = { pose_estimate[3] * 180 / M_PI,pose_estimate[4] * 180 / M_PI,pose_estimate[5] * 180 / M_PI };
			poseEstimate = pose_angle[0];
			// Keeping track of FPS
			fps_tracker.AddFrame();

			// Displaying the tracking visualizations
			visualizer->SetImage(captured_image, Identify::sequence_reader.fx, Identify::sequence_reader.fy, Identify::sequence_reader.cx, Identify::sequence_reader.cy);
			visualizer->SetObservationFaceAlign(sim_warped_img);
			visualizer->SetObservationHOG(hog_descriptor, num_hog_rows, num_hog_cols);
			visualizer->SetObservationLandmarks(face_model->detected_landmarks, face_model->detection_certainty, face_model->GetVisibilities());
			visualizer->SetObservationPose(pose_estimate, face_model->detection_certainty);
			visualizer->SetObservationGaze(gazeDirection0, gazeDirection1, LandmarkDetector::CalculateAllEyeLandmarks(*face_model), LandmarkDetector::Calculate3DEyeLandmarks(*face_model, Identify::sequence_reader.fx, Identify::sequence_reader.fy, Identify::sequence_reader.cx, Identify::sequence_reader.cy), face_model->detection_certainty);
			visualizer->SetObservationActionUnits(face_analyser->GetCurrentAUsReg(), face_analyser->GetCurrentAUsClass());
			visualizer->SetFps(fps_tracker.GetFPS());

			// detect key presses
			// 可以删掉
			char character_press = visualizer->ShowObservation();

			// quit processing the current sequence (useful when in Webcam mode)

			cv::Point3f gaze_direction0;
			cv::Vec6f head_pose;
			// 进行条件判断
			count_time++;

			//如果 头部 【-20 23】之间 眼睛【-20 20】
#ifdef OPEN_GAZE_Recongize//gaze角度
			if (gaze_angle > 12.0 || gaze_angle < -4.0)
			{
				Identify::count_gaze++;
				if (Identify::count_gaze >= 30 * 5)
				{
					std::cout << "************************************" << std::endl;
					Identify::waring_out(1, gaze_angle);
					std::cout << "************************************" << std::endl;
					Identify::count_gaze = 0;
				}
			}
#endif
#ifdef OPEN_POSE_Recongize//POSE角度
			//	std::cout << "POSE[3]"<< pose_angle[0] << std::endl;//y轴
			//	std::cout << "POSE[4]" << pose_angle[1]<< std::endl;//X轴
			//	std::cout << "POSE[5]" << pose_angle[2]  << std::endl;
				//x轴
			if (pose_angle[1] > pose_threshold_right || pose_angle[1] < pose_threshold_left)
			{
				Identify::count_pose++;

				if (Identify::count_pose >= 30 * 5)
				{
					std::cout << "************************************" << std::endl;
					Identify::waring_out(0, pose_angle[1]);
					std::cout << "************************************" << std::endl;
					Identify::count_pose = 0;
				}
			}
			//y轴（向下）
			if (pose_angle[0] > pose_threshold_down || pose_angle[0] < pose_threshold_up)
			{
				Identify::count_pose++;

				if (Identify::count_pose >= 30 * 5)
				{
					std::cout << "************************************" << std::endl;
					Identify::waring_out(2, pose_angle[0]);
					std::cout << "************************************" << std::endl;
					Identify::count_pose = 0;
				}
			}
#endif
#ifdef OPEN_EYE_Recongize//眼睛眨眼频率
			//眼睛轮廓特征点  左眼【8 19】 右眼【36 47】
			std::vector<cv::Point3f> eye_landmarks3d = LandmarkDetector::Calculate3DEyeLandmarks(*face_model, Identify::sequence_reader.fx, Identify::sequence_reader.fy, Identify::sequence_reader.cx, Identify::sequence_reader.cy);
			//			double lefteye_y_value1 = 0, lefteye_y_value2 = 0, righteye_y_value1 = 0, righteye_y_value2 = 0;
						//std::cout << "*************** time ***************" << std::endl << Identify::sequence_reader.time_stamp << std::endl;
			lefteye_y_value = eye_landmarks3d[17].y - eye_landmarks3d[11].y, righteye_y_value = eye_landmarks3d[45].y - eye_landmarks3d[39].y;
			//double lefteye_y_value = lefteye_y_value1 - lefteye_y_value2, righteye_y_value = righteye_y_value1 - righteye_y_value2;
			//std::cout << "  lefteye_y_value " << "   " << lefteye_y_value << "  righteye_y_value " << "   " << righteye_y_value << std::endl;
			// 如果达到眼睛开合度要求
			if (lefteye_y_value < Threshold_lefteye_y && righteye_y_value < Threshold_righteye_y)
			{

				Identify::count_eye_influent++;
			}
			if (Identify::sequence_reader.time_stamp - Identify::eye_time >= eye_one_time)//时间累积30s
			{
				//如果眨眼频率超过规定值
				if (Identify::count_eye_influent > eye_frequce)
				{
					std::cout << "  please open you eyes frequce" << std::endl;

				}
				Identify::count_eye_influent = 0;
				Identify::eye_time = Identify::sequence_reader.time_stamp;
			}

#endif // OPEN_EYE_Recongize
#ifdef OPEN_FACE_Recongize//点头频率
			// 面部 特征点提取
			// 利用面部特征点 以上方法有误 
			// 下面是利用头部姿态中的 y方向进行瞌睡时点头识别
//			std::cout << "   " << pose_estimate[3] << std::endl;
			//仰头
			if (pose_estimate[3] < up_reach_threshold)
			{
				Identify::pose_up_count++;
				std::cout << "  pose_up_count: " << Identify::pose_up_count << std::endl;

			}
			if (Identify::sequence_reader.time_stamp - face_uptime >= face_uptime_Threshold)
			{

				if (Identify::pose_up_count > pose_up_threshold_count)
				{

					std::cout << "  ******************************your head is in      up      " << std::endl;
				}
				Identify::pose_up_count = 0;
				Identify::face_uptime = Identify::sequence_reader.time_stamp;
			}
			//低头
			if (pose_estimate[3] > down_reach_threshold)
			{
				std::cout << "  pose_down_count: " << pose_down_count << std::endl;
				Identify::pose_down_count++;

			}
			if (Identify::sequence_reader.time_stamp - face_downtime >= face_downtime_Threshold)
			{

				if (Identify::pose_down_count > pose_down_threshold_count)
				{
					std::cout << "  *****************************your head is in      down      " << std::endl;

				}
				Identify::pose_down_count = 0;
				Identify::face_downtime = Identify::sequence_reader.time_stamp;
			}

#endif // OPEN_FACE_Recongize
#ifdef OPEN_Mouse_recongize
			const cv::Mat_<float>& landmarks_3D = face_model->GetShape(Identify::sequence_reader.fx, Identify::sequence_reader.fy, Identify::sequence_reader.cx, Identify::sequence_reader.cy);
			int mouse_count = 0;
			double up_lip = 0, down_lip = 0;
			for (auto lmk : landmarks_3D)
			{
				mouse_count++;
				if (mouse_count >= (49 + 68 + 1) && mouse_count <= (53 + 68 + 1))
				{
					up_lip = up_lip + lmk;
				}
				if (mouse_count >= (55 + 68 + 1) && mouse_count <= (59 + 68 + 1))
				{
					down_lip = down_lip + lmk;
				}
			}
			lip = down_lip - up_lip;
			if (lip > up_lip_threshold)
			{
				std::cout << "  mouse_error " << lip << std::endl;

			}

#endif // OPEN_Mouse_recongize

			// Reporting progress
			// 可以删掉
			if (Identify::sequence_reader.GetProgress() >= reported_completion / 10.0)
			{
				std::cout << reported_completion * 10 << "% ";
				if (reported_completion == 10)
				{
					std::cout << std::endl;
				}
				reported_completion = reported_completion + 1;
			}

			// Grabbing the next frame in the sequence
		}

		INFO_STREAM("Closing input reader");
		INFO_STREAM("Closed successfully");

		// Reset the models for the next video
	}
}
