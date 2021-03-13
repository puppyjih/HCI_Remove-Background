#include <iostream>
#include <math.h>
#include <vector>
#include <string>
/// opencv
#include "opencv2/core.hpp"
#include <opencv2/highgui.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/video.hpp>
#include <opencv2/imgproc.hpp>
#define _WIN32 // if Your OS is Window
#ifdef _WIN32
#include <Windows.h>
#define sleep Sleep
#define TOSECONDS 1000
#else
#include <unistd.h>
#define TOSECONDS 1
#endif
using namespace cv;

std::string CURRENT_DIR("C:/image/");
std::string AddImageFile = CURRENT_DIR + "board2.jpg";
Mat AddImage;

Mat mask;
Mat AddImageMask;

Mat SetBackgroundImage(VideoCapture &vc)
{
    Mat ret;
    vc.release();
    vc = VideoCapture(0);
    vc.read(ret);
    return ret;
}

uchar sub(const uchar &a, const uchar &b)
{
    if (a > b)
    {
        return a - b;
    }
    else
    {
        return b - a;
    }
}

Mat RemoveBackground(const Mat &background, const Mat &input, const uchar &thres)
{
    Mat bg, ret;
    bg = background.clone();
    ret = input.clone();

    bool check[3] = {false, false, false};

    for (int y = 0; y < bg.size().height; ++y)
    {
        uchar *pointer_bg = bg.ptr<uchar>(y);
        uchar *pointer_ret = ret.ptr<uchar>(y);
        uchar *pointer_mask = mask.ptr<uchar>(y);

        for (int x = 0; x < bg.size().width; ++x)
        {

            for (int c = 0; c < 3; ++c)
            {
                uchar c_bg = pointer_bg[x * 3 + c];
                uchar c_ret = pointer_ret[x * 3 + c];
                if (abs(c_bg - c_ret) < thres)
                {
                    check[c] = true;
                }
            }
            if (check[0] == true && check[1] == true && check[2] == true)
            {
                pointer_mask[x] = 0;
            }
            else
            {
                pointer_mask[x] = 255;
            }

            check[0] = false;
            check[1] = false;
            check[2] = false;
        }
    }

    return ret;
}

void Erode(const Mat &input, Mat &output, const int size = 3, const int type = MORPH_RECT)
{
    erode(input, output, getStructuringElement(type, Size(2 * size + 1, 2 * size + 1), Point(1, 1)));
}
void Dilate(const Mat &input, Mat &output, const int size = 3, const int type = MORPH_RECT)
{
    dilate(input, output, getStructuringElement(type, Size(2 * size + 1, 2 * size + 1), Point(1, 1)));
}
void EraseNoise(const Mat &input, Mat &output)
{
    int size = 2;
    // int type = MORPH_CROSS;
    int type = MORPH_ELLIPSE;
    Erode(input, output, size, type);
    Dilate(output, output, size, type);
}

Mat mult(const Mat &mask, const Mat &result)
{
    Mat maskC = mask.clone();
    Mat resultC = result.clone();
    for (int y = 0; y < mask.size().height; ++y)
    {
        uchar *init = maskC.ptr<uchar>(y);
        uchar *pointer_ret = resultC.ptr<uchar>(y);
        for (int x = 0; x < mask.size().width; ++x)
        {
            pointer_ret[x * 3 + 0] *= init[x] / 255;
            pointer_ret[x * 3 + 1] *= init[x] / 255;
            pointer_ret[x * 3 + 2] *= init[x] / 255;
        }
    }
    return resultC;
}
// Input: cam video
// Output: only person

