/*=========================================================================

Program:   ORFEO Toolbox
Language:  C++
Date:      $Date$
Version:   $Revision$


Copyright (c) Centre National d'Etudes Spatiales. All rights reserved.
See OTBCopyright.txt for details.


This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notices for more information.

=========================================================================*/

#include "otbImageFileReader.h"
#include "otbImageFileWriter.h"
#include "otbVectorImage.h"

#include "otbAsynchronousModel.h"

namespace otb
{
namespace Process
{


/** Constructor */
AsynchronousModel
::AsynchronousModel() : m_Process1(NULL), m_Process2(NULL), m_MagicMain(NULL)
{
  std::cout<<"Const. AsynchronousModel : "<< this <<std::endl; 
  m_Process1 = new AsynchronousProcess("Process 1");
  m_Process2 = new AsynchronousProcess("Process 2");
  m_Process1->SetThreader( this->GetThreader());
  m_Process2->SetThreader( this->GetThreader());
  this->GetThreader()->SetNumberOfThreads(3);

  m_Process1->Start();
  sleep(1);
  m_Process2->Start();
}


/** Destructor */
AsynchronousModel
::~AsynchronousModel() 
{
  delete m_Process1;
  delete m_Process2;
}


void
AsynchronousModel
::LaunchProcess()
{
  m_Process1->Start();
  sleep(1);
  m_Process2->Start();
}

void
AsynchronousModel
::NotifyAll()
{
  if(m_MagicMain != NULL)
    m_MagicMain->Notify();
}

void
AsynchronousModel
::Register( MagicMain * myMain )
{
  m_MagicMain = myMain;
}

void
AsynchronousModel
::Run(void *t)
{
  struct itk::MultiThreader::ThreadInfoStruct * pInfo = (itk::MultiThreader::ThreadInfoStruct *)(t);
  AsynchronousModel* lThis = (AsynchronousModel*)(pInfo->UserData);
  std::cout<<"Run AsynchronousModel : "<<lThis<<std::endl; 

  bool done = true;
  bool stopped = false;
  int val1, val2 = 0;
  int val1Old, val2Old = -1;
  int val1OldBis, val2OldBis = -1;

  while(done)
    {
      val1 = lThis->m_Process1->GetProcessStatus();
      val2 = lThis->m_Process2->GetProcessStatus();

      if( val1OldBis!=val1 || val2OldBis!=val2 )
 	{
	  std::cout<<"Processor 1: "<< val1 <<"/100,  Processor 2: "<< val2 <<"/100"<<std::endl;
	  val1OldBis = val1;
	  val2OldBis = val2;
	}

      int diff1 = val1-val1Old;
      int diff2 = val2-val2Old;

      if( diff1 >= 10 || diff2 >= 10 )
 	{
	  std::cout<<"------ Processor 1: "<< val1 <<"/100,  Processor 2: "<< val2 <<"/100"<<std::endl;
	  lThis->NotifyAll();
	  val1Old = val1;
	  val2Old = val2;
	}        
  
  if( val1==100 && val2==100 )
 	{
	  done = false;
	}
    }
  std::cout<<"Processes finished"<<std::endl;

  /*
  while(done)
    {
      val1 = lThis->m_Process1->GetProcessStatus();
      val2 = lThis->m_Process2->GetProcessStatus();
      
      if( val1Old!=val1 || val2Old!=val2 )
 	{
	  std::cout<<"Processor 1: "<< val1 <<"/100,  Processor 2: "<< val2 <<"/100"<<std::endl;
	}        

      if(val1==50 && stopped == false)
	{
	  try
	    {
	      lThis->m_Process1->Stop();
	    }
	  catch(...)
	    {
	    }
	  stopped = true;
	}

      val1Old = val1;
      val2Old = val2;

  if( val1==100 && val2==100 )
 	{
	  done = false;
	}
    }
  std::cout<<"Processes finished"<<std::endl;
  */

}

} // namespace Model
} //namespace otb