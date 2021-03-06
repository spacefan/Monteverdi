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
#ifndef __otbMeanShiftModule_h
#define __otbMeanShiftModule_h

#include "otbEventsListener.h"

// include the base class
#include "otbModule.h"

// the MVC classes
#include "otbMeanShiftModuleController.h"
#include "otbMeanShiftModuleModel.h"
#include "otbMeanShiftModule.h"

// include the OTB/ITK elements
#include "otbVectorImage.h"
#include "otbImage.h"

namespace otb
{
/** \class MeanShiftModule
 *
 *  \brief This is the MeanShift module, which allows to perform
 *  MeanShift filtering, segmentation and clustering.
 *
 *
 */

class ITK_ABI_EXPORT MeanShiftModule
//   : public Module, public ListenerBase
  : public Module, public EventsListener<std::string>
{
public:
  /** Standard class typedefs */
  typedef MeanShiftModule               Self;
  typedef Module                        Superclass;
  typedef itk::SmartPointer<Self>       Pointer;
  typedef itk::SmartPointer<const Self> ConstPointer;

  /** New macro */
  itkNewMacro(Self);

  /** Type macro */
  itkTypeMacro(MeanShiftModule, Module);

  /** MVC typedefs */
  typedef otb::MeanShiftModuleController ControllerType;
  typedef otb::MeanShiftModuleModel      ModelType;
  typedef otb::MeanShiftModuleView       ViewType;

  /** Data typedefs */
  typedef ModelType::VectorImageType  FloatingVectorImageType;
  typedef ModelType::LabeledImageType LabelImageType;

  itkGetObjectMacro(View, MeanShiftModuleView);

protected:
  /** Constructor */
  MeanShiftModule();

  /** Destructor */
  virtual ~MeanShiftModule();

  /** Notify Monteverdi application that featureExtraction has a result */
  void Notify(const std::string& event);

  /** PrintSelf method */
  virtual void PrintSelf(std::ostream& os, itk::Indent indent) const;

  /** The custom run command */
  virtual void Run();

  /** Show the Module GUI */
  virtual bool CanShow(){return true; }

  /** Show the Module GUI */
  virtual void Show()
  {
    m_View->Show();
  }

  /** Hide the Module GUI */
  virtual void Hide()
  {
    m_View->Hide();
  }

private:
  MeanShiftModule(const Self&); //purposely not implemented
  void operator =(const Self&); //purposely not implemented

  ControllerType::Pointer m_Controller;
  ViewType::Pointer       m_View;
  ModelType::Pointer      m_Model;
};

} // End namespace otb

#endif
