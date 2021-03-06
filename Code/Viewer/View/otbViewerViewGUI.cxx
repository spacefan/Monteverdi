/*=========================================================================

  Program:   ORFEO Toolbox
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


  Copyright (c) Centre National d'Etudes Spatiales. All rights reserved.
See OTBCopyright.txt for details.


    This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULA
     PURPOSE,  See the above copyright notices for more information.

=========================================================================*/
#include <FL/Fl_Text_Buffer.H>
#include "otbViewerViewGUI.h"
#include "otbMacro.h"
#include "itksys/SystemTools.hxx"

namespace otb
{
/**
 * Constructor
 */

ViewerViewGUI
::ViewerViewGUI():m_TemplateViewerName(""), m_DisplayedLabel("+ "),
                  m_UndisplayedLabel("- "), m_DiaporamaCurrentIndex(0)
{

  m_VisuView              =  VisuViewType::New();
  m_PreviewWidget         =  ImageWidgetType::New();
  m_WidgetManagerList     =  WidgetManagerList::New();
  m_LinkWidgetManagerList =  WidgetManagerList::New();

  //Get an instance of the model
  m_ViewerModel = ViewerModel::GetInstance();
  m_ViewerModel->RegisterListener(this);

  //Create the component of the GUI
  this->CreateGUI();
  bSplitted->activate();
  bPacked->activate();

  //Create buffer for the guiViewerInformation
  Fl_Text_Buffer * buffer = new Fl_Text_Buffer();
  this->guiViewerInformation->buffer(buffer);

  //init the previewWindow
  m_PreviewWidget->label("PreviewWidget");
  gPreviewWindow->add(m_PreviewWidget);
  gPreviewWindow->box(FL_NO_BOX);
  gPreviewWindow->resizable(gPreviewWindow);
  m_PreviewWidget->resize(gPreviewWindow->x(), gPreviewWindow->y(), gPreviewWindow->w(), gPreviewWindow->h() );

  //Color Definition
  m_Red.Fill(0);
  m_Green.Fill(0);
  m_Blue.Fill(0);
  m_Grey.Fill(0.5);
  m_Red[0]  = 1.;   m_Red[3]   = 0.5;
  m_Green[1]= 1.;   m_Green[3] = 0.5;
  m_Blue[2] = 1.;   m_Blue[3]  = 0.5;

  //Slide Show
  m_Widget  =  PackedWidgetManagerType::New();
}

void
ViewerViewGUI
::OpenImage(const char * inputFileName)
{
  unsigned int numberOfOpenedImages = m_ViewerController->OpenInputImage(inputFileName);
 //Initialize
  this->Initialize(numberOfOpenedImages);

  // Select the last opened image
  guiImageList->value( guiImageList->size() );
  //Udpate the ViewerGUISetup
  unsigned int selectedItem = guiImageList->value();
  this->UpdateViewerSetupWindow(selectedItem);
  this->ViewerSetupOk();

  this->SelectAction();
}

/**
 *
 */
void
ViewerViewGUI
::OpenImage()
{
  std::string pikedFileName="";
  char * cfname;
  cfname = fl_file_chooser("Pick an image file", "*.*", pikedFileName.c_str());

  if (cfname == NULL || strlen(cfname)<1)
    {
      otbMsgDebugMacro(<<"Empty file name!");
      return;
    }

  Fl::check();
  guiMainWindow->redraw();

  if( m_ViewerModel->IsJPEG2000File( cfname ) )
    {
    guiJpeg2000Res->clear();

    std::vector<unsigned int> res;
    std::vector<std::string> desc;
    m_ViewerModel->GetJPEG2000ResolutionAndInformations( cfname, res, desc );

    for( unsigned int j=0; j<res.size(); j++ )
      {
      guiJpeg2000Res->add( desc[j].c_str() );
      }
    guiJpeg2000Res->value(0);
    guiJpeg2000Filename->value( cfname );

    guiJpeg2000ResSelection->redraw();
    guiJpeg2000ResSelection->show();
    }

  else if (m_ViewerModel->IsHDFFile( cfname ))
  {

   guiHDFDataset->clear();
   std::vector<std::string> names;
   std::vector<std::string> desc;
   m_ViewerModel->GetHDFDataset( cfname, names, desc );
   for( unsigned int j=0; j<names.size(); j++ )
      {
      guiHDFDataset->add( desc[j].c_str() );
      }
   guiHDFDataset->value(0);
   guiHDFFilename->value( cfname );
   guiHDFDatasetSelection->redraw();
   guiHDFDatasetSelection->show();

  }
  else
    {
    //Initialize
    this->OpenImage(cfname);
    }
}


/**
 *
 */
void
ViewerViewGUI
::Initialize(const unsigned int & numberOfOpenedImages)
{
  for ( unsigned int i = 0; i < numberOfOpenedImages; i++ )
  {
    //Initialise the boolean pair
    PairType      pair(false, false); //(Not displayed , Packed View)

    //Put a new Widget in the list
    if(bSplitted->value() && !bPacked->value())
      {
        SplittedWidgetManagerPointer widget     = SplittedWidgetManagerType::New();
        SplittedWidgetManagerPointer linkwidget = SplittedWidgetManagerType::New();

        m_WidgetManagerList->PushBack(widget);
        m_LinkWidgetManagerList->PushBack(linkwidget);
        pair.second = true;
      }
    else
      {
        PackedWidgetManagerPointer widget     = PackedWidgetManagerType::New();
        PackedWidgetManagerPointer linkwidget = PackedWidgetManagerType::New();
        m_WidgetManagerList->PushBack(widget);
        m_LinkWidgetManagerList->PushBack(linkwidget);
      }

    //Put the status of the last added image
    m_DisplayStatusList.push_back(pair);
    m_LinkedDisplayStatusList.push_back(false);

    // Call the Controller
    //m_ViewerController->OpenInputImage(cfname);

    //Update the Progress Bar
    this->UpdateDiaporamaProgressBar();

    //Update the Link Setup
    this->UpdateLinkSetupWindow();

  }
}


void
ViewerViewGUI
::OpenJpeg2000Image()
{
  const std::string descRes = guiJpeg2000Res->value();
  const char * cfname =  guiJpeg2000Filename->value();

  // Find the resolution id from the selected description
  std::vector<unsigned int> res;
  std::vector<std::string> desc;
  m_ViewerModel->GetJPEG2000ResolutionAndInformations( cfname, res, desc );
  unsigned int resVal;
  bool found = false;
  unsigned int id = 0;
  while ( id<desc.size() && !found)
    {
    if( desc[id] == descRes )
      {
      resVal = res[id];
      found = true;
      }
    id++;
    }

  if (!found)
    {
    itkExceptionMacro( "Unable to find the resolution associated to the description "<<descRes);
    }

  unsigned int numberOfOpenedImages = m_ViewerController->OpenInputImage(cfname, resVal);
  this->Initialize(numberOfOpenedImages);
}

void
ViewerViewGUI
::OpenHDFImage()
{
  const std::string descDataset = guiHDFDataset->value();
  const char * cfname =  guiHDFFilename->value();

  // Find the dataset id from the selected description
  std::vector<std::string> names;
  std::vector<std::string> desc;
  m_ViewerModel->GetHDFDataset( cfname, names, desc );
  bool isFound = false;
  unsigned int id = 0;
  unsigned int datasetId = 0;
  while ( id<desc.size() && !isFound)
    {
    size_t found;
    found = desc[id].find(descDataset);
    if( found!=std::string::npos )
      {
      isFound = true;
      datasetId = id;
      }
    id++;
    }

  if (!isFound)
    {
    itkExceptionMacro( "Unable to find the dataset associated to the description "<<descDataset);
    }

  unsigned int numberOfOpenedImages = m_ViewerController->OpenInputImage(cfname, datasetId);
  this->Initialize(numberOfOpenedImages);
}

/**
 * Handle the notification of the model
 */
void
ViewerViewGUI
::Notify()
{
  if(m_ViewerModel->GetHasImageOpened())
    this->AddImageListName();

  //Update the widget when channel order modified
  if(m_ViewerModel->GetHasChangedChannelOrder())
    {
      unsigned int selectedItem = guiImageList->value();
      if (selectedItem == 0)
       return;        // no image selected, return

      //DipalyPreviewWidget
      this->DisplayPreviewWidget(selectedItem);

      //Update the widget dispalyed
      if(m_DisplayStatusList[selectedItem-1].first)
       {
         m_WidgetManagerList->GetNthElement(selectedItem-1)->Refresh();
         this->Display(m_WidgetManagerList, selectedItem);
       }

      this->UpdateInformation(selectedItem);
    }
}

/**
 * CloseImage , Send the notification to the controller then to the model
 */
void
ViewerViewGUI
::CloseImage()
{
  unsigned int selectedItem = guiImageList->value();
  if (selectedItem == 0)
    return;        // no image selected, return

  //Hide if showned
  if(m_DisplayStatusList[selectedItem-1].first)
    {
      this->Undisplay(selectedItem);
    }

  //Check if the closed image is linked and showed
  // if it is the case : undisplay it and update the linkSetup
  if(guiLinkSetupWindow->shown() != 0)
    {
      if(guiImageList->size() > 1)
       {
         if(m_LinkedDisplayStatusList[selectedItem-1])
           for(unsigned int i = 0; i<m_LinkedDisplayStatusList.size(); i++)
             if(m_LinkedDisplayStatusList[i])
              {
                m_LinkWidgetManagerList->GetNthElement(i)->Hide();
                m_LinkedDisplayStatusList[i] = false;
              }
       }
    }


  //Erase the item selected
  m_PreviewWidget->hide();
  guiImageList->remove(selectedItem);

  //Link when all images are closed
  if(guiLinkSetupWindow->shown() != 0)
    if(guiImageList->size() == 0)
      this->LinkSetupOk();

  //Erase from the lists
  m_DisplayStatusList.erase( m_DisplayStatusList.begin()+(selectedItem-1));
  m_LinkedDisplayStatusList.erase( m_LinkedDisplayStatusList.begin()+(selectedItem-1));
  m_WidgetManagerList->Erase(selectedItem-1);
  m_LinkWidgetManagerList->Erase(selectedItem-1);


  //Diaporama
  if(guiDiaporama->shown() != 0)
    {
      if(guiImageList->size() > 0)
       {
         //If the closed image is the current showed in the diaporama
         if(selectedItem == m_DiaporamaCurrentIndex+1)
           {
             //if the closed image is the last one in the diapo : show the previous
             if(selectedItem -1 == static_cast<unsigned int>(guiImageList->size()))
              {
                this->DiaporamaPrevious();
              }
             else
              {
                //if the closed image is the first one : show the next one
                this->UpdateDiaporamaProgressBar();
                //Increment to show the next image
                ++m_DiaporamaCurrentIndex;
                this->DisplayDiaporama();
                //Decrement because we remove the first image, so the current display become the first
                --m_DiaporamaCurrentIndex;
              }
           }
         else
           {
             if(selectedItem < m_DiaporamaCurrentIndex+1)
              {
              //Increment the current index because an image before the one displayed is removed
                --m_DiaporamaCurrentIndex;
              }
             this->UpdateDiaporamaProgressBar();
           }
       }
    }

    //Case all Images closed
    if(guiDiaporama->shown() != 0)
    {
      if(m_DisplayStatusList.size() == 0)
       {
         m_DiaporamaCurrentIndex=0;
         //If no image to display anymore : quit the diaporamaGUI
         this->DiaporamaQuit();
       }
    }

    //
    //Udpate the controller of the image linked with the suppressed one
    if(guiImageList->size() > 0)
    this->InitializeImageController(selectedItem);

  //Call the controller
  m_ViewerController->CloseImage(selectedItem);

  //Update the Link Setup
  this->UpdateLinkSetupWindow();


 }

