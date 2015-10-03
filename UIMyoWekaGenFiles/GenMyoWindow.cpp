#include "GenMyoWindow.h"
#include "GestureForm.h"
#include "RecordingDialog.h"
#include "FlagsDialog.h"
#include "MyoDataOuput.h"
#include "MyoDataInput.h"
#include "GesturesBuilder.h"
#include "ui_GenMyoWindow.h"
#include <QDebug>
#include <QFileDialog>
#include <QDialogButtonBox>

GenMyoWindow::GenMyoWindow(QWidget *parent)
  : QMainWindow(parent)
  , ui(new Ui::GenMyoWindow)
  , mMyoDialog(this)
{
    ui->setupUi(this);
    //start path
    mPath = QDir::currentPath();
    //start path
    mPathExport = QDir::currentPath();
    //start
    mMyoManager.start();
    //set manager to myo dialog
    mMyoDialog.setMyoManager(&mMyoManager);
}

GenMyoWindow::~GenMyoWindow()
{
    //close
    mMyoManager.close();
    //delete
    delete ui;
}


void GenMyoWindow::onAddGesture()
{
    //alloc item
    auto item=new QListWidgetItem();
    auto widget=new GestureForm(this);
    //init size
    item->setSizeHint(widget->sizeHint());
    //add item
    ui->mLWGestures->addItem(item);
    ui->mLWGestures->setItemWidget(item,widget);
    //on delete
    widget->setOnDelete(
    [this,item]()
    {
        mWekaItems.remove(item);
        delete item;
    });
    //on end recording
    auto onEndRecording=
    [this,item]()
    {
        mWekaItems[item] = mMyoManager.endRecording();
    };
    //on recording
    widget->setOnRecording(
    [this,widget,onEndRecording]()
    {
        //start
        mMyoManager.startRecording();
        RecordingDialog dialog(this);
        dialog.execute(onEndRecording);
    });
}

void GenMyoWindow::onNew()
{
    //clear data
    mWekaItems.clear();
    ui->mLWGestures->clear();
}

void GenMyoWindow::onOpen()
{
    //path
    QString sPath=QFileDialog::getOpenFileName(this,
                                               "Open",
                                               mPath,
                                               "Myo Files *.myodata");
    if( sPath.length() )
    {
        //save path
        mPath=sPath;
        //clear data
        mWekaItems.clear();
        ui->mLWGestures->clear();
        //load
        MyoListener::TypeInput myoLoader;
        myoLoader.read(
        //file map
        mPath.toStdString(),
        //read
        [this](int idClass,
               const std::string& className,
               const myo::RawDatas< int8_t, float, float, float, 8 >& row)
        {
            //add class
            if(ui->mLWGestures->count() <= idClass)
            {
                onAddGesture();
                auto item=ui->mLWGestures->item(idClass);
                GestureForm* widget=dynamic_cast<GestureForm*>(ui->mLWGestures->itemWidget(item));
                widget->setName(QString::fromStdString(className));
            }
            //put row
            auto item=ui->mLWGestures->item(idClass);
            mWekaItems[item].append(row);
        });
    }
}

void GenMyoWindow::onSave()
{
    if(!mWekaItems.empty())
    {
        //save
        if( mPath.length() )
            save();
        else
            onSaveAs();
    }
}

void GenMyoWindow::onSaveAs()
{
    if(!mWekaItems.empty())
    {
        //path
        QString sPath=QFileDialog::getSaveFileName(this,
                                                  "Save",
                                                   mPath,
                                                   "Myo Files *.myodata");

        if( sPath.length() )
        {
            mPath=sPath;
            save();
        }
    }
}

void GenMyoWindow::onExport()
{
    if(!mWekaItems.empty())
    {
        //get flags
        FlagsDialog flagsDialog(this);
        if(!flagsDialog.exec(mFlags)) return;
        //save
        if( mPathExport.length() )
        {
            if( mPathExport.endsWith(".arff") ) saveWEKA();
            else if( mPathExport.endsWith(".trening") ) saveFANN();
        }
        else
            onExportAs();
    }
}

void GenMyoWindow::onExportAs()
{
    if(!mWekaItems.empty())
    {
        //path
        QString sPath=QFileDialog::getSaveFileName(this,
                                                  "Export",
                                                   mPath,
                                                   "Weka Files *.arff;;"
                                                   "FANN Files *.fann");

        if( sPath.length() &&
           (sPath.endsWith(".arff") || sPath.endsWith(".fann")) )
        {
            //save path
            mPath=sPath;
            //get flags
            FlagsDialog flagsDialog(this);
            if(!flagsDialog.exec(mFlags)) return;
            //save file
            if(mPath.endsWith(".arff")) saveWEKA();
            else if(mPath.endsWith(".fann")) saveFANN();
        }
    }
}


void GenMyoWindow::open(const QString& str)
{
    //if(!mWekaItems.empty())
    {
        //path
        QString sPath=QFileDialog::getOpenFileName(this,
                                                  "Open",
                                                   mPath,
                                                   "Myo Files *.myodata");

        if( sPath.length() )
        {
            //path
            mPath=sPath;
            //get path
            std::string lPath=mPath.toStdString().c_str();
            assert(lPath.length());
            //save
            MyoListener::TypeInput input;
            input.read(lPath,
                       [](int idClass,
                          const std::string& className,
                          const MyoListener::TypeRaw& value)
            {

            });
        }
    }
}

