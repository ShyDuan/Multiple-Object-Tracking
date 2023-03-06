#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QApplication>
#include "qimage.h"
#include "qlabel.h"
#include "qtimer.h"
#include <QTime>
#include "qpixmap.h"
#include "qdebug.h"
#include <qdesktopwidget.h>

#include <iostream>
#include <fstream>
#include <map>
#include <random>
#include <chrono>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
//#include <opencv2/highgui.hpp>

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include "tracker.h"
#include "utils.h"

// 读取Label文件，读取数据
std::vector<std::vector<cv::Rect>> ProcessLabel(std::ifstream& label_file) {
    // 处理label————按帧索引对边界框进行分组
    std::vector<std::vector<cv::Rect>> bbox;
    std::vector<cv::Rect> bbox_per_frame;
    // Label索引
    int current_frame_index = 1;
    std::string line;

    while (std::getline(label_file, line)) {
        std::stringstream ss(line);
        // Label格式 <frame>, <id>, <bb_left>, <bb_top>, <bb_width>, <bb_height>, <conf>, <x>, <y>, <z>
        std::vector<float> label;
        std::string data;
        while (getline(ss , data, ',')) {
            label.push_back(std::stof(data));
        }

        if (static_cast<int>(label[0]) != current_frame_index) {
            current_frame_index = static_cast<int>(label[0]);
            bbox.push_back(bbox_per_frame);
            bbox_per_frame.clear();
        }

        // 忽略低置信度的检测结果，kMin请在utils.h中设置
        if (label[6] > kMinConfidence) {
            bbox_per_frame.emplace_back(label[2], label[3], label[4], label[5]);
        }
    }
    // 添加最后一帧的边界框
    bbox.push_back(bbox_per_frame);
    return bbox;
}

// mat输出至QLable
void LabelDisplayMat(QLabel *label, cv::Mat &mat)
{
    cv::Mat Rgb;
    QImage Img;
    if (mat.channels() == 3)//RGB Img
    {
        cv::cvtColor(mat, Rgb, CV_BGR2RGB);//颜色空间转换
        Img = QImage((const uchar*)(Rgb.data), Rgb.cols, Rgb.rows, Rgb.cols * Rgb.channels(), QImage::Format_RGB888);
    }
    else//Gray Img
    {
        Img = QImage((const uchar*)(mat.data), mat.cols, mat.rows, mat.cols*mat.channels(), QImage::Format_Indexed8);
    }
    label->setPixmap(QPixmap::fromImage(Img));
}


QImage cvMat2QImage(const cv::Mat& mat)
{
    // 8-bits unsigned, NO. OF CHANNELS = 1
    if(mat.type() == CV_8UC1)
    {
        QImage image(mat.cols, mat.rows, QImage::Format_Indexed8);
        // Set the color table (used to translate colour indexes to qRgb values)
        image.setColorCount(256);
        for(int i = 0; i < 256; i++)
        {
            image.setColor(i, qRgb(i, i, i));
        }
        // Copy input Mat
        uchar *pSrc = mat.data;
        for(int row = 0; row < mat.rows; row ++)
        {
            uchar *pDest = image.scanLine(row);
            memcpy(pDest, pSrc, mat.cols);
            pSrc += mat.step;
        }
        return image;
    }
    // 8-bits unsigned, NO. OF CHANNELS = 3
    else if(mat.type() == CV_8UC3)
    {
        // Copy input Mat
        const uchar *pSrc = (const uchar*)mat.data;
        // Create QImage with same dimensions as input Mat
        QImage image(pSrc, mat.cols, mat.rows, mat.step, QImage::Format_RGB888);
        return image.rgbSwapped();
    }
    else if(mat.type() == CV_8UC4)
    {
        qDebug() << "CV_8UC4";
        // Copy input Mat
        const uchar *pSrc = (const uchar*)mat.data;
        // Create QImage with same dimensions as input Mat
        QImage image(pSrc, mat.cols, mat.rows, mat.step, QImage::Format_ARGB32);
        return image.copy();
    }
    else
    {
        qDebug() << "ERROR: Mat could not be converted to QImage.";
        return QImage();
    }
}

void WaitSleep(int msec)
{
    QTime dieTime = QTime::currentTime().addMSecs(msec);
    while( QTime::currentTime() < dieTime )
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
}


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    MainWindow w;
    QDesktopWidget *desktop = QApplication::desktop();
    QLabel* label_tracking = new QLabel("", 0);
    label_tracking->resize(desktop->width(), desktop->height());
