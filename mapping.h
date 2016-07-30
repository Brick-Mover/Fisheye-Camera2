//
//  camera.h
//  Fisheye Camera
//
//  Created by SHAO Jiuru on 6/25/16.
//  Copyright © 2016 UCLA. All rights reserved.
//

#ifndef mercator_h
#define mercator_h

#include <assert.h>
#include <iostream>
#include <vector>
#include "matm.h"

using namespace std;

const float TOLERANCE = float(1.0e-04);

enum Walltype {xPos, xNeg, yPos, yNeg, zPos, zNeg};

struct Point
{
    Point(float x0, float y0, float z0) { x = x0; y = y0; z = z0; }
    Point& operator = (Point other);
    bool operator == (Point other);
    float x;
    float y;
    float z;
    void print() {
        cout << "p.x: " << x << endl;
        cout << "p.y: " << y << endl;
        cout << "p.z: " << z << endl;
        cout << endl;
    }
};

typedef Point Vec3;

struct Pixel
{
    Pixel(int r, int g, int b) {R = r; G = g; B = b;}
    Pixel() {}
    Pixel& operator = (Pixel other);
    unsigned char R;
    unsigned char G;
    unsigned char B;
    //int a;
    void print() const {
        cout << "p.R: " << int(R) << endl;
        cout << "p.G: " << int(G) << endl;
        cout << "p.B: " << int(B) << endl;
    }
};


const Pixel grey_pixel = Pixel(128, 128, 128);
const Point origin = Point(0.0, 0.0, 0.0);


struct Image
{
    Image(int x, int y, Walltype wt);
    
    Pixel getColor(float r, float c);
    int xDim;
    int yDim;
    Walltype t;
    vector<Pixel> pixels;
    void print();
};



struct Surrounding
{
    Surrounding(Image* pics[6]);
    Surrounding() {};
    Surrounding& operator = (Surrounding other);
    Image* xPos;
    Image* xNeg;
    Image* yPos;
    Image* yNeg;
    Image* zPos;
    Image* zNeg;
};



class Fisheye
{
public:
    Fisheye(float aperture,
            float xAngle, float yAngle, float zAngle, Point cameraPos, Surrounding s, int xDim, int yDim);
    vector<Pixel> getImage() const { return imagePlane; }
    void render();
    void renderPixel(int x, int y);
    void setColor(int x, int y, Pixel color);
    void initSurrounding(Image* s[6]);
    
private:
    float aperture;     // range of view
    Point cameraPos;
    Surrounding s;
    int xDim, yDim;     // size of image plane
    mat3 rotationZ;     // rotation around z axis
    mat3 rotationX;     // rotation around x axis
    mat3 rotationY;     // rotation around Y axis
    mat3 trans;         // multiplication of the three matrices above
    vector<Pixel> imagePlane; // the final image produced
    
};

#endif /* mercator_h */