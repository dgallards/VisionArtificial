#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    cap = new VideoCapture(0);
    winSelected = false;

    colorImage.create(240, 320, CV_8UC3);
    grayImage.create(240, 320, CV_8UC1);
    destColorImage.create(240, 320, CV_8UC3);
    destColorImage.setTo(0);
    destGrayImage.create(240, 320, CV_8UC1);
    destGrayImage.setTo(0);

    visorS = new ImgViewer(&grayImage, ui->imageFrameS);
    visorD = new ImgViewer(&destGrayImage, ui->imageFrameD);

    visorHistoS = new ImgViewer(260, 150, (QImage *)NULL, ui->histoFrameS);
    visorHistoD = new ImgViewer(260, 150, (QImage *)NULL, ui->histoFrameD);

    connect(&timer, SIGNAL(timeout()), this, SLOT(compute()));
    connect(ui->captureButton, SIGNAL(clicked(bool)), this, SLOT(start_stop_capture(bool)));
    connect(ui->colorButton, SIGNAL(clicked(bool)), this, SLOT(change_color_gray(bool)));
    connect(visorS, SIGNAL(mouseSelection(QPointF, int, int)), this, SLOT(selectWindow(QPointF, int, int)));
    connect(visorS, SIGNAL(mouseClic(QPointF)), this, SLOT(deselectWindow(QPointF)));

    connect(ui->pixelTButton, SIGNAL(clicked()), &pixelTDialog, SLOT(show()));
    connect(pixelTDialog.okButton, SIGNAL(clicked()), &pixelTDialog, SLOT(hide()));

    connect(pixelTDialog.okButton, SIGNAL(clicked()), &pixelTDialog, SLOT(okButtonClicked()));

    connect(ui->kernelButton, SIGNAL(clicked()), &lFilterDialog, SLOT(show()));
    connect(lFilterDialog.okButton, SIGNAL(clicked()), &lFilterDialog, SLOT(hide()));

    connect(ui->operOrderButton, SIGNAL(clicked()), &operOrderDialog, SLOT(show()));
    connect(operOrderDialog.okButton, SIGNAL(clicked()), &operOrderDialog, SLOT(hide()));

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
    if (ui->captureButton->isChecked() && cap->isOpened())
    {
        *cap >> colorImage;
        cv::resize(colorImage, colorImage, Size(320, 240));
        cvtColor(colorImage, grayImage, COLOR_BGR2GRAY);
        cvtColor(colorImage, colorImage, COLOR_BGR2RGB);
    }

    //En este punto se debe incluir el código asociado con el procesamiento de cada captura

    realTimeChanges();

    //Actualización de los visores
    if (!ui->colorButton->isChecked())
    {
        updateHistograms(grayImage, visorHistoS);
        updateHistograms(destGrayImage, visorHistoD);
    }
    else
    {
        //convierte colorImage de espacio de color RGB a YUV
        Mat yuvImage, yuvImage2;
        cvtColor(colorImage, yuvImage, COLOR_RGB2YUV);
        cvtColor(destColorImage, yuvImage2, COLOR_RGB2YUV);
        //separa los canales de la imagen
        std::vector<Mat> yuvChannels, yuvChannels2;
        split(yuvImage, yuvChannels);
        split(yuvImage2, yuvChannels2);
        //actualiza los histogramas
        updateHistograms(yuvChannels[0], visorHistoS);
        updateHistograms(yuvChannels2[0], visorHistoD);
    }

    if (winSelected)
    {
        visorS->drawSquare(QPointF(imageWindow.x + imageWindow.width / 2, imageWindow.y + imageWindow.height / 2), imageWindow.width, imageWindow.height, Qt::green);
    }
    visorS->update();
    visorD->update();
    visorHistoS->update();
    visorHistoD->update();
}

void MainWindow::updateHistograms(Mat image, ImgViewer *visor)
{
    if (image.type() != CV_8UC1)
        return;

    Mat histogram;
    int channels[] = {0, 0};
    int histoSize = 256;
    float grange[] = {0, 256};
    const float *ranges[] = {grange};
    double minH, maxH;

    calcHist(&image, 1, channels, Mat(), histogram, 1, &histoSize, ranges, true, false);
    minMaxLoc(histogram, &minH, &maxH);

    float maxY = visor->getHeight();

    for (int i = 0; i < 256; i++)
    {
        float hVal = histogram.at<float>(i);
        float minY = maxY - hVal * maxY / maxH;

        visor->drawLine(QLineF(i + 2, minY, i + 2, maxY), Qt::red);
    }
}

void MainWindow::start_stop_capture(bool start)
{
    if (start)
        ui->captureButton->setText("Stop capture");
    else
        ui->captureButton->setText("Start capture");
}

