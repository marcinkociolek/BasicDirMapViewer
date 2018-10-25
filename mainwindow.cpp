#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QMessageBox>

#include <iostream>
#include <string>

#include <boost/filesystem.hpp>
#include <boost/regex.hpp>

#include <math.h>

#include <chrono>

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

//#include "gradient.h"
#include "DispLib.h"
#include "StringFcLib.h"
#include "tileparams.h"
#include "dirdetectionparams.h"

#define PI 3.14159265

using namespace boost;
using namespace std;
using namespace boost::filesystem;
using namespace cv;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->comboBoxImageScale->addItem("Images scale x 4");
    ui->comboBoxImageScale->addItem("Images scale x 2");
    ui->comboBoxImageScale->addItem("Images scale x 1");
    ui->comboBoxImageScale->addItem("Images scale x 1/2");
    ui->comboBoxImageScale->addItem("Images scale x 1/4");
    ui->comboBoxImageScale->setCurrentIndex(2);
    imageScale = 1.0;

    showSudocolor = ui->checkBoxShowSudoColor->checkState();
    minIm = ui->doubleSpinBoxImMin->value();
    maxIm = ui->doubleSpinBoxImMax->value();

    showShape = ui->checkBoxShowSape->checkState();
    showDirection = ui->checkBoxShowDirection->checkState();

    tileLineWidth = ui->spinBoxShapeLineWidth->value();
    directionLineWidth = ui->spinBoxDirectionLineWidth->value();
    directionLineLenght = ui->doubleSpinBoxDirectionLineLenght->value();

    histogramScaleHeight = ui->spinBoxHistogramScaleHeight->value();
    histogramBarWidth = ui->spinBoxHistogramBarWidth->value();
    histogramScaleCoef = ui->spinBoxHistogramScaleCoef->value();
    DirectionalityHist = 0;

    sameFolders = ui->checkBoxSameFolders->checkState();
    autoLoadVectors = ui->checkBoxAuloLoadVectors->checkState();

    showCartesianDirHisogram = ui->checkBoxShowDirCartesianHistogram->checkState();
    showPolarDirHisogram = ui->checkBoxShowPolarHistogram->checkState();
    showConnectedDirHistograms = ui->checkBoxShowConnected->checkState();

}

MainWindow::~MainWindow()
{
    delete ui;
}
//------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------
//          My functions to move
//------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------
FileParams  GetDirectionData(path FileToOpen)
{
    FileParams LocalParams;
    //check if file exists
    if (!exists(FileToOpen))
        return LocalParams;

    std::ifstream inFile(FileToOpen.string());
    if (!inFile.is_open())
    {
        return LocalParams;
    }
    // ------------------read params from file-----------------------------

    string Line;

    //read input directory
    bool inputDirFound = 0;
    while (inFile.good())
    {
        getline(inFile, Line);
        regex LinePattern("Input Directory.+");
        if (regex_match(Line.c_str(), LinePattern))
        {
            inputDirFound = 1;
            break;
        }
    }
    if(inputDirFound)
        LocalParams.ImFolderName = Line.substr(19);
    else
        return LocalParams;

    // read tile shape
    bool tileShapeFound = 0;
    while (inFile.good())
    {
        getline(inFile, Line);
        regex LinePattern("Tile Shape:.+");
        if (regex_match(Line.c_str(), LinePattern))
        {
            tileShapeFound = 1;
            break;
        }
    }
    if(tileShapeFound)
        LocalParams.tileShape = stoi(Line.substr(12,1));
    else
        return LocalParams;

    //readTileSizeX
    bool tileSizeFound = 0;
    while (inFile.good())
    {
        getline(inFile, Line);

        regex LinePattern("Tile width x:.+");
        if (regex_match(Line.c_str(), LinePattern))
        {
            tileSizeFound = 1;
            break;
        }
    }
    if(tileSizeFound)
            LocalParams.tileSize = stoi(Line.substr(13));
    else
        return LocalParams;

    // read input file name
    bool fileNameFound = 0;
    while (inFile.good())
    {
        getline(inFile, Line);

        regex LinePattern("File Name.+");
        if (regex_match(Line.c_str(), LinePattern))
        {
            fileNameFound = 1;
            break;
        }
    }
    if(fileNameFound)
            LocalParams.ImFileName.append(Line.substr(11));
    else
        return LocalParams;

    // read size of data vector
    bool namesLineFound = 0;
    while (inFile.good())
    {
        getline(inFile, Line);

        regex LinePattern("Tile Y.+");
        if (regex_match(Line.c_str(), LinePattern))
        {
            namesLineFound = 1;
            break;
        }
    }
    if(!namesLineFound)
        return LocalParams;

    LocalParams.ValueCount = 0;
    size_t stringPos = 0;
    while(1)
    {
        stringPos = Line.find("\t",stringPos);

        if(stringPos == string::npos)
            break;
        LocalParams.ValueCount++;
        stringPos++;
    }
    // read feature names
    std::stringstream InStringStream(Line);

    LocalParams.NamesVector.empty();
    char ColumnName[256];
    while(InStringStream.good())
    {
        InStringStream.getline(ColumnName,250,'\t');
        LocalParams.NamesVector.push_back(ColumnName);
    }

    //list<int> TilesParams;
    LocalParams.ParamsVect.empty();
//read directionalities
    while(inFile.good())
    {
        TileParams params;
        getline(inFile,Line);
        params.FromString(Line);
        if(params.tileX > -1 && params.tileY > -1)
            LocalParams.ParamsVect.push_back(params);
    }

    inFile.close();
    return LocalParams;
}
//----------------------------------------------------------------------------------

