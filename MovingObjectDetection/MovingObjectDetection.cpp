// MovingObjectDetection.cpp : main project file.
// 20160821 written by LIM ZI YI (joslimzy@gmail.com)
// Visual Studio version: vc11 (2012)
// OpenCV version: 2.4.11
// Function : Sample of detect moving object through accumulateWeighted(), and save detected image to file.

#include "stdafx.h"
#include <opencv2/opencv.hpp>
#include <time.h>  
#include <iostream>
#include <fstream>

//Global Parameters
const std::string videoStreamAddress = "http://viewer:viewer@192.168.11.2/mjpg/video.mjpg"; //IP Cam stream URL
double accumulate_alpha = 0.05; //accumulateWeighted's alpha
int delay_sec = 2; //Delay seconds to avoid detect the same object
double ratio_threshold = 10; //Over this threshold to detect object
int erode_iterations = 5; //Erode iterations
int dilate_iterations = 5; //Dilate iterations
bool detected_object = false;

//Structure for setting roi (mouse event)
struct MouseInfo{
	bool mouse_down;
	bool rect_set;
	cv::Rect rect;
	cv::Point pt1;
	cv::Point pt2;
}mouseinfo;

//Read setting from file
void read_parameters()
{
	std::ifstream file_parameters("parameters.dat");
	if(file_parameters.is_open())
	{
		file_parameters >>  mouseinfo.rect.x;
		file_parameters >>  mouseinfo.rect.y;
		file_parameters >>  mouseinfo.rect.width;
		file_parameters >>  mouseinfo.rect.height;
		file_parameters.close();
	}else 
		std::cout << "Open File: Unable to open file";

}

//Write setting from file
void write_parameters()
{
	std::ofstream file_parameters("parameters.dat");
	if(file_parameters.is_open())
	{
		file_parameters << mouseinfo.rect.x << " " << mouseinfo.rect.y << " " << mouseinfo.rect.width << " " << mouseinfo.rect.height << "\n";
		file_parameters.close();
	}else 
		std::cout << "Write File: Unable to open file";
}

//Callback function for mouse event
void MouseCallBackFunc(int event, int x, int y, int flags, void* userdata)
{
	
    if  ( event == cv::EVENT_LBUTTONDOWN )
    {
		mouseinfo.pt1 = cv::Point(x,y);
		mouseinfo.mouse_down = true;
    }
    else if  ( event == cv::EVENT_MOUSEMOVE )
    {
		if(mouseinfo.mouse_down)
		{
			mouseinfo.pt2 = cv::Point(x,y);
			mouseinfo.rect = cv::Rect(mouseinfo.pt1.x,
				mouseinfo.pt1.y,
				abs(mouseinfo.pt2.x - mouseinfo.pt1.x),
				abs(mouseinfo.pt2.y - mouseinfo.pt1.y));
		}
    }
    else if  ( event == cv::EVENT_LBUTTONUP )
    {
		mouseinfo.rect = cv::Rect(mouseinfo.pt1.x,
			mouseinfo.pt1.y,
			abs(mouseinfo.pt2.x - mouseinfo.pt1.x),
			abs(mouseinfo.pt2.y - mouseinfo.pt1.y));
		mouseinfo.mouse_down = false;
		if(abs(mouseinfo.pt2.x - mouseinfo.pt1.x)<=1 || abs(mouseinfo.pt2.y - mouseinfo.pt1.y) <=1)
			mouseinfo.rect_set = false;
		else
		{
			//Save roi setting to file
			write_parameters();
			mouseinfo.rect_set = true;
		}
    }
}

