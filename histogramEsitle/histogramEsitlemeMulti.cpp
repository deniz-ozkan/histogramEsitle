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
	void histogramEsitle(int);
	friend void *histogramEsitleThread(void *);
	
};


//multithread için parçalanmış görselin sınırlarını belirleyen sınıf
class GorselSinirlari {
private: 
	int basla;
	int bitir;
	Image *img;		

//constructor ve yaratılacak nesnein bu sınıfın elemanlarına erişmesi için friend constructor
public:
	GorselSinirlari(int Basla, int Bitir, Image *Img) {
		basla = Basla;
		bitir  = Bitir;
		img = Img;
	}
	friend void *histogramEsitleThread(void*); 	
};



//her bir bölünmüş parça için eşitleme yapan fonksiyon
void *histogramEsitleThread(void *sinirlar) {

	//bölünmüş parçalara erişmek için ilgili sınıftan nesne yaratılmıştır.
	GorselSinirlari *s = static_cast<GorselSinirlari *>(sinirlar);		
			
	//histogram eşitleme işlemini yapan döngüler
	for(int i=0; i<s->img->output.cols; i++)
		for(int j=s->basla; j<s->bitir; j++)
			s->img->output.at<uchar>(i, j) = s->img->histogram[s->img->original.at<uchar>(i, j)];
	return sinirlar;
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



void Image :: histogramEsitle(int threadSayisi) {
	//histogram hesaplama fonksiyonu çağrılır	
	histHesapla();
	
	//histogram eşitleme hesaplaması fonksiyonu çağrılır
	histogramEsitleHesapla();

	//thread sayısına göre resmin kaç parçaya bölüneceğini ve bölünen parçalara başlangıç ve bitiş değeri ataması yapan kod parçası
	if(original.cols < threadSayisi)
		threadSayisi = original.cols;
	
	pthread_t Thread[threadSayisi]; //elle girilecek thread sayısı bu diziye atanır 

	//adım sayısını hesaplama
	int adimSayisi = original.cols / threadSayisi;
	int basla, bitir;
	
	cout << "\n\nGörsel Boyutu:\nYükseklik = " << original.rows << endl;
	cout << "Genişlik = " << original.cols << endl << endl;

	clock_t baslangicZamani = clock();
	for(int i=0; i<threadSayisi; i++) {
		basla = (i * adimSayisi);
		
		if(i == threadSayisi) 
		bitir = original.cols;
		else bitir = basla + adimSayisi;

		cout << "Thread numarası: " << i << "\tBaslangıç Pikseli = " << basla << "\t Bitiş Pikseli = " << bitir << endl;

		// görselin bölündüğü parçalar için elle girilen thread sayısı kadar thread yaratılır.
		pthread_create(&Thread[i], NULL, histogramEsitleThread, (new GorselSinirlari(basla, bitir, this)));
	}

	//yaratılan threadler join ile bağlanır
	for(int i=0; i<threadSayisi; i++)
		pthread_join(Thread[i], NULL);

	//çalışma süresini gösteren kod
	cout << fixed << setprecision(11);
	cout << "\nÇalışma Süresi: " << (double( clock() - baslangicZamani ) / (double)CLOCKS_PER_SEC) / 1000 << " saniye." << endl;	


	//tüm işlemler sonucu oluşan görüntüler ekrana verilir.
	display(output, "Histogram Eşitlenmiş Görüntü");
	
	histGraphOutput = calculateHistogramImage(output);
	display(histGraphOutput, "Son Histogram Hali");
}



int main(int argc, char *argv[]) {


	
	if(argc != 3) {
		cout << "Eksik veya fazla argüman girildi." << endl;
	} else {
		try {
			//image sınıfına görsel argüman olarak verilir.
			Image image(argv[1]);
			//atoi -> string to int dönüşümü
			//thread sayısı fonksiyona geçilir.
			image.histogramEsitle(atoi(argv[2]));
		} catch(invalid_argument e) {
			cout << "\n\nHatalı argüman : " << e.what() << endl << endl;
		}
	}
	return 0;
}

