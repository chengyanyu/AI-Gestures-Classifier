//
//  kNN.cpp
//  FANNMyo
//
//  Created by Gabriele Di Bari on 04/10/15.
//  Copyright © 2015 Gabriele Di Bari. All rights reserved.
//
#include "../DataSetReader.h"
#include "kNN.h"
#include <assert.h>
#include <cmath>
#include <map>

kNN::kNN(const kNN::DataSet& dataset)
:mDataSet(dataset)
{
}

kNN::Result kNN::classify(const kNN::DataRaw& row,
                          unsigned int nNN,
                          DistanceType type,
                          DistanceWeight typeWeight ) const
{
    //field selected
    struct SelectField
    {
        double           mDistance = 0.0;
        const DataField* mField    = nullptr;
    };
    //list of 'k' NN
    std::vector< SelectField > selected;
    //resize the buffer K's list buffer
    selected.resize(nNN);
	//count of row added
	int rowAdded = 0;
	//ref to most distance field
	SelectField* mostDistanceField = nullptr;
    //serach in data set
    //for(auto& field:mDataSet
#if defined(_MSC_VER) && (_MSC_VER <= 1800)
    for (DataSetReader::Rows::const_iterator 
         itField  = this->mDataSet.cbegin(); 
         itField != this->mDataSet.cend();
         ++itField)
    {
        auto& field = *itField;
#else
    for (auto& field : mDataSet)
    {
#endif

        //compute distance
        double rfdistance = 0.0;
        //select distance type
        switch (type)
        {
            default:
            case EUCLIDE_DISTANCE:   rfdistance = distance(field.mRaw, row);  break;
            case MANHATTAN_DISTANCE: rfdistance = manhattan(field.mRaw, row);  break;
            case HAMMING_DISTANCE:   rfdistance = hamming(field.mRaw, row);  break;
        }
		//not all selected
		if (rowAdded < selected.size())
		{
			for (auto& selectRow : selected)
			{
				if (!selectRow.mField)
				{
					selectRow.mDistance = rfdistance;
					selectRow.mField = &field;
					//get max
					if (!mostDistanceField || mostDistanceField->mDistance < rfdistance)
					{
						mostDistanceField = &selectRow;
					}
					//inc 
					++rowAdded;
					//next
					break;
				}
			}
		}
		//all selected rows are occupied
		else 
		{
			if (mostDistanceField->mDistance > rfdistance)
			{
				mostDistanceField->mDistance = rfdistance;
				mostDistanceField->mField = &field;
				//search most distance field
				for (auto& selectRow : selected)
				{
					//get max
					if (mostDistanceField->mDistance < rfdistance)
					{
						mostDistanceField = &selectRow;
					}
				}
			}
		}
    }
    //democratic selection
    struct WightAndCount
    {
        double mWeight { 0.0 };
        size_t mCount  { 0   };
        //utility operators
        WightAndCount& operator += (const WightAndCount& in)
        {
            mWeight += in.mWeight;
            mCount  += in.mCount;
            return (*this);
        }
    };
    std::map< double, WightAndCount > selectedMap;
    //count occurrences
    for(auto& select : selected)
        if(select.mField)
        {
            switch (typeWeight)
            {
                default:
                case DEMOCRATIC:
                    ++selectedMap[ select.mField->mClass ].mWeight;
                    break;
                case ONE_ON_DISTANCE:
                    selectedMap[ select.mField->mClass ].mWeight += 1.0 / select.mDistance;
                    break;
                case ONE_MINUS_DISTANCE:
                    selectedMap[ select.mField->mClass ].mWeight += 1.0 - select.mDistance;
                    break;
            }
            ++selectedMap[ select.mField->mClass ].mCount;
        }
    //is a unsuccess
    if(selectedMap.empty()) {  return Result();  }
    //select class whit max weight
    double         classes  = selectedMap.begin()->first;
    WightAndCount    cinfo  = selectedMap.begin()->second;
    //find max info
    WightAndCount maxinfo;
    
    for(auto it:selectedMap)
    {
        if(it.second.mWeight > cinfo.mWeight)
        {
            classes   = it.first;
            cinfo     = it.second;
        }
        //add weight
        maxinfo += it.second;
    }
    //compute error
    double error = 0.0;
    switch (typeWeight)
    {
        default:
        case DEMOCRATIC:
            error = 1.0 - ( cinfo.mWeight / nNN );
            break;
        case ONE_ON_DISTANCE:
            error =  1.0 - ((1.0/maxinfo.mWeight) / (1.0/cinfo.mWeight)) ;
            break;
        case ONE_MINUS_DISTANCE:
            error = 1.0 - ( (-cinfo.mWeight  +cinfo.mCount) /
                           (-maxinfo.mWeight+maxinfo.mCount) );
            break;
    }
    //return selected
    return Result( true,  classes,  error);
}

double kNN::distance(const kNN::DataRaw& left,const kNN::DataRaw& right)
{
    assert(left.size() == right.size());
    double distance = 0.0;
    //compute distance
#if defined(_OPENMP)
    size_t lSize = left.size();
#pragma omp parallel for reduction(+:distance)
    for(size_t i=0; i < lSize; ++i)
    {
        double idiff  = left[i] - right[i];
        idiff *= idiff;
        distance = distance + idiff;
    }
#else
    for(size_t i=0; i != left.size(); ++i)
    {
        double idiff = left[i] - right[i];
        distance    += idiff * idiff;
    }
#endif
    return std::sqrt(distance);
}

double kNN::manhattan(const DataRaw& left,const DataRaw& right)
{
    assert(left.size() == right.size());
    double distance = 0.0;
    //compute distance
#if defined(_OPENMP)
    size_t lSize = left.size();
#pragma omp parallel for reduction(+:distance)
    for(size_t i=0; i < lSize; ++i)
    {
        double dis=std::abs(left[i] - right[i]);
        distance  = distance + dis;
    }
#else
    for(size_t i=0; i != left.size(); ++i)
    {
        distance += std::abs(left[i] - right[i]);
    }
#endif
    return distance;
}

double kNN::hamming(const DataRaw& left,const DataRaw& right)
{
    assert(left.size() == right.size());
    double distance = 0.0;
    //compute distance
#if defined(_OPENMP)
    size_t lSize = left.size();
#pragma omp parallel for reduction(+:distance)
    for(size_t i=0; i < lSize; ++i)
    {
        // hamming :
        // if is equal  then  0 else 1
        double dis=(double)(left[i] != right[i]);
        distance  = distance + dis;
    }
#else
    for(size_t i=0; i != left.size(); ++i)
    {
        // hamming :
        // if is equal  then  0 else 1
        distance += (left[i] != right[i]);
    }
#endif
    return distance;
}