 /**
  * Show the mainWindow
  */
 void
 ViewerViewGUI
 ::Show()
 {
   guiMainWindow->show();
 }

 /**
  * Update the filename
  */
 void
 ViewerViewGUI
 ::AddImageListName()
 {
   //Update the Image List widget
   unsigned int len     = m_ViewerModel->GetObjectList().size();
   std::string fileName = m_ViewerModel->GetObjectList().at(len-1).pFileName;
   int slashIndex       = fileName.find_last_of("/", fileName.size());

   std::ostringstream oss;
   oss<<m_UndisplayedLabel;
   oss<<m_TemplateViewerName<<fileName.substr(slashIndex+1, fileName.size());
   guiImageList->add(oss.str().c_str());
   guiImageList->redraw();
 }

 /**
  *
  */
 void
 ViewerViewGUI
 ::SelectAction()
 {
   unsigned int selectedItem = guiImageList->value();
   if (selectedItem == 0)
     {
       // no image selected, return
       return;
     }

   //Update the radio button
   if( m_DisplayStatusList[selectedItem-1].second)
     {
       bSplitted->value(true);
       bPacked->value(false);
     }
   else
     {
       bSplitted->value(false);
       bPacked->value(true);
     }

   //DipalyPreviewWidget
   this->DisplayPreviewWidget(selectedItem);

   //Udpate the ViewerGUISetup
   this->UpdateViewerSetupWindow(selectedItem);

   //Update SelectedImageInformation
   this->UpdateInformation(selectedItem);

   //LINK SETUP
   this->UpdateLinkSetupWindow();
 }
 /**
  *
  */
 void
 ViewerViewGUI
 ::DisplayPreviewWidget(unsigned int selectedItem)
 {
   //Build the m_PreviewWidget
   VisuModelPointerType rendering = m_ViewerModel->GetObjectList().at(selectedItem-1).pRendering;

   m_PreviewWidget->ClearBuffer();
   ViewerModelType::ViewerImageType * quickLook = rendering->GetRasterizedQuicklook();
   m_PreviewWidget->ReadBuffer(quickLook, quickLook->GetLargestPossibleRegion());

   double newIsotropicZoom = this->UpdatePreviewWidgetIsotropicZoom(quickLook->GetLargestPossibleRegion().GetSize());
   m_PreviewWidget->SetIsotropicZoom(newIsotropicZoom);
   m_PreviewWidget ->show();
   m_PreviewWidget->redraw();
 }


