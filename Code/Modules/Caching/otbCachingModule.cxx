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
#include "otbCachingModule.h"
#include <FLU/Flu_File_Chooser.h>
#include <FL/Fl.H>
#include <algorithm>
#include "otbMsgReporter.h"
#include "otbCachingPathManager.h"

#include "otbCachingModule.h"

namespace otb
{
/** Constructor */
CachingModule::CachingModule()
{
  // This module needs pipeline locking
  this->NeedsPipelineLockingOn();

  // Describe inputs
  this->AddInputDescriptor<FloatingVectorImageType>("InputDataSet", "Dataset to write");
  this->AddTypeToInputDescriptor<FloatingImageType>("InputDataSet");
  this->AddTypeToInputDescriptor<CharVectorImageType>("InputDataSet");
  this->AddTypeToInputDescriptor<LabeledImageType>("InputDataSet");

    // Create the caching dir if not already created
  CachingPathManager::GetInstance()->GetAValidCachingPath();
  m_CachingPath = CachingPathManager::GetInstance()->GetFullCachingPath();
  CachingPathManager::GetInstance()->CreateCachingDirectory();

  m_FilePath = "";
  m_WatchProgress = true;

  // Build gui
  this->BuildGUI();

  pBar->minimum(0);
  pBar->maximum(1);

  pBar->label("Caching dataset (0%)");
}

/** Destructor */
CachingModule::~CachingModule()
{
  // The producted file is not erase here.
  // We remove the entire Caching directory.
}

/** PrintSelf method */
void CachingModule::PrintSelf(std::ostream& os, itk::Indent indent) const
{
  // Call superclass implementation
  Superclass::PrintSelf(os, indent);
}

void CachingModule::UpdateProgress()
{
  double progress = m_WritingProcess->GetProgress();

  std::ostringstream oss1, oss2;
  oss1.str("");
  oss1 << "Caching dataset" << "  (" << std::floor(100 * progress) << "%)";
  oss2.str("");
  oss2 << std::floor(100 * progress);
  oss2 << "%";
  pBar->value(progress);
  wCachingWindow->copy_label(oss1.str().c_str());
  pBar->copy_label(oss2.str().c_str());
}

void CachingModule::UpdateProgressCallback(void * data)
{
  Self::Pointer caching = static_cast<Self *>(data);

  if (caching.IsNotNull())
    {
    caching->UpdateProgress();
    }
}

double CachingModule::GetProgress() const
{
  if (m_WritingProcess.IsNotNull())
    {
    return m_WritingProcess->GetProgress();
    }
  else
    {
    return 0.;
    }
}

bool CachingModule::RemoveCachingDirectory() const
{
  return itksys::SystemTools::RemoveADirectory(m_CachingPath.c_str());
}

void CachingModule::ThreadedWatch()
{
  double last = 0;
  double updateThres = 0.01;
  double current = -1;

  Fl::lock();
  Fl::awake(&ShowWindowCallback, this);
  Fl::unlock();

  while (this->IsBusy())
    {
    if (m_WritingProcess.IsNotNull())
      {
      current = m_WritingProcess->GetProgress();
      if (current - last > updateThres)
        {
        // Make the main fltk loop update progress fields
        Fl::awake(&UpdateProgressCallback, this);
        last = current;
        }
      }

    // Sleep for a while
    Sleep(500);
    }

  // Wait for the reader to be set
  while (this->IsBusy())
    {
    Sleep(500);
    }

  // Notify new outputs
  Fl::lock();
  Fl::awake(&HideWindowCallback, this);
  Fl::unlock();
}


void CachingModule::ThreadedRun()
{
  this->BusyOn();

  FloatingVectorImageType::Pointer vectorImage = this->GetInputData<FloatingVectorImageType>("InputDataSet");
  FloatingImageType::Pointer       singleImage = this->GetInputData<FloatingImageType>("InputDataSet");
  CharVectorImageType::Pointer     charVectorImage = this->GetInputData<CharVectorImageType>("InputDataSet");
  LabeledImageType::Pointer        labeledImage = this->GetInputData<LabeledImageType>("InputDataSet");

  Superclass::InputDataDescriptorMapType::const_iterator inputIt = this->GetInputsMap().find("InputDataSet");

  if (inputIt == this->GetInputsMap().end())
    {
    itkExceptionMacro(<< "Could not find input with key InputDataSet");
    }

  std::string sourceId = inputIt->second.GetNthData(0).GetSourceInstanceId();
  std::string outputKey = inputIt->second.GetNthData(0).GetSourceOutputKey();

  // Create description
  std::ostringstream oss;
  oss << "Cached data from" << " " << sourceId << " (" << outputKey << ")";
  std::string description = oss.str();
  oss.str("");

  // Ensure no white spaces
  std::replace(sourceId.begin(), sourceId.end(), ' ', '_');
  std::replace(outputKey.begin(), outputKey.end(), ' ', '_');

  // Build cached file name
  oss << m_CachingPath << sourceId << outputKey << ".tif";
  m_FilePath = oss.str();
  oss.str("");

  try
    {
    if (charVectorImage.IsNotNull())
      {
      // Writing
      CharVWriterType::Pointer charVWriter = CharVWriterType::New();
      charVWriter->SetInput(charVectorImage);
      charVWriter->SetFileName(m_FilePath);
      m_WritingProcess = charVWriter;
      charVWriter->Update();

      // Reading
      CharVReaderType::Pointer charVReader = CharVReaderType::New();
      charVReader->SetFileName(m_FilePath);
      charVReader->UpdateOutputInformation();
      m_ReadingProcess = charVReader;
      this->AddOutputDescriptor(charVReader->GetOutput(), "CachedData", description, true);

      // Notify new outputs
      Fl::lock();
      this->NotifyOutputsChange();
      Fl::unlock();
      }

    else if (vectorImage.IsNotNull())
      {
      // Writing
      FPVWriterType::Pointer fPVWriter = FPVWriterType::New();
      fPVWriter->SetInput(vectorImage);
      fPVWriter->SetFileName(m_FilePath);
      m_WritingProcess = fPVWriter;
      fPVWriter->Update();

      // Reading
      FPVReaderType::Pointer fPVReader = FPVReaderType::New();
      fPVReader->SetFileName(m_FilePath);
      fPVReader->UpdateOutputInformation();
      m_ReadingProcess =  fPVReader;
      this->AddOutputDescriptor(fPVReader->GetOutput(), "CachedData", description, true);

      // Notify new outputs
      Fl::lock();
      this->NotifyOutputsChange();
      Fl::unlock();
      }
    else if (singleImage.IsNotNull())
      {
      // Writing
      FPWriterType::Pointer fPWriter = FPWriterType::New();
      fPWriter->SetInput(singleImage);
      fPWriter->SetFileName(m_FilePath);
      m_WritingProcess = fPWriter;
      fPWriter->Update();

      // Reading
      FPReaderType::Pointer fPReader = FPReaderType::New();
      fPReader->SetFileName(m_FilePath);
      fPReader->UpdateOutputInformation();
      m_ReadingProcess =  fPReader;
      this->AddOutputDescriptor(fPReader->GetOutput(), "CachedData", description, true);

      // Notify new outputs
      Fl::lock();
      this->NotifyOutputsChange();
      Fl::unlock();
      }
  else if (labeledImage.IsNotNull())
      {
      // Writing
      LabeledWriterType::Pointer labeledWriter = LabeledWriterType::New();
      labeledWriter->SetInput(labeledImage);
      labeledWriter->SetFileName(m_FilePath);
      m_WritingProcess = labeledWriter;
      labeledWriter->Update();

      // Reading
      LabeledReaderType::Pointer labeledReader = LabeledReaderType::New();
      labeledReader->SetFileName(m_FilePath);
      labeledReader->UpdateOutputInformation();
      m_ReadingProcess = labeledReader;
      this->AddOutputDescriptor(labeledReader->GetOutput(), "CachedData", description, true);

      // Notify new outputs
      Fl::lock();
      this->NotifyOutputsChange();
      Fl::unlock();
      }
    else
      {
      this->BusyOff();
      itkExceptionMacro(<< "Input data are NULL.");
      }
    }
  catch (itk::ExceptionObject& err)
    {
    // Make the main fltk loop update Msg reporter
    m_ErrorMsg = err.GetDescription();
    Fl::awake(&SendErrorCallback, &m_ErrorMsg);
    this->BusyOff();
    }
  this->BusyOff();
}

void CachingModule::HideWindow()
{
  wCachingWindow->hide();
}

void CachingModule::HideWindowCallback(void * data)
{
  Self::Pointer caching = static_cast<Self *>(data);

  if (caching.IsNotNull())
    {
      caching->HideWindow();
    }
}

void CachingModule::ShowWindow()
{
  wCachingWindow->show();
}

void CachingModule::ShowWindowCallback(void * data)
{
  Self::Pointer caching = static_cast<Self *>(data);

  if (caching.IsNotNull())
    {
    caching->ShowWindow();
    }
}

void CachingModule::Run()
{
  CachingPathManager::GetInstance()->GetAValidCachingPath();
  m_CachingPath = CachingPathManager::GetInstance()->GetCachingPath();

  this->StartProcess2();
  if (m_WatchProgress)
    {
    this->StartProcess1();
    }
}

void CachingModule::SendErrorCallback(void * data)
{
  std::string * error = static_cast<std::string *>(data);
  //TODO test if error is null
  if (error == NULL)
    {
    MsgReporter::GetInstance()->SendError("Unknown error during update");
    }
  else
    {
    MsgReporter::GetInstance()->SendError(error->c_str());
    }
}

} // End namespace otb
