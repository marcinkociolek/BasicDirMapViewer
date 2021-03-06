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
#include "NormalizationLib.h"
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

    ui->comboBoxDisplayRange->addItem("user selected");
    ui->comboBoxDisplayRange->addItem("Image Min Max");
    ui->comboBoxDisplayRange->addItem("Image Mean +/- STD");
    ui->comboBoxDisplayRange->addItem("Image 1%-99%");
    ui->comboBoxDisplayRange->setCurrentIndex(2);

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
    DirectionalityHist = nullptr;

    sameFolders = ui->checkBoxSameFolders->checkState();
    autoLoadVectors = ui->checkBoxAuloLoadVectors->checkState();

    showCartesianDirHisogram = ui->checkBoxShowDirCartesianHistogram->checkState();
    showPolarDirHisogram = ui->checkBoxShowPolarHistogram->checkState();
    showConnectedDirHistograms = ui->checkBoxShowConnected->checkState();

    featureNr = ui->spinBoxFeatureNr->value();

    featureHistogramScaleHeight = ui->spinBoxFeatureHistogramScaleHeight->value();
    featureHistogramBarWidth = ui->spinBoxFeatureHistogramBarWidth->value();
    featureHistogramScaleCoef = ui->spinBoxFeatureHistogramScaleCoef->value();

    showFeataureHisogram = ui->checkBoxShowFeatureHistogram->checkState();

}