 /**
  * Compute the size of the
  */
 double
 ViewerViewGUI
 ::UpdatePreviewWidgetIsotropicZoom(SizeType size)
 {
   int h = gPreviewWindow->h();
   int w = gPreviewWindow->w();

   double zoomW = static_cast<double>(w)/static_cast<double>(size[0]);
   double zoomH = static_cast<double>(h)/static_cast<double>(size[1]);

   return std::min(zoomW, zoomH);
 }

 /**
  * Show Hide
  */
 void
 ViewerViewGUI
 ::ShowHide()
 {
   unsigned int selectedItem = guiImageList->value();
   if (selectedItem == 0)
     {
       // no image selected, return
       return;
     }
   if(guiDiaporama->shown() == 0 && guiLinkSetupWindow->shown() == 0)
     {
       //check what to do
       if(!m_DisplayStatusList[selectedItem-1].first)
        {
          //New Display
          m_DisplayStatusList[selectedItem-1].first = true;
          this->UpdateImageListShowed(selectedItem, m_DisplayedLabel);
          this->Display(m_WidgetManagerList, selectedItem);
        }
       else
        {
          m_DisplayStatusList[selectedItem-1].first = false;
          this->UpdateImageListShowed(selectedItem, m_UndisplayedLabel);
          this->Undisplay(selectedItem);
        }
     }
 }

 /**
  * Display the three widget
  */
 void
 ViewerViewGUI
 ::Display(WidgetManagerList::Pointer  widgetList, unsigned int selectedItem)
 {
   const ObjectsTrackedType & objTracked = m_ViewerModel->GetObjectList().at(selectedItem-1);
   //Get the view stored in the model
   CurvesWidgetType::Pointer curveWidget =  objTracked.pCurveWidget;
   VisuViewPointerType currentVisuView =  objTracked.pVisuView;

   //First get the histogram list
   RenderingFunctionType::Pointer pRenderingFunction = objTracked.pRenderFunction;


   curveWidget->ClearAllCurves();

   if (pRenderingFunction->GetPixelRepresentationSize() >=3)
   {
     HistogramCurveType::Pointer bhistogram = HistogramCurveType::New();
     bhistogram->SetHistogramColor(m_Blue);
     bhistogram->SetLabelColor(m_Blue);
     bhistogram->SetHistogram(objTracked.pLayer->GetHistogramList()->GetNthElement(2));
     curveWidget->AddCurve(bhistogram);
   }

   if (pRenderingFunction->GetPixelRepresentationSize() >=2)
   {
     HistogramCurveType::Pointer ghistogram = HistogramCurveType::New();
     ghistogram->SetHistogramColor(m_Green);
     ghistogram->SetLabelColor(m_Green);
     ghistogram->SetHistogram(objTracked.pLayer->GetHistogramList()->GetNthElement(1));
     curveWidget->AddCurve(ghistogram);
   }

   HistogramCurveType::Pointer rhistogram = HistogramCurveType::New();
   if (pRenderingFunction->GetPixelRepresentationSize() == 1)
   {
     rhistogram->SetHistogramColor(m_Grey);
     rhistogram->SetLabelColor(m_Grey);
   }
   else
   {
     rhistogram->SetHistogramColor(m_Red);
     rhistogram->SetLabelColor(m_Red);
   }
   rhistogram->SetHistogram(objTracked.pLayer->GetHistogramList()->GetNthElement(0));
   curveWidget->AddCurve(rhistogram);

   curveWidget->SetXAxisLabel("Pixels");
   curveWidget->SetYAxisLabel("Frequency");

   //Get the pixelView
   PixelDescriptionViewType::Pointer pixelView = objTracked.pPixelView;

   widgetList->GetNthElement(selectedItem-1)->RegisterFullWidget(currentVisuView->GetFullWidget());
   widgetList->GetNthElement(selectedItem-1)->RegisterScrollWidget(currentVisuView->GetScrollWidget());
   widgetList->GetNthElement(selectedItem-1)->RegisterZoomWidget(currentVisuView->GetZoomWidget());
   widgetList->GetNthElement(selectedItem-1)->RegisterPixelDescriptionWidget(pixelView->GetPixelDescriptionWidget());
   widgetList->GetNthElement(selectedItem-1)->RegisterHistogramWidget(curveWidget);
   widgetList->GetNthElement(selectedItem-1)->SetLabel((this->CutFileName2(selectedItem-1)).c_str());
   widgetList->GetNthElement(selectedItem-1)->Show();
 }

