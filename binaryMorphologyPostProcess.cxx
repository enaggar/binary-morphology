#include "itkImage.h"
#include "itkBinaryMorphologicalClosingImageFilter.h"
#include "itkBinaryBallStructuringElement.h"
#include "itkSubtractImageFilter.h"

#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkRescaleIntensityImageFilter.h"

#include "itkImageRegionIterator.h"

//namespace {
//	typedef itk::Image<unsigned char, 2> ImageType;
//}

typedef unsigned char															PixelType;
typedef itk::Image<PixelType, 3> 					ImageType;
typedef itk::ImageFileReader<ImageType> 	ReaderType;

typedef unsigned char 																	WritePixelType;
typedef itk::Image<WritePixelType, 3>						WriteImageType;
typedef itk::ImageFileWriter<WriteImageType>	WriterType;

typedef itk::ImageRegionIterator< ImageType>				IteratorType; 

/*
arg1 --> Itk Image File
arg2 --> radius1 size
arg3 --> radius2 size
arg4 --> radius3 size
*/


void calculateAccHealthVsNonHealth(std::vector<int> predicted , std::vector<int> truth,std::string toBePrint , int index,std::vector<double > &output)
{
 
	double P_health = 0.0;
	double P_nonHealth = 0.0;
 
	double PT_health = 0.0;
	double PT_nonHealth = 0.0;
 
	double T_health = 0.0;
	double T_nonHealth = 0.0;
 
    int n = truth.size();
    for(int i = 0; i < n; i++){
        if(truth[i] == 0){//health
            T_health++;
            if(truth[i] == predicted[i]){
                PT_health++;
            }
        }else{
            T_nonHealth++;
            if(truth[i] == predicted[i]){
                PT_nonHealth++;
            }
        }
    }
 
    for(int i = 0; i < n; i++){
        if(predicted[i] == 0){//health
            P_health++;
        }else{
            P_nonHealth++;
        }
    }
    double dice, sens, spec;
    std::cout<<"Stats for "<<toBePrint<<" labels"<<std::endl;
    dice = (PT_nonHealth)/((T_nonHealth + P_nonHealth)/2);
    sens = PT_nonHealth/T_nonHealth;
    spec = (PT_health)/(T_health);
    output[index]=output[index]+dice;
    output[index+1]=output[index+1]+sens;
     output[index+2]=output[index+2]+spec;
    std::cout<<"Dice: "<<dice<<std::endl;
    std::cout<<"Senstivity: "<<sens<<std::endl;
    std::cout<<"Spec.: "<<spec<<std::endl;
   /* std::cout<<"Stats for Health label"<<std::endl;
    dice = (PT_health)/((T_health + P_health)/2);
    sens = PT_health/T_health;
    spec = (PT_nonHealth)/(T_nonHealth);
    std::cout<<"Dice: "<<dice<<std::endl;
    std::cout<<"Senstivity: "<<sens<<std::endl;
    std::cout<<"Spec.: "<<spec<<std::endl;
 */
 
}
 
