#include "ModelForm.h"
#include "ui_ModelForm.h"
#include <QProcess>
#include <QFileDialog>
#include <QDebug>
#include <QDir>
#include <QTemporaryFile>
/*
    QString mType;       //! type regression
    QString mWeight;     //! weight
    bool    mFFN;        //! Use FFN for EMGs values
    int     mRtoc;       //! k Near to classify
*/
static ModelForm::kNNParams defaultkNNParams =
{
    "EUCLIDE_DISTANCE",   //type
    "ONE_ON_DISTANCE",    //weight
    true,                 //FFT
    5,                    //records_to_classify
};

/*
    QString mType;
    QString mKernel;
    double  mCacheSize;
    double  mCoef0;
    int     mDegree;
    double  mEps;
    double  mGamma;
    double  mNu;
    double  mP;
    bool    mProbability;
    bool    mShrinking;
*/
#if 0
static ModelForm::SVMParams defaultSVMParams =
{
    "NU_SVC",   //type
    "RBF",      //kernel
    100.0f,     //cache
    0.1f,       //coef0
    3,          //degree
    0.1f,       //eps
    0.2f,       //gamma
    0.3f,       //nu
    0.0f,       //p
    1.0f,       //const
    true,       //FFN
    true,       //probability
    true        //shrinking
};
#else
static ModelForm::SVMParams defaultSVMParams =
{
    "C_SVC",    //type
    "LINEAR",   //kernel
    2000.0f,    //cache
    0.01f,      //coef0
    3,          //degree
    0.01f,      //eps
    0.1f,       //gamma
    0.1f,       //nu
    0.1f,       //p
    2.0f,       //const
    true,       //FFN
    true,       //probability
    true        //shrinking
};
#endif

/*
    int     mNumIterations;
    double  mLearningRate;
    bool    mCalcAccuracyMse;
    bool    mPrint;
*/
static ModelForm::RBFNetworkParams  defaultRBFNetworkParams =
{
    50000,      //iterations
    0.5,        //learning rate
    true,       //calc accuracy mse
    false       //print debug info
};

ModelForm::ModelForm(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ModelForm)
{
    ui->setupUi(this);
    //default is kNN
    apply(ModelType::M_kNN);
    //apply default kNN
    apply(defaultkNNParams);
    //apply default svm
    apply(defaultSVMParams);
    //apply default rbf network params
    apply(defaultRBFNetworkParams);
}

ModelForm::~ModelForm()
{
    delete ui;
}
//set directory
void ModelForm::onSearchDir(bool)
{
    //model type
    QString  strtype   = "GesturesClassifierApplication";
    //select
    switch(getType())
    {
        case ModelType::M_kNN:         strtype = "kNN (*.knn)"; break;
        case ModelType::M_SVM:         strtype = "SVM (*.svm)"; break;
        case ModelType::M_RBF_Network: strtype = "RBF_Network (*.net)"; break;
        default: return; break;
    };
    //path
    QString path =
    mModelPath.size() ? QDir(mModelPath).absolutePath() :
                        QDir(mDatasetPath).absolutePath();
    //remove extension
    path = path.section(".",0,0);
    //path
    mModelPath=QFileDialog::getSaveFileName(this,
                                            "Save",
                                            path,
                                            strtype);
    ui->mLEDir->setText(mModelPath);

}
//apply event
void ModelForm::onApply(bool event)
{
    //if have a valid paths
    if(!mModelPath.size()) return;
    //this app dir
    QString  thizdir = QCoreApplication::applicationDirPath();
#if __APPLE__
    QString  dir = thizdir+"/../Resources";
#else
    const QString&  dir = thizdir;
#endif
    QString  name   = "GesturesClassifierApplication";
    QString  apppath= dir + "/" + name;
    //debug
    qDebug() << apppath;
    //params
    QStringList  params;
    //set ouput
    params << "-i" << mDatasetPath;
    params << "-o"<< mModelPath;
    //type
    ModelType type = getType();
    //type to args
    params << toStrListParams(type);
    //cases
    switch(type)
    {
        case ModelType::M_kNN:         params << toStrListParams(getkNNParams()); break;
        case ModelType::M_SVM:         params << toStrListParams(getSVMParams()); break;
        case ModelType::M_RBF_Network: params << toStrListParams(getRBFParams()); break;
        default: break;
    };
    //process
    QProcess gcapp;
    //execute
    gcapp.start(apppath,params);
    gcapp.waitForFinished();
    //print ouput
    ui->mTBConsole->setText(
                + "Console command:\n"
                + name
                + " "
                + params.join(' ')
                + "\n\n"
                + "Output:\n"
                + gcapp.readAllStandardOutput()
    );
}


