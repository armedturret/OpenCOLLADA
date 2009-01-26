/*
    Copyright (c) 2008 NetAllied Systems GmbH

    This file is part of MayaDataModel.

    Licensed under the MIT Open Source License,
    for details please see LICENSE file or the website
    http://www.opensource.org/licenses/mit-license.php
*/
#ifndef __MayaDM_BIRAILSRF_H__
#define __MayaDM_BIRAILSRF_H__
#include "MayaDMTypes.h"
#include "MayaDMConnectables.h"
#include "MayaDMAbstractBaseCreate.h"
namespace MayaDM
{
class BirailSrf : public AbstractBaseCreate
{
public:
public:
	BirailSrf(FILE* file,const std::string& name,const std::string& parent=""):AbstractBaseCreate(file, name, parent, "birailSrf"){}
	virtual ~BirailSrf(){}
	void setTransformMode(unsigned int tm)
	{
		if(tm == 0) return;
		fprintf(mFile,"setAttr \".tm\" %i;\n", tm);

	}
	void getInputRail1()
	{
		fprintf(mFile,"\"%s.ir1\"",mName.c_str());

	}
	void getInputRail2()
	{
		fprintf(mFile,"\"%s.ir2\"",mName.c_str());

	}
	void getSweepStyle()
	{
		fprintf(mFile,"\"%s.ss\"",mName.c_str());

	}
	void getTransformMode()
	{
		fprintf(mFile,"\"%s.tm\"",mName.c_str());

	}
	void getOutputSurface()
	{
		fprintf(mFile,"\"%s.os\"",mName.c_str());

	}
protected:
	BirailSrf(FILE* file,const std::string& name,const std::string& parent,const std::string& nodeType):AbstractBaseCreate(file, name, parent, nodeType) {}

};
}//namespace MayaDM
#endif//__MayaDM_BIRAILSRF_H__