void ShowShape(Mat ImShow, int x,int y, int tileShape, int tileSize, int tileLineThickness, Scalar LineColor)
{
    switch (tileShape)
    {
    case 1:
        ellipse(ImShow, Point(x, y),
        Size(tileSize / 2, tileSize / 2), 0.0, 0.0, 360.0,
        LineColor, tileLineThickness);
        break;
    case 2:

    {
        int edgeLength = tileSize / 2;
        Point vertice0(x - edgeLength / 2, y - (int)((float)edgeLength * 0.8660254));
        Point vertice1(x + edgeLength - edgeLength / 2, y - (int)((float)edgeLength * 0.8660254));
        Point vertice2(x + edgeLength, y);
        Point vertice3(x + edgeLength - edgeLength / 2, y + (int)((float)edgeLength * 0.8660254));
        Point vertice4(x - edgeLength / 2, y + (int)((float)edgeLength * 0.8660254));
        Point vertice5(x - edgeLength, y);

        line(ImShow, vertice0, vertice1, LineColor, tileLineThickness);
        line(ImShow, vertice1, vertice2, LineColor, tileLineThickness);
        line(ImShow, vertice2, vertice3, LineColor, tileLineThickness);
        line(ImShow, vertice3, vertice4, LineColor, tileLineThickness);
        line(ImShow, vertice4, vertice5, LineColor, tileLineThickness);
        line(ImShow, vertice5, vertice0, LineColor, tileLineThickness);
    }
        break;
    default:
        rectangle(ImShow, Point(x - tileSize / 2, y - tileSize / 2),
            Point(x - tileSize / 2 + tileSize, y - tileSize / 2 + tileSize / 2),
            LineColor, tileLineThickness);
        break;
    }
}
//------------------------------------------------------------------------------------------------------------------------------
int *GetDirHistogramForOneImage(FileParams Params)
{
    int *DirHist = new int[180];
    for(int dir = 0 ; dir < 180; dir++)
    {
        DirHist[dir] = 0;
        //DirHist[dir] = 20 ;
    }

    int numOfTiles = Params.ParamsVect.size();
    for(int tile = 0; tile < numOfTiles; tile++)
    {
        int dir = Params.ParamsVect[tile].Params[0];

        if( dir >= 0 && dir < 180)
        {
            DirHist[dir]++;
        }


    }

    return DirHist;
}
//------------------------------------------------------------------------------------------------------------------------------
void PlotDirHistPlanar(int *Hist, int yScale, int barWidth, int scaleCoef)
{
    const int topOffset = 30;
    const int bottomOffset = 30;
    const int scaleBarLenht = 5;
    const int leftOffset = 60;
    const int rightOffset = 20;
    const int digitWidth = 13;
    const int digitHeight = 10;

    int yScaleHeight = 100 * yScale;

    Mat ImToShow = Mat(yScaleHeight + topOffset + bottomOffset,leftOffset + rightOffset + 180*(1+barWidth),CV_8UC3,Scalar(255,255,255));

    line(ImToShow,Point(leftOffset - 2,yScaleHeight + topOffset),Point(leftOffset - 2,topOffset),Scalar(255.0,0.0,0.0,0.0));
    line(ImToShow,Point(leftOffset - 2,yScaleHeight + topOffset),Point(leftOffset+180*(1+barWidth),yScaleHeight + topOffset),Scalar(255.0,0.0,0.0,0.0));

    for(int y = 0; y <= yScaleHeight; y+= 100/2)
    {
        line(ImToShow,Point(leftOffset - scaleBarLenht,yScaleHeight + topOffset - y),Point(leftOffset-2 ,yScaleHeight + topOffset - y),Scalar(255.0,0.0,0.0,0.0));
    }
    for(int y = 0; y <= yScale; y++)
    {
        string text = to_string((int)round((double)y * pow(10.0,scaleCoef)));
        int nrOfdigits = (int)(text.size());
        putText(ImToShow,text,Point(leftOffset - scaleBarLenht -2 - nrOfdigits * digitWidth, yScaleHeight - y*100 + topOffset + digitHeight / 2), FONT_HERSHEY_COMPLEX_SMALL, 1.0, Scalar(255.0,0.0,0.0,0.0));
    }
    for(int x = 0; x <= 180; x+= 45)
    {
        line(ImToShow,Point(leftOffset + x * (1 + barWidth) + barWidth /2, yScaleHeight + topOffset),
                      Point(leftOffset + x * (1 + barWidth) + barWidth /2,yScaleHeight + topOffset + scaleBarLenht),
                      Scalar(255.0,0.0,0.0,0.0));

        string text = to_string(x);
        int nrOfdigits = (int)(text.size());
        putText(ImToShow,text,Point(leftOffset + x * (1 + barWidth)  - nrOfdigits * digitWidth / 2 ,
                                    yScaleHeight + topOffset + digitHeight * 2 + scaleBarLenht), FONT_HERSHEY_COMPLEX_SMALL, 1.0, Scalar(255.0,0.0,0.0,0.0));
    }

    //putText(ImToShow,"0",Point(30,yScaleHeight + 15), FONT_HERSHEY_COMPLEX_SMALL, 1.0, Scalar(255.0,0.0,0.0,0.0));

    Point orygin = Point(leftOffset, yScaleHeight + topOffset);
    for(int dir = 0; dir < 180; dir++)
    {
        int barLenght = (int)round((double)Hist[dir] * pow(10.0,scaleCoef * (-1))*100);

        Point start = orygin + Point(dir*(1 + barWidth), 0);
        if (barLenght > yScaleHeight)
        {
            barLenght = yScaleHeight;
            Point stop  = start + Point(barWidth - 1,0 - barLenght);
            rectangle(ImToShow,start,stop,Scalar(0.0, 0.0, 255.0, 0.0),CV_FILLED);
        }
        else
        {
            Point stop  = start + Point(barWidth - 1,0 - barLenght);
            rectangle(ImToShow,start,stop,Scalar(0.0, 0.0, 0.0, 0.0),CV_FILLED);
        }
    }
    imshow("PlanarHist",ImToShow);
}
//------------------------------------------------------------------------------------------------------------------------------
void PlotDirHistPolar(int *Hist, int yScale, int barWidth, int scaleCoef, bool showConnected)
{
    const int topOffset = 30;
    const int bottomOffset = 30;
    const int scaleBarLenht = 5;
    const int leftOffset = 60;
    const int rightOffset = 20;
    const int digitWidth = 13;
    const int digitHeight = 10;
    const double pi = 3.14159265359;

    int yScaleHeight = 100 * yScale;

    Mat ImToShow = Mat(yScaleHeight*2+1 + topOffset + bottomOffset, yScaleHeight*2+1+leftOffset + rightOffset, CV_8UC3, Scalar(255,255,255));
    circle(ImToShow,Point(leftOffset + yScaleHeight, topOffset + yScaleHeight),1,Scalar(0.0,255.0,0.0,0.0));
    for(int y = 50; y <= yScaleHeight; y+= 50)
    {
        circle(ImToShow,Point(leftOffset + yScaleHeight, topOffset + yScaleHeight),y,Scalar(0.0,255.0,0.0,0.0));
    }
    for(int y = 1; y <= yScale; y++)
    {
        string text = to_string((int)round((double)y * pow(10.0,scaleCoef)));
        int nrOfdigits = (int)(text.size());
        putText(ImToShow,text,Point(leftOffset + yScaleHeight - nrOfdigits * digitWidth / 2, topOffset + yScaleHeight - y*100),
                FONT_HERSHEY_COMPLEX_SMALL, 1.0, Scalar(255.0,0.0,0.0,0.0));
    }

    if(!showConnected)
    {
        Point start = Point(leftOffset + yScaleHeight ,
                            topOffset  + yScaleHeight);

        for(int dir = 0; dir < 360; dir++)
        {
            double barLenght = (double)Hist[dir%180] * pow(10.0,scaleCoef * (-1)) * 100.0;
            if(barLenght <= yScaleHeight)
            {
                Point stop = Point(barLenght * sin((double)dir / 180.0 * pi), barLenght * cos((double)dir / 180.0 * pi));
                line(ImToShow,start, start + stop, Scalar(0.0, 0.0, 0.0, 0.0),barWidth);
            }
            else
            {
                barLenght = yScaleHeight ;
                Point stop = Point(barLenght * sin((double)dir / 180.0 * pi), barLenght * cos((double)dir / 180.0 * pi));
                line(ImToShow,start, start + stop, Scalar(0.0, 0.0, 255.0, 0.0),barWidth);
            }
        }
    }
    else
    {
        Point start = Point(leftOffset + yScaleHeight ,
                            topOffset  + yScaleHeight);
        Point previous;
        double barLenght = (double)Hist[0] * pow(10.0,scaleCoef * (-1)) * 100.0;
        Point stop = Point(barLenght * sin(0.0 / 180.0 * pi), barLenght * cos(0.0 / 180.0 * pi));

        previous = start+stop;
        for(int dir = 1; dir <= 360; dir++)
        {
            barLenght = (double)Hist[dir%180] * pow(10.0,scaleCoef * (-1)) * 100.0;
            stop = Point(barLenght * sin((double)dir / 180.0 * pi), barLenght * cos((double)dir / 180.0 * pi));
            line(ImToShow,previous, start + stop, Scalar(0.0, 0.0, 0.0, 0.0),1);
            previous = start+stop;
        }
    }

    imshow("Polar Hist",ImToShow);

}