void ModelForm::onTestModel(bool event)
{
    //test file
    if(!QFileInfo(mModelPath).exists()) return;
    //this app dir
    QString  thizdir = QCoreApplication::applicationDirPath();
    QString  name   = "GesturesClassifierExemple";
#if __APPLE__
    QString  dir = thizdir+"/../Resources";
    QString  apppath= dir + "/" + name;

    QString base_pathsh = QDir::tempPath()+"/do_test_model.sh";
    QTemporaryFile dosh(base_pathsh);
    dosh.setAutoRemove(false);
    dosh.open();
    //real path
    {
        QTextStream stream(&dosh);
        stream << QString("#!/bin/bash\n");
        stream << QString("cd "+QDir(dir).canonicalPath()+"\n");
        stream << QString("./"+name+" "+mModelPath);
    }
    dosh.close();
    //executable
    QFile(dosh.fileName()).setPermissions(QFile::ReadUser|QFile::ExeUser);
    //args
    QStringList args;
    args << "-b";
    args << "com.apple.terminal";
    args << QFileInfo(dosh.fileName()).canonicalFilePath();
    QProcess::startDetached("open",args);
#else
    const QString&  dir = thizdir;
    QString apppath= dir + "/" + name;
    //execute
    QProcess::startDetached(apppath,QStringList(mModelPath));
#endif
}

void ModelForm::onKNN(bool event)
{
    if(event)
    {
        ui->mGBKnn->setEnabled(true);
        ui->mGBSVM->setEnabled(false);
        ui->mGBRBFNetwork->setEnabled(false);
    }
}

void ModelForm::onSVM(bool event)
{
    if(event)
    {
        ui->mGBKnn->setEnabled(false);
        ui->mGBSVM->setEnabled(true);
        ui->mGBRBFNetwork->setEnabled(false);
    }
}

void ModelForm::onRBFNetwork(bool event)
{
    if(event)
    {
        ui->mGBKnn->setEnabled(false);
        ui->mGBSVM->setEnabled(false);
        ui->mGBRBFNetwork->setEnabled(true);
    }
}

//execute
bool ModelForm::execute(const QString& path)
{
    //dataset path
    mDatasetPath = path;
    //execute
    return exec() == 0;
}

//apply to form
void ModelForm::apply(ModelForm::ModelType type)
{
    switch (type)
    {
        default:
        case ModelType::M_kNN:
            ui->mRBkNN->setChecked(true);
        break;
        case ModelType::M_SVM:
            ui->mRBSVM->setChecked(true);
        break;
        case ModelType::M_RBF_Network:
            ui->mRBRBFNetwork->setChecked(true);
        break;
    }
}

void ModelForm::apply(const ModelForm::kNNParams& params)
{
    ui->mCBkNNType->setCurrentText(params.mType);
    ui->mCBkNNWeight->setCurrentText(params.mWeight);
    ui->mCBkNNFFT->setCurrentText(params.mFFN ? "TRUE" : "FALSE");
    ui->mSBkNNRtoc->setValue(params.mRtoc);
}

void ModelForm::apply(const ModelForm::SVMParams& params)
{
    ui->mCBType->setCurrentText(params.mType);
    ui->mCBKernel->setCurrentText(params.mKernel);
    ui->mDSBCacheSize->setValue(params.mCacheSize);
    ui->mDSBCoef0->setValue(params.mCoef0);
    ui->mSBDegree->setValue(params.mDegree);
    ui->mDSBEps->setValue(params.mEps);
    ui->mDSBGamma->setValue(params.mGamma);
    ui->mDSBNu->setValue(params.mNu);
    ui->mDSBP->setValue(params.mP);
    ui->mDSBConst->setValue(params.mConst);
    ui->mCBFTT->setCurrentText(params.mFFN ? "TRUE" : "FALSE");
    ui->mCBProbability->setCurrentText(params.mProbability ? "TRUE" : "FALSE");
    ui->mCBShrinking->setCurrentText(params.mShrinking ? "TRUE" : "FALSE");
}

void ModelForm::apply(const ModelForm::RBFNetworkParams& params)
{
    ui->mSBNumIterations->setValue(params.mNumIterations);
    ui->mDSBLearningRate->setValue(params.mLearningRate);
    ui->mCBAccuracyMse->setCurrentText(params.mCalcAccuracyMse ? "TRUE" : "FALSE");
    ui->mCBPrint->setCurrentText(params.mPrint ? "TRUE" : "FALSE");
}