int main (int argc, char ** argv) {

	if (argc < 3) {
		std::cerr << "Missing Arguments\n";
		return EXIT_FAILURE;
	}
	int numberOfPatients=0;
	std::ifstream fileToRead(argv[1]);
	std::vector<double > results(9 , 0);
	std::vector<std::vector<double> > eachPatientResults;
	std::string textFileName="";
	std::string mhaFileName="";
	
	while(!fileToRead.eof())
	{
		std::cout<<"starting patient id = "<<numberOfPatients<<std::endl;
		ReaderType::Pointer reader = ReaderType::New();
		fileToRead>>textFileName;
		const char *textFileNamePointer=textFileName.c_str();
		if(fileToRead.eof())
		{
			break;
		}
		fileToRead>>mhaFileName;
		reader->SetFileName(mhaFileName);
		try {
			reader->Update();
		} catch (itk::ExceptionObject &err) {
			std::cerr << "ExceptionObject caught !" << std::endl;
			std::cerr << err << std::endl;
			return EXIT_FAILURE;
		}

		FILE *lable_file = fopen (textFileNamePointer, "r");
		if (lable_file == NULL) {
			std::cerr << "lable File Not Found\n";
			return EXIT_FAILURE;
		}

		int structure_radius[3];
		for (int i=2;i<5;i++) {
			if (!sscanf (argv[i], "%d", &structure_radius[i-2])) {
				std::cerr << "Structure Element Radius " << (i-1) << " not provided\n";
				return EXIT_FAILURE;
			}
		}
		
		

		printf ("%s %s %d %d %d\n", argv[1], argv[2], structure_radius[0], structure_radius[1], structure_radius[2]);

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
		
		
		ImageType::Pointer core_image = ImageType::New();
		core_image->SetRegions(reader->GetOutput()->GetRequestedRegion());
		core_image->Allocate();

		IteratorType core_image_itr(
			core_image,
			reader->GetOutput()->GetRequestedRegion()
		);	



		ImageType::Pointer enhanced_image = ImageType::New();
		enhanced_image->SetRegions(reader->GetOutput()->GetRequestedRegion());
		enhanced_image->Allocate();

		IteratorType enhanced_image_itr(
			enhanced_image,
			reader->GetOutput()->GetRequestedRegion()
		);	
	ImageType::Pointer main_image = ImageType::New();
		main_image->SetRegions(reader->GetOutput()->GetRequestedRegion());
		main_image->Allocate();

		IteratorType main(
			main_image,
			reader->GetOutput()->GetRequestedRegion()
		);


		char line[20];
		int read = 0;
		int empty = 0;
		float val = 0;
		std::vector<int> tumor;
		std::vector<int> tumorTruth;
		std::vector<int> core;
		std::vector<int> coreTruth;
		std::vector<int> enhanced;
		std::vector<int> enhancedTruth;
		int enhancedTruthCounter=0;
        int classiferEnhancedCounter=0;
		for (it.GoToBegin(), bin.GoToBegin(),core_image_itr.GoToBegin(), main.GoToBegin(),enhanced_image_itr.GoToBegin();!it.IsAtEnd();
		++enhanced_image_itr,++it,++core_image_itr, ++bin, ++main) {
			unsigned char cur_pixel = it.Get();
			int pred, real;
			if (fabs(cur_pixel-255) <= 1e-5) {
				fgets (line, 20, lable_file);
				int succ = sscanf (line, "\"lable%d\" \"lable%d\"", &pred, &real);
				if (succ != 2) {
					printf ("ERROR\n");
				}
				if (pred != 0) {
					tumor.push_back(1);
					
					read ++;
					bin.Set(255);
				} else {
					bin.Set(0);
					tumor.push_back(0);
					val += cur_pixel;
					empty ++;
				}
				
				if (real != 0) {
					tumorTruth.push_back(1);
					main.Set(255);
				} else {
					tumorTruth.push_back(0);
					main.Set(0);
				}
				// core
				if(pred==1 || pred==3 || pred==4)
				{
					core_image_itr.Set(255);
					core.push_back(1);
					
				}
				else
				{
					core_image_itr.Set(0);
					core.push_back(0);
				}
				if(real==1 || real==3 || real==4)
				{
					coreTruth.push_back(1);
				}
				else
				{
					coreTruth.push_back(0);
				}
				
				if(pred==4)
				{
					enhanced_image_itr.Set(255);
					enhanced.push_back(1);
					classiferEnhancedCounter++;
				}
				else
				{
					enhanced_image_itr.Set(0);
					enhanced.push_back(0);
				}
				if(real==4)
				{
					enhancedTruth.push_back(1);
					enhancedTruthCounter++;

				}
				else
				{
					enhancedTruth.push_back(0);
				}
			}
		}
		std::vector< double > output2(9,0);
		calculateAccHealthVsNonHealth(tumor , tumorTruth,"tumor",0,output2 );
		calculateAccHealthVsNonHealth(core , coreTruth,"core" , 0,output2 );
		calculateAccHealthVsNonHealth(enhanced , enhancedTruth,"enhanced",0,output2);
		std::cout<<"*********************truth enhanced = "<<enhancedTruthCounter<<std::endl;
        std::cout<<"*********************classifed enhanced = "<<classiferEnhancedCounter<<std::endl;
		printf ("%d Pixels read, %d empty with val %f\n", read, empty, val);
		fclose (lable_file);

		typedef itk::BinaryBallStructuringElement<PixelType, 3> StructuringElementType;
		StructuringElementType structuringElement[3];
		for (int i=0;i<3;i++) {
			structuringElement[i].SetRadius(structure_radius[i]);
			structuringElement[i].CreateStructuringElement();
		}

		typedef itk::BinaryMorphologicalClosingImageFilter <ImageType, ImageType, StructuringElementType>
						BinaryMorphologicalClosingImageFilterType;
		BinaryMorphologicalClosingImageFilterType::Pointer closingFilter
						= BinaryMorphologicalClosingImageFilterType::New();
		closingFilter->SetInput(binary_image);
		closingFilter->SetKernel(structuringElement[0]);
		closingFilter->Update();
		
		
		BinaryMorphologicalClosingImageFilterType::Pointer closingFilterCore
						= BinaryMorphologicalClosingImageFilterType::New();
		closingFilterCore->SetInput(core_image);
		closingFilterCore->SetKernel(structuringElement[1]);
		closingFilterCore->Update();



		BinaryMorphologicalClosingImageFilterType::Pointer closingFilterEnhancing
						= BinaryMorphologicalClosingImageFilterType::New();
		closingFilterEnhancing->SetInput(enhanced_image);
		closingFilterEnhancing->SetKernel(structuringElement[2]);
		closingFilterEnhancing->Update();

		typedef itk::SubtractImageFilter<ImageType> SubtractType;
		SubtractType::Pointer diff = SubtractType::New();
		diff->SetInput1(binary_image);
		diff->SetInput2(closingFilter->GetOutput());

		/*WriterType::Pointer writer = WriterType::New();
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
		writer->SetInput(closingFilter->GetOutput());
		try {
			writer->Update();
		} catch (itk::ExceptionObject &err){
			std::cerr << "ExceptionObject caught!" << std::endl;
			std::cerr << err << std::endl;
			return EXIT_FAILURE;
		}

		writer = WriterType::New();
		writer->SetFileName("main_image.mha");
		writer->SetInput(main_image);
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
*/
		IteratorType filtered_iterator(
			closingFilter->GetOutput(),
			reader->GetOutput()->GetRequestedRegion()
		);
		
		IteratorType filtered_core_iterator(
			closingFilterCore->GetOutput(),
			reader->GetOutput()->GetRequestedRegion()
		);
		
		IteratorType filtered_enhanced_iterator(
			closingFilterEnhancing->GetOutput(),
			reader->GetOutput()->GetRequestedRegion()
		);

		lable_file = fopen (textFileNamePointer, "r");
		FILE *new_file = fopen ("new_lable.txt", "w");
		std::vector<int > tumorBinary;
		std::vector<int > coreBinary;
		std::vector<int > enhancedBinary;
		int enhancedCounter=0;
		std::cout<<"dddddddddddddddddddddddddddddddddddggg"<<std::endl;
		for (filtered_enhanced_iterator.GoToBegin(),it.GoToBegin(), filtered_core_iterator.GoToBegin(),filtered_iterator.GoToBegin();!it.IsAtEnd();++filtered_core_iterator,++it, ++filtered_iterator,++filtered_enhanced_iterator) {
			unsigned char cur_pixel = filtered_iterator.Get();
			unsigned char cur_pixel_core = filtered_core_iterator.Get();
			unsigned char cur_pixel_enhanced = filtered_enhanced_iterator.Get();
			unsigned char ori_pixel = it.Get();
			if (fabs(ori_pixel-255) <= 1e-5) {
				
				fgets (line, 20, lable_file);
				int pred, real;
				sscanf (line, "lable%d lable%d", &pred, &real);
				if (fabs(cur_pixel-255.000) <= 1e-5 || fabs(cur_pixel_core-255.000) <= 1e-5 || fabs(cur_pixel_enhanced-255.000) <= 1e-5) {
					tumorBinary.push_back(1);
					//fprintf (new_file, "lable%d lable%d\n", pred, real);
				} else {
					tumorBinary.push_back(0);
					
					//fprintf (new_file, "lable0 lable%d\n", real);
				}
				if (fabs(cur_pixel_core-255.000) <= 1e-5 || fabs(cur_pixel_enhanced-255.000) <= 1e-5) {
					coreBinary.push_back(1);
					
					//fprintf (new_file, "lable%d lable%d\n", pred, real);
				} else {
					coreBinary.push_back(0);
					
					//fprintf (new_file, "lable0 lable%d\n", real);
				}
				
				if (fabs(cur_pixel_enhanced-255.000) <= 1e-5) {
					enhancedBinary.push_back(1);
					enhancedCounter++;
					//fprintf (new_file, "lable%d lable%d\n", pred, real);
				} else {
					enhancedBinary.push_back(0);
					
					//fprintf (new_file, "lable0 lable%d\n", real);
				}
				
			}
		}
		if(enhancedCounter==0)
		{
			enhancedBinary = enhanced;
		}
std::cout<<"ddddddddddddddddddddddddddddddddddd"<<std::endl;
		calculateAccHealthVsNonHealth(tumorBinary , tumorTruth,"tumor after postProcessing",0 , results);
		calculateAccHealthVsNonHealth(coreBinary , coreTruth,"core after postProcessing",3,results);
		calculateAccHealthVsNonHealth(enhancedBinary , enhancedTruth,"enhanced after postProcessing",6,results);
		printf ("New lable file created\n");
		std::vector<double> patientData(9,0);
		calculateAccHealthVsNonHealth(tumorBinary , tumorTruth,"tumor after postProcessing",0 , patientData);
		calculateAccHealthVsNonHealth(coreBinary , coreTruth,"core after postProcessing",3,patientData);
		calculateAccHealthVsNonHealth(enhancedBinary , enhancedTruth,"enhanced after postProcessing",6,patientData);
		eachPatientResults.push_back(patientData);
		
		numberOfPatients++;
		ImageType::Pointer outputImage = ImageType::New();
		outputImage->SetRegions(reader->GetOutput()->GetRequestedRegion());
		outputImage->Allocate();

		IteratorType outIt(outputImage, reader->GetOutput()->GetRequestedRegion());
		int index=0;
		for (outIt.GoToBegin(),it.GoToBegin();!outIt.IsAtEnd();++it,++outIt) 
		{
			unsigned char ori_pixel = it.Get();
			if(fabs(ori_pixel-255) <= 1e-5)
			{
				if(enhancedBinary[index]==1)
				{
					outIt.Set(4);
				}
				else if(coreBinary[index]==1)
				{
					outIt.Set(3);
				}
				else if(tumorBinary[index]==1)
				{
					outIt.Set(2);
				}
				else
				{
					outIt.Set(0);
				}
				
				index++;
			}
			else
			{
				outIt.Set(0);
			}
		}
		WriterType::Pointer writer = WriterType::New();
		writer->SetFileName("Final_image.mha");
		writer->SetInput(outputImage);
		try {
			writer->Update();
		} catch (itk::ExceptionObject &err){
			std::cerr << "ExceptionObject caught!" << std::endl;
			std::cerr << err << std::endl;
			return EXIT_FAILURE;
		}
		//return EXIT_SUCCESS;


}
std::cout<<numberOfPatients<<std::endl;

std::cout<<"average dice tumor = " <<results[0]/numberOfPatients<<std::endl;
std::cout<<"average Senstivity tumor = "<< results[1]/numberOfPatients<<std::endl;
std::cout<<"average Spec tumor = " <<results[2]/numberOfPatients<<std::endl;

std::cout<<"average dice core = " <<results[3]/numberOfPatients<<std::endl;
std::cout<<"average Senstivity core = "<< results[4]/numberOfPatients<<std::endl;
std::cout<<"average Spec core = " <<results[5]/numberOfPatients<<std::endl;

std::cout<<"average dice enhanced = " <<results[6]/numberOfPatients<<std::endl;
std::cout<<"average Senstivity enhanced = "<< results[7]/numberOfPatients<<std::endl;
std::cout<<"average Spec enhanced = " <<results[8]/numberOfPatients<<std::endl;



std::vector<double> meansResults;
for(int i=0;i<9;i++)
{
	meansResults.push_back(results[i]/numberOfPatients);
}
std::vector<double> varianceResults;

for(int i=0;i<9;i++)
{
	varianceResults.push_back(0);
	
	for(int j=0;j<eachPatientResults.size();j++)
	{
		varianceResults[i]+=((meansResults[i] - eachPatientResults[j][i])*(meansResults[i] - eachPatientResults[j][i]));
	}
	varianceResults[i]=varianceResults[i]/(numberOfPatients-1);
}

std::cout<<"variance dice tumor = " <<varianceResults[0]<<std::endl;
std::cout<<"variance Senstivity tumor = "<< varianceResults[1]<<std::endl;
std::cout<<"variance Spec tumor = " <<varianceResults[2]<<std::endl;

std::cout<<"variance dice core = " <<varianceResults[3]<<std::endl;
std::cout<<"variance Senstivity core = "<< varianceResults[4]<<std::endl;
std::cout<<"variance Spec core = " <<varianceResults[5]<<std::endl;

std::cout<<"variance dice enhanced = " <<varianceResults[6]<<std::endl;
std::cout<<"variance Senstivity enhanced = "<< varianceResults[7]<<std::endl;
std::cout<<"variance Spec enhanced = " <<varianceResults[8]<<std::endl;


}
