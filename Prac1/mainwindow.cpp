#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QToolTip>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    cap = new VideoCapture(0);
    winSelected = false;

    colorImage.create(240,320,CV_8UC3);
    grayImage.create(240,320,CV_8UC1);
    destColorImage.create(240,320,CV_8UC3);
    destColorImage.setTo(0);
    destGrayImage.create(240,320,CV_8UC1);
    destGrayImage.setTo(0);

    visorS = new ImgViewer(&grayImage, ui->imageFrameS);
    visorD = new ImgViewer(&destGrayImage, ui->imageFrameD);

    connect(&timer,SIGNAL(timeout()),this,SLOT(compute()));
    connect(ui->captureButton,SIGNAL(clicked(bool)),this,SLOT(start_stop_capture(bool)));
    connect(ui->colorButton,SIGNAL(clicked(bool)),this,SLOT(change_color_gray(bool)));
    connect(visorS,SIGNAL(mouseSelection(QPointF, int, int)),this,SLOT(selectWindow(QPointF, int, int)));
    connect(visorS,SIGNAL(mouseClic(QPointF)),this,SLOT(deselectWindow(QPointF)));
    timer.start(30);

}

MainWindow::~MainWindow()
{
    delete ui;
    delete cap;
    delete visorS;
    delete visorD;
    colorImage.release();
    grayImage.release();
    destColorImage.release();
    destGrayImage.release();
}

void MainWindow::compute()
{

    //Captura de imagen
    if(ui->captureButton->isChecked() && cap->isOpened())
    {
        *cap >> colorImage;
        cv::resize(colorImage, colorImage, Size(320,240));
        cvtColor(colorImage, grayImage, COLOR_BGR2GRAY);
        cvtColor(colorImage, colorImage, COLOR_BGR2RGB);

    }


    //En este punto se debe incluir el código asociado con el procesamiento de cada captura


    //Actualización de los visores

    if(winSelected)
        visorS->drawSquare(QRect(imageWindow.x, imageWindow.y, imageWindow.width,imageWindow.height), Qt::green );

    visorS->update();
    visorD->update();

}

void MainWindow::start_stop_capture(bool start)
{
    if(start)
        ui->captureButton->setText("Stop capture");
    else
        ui->captureButton->setText("Start capture");
}

void MainWindow::change_color_gray(bool color)
{
    if(color)
    {
        ui->colorButton->setText("Gray image");
        visorS->setImage(&colorImage);
        destColorImage = colorImage.clone();
        visorD->setImage(&destColorImage);
    }
    else
    {
        ui->colorButton->setText("Color image");
        visorS->setImage(&grayImage);
        destGrayImage = grayImage.clone();
        visorD->setImage(&destGrayImage);
    }
}

void MainWindow::selectWindow(QPointF p, int w, int h)
{
    QPointF pEnd;
    if(w>0 && h>0)
    {
        imageWindow.x = p.x()-w/2;
        if(imageWindow.x<0)
            imageWindow.x = 0;
        imageWindow.y = p.y()-h/2;
        if(imageWindow.y<0)
            imageWindow.y = 0;
        pEnd.setX(p.x()+w/2);
        if(pEnd.x()>=320)
            pEnd.setX(319);
        pEnd.setY(p.y()+h/2);
        if(pEnd.y()>=240)
            pEnd.setY(239);
        imageWindow.width = pEnd.x()-imageWindow.x+1;
        imageWindow.height = pEnd.y()-imageWindow.y+1;

        winSelected = true;
    }
}

void MainWindow::deselectWindow(QPointF p)
{
    std::ignore = p;
    winSelected = false;
     if(ui->colorButton->isChecked()){
         Vec3b data = colorImage.at<Vec3b>(p.y(), p.x());

         //show a Qtootip with the color of the pixel selected in the image window (RGB) at the mouse position
         QString text = QString("R: %1 G: %2 B: %3").arg(data[0]).arg(data[1]).arg(data[2]);
         QToolTip::showText(QCursor::pos(), text);
     } else {
         //the same but for gray image
            QString text = QString("Gray: %1").arg(grayImage.at<uchar>(p.y(), p.x()));
            QToolTip::showText(QCursor::pos(), text);

     }



}