void MainWindow::change_color_gray(bool color)
{
    if (color)
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

void MainWindow::selectWindow(QPointF p, int w, int h)
{
    QPointF pEnd;
    if (w > 0 && h > 0)
    {
        imageWindow.x = p.x() - w / 2;
        if (imageWindow.x < 0)
            imageWindow.x = 0;
        imageWindow.y = p.y() - h / 2;
        if (imageWindow.y < 0)
            imageWindow.y = 0;
        pEnd.setX(p.x() + w / 2);
        if (pEnd.x() >= 320)
            pEnd.setX(319);
        pEnd.setY(p.y() + h / 2);
        if (pEnd.y() >= 240)
            pEnd.setY(239);
        imageWindow.width = pEnd.x() - imageWindow.x + 1;
        imageWindow.height = pEnd.y() - imageWindow.y + 1;

        winSelected = true;
    }
}

void MainWindow::deselectWindow(QPointF p)
{
    std::ignore = p;
    winSelected = false;
}

void MainWindow::okButtonClicked()
{
}

void MainWindow::realTimeChanges()
{


    cvtColor(colorImage, yuvColorImage, COLOR_RGB2YUV);
    split(yuvColorImage, yuvChannels);

    auxGrayImage=grayImage.clone();

    switch (ui->operationComboBox->currentIndex())
    {
    case 0: //pixelT
        pixelT();
        break;
    case 1: //thresholding
        threshold();
        break;
    case 2: //equalize
        equalize();
        break;
    case 3: //gauusian blur
        gaussianBlur();
        break;
    case 4: //median blur
        medianFilter();
        break;
    case 5: //linear filter
        linearFilter();
        break;
    case 6: //dilate
        dilate();
        break;
    case 7: //erode
        erode();
        break;
    case 8: //apply sevral filters
        applySeveral();
        break;
    case 9:
        negative();
        break;
    default:

        break;
    }

    merge(yuvChannels, yuvColorImage);
    //convert the image back to RGB
    cvtColor(yuvColorImage, destColorImage, COLOR_YUV2RGB);
    destGrayImage = auxGrayImage;


}

void MainWindow::on_loadButton_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), "/home", tr("Images (*.png *.xpm *.jpg *.jpeg)"));

    if (fileName.isEmpty())
    {
        QMessageBox::information(this, "Error", "No file selected");
    }
    else
    {
        colorImage = imread(fileName.toStdString());
        cvtColor(colorImage, grayImage, COLOR_BGR2GRAY);
        cvtColor(colorImage, colorImage, COLOR_BGR2RGB);
        cv::resize(colorImage, colorImage, Size(320, 240));
        cv::resize(grayImage, grayImage, Size(320, 240));
        if (ui->colorButton->isChecked())
        {
            destColorImage = colorImage.clone();
        }
        else
        {
            destGrayImage = grayImage.clone();
        }
        visorD->update();
    }
}

void MainWindow::on_saveButton_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"), "/home/example.png", tr("Images (*.png *.xpm *.jpg)"));
    Mat imageToSave;
    if (ui->colorButton->isChecked())
    {

        cvtColor(destColorImage, imageToSave, COLOR_RGB2BGR);
        imwrite(fileName.toStdString(), imageToSave);
    }
    else
    {
        cvtColor(destGrayImage, imageToSave, COLOR_GRAY2BGR);
        imwrite(fileName.toStdString(), imageToSave);
    }
}

void MainWindow::pixelT(){
    int r1 = pixelTDialog.grayTransformW->item(0,0)->text().toInt();
    int r2 = pixelTDialog.grayTransformW->item(1,0)->text().toInt();
    int r3 = pixelTDialog.grayTransformW->item(2,0)->text().toInt();
    int r4 = pixelTDialog.grayTransformW->item(3,0)->text().toInt();

    int s1 = pixelTDialog.grayTransformW->item(0,1)->text().toInt();
    int s2 = pixelTDialog.grayTransformW->item(1,1)->text().toInt();
    int s3 = pixelTDialog.grayTransformW->item(2,1)->text().toInt();
    int s4 = pixelTDialog.grayTransformW->item(3,1)->text().toInt();

    std::vector<int> vectorR = {r1, r2, r3, r4};
    std::vector<int> vectorS = {s1, s2, s3, s4};

    std::vector<uchar>tablaWT(256);

    //create the lookup table using the vectorD and vectorS
    for (int i = 0; i<3 ; i++)
    {
        for (int r = vectorR[i] ; r <= vectorR[i+1] ; r++)
        {
            tablaWT[r] = ((r-vectorR[i])*(vectorS[i+1]-vectorS[i]))/(vectorR[i+1]-vectorR[i])+vectorS[i];
        }
    }

    if(ui->colorButton->isChecked()){
        LUT(yuvChannels[0], tablaWT, yuvChannels[0]);
    }else{
        LUT(auxGrayImage,tablaWT,auxGrayImage);
    }


}
void MainWindow::threshold(){
    if (ui->colorButton->isChecked())
    {
        cv::threshold(yuvChannels[0], yuvChannels[0], ui->thresholdSpinBox->value(), 255, THRESH_BINARY);
    }
    else
    {
        cv::threshold(auxGrayImage, auxGrayImage, ui->thresholdSpinBox->value(), 255, THRESH_BINARY);
    }
}

void MainWindow::equalize(){
    if (ui->colorButton->isChecked())
    {
        equalizeHist(yuvChannels[0], yuvChannels[0]);
    }
    else
    {
        equalizeHist(auxGrayImage, auxGrayImage);
    }
}

