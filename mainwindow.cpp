#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QMessageBox>

#include <iostream>
#include <string>

#include <boost/filesystem.hpp>
#include <boost/regex.hpp>


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
void PlotDirHistPlanar(int *Hist, int yScaleHeight, int barWidth)
{
    Mat ImToShow = Mat(yScaleHeight+30,60+180*(1+barWidth),CV_8UC3,Scalar(255,255,255));

    line(ImToShow,Point(49,yScaleHeight + 15),Point(49,10),Scalar(0.0,0.0,0.0,0.0));
    line(ImToShow,Point(44,yScaleHeight + 11),Point(50+180*(1+barWidth),yScaleHeight + 11),Scalar(0.0,0.0,0.0,0.0));

    for(int dir = 0; dir < 180; dir++)
    {
        int yOffset = yScaleHeight - Hist[dir]*10;
        int yPosition;
        Point start = Point(50 + dir*(1 + barWidth),yScaleHeight + 10);
        if (yOffset < 0)
        {
            yPosition = 10;
            Point stop  = Point(50 + dir*(1 + barWidth) + barWidth - 1,yPosition);
            rectangle(ImToShow,start,stop,Scalar(0.0, 0.0, 255.0, 0.0),CV_FILLED);
        }
        else
        {
            yPosition = 10 + yOffset;
            Point stop  = Point(50 + dir*(1 + barWidth) + barWidth - 1,yPosition);
            rectangle(ImToShow,start,stop,Scalar(0.0, 0.0, 0.0, 0.0),CV_FILLED);
        }




        //if(maxCount>Hist[dir])
        //    line(ImToShow,Point(dir,maxCount),Point(dir,maxCount-Hist[dir]),Scalar(0.0,0,0.0,0.0));
        //else
        //    line(ImToShow,Point(dir,maxCount),Point(dir,maxCount-1),Scalar(0.0,0,255.0,0.0));
    }
    //rectangle(ImToShow,Point(10,10),Point(100,100),Scalar(255,255,255,0),-1);
    imshow("PlanarHist",ImToShow);
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

    int *Hist = GetDirHistogramForOneImage(Params);
    PlotDirHistPlanar(Hist,200,2);
    delete[] Hist;

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