//------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------
//          My functions
//------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------
void MainWindow::OpenDirectionFolder()
{
    if (!exists(DirectionalityFolder))
    {
        ui->textEditOut->append(QString::fromStdString(" Directionality folder : " + DirectionalityFolder.string()+ " not exists "));
        DirectionalityFolder = "d:\\";
    }
    if (!is_directory(DirectionalityFolder))
    {
        ui->textEditOut->append(QString::fromStdString(" Directionality folder : " + DirectionalityFolder.string()+ " This is not a directory path "));
        DirectionalityFolder = "C:\\Data\\";
    }
    ui->lineEditDirectionFolder->setText(QString::fromStdString(DirectionalityFolder.string()));
    ui->listWidgetDirFiles->clear();
    for (directory_entry& FileToProcess : directory_iterator(DirectionalityFolder))
    {
        regex FilePattern(ui->lineEditRegexDirectionalityFile->text().toStdString());
        if (!regex_match(FileToProcess.path().filename().string().c_str(), FilePattern ))
            continue;

        path PathLocal = FileToProcess.path();
        if (!exists(PathLocal))
        {
            ui->textEditOut->append(QString::fromStdString(PathLocal.filename().string() + " File not exists" ));
            break;
        }
        ui->listWidgetDirFiles->addItem(PathLocal.filename().string().c_str());
    }

}
//------------------------------------------------------------------------------------------------------------------------------
void MainWindow::OpenImageFolder()
{
    if (!exists(ImageFolder))
    {
        ui->textEditOut->append(QString::fromStdString(" Image folder : " + ImageFolder.string()+ " not exists "));
        ImageFolder = "d:\\";
    }
    if (!is_directory(ImageFolder))
    {
        ui->textEditOut->append(QString::fromStdString(" Image folder : " + ImageFolder.string()+ " This is not a directory path "));
        DirectionalityFolder = "C:\\Data\\";
    }
    ui->lineEditImageFolder->setText(QString::fromStdString(ImageFolder.string()));
    ui->listWidgetImageFiles->clear();
    for (directory_entry& FileToProcess : directory_iterator(ImageFolder))
    {
        regex FilePattern(ui->lineEditRegexImageFile->text().toStdString());
        if (!regex_match(FileToProcess.path().filename().string().c_str(), FilePattern ))
            continue;
        path PathLocal = FileToProcess.path();
        if (!exists(PathLocal))
        {
            ui->textEditOut->append(QString::fromStdString(PathLocal.filename().string() + " File not exists" ));
            break;
        }
        ui->listWidgetImageFiles->addItem(PathLocal.filename().string().c_str());
    }

}
//------------------------------------------------------------------------------------------------------------------------------
void MainWindow::FreeImageVectors()
{
    while(ImVect.size() > 0)
    {
        ImVect.back().release();

        ImVect.pop_back();
    }
    while(FileParVect.size() > 0)
    {
        FileParVect.back().ParamsVect.clear();

        FileParVect.pop_back();
    }
}
//------------------------------------------------------------------------------------------------------------------------------
void MainWindow::LoadVectors()
{
    FreeImageVectors();
    if (!exists(ImageFolder))
    {
        ui->textEditOut->append(QString::fromStdString(" Image folder : " + ImageFolder.string()+ " not exists "));
        return;
    }
    if (!is_directory(ImageFolder))
    {
        ui->textEditOut->append(QString::fromStdString(" Image folder : " + ImageFolder.string()+ " This is not a directory path "));
        return;
    }
    for (int i = 0; i < ui->listWidgetImageFiles->count(); i++)
    {
        path PathLocal = ImageFolder;
        PathLocal.append(ui->listWidgetImageFiles->item(i)->text().toStdString());
        if (!exists(PathLocal))
        {
            ui->textEditOut->append(QString::fromStdString("File : " + PathLocal.filename().string() + " - not exists \n" ));
        }
        Mat ImLocal;
        ImLocal = imread(PathLocal.string(), CV_LOAD_IMAGE_ANYDEPTH);
        if(ImLocal.type() != CV_16U)
        {
            ImLocal.convertTo(ImLocal,CV_16U);
        }
        if (!ImLocal.empty())
            ImVect.push_back(ImLocal);
    }
    int vectSizeImages = ImVect.size();

    ui->textEditOut->append(QString::fromStdString("Images count: " + to_string(vectSizeImages)));
    for (int i = 0; i < ui->listWidgetImageFiles->count(); i++)
    {
        path PathLocal = DirectionalityFolder;
        PathLocal.append(ui->listWidgetImageFiles->item(i)->text().toStdString() + ".txt");
        if (!exists(PathLocal))
        {
            ui->textEditOut->append(QString::fromStdString("File : " + PathLocal.filename().string() + " - not exists \n" ));
        }
        FileParams ParamsLocal = GetDirectionData(PathLocal.string());

        if (ParamsLocal.ParamsVect.size())
            FileParVect.push_back(ParamsLocal);
    }
    int vectSizeDirections = ImVect.size();

    ui->textEditOut->append(QString::fromStdString("Direction file count: " + to_string(vectSizeDirections)));

    imageToShow = 0;

    ui->spinBoxImageNr->setValue(imageToShow);
    ui->spinBoxImageNr->setMaximum(vectSizeImages - 1);

    ShowImage();

}
//------------------------------------------------------------------------------------------------------------------------------
void MainWindow::ShowImage()
{
    if(ImVect.empty())
        return;

    Mat ImIn = ImVect[imageToShow];

    FileParams Params = FileParVect[imageToShow];

    Mat ImToShow;
    if(ImIn.empty())
        return;
    if(showSudocolor)
        ImToShow = ShowImage16PseudoColor(ImIn,minIm,maxIm);
    else
        ImToShow = ShowImage16Gray(ImIn,minIm,maxIm);

    int numOfDirections = (int)Params.ParamsVect.size();

    for(int i = 0; i < numOfDirections; i++)
    {
        int x  = Params.ParamsVect[i].tileX;
        int y  = Params.ParamsVect[i].tileY;
        double angle = Params.ParamsVect[i].Params[0];

        int lineOffsetX = (int)round(directionLineLenght * sin(angle * PI / 180.0));
        int lineOffsetY = (int)round(directionLineLenght * cos(angle * PI / 180.0));

        if (angle >= -600.0 /* && Params.ParamsVect[i].Params[3] >= meanIntensityTreshold */ )
        {
            if(showShape)
            {
                if(showSudocolor)
                    ShowShape(ImToShow, x, y, Params.tileShape, Params.tileSize, tileLineWidth,Scalar(0.0, 0.0, 0.0, 0.0));
                else
                    ShowShape(ImToShow, x, y, Params.tileShape, Params.tileSize, tileLineWidth,Scalar(0.0, 0.0, 255.0, 0.0));
            }
            if(showDirection)
            {
                if(showSudocolor)
                    line(ImToShow, Point(x - lineOffsetX, y - lineOffsetY), Point(x + lineOffsetX, y + lineOffsetY), Scalar(0.0, 0.0, 0.0, 0.0), directionLineWidth);
                else
                    line(ImToShow, Point(x - lineOffsetX, y - lineOffsetY), Point(x + lineOffsetX, y + lineOffsetY), Scalar(0.0, 0.0, 255.0, 0.0), directionLineWidth);
            }
        }
    }


    if (imageScale !=1.0)
        cv::resize(ImToShow,ImToShow,Size(),imageScale,imageScale,INTER_NEAREST);
    imshow("Image",ImToShow);
    delete[] DirectionalityHist;
    DirectionalityHist = GetDirHistogramForOneImage(Params);
    ShowHistograms();
    ui->textEditOut->append(QString::fromStdString("Params count" + to_string(Params.ParamsVect[0].paramsCount)));

}

