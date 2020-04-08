#ifndef FeatureExtraction_H
#define FeatureExtraction_H
#pragma comment(lib,"FaceAnalyser.lib")
#pragma comment(lib,"GazeAnalyser.lib")
#pragma comment(lib,"LandmarkDetector.lib")
#pragma comment(lib,"Utilities.lib")
#include "LandmarkCoreIncludes.h"//
#include <RecorderCSV.h>//
#include "math.h"
#include <Face_utils.h>
#include <FaceAnalyser.h>
#include <GazeEstimation.h>
#include <RecorderOpenFace.h>//
#include <RecorderOpenFaceParameters.h>//
#include <SequenceCapture.h>//
#include <Visualizer.h>//
#include <VisualizationUtils.h> //


#ifndef CONFIG_DIR
#define CONFIG_DIR "~"
#endif
#define M_PI 3.14159265
#define INFO_STREAM( stream ) \
std::cout << stream << std::endl
using namespace std;
#define WARN_STREAM( stream ) \
std::cout << "Warning: " << stream << std::endl

#define ERROR_STREAM( stream ) \
std::cout << "Error: " << stream << std::endl

#define OPEN_GAZE_Recongize 
#define OPEN_POSE_Recongize 
#define OPEN_EYE_Recongize 
#define OPEN_FACE_Recongize 
#define OPEN_Mouse_recongize
#ifdef OPEN_GAZE_Recongize
//视线方向采用阈值限位
constexpr auto gaze_threshold_left = 18.0;
constexpr auto gaze_threshold_right = -18.0;
#endif
#ifdef OPEN_POSE_Recongize
//头部左右方向采用阈值限位
constexpr auto pose_threshold_left = -21.0;
constexpr auto pose_threshold_right = 30.0;
constexpr auto pose_threshold_up = -14.0;
constexpr auto pose_threshold_down = 15;
#endif
#ifdef OPEN_EYE_Recongize
//眼睛眨眼频率采用 30s内眨眼次数
constexpr auto Threshold_lefteye_y = 5.5;
constexpr auto Threshold_righteye_y = 5.3;
constexpr auto eye_one_time = 30*5; // (5s内)
constexpr auto eye_frequce = 15; // 30s内人的眨眼次数
#endif 
#ifdef OPEN_FACE_Recongize
constexpr auto face_downtime_Threshold = 30*3;
constexpr auto face_uptime_Threshold = 30*3;
constexpr auto pose_up_threshold_count = 5 * 1;// 30s内仰头次数
constexpr auto pose_down_threshold_count = 5 * 1;// 30s内低头次数
constexpr auto up_reach_threshold = -14.0;
constexpr auto down_reach_threshold = 15;
#endif // OPEN_FACE_Recongize
#ifdef OPEN_Mouse_recongize
constexpr auto up_lip_threshold = 200;
#endif // OPEN_Mouse_recongize


class Identify
{
public:
//函数
	Identify();
	~Identify();
	void face_identify();//把命令行的参数传进去
	bool contro_Identify = false;
	bool detection_success ;
	void waring_out(int key, double data);
	std::vector<std::string> get_arguments(int argc, const char **argv);
	void printErrorAndAbort(const std::string & error);
	bool CheckCamera();
//参数
	std::vector<std::string> arguments;

//在player.cpp中用的类
	Utilities::SequenceCapture sequence_reader;
	LandmarkDetector::CLNF *face_model;
	FaceAnalysis::FaceAnalyserParameters *face_analysis_params;
	FaceAnalysis::FaceAnalyser *face_analyser;
//在player.cpp中用的变量
	double gaze_angle;
	char *idenInfo;
	cv::Vec6d pose_angle;
	double lefteye_y_value, righteye_y_value;
	double lip;
	double poseEstimate;
#ifdef OPEN_GAZE_Recongize
	int count_gaze = 0;
#endif
#ifdef OPEN_POSE_Recongize
	int  count_pose = 0;
#endif
#ifdef OPEN_EYE_Recongize
	int count_eye_influent = 0;
	float eye_time = 0;
#endif
#ifdef OPEN_FACE_Recongize
	int pose_up_count = 0, pose_down_count = 0;
	float face_downtime = 0, face_uptime = 0;
#endif
private:
	int argc = 3;

	LandmarkDetector::FaceModelParameters *det_parameters;
	Utilities::Visualizer *visualizer;

	int count_time = 0;
};
#endif
