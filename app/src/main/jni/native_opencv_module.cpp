#include <jni.h>

#include <android/log.h>
#include <android/bitmap.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits>

#include "opencv2/imgproc.hpp"


using namespace cv;
using namespace std;

#define  LOG_TAG    "NativeCamera"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)



/* Global Variables */

cv::Mat lut;



void colorReduction(const cv::Mat& src, cv::Mat& dst, int div = 1) {

    if(lut.empty())
        lut.create(1,256,CV_8U);

    uchar* rdata = reinterpret_cast<uchar*>(lut.data);

    for (int i = 0; i<256; i++) {
        *rdata++ = i/div*div + div >> 1;
    }

    cv::LUT( src, lut, dst);
}





int64 t;
Mat srcBGR;


#ifdef __cplusplus
extern "C" {
#endif


JNIEXPORT void JNICALL
Java_ph_edu_dlsu_nativecamerapreview_CameraActivity_process(JNIEnv *env, jobject instance,
                                                            jobject pTarget, jbyteArray pSource,
                                                            jint mode) {
    AndroidBitmapInfo bitmapInfo;
    uint32_t* bitmapContent; // Links to Bitmap content

    if(AndroidBitmap_getInfo(env, pTarget, &bitmapInfo) < 0) abort();
    if(bitmapInfo.format != ANDROID_BITMAP_FORMAT_RGBA_8888) abort();
    if(AndroidBitmap_lockPixels(env, pTarget, (void**)&bitmapContent) < 0) abort();

    /// Access source array data... OK
    jbyte* source = (jbyte*)env->GetPrimitiveArrayCritical(pSource, 0);
    if (source == NULL) abort();

    /// cv::Mat for YUV420sp source and output BGRA
    Mat src(bitmapInfo.height + bitmapInfo.height/2, bitmapInfo.width, CV_8UC1, (unsigned char *)source);
    Mat mbgra(bitmapInfo.height, bitmapInfo.width, CV_8UC4, (unsigned char *)bitmapContent);


/***********************************************************************************************/
    /// Native Image Processing HERE...
    if(srcBGR.empty())
        srcBGR = Mat(bitmapInfo.height, bitmapInfo.width, CV_8UC3);


    cvtColor(src, srcBGR, CV_YUV420sp2RGB);  // 3-6 ms


    t = getTickCount();

    colorReduction(srcBGR, srcBGR, mode); // 4.19 ms

    LOGI("colorReduction took %0.2f ms.", 1000*((float)getTickCount() - t)/getTickFrequency());

    cvtColor(srcBGR, mbgra, CV_BGR2BGRA);

/************************************************************************************************/

    /// Release Java byte buffer and unlock backing bitmap
    env-> ReleasePrimitiveArrayCritical(pSource,source,0);
    if (AndroidBitmap_unlockPixels(env, pTarget) < 0) abort();
}


#ifdef __cplusplus
}
#endif