#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include <boost/filesystem.hpp>

#include <opencv2/core/core.hpp>

#include "tileparams.h"
#include "dirdetectionparams.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    //boost::filesystem::path FileToOpen;
    boost::filesystem::path DirectionalityFolder;
    boost::filesystem::path ImageFolder;
    boost::filesystem::path OutFolder;
    //boost::filesystem::path ImFileName;

    bool sameFolders;
    bool autoLoadVectors;

    std::vector<cv::Mat> ImVect;
    std::vector<FileParams> FileParVect;

    int *DirectionalityHist;

    int histogramScaleHeight;
    int histogramBarWidth;
    int histogramScaleCoef;



    int imageToShow;
    double imageScale;

    bool showSudocolor;
    bool showShape;
    bool showDirection;

    int tileLineWidth;
    int directionLineWidth;
    double directionLineLenght;


    double minIm;
    double maxIm;

    void OpenDirectionFolder();
    void OpenImageFolder();
    void FreeImageVectors();
    void LoadVectors();
    void ShowImage();
    //FileParams  MainWindow::GetDirectionData(path FileToOpen);

private slots:
    void on_pushButtonOpenDirFolder_clicked();

    void on_pushButtonOpenImageFolder_clicked();

    void on_lineEditRegexDirectionalityFile_returnPressed();

    void on_lineEditRegexImageFile_returnPressed();

    void on_pushButtonLoadVectors_clicked();

    void on_spinBoxImageNr_valueChanged(int arg1);

    void on_comboBoxImageScale_currentIndexChanged(int index);

    void on_checkBoxShowSudoColor_toggled(bool checked);

    void on_doubleSpinBoxImMin_valueChanged(double arg1);

    void on_doubleSpinBoxImMax_valueChanged(double arg1);

    void on_spinBoxDirectionLineWidth_valueChanged(int arg1);

    void on_doubleSpinBoxDirectionLineLenght_valueChanged(double arg1);

    void on_checkBoxShowSape_toggled(bool checked);

    void on_checkBoxShowDirection_toggled(bool checked);

    void on_checkBoxSameFolders_toggled(bool checked);

    void on_checkBoxAuloLoadVectors_toggled(bool checked);

    void on_spinBoxHistogramBarWidth_valueChanged(int arg1);

    void on_spinBoxHistogramScaleHeight_valueChanged(int arg1);



    void on_spinBoxHistogramScaleCoef_valueChanged(int arg1);



private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H