 /**
  */
 void
 ViewerViewGUI
 ::SplittedViewMode()
 {
     unsigned int selectedItem = guiImageList->value();
   if (selectedItem == 0)
     {
       // no image selected, return
       return;
     }

   SplittedWidgetManagerPointer widget  =  SplittedWidgetManagerType::New();
   m_WidgetManagerList->GetNthElement(selectedItem-1)->Hide();
   m_WidgetManagerList->SetNthElement(selectedItem-1, widget);

   //Update the list link
   m_LinkWidgetManagerList->GetNthElement(selectedItem-1)->Hide();
   m_LinkWidgetManagerList->SetNthElement(selectedItem-1, widget);

   if(m_LinkedDisplayStatusList[selectedItem-1])
     {
       this->Display(m_LinkWidgetManagerList, selectedItem);
     }

   //If the guilink or te guiDiapo is opened don't display and don't change the view mode
   if(guiDiaporama->shown() == 0 && guiLinkSetupWindow->shown() == 0)
     {
       //Update the view mode if the link mode is not updated
       if(!m_DisplayStatusList[selectedItem-1].first && !m_LinkedDisplayStatusList[selectedItem-1])
        {
          m_DisplayStatusList[selectedItem-1].second = true;
        }

       //If already displayed : update the view mode and display the new viewMode
       if(m_DisplayStatusList[selectedItem-1].first && !m_LinkedDisplayStatusList[selectedItem-1] )
        {
          m_DisplayStatusList[selectedItem-1].second = true;
          this->Display(m_WidgetManagerList, selectedItem);
        }
     }
 }

 /**
 *
 */
 void
 ViewerViewGUI
 ::PackedViewMode()
 {
   unsigned int selectedItem = guiImageList->value();
   if (selectedItem == 0)
     {
       // no image selected, return
       return;
     }

   //
   PackedWidgetManagerPointer widget  =  PackedWidgetManagerType::New();
   m_WidgetManagerList->GetNthElement(selectedItem-1)->Hide();
   m_WidgetManagerList->SetNthElement(selectedItem-1, widget);

   //Update the link list
   m_LinkWidgetManagerList->GetNthElement(selectedItem-1)->Hide();
   m_LinkWidgetManagerList->SetNthElement(selectedItem-1, widget);

   if(m_LinkedDisplayStatusList[selectedItem-1])
     {
       this->Display(m_LinkWidgetManagerList, selectedItem);
     }


   //If the guilink or te guiDiapo is opened don't display and don't change the view mode
   if(guiDiaporama->shown() == 0 && guiLinkSetupWindow->shown() == 0)
     {
       //Update the view mode
       if(!m_DisplayStatusList[selectedItem-1].first && !m_LinkedDisplayStatusList[selectedItem-1])
        {
          m_DisplayStatusList[selectedItem-1].second = false;
        }

       //If already displayed : update the view mode and display the new viewMode
       if(m_DisplayStatusList[selectedItem-1].first && !m_LinkedDisplayStatusList[selectedItem-1] )
        {
          m_DisplayStatusList[selectedItem-1].second = false;
          this->Display(m_WidgetManagerList, selectedItem);
        }
     }
 }

 /**
  * Update the guiImageList
  * Put a  "+" if the view is being showed, or a "-" otherwise in the begining of the imageName
  */
 void
 ViewerViewGUI
 ::UpdateImageListShowed(unsigned int selectedItem, std::string status)
 {
   /* Update the ImageList using the status label "+" or "-" */
   std::string fileName = m_ViewerModel->GetObjectList().at(selectedItem-1).pFileName;
   int slashIndex = fileName.find_last_of("/", fileName.size());

   std::ostringstream oss;
   oss<<status;
   oss<<m_TemplateViewerName<<fileName.substr(slashIndex+1, fileName.size());
   guiImageList->text(selectedItem, oss.str().c_str());
   oss.str("");
 }

 /**
  * Hide all the widget opened
  */
 void
 ViewerViewGUI
 ::Undisplay(unsigned int selectedItem)
 {
   m_WidgetManagerList->GetNthElement(selectedItem-1)->Hide();
   m_WidgetManagerList->GetNthElement(selectedItem-1)->UnRegisterAll();
 }

 /**
  * Hide all the widget opened
  */
 void
 ViewerViewGUI
 ::HideAll()
 {
   //Hide all only if the diaporama and link window are not displayed
   if(guiDiaporama->shown() == 0 && guiLinkSetupWindow->shown() == 0)
     this->CloseAllDisplayedImages(true);
 }