int main()
{
    // Load Image
    AddImage = imread(AddImageFile.c_str());
    // End Load Image
    VideoCapture vc(0);
    Mat backgroundImage;
    Mat getFrame;
    Mat result;
    Mat grayFrame;
    char command;

    std::cout << "배경 변경 프로그램을 실행합니다. 우선 배경 인식을 위해 카메라 밖으로 벗어나주세요.\n";
    std::cout << "추가 배경 이미지를 삽입할 경로를 확인해주세요. 현재 경로는 " << AddImageFile << " 입니다.\n\n";
    for (int i = 5; i >= 0; i--)
    {
        std::cout << "\r" << i << "초 후 배경을 촬영합니다. 카메라 밖으로 벗어나주세요.";
        sleep(1 * TOSECONDS);
    }

    std::cout << "\n\nSetting Background..\t";
    backgroundImage = SetBackgroundImage(vc);

    cvtColor(backgroundImage.clone(), mask, CV_BGR2GRAY);

    for (int y = 0; y < mask.size().height; ++y)
    {
        uchar *init = mask.ptr<uchar>(y);
        for (int x = 0; x < mask.size().width; ++x)
        {
            init[x] = 0;
        }
    }
    bool isQuit = false;
    bool isOnlyBG = true;
    bool isWithADimg = false;
    bool showCommand = true;

    while (true)
    {

        if (showCommand)
        {
            std::cout << "Done\n\nVideo Input Start\n";
            std::cout << "Command List\n";
            std::cout << "1: Set background\t 2: Only remove background\t 3: Remove background with Addtional image\t 4: Change Additional image Directory q: Quit program\n";
            showCommand = false;
        }

        vc.read(getFrame);
        imshow("Origin Image", getFrame);

        result = RemoveBackground(backgroundImage, getFrame, 65);

        Dilate(mask, mask, 7);
        Dilate(mask, mask, 7);
        Dilate(mask, mask, 7);
        Dilate(mask, mask, 7);
        Erode(mask, mask, 7);
        Erode(mask, mask, 7);
        Erode(mask, mask, 7);
        Erode(mask, mask, 7);

        Dilate(mask, mask, 2);
        imshow("Binary Mask Image", mask);
        result = mult(mask, result);

        if (isOnlyBG)
            imshow("Only remove background", result);

        AddImageMask = ~mask;
        //-------------------------------------------------- Add Image ---------------------------------------------//
        cv::resize(AddImage, AddImage, mask.size());

        Mat toAdd = mult(AddImageMask, AddImage);

        result = result + toAdd;

        if (isWithADimg)
            imshow("Remove background with Addtional image", result);

        command = waitKey(30);

        switch (command)
        {
        case '1':
            std::cout << "1을 입력하셨습니다. 배경을 재설정합니다.\n";
            for (int i = 5; i >= 0; i--)
            {
                std::cout << "\r" << i << "초 후 배경을 촬영합니다. 카메라 밖으로 벗어나주세요.";
                sleep(1 * TOSECONDS);
            }
            std::cout << "\nSetting background..";
            backgroundImage = SetBackgroundImage(vc);
            std::cout << "\tDone\n";
            showCommand = true;
            break;
        case '2':
            std::cout << "2를 입력하셨습니다. 배경만 제거합니다.\n";
            isOnlyBG = true;
            isWithADimg = false;
            showCommand = true;
            destroyWindow("Remove background with Addtional image");
            break;
        case '3':
            std::cout << "3를 입력하셨습니다. 추가 이미지를 입힙니다.\n";
            isOnlyBG = false;
            isWithADimg = true;
            showCommand = true;
            destroyWindow("Only remove background");
            break;
        case '4':
        {
            std::cout << "4를 입력하셨습니다. 추가 이미지 디렉토리를 변경합니다.\n";
            std::getline(std::cin, AddImageFile);
            std::cout << "입력한 경로 : " << AddImageFile << "\n";
            Mat tmp = imread(AddImageFile.c_str());
            if (!tmp.data)
            {
                std::cout << "파일을 여는 데 실패했습니다. 이전 이미지 디렉토리로 돌아갑니다.\n";
            }
            else
            {
                AddImage = tmp;
            }
            showCommand = true;
        }
        break;
        case 'q':
        case 'Q':
            std::cout << "q를 입력하셨습니다. 프로그램을 종료합니다.\n";
            isQuit = true;
            break;
        default:
            break;
        }

        if (isQuit)
            break;

        //if (waitKey(30) == 27) break;
    }

    return 0;
}