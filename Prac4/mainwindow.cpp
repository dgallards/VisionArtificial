#include "mainwindow.h"
#include "ui_mainwindow.h"

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

    connect(&timer, SIGNAL(timeout()), this, SLOT(compute()));
    connect(ui->captureButton, SIGNAL(clicked(bool)), this, SLOT(start_stop_capture(bool)));
    connect(ui->colorButton, SIGNAL(clicked(bool)), this, SLOT(change_color_gray(bool)));
    connect(visorS, SIGNAL(mouseSelection(QPointF, int, int)), this, SLOT(selectWindow(QPointF, int, int)));
    connect(visorS, SIGNAL(mouseClic(QPointF)), this, SLOT(deselectWindow(QPointF)));

    connect(ui->categoriesButton, SIGNAL(clicked()), &categoriesDialog, SLOT(show()));
    connect(categoriesDialog.okButton, SIGNAL(clicked()), &categoriesDialog, SLOT(hide()));
    connect(categoriesDialog.okButton, SIGNAL(clicked()), this, SLOT(categoriesButton()));

    net = dnn::readNetFromCaffe("/home/fcn/fcn.prototxt", "/home/fcn/fcn.caffemodel");

    // save in each position in colors the color of the category in each line of the file fcn-colors.txt


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

    // Captura de imagen
    if (ui->captureButton->isChecked() && cap->isOpened())
    {
        *cap >> colorImage;
        cv::resize(colorImage, colorImage, Size(320, 240));
        cvtColor(colorImage, grayImage, COLOR_BGR2GRAY);
        cvtColor(colorImage, colorImage, COLOR_BGR2RGB);
    }

    // En este punto se debe incluir el código asociado con el procesamiento de cada captura

    // Actualización de los visores

    if (winSelected)
        visorS->drawSquare(QRect(imageWindow.x, imageWindow.y, imageWindow.width, imageWindow.height), Qt::green);

    visorS->update();
    visorD->update();
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

void MainWindow::on_loadImageButton_clicked()
{

    ui->captureButton->setChecked(false);
    ui->captureButton->setText("Start capture");
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), "/home", tr("Images (*.png *.xpm *.jpg *.jpeg)"));

    if (fileName.isEmpty())
    {
        QMessageBox::information(this, "Error", "No file selected");
        ui->captureButton->setChecked(true);
        ui->captureButton->setText("Stop capture");
    }
    else
    {
        //cap->release();

        //ui->captureButton->setText("Stop capture");

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

    ui->imageHeightSpinBox->setValue(320);
    ui->imageWidthSpinBox->setValue(240);
}

void MainWindow::on_segmentButton_clicked()
{
    // convertir la greyimage a BGR
    Mat colorImageBGR;

    cvtColor(colorImage, colorImageBGR, COLOR_RGB2BGR);

    Mat blob = dnn::blobFromImage(colorImageBGR, 1.0, Size(ui->imageWidthSpinBox->value(), ui->imageHeightSpinBox->value()), mean(colorImageBGR), false, false);
    net.setInput(blob);
    Mat output;
    net.forward(output);
    //for each pixel in the Mat output, set each pixel to the color of the category


    Mat auximage;
    auximage.create(ui->imageHeightSpinBox->value(),ui->imageWidthSpinBox->value(),CV_8UC3);
    //auximage = destColorImage.clone();
    //cvtColor(auximage, auximage, COLOR_RGB2BGR);
    for (int i = 0; i < auximage.rows; i++)
    {
        for (int j = 0; j < auximage.cols; j++)
        {
            //get the greater value from the output and save the index of the category
            float bestValue=0.0;
            int category=0;
            for(int k = 0; k < 21; k++)
            {
                int idx[4] = {0, k, i, j};
                if(output.at<float>(idx) > bestValue)
                {
                    bestValue = output.at<float>(idx);
                    category=k;
                }
            }

            auximage.at<Vec3b>(i, j) = Vec3b(colors[category].red(), colors[category].green(), colors[category].blue());
        }
    }


    cv::resize(auximage, auximage, Size(320, 240), 0.0, 0.0, INTER_NEAREST);
    int p = ui->categoryColorSlider->value();
    float p_f = p/100.;
    float d = 1 - p_f;

    destColorImage = auximage * p_f + colorImage * d;
    destGrayImage = auximage * p_f + colorImage * d;


    cvtColor(destGrayImage,destGrayImage, COLOR_BGR2GRAY);

}

void MainWindow::categoriesButton(){
    std::ifstream file("/home/fcn/fcn-colors.txt");
    for (int i = 0; i < 21; i++)
    {
        std::string line;
        std::getline(file, line);
        std::stringstream ss(line);
        std::string token;

        std::getline(ss, token, ',');
        colors[i].setRed(std::stoi(token));

        std::getline(ss, token, ',');
        colors[i].setGreen(std::stoi(token));

        std::getline(ss, token, '\n');
        colors[i].setBlue(std::stoi(token));

        if(!categoriesDialog.listWidget->item(i)->checkState()){
            colors[i].setRed(0);
            colors[i].setGreen(0);
            colors[i].setBlue(0);
        }
    }
}

void MainWindow::on_combineButton_clicked()
{
    // convertir la greyimage a BGR
    Mat colorImageBGR;

    cvtColor(colorImage, colorImageBGR, COLOR_RGB2BGR);
}
