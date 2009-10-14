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


// Disabling deprecation warning
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4996)
#endif
#include "otbMonteverdiViewGroup.h"
#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_Menu_Window.H>
#include <FL/Fl_Group.H>
#include <FLU/Flu_Tree_Browser.h>

#include "otbMonteverdiModel.h"
#include "otbMonteverdiControllerInterface.h"

#include "otbInputViewGUI.h"

#include "itkWeakPointer.h"

#include "itkObject.h"


namespace otb
{
/** \class MonteverdiViewGUI
 *
 */
class MonteverdiViewGUI
  : public MonteverdiViewGroup, public itk::Object, public EventsListener<MonteverdiEvent>
{
public:

  /** Standard class typedefs */
  typedef MonteverdiViewGUI                               Self;
  typedef itk::Object                                     Superclass;
  typedef itk::SmartPointer<Self>                         Pointer;
  typedef itk::SmartPointer<const Self>                   ConstPointer;

  /** Standard type macros */
  itkNewMacro(Self);
  itkTypeMacro(MonteverdiViewGUI,itk::Object);

  /** Typedefs */
  typedef MonteverdiModel                                 MonteverdiModelType;
  typedef MonteverdiControllerInterface::Pointer          MonteverdiControllerInterfacePointerType;
  typedef MonteverdiModelType::ModuleDescriptorMapType    ModuleDescriptorMapType;
  typedef MonteverdiModelType::ModuleMapType              ModuleMapType;
  typedef Module::OutputDataDescriptorMapType             OutputDataDescriptorMapType;
  typedef Module::InputDataDescriptorMapType              InputDataDescriptorMapType;

  /** Inner class to represent the content of a node */
  class NodeDescriptor
  {
    friend class MonteverdiViewGUI;
  public:
    /** Setters */
    void SetModuleInstanceId(std::string id){ m_ModuleInstanceId = id; }
    void SetOutputKey(std::string id){ m_OutputKey = id; }
  
    /** Getters */
    const std::string GetOutputKey(){return m_OutputKey;}
    const std::string GetModuleInstanceId(){return m_ModuleInstanceId;}

  protected:
   // key to identify the module instance
    std::string m_ModuleInstanceId;

    // key to identify the output
    std::string m_OutputKey;
  };

  /** Get/Set the controller */
  itkGetObjectMacro(MonteverdiController,MonteverdiControllerInterface);
  itkSetObjectMacro(MonteverdiController,MonteverdiControllerInterface);

  /** Get the inputViewGUI */
  itkGetObjectMacro(InputViewGUI,InputViewGUI);

  /** Process event from the model */
  virtual void Notify(const MonteverdiEvent & event);

  /** Init the widgets (could be moved in the constructor */
  void InitWidgets();

  /** Show the main interface */
  void Show();

  /** Exit the application */
  void Quit();

  /** Create a module instance according to its name */
  void CreateModuleByKey(const char * modulekey);

protected:
   /** Constructor */
  MonteverdiViewGUI();

  /** Destructor */
  virtual ~MonteverdiViewGUI();

  /** Generate the menu according to registered modules */
  void BuildMenus();

  /** First build of the data tree */
  void BuildTree();

  /** Update the tree information regarding the given instanceId */
  void UpdateTree(const std::string & instanceId);

  /** Generate the gui to choose inputs for a module */
  void BuildInputsGUI(const std::string & modulekey);

  /** Show the help */
  void Help();

  /** Static callbacks */
  static void GenericCallback(Fl_Menu_* w, void* v);
  static void HelpCallback(Fl_Menu_ *, void*);
  static void QuitCallback(Fl_Menu_ *, void*);
  static void TreeCallback( Fl_Widget* w, void* );


private:
  MonteverdiViewGUI(const Self&); //purposely not implemented
  void operator=(const Self&); //purposely not implemented

  /** Pointer to the model */
  MonteverdiModel::Pointer m_MonteverdiModel;
  /** Pointer to the controller */
  itk::WeakPointer<MonteverdiControllerInterface> m_MonteverdiController;

  /** The Flu tree browser */
  Flu_Tree_Browser        *m_Tree;

  /** The module input selection interface */
  InputViewGUI::Pointer   m_InputViewGUI;

  /** The help box text buffer */
  Fl_Text_Buffer * m_HelpTextBuffer;

};
}//end namespace otb

#endif
