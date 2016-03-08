#include "itkImage.h"
#include "itkBinaryMorphologicalOpeningImageFilter.h"
#include "itkBinaryBallStructuringElement.h"
#include "itkSubtractImageFilter.h"

#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkRescaleIntensityImageFilter.h"

#include "itkImageRegionIterator.h"

//namespace {
//	typedef itk::Image<unsigned char, 2> ImageType;
//}

typedef float															PixelType;
typedef itk::Image<PixelType, 3> 					ImageType;
typedef itk::ImageFileReader<ImageType> 	ReaderType;

typedef float 																	WritePixelType;
typedef itk::Image<WritePixelType, 3>						WriteImageType;
typedef itk::ImageFileWriter<WriteImageType>	WriterType;

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

	printf ("%s %s %d\n", argv[1], argv[2], structure_radius);

	IteratorType it(
		reader->GetOutput(),
		reader->GetOutput()->GetRequestedRegion()
		);

	ImageType::Pointer binary_image = ImageType::New();
	binary_image->SetRegions(reader->GetOutput()->GetRequestedRegion());
	binary_image->Allocate();

	IteratorType bin(binary_image, reader->GetOutput()->GetRequestedRegion());	

	char line[20];
	int read = 0;
	int empty = 0;
	float val = 0;
	for (it.GoToBegin(), bin.GoToBegin();!it.IsAtEnd();++it, ++bin) {
		float cur_pixel = it.Get();
		if (fabs(cur_pixel-255.000) <= 1e-5) {
			read ++;
			fgets (line, 20, label_file);
			int label;
			sscanf (line, "lable%d", &label);
			if (label) {
				bin.Set(50);
			} else {
				continue;
			}
		} else {
			val += cur_pixel;
			empty ++;
		}
	}
	printf ("%d Pixels read, %d empty with val %f\n", read, empty, val);

	typedef itk::BinaryBallStructuringElement<PixelType, 3> StructuringElementType;
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

	WriterType::Pointer writer = WriterType::New();
	writer->SetFileName("binary_image.mha");
	writer->SetInput(binary_image);
	try {
		writer->Update();
	} catch (itk::ExceptionObject &err){
		std::cerr << "ExceptionObject caught !" << std::endl;
		std::cerr << err << std::endl;
		return EXIT_FAILURE;
	}

	writer = WriterType::New();
	writer->SetFileName("filtered_image.mha");
	writer->SetInput(openingFilter->GetOutput());
	try {
		writer->Update();
	} catch (itk::ExceptionObject &err){
		std::cerr << "ExceptionObject caught!" << std::endl;
		std::cerr << err << std::endl;
		return EXIT_FAILURE;
	}

	writer = WriterType::New();
	writer->SetFileName("difference.mha");
	writer->SetInput(diff->GetOutput());
	try {
		writer->Update();
	} catch (itk::ExceptionObject &err) {
		std::cerr << "ExceptionObject caught!" << std::endl;
		std::cerr << err << std::endl;
		return EXIT_FAILURE;
	}


	return EXIT_SUCCESS;
}