void MainWindow::ShowHistograms()
{
    if(showCartesianDirHisogram)
        PlotDirHistPlanar(DirectionalityHist,histogramScaleHeight,histogramBarWidth,histogramScaleCoef);
    if(showPolarDirHisogram)
        PlotDirHistPolar(DirectionalityHist,histogramScaleHeight,histogramBarWidth,histogramScaleCoef,showConnectedDirHistograms);

}
//------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------
//          Slots
//------------------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------------------
void MainWindow::on_pushButtonOpenDirFolder_clicked()
{
    QFileDialog dialog(this, "Open Folder");
    dialog.setFileMode(QFileDialog::Directory);
    dialog.setDirectory(QString::fromStdString(DirectionalityFolder.string()));

    //QStringList FileList= dialog.e
    if(dialog.exec())
    {
        DirectionalityFolder = dialog.directory().path().toStdWString();
    }
    else
        return;

    OpenDirectionFolder();
    if(sameFolders)
    {
        ImageFolder = DirectionalityFolder;
        OpenImageFolder();
        if(autoLoadVectors)
            LoadVectors();
    }
}

void MainWindow::on_pushButtonOpenImageFolder_clicked()
{
    QFileDialog dialog(this, "Open Folder");
    dialog.setFileMode(QFileDialog::Directory);
    dialog.setDirectory(QString::fromStdString(ImageFolder.string()));

    //QStringList FileList= dialog.e
    if(dialog.exec())
    {
        ImageFolder = dialog.directory().path().toStdWString();
    }
    else
        return;

    OpenImageFolder();
}

