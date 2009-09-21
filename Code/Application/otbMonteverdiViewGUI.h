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
#ifndef __otbMonteverdiViewGUI_h
#define __otbMonteverdiViewGUI_h


#include "otbListenerBase.h"

// Disabling deprecation warning
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4996)
#endif
#include "otbMonteverdiViewGroup.h"
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <FL/Fl_Group.H>
#include <FLU/Flu_Tree_Browser.h>

#include "otbMonteverdiModel.h"
#include "otbMonteverdiControllerInterface.h"

#include "itkObject.h"
#include "otbMVCModel.h"
#include "otbListenerBase.h"


namespace otb
{
/** \class MonteverdiViewGUI
 *
 */
class MonteverdiViewGUI
  : public ListenerBase, public otbMonteverdiViewGroup, public itk::Object
{
public:

  /** Standard class typedefs */
  typedef MonteverdiViewGUI             Self;
  typedef itk::Object                   Superclass;
  typedef itk::SmartPointer<Self>       Pointer;
  typedef itk::SmartPointer<const Self> ConstPointer;

  /** This is protected for the singleton. Use GetInstance() instead. */
  itkNewMacro(Self);
  itkTypeMacro(MonteverdiViewGUI,itk::Object);

  typedef MonteverdiModel                                 MonteverdiModelType;
  typedef MonteverdiControllerInterface::Pointer          MonteverdiControllerInterfacePointerType;
  typedef MonteverdiModelType::ModuleDescriptorMapType    ModuleDescriptorMapType;
  typedef MonteverdiModelType::ModuleMapType              ModuleMapType;

  typedef Module::OutputDataDescriptorMapType             OutputDataDescriptorMapType;


  /** Set the controller */
  itkGetObjectMacro(MonteverdiController,MonteverdiControllerInterface);
  itkSetObjectMacro(MonteverdiController,MonteverdiControllerInterface);


  /** Event from the model */
  virtual void Notify();
  void InitWidgets();
  void BuildTree();
  void Show();

  /** Constructor */
  MonteverdiViewGUI();


protected:

  /** Destructor */
  virtual ~MonteverdiViewGUI();
  virtual void Quit();

  void BuildMenus();
  void AddChild( std::string childname );
  void CreateModuleByKey(const char * modulekey);
  static void GenericCallback (Fl_Menu_* w, void* v);

  typedef std::pair<Self *,std::string> CallbackParameterType;




private:

  MonteverdiViewGUI(const Self&); //purposely not implemented
  void operator=(const Self&); //purposely not implemented


  /** Pointer to the model */
  MonteverdiModel::Pointer m_MonteverdiModel;
  /** Pointer to the controller */
  MonteverdiControllerInterface::Pointer m_MonteverdiController;

  std::vector<CallbackParameterType *> m_vector_param;

  Flu_Tree_Browser        *m_Tree;

};
}//end namespace otb

#endif