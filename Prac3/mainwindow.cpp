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
    matcher = cv::BFMatcher::create(cv::NORM_HAMMING,false);
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
    qDebug("hago el detect and compute");
    std::vector<std::vector<DMatch>> matches;

    if(!matcher->empty() && KP.size()>10){
        qDebug("El matcher es valido");
        
        matcher->knnMatch(desc,matches,3);
        qDebug("llego aqui");

        //convert matches

        std::vector<std::vector<std::vector<DMatch>>> objectMatches;

        objectMatches.resize(3);

        //clear

        for (int i = 0; i<(int) objectMatches.size() ;i++ ) {
            objectMatches[i].resize(3);
        }
        qDebug("hago el clear");

        int threshold = 30;

        for(std::vector<DMatch> match: matches){
            for (DMatch m: match){
                if(m.distance < threshold){
                    objectMatches[colect2object[m.imgIdx/3]][m.imgIdx % 3].push_back(m);
                }
            }
        }
        qDebug("comienzo el proceso de la homografia");
        //Homografía
        int indexrow = 0;
        int indexcol = 0;

        int bestIndexY = -1;
        int bestIndexX = -1;
        int tam=0;
        //por cada fila de objetcMatches
        for(std::vector<std::vector<DMatch>> row: objectMatches){
            for(std::vector<DMatch> matches: row){ //por cada vector de matches
                //save index of the row
                qDebug()<<"el tamaño es:";
                qDebug()<<matches.size();
                qDebug()<<"col:";
                qDebug()<<indexcol;
                qDebug()<<"fila:";
                qDebug()<<indexrow;
                if(matches.size() > tam){ //guardamos el mejor vector de cada fila
                    tam = matches.size();
                    bestIndexX = indexcol;
                    bestIndexY = indexrow;
                }
                indexcol++;
            }
            indexrow++;
            indexcol = 0;
        }
        qDebug()<<"guardo los puntos: ";
        qDebug()<<bestIndexY;
        qDebug()<<bestIndexX;
        //guardamos los puntos

        if(bestIndexX!=-1&&bestIndexY!=-1&&tam>10){
            std::vector<Point2f> imagePoints;
            std::vector<Point2f> objectPoints;

            for (int i=0;i<tam ;i++ ) {
                imagePoints.push_back(KP[objectMatches[bestIndexY][bestIndexX][i].queryIdx].pt);
                objectPoints.push_back(objetos[bestIndexY].puntosClave[bestIndexX][objectMatches[bestIndexY][bestIndexX][i].trainIdx].pt);
            }
            //        imagePoints.push_back(KP[objectMatches[bestIndexY][bestIndexX][0].queryIdx].pt);
            //        imagePoints.push_back(KP[objectMatches[bestIndexY][bestIndexX][1].queryIdx].pt);
            //        imagePoints.push_back(KP[objectMatches[bestIndexY][bestIndexX][2].queryIdx].pt);
            qDebug("puntos de imagen guardados");
            //        objectPoints.push_back(objetos[bestIndexY].puntosClave[bestIndexX][objectMatches[bestIndexY][bestIndexX][0].trainIdx].pt);
            //        objectPoints.push_back(objetos[bestIndexY].puntosClave[bestIndexX][objectMatches[bestIndexY][bestIndexX][1].trainIdx].pt);
            //        objectPoints.push_back(objetos[bestIndexY].puntosClave[bestIndexX][objectMatches[bestIndexY][bestIndexX][2].trainIdx].pt);

            qDebug("comienzo homografia");
            auto H = findHomography(objectPoints,imagePoints,LMEDS);
            qDebug("comienzo a guardar esquinas");
            std::vector<Point2f> objectCorners;
            std::vector<Point2f> imageCorners;
            float w = objetos[bestIndexY].imagen[bestIndexX].cols;
            float h = objetos[bestIndexY].imagen[bestIndexX].rows;
            objectCorners = {Point2f(0, 0), Point2f(w-1, 0), Point2f(w-1, h-1), Point2f(0, h-1)};
            qDebug("termino guardar esquinas");

            perspectiveTransform(objectCorners,imageCorners,H);
            qDebug("transformo prespectiva");

            //create a Qvector of QPoints
            QVector<QPoint> imageCornersQ;
            //convert imageCorners to QVector
            for(Point2f p: imageCorners)
                imageCornersQ.push_back(QPoint(p.x,p.y));
            visorS->drawPolyLine(imageCornersQ,Qt::red);


            qDebug("termino los matches");
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
        winSelected=false;
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
        cv::resize(objetos[ui->comboBox->currentIndex()].imagen[0], objetos[ui->comboBox->currentIndex()].imagen[1], Size(0,0), 1.5, 1.5, INTER_LINEAR);
        //scale image[1] x0.5
        cv::resize(objetos[ui->comboBox->currentIndex()].imagen[0], objetos[ui->comboBox->currentIndex()].imagen[2], Size(0,0), 0.75, 0.75, INTER_LINEAR);

        orb->detectAndCompute(objetos[ui->comboBox->currentIndex()].imagen[0], Mat(), objetos[ui->comboBox->currentIndex()].puntosClave[0], objetos[ui->comboBox->currentIndex()].descriptors[0]);
        orb->detectAndCompute(objetos[ui->comboBox->currentIndex()].imagen[1], Mat(), objetos[ui->comboBox->currentIndex()].puntosClave[1], objetos[ui->comboBox->currentIndex()].descriptors[1]);
        orb->detectAndCompute(objetos[ui->comboBox->currentIndex()].imagen[2], Mat(), objetos[ui->comboBox->currentIndex()].puntosClave[2], objetos[ui->comboBox->currentIndex()].descriptors[2]);
        
        //create & regenerate description collection
        if(!objetos[ui->comboBox->currentIndex()].puntosClave[0].empty()&&!objetos[ui->comboBox->currentIndex()].puntosClave[1].empty()&&!objetos[ui->comboBox->currentIndex()].puntosClave[2].empty()){
            matcher->clear();
            int counter = 0;

            for(int i = 0; i < 3; i++){
                if(objetos[i].valid){
                    matcher->add(objetos[ui->comboBox->currentIndex()].descriptors[i]);
                    colect2object[counter]=i;
                    counter++;
                }
            }
        }else{
            objetos[ui->comboBox->currentIndex()].valid = false;

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





