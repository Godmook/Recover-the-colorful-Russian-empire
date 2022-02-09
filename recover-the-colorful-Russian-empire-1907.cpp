#pragma warning(disable:4996)
#include <opencv2/opencv.hpp>
#include <Windows.h>
#include<stdio.h>
#include<string.h>
/*범위 초과하는지 확인하는 함수*/
int OOB(int e, int left, int right) {
    if (e < left || e >= right)return 1;
    else return 0;
}
/*차이값 계산하는 함수*/
float getDiff2(CvScalar a, CvScalar b)
{
    return (a.val[0] - b.val[0]) * (a.val[0] - b.val[0])
        + (a.val[1] - b.val[1]) * (a.val[1] - b.val[1])
        + (a.val[2] - b.val[2]) * (a.val[2] - b.val[2]);
}
/*이동하는 좌표값 저장하기 위한 구조체*/
typedef struct source {
    int bx;
    int by;
}sc;
/*절대값 찾는 함수*/
int abs(int a) {
    int c = a > 0 ? a : -a;
    return c;
}
/*diff값 찾는 함수*/
sc searchminLoss(int xleft, int xright, int interval, int ystart, int yend, int h, int w, IplImage* img, int flag, int xstart, int xend, int yleft, int yright) {
    sc c;
    float diffr = 0.0f;
    float mindiff = 1000000000;
    CvScalar f1;
    for (int u = xleft; u >= xright; u -= interval) {
        for (int v = yleft; v >= yright; v -= interval) {
            int count = 0;
            for (int y = ystart; y < yend; y += 3) {
                for (int x = xstart; x < xend; x += 3) {
                    int x2 = x + u;
                    int y2 = y + v;
                    if (x2<xstart || x2>xend - 1) continue;
                    if (y2<ystart || y2>yend - 1) continue;
                    if (flag == 1)f1 = cvGet2D(img, y - h, x);
                    else f1 = cvGet2D(img, y + h, x);
                    CvScalar f2 = cvGet2D(img, y2, x2);
                    diffr += getDiff2(f1, f2);
                    count++;
                }
            }
            diffr /= count;
            if (diffr < mindiff) {
                mindiff = diffr;
                c.bx = u;
                c.by = v;
            }
        }
    }
    return c;
}
/*이미지 합치는 함수*/
IplImage* combineImage(IplImage* img, sc blue, sc red, int h, int w) {
    IplImage* dst = cvCreateImage(cvSize(w, h), 8, 3);
    cvSet(dst, cvScalar(0, 0, 0));
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            double b = 0, g = 0, r = 0;
            int fby = y + blue.by, fbx = x + blue.bx, gy = y + h, gx = x, fry = y + red.by + 2 * h, frx = x + red.bx;
            b = (OOB(fby, 0, h) == 1 || OOB(fbx, 0, w) == 1) ? 0 : cvGet2D(img, fby, fbx).val[0];
            g = cvGet2D(img, gy, gx).val[1];
            r = (OOB(fry, 2 * h, 3 * h) == 1 || OOB(frx, 0, w) == 1) ? 0 : cvGet2D(img, fry, frx).val[2];
            cvSet2D(dst, y, x, cvScalar(b, g, r));
        }
    }
    return dst;
    cvReleaseImage(&dst);
}
/*간격 줄이면서 탐색하는 함수*/
sc lossInterval(int xleft, int yleft, int interval, int vary, int xstart, int xend, int ystart, int yend, int h, int w, IplImage* img, int flag) {
    sc result;
    while (interval > 0) {
        result = searchminLoss(xleft + vary, xleft - vary, interval, ystart, yend, h, w, img, flag, xstart, xend, yleft + vary, yleft - vary);
        vary /= 2;
        xleft = result.bx;
        yleft = result.by;
        if (abs(xleft) > 30 || abs(yleft) > 35) {
            if (vary > 5) {
                vary = 10;
                xleft = 0;
                yleft = 0;
                interval = 1;
                result = searchminLoss(xleft + vary, xleft - vary, interval, ystart, yend, h, w, img, flag, xstart, xend, yleft + vary, yleft - vary);
            }
        }
        interval /= 2;
    }
    return result;
}


int main() {
    printf("Test CV\n");
    printf("Input File Name:");
    char name[200];
    scanf("%s", name);
        IplImage* img = cvLoadImage(name);  //이미지 불러오기
        if (img == nullptr) {
            printf("File not Found!");
            exit(-1);
        }
        CvSize size = cvGetSize(img);   //이미지 사이즈 가지고 오기

        int w = size.width;
        int h = size.height / 3;    //새로 만들 이미지 높이
        int xt = w / 3; //탐색 범위
        int yt = h / 3; //탐색 범위
        if (h > 500) {  //큰 이미지는 원래 탐색범위의 절반을 탐색한다.
            xt /= 2;
            yt /= 2;
        }
        sc k[2];    //이동할 좌표 저장할 구조체 배열 생성
        /*파란색 이동하기 시작*/
        k[0] = lossInterval(0, 0, 16, 40, w / 2 - xt, w / 2 + xt, h / 2 - yt, h / 2 + yt, h, w, img, 0);
        /*빨간색 이동하기 시작*/
        k[1] = lossInterval(0, 0, 16, 40, w / 2 - xt, w / 2 + xt, 2 * h + h / 2 - yt, 2 * h + h / 2 + yt, h, w, img, 1);
        IplImage* combine = combineImage(img, k[0], k[1], h, w);//이미지 합치기
        cvShowImage("img", img);    //원본 이미지 출력
        cvShowImage("combine", combine);    //합친 이미지 출력
        cvWaitKey();
        cvReleaseImage(&img);
        cvReleaseImage(&combine);
    return 0;

}