void MainWindow::on_lineEditRegexDirectionalityFile_returnPressed()
{
    OpenDirectionFolder();
}


void MainWindow::on_lineEditRegexImageFile_returnPressed()
{
    OpenImageFolder();
}

void MainWindow::on_pushButtonLoadVectors_clicked()
{
    LoadVectors();
    ShowImage();

}

void MainWindow::on_spinBoxImageNr_valueChanged(int arg1)
{
    imageToShow = arg1;
    ShowImage();
}

void MainWindow::on_comboBoxImageScale_currentIndexChanged(int index)
{
    switch(index)
    {
    case 0:
        imageScale = 4.0;
        break;
    case 1:
        imageScale = 2.0;
        break;
    case 2:
        imageScale = 1.0;
        break;

    case 3:
        imageScale = 1.0/2.0;
        break;
    case 4:
        imageScale = 1.0/4.0;
        break;
    default:
        imageScale = 1.0;
        break;
    }
    ShowImage();
}

void MainWindow::on_checkBoxShowSudoColor_toggled(bool checked)
{
    showSudocolor = checked;
    ShowImage();
}

void MainWindow::on_doubleSpinBoxImMin_valueChanged(double arg1)
{
    minIm = arg1;
    ShowImage();
}

void MainWindow::on_doubleSpinBoxImMax_valueChanged(double arg1)
{
    maxIm = arg1;
    ShowImage();
}

