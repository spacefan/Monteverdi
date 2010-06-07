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

#include "otbSupervisedClassificationView.h"

namespace otb
{

void
SupervisedClassificationView
::Quit()
{
//  m_Controller->OK();
  this->HideAll();
}

void
SupervisedClassificationView
::HideAll()
{
  wMainWindow->hide();
  wSVMSetup->hide();
  wValidationWindow->hide();
}

void
SupervisedClassificationView
::Show()
{
  wMainWindow->show();
}

void
SupervisedClassificationView
::SetModel(SupervisedClassificationModel* model)
{
  m_Model = model;

  m_Model->RegisterListener(this);
}

}