 /**
  * Close all the displayed images:
  * showHideEvent : if true clear the showedlist else don't clear.
  */
 void
 ViewerViewGUI
 ::CloseAllDisplayedImages(bool showHideEvent)
 {
   // Set the display Label to undislayed
   for(unsigned int i = 0; i<m_DisplayStatusList.size(); i++)
     {
       if(m_DisplayStatusList[i].first)
        {
          if(showHideEvent)
            {
              this->UpdateImageListShowed(i+1, m_UndisplayedLabel);
              m_DisplayStatusList[i].first = false;
            }
          m_WidgetManagerList->GetNthElement(i)->Hide();
          m_WidgetManagerList->GetNthElement(i)->UnRegisterAll();
        }
     }
 }

 /**
  *
  */
 void
 ViewerViewGUI
 ::ShowTemporaryClosedDisplay()
 {
   for(unsigned int i = 0; i<m_DisplayStatusList.size(); i++)
     if(m_DisplayStatusList[i].first )
       this->Display(m_WidgetManagerList, i+1);
 }


 /**
  * Quit GUI
  */
 void
 ViewerViewGUI
 ::Quit()
 {
   guiMainWindow->hide();

   for(unsigned int i = 0; i<m_DisplayStatusList.size(); i++)
     if(m_DisplayStatusList[i].first)
       {
        m_WidgetManagerList->GetNthElement(i)->Hide();
        m_WidgetManagerList->GetNthElement(i)->UnRegisterAll();
       }

   //Quit the Diaporama if shown
   if(guiDiaporama->shown() != 0 )
     {
       guiDiaporama->hide();
       m_Widget->Hide();
     }

   //Hide the linkSetupWindow
   if(guiLinkSetupWindow->shown() != 0)
     {
       guiLinkSetupWindow->hide();
       //Close the  displays linked
       for(unsigned int i = 0; i<m_LinkedDisplayStatusList.size(); i++)
        if(m_LinkedDisplayStatusList[i])
          {
            m_LinkWidgetManagerList->GetNthElement(i)->Hide();
          }
     }

 }

 /**
  *
  */
 void
 ViewerViewGUI
 ::UpdateInformation(unsigned int selectedItem)
 {
   std::ostringstream oss;
   oss.str("");
   const ObjectsTrackedType & objTracked = m_ViewerModel->GetObjectList().at(selectedItem-1);
   std::string selectedImageName = objTracked.pFileName;
   // Clear the info buffer
   guiViewerInformation->buffer()->remove(0, guiViewerInformation->buffer()->length());
   oss<<"Filename: "<<selectedImageName<<std::endl;
   guiViewerInformation->insert(oss.str().c_str());
   oss.str("");
   oss<<"Image information: "<<std::endl;
   guiViewerInformation->insert(oss.str().c_str());
   oss.str("");


   if (m_ViewerModel->IsComplexFile( m_ViewerModel->GetObjectList().at(selectedItem-1).pFileName ))
     {
     oss<<"Number of bands: "<<objTracked.pReader->GetOutput()->GetNumberOfComponentsPerPixel()/2<<std::endl;
     oss<<"(Complex image: "<<objTracked.pReader->GetOutput()->GetNumberOfComponentsPerPixel()<< " bands displayed: Even bands are real parts, odd ones are imaginary parts.)";
     }
   else
     {
     oss<<"Number of bands: "<<objTracked.pReader->GetOutput()->GetNumberOfComponentsPerPixel();
     }
   oss<<std::endl;
   oss<<"Size: "<<objTracked.pReader->GetOutput()->GetLargestPossibleRegion().GetSize()<<std::endl;

   guiViewerInformation->insert(oss.str().c_str());
   oss.str("");

   //update band information
   // Select the current rendering function
   RenderingFunctionType::Pointer renderer = objTracked.pRenderFunction;

   ChannelListType channels = renderer->GetChannelList();

   //complex mode
   switch(objTracked.pViewType ){
      //grayscale mode
      case 0:
         oss<<"Grayscale mode : Channel=" << channels[0]+1 << std::endl;
         break;
      //Complex mode (modulus)
      case 1:
         oss<<"Complex mode : Modulus computed using bands "<< channels[0]+1 << " (real part), "<<channels[1]+1<<" (imaginary part)"<<std::endl;
         break;
      //Complex mode (phase)
      case 2:
         oss<<"Complex mode : Phase computed using bands "<< channels[0]+1 << " (real part), "<<channels[1]+1<<" (imaginary part)"<<std::endl;
         break;
      //rgb mode
      case 3:
         oss<<"Displayed channels : R=" << channels[0]+1 << ", G=" << channels[1]+1 << ", B=" << channels[2]+1 << "." << std::endl;
         break;
   }

   guiViewerInformation->insert(oss.str().c_str());
   oss.str("");
 }

 /**
  *
  */
 void
 ViewerViewGUI
 ::UpdateViewerSetupWindow(unsigned int selectedItem)
 {
   unsigned int nbComponent = m_ViewerModel->GetObjectList().at(selectedItem-1).pReader->GetOutput()->GetNumberOfComponentsPerPixel();

   // Constrain min and max
   guiGrayscaleChannelChoice->range(1, nbComponent);

   guiRedChannelChoice->range(1, nbComponent);
   guiGreenChannelChoice->range(1, nbComponent);
   guiBlueChannelChoice->range(1, nbComponent);

   guiRealChannelChoice->range(1, nbComponent);
   guiImaginaryChannelChoice->range(1, nbComponent);

   guiViewerSetupWindow->redraw();

   if (m_ViewerModel->IsComplexFile( m_ViewerModel->GetObjectList().at(selectedItem-1).pFileName ))
     {
     this->ComplexSet();
     }
   else
      {
      switch(nbComponent){
      case 1 :
      this->GrayScaleSet();
      break;
      case 4 :
      this->RGBSet();
      break;
      case 3 :
      this->RGBSet();
      break;
      case 2:
      this->GrayScaleSet();
      break;
      default :
      break;
      }
      }
   guiViewerSetupName->value(this->CutFileName(selectedItem-1));
   //this->ViewerSetupOk();

 }