void MainWindow::on_FileRead_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"),"/home",tr("Images (*.png *.xpm *.jpg *.jpeg)"));

    if(fileName.isEmpty()){
        QMessageBox::information(this, "Error", "No file selected");

    }
    else
    {
        colorImage = imread(fileName.toStdString());
        cvtColor(colorImage, grayImage, COLOR_BGR2GRAY);
        cvtColor(colorImage, colorImage, COLOR_BGR2RGB);
        cv::resize(colorImage, colorImage, Size(320,240));
        cv::resize(grayImage, grayImage, Size(320,240));
        if(ui->colorButton->isChecked())
        {
            destColorImage = colorImage.clone();
        }else{
            destGrayImage = grayImage.clone();
        }
        visorD->update();
    }
}


void MainWindow::on_SaveToFile_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"),"/home/example.png",tr("Images (*.png *.xpm *.jpg)"));
    Mat imageToSave;
    if(ui->colorButton->isChecked()){

        cvtColor(destColorImage, imageToSave, COLOR_RGB2BGR);
        imwrite(fileName.toStdString(), imageToSave);
    }else{
        cvtColor(destGrayImage, imageToSave, COLOR_GRAY2BGR);
        imwrite(fileName.toStdString(), imageToSave);
    }
}


void MainWindow::on_CopyChannels_clicked()
{
    if(ui->colorButton->isChecked()){
        std::vector<Mat> channels;
        split(colorImage,channels);

        //if checkBox_channelR is checked, copy channel 0 to destImage
        if(!ui->checkBox_channelR->isChecked())
            channels[0].setTo(0);
        //if checkBox_channelG is checked, copy channel 1 to destImage
        if(!ui->checkBox_channelG->isChecked())
            channels[1].setTo(0);
        //if checkBox_channelB is checked, copy channel 2 to destImage
        if(!ui->checkBox_channelB->isChecked())
            channels[2].setTo(0);
        merge(channels,destColorImage);

        visorD->update();

    }
}


void MainWindow::on_copyWindow_clicked()
{
    if(winSelected){
        int x = (320-imageWindow.width)/2;
        int y = (240-imageWindow.height)/2;
        if(ui->colorButton->isChecked()){
            destColorImage.setTo(0);
            colorImage(imageWindow).copyTo(destColorImage(Rect(x,y,imageWindow.width,imageWindow.height))); //cambiar newImage por dest
        }else{
            destGrayImage.setTo(0);
            grayImage(imageWindow).copyTo(destGrayImage(Rect(x,y,imageWindow.width,imageWindow.height))); //cambiar newImage por dest
        }
        visorD->update();
    }else{
        QMessageBox::information(this, "Warning", "No area selected");
    }
}


void MainWindow::on_resizeWin_clicked()
{
    if(winSelected){
        if(ui->colorButton->isChecked()){
            Mat cropped = Mat(colorImage,imageWindow);
            cv::resize(cropped, destColorImage, Size(320,240));
        }else{
            Mat cropped = Mat(grayImage,imageWindow);
            cv::resize(cropped, destGrayImage, Size(320,240));
        }
        visorD->update();
    }else{
        QMessageBox::information(this, "Warning", "No area selected");
    }
}


void MainWindow::on_EnlargeWin_clicked()
{
    if(winSelected){
        float  fx = 320.0/imageWindow.width;
        float  fy = 240.0/imageWindow.height;
        fx = std::min(fx,fy);
        if(ui->colorButton->isChecked()){
            destColorImage.setTo(0);

            Mat cropped = Mat(colorImage,imageWindow);
            cv::resize(cropped,cropped,Size(),fx,fx);
            int x = (320-cropped.cols)/2;
            int y = (240-cropped.rows)/2;
            cropped.copyTo(destColorImage(Rect(x,y,cropped.cols,cropped.rows)));
        }else{
            destGrayImage.setTo(0);

            Mat cropped = Mat(grayImage,imageWindow);
            cv::resize(cropped,cropped,Size(),fx,fx);
            int x = (320-cropped.cols)/2;
            int y = (240-cropped.rows)/2;
            cropped.copyTo(destGrayImage(Rect(x,y,cropped.cols,cropped.rows)));
        }
        visorD->update();


    }else{
        QMessageBox::information(this, "Warning", "No area selected");
    }
}