void MainWindow::gaussianBlur(){
    if (ui->colorButton->isChecked())
    {
        GaussianBlur(yuvChannels[0], yuvChannels[0], Size(ui->gaussWidthBox->value(), ui->gaussWidthBox->value()), 0, 0);
    }
    else
    {
        GaussianBlur(auxGrayImage, auxGrayImage, Size(ui->gaussWidthBox->value(), ui->gaussWidthBox->value()), 0, 0);
    }

}
void MainWindow::linearFilter(){
    Mat kernel = (Mat_<float>(3, 3) << lFilterDialog.kernelWidget->item(0, 0)->text().toFloat(), lFilterDialog.kernelWidget->item(0, 1)->text().toFloat(), lFilterDialog.kernelWidget->item(0, 2)->text().toFloat(),
                  lFilterDialog.kernelWidget->item(1, 0)->text().toFloat(), lFilterDialog.kernelWidget->item(1, 1)->text().toFloat(), lFilterDialog.kernelWidget->item(1, 2)->text().toFloat(),
                  lFilterDialog.kernelWidget->item(2, 0)->text().toFloat(), lFilterDialog.kernelWidget->item(2, 1)->text().toFloat(), lFilterDialog.kernelWidget->item(2, 2)->text().toFloat());

    if (ui->colorButton->isChecked())
    {
        filter2D(yuvChannels[0], yuvChannels[0], -1, kernel);
    }
    else
    {
        filter2D(grayImage, auxGrayImage, -1, kernel);
    }
}
void MainWindow::medianFilter(){
    if (ui->colorButton->isChecked())
    {
        medianBlur(yuvChannels[0], yuvChannels[0], ui->gaussWidthBox->value());
    }
    else
    {
        medianBlur(grayImage, auxGrayImage, ui->gaussWidthBox->value());
    }
}
void MainWindow::dilate(){
    if (ui->colorButton->isChecked())
    {
        //apply threshold
        cv::threshold(yuvChannels[0], yuvChannels[0], ui->thresholdSpinBox->value(), 255, THRESH_BINARY);
        //apply dilate
        cv::dilate(yuvChannels[0], yuvChannels[0], Mat());
    }
    else
    {
        cv::threshold(auxGrayImage, auxGrayImage, ui->thresholdSpinBox->value(), 255, THRESH_BINARY);
        cv::dilate(auxGrayImage, auxGrayImage, Mat());
    }
}
void MainWindow::erode(){
    if (ui->colorButton->isChecked())
    {
        cv::threshold(yuvChannels[0], yuvChannels[0], ui->thresholdSpinBox->value(), 255, THRESH_BINARY);
        //apply erode
        cv::erode(yuvChannels[0], yuvChannels[0], Mat());
    }
    else
    {
        cv::threshold(auxGrayImage, auxGrayImage, ui->thresholdSpinBox->value(), 255, THRESH_BINARY);
        cv::erode(auxGrayImage, auxGrayImage, Mat());
    }
}

void MainWindow::negative(){

    std::vector<int> vectorR = {0, 85, 170, 255};
    std::vector<int> vectorS = {255, 170, 85, 0};

    std::vector<uchar>tablaWT(256);

    //create the lookup table using the vectorD and vectorS
    for (int i = 0; i<3 ; i++)
    {
        for (int r = vectorR[i] ; r <= vectorR[i+1] ; r++)
        {
            tablaWT[r] = ((r-vectorR[i])*(vectorS[i+1]-vectorS[i]))/(vectorR[i+1]-vectorR[i])+vectorS[i];
        }
    }

    if(ui->colorButton->isChecked()){
        LUT(yuvChannels[0], tablaWT, yuvChannels[0]);
    }else{
        LUT(auxGrayImage,tablaWT,auxGrayImage);
    }
}


void MainWindow::applySeveral(){
    std::vector<bool> numOperations = {operOrderDialog.firstOperCheckBox->isChecked(),operOrderDialog.secondOperCheckBox->isChecked(),operOrderDialog.thirdOperCheckBox->isChecked(),operOrderDialog.fourthOperCheckBox->isChecked()};

    std::vector<int> operationIndex = {operOrderDialog.operationComboBox1->currentIndex(),operOrderDialog.operationComboBox2->currentIndex(),operOrderDialog.operationComboBox3->currentIndex(),operOrderDialog.operationComboBox4->currentIndex()};
    for (int i = 0; i<4 ; i++ ) {
        if(numOperations[i]==true){
            switch(operationIndex[i]){
            case 0: //pixelT
                pixelT();
                break;
            case 1: //thresholding
                threshold();
                break;
            case 2: //equalize
                equalize();
                break;
            case 3: //gauusian blur
                gaussianBlur();
                break;
            case 4: //median blur
                medianFilter();
                break;
            case 5: //linear filter
                linearFilter();
                break;
            case 6: //dilate
                dilate();
                break;
            case 7: //erode
                erode();
                break;
            case 8:
                negative();
            }

        }
    }



}

