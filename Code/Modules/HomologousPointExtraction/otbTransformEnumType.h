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

#ifndef __otbTransformEnumType_h
#define __otbTransformEnumType_h

namespace otb
{

typedef enum {
  TRANSLATION,
  AFFINE,
  SIMILARITY2D,
  UNKNOWN
}  TransformEnumType;

}

#endif
