#include <cstdlib>

#include "otbImage.h"
#include "otbImageFileReader.h"
#include "itkBinaryThresholdImageFilter.h"

#include "otbModule.h"

#include "otbImageFileWriter.h"

//Execution
// ./Prototype1 ~/OTB/trunk/OTB-Data/Examples/qb_RoadExtract.tif


int main(int argc, char* argv[])
{
  if (argc < 2)
  {
    std::cerr << "Missing Parameters " << std::endl;
    std::cerr << "Usage: " << argv[0];
    std::cerr << " image outImage" << std::endl;
    return EXIT_FAILURE;
  }

  typedef otb::Image<double, 2> ImageType;
  typedef otb::ImageFileReader<ImageType> ReaderType;
  typedef itk::BinaryThresholdImageFilter<ImageType, ImageType> ThresholdType;
  typedef itk::ProcessObject ProcessType;

  //First idea, checking that ProcessObject manipulation works

  //Either declare the filter with its type first
  //then manipulate a a ProcessObject
  ReaderType::Pointer reader = ReaderType::New();
  reader->SetFileName(argv[1]);
  ProcessType::Pointer process1 = reader.GetPointer();

  //Or directly declare it as a ProcessObject
  ProcessType::Pointer process2 = (ThresholdType::New()).GetPointer();

  //Syntax at the ProcessObject level is a bit convoluted
  process2->GetInputs()[0] = process1->GetOutputs()[0];
  process2->Update();


  //Now, we try to get a more generic interface, the problem being passing the
  //parameters uniformly

  typedef otb::ModuleBase ModuleBase;
  ModuleBase::Pointer moduleReader = (otb::ModuleReader::New()).GetPointer();
  otb::Parameter<std::string>* filename = new otb::Parameter<std::string>(argv[1]);
  moduleReader->SetParameters("FileName", filename);

  /// Describe first module IO
  ModuleBase::InputDataDescriptorMapType inmap1 =  moduleReader->GetInputDataDescriptorsMap();
  ModuleBase::OutputDataDescriptorMapType outmap1 =  moduleReader->GetOutputDataDescriptorsMap();

  std::cout<<std::endl;
  std::cout<<"Module 1 (I/O): "<<std::endl;
  std::cout<<std::endl;
  
  for(ModuleBase::InputDataDescriptorMapType::const_iterator inIt1 = inmap1.begin(); inIt1 != inmap1.end();++inIt1)
    {
    std::cout<<"Name: "<<inIt1->second.m_DataKey<<", type: "<<inIt1->second.m_DataType<<", description: "<<inIt1->second.m_DataDescription;
    if(inIt1->second.m_Optional)
      std::cout<<" (optional)";
    if(inIt1->second.m_Multiple)
      std::cout<<" (multiple)";
    std::cout<<std::endl;
    }

  for(ModuleBase::OutputDataDescriptorMapType::const_iterator outIt1 = outmap1.begin(); outIt1 != outmap1.end();++outIt1)
    {
    std::cout<<"Name: "<<outIt1->second.m_DataKey<<", type: "<<outIt1->second.m_DataType<<", description: "<<outIt1->second.m_DataDescription<<", cardinal: "<<outIt1->second.m_NumberOfData<<std::endl;
    }


  ModuleBase::Pointer moduleThreshold = (otb::ModuleThreshold::New()).GetPointer();
  otb::Parameter<double>* thres = new otb::Parameter<double>(150.0);
  moduleThreshold->SetParameters("UpperThreshold", thres);

  /// Describe second module IO
  ModuleBase::InputDataDescriptorMapType inmap2 =  moduleThreshold->GetInputDataDescriptorsMap();
  ModuleBase::OutputDataDescriptorMapType outmap2 =  moduleThreshold->GetOutputDataDescriptorsMap();

  std::cout<<std::endl;
  std::cout<<"Module 2 (Threshold): "<<std::endl;
  std::cout<<std::endl;

  for(ModuleBase::InputDataDescriptorMapType::const_iterator inIt2 = inmap2.begin(); inIt2 != inmap2.end();++inIt2)
    {
    std::cout<<"Name: "<<inIt2->second.m_DataKey<<", type: "<<inIt2->second.m_DataType<<", description: "<<inIt2->second.m_DataDescription;
    if(inIt2->second.m_Optional)
      std::cout<<" (optional)";
    if(inIt2->second.m_Multiple)
      std::cout<<" (multiple)";
    std::cout<<std::endl;
    }

  for(ModuleBase::OutputDataDescriptorMapType::const_iterator outIt2 = outmap2.begin(); outIt2 != outmap2.end();++outIt2)
    {
    std::cout<<"Name: "<<outIt2->second.m_DataKey<<", type: "<<outIt2->second.m_DataType<<", description: "<<outIt2->second.m_DataDescription<<", cardinal: "<<outIt2->second.m_NumberOfData<<std::endl;
    }

  //Convenience accessor can be defined at the module level
  //to make the syntax better.
  moduleThreshold->AddInputData("InputImage",moduleReader->GetOutputByKey("OutputImage",0));

  // Update the module
  moduleThreshold->Update();


  ModuleBase::Pointer moduleWriter = (otb::ModuleWriter::New()).GetPointer();
  otb::Parameter<std::string>* outfname = new otb::Parameter<std::string>(argv[2]);
  moduleWriter->SetParameters("FileName", outfname);

  /// Describe second module IO
  ModuleBase::InputDataDescriptorMapType inmap3 =  moduleWriter->GetInputDataDescriptorsMap();
  ModuleBase::OutputDataDescriptorMapType outmap3 =  moduleWriter->GetOutputDataDescriptorsMap();

  std::cout<<std::endl;
  std::cout<<"Module 3 (Writer): "<<std::endl;
  std::cout<<std::endl;

  for(ModuleBase::InputDataDescriptorMapType::const_iterator inIt3 = inmap3.begin(); inIt3 != inmap3.end();++inIt3)
    {
    std::cout<<"Name: "<<inIt3->second.m_DataKey<<", type: "<<inIt3->second.m_DataType<<", description: "<<inIt3->second.m_DataDescription;
    if(inIt3->second.m_Optional)
      std::cout<<" (optional)";
    if(inIt3->second.m_Multiple)
      std::cout<<" (multiple)";
    std::cout<<std::endl;
    }

  for(ModuleBase::OutputDataDescriptorMapType::const_iterator outIt3 = outmap3.begin(); outIt3 != outmap3.end();++outIt3)
    {
    std::cout<<"Name: "<<outIt3->second.m_DataKey<<", type: "<<outIt3->second.m_DataType<<", description: "<<outIt3->second.m_DataDescription<<", cardinal: "<<outIt3->second.m_NumberOfData<<std::endl;
    }
  std::cout<<std::endl;

  //Convenience accessor can be defined at the module level
  //to make the syntax better.
  moduleWriter->AddInputData("InputImage",moduleThreshold->GetOutputByKey("OutputImage",0));

  // Update the module
  moduleWriter->Update();

  return EXIT_SUCCESS;
}

