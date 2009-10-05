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
#ifndef __otbModule_h
#define __otbModule_h

#include "itkObject.h"
#include "itkObjectFactory.h"

#include "otbDataObjectWrapper.h"
#include "otbInputDataDescriptor.h"
#include "otbOutputDataDescriptor.h"
#include "otbEventsSender.h"
#include "otbMonteverdiEvent.h"
#include "otbTypeManager.h"
#include "otbAsynchronousProcessBase.h"

namespace otb
{
/** \class Module
 *  \brief Base class for modules.
 *
 * The GetInputsMap() and GetOutputsMap() methods can be used to
 * retrieve inputs and outputs descriptors.
 *
 * Data must be passed and retrieve via the AddInputByKey() and
 * GetOutputByKey() methods.
 *
 * These methods performs key and types checking and call the
 * protected methods AssignInputByKey() and RetrieveOutputByKey()
 * which are the methods actually reimplemented by subclasses.
 *
 *  \sa DataObjectWrapper, InputDataDescriptor, OutputDataDescriptor
 */

class ITK_EXPORT Module
  : public EventsSender<MonteverdiEvent>, public AsynchronousProcessBase
{
public:
  /** Standard class typedefs */
  typedef Module                        Self;
  typedef itk::Object                   Superclass;
  typedef itk::SmartPointer<Self>       Pointer;
  typedef itk::SmartPointer<const Self> ConstPointer;

  /** New macro */
  itkNewMacro(Self);

  /** Type macro */
  itkTypeMacro(Module,itk::Object);

  /** Input data description map */
  typedef std::map<std::string, InputDataDescriptor>  InputDataDescriptorMapType;

  /** Output data description map */
  typedef std::map<std::string, OutputDataDescriptor> OutputDataDescriptorMapType;

  /** Add an input data by its key. This method performs key and type
   * checking before invoking the protected method
   * AssignInputByKey(). */
  void AddInputByKey(const std::string & key, const DataObjectWrapper & data);

  /** Get an output by its key. This method performs key and type
   * checking before invoking the protected method
   * AssignInputByKey(). */
  const DataObjectWrapper GetOutputByKey(const std::string & key, unsigned int idx = 0) const;

  /** Get the input data descriptors map */
  const InputDataDescriptorMapType  & GetInputsMap() const;

  /** Get the output data descriptors map */
  const OutputDataDescriptorMapType & GetOutputsMap() const;

  /** Check that every mandatory input has been filled and call the
   * protected virtual run method */
  void Start();

  /** Notify outputs change */
  void NotifyOutputsChange();

  /** Set/Get the instance id */
  itkSetStringMacro(InstanceId);
  itkGetStringMacro(InstanceId);

  /** Erase an output with its key */
  void EraseOutputByKey(const std::string & key);

protected:
  /** Constructor */
  Module();

  /** Destructor */
  virtual ~Module();

  /** PrintSelf method */
  virtual void PrintSelf(std::ostream& os, itk::Indent indent) const;

  /** Clear output descriptors */
  void ClearOutputDescriptors();

  /** Add a new input descriptor */
  template <typename T> void AddInputDescriptor(const std::string & key, const std::string & description, bool optional = false, bool multiple = false);

  /** Add additional supported types for a given input descriptors */
  template <typename T> void AddTypeToInputDescriptor(const std::string & key);

  /** Add a new output descriptor */
  template <typename T> void AddOutputDescriptor(T* data, const std::string & key, const std::string & description);

  /** Add a new output descritpor (SmartPointer version) */
  template <typename T> void AddOutputDescriptor(itk::SmartPointer<T> data, const std::string & key, const std::string & description);

  /** Retrieve the actual data from the map (returns NULL if wrong DataType */
  template <typename T> T * GetInputData(const std::string & key, unsigned int idx = 0) const;

  /** Retrieve the actual data description from the map */
  template <typename T> std::string GetInputDataDescription(const std::string & key, unsigned int idx = 0) const;

  /** Get the number of input data by key */
  unsigned int GetNumberOfInputDataByKey(const std::string & key) const;

  /** The custom run command */
  virtual void Run();

  /** The custom run command (asynchronous threaded version) */
  virtual void ThreadedRun();

  /** The custom Watch common to report progress of the ThreadedRun */
  virtual void ThreadedWatch();

  /** Instance id (intentionnaly left protected) */
  std::string m_InstanceId;

private:
  Module(const Self&); //purposely not implemented
  void operator=(const Self&); //purposely not implemented

  // Hiding the Asynchronous process base methods */
  virtual void RunProcess1(void *v);
  virtual void RunProcess2(void *v);
  

  /** Input descriptor map */
  InputDataDescriptorMapType m_InputsMap;

  /** Output descriptor map */
  OutputDataDescriptorMapType m_OutputsMap;
};


} // End namespace otb

#ifndef OTB_MANUAL_INSTANTIATION
#include "otbModule.txx"
#endif

#endif