MainWindow::~MainWindow()
{


    FreeImageVectors();
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
    if(ImShow.empty())
        return;
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
    if(Hist == nullptr)
        return;
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
    if(Hist == nullptr)
        return;
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
double FindResultingDirection(int *DirectionHistogram)
{
    double longestResultingVector = 0;
    double resultingDirection = -1000;
    for(int startDir = 0; startDir<180; startDir++)
    {
        double a = 0.0;
        double b = 0.0;
        int stopDir = startDir + 180;

        for(int dir = startDir; dir < stopDir; dir++)
        {
            int normDir;
            if(dir < 180)
                normDir = dir;
            else
                normDir = dir - 180;
            double length = (double)DirectionHistogram[normDir];
            a += length * cos((double)dir/180.0*PI);
            b += length * sin((double)dir/180.0*PI);
        }
        double localLength = sqrt(a*a+b*b);
        if(longestResultingVector < localLength)
        {
            longestResultingVector = localLength;
            if(a == 0.0 && b == 0.0)
                resultingDirection = -1000.0;
            else
                resultingDirection = atan2(b,a)/PI*180.0;

        }
    }
    if(resultingDirection < 0.0)
        resultingDirection += 360.0;
    if (resultingDirection >= 180.0)
        resultingDirection -=180.0;
    if(resultingDirection < 0.0 || resultingDirection > 180.0)
        return -1000.0;

    return resultingDirection;
}
//----------------------------------------------------------------------------------
double FindSpread(int *DirectionHistogram, double resultingDir)
{
    if (resultingDir < -999.0)
        return -1000.0;

    int startDir =  (int)round(resultingDir) - 90;
    int stopDir = startDir + 180;
    double sum = 0;
    double count = 0;
    for(int dir = startDir; dir < stopDir; dir++)
    {
        int normDir;
        normDir = dir;
        if(normDir >= 180)
            normDir = dir - 180;
        if(normDir < 0)
            normDir = dir + 180;
        sum += (resultingDir - dir) * (resultingDir - dir) * DirectionHistogram[normDir];
        count += DirectionHistogram[normDir];
    }
    if(count != 0.0)
        return sqrt(sum/count);
    else
        return -2000.0;
}
//----------------------------------------------------------------------------------
double FindMeanOfFeature(FileParams Params,int featureNr)
{
    double mean = 0.0;
    double counter = 0.0;
    int numOfTiles = Params.ParamsVect.size();
    if( Params.ParamsVect[0].Params.size() <= featureNr)
        return -11111.0;

    for(int tile = 0; tile < numOfTiles; tile++)
    {
        if( Params.ParamsVect[tile].Params.size() > featureNr)
        {
            mean += Params.ParamsVect[tile].Params[featureNr];
            counter += 1.0;
        }
    }

    return mean/counter;
}
//----------------------------------------------------------------------------------
double FindStdOfFeature(FileParams Params,double mean, int featureNr)
{
    double sum = 0.0;
    double counter = 0.0;
    int numOfTiles = Params.ParamsVect.size();
    if( Params.ParamsVect[0].Params.size() <= featureNr)
        return -11111.0;

    for(int tile = 0; tile < numOfTiles; tile++)
    {
        if( Params.ParamsVect[tile].Params.size() > featureNr)
        {
            double diff = mean - Params.ParamsVect[tile].Params[featureNr];
            sum += (diff * diff);
            counter += 1.0;
        }
    }

    return sqrt(sum)/counter;
}
//----------------------------------------------------------------------------------
double FindMaxOfFeature(FileParams Params, int featureNr)
{

    int numOfTiles = Params.ParamsVect.size();
    if( Params.ParamsVect[0].Params.size() <= featureNr)
        return -11111.0;

    double max = Params.ParamsVect[0].Params[featureNr];
    for(int tile = 1; tile < numOfTiles; tile++)
    {
        if( Params.ParamsVect[tile].Params.size() > featureNr)
        {
            double val = Params.ParamsVect[tile].Params[featureNr];
            if(max < val)
                max = val;
        }
    }

    return max;
}
//----------------------------------------------------------------------------------
double FindMinOfFeature(FileParams Params, int featureNr)
{

    int numOfTiles = Params.ParamsVect.size();
    if( Params.ParamsVect[0].Params.size() <= featureNr)
        return -11111.0;

    double min = Params.ParamsVect[0].Params[featureNr];
    for(int tile = 1; tile < numOfTiles; tile++)
    {
        if( Params.ParamsVect[tile].Params.size() > featureNr)
        {
            double val = Params.ParamsVect[tile].Params[featureNr];
            if(min > val)
                min = val;
        }
    }

    return min;
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
    //ui->textEditOut->clear();
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
            continue;
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
            continue;
        }
        FileParams ParamsLocal = GetDirectionData(PathLocal.string());

        if (ParamsLocal.ParamsVect.size())
            FileParVect.push_back(ParamsLocal);
    }
    int vectSizeDirections = ImVect.size();

    ui->textEditOut->append(QString::fromStdString("Direction file count: " + to_string(vectSizeDirections)));

    if(ImVect.empty())
        return;

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

    GetDisplayParams();

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
    ui->spinBoxFeatureNr->setMaximum(Params.ParamsVect[0].paramsCount - 1);
    featureNr = ui->spinBoxFeatureNr->value();

    //ui->textEditOut->append(QString::fromStdString("Params count" + to_string(Params.ParamsVect[0].paramsCount)));

    double resultingDirection = FindResultingDirection(DirectionalityHist);
    double directionSpread  = FindSpread(DirectionalityHist, resultingDirection);

    ui->textEditOut->append(QString::fromStdString("resulting direction: " + to_string(resultingDirection) +
                                                   " direction spread: " + to_string(directionSpread)));

}

void MainWindow::ShowHistograms()
{
    if(showCartesianDirHisogram)
        PlotDirHistPlanar(DirectionalityHist,histogramScaleHeight,histogramBarWidth,histogramScaleCoef);
    if(showPolarDirHisogram)
        PlotDirHistPolar(DirectionalityHist,histogramScaleHeight,histogramBarWidth,histogramScaleCoef,showConnectedDirHistograms);

    if(showFeataureHisogram)
    {
        FeatHistogram.GetHitogram(FileParVect[imageToShow], featureNr);
        FeatHistogram.PlotDirHistPlanar(featureHistogramScaleHeight,featureHistogramScaleCoef,featureHistogramBarWidth);
    }
}
//------------------------------------------------------------------------------------------------------------------------------
void MainWindow::GetDisplayParams()
{
    //float maxImVal, minImVal;
    Mat ImIn = ImVect[imageToShow];

    switch (ui->comboBoxDisplayRange->currentIndex())
    {
    case 1:
        NormParamsMinMax(ImIn, &maxIm, &minIm);
        //ui->doubleSpinBoxImMax->setValue(maxImVal);
        //ui->doubleSpinBoxImMin->setValue(minImVal);
        break;
    case 2:
        NormParamsMeanP3Std(ImIn, &maxIm, &minIm);
        //ui->doubleSpinBoxImMax->setValue(maxImVal);
        //ui->doubleSpinBoxImMin->setValue(minImVal);
        break;
    case 3:
        NormParams1to99perc(ImIn, &maxIm, &minIm);
        //ui->doubleSpinBoxImMax->setValue(maxImVal);
        //ui->doubleSpinBoxImMin->setValue(minImVal);
        break;


    default:
        break;
    }
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
    ShowHistograms();
}

