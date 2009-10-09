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
#ifndef __otbInputChoiceDescriptor_h
#define __otbInputChoiceDescriptor_h

#include <string>
#include <vector>
#include "itkObject.h"
#include "itkObjectFactory.h"
#include <FL/Fl_Choice.H>
#include <FL/Fl_Browser.H>

namespace otb
{

/** \class InputChoiceDescriptor
 *  \brief This small class describes the different input choices of a InputViewGUI.
 *
 *  The Optional flag allows to define optional inputs.
 *
 *  The Multiple flag allows to define unbounded multiple inputs.
 *
 */
 
class ITK_EXPORT InputChoiceDescriptor
  : public itk::Object
{
public:

 /** Standard class typedefs */
  typedef InputChoiceDescriptor         Self;
  typedef itk::Object                   Superclass;
  typedef itk::SmartPointer<Self>       Pointer;
  typedef itk::SmartPointer<const Self> ConstPointer;

  /** Standard type macros */
  itkNewMacro(Self);
  itkTypeMacro(InputChoiceDescriptor,itk::Object);

  /** Typedefs */
  // contains a module instance Id and a data key
  typedef std::pair<std::string,std::string>   StringPairType;
  typedef std::map<int, StringPairType>        StringPairMapType;

  /** Getters/Setters */
  itkSetMacro(Multiple, bool);
  itkSetMacro(Optional, bool);

  /** Is the input data optional ? */
  bool IsOptional() const;

  /** Is the input data multiple ? */
  bool IsMultiple() const;

  StringPairType GetSelected() const;
  bool HasSelected() const;


  Fl_Choice *                   m_FlChoice;
  Fl_Browser *                  m_FlBrowser;
  StringPairMapType             m_ChoiceMap;
  /** if input is multiple, we keep the indexes */
  std::vector<int>              m_Indexes;

protected:
  /** Constructor */
  InputChoiceDescriptor(){m_Optional=false; m_Multiple=false;};
  /** Destructor */
  virtual ~InputChoiceDescriptor(){};


private:
  InputChoiceDescriptor(const Self&); //purposely not implemented
  void operator=(const Self&); //purposely not implemented

  /** The optional flag */
  bool m_Optional;

  /** The multiple flag */
  bool m_Multiple;

};

} // End namespace otb

#endif
