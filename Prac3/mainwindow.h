#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/video/video.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/calib3d/calib3d.hpp>

#include <imgviewer.h>



using namespace cv;

namespace Ui {
    class MainWindow;
}

struct Objeto{
    Mat imagen[3];
    Point2i puntos;
    bool valid=false;
    Mat descriptors[3];
    std::vector<KeyPoint>puntosClave[3];
    std::vector<std::vector<DMatch>> objectMatches[3][3];
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    QTimer timer;

    VideoCapture *cap;
    ImgViewer *visorS, *visorD;
    Mat colorImage, grayImage;
    Mat destColorImage, destGrayImage;
    Objeto objetos[3];
    bool winSelected;
    Rect imageWindow;
    Ptr<ORB> orb;
    BFMatcher matcher;
    std::vector<KeyPoint> KP;
    Mat desc;



public slots:
    void compute();
    void start_stop_capture(bool start);
    void change_color_gray(bool color);
    void add_object();
    void del_object();
    void selectWindow(QPointF p, int w, int h);
    void deselectWindow(QPointF p);

private slots:
};


#endif // MAINWINDOW_H
