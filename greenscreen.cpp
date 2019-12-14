#include <opencv2/core/utility.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <string>
#include <iostream>
#include <chrono>
#include <vector>
#include <unordered_map>

class ChromaKeyAlg{

  virtual void applyGreenScreen(const cv::Mat& bkgdImage, 
																 const cv::Mat& fgImage, 
																 cv::Mat& outputImage) const = 0;
};

class CbCrChromaKeyAlg: public ChromaKeyAlg{

  public:

  CbCrChromaKeyAlg(const cv::Vec3b& BGRKey, int toleranceA = 46210, int toleranceB = 46240, int delta = 128)
    : BGRKey(BGRKey), toleranceA(toleranceA), toleranceB(toleranceB){
    CbKey = BGR2Cb(BGRKey, delta);
    CrKey = BGR2Cr(BGRKey, delta);
  }

  void applyGreenScreen(const cv::Mat& bkgdImage, const cv::Mat& fgImage, cv::Mat& outputImage) const override{

    for(int i = 0; i < fgImage.rows; ++i){
      for(int j = 0; j < fgImage.cols; ++j){
			  cv::Vec3b fgPixel			 = fgImage.at<cv::Vec3b>(i, j);
				cv::Vec3b bgkdPixel		 = bkgdImage.at<cv::Vec3b>(i, j);
				cv::Vec3b &outputPixel = outputImage.at<cv::Vec3b>(i, j);

				int Cb	 = (int) BGR2Cb(fgPixel, delta);
				int Cr	 = (int) BGR2Cr(fgPixel, delta);
				int mask = (int) 1 - CbCrPixelAlpha(Cb, Cr);

				outputPixel[0] = std::max(fgPixel[0] - mask*BGRKey[0], 0) + mask*bgkdPixel[0];
				outputPixel[1] = std::max(fgPixel[1] - mask*BGRKey[1], 0) + mask*bgkdPixel[1];
				outputPixel[2] = std::max(fgPixel[2] - mask*BGRKey[2], 0) + mask*bgkdPixel[2];
								
			}
		}
	}

  static float BGR2Cb(const cv::Vec3b& pixel, float delta){
	 
	  float Y  = BGR2Y(pixel);
		float Cb = (pixel[0] - Y) * 0.564f + delta;

		return Cb;
	}

  static float BGR2Cr(const cv::Vec3b& pixel, float delta){

	  float Y  = BGR2Y(pixel);
		float Cr = (pixel[2] - Y) * 0.713f + delta;

		return Cr;
	}

  static float BGR2Y(const cv::Vec3b& pixel){
	  
    float Y = 0.299f * pixel[2] + 0.587f * pixel[1] + 0.114f * pixel[0];
				
		return Y;
	}

		private:

  float CbCrPixelAlpha(int Cb, int Cr) const{
				
    float distance = CbCrDistance(Cb, Cr);
    
    if(distance < toleranceA){
		  return 0.0f;
		}
		else if(distance < toleranceB){
      return (distance	 - toleranceA)/
						 (toleranceB - toleranceA);
		}

	  return 1.0f;
	}

  float CbCrDistance(int Cb, int Cr) const{

	  float deltaCb = CbKey - Cb;
		float deltaCr = CrKey - Cr;

		return sqrt(deltaCb * deltaCb + deltaCr * deltaCr);
	}

	private:

	cv::Vec3b BGRKey;
	int				toleranceA;
	int				toleranceB;
	int				CbKey;
	int				CrKey;
	int				delta;
};

class HSVChromaKeyAlg: public ChromaKeyAlg{

  public:

  void applyGreenScreen(const cv::Mat& bkgdImage, 
                          const cv::Mat& fgImage,
                          cv::Mat& outputImage){}

  private:
}

class GreenScreenError{

  public:

	GreenScreenError(const std::string& errorMessage)
	  : errorMessage(errorMessage){
	}

	virtual const char* what(){
	  return errorMessage.c_str();
	}

	private:
		
  std::string errorMessage;
};

class GreenScreen{
  public:

	virtual void captureVideo() const = 0;
	virtual void captureVideo(const std::string& videoFile) const = 0;
};


class GreenScreenImage: public GreenScreen{
  public:
		
  GreenScreenImage(const std::string& bkgdImageFile, const cv::Vec3b& BGRKey)
				: chromaAlg(BGRKey){
				bkgdImage = cv::imread(bkgdImageFile);
				
	  if(!bkgdImage.data){
		  throw GreenScreenError("Invalid background file");
			}
	}

	void captureVideo() const override{

	  cv::VideoCapture capture(0);
		if(!capture.isOpened()){
		  throw GreenScreenError("Camera failed to open");
		}

		while(true){
		  cv::Mat frame;		
			cv::Mat scaledImage;

			capture.read(frame);
			cv::resize(bkgdImage,
			scaledImage,
			frame.size());
						
      cv::Mat outputImage(frame.size(), frame.type());
			chromaAlg.applyGreenScreen(scaledImage, frame, outputImage);
			cv::imshow("frame", frame); 

			if(cv::waitKey(30) >= 0)
			  break;
			}
  }

  void captureVideo(const std::string& videoFile) const override{
				
	  cv::VideoCapture capture(videoFile);
		if(!capture.isOpened()){
		  throw GreenScreenError("Invalid video file");
		}

		while(capture.isOpened()){
						
		  cv::Mat frame;		
			cv::Mat scaledImage;	

			capture.read(frame);
			cv::resize(bkgdImage,
			scaledImage,
			frame.size());

			cv::imshow("frame", frame);								 
		}
	}
  
  private:

	cv::Mat					 bkgdImage;
	CbCrChromaKeyAlg chromaAlg;
};

int main(int argc, char* argv[]){

  cv::Vec3b BGRKey(26, 255, 83);
  cv::Mat fgImage		= cv::imread("guy.jpg");
	cv::Mat bkgdImage = cv::imread("outside.jpg"); 
	cv::Mat scaledImage;
	cv::Mat outputImage(fgImage.size(), fgImage.type());
		
  cv::resize(bkgdImage,
						 scaledImage,
						 fgImage.size());

	CbCrChromaKeyAlg chromaAlg(BGRKey);

	if(scaledImage.size() != fgImage.size() || fgImage.size() != outputImage.size()){
	  std::cerr << "Size is invalid!\n";
		return EXIT_FAILURE;
	}

	chromaAlg.applyGreenScreen(scaledImage, fgImage, outputImage);

	cv::imshow("GreenScreen", outputImage);
	cv::imshow("Foreground", fgImage);
	cv::imshow("Backgroudn", bkgdImage);
	cv::imshow("ScaledImage", scaledImage);

	cv::waitKey(0);
	return 0;
}