 /**
  * RGBSet();
  */
 void
 ViewerViewGUI
 ::RGBSet()
 {
   otbMsgDevMacro( << "RGBSet");

   unsigned int selectedItem = guiImageList->value();
   if (selectedItem == 0)
     {
       // no image selected, return
       return;
     }

   unsigned int nbComponent = m_ViewerModel->GetObjectList().at(selectedItem-1).pReader->GetOutput()->GetNumberOfComponentsPerPixel();

   guiViewerSetupColorMode->set();
   guiViewerSetupComplexMode->clear();
   guiViewerSetupGrayscaleMode->clear();
   guiGrayscaleChannelChoice->deactivate();
   guiRealChannelChoice->deactivate();
   guiImaginaryChannelChoice->deactivate();
   bAmplitude->deactivate();
   bPhase->deactivate();

   guiRedChannelChoice->activate();
   guiGreenChannelChoice->activate();
   guiBlueChannelChoice->activate();

   assert(m_ViewerModel->GetObjectList().at(selectedItem-1).pRenderFunction.GetPointer());

   RenderingFunctionType::Pointer renderingFunction = m_ViewerModel->GetObjectList().at(selectedItem-1).pRenderFunction;

   // Get the current channel list (may have been already set by the user in case of a re-opening of the viewer)
   ChannelListType channels = renderingFunction->GetChannelList();

   unsigned int i = 0;
   while (channels.size() < 3)
   {
     channels.push_back(i);
     ++i;
   }

   guiRedChannelChoice->value(std::min(channels[0] + 1, nbComponent));
   guiGreenChannelChoice->value(std::min(channels[1] + 1, nbComponent));
   guiBlueChannelChoice->value(std::min(channels[2] + 1, nbComponent));

 }

 /**
  * GrayScale();
  */
 void
 ViewerViewGUI
 ::GrayScaleSet()
 {
   unsigned int selectedItem = guiImageList->value();
   if (selectedItem == 0)
     {
       // no image selected, return
       return;
     }
   unsigned int nbComponent = m_ViewerModel->GetObjectList().at(selectedItem-1).pReader->GetOutput()->GetNumberOfComponentsPerPixel();

   guiViewerSetupGrayscaleMode->set();
   guiViewerSetupComplexMode->clear();
   guiViewerSetupColorMode->clear();

   guiRealChannelChoice->deactivate();
   guiImaginaryChannelChoice->deactivate();
   bAmplitude->deactivate();
   bPhase->deactivate();
   guiRedChannelChoice->deactivate();
   guiGreenChannelChoice->deactivate();
   guiBlueChannelChoice->deactivate();

   guiGrayscaleChannelChoice->activate();

   assert(m_ViewerModel->GetObjectList().at(selectedItem-1).pRenderFunction.GetPointer());

   RenderingFunctionType::Pointer renderingFunction = m_ViewerModel->GetObjectList().at(selectedItem-1).pRenderFunction;

   ChannelListType channels = renderingFunction->GetChannelList();
   if (channels.size() < 1)
   {
     channels.push_back(0);
   }

   guiGrayscaleChannelChoice->value(std::min(channels[0] + 1, nbComponent));
 }


 void
 ViewerViewGUI
 ::ComplexSet()
 {
   unsigned int selectedItem = guiImageList->value();
   if (selectedItem == 0)
   {
     // no image selected, return
     return;
   }
   unsigned int nbComponent = m_ViewerModel->GetObjectList().at(selectedItem-1).pReader->GetOutput()->GetNumberOfComponentsPerPixel();

   guiViewerSetupComplexMode->set();
   guiViewerSetupColorMode->clear();
   guiViewerSetupGrayscaleMode->clear();
   guiGrayscaleChannelChoice->deactivate();
   guiRedChannelChoice->deactivate();
   guiGreenChannelChoice->deactivate();
   guiBlueChannelChoice->deactivate();
   guiRealChannelChoice->activate();
   guiImaginaryChannelChoice->activate();
   bAmplitude->activate();
   bPhase->activate();


   assert(m_ViewerModel->GetObjectList().at(selectedItem-1).pRenderFunction.GetPointer());
   RenderingFunctionType::Pointer renderingFunction = m_ViewerModel->GetObjectList().at(selectedItem-1).pRenderFunction;

   ChannelListType channels = renderingFunction->GetChannelList();
   if (channels.size() == 0)
   {
      channels.push_back(0);
      channels.push_back(1);
   }
   else if (channels.size() == 1)
   {
      channels.push_back(1);
   }

   guiRealChannelChoice->value(std::min(channels[0] + 1, nbComponent));
   guiImaginaryChannelChoice->value(std::min(channels[1] + 1, nbComponent));
 }


 /**
  * ViewerSetup();
  */
 void
 ViewerViewGUI
 ::ViewerSetup()
 {
   unsigned int selectedItem = guiImageList->value();
   if (selectedItem == 0)
   {
     // no image selected, return
     return;
   }
   guiViewerSetupWindow->show();
 }

