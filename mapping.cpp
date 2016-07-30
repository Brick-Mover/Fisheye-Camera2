//
//  camera.cpp
//  Fisheye Camera
//
//  Created by SHAO Jiuru on 6/25/16.
//  Copyright © 2016 UCLA. All rights reserved.
//


#include "mapping.h"
#include "matm.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string.h>

#include <vector>
#include <math.h>
#include <cstdlib>


using namespace std;


void eat_comment(ifstream &f);
void load_ppm(Image* img, const string &name);
void saveImg(vector<Pixel> pic, int length, int height);
void initializeWalls(Image* pic[6]);


int main(int argc, const char * argv[])
{
    
    Image* xP = new Image(500, 500, xPos);
    Image* xN = new Image(500, 500, xNeg);
    Image* yP = new Image(500, 500, yPos);
    Image* yN = new Image(500, 500, yNeg);
    Image* zP = new Image(500, 500, zPos);
    Image* zN = new Image(500, 500, zNeg);
    Image* pics[6] = {xP, xN, yP, yN, zP, zN};
    initializeWalls(pics);
    Surrounding s = Surrounding(pics);
    
    
    Fisheye* f = new Fisheye(M_PI/4,0,0,M_PI_4,origin,s,600,600);
    f->render();
    vector<Pixel> result = f->getImage();
    saveImg(result, 600, 600);
}


Fisheye::Fisheye(float aperture,
                 float xAngle, float yAngle, float zAngle,
                 Point cPos, Surrounding s,
                 int xDim, int yDim): cameraPos(cPos)
{
    this->aperture = aperture;
    this->xDim = xDim;
    this->yDim = yDim;
    this->s = s;
    imagePlane.reserve(yDim*xDim);
    rotationX = mat3(vec3(1.0, 0.0, 0.0),
                     vec3(0, cosf(xAngle), -sinf(xAngle)),
                     vec3(0, sinf(xAngle),  cosf(xAngle))
                     );
    rotationY = mat3(vec3(cosf(yAngle), 0, sinf(yAngle)),
                     vec3(0.0, 1.0, 0.0),
                     vec3(-sinf(yAngle), 0, cosf(yAngle))
                     );
    rotationZ = mat3(vec3(cosf(zAngle), -sinf(zAngle), 0.0),
                     vec3(sinf(zAngle), cosf(zAngle), 0),
                     vec3(0.0, 0.0, 1.0)
                     );
    trans = rotationX * rotationY * rotationZ;
}


void Fisheye::render()
{
    // On view plane, render each pixel
    for (int r = 0; r < yDim; r++) {
        for (int c = 0; c < xDim; c++) {
            renderPixel(r, c);
        }
    }
//    renderPixel(135,415);
}


