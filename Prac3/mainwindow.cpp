#include "mainwindow.h"
#include "ui_mainwindow.h"

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
    connect(ui->addButton,SIGNAL(clicked(bool)),this, SLOT(add_object()));
    connect(ui->delButton,SIGNAL(clicked(bool)),this, SLOT(del_object()));
    connect(visorS,SIGNAL(mouseSelection(QPointF, int, int)),this,SLOT(selectWindow(QPointF, int, int)));
    connect(visorS,SIGNAL(mouseClic(QPointF)),this,SLOT(deselectWindow(QPointF)));
    orb =  cv::ORB::create();
    matcher = cv::BFMatcher(cv::NORM_HAMMING,true);
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

    if(objetos[ui->comboBox->currentIndex()].valid){
        destGrayImage.setTo(0);
        objetos[ui->comboBox->currentIndex()].imagen[0].copyTo(destGrayImage(Rect(objetos[ui->comboBox->currentIndex()].puntos.x,objetos[ui->comboBox->currentIndex()].puntos.y,objetos[ui->comboBox->currentIndex()].imagen[0].cols,objetos[ui->comboBox->currentIndex()].imagen[0].rows))); //cambiar newImage por dest
    }else{
        destGrayImage.setTo(0);
    }

    //ahora hacemos el matching
    orb->detectAndCompute(grayImage,Mat(),KP,desc);

    std::vector<std::vector<DMatch>> matches;

    if(!matcher.empty()){

        matcher.knnMatch(desc,matches,3);

        for(int i = 0; i < matches.size() ; i++){
            for(int j = 0 ; j<3 ; j++){
                if(matches[i][j].distance <= 30){
                    
                }
            }
            
        }
    }


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
        visorD->setImage(&destColorImage);
    }
    else
    {
        ui->colorButton->setText("Color image");
        visorS->setImage(&grayImage);
        visorD->setImage(&destGrayImage);
    }
}

void MainWindow::add_object(){
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
        
        objetos[ui->comboBox->currentIndex()].imagen[0].setTo(0);
        objetos[ui->comboBox->currentIndex()].imagen[0] = destGrayImage(Rect(x,y,imageWindow.width,imageWindow.height)).clone();
        objetos[ui->comboBox->currentIndex()].puntos = Point2i(x,y);
        objetos[ui->comboBox->currentIndex()].valid = true;

        //scale image[0] x1.5
        cv::resize(objetos[ui->comboBox->currentIndex()].imagen[0], objetos[0].imagen[1], Size(0,0), 1.5, 1.5, INTER_LINEAR);
        //scale image[1] x0.5
        cv::resize(objetos[ui->comboBox->currentIndex()].imagen[0], objetos[0].imagen[2], Size(0,0), 0.75, 0.75, INTER_LINEAR);

        orb->detectAndCompute(objetos[ui->comboBox->currentIndex()].imagen[0], Mat(), objetos[ui->comboBox->currentIndex()].puntosClave[0], objetos[ui->comboBox->currentIndex()].descriptors[0]);
        orb->detectAndCompute(objetos[ui->comboBox->currentIndex()].imagen[1], Mat(), objetos[ui->comboBox->currentIndex()].puntosClave[1], objetos[ui->comboBox->currentIndex()].descriptors[1]);
        orb->detectAndCompute(objetos[ui->comboBox->currentIndex()].imagen[2], Mat(), objetos[ui->comboBox->currentIndex()].puntosClave[2], objetos[ui->comboBox->currentIndex()].descriptors[2]);
        
        //create description collection
        for(int i = 0; i < 3; i++){
            if(!objetos[ui->comboBox->currentIndex()].descriptors[i].empty()){
                matcher.add(objetos[ui->comboBox->currentIndex()].descriptors[i]);
            }
        }

    }
}

void MainWindow::del_object(){

    objetos[ui->comboBox->currentIndex()].valid=false;

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
}





