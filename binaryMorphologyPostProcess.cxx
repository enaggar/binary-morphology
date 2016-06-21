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

typedef unsigned char											PixelType;
typedef itk::Image<PixelType, 3> 					ImageType;
typedef itk::ImageFileReader<ImageType> 	ReaderType;

typedef unsigned char											WritePixelType;
typedef itk::Image<WritePixelType, 3>						WriteImageType;
typedef itk::ImageFileWriter<WriteImageType>	WriterType;

typedef itk::ImageRegionIterator< ImageType>				IteratorType; 

/*
arg1 --> Itk Image File
arg3 --> radius size
*/
int main (int argc, char ** argv) {

	if (argc < 3) {
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

	int structure_radius;
	if (!sscanf (argv[2], "%d", &structure_radius)) {
		std::cerr << "Structure Element Radius not provided\n";
		return EXIT_FAILURE;
	}

	printf ("%s %d\n", argv[1], structure_radius);

	IteratorType it(
		reader->GetOutput(),
		reader->GetOutput()->GetRequestedRegion()
	);

	ImageType::Pointer binary_image = ImageType::New();
	binary_image->SetRegions(reader->GetOutput()->GetRequestedRegion());
	binary_image->Allocate();

	IteratorType bin(
		binary_image,
		reader->GetOutput()->GetRequestedRegion()
	);	

	char line[20];
	int read = 0;
	int empty = 0;
	float val = 0;
	for (it.GoToBegin(), bin.GoToBegin();!it.IsAtEnd();++it, ++bin) {
		PixelType cur_pixel = it.Get();
		int pred, real;
		if (cur_pixel != 0) {
			bin.Set(255);
		}
	}

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
	writer->SetUseCompression(true);
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
	writer->SetUseCompression(true);
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
	writer->SetUseCompression(true);
	try {
		writer->Update();
	} catch (itk::ExceptionObject &err) {
		std::cerr << "ExceptionObject caught!" << std::endl;
		std::cerr << err << std::endl;
		return EXIT_FAILURE;
	}

	IteratorType filtered_iterator(
		openingFilter->GetOutput(),
		reader->GetOutput()->GetRequestedRegion()
	);

	ImageType::Pointer final_image = ImageType::New();
	final_image->SetRegions(reader->GetOutput()->GetRequestedRegion());
	final_image->Allocate();

	IteratorType fin(
		final_image,
		reader->GetOutput()->GetRequestedRegion()
	);	

	for (it.GoToBegin(), filtered_iterator.GoToBegin(), fin.GoToBegin();!it.IsAtEnd();++it, ++filtered_iterator, ++fin) {
		PixelType cur_pixel = filtered_iterator.Get();
		PixelType ori_pixel = it.Get();
		if (ori_pixel != 0) {
			if (cur_pixel != 0) {
				fin.Set(ori_pixel);
			}
		} else if (cur_pixel != 0) {
			fin.Set(1);
		} else {
			fin.Set(0);
		}
	}

	writer = WriterType::New();
	writer->SetFileName("final_image.mha");
	writer->SetInput(final_image);
	writer->SetUseCompression(true);
	try {
		writer->Update();
	} catch (itk::ExceptionObject &err){
		std::cerr << "ExceptionObject caught!" << std::endl;
		std::cerr << err << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}