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
#ifndef __otbTileExportModule_cxx
#define __otbTileExportModule_cxx

#include "otbTileExportModule.h"
#include "itksys/SystemTools.hxx"
#include <FLU/Flu_File_Chooser.h>
#include "otbMsgReporter.h"
#include "otbGeoInformationConversion.h"


namespace otb
{
/**
 * Constructor
 */
TileExportModule::TileExportModule() : m_Logo(NULL), m_LogoFilename(),
                                       m_HasLegend(false), m_HasLogo(false),
                                       m_TileSize(-1), m_KmzFile(NULL),
                                       m_RootKmlFile(), m_MaxDepth(0),
                                       m_CurIdx(0), m_CurrentProduct(0),
                                       m_CurrentDepth(-1), m_ProductVector(),
                                       m_InputHaveMetaData(true)
{
  // Add a multiple inputs
  this->AddInputDescriptor<FloatingVectorImageType>("InputImage", "Input image", false);
  this->AddTypeToInputDescriptor<SingleImageType>("InputImage");
  this->AddInputDescriptor<FloatingVectorImageType>("InputLegend", "Input Legend", true, true);
  //this->AddTypeToInputDescriptor<SingleImageType>("InputLegend"); //TODO
  this->AddInputDescriptor<FloatingVectorImageType>("InputLogo", "Input Logo", true, false);
  //this->AddTypeToInputDescriptor<SingleImageType>("InputLogo"); //TODO

  // Initialize some values
  // TODO : fix those values
  m_UpperLeftCorner.Fill(1000.);
  m_UpperRightCorner.Fill(1000.);
  m_LowerLeftCorner.Fill(1000.);
  m_LowerRightCorner.Fill(1000.);

  // Instanciate the caster
  m_CastToVectorImageFilter = CastToVectorImageFilterType::New();

  // Build the GUI
  this->BuildGUI();
}

/**
 * Destructor
 */
TileExportModule::~TileExportModule()
{}

/**
 * PrintSelf method
 */
void TileExportModule::PrintSelf(std::ostream& os, itk::Indent indent) const
{
  // Call superclass implementation
  Superclass::PrintSelf(os, indent);
}

/** The custom run command */
void TileExportModule::Run()
{
  // Get & rescale intensity of the input image
  m_VectorImage = this->GetInputData<FloatingVectorImageType>("InputImage");

  // Try to get a single image
  // If the input image is an otb::Image instead of VectorImage then cast it
  // in Vector Image and continue the processing
  SingleImageType::Pointer singleImage = this->GetInputData<SingleImageType>("InputImage");

  if (!singleImage.IsNull() && m_VectorImage.IsNull())
    {
    m_CastToVectorImageFilter->SetInput(singleImage);
    m_CastToVectorImageFilter->UpdateOutputInformation();
    m_VectorImage = m_CastToVectorImageFilter->GetOutput();
    }

  if (m_VectorImage.IsNull())
    {
    MsgReporter::GetInstance()->SendError("Input Image Is null");
    }

  m_VectorImage->UpdateOutputInformation();

  // Update the IsProductHaveMetaData
  this->IsProductHaveMetaData(0);

  // Set channel GUI
  unsigned int numberOfChannel = m_VectorImage->GetNumberOfComponentsPerPixel();

  for (unsigned int idx = 1; idx <= numberOfChannel; idx++)
    {
    std::ostringstream val;
    val << idx;
    this->cRedChannel->add(val.str().c_str());
    this->cGreenChannel->add(val.str().c_str());
    this->cBlueChannel->add(val.str().c_str());
    }

  // Set tile size GUI
  this->cTileSize->add("64");
  this->cTileSize->add("128");
  this->cTileSize->add("256");
  this->cTileSize->add("512");
  this->cTileSize->add("1024");

  // Set default value to 256
  this->cTileSize->value(2);

  // Get Legends
  if (this->GetNumberOfInputDataByKey("InputLogo") > 0)
    {
    m_HasLogo = true;
    m_Logo = this->GetInputData<FloatingVectorImageType>("InputLogo");
    }

  // Show the GUI
  this->Show();

  // Get the filename
  std::string fname  = this->GetInputDataDescription<FloatingVectorImageType>("InputImage");
  m_CurrentImageName = this->GetCuttenFileName(fname, 0);

  // Create an information class for each new product
  ProductInformationType newProduct;
  newProduct.m_Id   = 0;
  newProduct.m_Name = m_CurrentImageName;
  newProduct.m_Description = "Image Product";

  // Activate or not the Geo group
  if (!m_InputHaveMetaData && gExtended->value())
    {
    vgxGELatLongBoxGroup->activate();
    }

  // Try to guess them
  if (m_VectorImage->GetNumberOfComponentsPerPixel() > 3)
    {
    newProduct.m_Composition[0] = 2;
    newProduct.m_Composition[1] = 1;
    newProduct.m_Composition[2] = 0;

    this->cRedChannel->value(2);
    this->cGreenChannel->value(1);
    this->cBlueChannel->value(0);
    }
  else if (m_VectorImage->GetNumberOfComponentsPerPixel() == 3)
    {
    newProduct.m_Composition[0] = 0;
    newProduct.m_Composition[1] = 1;
    newProduct.m_Composition[2] = 2;

    this->cRedChannel->value(0);
    this->cGreenChannel->value(1);
    this->cBlueChannel->value(2);
    }
  else if (m_VectorImage->GetNumberOfComponentsPerPixel() == 1)
    {
    newProduct.m_Composition[0] = 0;
    newProduct.m_Composition[1] = 0;
    newProduct.m_Composition[2] = 0;

    this->cRedChannel->value(0);
    this->cGreenChannel->value(0);
    this->cBlueChannel->value(0);
    }

  // Initialize the corners with a high value
    for (unsigned int it = 0; it < newProduct.m_CornerList.Size(); it++)
      newProduct.m_CornerList[it] = 1000.;

    // Add the product class to the list
    m_ProductVector.push_back(newProduct);
}

/**
 * Browse Callback
 */
void TileExportModule::Browse()
{
  const char * filename = NULL;

  filename = flu_save_chooser("Choose the dataset file...", "*.kmz", "");

  if (filename == NULL)
    {
    otbMsgDebugMacro(<< "Empty file name!");
    return;
    }
  vFilePath->value(filename);
}

/**
 * OK CallBack
 */
void TileExportModule::SaveDataSet()
{
  std::string filepath = vFilePath->value();
  if (filepath.empty()) return;

  m_Path = itksys::SystemTools::GetFilenamePath(filepath);

  // Problem if the path if empty : it means we are in the current
  // directory
  if(m_Path.empty())
    m_Path = "./";

  m_FileName = itksys::SystemTools::GetFilenameWithoutExtension(filepath);

  // Expand the path
  m_Path = itksys::SystemTools::CollapseFullPath(m_Path.c_str());
  itksys::SystemTools::MakeDirectory(m_Path.c_str());

  // Create the extension following the user choice
  if (gExtended->value())
    {
    m_KmzExtension = "xt.kmz";
    m_KmlExtension = "xt.kml";
    }
  else
    {
    m_KmzExtension = ".kmz";
    m_KmlExtension = ".kml";
    }
  m_KmzFileName << m_Path << "/" << m_FileName << m_KmzExtension;

  // Check that the kmz file can be written
  if (!itksys::SystemTools::Touch(m_KmzFileName.str().c_str(), true))
    {
    std::ostringstream oss;
    oss<<"Cannot write the Kmz file (" << m_KmzFileName.str() << ") is not writeable, please choose another one.";
    MsgReporter::GetInstance()->SendError(oss.str());
    this->Hide();
    return;
    }
  itksys::SystemTools::RemoveFile(m_KmzFileName.str().c_str());

  // Create a kmz file

  m_KmzFile = kmlengine::KmzFile::Create(m_KmzFileName.str().c_str());

  // Set tile size
  switch (this->cTileSize->value())
    {
    case 0:
      m_TileSize = 64;
      break;
    case 1:
      m_TileSize = 128;
      break;
    case 2:
      m_TileSize = 256;
      break;
    case 3:
      m_TileSize = 512;
      break;
    case 4:
      m_TileSize = 1024;
      break;
    default:
      m_TileSize = 256;
      break;
    }

  // Generate Logo
  if (m_HasLogo)
    {
    m_LogoFilename << m_Path;
    m_LogoFilename << "/logo.jpeg";

    itksys::SystemTools::MakeDirectory(m_Path.c_str());

    CastFilterType::Pointer castFiler = CastFilterType::New();
    castFiler->SetInput(m_Logo);

    m_VectorWriter = VectorWriterType::New();
    m_VectorWriter->SetFileName(m_LogoFilename.str());
    m_VectorWriter->SetInput(castFiler->GetOutput());
    m_VectorWriter->Update();

    // Add the logo to the kmz
    std::ostringstream logo_root_path_in_kmz;
    logo_root_path_in_kmz << "logo.jpeg";

    std::ostringstream logo_absolut_path;
    logo_absolut_path << m_LogoFilename.str();

    this->AddFileToKMZ(logo_absolut_path, logo_root_path_in_kmz);

    // Remove the logo file
    if( itksys::SystemTools::FileExists(logo_absolut_path.str().c_str()) == true )
      {
        if (itksys::SystemTools::RemoveFile(logo_absolut_path.str().c_str()) == false)
          {
            std::ostringstream oss;
            oss << "Error while deleting the logo file " << logo_absolut_path.str();
            MsgReporter::GetInstance()->SendError(oss.str());
            this->Hide();
          }
      }
    else
      {
        //TODO : remove log when bug closed
        std::cout<<"LOG: Impossible to remove logo file, because doesn't exist... "<<logo_absolut_path.str()<<std::endl;
      }
    }

  // Store the legend associations
  this->StoreAssociations();

  // Do the tiling for the current image
  if (m_InputHaveMetaData)
    this->Tiling(0);
  else
    this->ExportNonGeoreferencedProduct(0);

  // Reset the boost::intrusive_ptr<KmzFile> :
  // TODO : when upgrading boost > 1.42 use method release().
  m_KmzFile = NULL;

  // close the GUI
  this->Hide();
}

void TileExportModule::Tiling(unsigned int curIdx)
{
  unsigned int numberOfChannel = m_VectorImage->GetNumberOfComponentsPerPixel();

  /** Image statistics*/
  FloatingVectorImageType::PixelType inMin(numberOfChannel), inMax(numberOfChannel), outMin(numberOfChannel), outMax(
    numberOfChannel);
  outMin.Fill(0);
  outMax.Fill(255);

  // Build wgs ref to compute long/lat
  // 4326 is the SRID number of WGS84
  std::string wgsRef = otb::GeoInformationConversion::ToWKT(4326);

  // Get the image size
  SizeType size;
  size = m_VectorImage->GetLargestPossibleRegion().GetSize();

  int sizeX = size[0];
  int sizeY = size[1];

  // Compute max depth
  int maxDepth =
    static_cast<int>(std::max(vcl_ceil(vcl_log(static_cast<float>(sizeX) / static_cast<float>(m_TileSize)) / vcl_log(2.0)),
                              vcl_ceil(vcl_log(static_cast<float>(sizeY) / static_cast<float>(m_TileSize)) / vcl_log(2.0))));
  m_MaxDepth = maxDepth;
  m_CurIdx = curIdx;

  // Compute nbTile
  int nbTile = 0;

  for (int i = 0; i <= maxDepth; i++)
    {
    int ratio = static_cast<int>(vcl_pow(2., (maxDepth - i)));
    nbTile += (((sizeX / ratio) / m_TileSize) + 1)  * (((sizeY / ratio) / m_TileSize) + 1);
    }

  // Progress bar
  pBar->minimum(0.0);
  pBar->maximum(static_cast<float>(nbTile));
  pBar->value(0);

  // Extract size & index
  SizeType  extractSize;
  IndexType extractIndex;

  for (int depth = 0; depth <= maxDepth; depth++)
    {
    // update the attribute value Current Depth
    m_CurrentDepth = depth;

    // Resample image to the max Depth
    int sampleRatioValue = 1 << (maxDepth - depth);

    if ( sampleRatioValue > 1)
      {
      m_StreamingShrinkImageFilter = StreamingShrinkImageFilterType::New();
      m_StreamingShrinkImageFilter->SetShrinkFactor(sampleRatioValue);
      m_StreamingShrinkImageFilter->SetInput(m_VectorImage);
      m_StreamingShrinkImageFilter->GetStreamer()->SetAutomaticStrippedStreaming(0);
      m_StreamingShrinkImageFilter->Update();

      m_VectorRescaleIntensityImageFilter = VectorRescaleIntensityImageFilterType::New();
      m_VectorRescaleIntensityImageFilter->SetInput(m_StreamingShrinkImageFilter->GetOutput());
      m_VectorRescaleIntensityImageFilter->SetOutputMinimum(outMin);
      m_VectorRescaleIntensityImageFilter->SetOutputMaximum(outMax);

      if ( depth == 0)
        {
        m_VectorRescaleIntensityImageFilter->Update();
        inMin = m_VectorRescaleIntensityImageFilter->GetInputMinimum();
        inMax = m_VectorRescaleIntensityImageFilter->GetInputMaximum();
        }
      else
        {
        m_VectorRescaleIntensityImageFilter->SetInputMinimum(inMin);
        m_VectorRescaleIntensityImageFilter->SetInputMaximum(inMax);
        m_VectorRescaleIntensityImageFilter->SetAutomaticInputMinMaxComputation(false);
        }

      // New resample vector image
      m_ResampleVectorImage = m_VectorRescaleIntensityImageFilter->GetOutput();
      }
    else
      {
      m_VectorRescaleIntensityImageFilter = VectorRescaleIntensityImageFilterType::New();
      m_VectorRescaleIntensityImageFilter->SetInput(m_VectorImage);
      m_VectorRescaleIntensityImageFilter->SetOutputMinimum(outMin);
      m_VectorRescaleIntensityImageFilter->SetOutputMaximum(outMax);

      m_VectorRescaleIntensityImageFilter->SetInputMinimum(inMin);
      m_VectorRescaleIntensityImageFilter->SetInputMaximum(inMax);
      m_VectorRescaleIntensityImageFilter->SetAutomaticInputMinMaxComputation(false);

      m_ResampleVectorImage = m_VectorRescaleIntensityImageFilter->GetOutput();
      }

    m_ResampleVectorImage->UpdateOutputInformation();

    // Get the image size
    size = m_ResampleVectorImage->GetLargestPossibleRegion().GetSize();

    sizeX = size[0];
    sizeY = size[1];

    int x = 0;
    int y = 0;

    // Tiling resample image
    for (int tx = 0; tx < sizeX; tx += m_TileSize)
      {
      for (int ty = 0; ty < sizeY; ty += m_TileSize)
        {
        if ((tx + m_TileSize) >= sizeX)
          {
          extractIndex[0] = tx;
          extractSize[0] = sizeX - tx;
          }
        else
          {
          extractIndex[0] = tx;
          extractSize[0] = m_TileSize;
          }

        if ((ty + m_TileSize) >= sizeY)
          {
          extractIndex[1] = ty;
          extractSize[1] = sizeY - ty;
          }
        else
          {
          extractIndex[1] = ty;
          extractSize[1] = m_TileSize;
          }

        itksys::SystemTools::MakeDirectory(m_Path.c_str());

        // Generate Tile filename
        std::ostringstream ossFileName;
        ossFileName << m_Path;
        ossFileName << "/";
        ossFileName << y;
        ossFileName << ".jpg";

        // Extract ROI
        m_VectorImageExtractROIFilter = VectorImageExtractROIFilterType::New();

        // Set extract roi parameters
        m_VectorImageExtractROIFilter->SetStartX(extractIndex[0]);
        m_VectorImageExtractROIFilter->SetStartY(extractIndex[1]);
        m_VectorImageExtractROIFilter->SetSizeX(extractSize[0]);
        m_VectorImageExtractROIFilter->SetSizeY(extractSize[1]);

        // Set Channels to extract : limited to three channels
        for (unsigned int idx = 0; idx < numberOfChannel && idx < 3; idx++)
          m_VectorImageExtractROIFilter->SetChannel(m_ProductVector[m_CurrentProduct].m_Composition[idx] + 1);

        // Set extract roi input
        m_VectorImageExtractROIFilter->SetInput(m_ResampleVectorImage);

        // Configure writer
        m_VectorWriter = VectorWriterType::New();
        m_VectorWriter->SetFileName(ossFileName.str().c_str());
        m_VectorWriter->SetInput(m_VectorImageExtractROIFilter->GetOutput());
        m_VectorWriter->Update();

        /** TODO : Generate KML for this tile */
        // Search Lat/Lon box
        m_Transform = TransformType::New();
        m_Transform->SetInputKeywordList(m_ResampleVectorImage->GetImageKeywordlist());
        m_Transform->SetInputProjectionRef(m_ResampleVectorImage->GetProjectionRef());
        m_Transform->SetOutputProjectionRef(wgsRef);
        // if(!m_DEMDirectory.empty())
        //   m_Transform->SetDEMDirectory(m_DEMDirectory);
        m_Transform->InstanciateTransform();

        InputPointType  inputPoint;
        OutputPointType outputPoint;
        IndexType       indexTile;
        SizeType        sizeTile, demiSizeTile;

        sizeTile = extractSize;
        demiSizeTile[0] = (sizeTile[0] / 2) - 1;
        demiSizeTile[1] = (sizeTile[1] / 2) - 1;

        // Compute North value
        indexTile[0] = extractIndex[0] + demiSizeTile[0];
        indexTile[1] = extractIndex[1];
        m_ResampleVectorImage->TransformIndexToPhysicalPoint(indexTile, inputPoint);
        outputPoint = m_Transform->TransformPoint(inputPoint);
        double north = outputPoint[1];

        // Compute South value
        indexTile[0] = extractIndex[0] + demiSizeTile[0];
        indexTile[1] = extractIndex[1] + sizeTile[1];
        m_ResampleVectorImage->TransformIndexToPhysicalPoint(indexTile, inputPoint);
        outputPoint = m_Transform->TransformPoint(inputPoint);
        double south = outputPoint[1];

        // Compute East value
        indexTile[0] = extractIndex[0] + sizeTile[0];
        indexTile[1] = extractIndex[1] + demiSizeTile[1];
        m_ResampleVectorImage->TransformIndexToPhysicalPoint(indexTile, inputPoint);
        outputPoint = m_Transform->TransformPoint(inputPoint);
        double east = outputPoint[0];

        // Compute West value
        indexTile[0] = extractIndex[0];
        indexTile[1] = extractIndex[1] + demiSizeTile[1];
        m_ResampleVectorImage->TransformIndexToPhysicalPoint(indexTile, inputPoint);
        outputPoint = m_Transform->TransformPoint(inputPoint);
        double west = outputPoint[0];

        // Compute center value (lat / long)
        indexTile[0] = extractIndex[0] + demiSizeTile[1];
        indexTile[1] = extractIndex[1] + demiSizeTile[1];
        m_ResampleVectorImage->TransformIndexToPhysicalPoint(indexTile, inputPoint);
        outputPoint = m_Transform->TransformPoint(inputPoint);
        double centerLat = outputPoint[1];
        double centerLong = outputPoint[0];

        /** GX LAT LON **/
        // Compute lower left corner
        indexTile[0] = extractIndex[0];
        indexTile[1] = extractIndex[1] + sizeTile[1];
        m_ResampleVectorImage->TransformIndexToPhysicalPoint(indexTile, inputPoint);
        outputPoint = m_Transform->TransformPoint(inputPoint);

        InputPointType testPoint;
        testPoint[0] =  indexTile[0];
        testPoint[1] =  indexTile[1];
        OutputPointType lowerLeftCorner = outputPoint;

        // Compute lower right corner
        indexTile[0] = extractIndex[0] + sizeTile[0];
        indexTile[1] = extractIndex[1] + sizeTile[1];
        m_ResampleVectorImage->TransformIndexToPhysicalPoint(indexTile, inputPoint);
        outputPoint = m_Transform->TransformPoint(inputPoint);
        OutputPointType lowerRightCorner = outputPoint;

        // Compute upper right corner
        indexTile[0] = extractIndex[0] + sizeTile[0];
        indexTile[1] = extractIndex[1];
        m_ResampleVectorImage->TransformIndexToPhysicalPoint(indexTile, inputPoint);
        outputPoint = m_Transform->TransformPoint(inputPoint);
        OutputPointType upperRightCorner = outputPoint;

        // Compute upper left corner
        indexTile[0] = extractIndex[0];
        indexTile[1] = extractIndex[1];
        m_ResampleVectorImage->TransformIndexToPhysicalPoint(indexTile, inputPoint);
        outputPoint = m_Transform->TransformPoint(inputPoint);
        OutputPointType upperLeftCorner = outputPoint;

        /** END GX LAT LON */

        // Create KML - Filename - PathName - tile number - North - South - East - West
        if (sampleRatioValue == 1)
          {
          if (!gExtended->value()) // Extended format
            this->GenerateKML(m_Path, depth, x, y, north, south, east, west);
          else this->GenerateKMLExtended(
              m_Path, depth, x, y, lowerLeftCorner, lowerRightCorner, upperRightCorner, upperLeftCorner);
          }
        else
          {
          // Search tiles to link
          int tileXStart = extractIndex[0] / (m_TileSize / 2);
          int tileYStart = extractIndex[1] / (m_TileSize / 2);

          // Create KML with link
          if (!gExtended->value())
            {
            this->GenerateKMLWithLink(m_Path, depth, x, y, tileXStart, tileYStart,
                                      north, south, east, west, centerLong, centerLat);
            }
          else
            {
            this->GenerateKMLExtendedWithLink(
              m_Path, depth, x, y, tileXStart, tileYStart,
              lowerLeftCorner, lowerRightCorner, upperRightCorner, upperLeftCorner,
              centerLong, centerLat);
            }
          }

        if (depth == 0)
          {
          // Add the headers and the basic stuffs in the kml only once.
          if (curIdx == 0)
            {
            this->RootKmlProcess(north, south, east, west);
            }

          // Add the bounding box kml
          this->BoundingBoxKmlProcess(north, south, east, west);
          }

        // Add the files to the kmz file
        // Relative path to the root directory  in the kmz file
        std::ostringstream jpg_in_kmz, kml_in_kmz;
        jpg_in_kmz << m_CurrentImageName << "/" << depth << "/" << x << "/" << y << ".jpg";
        kml_in_kmz << m_CurrentImageName << "/" << depth << "/" << x << "/" << y << m_KmlExtension;

        // Absolute path where are stored the files
        std::ostringstream jpg_absolute_path, kml_absolute_path;
        jpg_absolute_path << m_Path << "/" << y << ".jpg";
        kml_absolute_path << m_Path << "/" << y << m_KmlExtension;

        //Add the files to the kmz
        this->AddFileToKMZ(jpg_absolute_path, jpg_in_kmz);
        this->AddFileToKMZ(kml_absolute_path, kml_in_kmz);

        // Remove the unecessary files
        if( itksys::SystemTools::FileExists(kml_absolute_path.str().c_str()) == true &&
            itksys::SystemTools::FileExists(jpg_absolute_path.str().c_str()) == true )
          {
            if ( (itksys::SystemTools::RemoveFile(kml_absolute_path.str().c_str())==false)
                 ||  (itksys::SystemTools::RemoveFile(jpg_absolute_path.str().c_str()) == false) )
              {
          itkExceptionMacro(
                            << "Error while deleting the file" << kml_absolute_path.str() << "or file " << jpg_absolute_path.str());
              }
          }
        else
          {
            //TODO : remove log when bug closed
            std::cout<<"LOG: Impossible to remove kml and jpeg files, at least, one of them doesn't exist... "<<kml_absolute_path.str()<<"  "<<jpg_absolute_path.str()<<std::endl;
          }

        // Progress bar
        float progressValue = pBar->value() + 1;
        pBar->value(progressValue);

        std::ostringstream labelValue;
        labelValue << progressValue;
        labelValue << " / ";
        labelValue << nbTile;

        pBar->copy_label(labelValue.str().c_str());

        Fl::check();

        y++;
        }
      x++;
      y = 0;
      }
    }
}

/**
 */
std::string
TileExportModule::GetCuttenFileName(std::string description, unsigned int idx)
{
  std::string currentImageName;
  std::string tempName;

  std::ostringstream oss;
  oss << "tiles_" << idx;
  tempName = oss.str();

  // Replace all the blanks in the string
  unsigned int i = 0;
  while (i < tempName.length())
    {
    if (tempName[i] != ' ') currentImageName += tempName[i];
    i++;
    }

  return currentImageName;
}

/**
*
*/
int
TileExportModule::AddFileToKMZ(std::ostringstream& absolutePath, std::ostringstream& kmz_in_path)
{
  std::string absolute = absolutePath.str();
  std::string relative = kmz_in_path.str();
  std::string file_data;
  int         errs = 0;
  if (!kmlbase::File::ReadFileToString(absolute, &file_data))
    {
    itkExceptionMacro(<< "Error while reading file " << absolute);
    }
  else
    {
    m_KmzFile->AddFile(file_data, relative);
    }

  return errs;
}

/**
 * Actually the root kml is not fully generated :
 * It generates only the part till the network link
 * In case of multiple inputs, other network liks are
 * needed. See the method Add NetworkLinkToRootKML.
 */
void
TileExportModule::GenerateKMLRoot(std::string title,
                                  double north,
                                  double south,
                                  double east,
                                  double west,
                                  bool extended)
{
  // Give a name to the root file
  std::ostringstream kmlname;
  kmlname << m_Path;
  kmlname << "/";
  kmlname << m_FileName;
  kmlname << m_KmlExtension;
  m_RootKmlFile.open(kmlname.str().c_str());
  m_RootKmlFile << std::fixed << std::setprecision(6);

  m_RootKmlFile << "<?xml version=\"1.0\" encoding=\"utf-8\"?>" << std::endl;
  m_RootKmlFile << "<kml xmlns=\"http://www.opengis.net/kml/2.2\"" << std::endl;
  m_RootKmlFile << " xmlns:gx=\"http://www.google.com/kml/ext/2.2\">" << std::endl;
  m_RootKmlFile << "\t<Document>" << std::endl;
  m_RootKmlFile << "\t\t<name>" << title << "</name>" << std::endl;
  m_RootKmlFile << "\t\t<description></description>" << std::endl;
  m_RootKmlFile << "\t\t<LookAt>" << std::endl;
  m_RootKmlFile << "\t\t\t<longitude>" << (east + west) / 2. << "</longitude>" << std::endl;
  m_RootKmlFile << "\t\t\t<latitude>" << (south + north) / 2. << "</latitude>" << std::endl;
  m_RootKmlFile << "\t\t\t<altitude>35000</altitude>" << std::endl;
  m_RootKmlFile << "\t\t\t<range>35000</range>" << std::endl;
  m_RootKmlFile << "\t\t</LookAt>" << std::endl;
}

/**
 * Close the root kml
 */
void
TileExportModule::
CloseRootKML()
{
  if (m_HasLogo)
    {
    RegionType logoReg = m_Logo->GetLargestPossibleRegion();
    SizeType   logoSize = logoReg.GetSize();
    double     lx = static_cast<double>(logoSize[0]);
    double     ly = static_cast<double>(logoSize[1]);
    int        sizey = 150/4;
    int        sizex = static_cast<int>((lx / ly * sizey));

    /** LOGO **/
    m_RootKmlFile << "\t\t<ScreenOverlay>" << std::endl;
    m_RootKmlFile << "\t\t\t<Icon>" << std::endl;
    m_RootKmlFile << "\t\t\t\t<href>logo.jpeg</href>" << std::endl;
    m_RootKmlFile << "\t\t\t</Icon>" << std::endl;
    m_RootKmlFile << "\t\t\t<name>Logo</name>" << std::endl;
    m_RootKmlFile << "\t\t\t<overlayXY x=\"1\" y=\"1\" xunits=\"fraction\" yunits=\"fraction\"/>" << std::endl;
    m_RootKmlFile << "\t\t\t<screenXY x=\"1\" y=\"1\" xunits=\"fraction\" yunits=\"fraction\"/>" << std::endl;
    m_RootKmlFile << "\t\t\t<size x=\"" << sizex << "\" y=\"" << sizey <<
    "\" xunits=\"pixels\" yunits=\"pixels\"/> " << std::endl;
    m_RootKmlFile << "\t\t</ScreenOverlay>" << std::endl;

    /** LOGO **/
    }

  m_RootKmlFile << "\t</Document>" << std::endl;
  m_RootKmlFile << "</kml>" << std::endl;

  m_RootKmlFile.close();
}

/**
 * Add A networkLink to the root kml
 *
 */
void
TileExportModule::
AddNetworkLinkToRootKML(double north,
                        double south,
                        double east,
                        double west,
                        std::string directory,
                        bool addRegion,
                        unsigned int pos)
{
  m_RootKmlFile << "\t\t<Document>" << std::endl;
  m_RootKmlFile << "\t\t\t<name>" << m_ProductVector[pos].m_Name << "</name>" << std::endl;
  m_RootKmlFile << "\t\t\t<Description>" << m_ProductVector[pos].m_Description << "</Description>" << std::endl;
  m_RootKmlFile << "\t\t\t<LookAt>" << std::endl;
  m_RootKmlFile << "\t\t\t\t<longitude>" << (east + west) / 2. << "</longitude>" << std::endl;
  m_RootKmlFile << "\t\t\t\t<latitude>" << (south + north) / 2. << "</latitude>" << std::endl;
  m_RootKmlFile << "\t\t\t\t<altitude>35000</altitude>" << std::endl;
  m_RootKmlFile << "\t\t\t\t<range>35000</range>" << std::endl;
  m_RootKmlFile << "\t\t\t</LookAt>" << std::endl;

  // Georeferenced Products
  // Add network link
  // If not geo add a ground Overlay with image
  // as an icon
  if (m_InputHaveMetaData)
    {
    m_RootKmlFile << "\t\t\t<NetworkLink>" << std::endl;
    m_RootKmlFile << "\t\t\t\t<name>" << m_ProductVector[pos].m_Name << "</name>" <<  std::endl;
    m_RootKmlFile << "\t\t\t\t<open>1</open>" << std::endl;
    m_RootKmlFile << "\t\t\t<Style>" << std::endl;
    m_RootKmlFile << "\t\t\t\t<ListStyle id=\"hideChildren\">" << std::endl;
    m_RootKmlFile << "\t\t\t\t\t<listItemType>checkHideChildren</listItemType>" << std::endl;
    m_RootKmlFile << "\t\t\t\t</ListStyle>" << std::endl;
    m_RootKmlFile << "\t\t\t</Style>" << std::endl;
    if (addRegion)
      {
      m_RootKmlFile << "\t\t\t\t<Region>" << std::endl;
      m_RootKmlFile << "\t\t\t\t\t<Lod>" << std::endl;
      m_RootKmlFile << "\t\t\t\t\t\t<minLodPixels>" << m_TileSize / 2 << "</minLodPixels>" << std::endl;
      /** If the last depth, put level of detail LOD to infinite (ie -1)*/
      if (m_CurrentDepth == m_MaxDepth) m_RootKmlFile << "\t\t\t\t\t\t<maxLodPixels>-1</maxLodPixels>" << std::endl;
      else m_RootKmlFile << "\t\t\t\t\t\t<maxLodPixels>" << m_TileSize * 4 << "</maxLodPixels>" << std::endl;
      m_RootKmlFile << "\t\t\t\t\t</Lod>" << std::endl;
      m_RootKmlFile << "\t\t\t\t\t<LatLonAltBox>" << std::endl;
      m_RootKmlFile << "\t\t\t\t\t\t<north>" << north << "</north>" << std::endl;
      m_RootKmlFile << "\t\t\t\t\t\t<south>" << south << "</south>" << std::endl;
      m_RootKmlFile << "\t\t\t\t\t\t<east>" << east << "</east>" << std::endl;
      m_RootKmlFile << "\t\t\t\t\t\t<west>" << west << "</west>" << std::endl;
      m_RootKmlFile << "\t\t\t\t\t</LatLonAltBox>" << std::endl;
      m_RootKmlFile << "\t\t\t\t</Region>" << std::endl;
      }

    m_RootKmlFile << "\t\t\t\t<Link>" << std::endl;
    m_RootKmlFile << "\t\t\t\t\t<href>" << directory << "/0/0/0" << m_KmlExtension << "</href>" << std::endl;
    m_RootKmlFile << "\t\t\t\t\t<viewRefreshMode>onRegion</viewRefreshMode>" << std::endl;
    m_RootKmlFile << "\t\t\t\t</Link>" << std::endl;
    m_RootKmlFile << "\t\t\t</NetworkLink>" << std::endl;
    }
  else
    {
    m_RootKmlFile << "\t\t\t<GroundOverlay>" << std::endl;
    m_RootKmlFile << "\t\t\t\t<name>" << m_ProductVector[pos].m_Name << "</name>" << std::endl;
    m_RootKmlFile << "\t\t\t\t<Icon>" << std::endl;
    m_RootKmlFile << "\t\t\t\t\t<href>" << directory << "/0.jpg" << "</href>" << std::endl;
    m_RootKmlFile << "\t\t\t\t</Icon>" << std::endl;

    if (!gExtended->value())
      {
      m_RootKmlFile << "\t\t\t\t<LatLonBox>" << std::endl;
      m_RootKmlFile << "\t\t\t\t\t<north>" << north << "</north>" << std::endl;
      m_RootKmlFile << "\t\t\t\t\t<south>" << south << "</south>" << std::endl;
      m_RootKmlFile << "\t\t\t\t\t<east>" << east << "</east>" << std::endl;
      m_RootKmlFile << "\t\t\t\t\t<west>" << west << "</west>" << std::endl;
      m_RootKmlFile << "\t\t\t\t</LatLonBox>" << std::endl;
      }
    else
      {
      m_RootKmlFile << "\t\t\t<gx:LatLonQuad>" << std::endl;
      m_RootKmlFile << "\t\t\t\t<coordinates>" << std::endl;
      m_RootKmlFile << " " << m_LowerLeftCorner[0]  << "," << m_LowerLeftCorner[1];
      m_RootKmlFile << " " << m_LowerRightCorner[0] << "," << m_LowerRightCorner[1];
      m_RootKmlFile << " " << m_UpperRightCorner[0] << "," << m_UpperRightCorner[1];
      m_RootKmlFile << " " << m_UpperLeftCorner[0]  << "," << m_UpperRightCorner[1]  << std::endl;
      m_RootKmlFile << "\t\t\t\t</coordinates>" << std::endl;
      m_RootKmlFile << "\t\t\t</gx:LatLonQuad>" << std::endl;
      }
    m_RootKmlFile << "\t\t\t</GroundOverlay>" << std::endl;
    }

  m_RootKmlFile << "\t\t\t<Folder>" << std::endl;
  m_RootKmlFile << "\t\t\t\t<name>The bounding box </name>" << std::endl;
  m_RootKmlFile << "\t\t\t\t<Style>" << std::endl;
  m_RootKmlFile << "\t\t\t\t\t<ListStyle id=\"hideChildren\">" << std::endl;
  m_RootKmlFile << "\t\t\t\t\t\t<listItemType>checkHideChildren</listItemType>" << std::endl;
  m_RootKmlFile << "\t\t\t\t\t</ListStyle>" << std::endl;
  m_RootKmlFile << "\t\t\t\t</Style>" << std::endl;
  m_RootKmlFile << "\t\t\t\t<NetworkLink>" << std::endl;
  m_RootKmlFile << "\t\t\t\t<Region>" << std::endl;
  m_RootKmlFile << "\t\t\t\t\t<Lod>" << std::endl;
  m_RootKmlFile << "\t\t\t\t\t\t<minLodPixels>" << m_TileSize / 2 << "</minLodPixels>" << std::endl;
  m_RootKmlFile << "\t\t\t\t\t\t<maxLodPixels>-1</maxLodPixels>" << std::endl;
  m_RootKmlFile << "\t\t\t\t\t</Lod>" << std::endl;
  m_RootKmlFile << "\t\t\t\t</Region>" << std::endl;
  m_RootKmlFile << "\t\t\t\t<Link>" << std::endl;
  m_RootKmlFile << "\t\t\t\t\t<href>bounds/bound_" << pos << m_KmlExtension << "</href>" << std::endl;
  m_RootKmlFile << "\t\t\t\t\t<viewRefreshMode>onRegion</viewRefreshMode>" << std::endl;
  m_RootKmlFile << "\t\t\t\t</Link>" << std::endl;
  m_RootKmlFile << "\t\t\t\t</NetworkLink>" << std::endl;
  m_RootKmlFile << "\t\t\t</Folder>" << std::endl;

  // Add a placemark with the images used as legend
  // If any
  this->AddLegendToRootKml(north, south, east, west, pos);

  m_RootKmlFile << "\t\t</Document>" << std::endl;
}

/**
 * Add a legend associated to the current product
 *
 */
void
TileExportModule::
AddLegendToRootKml(double north, double south, double east, double west, unsigned int pos)
{

  double lat = (north + south) / 2.;
  double lon = (east + west) / 2.;

  if (m_ProductVector[pos].m_AssociatedLegends.size() > 0)
    {
    m_RootKmlFile << "\t\t\t\t<Placemark>" << std::endl;
    m_RootKmlFile << "\t\t\t\t\t<name>Legend</name>" << std::endl;
    m_RootKmlFile << "\t\t\t\t\t<description>" << std::endl;
    m_RootKmlFile << "\t\t\t\t\t\t<![CDATA[ Legend of the product " << m_ProductVector[pos].m_Name << std::endl;
    for (unsigned int i = 0; i < m_ProductVector[pos].m_AssociatedLegends.size(); i++)
      m_RootKmlFile << "\t\t\t\t\t\t<img src=\"legends/legend_" << pos <<
      m_ProductVector[pos].m_AssociatedLegends[i] << ".jpeg\" width=\"215\" height=\"175\"  >";
    m_RootKmlFile << "\t\t\t\t\t\t ]]>" << std::endl;

    m_RootKmlFile << "\t\t\t\t\t</description>" << std::endl;
    m_RootKmlFile << "\t\t\t\t\t<Point>" << std::endl;
    m_RootKmlFile << "\t\t\t\t\t\t<coordinates>" << lon << "," << lat << "</coordinates>" << std::endl;
    m_RootKmlFile << "\t\t\t\t\t</Point>" << std::endl;
    m_RootKmlFile << "\t\t\t\t</Placemark>" << std::endl;
    }
}

void
TileExportModule::
GenerateBoundingKML(double north, double south, double east, double west)
{
  std::ostringstream kmlname;
  kmlname << m_Path;
  kmlname << "/";
  kmlname << "bound_" << m_CurrentProduct << m_KmlExtension;
  std::ofstream fileTest(kmlname.str().c_str());

  fileTest << std::fixed << std::setprecision(6);

  fileTest << "<?xml version=\"1.0\" encoding=\"utf-8\"?>" << std::endl;
  fileTest << "<kml xmlns=\"http://www.opengis.net/kml/2.2\"" << std::endl;
  fileTest << " xmlns:gx=\"http://www.google.com/kml/ext/2.2\">" << std::endl;

  fileTest << "\t<Document>" << std::endl;
  fileTest << "\t\t<name> Bounding box of the  product " << m_ProductVector[m_CurrentProduct].m_Name << "</name>" <<
  std::endl;
  fileTest << "\t\t<open>1</open>" << std::endl;
  fileTest << "\t\t<Placemark>" << std::endl;
  fileTest << "\t\t\t<description>The bounding Box of the image</description>" << std::endl;
  fileTest << "\t\t<LineString>" << std::endl;
  fileTest << "\t\t\t<extrude>0</extrude>" << std::endl;
  fileTest << "\t\t\t<tessellate>1</tessellate>" << std::endl;
  fileTest << "\t\t\t<altitudeMode>clampedToGround</altitudeMode>" << std::endl;
  fileTest << "\t\t\t<coordinates>" << std::endl;

  if (m_InputHaveMetaData)
    {
    fileTest << "\t\t\t\t\t" <<  west << "," << north << std::endl;
    fileTest << "\t\t\t\t\t" <<  east << "," << north << std::endl;
    fileTest << "\t\t\t\t\t" <<  east << "," << south << std::endl;
    fileTest << "\t\t\t\t\t" <<  west << "," << south << std::endl;
    fileTest << "\t\t\t\t\t" <<  west << "," << north << std::endl;
    }
  else
    {
    if (!gExtended->value())
      {
      fileTest << "\t\t\t\t\t" <<  west << "," << north << std::endl;
      fileTest << "\t\t\t\t\t" <<  east << "," << north << std::endl;
      fileTest << "\t\t\t\t\t" <<  east << "," << south << std::endl;
      fileTest << "\t\t\t\t\t" <<  west << "," << south << std::endl;
      fileTest << "\t\t\t\t\t" <<  west << "," << north << std::endl;
      }
    else
      {
      fileTest << "\t\t\t\t\t" << m_LowerLeftCorner[0]  << "," << m_LowerLeftCorner[1] << std::endl;
      fileTest << "\t\t\t\t\t" << m_LowerRightCorner[0] << "," << m_LowerRightCorner[1] << std::endl;
      fileTest << "\t\t\t\t\t" << m_UpperRightCorner[0] << "," << m_UpperRightCorner[1] << std::endl;
      fileTest << "\t\t\t\t\t" << m_UpperLeftCorner[0]  << "," << m_UpperRightCorner[1]  << std::endl;
      fileTest << "\t\t\t\t\t" << m_LowerLeftCorner[0]  << "," << m_LowerLeftCorner[1] << std::endl;
      }
    }
  fileTest << "\t\t\t</coordinates>" << std::endl;
  fileTest << "\t\t</LineString>" << std::endl;
  fileTest << "\t\t</Placemark>" << std::endl;

  fileTest << "\t</Document>" << std::endl;
  fileTest << "</kml>" << std::endl;

  fileTest.close();
}

void
TileExportModule::
GenerateKML(std::string pathname, int depth, int x, int y, double north, double south, double east, double west)
{
  std::ostringstream kmlname;
  kmlname << pathname;
  kmlname << "/";
  kmlname << y << ".kml";
  std::ofstream fileTest(kmlname.str().c_str());

  fileTest << std::fixed << std::setprecision(6);

  fileTest << "<?xml version=\"1.0\" encoding=\"utf-8\"?>" << std::endl;
  fileTest << "<kml xmlns=\"http://www.opengis.net/kml/2.2\"" << std::endl;
  fileTest << " xmlns:gx=\"http://www.google.com/kml/ext/2.2\">" << std::endl;
  fileTest << "\t<Document>" << std::endl;
  fileTest << "\t\t<name>" << y << ".kml</name>" << std::endl;
  fileTest << "\t\t<Region>" << std::endl;
  fileTest << "\t\t\t<Lod>" << std::endl;
  fileTest << "\t\t\t\t<minLodPixels>" << m_TileSize / 2 << "</minLodPixels>" << std::endl;
  if (m_CurrentDepth == m_MaxDepth) fileTest << "\t\t\t\t<maxLodPixels>-1</maxLodPixels>" << std::endl;
  else fileTest << "\t\t\t\t<maxLodPixels>" << m_TileSize * 4 << "</maxLodPixels>" << std::endl;
  fileTest << "\t\t\t</Lod>" << std::endl;
  fileTest << "\t\t\t<LatLonAltBox>" << std::endl;
  fileTest << "\t\t\t\t<north>" << north << "</north>" << std::endl;
  fileTest << "\t\t\t\t<south>" << south << "</south>" << std::endl;
  fileTest << "\t\t\t\t<east>" << east << "</east>" << std::endl;
  fileTest << "\t\t\t\t<west>" << west << "</west>" << std::endl;
  fileTest << "\t\t\t</LatLonAltBox>" << std::endl;
  fileTest << "\t\t</Region>" << std::endl;
  fileTest << "\t\t<GroundOverlay>" << std::endl;
  fileTest << "\t\t\t<drawOrder>" << depth + (m_CurIdx * m_MaxDepth) << "</drawOrder>" << std::endl;
  fileTest << "\t\t\t<Icon>" << std::endl;
  fileTest << "\t\t\t\t<href>" << y << ".jpg" << "</href>" << std::endl;
  fileTest << "\t\t\t</Icon>" << std::endl;
  fileTest << "\t\t\t<LatLonAltBox>" << std::endl;
  fileTest << "\t\t\t\t<north>" << north << "</north>" << std::endl;
  fileTest << "\t\t\t\t<south>" << south << "</south>" << std::endl;
  fileTest << "\t\t\t\t<east>" << east << "</east>" << std::endl;
  fileTest << "\t\t\t\t<west>" << west << "</west>" << std::endl;
  fileTest << "\t\t\t</LatLonAltBox>" << std::endl;
  fileTest << "\t\t</GroundOverlay>" << std::endl;
  fileTest << "\t</Document>" << std::endl;
  fileTest << "</kml>" << std::endl;

  fileTest.close();
}

void
TileExportModule::
GenerateKMLWithLink(std::string pathname,
                    int depth, int x, int y, int tileStartX, int tileStartY,
                    double north, double south, double east, double west, double centerLong, double centerLat)
{
  std::ostringstream kmlname;
  kmlname << pathname;
  kmlname << "/";
  kmlname << y << ".kml";
  std::ofstream fileTest(kmlname.str().c_str());

  fileTest << std::fixed << std::setprecision(6);

  fileTest << "<?xml version=\"1.0\" encoding=\"utf-8\"?>" << std::endl;
  fileTest << "<kml xmlns=\"http://www.opengis.net/kml/2.2\"" << std::endl;
  fileTest << " xmlns:gx=\"http://www.google.com/kml/ext/2.2\">" << std::endl;
  fileTest << "\t<Document>" << std::endl;
  fileTest << "\t\t<name>" << y << ".kml</name>" << std::endl;
  fileTest << "\t\t<Region>" << std::endl;
  fileTest << "\t\t\t<Lod>" << std::endl;
  fileTest << "\t\t\t\t<minLodPixels>" << m_TileSize / 2 << "</minLodPixels>" << std::endl;
  if (m_CurrentDepth == m_MaxDepth) fileTest << "\t\t\t\t<maxLodPixels>-1</maxLodPixels>" << std::endl;
  else fileTest << "\t\t\t\t<maxLodPixels>" << m_TileSize * 2 << "</maxLodPixels>" << std::endl;
  fileTest << "\t\t\t</Lod>" << std::endl;
  fileTest << "\t\t\t<LatLonAltBox>" << std::endl;
  fileTest << "\t\t\t\t<north>" << north << "</north>" << std::endl;
  fileTest << "\t\t\t\t<south>" << south << "</south>" << std::endl;
  fileTest << "\t\t\t\t<east>" << east << "</east>" << std::endl;
  fileTest << "\t\t\t\t<west>" << west << "</west>" << std::endl;
  fileTest << "\t\t\t</LatLonAltBox>" << std::endl;
  fileTest << "\t\t</Region>" << std::endl;
  fileTest << "\t\t<GroundOverlay>" << std::endl;
  fileTest << "\t\t\t<drawOrder>" << depth + (m_CurIdx * m_MaxDepth) << "</drawOrder>" << std::endl;
  fileTest << "\t\t\t<Icon>" << std::endl;
  fileTest << "\t\t\t\t<href>" << y << ".jpg" << "</href>" << std::endl;
  fileTest << "\t\t\t</Icon>" << std::endl;
  fileTest << "\t\t\t<LatLonAltBox>" << std::endl;
  fileTest << "\t\t\t\t<north>" << north << "</north>" << std::endl;
  fileTest << "\t\t\t\t<south>" << south << "</south>" << std::endl;
  fileTest << "\t\t\t\t<east>" << east << "</east>" << std::endl;
  fileTest << "\t\t\t\t<west>" << west << "</west>" << std::endl;
  fileTest << "\t\t\t</LatLonAltBox>" << std::endl;
  fileTest << "\t\t</GroundOverlay>" << std::endl;

  /* Sous tuile 1 */
  std::ostringstream fileTile1;
  fileTile1 << "../../";
  fileTile1 << depth + 1;
  fileTile1 << "/";
  fileTile1 << tileStartX;
  fileTile1 << "/";
  fileTile1 << tileStartY;

  fileTest << "\t\t<NetworkLink>" << std::endl;
  fileTest << "\t\t\t<name>" << fileTile1.str() << ".jpg</name>" << std::endl;
  fileTest << "\t\t\t<Region>" << std::endl;
  fileTest << "\t\t\t\t<Lod>" << std::endl;
  fileTest << "\t\t\t\t\t<minLodPixels>" << m_TileSize / 2 << "</minLodPixels>" << std::endl;
  if (m_CurrentDepth == m_MaxDepth) fileTest << "\t\t\t\t<maxLodPixels>-1</maxLodPixels>" << std::endl;
  else fileTest << "\t\t\t\t<maxLodPixels>" << m_TileSize * 2 << "</maxLodPixels>" << std::endl;
  fileTest << "\t\t\t\t</Lod>" << std::endl;
  fileTest << "\t\t\t\t<LatLonAltBox>" << std::endl;
  fileTest << "\t\t\t\t\t<north>" << north << "</north>" << std::endl;
  fileTest << "\t\t\t\t\t<south>" << centerLat << "</south>" << std::endl;
  fileTest << "\t\t\t\t\t<east>" << centerLong << "</east>" << std::endl;
  fileTest << "\t\t\t\t\t<west>" << west << "</west>" << std::endl;
  fileTest << "\t\t\t\t</LatLonAltBox>" << std::endl;
  fileTest << "\t\t\t</Region>" << std::endl;
  fileTest << "\t\t\t<Link>" << std::endl;
  fileTest << "\t\t\t\t<href>" << fileTile1.str() << ".kml</href>" << std::endl;
  fileTest << "\t\t\t\t<viewRefreshMode>onRegion</viewRefreshMode>" << std::endl;
  fileTest << "\t\t\t\t<viewFormat/>" << std::endl;
  fileTest << "\t\t\t</Link>" << std::endl;
  fileTest << "\t\t</NetworkLink>" << std::endl;

  /* Sous tuile 2 */
  std::ostringstream fileTile2;
  fileTile2 << "../../";
  fileTile2 << depth + 1;
  fileTile2 << "/";
  fileTile2 << tileStartX + 1;
  fileTile2 << "/";
  fileTile2 << tileStartY;

  fileTest << "\t\t<NetworkLink>" << std::endl;
  fileTest << "\t\t\t<name>" << fileTile2.str() << ".jpg</name>" << std::endl;
  fileTest << "\t\t\t<Region>" << std::endl;
  fileTest << "\t\t\t\t<Lod>" << std::endl;
  fileTest << "\t\t\t\t\t<minLodPixels>" << m_TileSize / 2 << "</minLodPixels>" << std::endl;
  if (m_CurrentDepth == m_MaxDepth) fileTest << "\t\t\t\t<maxLodPixels>-1</maxLodPixels>" << std::endl;
  else fileTest << "\t\t\t\t<maxLodPixels>" << m_TileSize * 4 << "</maxLodPixels>" << std::endl;
  fileTest << "\t\t\t\t</Lod>" << std::endl;
  fileTest << "\t\t\t\t<LatLonAltBox>" << std::endl;
  fileTest << "\t\t\t\t\t<north>" << north << "</north>" << std::endl;
  fileTest << "\t\t\t\t\t<south>" << centerLat << "</south>" << std::endl;
  fileTest << "\t\t\t\t\t<east>" << east << "</east>" << std::endl;
  fileTest << "\t\t\t\t\t<west>" << centerLong << "</west>" << std::endl;
  fileTest << "\t\t\t\t</LatLonAltBox>" << std::endl;
  fileTest << "\t\t\t</Region>" << std::endl;
  fileTest << "\t\t\t<Link>" << std::endl;
  fileTest << "\t\t\t\t<href>" << fileTile2.str() << ".kml</href>" << std::endl;
  fileTest << "\t\t\t\t<viewRefreshMode>onRegion</viewRefreshMode>" << std::endl;
  fileTest << "\t\t\t\t<viewFormat/>" << std::endl;
  fileTest << "\t\t\t</Link>" << std::endl;
  fileTest << "\t\t</NetworkLink>" << std::endl;

  /* Sous tuile 3 */
  std::ostringstream fileTile3;
  fileTile3 << "../../";
  fileTile3 << depth + 1;
  fileTile3 << "/";
  fileTile3 << tileStartX + 1;
  fileTile3 << "/";
  fileTile3 << tileStartY + 1;

  fileTest << "\t\t<NetworkLink>" << std::endl;
  fileTest << "\t\t\t<name>" << fileTile3.str() << ".jpg</name>" << std::endl;
  fileTest << "\t\t\t<Region>" << std::endl;
  fileTest << "\t\t\t\t<Lod>" << std::endl;
  fileTest << "\t\t\t\t\t<minLodPixels>" << m_TileSize / 2 << "</minLodPixels>" << std::endl;
  if (m_CurrentDepth == m_MaxDepth) fileTest << "\t\t\t\t<maxLodPixels>-1</maxLodPixels>" << std::endl;
  else fileTest << "\t\t\t\t<maxLodPixels>" << m_TileSize * 4 << "</maxLodPixels>" << std::endl;
  fileTest << "\t\t\t\t</Lod>" << std::endl;
  fileTest << "\t\t\t\t<LatLonAltBox>" << std::endl;
  fileTest << "\t\t\t\t\t<north>" << centerLat << "</north>" << std::endl;
  fileTest << "\t\t\t\t\t<south>" << south << "</south>" << std::endl;
  fileTest << "\t\t\t\t\t<east>" << east << "</east>" << std::endl;
  fileTest << "\t\t\t\t\t<west>" << centerLong << "</west>" << std::endl;
  fileTest << "\t\t\t\t</LatLonAltBox>" << std::endl;
  fileTest << "\t\t\t</Region>" << std::endl;
  fileTest << "\t\t\t<Link>" << std::endl;
  fileTest << "\t\t\t\t<href>" << fileTile3.str() << ".kml</href>" << std::endl;
  fileTest << "\t\t\t\t<viewRefreshMode>onRegion</viewRefreshMode>" << std::endl;
  fileTest << "\t\t\t\t<viewFormat/>" << std::endl;
  fileTest << "\t\t\t</Link>" << std::endl;
  fileTest << "\t\t</NetworkLink>" << std::endl;

  /* Sous tuile 4 */
  std::ostringstream fileTile4;
  fileTile4 << "../../";
  fileTile4 << depth + 1;
  fileTile4 << "/";
  fileTile4 << tileStartX;
  fileTile4 << "/";
  fileTile4 << tileStartY + 1;

  fileTest << "\t\t<NetworkLink>" << std::endl;
  fileTest << "\t\t\t<name>" << fileTile4.str() << ".jpg</name>" << std::endl;
  fileTest << "\t\t\t<Region>" << std::endl;
  fileTest << "\t\t\t\t<Lod>" << std::endl;
  fileTest << "\t\t\t\t\t<minLodPixels>" << m_TileSize / 2 << "</minLodPixels>" << std::endl;
  if (m_CurrentDepth == m_MaxDepth) fileTest << "\t\t\t\t<maxLodPixels>-1</maxLodPixels>" << std::endl;
  else fileTest << "\t\t\t\t<maxLodPixels>" << m_TileSize * 4 << "</maxLodPixels>" << std::endl;

  fileTest << "\t\t\t\t</Lod>" << std::endl;
  fileTest << "\t\t\t\t<LatLonAltBox>" << std::endl;
  fileTest << "\t\t\t\t\t<north>" << centerLat << "</north>" << std::endl;
  fileTest << "\t\t\t\t\t<south>" << south << "</south>" << std::endl;
  fileTest << "\t\t\t\t\t<east>" << centerLong << "</east>" << std::endl;
  fileTest << "\t\t\t\t\t<west>" << west << "</west>" << std::endl;
  fileTest << "\t\t\t\t</LatLonAltBox>" << std::endl;
  fileTest << "\t\t\t</Region>" << std::endl;
  fileTest << "\t\t\t<Link>" << std::endl;
  fileTest << "\t\t\t\t<href>" << fileTile4.str() << ".kml</href>" << std::endl;
  fileTest << "\t\t\t\t<viewRefreshMode>onRegion</viewRefreshMode>" << std::endl;
  fileTest << "\t\t\t\t<viewFormat/>" << std::endl;
  fileTest << "\t\t\t</Link>" << std::endl;
  fileTest << "\t\t</NetworkLink>" << std::endl;

  fileTest << "\t</Document>" << std::endl;
  fileTest << "</kml>" << std::endl;
  fileTest.close();

}

void
TileExportModule::
GenerateKMLExtended(std::string pathname, int depth, int x, int y,
                    OutputPointType lowerLeft, OutputPointType lowerRight,
                    OutputPointType upperRight, OutputPointType upperLeft)
{
  std::ostringstream kmlname;
  kmlname << pathname;
  kmlname << "/";
  kmlname << y << "xt.kml";
  std::ofstream fileTest(kmlname.str().c_str());

  fileTest << std::fixed << std::setprecision(6);

  fileTest << "<?xml version=\"1.0\" encoding=\"utf-8\"?>" << std::endl;
  fileTest << "<kml xmlns=\"http://www.opengis.net/kml/2.2\"" << std::endl;
  fileTest << " xmlns:gx=\"http://www.google.com/kml/ext/2.2\">" << std::endl;
  fileTest << "\t<Document>" << std::endl;
  fileTest << "\t\t<name>" << y << "xt.kml</name>" << std::endl;
  fileTest << "\t\t<Region>" << std::endl;
  fileTest << "\t\t\t<Lod>" << std::endl;
  fileTest << "\t\t\t\t<minLodPixels>" << m_TileSize / 2 << "</minLodPixels>" << std::endl;
  fileTest << "\t\t\t\t<maxLodPixels>-1</maxLodPixels>" << std::endl;
  fileTest << "\t\t\t</Lod>" << std::endl;
  fileTest << "\t\t\t<gx:LatLonQuad>" << std::endl;
  fileTest << "\t\t\t\t<coordinates>" << std::endl;
  fileTest << "\t\t\t\t\t" << lowerLeft[0]  << "," << lowerLeft[1];
  fileTest << " " << lowerRight[0] << "," << lowerRight[1];
  fileTest << " " << upperRight[0] << "," << upperRight[1];
  fileTest << " " << upperLeft[0]  << "," << upperLeft[1]  << std::endl;
  fileTest << "\t\t\t\t</coordinates>" << std::endl;
  fileTest << "\t\t\t</gx:LatLonQuad>" << std::endl;
  fileTest << "\t\t</Region>" << std::endl;
  fileTest << "\t\t<GroundOverlay>" << std::endl;
  fileTest << "\t\t\t<drawOrder>" << depth + (m_CurIdx * m_MaxDepth) << "</drawOrder>" << std::endl;
  fileTest << "\t\t\t<Icon>" << std::endl;
  fileTest << "\t\t\t\t<href>" << y << ".jpg" << "</href>" << std::endl;
  fileTest << "\t\t\t</Icon>" << std::endl;
  fileTest << "\t\t\t<gx:LatLonQuad>" << std::endl;
  fileTest << "\t\t\t\t<coordinates>" << std::endl;
  fileTest << "\t\t\t\t\t" << lowerLeft[0]  << "," << lowerLeft[1];
  fileTest << " " << lowerRight[0] << "," << lowerRight[1];
  fileTest << " " << upperRight[0] << "," << upperRight[1];
  fileTest << " " << upperLeft[0]  << "," << upperLeft[1]  << std::endl;
  fileTest << "\t\t\t\t</coordinates>" << std::endl;
  fileTest << "\t\t\t</gx:LatLonQuad>" << std::endl;
  fileTest << "\t\t</GroundOverlay>" << std::endl;
  fileTest << "\t</Document>" << std::endl;
  fileTest << "</kml>" << std::endl;

  fileTest.close();
}

void
TileExportModule::
GenerateKMLExtendedWithLink(std::string pathname,
                            int depth, int x, int y, int tileStartX, int tileStartY,
                            OutputPointType lowerLeft, OutputPointType lowerRight,
                            OutputPointType upperRight, OutputPointType upperLeft,
                            double centerLong, double centerLat)
{
  std::ostringstream kmlname;
  kmlname << pathname;
  kmlname << "/";
  kmlname << y << "xt.kml";
  std::ofstream fileTest(kmlname.str().c_str());

  fileTest << std::fixed << std::setprecision(6);

  fileTest << "<?xml version=\"1.0\" encoding=\"utf-8\"?>" << std::endl;
  fileTest << "<kml xmlns=\"http://www.opengis.net/kml/2.2\"" << std::endl;
  fileTest << " xmlns:gx=\"http://www.google.com/kml/ext/2.2\">" << std::endl;
  fileTest << "\t<Document>" << std::endl;
  fileTest << "\t\t<name>" << y << "xt.kml</name>" << std::endl;
  fileTest << "\t\t<Region>" << std::endl;
  fileTest << "\t\t\t<Lod>" << std::endl;
  fileTest << "\t\t\t\t<minLodPixels>" << m_TileSize / 2 << "</minLodPixels>" << std::endl;
  fileTest << "\t\t\t\t<maxLodPixels>-1</maxLodPixels>" << std::endl;
  fileTest << "\t\t\t</Lod>" << std::endl;
  fileTest << "\t\t\t<gx:LatLonQuad>" << std::endl;
  fileTest << "\t\t\t\t<coordinates>" << std::endl;
  fileTest << "\t\t\t\t\t" << lowerLeft[0]  << "," << lowerLeft[1];
  fileTest << " " << lowerRight[0] << "," << lowerRight[1];
  fileTest << " " << upperRight[0] << "," << upperRight[1];
  fileTest << " " << upperLeft[0]  << "," << upperLeft[1]  << std::endl;
  fileTest << "\t\t\t\t</coordinates>" << std::endl;
  fileTest << "\t\t\t</gx:LatLonQuad>" << std::endl;
  fileTest << "\t\t</Region>" << std::endl;
  fileTest << "\t\t<GroundOverlay>" << std::endl;
  fileTest << "\t\t\t<drawOrder>" << depth + (m_CurIdx * m_MaxDepth) << "</drawOrder>" << std::endl;
  fileTest << "\t\t\t<Icon>" << std::endl;
  fileTest << "\t\t\t\t<href>" << y << ".jpg" << "</href>" << std::endl;
  fileTest << "\t\t\t</Icon>" << std::endl;
  fileTest << "\t\t\t<gx:LatLonQuad>" << std::endl;
  fileTest << "\t\t\t\t<coordinates>" << std::endl;
  fileTest << "\t\t\t\t\t" << lowerLeft[0]  << "," << lowerLeft[1];
  fileTest << " " << lowerRight[0] << "," << lowerRight[1];
  fileTest << " " << upperRight[0] << "," << upperRight[1];
  fileTest << " " << upperLeft[0]  << "," << upperLeft[1]  << std::endl;
  fileTest << "\t\t\t\t</coordinates>" << std::endl;
  fileTest << "\t\t\t</gx:LatLonQuad>" << std::endl;
  fileTest << "\t\t</GroundOverlay>" << std::endl;

  /* Sous tuile 1 */
  std::ostringstream fileTile1;
  fileTile1 << "../../";
  fileTile1 << depth + 1;
  fileTile1 << "/";
  fileTile1 << tileStartX;
  fileTile1 << "/";
  fileTile1 << tileStartY;

  fileTest << "\t\t<NetworkLink>" << std::endl;
  fileTest << "\t\t\t<name>" << fileTile1.str() << ".jpg</name>" << std::endl;
  fileTest << "\t\t\t<Region>" << std::endl;
  fileTest << "\t\t\t\t<Lod>" << std::endl;
  fileTest << "\t\t\t\t\t<minLodPixels>" << m_TileSize / 2 << "</minLodPixels>" << std::endl;
  fileTest << "\t\t\t\t\t<maxLodPixels>-1</maxLodPixels>" << std::endl;
  fileTest << "\t\t\t\t</Lod>" << std::endl;
  fileTest << "\t\t\t\t<gx:LatLonQuad>" << std::endl;
  fileTest << "\t\t\t\t\t<coordinates>" << std::endl;
  fileTest << "\t\t\t\t\t\t" << lowerLeft[0]  << "," << centerLat;
  fileTest << " " << centerLong << "," << centerLat;
  fileTest << " " << centerLong << "," << upperLeft[1];
  fileTest << " " << upperLeft[0]  << "," << upperLeft[1]  << std::endl;
  fileTest << "\t\t\t\t\t</coordinates>" << std::endl;
  fileTest << "\t\t\t\t</gx:LatLonQuad>" << std::endl;
  fileTest << "\t\t\t</Region>" << std::endl;
  fileTest << "\t\t\t<Link>" << std::endl;
  fileTest << "\t\t\t\t<href>" << fileTile1.str() << "xt.kml</href>" << std::endl;
  fileTest << "\t\t\t\t<viewRefreshMode>onRegion</viewRefreshMode>" << std::endl;
  fileTest << "\t\t\t\t<viewFormat/>" << std::endl;
  fileTest << "\t\t\t</Link>" << std::endl;
  fileTest << "\t\t</NetworkLink>" << std::endl;

  /* Sous tuile 2 */
  std::ostringstream fileTile2;
  fileTile2 << "../../";
  fileTile2 << depth + 1;
  fileTile2 << "/";
  fileTile2 << tileStartX + 1;
  fileTile2 << "/";
  fileTile2 << tileStartY;

  fileTest << "\t\t<NetworkLink>" << std::endl;
  fileTest << "\t\t\t<name>" << fileTile2.str() << ".jpg</name>" << std::endl;
  fileTest << "\t\t\t<Region>" << std::endl;
  fileTest << "\t\t\t<Lod>" << std::endl;
  fileTest << "\t\t\t\t<minLodPixels>" << m_TileSize / 2 << "</minLodPixels>" << std::endl;
  fileTest << "\t\t\t\t<maxLodPixels>-1</maxLodPixels>" << std::endl;
  fileTest << "\t\t\t</Lod>" << std::endl;
  fileTest << "\t\t\t\t<gx:LatLonQuad>" << std::endl;
  fileTest << "\t\t\t\t\t<coordinates>" << std::endl;
  fileTest << "\t\t\t\t\t\t" << centerLong  << "," << centerLat;
  fileTest << " " << lowerRight[0] << "," << centerLat;
  fileTest << " " << upperRight[0] << "," << upperRight[1];
  fileTest << " " << centerLong << "," << upperLeft[1]  << std::endl;
  fileTest << "\t\t\t\t\t</coordinates>" << std::endl;
  fileTest << "\t\t\t\t</gx:LatLonQuad>" << std::endl;
  fileTest << "\t\t\t</Region>" << std::endl;
  fileTest << "\t\t\t<Link>" << std::endl;
  fileTest << "\t\t\t\t<href>" << fileTile2.str() << "xt.kml</href>" << std::endl;
  fileTest << "\t\t\t\t<viewRefreshMode>onRegion</viewRefreshMode>" << std::endl;
  fileTest << "\t\t\t\t<viewFormat/>" << std::endl;
  fileTest << "\t\t\t</Link>" << std::endl;
  fileTest << "\t\t</NetworkLink>" << std::endl;

  /* Sous tuile 3 */
  std::ostringstream fileTile3;
  fileTile3 << "../../";
  fileTile3 << depth + 1;
  fileTile3 << "/";
  fileTile3 << tileStartX + 1;
  fileTile3 << "/";
  fileTile3 << tileStartY + 1;

  fileTest << "\t\t<NetworkLink>" << std::endl;
  fileTest << "\t\t\t<name>" << fileTile3.str() << ".jpg</name>" << std::endl;
  fileTest << "\t\t\t<Region>" << std::endl;
  fileTest << "\t\t\t\t<Lod>" << std::endl;
  fileTest << "\t\t\t\t\t<minLodPixels>" << m_TileSize / 2 << "</minLodPixels>" << std::endl;
  fileTest << "\t\t\t\t\t<maxLodPixels>-1</maxLodPixels>" << std::endl;
  fileTest << "\t\t\t\t</Lod>" << std::endl;
  fileTest << "\t\t\t\t<gx:LatLonQuad>" << std::endl;
  fileTest << "\t\t\t\t\t<coordinates>" << std::endl;
  fileTest << "\t\t\t\t\t\t" << centerLong  << "," << lowerLeft[1];
  fileTest << " " << lowerRight[0] << "," << lowerRight[1];
  fileTest << " " << upperRight[0] << "," << centerLat;
  fileTest << " " << centerLong  << "," << centerLat  << std::endl;
  fileTest << "\t\t\t\t\t</coordinates>" << std::endl;
  fileTest << "\t\t\t\t</gx:LatLonQuad>" << std::endl;
  fileTest << "\t\t\t</Region>" << std::endl;
  fileTest << "\t\t\t<Link>" << std::endl;
  fileTest << "\t\t\t\t<href>" << fileTile3.str() << "xt.kml</href>" << std::endl;
  fileTest << "\t\t\t\t<viewRefreshMode>onRegion</viewRefreshMode>" << std::endl;
  fileTest << "\t\t\t\t<viewFormat/>" << std::endl;
  fileTest << "\t\t\t</Link>" << std::endl;
  fileTest << "\t\t</NetworkLink>" << std::endl;

  /* Sous tuile 4 */
  std::ostringstream fileTile4;
  fileTile4 << "../../";
  fileTile4 << depth + 1;
  fileTile4 << "/";
  fileTile4 << tileStartX;
  fileTile4 << "/";
  fileTile4 << tileStartY + 1;

  fileTest << "\t\t<NetworkLink>" << std::endl;
  fileTest << "\t\t\t<name>" << fileTile4.str() << ".jpg</name>" << std::endl;
  fileTest << "\t\t\t<Region>" << std::endl;
  fileTest << "\t\t\t\t<Lod>" << std::endl;
  fileTest << "\t\t\t\t\t<minLodPixels>" << m_TileSize / 2 << "</minLodPixels>" << std::endl;
  fileTest << "\t\t\t\t\t<maxLodPixels>-1</maxLodPixels>" << std::endl;
  fileTest << "\t\t\t\t</Lod>" << std::endl;
  fileTest << "\t\t\t\t<gx:LatLonQuad>" << std::endl;
  fileTest << "\t\t\t\t\t<coordinates>" << std::endl;
  fileTest << "\t\t\t\t\t\t" << lowerLeft[0]  << "," << lowerLeft[1];
  fileTest << "\t\t\t\t\t\t" << centerLong << "," << lowerRight[1];
  fileTest << "\t\t\t\t\t\t" << centerLong << "," << centerLat;
  fileTest << "\t\t\t\t\t\t" << upperLeft[0] << "," << centerLat  << std::endl;
  fileTest << "\t\t\t\t\t</coordinates>" << std::endl;
  fileTest << "\t\t\t\t</gx:LatLonQuad>" << std::endl;
  fileTest << "\t\t\t</Region>" << std::endl;
  fileTest << "\t\t\t<Link>" << std::endl;
  fileTest << "\t\t\t\t<href>" << fileTile4.str() << "xt.kml</href>" << std::endl;
  fileTest << "\t\t\t\t<viewRefreshMode>onRegion</viewRefreshMode>" << std::endl;
  fileTest << "\t\t\t\t<viewFormat/>" << std::endl;
  fileTest << "\t\t\t</Link>" << std::endl;
  fileTest << "\t\t</NetworkLink>" << std::endl;

  fileTest << "\t</Document>" << std::endl;
  fileTest << "</kml>" << std::endl;
  fileTest.close();

}

/**
 */
void
TileExportModule::RootKmlProcess(double north, double south, double east, double west)
{
  bool extended = gExtended->value();
  this->GenerateKMLRoot(m_FileName, north, south, east, west, extended);

  // Add the legend for this product if any
  this->AddCurrentProductLegends(0);

  // Add the flag netwotk link for each input image
  this->AddNetworkLinkToRootKML(north, south, east, west, m_CurrentImageName, true, 0);

  // Method to write a legend in the kmz
  this->AddCurrentProductLegends(0);

  // Last thing to do is to close the root kml
  this->CloseRootKML();

  // Add the root kml in the kmz
  std::ostringstream root_in_kmz;
  root_in_kmz << m_FileName << m_KmlExtension;

  std::ostringstream root_absolute_path;
  root_absolute_path << m_Path << "/" << root_in_kmz.str();

  // Add the root file in the kmz
  this->AddFileToKMZ(root_absolute_path, root_in_kmz);

  // Remove the root files
  if( itksys::SystemTools::FileExists(root_absolute_path.str().c_str()) == true )
    {
      if ( itksys::SystemTools::RemoveFile(root_absolute_path.str().c_str()) == false )
        {
          itkExceptionMacro(<< "Error while deleting the root file " << root_absolute_path.str());
        }
    }
  else
    {
      //TODO : remove log when bug closed
      std::cout<<"LOG: Impossible to remove root file, because doesn't exist... "<<root_absolute_path.str()<<std::endl;
    }
}

/** Add the legend to the product*/
void
TileExportModule::AddCurrentProductLegends(unsigned int curProd)
{
  for (unsigned int i = 0; i < m_ProductVector[curProd].m_AssociatedLegends.size(); i++)
    {
    std::ostringstream legendName;
    legendName << m_Path;
    legendName << "/legend_" << curProd << m_ProductVector[curProd].m_AssociatedLegends[i] << ".jpeg";

    FloatingVectorImageType::Pointer legend = this->GetInputData<FloatingVectorImageType>(
      "InputLegend",
      m_ProductVector[curProd].
      m_AssociatedLegends[i]);
    CastFilterType::Pointer castFiler = CastFilterType::New();
    castFiler->SetInput(legend);

    m_VectorWriter = VectorWriterType::New();
    m_VectorWriter->SetFileName(legendName.str().c_str());
    m_VectorWriter->SetInput(castFiler->GetOutput());
    m_VectorWriter->Update();

    // Add the legend to the kmz
    std::ostringstream legend_root_path_in_kmz;
    legend_root_path_in_kmz << "legends/legend_" << curProd << m_ProductVector[curProd].m_AssociatedLegends[i] <<
    ".jpeg";

    std::ostringstream legend_absolut_path;
    legend_absolut_path << legendName.str();

    this->AddFileToKMZ(legend_absolut_path, legend_root_path_in_kmz);

    // Remove the legend file
    if( itksys::SystemTools::FileExists(legend_absolut_path.str().c_str()) == true )
      {
        if (itksys::SystemTools::RemoveFile(legend_absolut_path.str().c_str()) == false)
          {
            std::cout<<legend_root_path_in_kmz.str()<<std::endl;
            itkExceptionMacro(<< "Error while deleting the legend file " << legend_absolut_path.str());
          }
      }
    else
      {
        //TODO : remove log when bug closed
        std::cout<<"LOG: Impossible to remove legend file, because doesn't exist... "<<legend_absolut_path.str()<<std::endl;
      }
    }
}

/**
  * Add the bounding box kml
  */

void
TileExportModule::BoundingBoxKmlProcess(double north, double south, double east, double west)
{
  // Create the bounding kml
  this->GenerateBoundingKML(north, south,  east, west);

  // Add the root kml in the kmz
  std::ostringstream bound_in_kmz;
  bound_in_kmz << "bounds/bound_" << m_CurrentProduct << m_KmlExtension;
  std::ostringstream bound_absolute_path;
  bound_absolute_path << m_Path << "/bound_" << m_CurrentProduct << m_KmlExtension;

  // Add the root file in the kmz
  this->AddFileToKMZ(bound_absolute_path, bound_in_kmz);

  // Remove the bounding files
  if( itksys::SystemTools::FileExists(bound_absolute_path.str().c_str()) == true )
    {
      if (itksys::SystemTools::RemoveFile(bound_absolute_path.str().c_str()) == false)
        {
          itkExceptionMacro(<< "Error while deleting the bound file" << bound_absolute_path.str());
        }
    }
  else
    {
      //TODO : remove log when bug closed
      std::cout<<"LOG: Impossible to remove bound file, because doesn't exist... "<<bound_absolute_path.str()<<std::endl;
    }
}

/**
  * Is Product Selected have geograhical
  * Update the global flag
  *
  * Is Product Selected have geograhical
  * Update the global flag
  */
void
TileExportModule::
IsProductHaveMetaData(unsigned int itkNotUsed(indexClicked))
{
  bool emptyProjRef = m_VectorImage->GetProjectionRef().empty();
  bool emptyKWL     = m_VectorImage->GetImageKeywordlist().GetSize() == 0 ? true : false;

  m_InputHaveMetaData = (!emptyProjRef || !emptyKWL);
}

/**
 * Check that the composition is well done
 */
bool
TileExportModule::CheckAndCorrectComposition(unsigned int clickedIndex)
{
  unsigned int nbComponent = m_VectorImage->GetNumberOfComponentsPerPixel();

  // Set the RedChannel on
  // the component - 1
  // position if the value
  // exceed the number of components
  if (this->cRedChannel->value() >= static_cast<int>(nbComponent))
    this->cRedChannel->value(nbComponent - 1);

  if (this->cGreenChannel->value() >= static_cast<int>(nbComponent))
    this->cGreenChannel->value(nbComponent - 1);

  if (this->cBlueChannel->value() >= static_cast<int>(nbComponent))
    this->cBlueChannel->value(nbComponent - 1);

  Fl::flush();

  return true;
}

/**
 * Check that the composition is well done
 */
void
TileExportModule::StoreAssociations()
{
  unsigned int nbLegends = this->GetNumberOfInputDataByKey("InputLegend");

  // associate the legend to the product
  for (unsigned int i = 0; i < nbLegends; i++)
    m_ProductVector[0].m_AssociatedLegends.push_back(i);
}

/**
 */
void
TileExportModule::ExportNonGeoreferencedProduct(unsigned int curIdx)
{
  // Get the values selected by the user
  double north, south, east, west;

  if (!gExtended->value())
    {
    if (vcl_abs(m_ProductVector[curIdx].m_CornerList[0] - 1000.) > 1e-15 &&
        vcl_abs(m_ProductVector[curIdx].m_CornerList[1] - 1000.) > 1e-15 &&
        vcl_abs(m_ProductVector[curIdx].m_CornerList[2] - 1000.) > 1e-15 &&
        vcl_abs(m_ProductVector[curIdx].m_CornerList[3] - 1000.) > 1e-15)
      {
      // Get the corners values
      north = m_ProductVector[curIdx].m_CornerList[1];
      south = m_ProductVector[curIdx].m_CornerList[3];
      east  = m_ProductVector[curIdx].m_CornerList[0];
      west  = m_ProductVector[curIdx].m_CornerList[2];
      }
    else
      {
      itkExceptionMacro(
        << "Product " << m_ProductVector[curIdx].m_Name <<
        " have no geographical informations, please set the upper left and lower right corners coordinates");
      }
    }

  // The other case : extended mode
  if (gExtended->value())
    {
    // A trick to see if all the value have been updated
    // Since all the corners are initialized with 1000.
    // and are updated if value edited in the GUI changed,
    // this comparaison did the trick
    if (vcl_abs(m_UpperLeftCorner[0] - 1000.) > 1e-15 &&
        vcl_abs(m_UpperLeftCorner[1] - 1000.) > 1e-15 &&
        vcl_abs(m_UpperRightCorner[0] - 1000.) > 1e-15 &&
        vcl_abs(m_UpperRightCorner[1] - 1000.) > 1e-15 &&
        vcl_abs(m_LowerLeftCorner[0] - 1000.) > 1e-15 &&
        vcl_abs(m_LowerLeftCorner[1] - 1000.) > 1e-15 &&
        vcl_abs(m_LowerRightCorner[0] - 1000.) > 1e-15 &&
        vcl_abs(m_LowerRightCorner[1] - 1000.) > 1e-15
        )
      {
      north = m_UpperLeftCorner[1];
      south = m_LowerLeftCorner[1];
      west  = m_UpperLeftCorner[0];
      east  = m_UpperRightCorner[0];
      }
    else
      {
      itkExceptionMacro(
        << "Product " << m_ProductVector[curIdx].m_Name <<
        " have no geographical informations, please set all the coordinates");
      }
    }

  // Add the headers and the basic stuffs in the kml only once.
  if (curIdx == 0)
    {
    // Generate the root kml
    this->RootKmlProcess(north, south, east, west);
    }

  // Generate the bounding box kml
  this->BoundingBoxKmlProcess(north, south, east, west);

  // Add the files to the kmz file
  // Relative path to the root directory  in the kmz file
  std::ostringstream jpg_in_kmz;
  jpg_in_kmz << m_CurrentImageName << "/0.jpg";

  // Absolute path where are stored the files
  std::ostringstream jpg_absolute_path;
  jpg_absolute_path << m_Path << "/0.jpg";

  // Write the file on the disk
  CastFilterType::Pointer          castFiler = CastFilterType::New();
  castFiler->SetInput(m_VectorImage);

  m_VectorWriter = VectorWriterType::New();
  m_VectorWriter->SetFileName(jpg_absolute_path.str().c_str());
  m_VectorWriter->SetInput(castFiler->GetOutput());
  m_VectorWriter->Update();

  //Add the files to the kmz
  this->AddFileToKMZ(jpg_absolute_path, jpg_in_kmz);

  // Remove the unecessary files
  if( itksys::SystemTools::FileExists(jpg_absolute_path.str().c_str()) == true )
    {
      if (itksys::SystemTools::RemoveFile(jpg_absolute_path.str().c_str()) == false)
        {
          itkExceptionMacro(<< "Error while deleting the file " << jpg_absolute_path.str());
        }
    }
  else
    {
      //TODO : remove log when bug closed
      std::cout<<"LOG: Impossible to remove jpg file, because doesn't exist... "<<jpg_absolute_path.str()<<std::endl;
    }
}

/**
  * Update Product Information
  */
void
TileExportModule::UpdateProductInformations()
{
  // Get the indexClicked product
  ProductInformationType product = m_ProductVector[0];
  unsigned int           indexClicked = 0; // This module take only one
                                           // product as input

  // Set the composition values
  if (this->CheckAndCorrectComposition(0))
    {
    m_ProductVector[indexClicked].m_Composition[0] = this->cRedChannel->value();
    m_ProductVector[indexClicked].m_Composition[1] = this->cGreenChannel->value();
    m_ProductVector[indexClicked].m_Composition[2] = this->cBlueChannel->value();
    }

  // If non geo Product, Fill the corners
  if (!m_InputHaveMetaData)
    {
    // The case we export the product in a GX:LatLong box
    if (gExtended->value())
      {
      if (vgxGELongUL->value()) m_UpperLeftCorner[0]  = vgxGELongUL->value();
      if (vgxGELatUL->value()) m_UpperLeftCorner[1]  = vgxGELatUL->value();

      if (vgxGELongUR->value()) m_UpperRightCorner[0] = vgxGELongUR->value();
      if (vgxGELatUR->value()) m_UpperRightCorner[1] = vgxGELatUR->value();

      if (vgxGELongLL->value()) m_LowerLeftCorner[0]  = vgxGELongLL->value();
      if (vgxGELatLL->value()) m_LowerLeftCorner[1]  = vgxGELatLL->value();

      if (vgxGELongLR->value()) m_LowerRightCorner[0] = vgxGELongLR->value();
      if (vgxGELatLR->value()) m_LowerRightCorner[1] = vgxGELatLR->value();
      }
    else
      {
      if (vGELongUL->value()) m_ProductVector[indexClicked].m_CornerList[0] = vGELongUL->value();
      if (vGELatUL->value()) m_ProductVector[indexClicked].m_CornerList[1] = vGELatUL->value();
      if (vGELongLR->value()) m_ProductVector[indexClicked].m_CornerList[2] = vGELongLR->value();
      if (vGELatLR->value()) m_ProductVector[indexClicked].m_CornerList[3] = vGELatLR->value();
      }
    }
}

/**
 * CallBack to handle the coordinate group
 */
void
TileExportModule::HandleCornersGroup()
{
  // Case 1 : Product geo : don't show the group
  if (m_InputHaveMetaData)
    {
    vgxGELatLongBoxGroup->deactivate();
    vGELatLongBoxGroup->hide();
    }

  // Case 2 : Product non geo : Depends on the extend button
  if (!m_InputHaveMetaData)
    {
    if (gExtended->value())
      {
      vGELatLongBoxGroup->hide();
      vgxGELatLongBoxGroup->show();
      }
    else
      {
      vGELatLongBoxGroup->show();
      vgxGELatLongBoxGroup->hide();
      }
    }
}
/**
 * BrowseDEM Callback
 */
void TileExportModule::BrowseDEM()
{
  const char * filename = NULL;

  const char* defaultPath = "";
  defaultPath = otb::ConfigurationManager::GetDEMDirectory().c_str();
  filename = flu_dir_chooser("Choose the folder...", defaultPath);

  if (filename == NULL)
    {
    otbMsgDebugMacro(<<"Empty file name!");
    return;
    }

  typedef otb::DEMHandler DEMHandlerType;
  DEMHandlerType::Pointer DEMTest = DEMHandlerType::Instance();

  if (!DEMTest->IsValidDEMDirectory(filename))
    {
     m_DEMDirectory = "";
     MsgReporter::GetInstance()->SendError("invalid DEM directory, no DEM directory has been set.");
     return;
    }

  vDEMDirectory->value(filename);

  // Set the DEM directory
  m_DEMDirectory = filename;

  DEMTest->OpenDEMDirectory(m_DEMDirectory);
}

/**
 * Use DEM callback
 */
void TileExportModule::UseDEM()
{
  if (cUseDEM->value())
    {
    vDEMDirectory->activate();
    bBrowseDEM->activate();
    }
  else
    {
    vDEMDirectory->deactivate();
    bBrowseDEM->deactivate();
    }
}

/**
 * Quit the GUI
 */
void TileExportModule::Quit()
{
  this->Hide();
}


} // End namespace otb

#endif