//get from form
ModelForm::ModelType ModelForm::getType() const
{
    if(ui->mRBkNN->isChecked())        return ModelType::M_kNN;
    if(ui->mRBSVM->isChecked())        return ModelType::M_SVM;
    if(ui->mRBRBFNetwork->isChecked()) return ModelType::M_RBF_Network;
    //default
    return ModelType::M_kNN;
}

ModelForm::kNNParams ModelForm::getkNNParams() const
{
    ModelForm::kNNParams output;

    output.mType = ui->mCBkNNType->currentText();
    output.mWeight = ui->mCBkNNWeight->currentText();
    output.mRtoc = ui->mSBkNNRtoc->value();
    output.mFFN = (ui->mCBkNNFFT->currentText() == "FALSE" ? false : true);

    return output;
}

ModelForm::SVMParams ModelForm::getSVMParams() const
{
    ModelForm::SVMParams output;
    output.mType = ui->mCBType->currentText();
    output.mKernel = ui->mCBKernel->currentText();
    output.mCacheSize = ui->mDSBCacheSize->value();
    output.mCoef0 = ui->mDSBCoef0->value();
    output.mDegree = ui->mSBDegree->value();
    output.mEps = ui->mDSBEps->value();
    output.mGamma = ui->mDSBGamma->value();
    output.mNu = ui->mDSBNu->value();
    output.mP = ui->mDSBP->value();
    output.mConst = ui->mDSBConst->value();
    output.mFFN = (ui->mCBFTT->currentText() == "FALSE" ? false : true);
    output.mProbability = (ui->mCBProbability->currentText() == "FALSE" ? false : true);
    output.mShrinking = (ui->mCBShrinking->currentText() == "FALSE" ? false : true);

    return output;
}

ModelForm::RBFNetworkParams ModelForm::getRBFParams() const
{
    ModelForm::RBFNetworkParams output;

    output.mNumIterations = ui->mSBNumIterations->value();
    output.mLearningRate = ui->mDSBLearningRate->value();
    output.mCalcAccuracyMse = (output.mCalcAccuracyMse = ui->mCBAccuracyMse->currentText() == "FALSE" ? false : true);
    output.mPrint = (output.mPrint = ui->mCBPrint->currentText() == "FALSE" ? false : true);

    return output;
}

//create cmd params
QStringList ModelForm::toStrListParams(ModelForm::ModelType& type)
{
    QStringList list;

    switch(type)
    {
        default:
        case ModelType::M_kNN:         list << "-c" << "kNN";        break;
        case ModelType::M_SVM:         list << "-c" << "SVM";        break;
        case ModelType::M_RBF_Network: list << "-c" << "RBFNetwork"; break;
    };

    return list;
}

QStringList ModelForm::toStrListParams(const ModelForm::kNNParams& params)
{
    QStringList list;
    list << "type" << params.mType;
    list << "weight" << params.mWeight;
    list << "rtoc" << QString::number(params.mRtoc);
    list << "fft" << QString(params.mFFN ? "true" : "false");
    return list;
}

QStringList ModelForm::toStrListParams(const ModelForm::SVMParams& params)
{
    QStringList list;
    list << "type" << params.mType;
    list << "kernel" << params.mKernel;
    list << "cache" << QString::number(params.mCacheSize);
    list << "coef0" << QString::number(params.mCoef0);
    list << "degree" << QString::number(params.mDegree);
    list << "eps" << QString::number(params.mEps);
    list << "gamma" << QString::number(params.mGamma);
    list << "nu" << QString::number(params.mNu);
    list << "p" << QString::number(params.mP);
    list << "const" << QString::number(params.mConst);
    list << "fft" << QString(params.mFFN ? "true" : "false");
    list << "probability" << QString(params.mProbability ? "true" : "false");
    list << "shrinking" << QString(params.mShrinking ? "true" : "false");

    return list;
}

QStringList ModelForm::toStrListParams(const ModelForm::RBFNetworkParams& params)
{
    QStringList list;

    list << "learning rate" << QString::number(params.mLearningRate);
    list << "iterations" << QString::number(params.mNumIterations);
    list << "accuracy mse" << QString(params.mCalcAccuracyMse ? "true" : "false");
    list << "print" << QString(params.mPrint ? "true" : "false");

    return list;
}