void MainWindow::on_checkBoxShowFeatureHistogram_toggled(bool checked)
{
    showFeataureHisogram = checked;
    ShowHistograms();
}

void MainWindow::on_spinBoxFeatureHistogramBarWidth_valueChanged(int arg1)
{
    featureHistogramBarWidth = arg1;
    ShowHistograms();
}

void MainWindow::on_spinBoxFeatureHistogramScaleHeight_valueChanged(int arg1)
{
    featureHistogramScaleHeight = arg1;
    ShowHistograms();
}

void MainWindow::on_spinBoxFeatureHistogramScaleCoef_valueChanged(int arg1)
{
    featureHistogramScaleCoef = arg1;
    ShowHistograms();
}

void MainWindow::on_listWidgetDirFiles_currentRowChanged(int currentRow)
{
    ui->spinBoxImageNr->setValue(currentRow);
    //ShowImage();
    //ShowHistograms();
}

void MainWindow::on_listWidgetImageFiles_currentRowChanged(int currentRow)
{
    ui->spinBoxImageNr->setValue(currentRow);
    //ShowImage();
    //ShowHistograms();
}

void MainWindow::on_pushButtonCalculateStatistics_clicked()
{
    if(FileParVect.empty())
        return;

    int nrOfFiles = FileParVect.size();

    string OutText = "";
    OutText += "FileName\t";
    OutText += "Resulting Direction\t";
    OutText += "Direction Spread\t";
    OutText += "\t";

    int nrOfFeatures = FileParVect[0].ValueCount;

    for(int i = 5; i < nrOfFeatures; i++)
    {
        OutText += "mean " + FileParVect[0].NamesVector[i] ;
        OutText += "\t";
        OutText += "std " + FileParVect[0].NamesVector[i] ;
        OutText += "\t";
        OutText += "min " + FileParVect[0].NamesVector[i] ;
        OutText += "\t";
        OutText += "max " + FileParVect[0].NamesVector[i] ;
        OutText += "\t\t";
    }

    OutText += "\n";
    for(int i = 0; i < nrOfFiles; i++)
    {
        FileParams Params = FileParVect[i];

        OutText += Params.ImFileName.stem().string();
        OutText += "\t";
        delete[] DirectionalityHist;
        DirectionalityHist = GetDirHistogramForOneImage(Params);


        double resultingDirection = FindResultingDirection(DirectionalityHist);
        double directionSpread  = FindSpread(DirectionalityHist, resultingDirection);

        OutText += to_string(resultingDirection) + "\t";
        OutText += to_string(directionSpread) + "\t";
        OutText += "\t";

        for(int k = 3; k < nrOfFeatures - 3; k++)
        {
            double meanfeatureVal = FindMeanOfFeature(Params,k);
            OutText += to_string(meanfeatureVal) + "\t";
            OutText += to_string(FindStdOfFeature(Params, meanfeatureVal, k)) + "\t";
            OutText += to_string(FindMinOfFeature(Params, k)) + "\t";
            OutText += to_string(FindMaxOfFeature(Params, k)) + "\t";
            OutText += "\t";
        }
        OutText += "\n";
    }

    path textOutFile = DirectionalityFolder;

    textOutFile.append("Summary.out");

    std::ofstream out (textOutFile.string());
    out << OutText;
    out.close();

}


void MainWindow::on_pushButtonSF_clicked()
{
    if(FileParVect.empty())
        return;

    size_t size = FileParVect.size();
    for(size_t i = 0; i< size; i++)
    {
        string LocalFileName = FileParVect[i].ImFileName.stem().string();
        string outStr = regex_replace(LocalFileName, regex(".+n"), "");
        int dir = stoi(outStr);
        ui->textEditOut->append(QString::fromStdString(LocalFileName + " ->" + outStr + " ->" + to_string(dir)));
    }
}