void Fisheye::renderPixel(int row, int col)
{
    // normalize (r,c) to coordinate [-1, 1]
    float xn = 2*float(col)/xDim - 1;
    float yn = 1 - 2*float(row)/yDim;
    float r = sqrtf(xn*xn + yn*yn);
    if (r > 1) {
        setColor(row, col, grey_pixel);
        return;
    }
    // get Cartesian coordinate on the sphere
    // theta: [0,2π] (xy-plane)
    float theta = atan2f(yn, xn);
    // phi: [0,π/2] (z direction)
    float phi = r * aperture / 2;
    
    float x = cosf(theta) * sinf(phi);
    float y = sinf(theta) * sinf(phi);
    float z = cosf(phi);
    
    float xsphere = z;
    float ysphere = -x;
    float zsphere = y;
    
//        cout << "xsphere: " << xsphere << endl;
//        cout << "ysphere: " << ysphere << endl;
//        cout << "zsphere: " << zsphere << endl;
    
    vec3 dir = trans * vec3(xsphere, ysphere, zsphere);
    
    float xyPlane = atan2f(dir.y, dir.x);
    if (xyPlane < 0)
        xyPlane += 2*M_PI;
    float yzPlane = atan2f(dir.z, dir.y);
    if (yzPlane < 0)
        yzPlane += 2*M_PI;
    float zxPlane = atan2f(dir.x, dir.z);
    if (zxPlane < 0)
        zxPlane += 2*M_PI;
    Image* w;
    float u, v;
    
//    cout << "theta: " << theta << endl;
//    cout << "phi: " << phi << endl;

    if (xyPlane > 2*M_PI)
        xyPlane = 2*M_PI;
    if (yzPlane > 2*M_PI)
        yzPlane = 2*M_PI;
    if (zxPlane > 2*M_PI)
        zxPlane = 2*M_PI;
    
    // determine which side the ray points to
    float mxy = xyPlane / M_PI_4;
    float myz = yzPlane / M_PI_4;
    float mzx = zxPlane / M_PI_4;
    if (myz > 1 && myz <= 3 && (mzx > 7 || mzx <= 1)) {
        w = s.zPos;
        u = (myz-1) / 2;
        if (mzx <= 1)
            mzx += 4;
        else mzx -= 4;
        v = (mzx-3) / 2;

//        cout << "testing " << endl;
    }  else if (mzx > 3 && mzx <= 5 && myz > 5 && myz <= 7) {
        w = s.zNeg;
        u = (myz - 5) / 2;
        v = (mzx - 3) / 2;
    }  else if (mxy <= 1 || mxy > 7) {
        w = s.xPos;
        if (mxy > 7)
            mxy -= 4;
        else mxy += 4;
        u = (mxy-3) / 2;
        v = 1-((mzx-1) / 2);
    }  else if  (1 < mxy && mxy <= 3) {
        w = s.yPos;
        if (myz > 7)
            myz -= 4;
        else myz += 4;
        u = (mxy-1) / 2;
        v = (myz-3) / 2;
    }  else if (3 < mxy && mxy <= 5) {
        w = s.xNeg;
        u = (mxy-3) / 2;
        v = (mzx-5) / 2;
    }  else {
        w = s.yNeg;
        u = 1-((mxy-5) / 2);
        v = 1-((myz-3) / 2);
    }
    
    if (u > 1)
        u = 1;
    else if (u < 0)
        u = 0;
    if (v > 1)
        v = 1;
    else if (v < 0)
        v = 0;
    
    //assert(u >= 0 && u <= 1 && v >= 0 && v <= 1);

    
    
    setColor(row, col, w->getColor(u, v));
    
//    w->print();
//    cout << "xsphere: " << xsphere << endl;
//    cout << "ysphere: " << ysphere << endl;
//    cout << "zsphere: " << zsphere << endl;
//    cout << "u: " << u << endl;
//    cout << "v: " << v << endl;
//    exit(1);
    
}






Pixel Image::getColor(float u, float v)
{
    int x = u*xDim;
    int y = v*yDim;
    int r = yDim - y;
    int c = xDim - x;
    assert(0 <= r*xDim+c <= xDim*yDim-1);
    return pixels[r*xDim+c];
}


Image::Image(int x, int y, Walltype wt)
{
    xDim = x;
    yDim = y;
    t = wt;
}


void Image::print()
{
    switch (t) {
        case xPos:
            cout << "xPos" << endl;
            break;
        case xNeg:
            cout << "xNeg" << endl;
            break;
        case yPos:
            cout << "yPos" << endl;
            break;
        case yNeg:
            cout << "yNeg" << endl;
            break;
        case zPos:
            cout << "zPos" << endl;
            break;
        case zNeg:
            cout << "zNeg" << endl;
            break;
    }
}


void Fisheye::setColor(int row, int col, Pixel color)
{
    assert(0 <= row*xDim+col <= xDim*yDim-1);
    imagePlane.push_back(color);
}