void GenMyoWindow::save()
{
    if(!mWekaItems.empty())
    {
        //get all items
        size_t nItems=ui->mLWGestures->count();
        //get path
        std::string lPath=mPath.toStdString().c_str();
        assert(lPath.length());
        //file
        MyoListener::TypeOuput ouput;
        ouput.open(lPath,nItems);

        //seva all
        for(size_t i=0;i!=nItems;++i)
        {
            auto item=ui->mLWGestures->item(i);
            auto it=mWekaItems.find(item);
            auto widget=dynamic_cast<GestureForm*>(ui->mLWGestures->itemWidget(item));
            if(it != mWekaItems.end())
            {
                ouput.append(widget->getName().toStdString(), it.value());
            }
        }
    }
}

void GenMyoWindow::saveWEKA()
{
    if(!mWekaItems.empty())
    {
        //get all items
        size_t nItems=ui->mLWGestures->count();
        //ouput
        MyoListener::TypeOuputWeka ouput;
        //path
        std::string lPath=mPath.toStdString().c_str();
        assert(lPath.length());
        //cases
        switch (mFlags.mMode)
        {
            case DataFlags::SEMPLE_MODE:
            {
                //vector of names
                std::vector<std::string> classNames;
                //get all class
                for(size_t i=0;i!=nItems;++i)
                {
                    auto item=ui->mLWGestures->item(i);
                    auto widget=dynamic_cast<GestureForm*>(ui->mLWGestures->itemWidget(item));
                    classNames.push_back(widget->getName().toStdString());
                }
                //file
                ouput.open(lPath,
                           mFlags,
                           classNames);
                //seva all
                for(size_t i=0;i!=nItems;++i)
                {
                    auto item=ui->mLWGestures->item(i);
                    auto it=mWekaItems.find(item);
                    if(it != mWekaItems.end())
                    {
                        ouput.append(classNames[i], it.value());
                    }
                }
            }
            break;
            case DataFlags::GESTURE_MODE:
            {
                GesturesBuilder gbuilder(mFlags);
                //put all
                for(size_t i=0;i!=nItems;++i)
                {
                    auto item=ui->mLWGestures->item(i);
                    auto widget=dynamic_cast<GestureForm*>(ui->mLWGestures->itemWidget(item));
                    auto it=mWekaItems.find(item);
                    if(it != mWekaItems.end())
                    {
                        gbuilder.append(widget->getName(), it.value());
                    }
                }
                //nreps
                size_t nreps = 1;
                //get ouput
                GesturesBuilder::GestureOutput ouputItems;
                gbuilder.finalize(nreps,ouputItems);
                //////////////////////////////////////////////////////////////
                //type of ouput
                DataFlags flags( mFlags );
                //set nreps
                flags.mReps = nreps;
                //get keys
                QList< QString > keys = ouputItems.keys();
                //ouput
                ouput.open(lPath,
                           flags,
                           keys);
                //write
                for(auto& name:keys)
                {
                    //list of input
                    auto& inputs=ouputItems.value(name);
                    //put all into file
                    for(auto& rows:inputs)
                    {
                        ouput.append(name.toStdString(), rows);
                    }
                }
                //////////////////////////////////////////////////////////////
            }
            default:
            break;
        }
    }
}

void GenMyoWindow::saveFANN()
{
    if(!mWekaItems.empty())
    {
        //get all items
        size_t nItems=ui->mLWGestures->count();
        //path
        std::string lPath=mPath.toStdString().c_str();
        assert(lPath.length());
        //ouput serialize
        MyoListener::TypeOuputFANN ouput;
        //types
        switch (mFlags.mMode)
        {
            case DataFlags::SEMPLE_MODE:
            {
                //vector of names
                QList< QString > classNames;
                //get all class
                for(size_t i=0;i!=nItems;++i)
                {
                    auto item=ui->mLWGestures->item(i);
                    auto widget=dynamic_cast<GestureForm*>(ui->mLWGestures->itemWidget(item));
                    classNames.push_back(widget->getName());
                }
                //file
                ouput.open(lPath,
                           mFlags,
                           classNames);
                //seva all
                for(size_t i=0;i!=nItems;++i)
                {
                    auto item=ui->mLWGestures->item(i);
                    auto widget=dynamic_cast<GestureForm*>(ui->mLWGestures->itemWidget(item));
                    auto it=mWekaItems.find(item);
                    if(it != mWekaItems.end())
                    {
                        ouput.append(widget->getName(), it.value());
                    }
                }
            }
            break;
            case DataFlags::GESTURE_MODE:
            {
                GesturesBuilder gbuilder(mFlags);
                //put all
                for(size_t i=0;i!=nItems;++i)
                {
                    auto item=ui->mLWGestures->item(i);
                    auto widget=dynamic_cast<GestureForm*>(ui->mLWGestures->itemWidget(item));
                    auto it=mWekaItems.find(item);
                    if(it != mWekaItems.end())
                    {
                        gbuilder.append(widget->getName(), it.value());
                    }
                }
                //nreps
                size_t nreps = 1;
                //get ouput
                GesturesBuilder::GestureOutput ouputItems;
                gbuilder.finalize(nreps,ouputItems);
                //////////////////////////////////////////////////////////////
                //type of ouput
                DataFlags flags( mFlags );
                //set nreps
                flags.mReps = nreps;
                //get keys
                QList< QString > keys = ouputItems.keys();
                //ouput
                ouput.open(lPath,
                           flags,
                           keys);
                //write
                for(auto& name:keys)
                {
                    //list of input
                    auto& inputs=ouputItems.value(name);
                    //put all into file
                    for(auto& rows:inputs)
                    {
                        ouput.append(name, rows);
                    }
                }
                //////////////////////////////////////////////////////////////
            }
            default:
            break;
        }
    }
}


void GenMyoWindow::onShowInputs(bool show)
{
    mMyoDialog.setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Dialog);
    mMyoDialog.show();
    mMyoDialog.setFocus();
    mMyoDialog.activateWindow();
    mMyoDialog.raise();
}