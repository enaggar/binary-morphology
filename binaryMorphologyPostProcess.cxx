#include "itkImage.h"
#include "itkBinaryMorphologicalOpeningImageFilter.h"
#include "itkBinaryBallStructuringElement.h"
#include "itkSubtractImageFilter.h"

//#include "QuickView.h" /*Need VTK*/

#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkRescaleIntensityImageFilter.h"

#include "itkConstNeighborhoodIterator.h"
#include "itkImageRegionIterator.h"

//namespace {
//	typedef itk::Image<unsigned char, 2> ImageType;
//}

typedef float															PixelType;
typedef itk::Image<PixelType, 2> 					ImageType;
typedef itk::ImageFileReader<ImageType> 	ReaderType;

typedef itk::ConstNeighborhoodIterator< ImageType >	NeighborhoodIteratorType;
typedef itk::ImageRegionIterator< ImageType>				IteratorType; 

/*
arg1 --> Itk Image File
arg2 --> Label File
arg3 --> radius size
*/
int main (int argc, char ** argv) {

	if (argc < 4) {
		std::cerr << "Missing Arguments\n";
		return EXIT_FAILURE;
	}

	ReaderType::Pointer reader = ReaderType::New();
	reader->SetFileName(argv[1]);
	try {
		reader->Update();
	} catch (itk::ExceptionObject &err) {
		std::cerr << "ExceptionObject caught !" << std::endl;
		std::cerr << err << std::endl;
		return EXIT_FAILURE;
	}

	FILE *label_file = fopen (argv[2], "r");
	if (label_file == NULL) {
		std::cerr << "Label File Not Found\n";
		return EXIT_FAILURE;
	}

	int structure_radius;
	if (!sscanf (argv[3], "%d", &structure_radius)) {
		std::cerr << "Structure Element Radius not provided\n";
		return EXIT_FAILURE;
	}

	NeighborhoodIteratorType::RadiusType iterator_radius;
	iterator_radius.Fill (1);
	NeighborhoodIteratorType it(
		iterator_radius,
		reader->GetOutput(),
		reader->GetOutput()->GetRequestedRegion()
		);

	ImageType::Pointer binary_image = ImageType::New();
	binary_image->SetRegions(reader->GetOutput()->GetRequestedRegion());
	binary_image->Allocate();

	IteratorType bin(binary_image, reader->GetOutput()->GetRequestedRegion());	

	for (it.GoToBegin(), bin.GoToBegin();!it.IsAtEnd();++it, ++bin) {
		/*
		int label;
		fscanf (label_file, "%d", &label);
		*/
		if (true/*label*/) {
			bin.Set(50);
		} else {
			continue;
		}
	}

	typedef itk::BinaryBallStructuringElement<PixelType, 2> StructuringElementType;
	StructuringElementType structuringElement;
	structuringElement.SetRadius(structure_radius);
	structuringElement.CreateStructuringElement();

	typedef itk::BinaryMorphologicalOpeningImageFilter <ImageType, ImageType, StructuringElementType>
					BinaryMorphologicalOpeningImageFilterType;
	BinaryMorphologicalOpeningImageFilterType::Pointer openingFilter
					= BinaryMorphologicalOpeningImageFilterType::New();
	openingFilter->SetInput(binary_image);
	openingFilter->SetKernel(structuringElement);
	openingFilter->Update();

	typedef itk::SubtractImageFilter<ImageType> SubtractType;
	SubtractType::Pointer diff = SubtractType::New();
	diff->SetInput1(binary_image);
	diff->SetInput2(openingFilter->GetOutput());

	/*QuickView viewer;
	std::stringstream desc;
	desc << "Original ";
	viewer.AddImage(binary_image.GetPointer(),
									true,
									desc.str());

	std::stringstream desc2;
	desc2 << "BinaryOpenining, radius = " << structure_radius;
	viewer.AddImage(openingFilter->GetOutput().
									true,
									desc2.str());

	std::stringstream desc3;
	desc3 << "Original - BinaryOpening";
	viewer.AddImage(diff->GetOutput(),
									true,
									desc3.str());
	viewer.Visualize();*/

	return EXIT_SUCCESS;
}