//Main entrance
int main(array<System::String ^> ^args)
{
	//Initialize variable
	mouseinfo.mouse_down = false;
	mouseinfo.rect_set = false;
	mouseinfo.rect = cv::Rect(0,0,1,1);
	mouseinfo.pt1 = cv::Point(0,0);
	mouseinfo.pt2 = cv::Point(0,0);
	bool initialize = true;
	clock_t start = clock();

	//VideoCapture and Mat initialize
	cv::VideoCapture vcap;
	cv::Mat frame;
	cv::Mat frame_grayscale;
	cv::Mat frame_background;
	cv::Mat frame_diff;
	cv::Mat frame_foreground;
	cv::Mat detect_area;
	cv::Mat output_draw;
	cv::Mat backImage;
	
	std::cout<<"Connecting to stream..."<<std::endl;
	//open the video stream and make sure it's opened
	if (!vcap.open(videoStreamAddress)) {
		std::cout << "Error opening video stream or file" << std::endl;
		system("pause");
		return -1;
	}
	std::cout<<"Connected..."<<std::endl;
 
	//read data
	read_parameters();
 
    //Create a window
    cv::namedWindow("OutputFrame", 1);

    //set the callback function for any mouse event
    cv::setMouseCallback("OutputFrame", MouseCallBackFunc, NULL);

	cv::Rect roi;
	roi = mouseinfo.rect;
	while (true)
	{
		if (!vcap.read(frame)) {
			std::cout << "No frame" << std::endl;
			cv::waitKey();
		}
		if(mouseinfo.rect_set)
		{
			mouseinfo.rect_set = false;
			roi = mouseinfo.rect;
		}
		
		//std::cout<<frame.type()<<"; "<<frame.channels()<<std::endl;
		if(initialize)
		{
			frame_background = cv::Mat::zeros(frame.size(), CV_32FC1);
			frame_diff = cv::Mat::zeros(frame.size(), CV_32FC1);
			initialize = false;
		}

		//clone frame
		output_draw = frame.clone();

		//pre process
		cv::medianBlur(frame,frame,3);

		//convertion
		cv::cvtColor(frame, frame_grayscale, CV_BGR2GRAY);
		//frame_grayscale = frame.clone();
		//frame.convertTo(frame_grayscale,CV_32FC1);
		if(frame_background.empty())
			frame_grayscale.convertTo(frame_background,CV_32F);

		//Background detection
		//frame_diff = frame_grayscale.clone();
		cv::accumulateWeighted(frame_grayscale, frame_background, accumulate_alpha);
		frame_background.convertTo(backImage,CV_8U);
		//cv::absdiff(frame_grayscale, backImage, frame_diff);
		cv::subtract(frame_background,frame_grayscale , frame_diff, cv::noArray(), CV_32F);
		cv::threshold(frame_diff, frame_foreground, 20, 255, CV_THRESH_BINARY);
		frame_foreground = frame_diff;
		

		cv::erode(frame_foreground, frame_foreground, cv::Mat(), cv::Point(-1, -1), erode_iterations, 1, 1);
		cv::dilate(frame_foreground, frame_foreground, cv::Mat(), cv::Point(-1, -1), dilate_iterations, 1, 1);
		
		//get custom area
		detect_area = frame_foreground(roi).clone();

		//calculate hte white pixel ratio
		int white_pixel = cv::countNonZero(detect_area);
		int area_pixel = detect_area.rows*detect_area.cols;
		double ratio = ((double)white_pixel / (double)area_pixel)*100; //ratio percentage

		//Show calculated values
		char str[200];
		sprintf(str,"White Pixel:  %d",white_pixel);
		cv::putText(output_draw, str, cvPoint(30,30), 
					cv::FONT_HERSHEY_COMPLEX_SMALL, 0.8, cvScalar(0,255,0), 1, CV_AA);
		sprintf(str,"Total Pixel: %d",area_pixel);
		cv::putText(output_draw, str, cvPoint(30,60), 
					cv::FONT_HERSHEY_COMPLEX_SMALL, 0.8, cvScalar(0,255,0), 1, CV_AA);
		sprintf(str,"Ratio: %.2lf\%",ratio);
		cv::putText(output_draw, str, cvPoint(30,90), 
					cv::FONT_HERSHEY_COMPLEX_SMALL, 0.8, cvScalar(0,255,0), 1, CV_AA);

		//Detect object checking
		int sec = ((clock () - start)/ CLOCKS_PER_SEC );
		cv::Scalar rect_color = cv::Scalar(0,255,0,0);
		if(ratio >= ratio_threshold 
			&& sec >= delay_sec) //Object detected and trigged action
		{
			cv::imshow("Detected", output_draw);
			start = clock();
			char DateTimeStr[200];
			System::DateTime now;
			sprintf(DateTimeStr,"Image\\%d-%d-%d_%d-%d-%d-%d.jpg",now.Now.Year,now.Now.Month,now.Now.Day,now.Now.Hour,now.Now.Minute,now.Now.Second,now.Now.Millisecond);
			cv::imwrite(DateTimeStr,frame);
			std::cout << "Saved: " << DateTimeStr << std::endl;
		}
		if(sec < delay_sec) //Delay seconds to avoid detect the same object
		{
			rect_color = cv::Scalar(0,0,255,0);
			detected_object = true;
		}else{ //Clear the state and conitnue detect moving object
			detected_object = false;
		}
		
		//output
		cv::rectangle(output_draw,mouseinfo.rect,rect_color,1);
		cv::imshow("OutputFrame", output_draw);
		//cv::imshow("frame_grayscale", frame_grayscale);
		//cv::imshow("frame_background", frame_background/255);
		//cv::imshow("frame_foreground", frame_foreground);
		//cv::imshow("frame_diff", frame_diff);
		//cv::imshow("Detect Area", detect_area);

		//click to exit
		if (cv::waitKey(5) >= 0) ;//break;
	}

	system("pause");
    return 0;

}