 void
 ViewerViewGUI
 ::ViewerSetupOk()
 {
   unsigned int selectedItem = guiImageList->value();
   if (selectedItem == 0)
     {
     // no image selected, return
     return;
     }

   if (guiViewerSetupColorMode->value())
     {
     int redChoice = static_cast<int>(guiRedChannelChoice->value() - 1);
     int greenChoice = static_cast<int>(guiGreenChannelChoice->value() - 1);
     int blueChoice = static_cast<int>(guiBlueChannelChoice->value() - 1);
     m_ViewerController->UpdateRGBChannelOrder(redChoice, greenChoice, blueChoice, selectedItem);
     m_ViewerModel->GetObjectList().at(selectedItem-1).pViewType = ViewerModel::RGB;
     }
   else if (guiViewerSetupGrayscaleMode->value())
     {
     int choice = static_cast<int>(guiGrayscaleChannelChoice->value() - 1);
     m_ViewerController->UpdateGrayScaleChannelOrder(choice, selectedItem);
     m_ViewerModel->GetObjectList().at(selectedItem-1).pViewType = ViewerModel::Grayscale;
     }
   else if (guiViewerSetupComplexMode->value())
     {
     int realChoice = static_cast<int>(guiRealChannelChoice->value() - 1);
     int imagChoice = static_cast<int>(guiImaginaryChannelChoice->value() - 1);
     if (bAmplitude->value())
            {
             m_ViewerController->UpdateAmplitudeChannelOrder(realChoice, imagChoice, selectedItem);
             m_ViewerModel->GetObjectList().at(selectedItem-1).pViewType = ViewerModel::ComplexMod;
            }
       else
            {
            m_ViewerModel->GetObjectList().at(selectedItem-1).pViewType = ViewerModel::ComplexPhase;
            m_ViewerController->UpdatePhaseChannelOrder(realChoice, imagChoice, selectedItem);
            }
     }
   // Update image info, to update the colored composition
   this->UpdateInformation( selectedItem );
 }

 void
 ViewerViewGUI
 ::ViewerSetupCancel()
 {
   guiViewerSetupWindow->hide();
 }


void
ViewerViewGUI
::Diaporama()
{
  if (guiImageList->size() == 0 || guiDiaporama->shown())
    {
    // no image selected, return
    return;
    }

  if(guiLinkSetupWindow->shown() == 0)
    {
    guiDiaporama->show();

    //Close the showed image without clearing the ShowedList
    this->CloseAllDisplayedImages(false);

    //Show the diaporama widget
    this->DisplayDiaporama();

    UpdateDiaporamaProgressBar();
    }
}


 /**
  * Cut a path to get only the imageName
  */
 const char *
 ViewerViewGUI
 ::CutFileName(unsigned int selectedItem)
 {
   std::string fileName     = m_ViewerModel->GetObjectList().at(selectedItem).pFileName;
   std::string  fileNameCut = itksys::SystemTools::GetFilenameName(fileName);
   return fileNameCut.c_str();
 }

