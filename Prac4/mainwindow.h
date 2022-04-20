#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <fstream>

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/video/video.hpp>
#include <opencv2/features2d/features2d.hpp>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/dnn/dnn.hpp>

#include <imgviewer.h>
#include <ui_categoriesForm.h>
#include <QFileDialog>
#include <QMessageBox>


using namespace cv;

namespace Ui {
    class MainWindow;
}


class CategoriesDialog : public QDialog, public Ui::Dialog
{
    Q_OBJECT
public:
    CategoriesDialog(QDialog *parent=0) : QDialog(parent){
        setupUi(this);
    }
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
    bool winSelected;
    Rect imageWindow;
    CategoriesDialog categoriesDialog;
    dnn::Net net;
    QColor colors[21];

public slots:
    void compute();
    void start_stop_capture(bool start);
    void change_color_gray(bool color);
    void selectWindow(QPointF p, int w, int h);
    void deselectWindow(QPointF p);

private slots:
    void on_loadImageButton_clicked();
    void on_categoriesButton_clicked();
    void on_segmentButton_clicked();
    void on_combineButton_clicked();
};


#endif // MAINWINDOW_H
