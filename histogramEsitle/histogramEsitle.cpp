#include "opencv2/imgcodecs.hpp"
#include "opencv2/opencv.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/imgproc/imgproc_c.h"
#include <iostream>
#include <pthread.h>
#include <ctime>
#include <iomanip>
#include <stdexcept>


// 8 bitlik görüntünün renk derinliği tanımlaması yapılmıştır.
#define DEPTH 256

//namespace tanımlamaları
using namespace std;
using namespace cv;


//Görselle ilgili ilk işlemler ve görseli ekrana verme için sınıf tanımlanmıştır. 
//Histogram initalize etme ve hesaplaması için yardımcı fonksiyonlar bu sınıf içindedir.
class Image {
private:
	//İşlem yapılan görseller ve histogramları için mat tipinden tanımlamalar yapıldı.
	cv::Mat original, output, histGraph, histGraphOutput;
	//histogram derinliğinden dizi boyutu belirlendi
	double histogram[DEPTH];

	//dizinin bütün değerleri 0'la dolduruldu
	void initializeHist() {
		for(int i=0; i<DEPTH; i++)
			histogram[i] = 0;
	}
	
	//verilen görselin satır ve sütununda gezildi ve histogram dizine atama yapıldı
	void histHesapla() {
		for(int i=0; i<original.cols; i++)
			for(int j=0; j<original.rows; j++)
				histogram[original.at<uchar>(i, j)]++;
	}

	//renkler üzerinde oynama yapıldı ve ilgili dizi indexine tekrar atama yapıldı.
	void histogramEsitleHesapla() {
		int toplam = original.rows * original.cols;		
	
		for(int i=1; i<DEPTH; i++)
			histogram[i] = histogram[i-1] + histogram[i];

		for(int i=0; i<DEPTH; i++) {
			histogram[i] = (histogram[i] / toplam) * (DEPTH - 1);
			histogram[i] = (int) (histogram[i] + (0.5));
		}
	}
	
		//ilgili görseli ekranda göstermek için fonksiyon
		void display(Mat &image, string title) {
		namedWindow(title, WINDOW_AUTOSIZE);
		imshow(title, image);

	}
	
	//histogram grafiği çizmek için fonksiyon
	
	cv::Mat calculateHistogramImage(const Mat &img){
    // histogram hesapla
    int histSize = 256; 
    float range[] = {0, 255};
    const float* histRange = {range};
    Mat histogramImage;
    cv::calcHist(&img, 1, 0, Mat(), histogramImage, 1, &histSize, &histRange);
    
    // yeni görsel oluştur
    Mat drawHistogram(256, 256, CV_8UC1, Scalar(255));
    float max = 0;
    for(int i=0; i<histSize; i++){
        if( max < histogramImage.at<float>(i))
            max = histogramImage.at<float>(i);
    }
		//grafiğin pencere üst sınırına ulaşmaması içi ölçeklendirme işlemi
    float scale = (0.95*256)/max;
    for(int i=0; i<histSize; i++){
        int intensity = static_cast<int>(histogramImage.at<float>(i)*scale);
        cv::line(drawHistogram,Point(i,255),Point(i,255-intensity),Scalar(0));
    }
    return drawHistogram;

}	
	
	//constructor tanımlamaları	
public:
	~Image();
	Image(string);
	void histogramHesapla();
	friend void *histogramEsitle(Image *);

};


//histogram eşitleme fonksiyonu
void *histogramEsitle(Image *img) {
		
		
	for(int i=0; i< img->output.cols; i++)
		for(int j=0; j<img->output.cols; j++)
			img->output.at<uchar>(i, j) = img->histogram[img->original.at<uchar>(i, j)];
	return img;
}



//Görseli açmak ve gerekli işlemleri yapmak için tanımlanmış fonksiyon 
Image :: Image(string filename) {
	original = imread(filename, IMREAD_GRAYSCALE);
	
	if(!original.data) throw invalid_argument("Dosya okunamadı veya bulunamadı.");	

	//histogram dizisi 0 değerleriyle doldurulur
	initializeHist();

	//elde edilecek görüntü için mat tipinden değişken tanımlanır
	output = Mat(original.rows, original.cols, CV_8UC1, Scalar::all(0));

	// orijinal görüntü ve histogram değeri için ekranda gösterme fonksiyonları çağrılır.
	display(original, "İlk Hali");
	
	histGraph = calculateHistogramImage(original);
	display(histGraph, "İlk Histogram Hali");
}
//tahsis edilen alanı temizlemek için destructor çağrılır
Image :: ~Image() {
	waitKey(0); //ekrandaki görselleri bir tuşa basılana kadar gösterir
}



void Image :: histogramHesapla() {
	//histogram hesaplama fonksiyonu çağrılır	
	histHesapla();
	
	//histogram eşitleme hesaplaması fonksiyonu çağrılır
	histogramEsitleHesapla();
	
	cout << "\n\nGörsel Boyutu:\nYükseklik = " << original.rows << endl;
	cout << "Genişlik = " << original.cols << endl << endl;

	clock_t baslangicZamani = clock();
	histogramEsitle(this);
	
	//çalışma süresini gösteren kod
	cout << fixed << setprecision(11);
	cout << "\nÇalışma Süresi: " << (double( clock() - baslangicZamani ) / (double)CLOCKS_PER_SEC) / 1000 << " saniye." << endl;	


	//tüm işlemler sonucu oluşan görüntüler ekrana verilir.
	display(output, "Histogram Eşitlenmiş Görüntü");
	
	histGraphOutput = calculateHistogramImage(output);
	display(histGraphOutput, "Son Histogram Hali");
}



int main(int argc, char *argv[]) {


	
	if(argc != 2) {
		cout << "Eksik veya fazla argüman girildi." << endl;
	} else {
		try {
			Image image(argv[1]);
			//atoi -> string int dönüşümü
			image.histogramHesapla();
		} catch(invalid_argument e) {
			cout << "\n\nHatalı argüman : " << e.what() << endl << endl;
		}
	}
	return 0;
}