Surrounding::Surrounding(Image* pics[6])
{
    xPos = pics[0];
    xNeg = pics[1];
    yPos = pics[2];
    yNeg = pics[3];
    zPos = pics[4];
    zNeg = pics[5];
}


/* trivial member functions such as assign overloading and constructors */
Point& Point::operator = (Point other)
{
    x = other.x;
    y = other.y;
    z = other.z;
    return *this;
}


bool Point::operator == (Point other)
{
    return other.x == this->x &&
    other.y == this->y &&
    other.z == this->z;
}


Pixel& Pixel::operator = (Pixel other)
{
    R = other.R;
    G = other.G;
    B = other.B;
    return *this;
}


Surrounding& Surrounding::operator = (Surrounding other)
{
    xPos = other.xPos;
    xNeg = other.xNeg;
    yPos = other.yPos;
    yNeg = other.yNeg;
    zPos = other.zPos;
    zNeg = other.zNeg;
    return *this;
}


/* BELOW: load and save images */
void saveImg(vector<Pixel> pic, int length, int height)
{
    FILE *fp = fopen("result.ppm", "wb");
    (void) fprintf(fp, "P6\n%d %d\n255\n", length, height);
    int count = 0;
    for (int j = 0; j < height; ++j)
    {
        for (int i = 0; i < length; ++i)
        {
            static unsigned char color[3];
            color[0] = pic[count].R;  /* red */
            color[1] = pic[count].G;  /* green */
            color[2] = pic[count].B;  /* blue */
            (void) fwrite(color, 1, 3, fp);
            count++;
        }
    }
    (void) fclose(fp);
}


void initializeWalls(Image* test[6])
{
    load_ppm(test[0], "xPos.ppm");
    load_ppm(test[1], "xNeg.ppm");
    load_ppm(test[2], "yPos.ppm");
    load_ppm(test[3], "yNeg.ppm");
    load_ppm(test[4], "zPos.ppm");
    load_ppm(test[5], "zNeg.ppm");
}


void eat_comment(ifstream &f)
{
    char linebuf[1024];
    char ppp;
    while (ppp = f.peek(), ppp == '\n' || ppp == '\r')
        f.get();
    if (ppp == '#')
        f.getline(linebuf, 1023);
}


void load_ppm(Image* img, const string &name)
{
    ifstream f(name.c_str(), ios::binary);
    if (f.fail())
    {
        cout << "Could not open file: " << name << endl;
        return;
    }
    
    // get type of file
    eat_comment(f);
    int mode = 0;
    string s;
    f >> s;
    if (s == "P3")
        mode = 3;
    else if (s == "P6")
        mode = 6;
    
    // get w
    eat_comment(f);
    f >> img->xDim;
    
    // get h
    eat_comment(f);
    f >> img->yDim;
    
    // get bits
    eat_comment(f);
    int bits = 0;
    f >> bits;
    
    // error checking
    if (mode != 3 && mode != 6)
    {
        cout << "Unsupported magic number" << endl;
        f.close();
        return;
    }
    if (img->xDim < 1)
    {
        cout << "Unsupported width: " << img->xDim << endl;
        f.close();
        return;
    }
    if (img->yDim < 1)
    {
        cout << "Unsupported height: " << img->yDim << endl;
        f.close();
        return;
    }
    if (bits < 1 || bits > 255)
    {
        cout << "Unsupported number of bits: " << bits << endl;
        f.close();
        return;
    }
    
    // load image data
    img->pixels.resize(img->xDim * img->yDim);
    
    if (mode == 6)
    {
        f.get();
        f.read((char*)&img->pixels[0], img->pixels.size() * 3);
    }
    else if (mode == 3)
    {
        for (int i = 0; i < img->pixels.size(); i++)
        {
            int v;
            f >> v;
            img->pixels[i].R = v;
            f >> v;
            img->pixels[i].G = v;
            f >> v;
            img->pixels[i].B = v;
        }
    }
    
    // close file
    f.close();
}