 std::string
  ViewerViewGUI
 ::CutFileName2(unsigned int selectedItem)
 {
   std::string fileName     = m_ViewerModel->GetObjectList().at(selectedItem).pFileName;
   return itksys::SystemTools::GetFilenameName(fileName);
 }

void
ViewerViewGUI
::DisplayDiaporama()
{
  const ObjectsTrackedType & objTracked = m_ViewerModel->GetObjectList().at(m_DiaporamaCurrentIndex);
  //Get the view stored in the model
  CurvesWidgetType::Pointer curveWidget     = objTracked.pCurveWidget;
  VisuViewPointerType       currentVisuView = objTracked.pVisuView;

  //First get the histogram list
  RenderingFunctionType::Pointer pRenderingFunction = objTracked.pRenderFunction;

  HistogramCurveType::Pointer rhistogram = HistogramCurveType::New();
  HistogramCurveType::Pointer ghistogram = HistogramCurveType::New();
  HistogramCurveType::Pointer bhistogram = HistogramCurveType::New();

  //Color Definition
  HistogramCurveType::ColorType Red;
  HistogramCurveType::ColorType Green;
  HistogramCurveType::ColorType Blue;
  Red.Fill(0);
  Green.Fill(0);
  Blue.Fill(0);
  Red[0]  = 1.;   Red[3]   = 0.5;
  Green[1]= 1.;   Green[3] = 0.5;
  Blue[2] = 1.;   Blue[3]  = 0.5;

  ghistogram->SetHistogramColor(Green);
  ghistogram->SetLabelColor(Green);
  bhistogram->SetHistogramColor(Blue);
  bhistogram->SetLabelColor(Blue);
  rhistogram->SetHistogramColor(Red);
  rhistogram->SetLabelColor(Red);

  const unsigned int nbBands = objTracked.pLayer->GetHistogramList()->Size();

  curveWidget->ClearAllCurves();

  if(  nbBands == 0 )
    {
    itkExceptionMacro("No bands detected in asked m_ViewerModel->GetObjectList() (index "<<m_DiaporamaCurrentIndex<<")");
    }

  if(  nbBands >= 1 )
    {
    rhistogram->SetHistogram(objTracked.pLayer->GetHistogramList()->GetNthElement(0));
    curveWidget->AddCurve(rhistogram);
    }

  if( nbBands >= 2 )
    {
    ghistogram->SetHistogram(objTracked.pLayer->GetHistogramList()->GetNthElement(1));
    curveWidget->AddCurve(ghistogram);
    }

  if( nbBands >= 3 )
    {
    bhistogram->SetHistogram(objTracked.pLayer->GetHistogramList()->GetNthElement(2));
    curveWidget->AddCurve(bhistogram);
    }

  curveWidget->SetXAxisLabel("Pixels");
  curveWidget->SetYAxisLabel("Frequency");

  //Get the pixelView
  PixelDescriptionViewType::Pointer pixelView = objTracked.pPixelView;

  //Edit the Widget
  m_Widget->UnRegisterAll();
  m_Widget->RegisterFullWidget(currentVisuView->GetFullWidget());
  m_Widget->RegisterScrollWidget(currentVisuView->GetScrollWidget());
  m_Widget->RegisterZoomWidget(currentVisuView->GetZoomWidget());
  m_Widget->RegisterPixelDescriptionWidget(pixelView->GetPixelDescriptionWidget());
  m_Widget->RegisterHistogramWidget(curveWidget);
  m_Widget->SetLabel(this->CutFileName(m_DiaporamaCurrentIndex));
  m_Widget->Refresh();
  m_Widget->Show();
}

void
ViewerViewGUI
::DiaporamaNext()
{
  if (m_DiaporamaCurrentIndex < static_cast<unsigned int>(guiImageList->size())-1)
  {
    ++m_DiaporamaCurrentIndex;
    this->DisplayDiaporama();
  }
  this->UpdateDiaporamaProgressBar();
}

void
ViewerViewGUI
::DiaporamaPrevious()
{
  if (m_DiaporamaCurrentIndex>0)
  {
    --m_DiaporamaCurrentIndex;
    this->DisplayDiaporama();
  }
  this->UpdateDiaporamaProgressBar();
}


void
ViewerViewGUI
::UpdateDiaporamaProgressBar()
{
  std::ostringstream oss;
  oss.str("");
  oss<<m_DiaporamaCurrentIndex+1<<"/"<<guiImageList->size();
  guiDiaporamaProgressBar->copy_label(oss.str().c_str());
  guiDiaporamaProgressBar->minimum(1.);
  guiDiaporamaProgressBar->maximum(static_cast<float>(guiImageList->size()));
  guiDiaporamaProgressBar->value(static_cast<float>(m_DiaporamaCurrentIndex));
}

void
ViewerViewGUI
::DiaporamaQuit()
{
  guiDiaporama->hide();
  m_Widget->Hide();
  m_Widget->UnRegisterAll();
  this->ShowTemporaryClosedDisplay();
}

/**
 *
 */

void
ViewerViewGUI
::LinkSetup()
{
  unsigned int selectedItem = guiImageList->value();
  if (selectedItem == 0)
  {
    // no image selected, return
    return;
  }
  if(guiDiaporama->shown() == 0 )
    guiLinkSetupWindow->show();
}


/**
 *
 */
void
ViewerViewGUI
::UpdateLinkSetupWindow()
{
  std::ostringstream oss;
  oss.str("");
  guiLinkListLeft->clear();
  guiLinkListRight->clear();
  guiLinkListLeft->value(0);
  guiLinkListRight->value(0);

  //Fill the guiLinkList

  for (int i = 0; i < guiImageList->size(); i++)
    {
      oss.str("");
      oss<<this->CutFileName(i);
      guiLinkListRight->add(oss.str().c_str());
      guiLinkListLeft->add(oss.str().c_str());
    }

  guiLinkXOffset->value("0");
  guiLinkYOffset->value("0");

  guiLinkListLeft->redraw();
  guiLinkListRight->redraw();
}

/**
*
*/
void
ViewerViewGUI
::LinkSetupSave()
{
  unsigned int leftChoice = guiLinkListLeft->value();
  unsigned int rightChoice = guiLinkListRight->value();

  if(leftChoice == 0 || rightChoice == 0)
    return;
  else
    {
      //Fill the offset
      OffsetType                      offSet;
      offSet[0] = atoi(guiLinkXOffset->value());
      offSet[1] = atoi(guiLinkYOffset->value());

      //Update a list to

      //Call the controller
      this->m_ViewerController->Link(leftChoice, rightChoice, offSet);

      //Close the temporary Showed images without clearing the showed list
      this->CloseAllDisplayedImages(false);

      //Add the linked image to the list
      m_LinkedDisplayStatusList[leftChoice-1] = true;
      m_LinkedDisplayStatusList[rightChoice-1] = true;

      //store  the images linked together
      UIntPairType        linkPair(leftChoice, rightChoice);
      m_LinkedImageList.push_back(linkPair);

      //Diplay the Linked images
      this->Display(m_LinkWidgetManagerList, leftChoice);
      this->Display(m_LinkWidgetManagerList, rightChoice);
    }
}

/**
*
*/
void
ViewerViewGUI
::LinkSetupOk()
{
  guiLinkSetupWindow->hide();

  //Close the  displays linked
  for(unsigned int i = 0; i<m_LinkedDisplayStatusList.size(); i++)
    {
      if(m_LinkedDisplayStatusList[i])
       {
         m_LinkWidgetManagerList->GetNthElement(i)->Hide();
         m_LinkedDisplayStatusList[i] = false;
       }
    }
  //Display temporary closed displays if any
  if(guiImageList->size()>0)
    this->ShowTemporaryClosedDisplay();
}

/**
 *
 */
void
ViewerViewGUI
::LinkSetupRemove()
{
  //Close the  displays linked
  for(unsigned int i = 0; i<m_LinkedDisplayStatusList.size(); i++)
    if(m_LinkedDisplayStatusList[i])
      {
       m_LinkWidgetManagerList->GetNthElement(i)->Hide();
       m_LinkedDisplayStatusList[i] = false;
      }
}

/**
 *
 */
void
ViewerViewGUI
::InitializeImageController(unsigned int selectedItem)
{
  std::vector<unsigned int>  tempElementToRemove;

  for(unsigned int i = 0; i< m_LinkedImageList.size(); i++ )
    {
      if(m_LinkedImageList[i].first == selectedItem || m_LinkedImageList[i].second == selectedItem)
       {

         if(m_LinkedImageList[i].first == selectedItem )
           m_ViewerController->UpdateImageViewController(m_LinkedImageList[i].second);

         if(m_LinkedImageList[i].second == selectedItem)
           m_ViewerController->UpdateImageViewController(m_LinkedImageList[i].first);

         tempElementToRemove.push_back(i);
       }
    }

  //Remove element from the list
  unsigned counter = 0;
  for(unsigned int p = 0; p < tempElementToRemove.size(); p ++ )
    {
    // counter because the size of the list is decreasing after each iteration
    m_LinkedImageList.erase(m_LinkedImageList.begin()+tempElementToRemove[p] - counter);
    counter++;
    }

  m_LinkedImageList.clear();
}


/**
 * PrintSelf Method
 */
void
ViewerViewGUI
::PrintSelf(std::ostream& os, itk::Indent indent) const
{
  Superclass::PrintSelf(os, indent);
}
} // End namespace otb