//    label_tracking->setScaledContents(true);
    label_tracking->setAlignment(Qt::AlignCenter);


    // 解析程序输入参数
    boost::program_options::options_description desc{"Options"};
    desc.add_options()
            ("help,h", "Help screen")
            ("display,d", "Display online tracker output (slow) [False]");

    boost::program_options::variables_map vm;
    boost::program_options::store(parse_command_line(argc, argv, desc), vm);
    boost::program_options::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << '\n';
        return -1;
    }

    // 判断是否需要可视化显示
    bool enable_display_flag = false;
    if (vm.count("display")) {
        enable_display_flag = true;
    }

    // 当需要可视化显示时
    std::vector<cv::Scalar> colors;
    if (enable_display_flag) {
        // 创建窗口显示原始图像
//        cv::namedWindow("Original", cv::WINDOW_AUTOSIZE);
        // 创建窗口显示跟踪结果
//        cv::namedWindow("Tracking", CV_WINDOW_NORMAL);
//        cv::setWindowProperty("Tracking", CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);  // set full sereen

        //  qt window

        // 生成随机颜色以可视化不同的bbox
        std::random_device rd;  // Will be used to obtain a seed for the random number engine
        std::mt19937 gen(rd()); // Standard mersenne_twister_engine seeded with rd()
        constexpr int max_random_value = 20;
        std::uniform_int_distribution<> dis(0, max_random_value);
        constexpr int factor = 255 / max_random_value;

        for (int n = 0; n < kNumColors; ++n) {
            // 使用dis把随机生成的无符号int转换为【0，7】中的int
            // Use dis to transform the random unsigned int generated by gen into an int in [0, 7]
            colors.emplace_back(cv::Scalar(dis(gen) * factor, dis(gen) * factor, dis(gen) * factor));
        }
    }


    std::string dataYear = "MOT15";
    std::vector<std::string> dataset_names{"ADL-Rundle-6",
//                                           "ADL-Rundle-8",
                                           "ETH-Bahnhof",
//                                           "ETH-Pedcross2",
                                           "ETH-Sunnyday",
//                                           "KITTI-13",
                                           "KITTI-17",
                                           "PETS09-S2L1",
                                           "TUD-Campus",
                                           "TUD-Stadtmitte",
                                           "Venice-2"};

    // 创建SORT跟踪器
    Tracker tracker;

    // 打印输出文件outlog
    std::ofstream outlog;
    outlog.open("../outlog.txt");

    for (const auto& dataset_name : dataset_names) {
        // 打开label文件，检测加载结果
        std::string label_path = "../" + dataYear + "Labels/train/" + dataset_name + "/det/det.txt";
        std::ifstream label_file(label_path);
        if (!label_file.is_open()) {
            std::cerr << "Could not open or find the label!!!" << std::endl;
            return -1;
        }
        std::vector<std::vector<cv::Rect>> all_detections = ProcessLabel(label_file);
        // 关闭label文件
        label_file.close();

        // 加载用于可视化的图像路径
        std::vector<cv::String> images;
        if (enable_display_flag) {
            // 加载图像
            cv::String path("../MOT15/train/" + dataset_name + "/img1/*.jpg");
            // Non-recursive
            cv::glob(path, images);
        }

        // 如不存在，创建输出文件夹
        std::string output_folder = "../" + dataYear + "output/";
        boost::filesystem::path output_folder_path(output_folder);
        if(boost::filesystem::create_directory(output_folder_path)) {
            std::cerr<< "Directory Created: "<< output_folder <<std::endl;
        }

        std::string output_path = output_folder + dataset_name + ".txt";
        std::ofstream output_file(output_path);

        if (output_file.is_open()) {
            std::cout << "Result will be exported to " << output_path << std::endl;
        } else {
            std::cerr << "Unable to open output file" << std::endl;
            return -1;
        }

        size_t total_frames = all_detections.size();

        auto t1 = std::chrono::high_resolution_clock::now();
        for (size_t i = 0; i < total_frames; i++) {
            auto frame_index = i + 1;

            outlog << "************* NEW FRAME ************* " << std::endl;

            /*** Run SORT tracker ***/
            const auto &detections = all_detections[i];
            tracker.Run(detections);
            const auto tracks = tracker.GetTracks();
            /*** Tracker update done ***/

            outlog << "Raw detections:" << std::endl;
            for (const auto &det : detections) {
                outlog << frame_index << "," << "-1" << "," << det.tl().x << "," << det.tl().y
                          << "," << det.width << "," << det.height << std::endl;
            }
            outlog << std::endl;

            for (auto &trk : tracks) {
                const auto &bbox = trk.second.GetStateAsBbox();
                // Note that we will not export coasted tracks
                // If we export coasted tracks, the total number of false negative will decrease (and maybe ID switch)
                // However, the total number of false positive will increase more (from experiments),
                // which leads to MOTA decrease
                // Developer can export coasted cycles if false negative tracks is critical in the system
                if (trk.second.coast_cycles_ < kMaxCoastCycles
                && (trk.second.hit_streak_ >= kMinHits || frame_index < kMinHits)) {
                    // 打印debug结果
                    outlog << frame_index << "," << trk.first << "," << bbox.tl().x << "," << bbox.tl().y
                              << "," << bbox.width << "," << bbox.height << ",1,-1,-1,-1"
                              << " Hit Streak = " << trk.second.hit_streak_
                              << " Coast Cycles = " << trk.second.coast_cycles_ << std::endl;

                    // 导出到文本文件以进行结果评估
                    output_file << frame_index << "," << trk.first << "," << bbox.tl().x << "," << bbox.tl().y
                                << "," << bbox.width << "," << bbox.height << ",1,-1,-1,-1\n";

                }
            }

            // 可视化跟踪结果
            if (enable_display_flag) {
                // 读取图像文件
                cv::Mat img = cv::imread(images[i]);
                cv::Mat img_tracking = img.clone();

                if (img.empty()) {
                    std::cerr << "Could not open or find the image!!!" << std::endl;
                    return -1;
                }

//                for (const auto &det : detections) {
//                    // 在红色边框中绘制检测结果
//                    cv::rectangle(img, det, cv::Scalar(0, 0, 255), 3);
//                }

                for (auto &trk : tracks) {
                    // 仅绘制符合特定条件的轨迹
                    if (trk.second.coast_cycles_ < kMaxCoastCycles &&
                        (trk.second.hit_streak_ >= kMinHits || frame_index < kMinHits)) {
                        const auto &bbox = trk.second.GetStateAsBbox();
                        cv::putText(img_tracking, std::to_string(trk.first), cv::Point(bbox.tl().x, bbox.tl().y - 10),
                                    cv::FONT_HERSHEY_DUPLEX, 2, cv::Scalar(255, 255, 255), 2);
                        cv::rectangle(img_tracking, bbox, colors[trk.first % kNumColors], 3);
                    }
                }

                // 显示检测和跟踪结果
//                cv::imshow("Original", img);
//                cv::imshow("Tracking", img_tracking);
                QImage img_show = cvMat2QImage(img_tracking);
                QPixmap mp;
                mp = mp.fromImage(img_show);
                QPixmap fixMP = mp.scaled(desktop->width(), desktop->height(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
                label_tracking->setPixmap(fixMP);
                label_tracking->show();

                WaitSleep(1);

//                // 延迟（毫秒）
//                auto key = cv::waitKey(33);

//                // 按下ESC退出
//                if (27 == key) {
//                    return 0;
//                }
//                else if (32 == key) {
//                    // 按空格暂停，再按一次继续
//                    while (true) {
//                        key = cv::waitKey(0);
//                        if (32 == key) {
//                            break;
//                        } else if (27 == key) {
//                            return 0;
//                        }
//                    }
//                }

            } // 可视化结束
        } // 迭代所有帧结束

        // 计算处理时间，得出帧率
        auto t2 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);

        outlog << "********************************" << std::endl;
        outlog << "Total tracking took: " << time_span.count() << " for " << total_frames << " frames" << std::endl;
        outlog << "FPS = " << total_frames / time_span.count() << std::endl;
        if (enable_display_flag) {
            std::cout << "FPS = " << total_frames / time_span.count() << std::endl;
            std::cout << "Note: to get real runtime results run without the option: --display" << std::endl;
        }
        outlog << "********************************" << std::endl;

        output_file.close();
    }
    // 迭代所有数据文件后关闭outlog
    outlog.close();

    return a.exec();
}