void MainWindow::on_spinBoxDirectionLineWidth_valueChanged(int arg1)
{
    directionLineWidth = arg1;
    ShowImage();
}

void MainWindow::on_doubleSpinBoxDirectionLineLenght_valueChanged(double arg1)
{
    directionLineLenght = arg1;
    ShowImage();
}

void MainWindow::on_checkBoxShowSape_toggled(bool checked)
{
    showShape = checked;
    ShowImage();
}

void MainWindow::on_checkBoxShowDirection_toggled(bool checked)
{
    showDirection = checked;
    ShowImage();
}

void MainWindow::on_checkBoxSameFolders_toggled(bool checked)
{
    sameFolders = checked;
}

void MainWindow::on_checkBoxAuloLoadVectors_toggled(bool checked)
{
    autoLoadVectors = checked;
}

void MainWindow::on_spinBoxHistogramBarWidth_valueChanged(int arg1)
{
    histogramBarWidth = arg1;
    ShowHistograms();
}


void MainWindow::on_spinBoxHistogramScaleHeight_valueChanged(int arg1)
{
    histogramScaleHeight = arg1;
    ShowHistograms();

}

void MainWindow::on_spinBoxHistogramScaleCoef_valueChanged(int arg1)
{
    histogramScaleCoef = arg1;
    ShowHistograms();

}

void MainWindow::on_checkBoxShowDirCartesianHistogram_toggled(bool checked)
{
    showCartesianDirHisogram = checked;
    ShowHistograms();
}

void MainWindow::on_checkBoxShowPolarHistogram_toggled(bool checked)
{
    showPolarDirHisogram = checked;
    ShowHistograms();
}

void MainWindow::on_checkBoxShowConnected_toggled(bool checked)
{
    showConnectedDirHistograms = checked;
    ShowHistograms();
}

void MainWindow::on_spinBoxFeatureNr_valueChanged(int arg1)
{
    featureNr = arg1;
}
