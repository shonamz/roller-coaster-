/*
 CSCI 420 Computer Graphics, USC
 Assignment 1: Height Fields
 C++ starter code
 
 Student username: <namazian>
 */

#include <iostream>
#include <cstring>
#include "openGLHeader.h"
#include "glutHeader.h"
#include "imageIO.h"
#include "openGLMatrix.h"
#include "basicPipelineProgram.h"
#include "chrono"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "openGLHeader.h"
#include "imageIO.h"
#include <vector>
#include "splinePipelineProgram.h"


#ifdef WIN32
#ifdef _DEBUG
#pragma comment(lib, "glew32d.lib")
#else
#pragma comment(lib, "glew32.lib")
#endif
#endif

#ifdef WIN32
char shaderBasePath[1024] = SHADER_BASE_PATH;
#else
char shaderBasePath[1024] = "../openGLHelper-starterCode";
#endif

using namespace std;

int mousePos[2]; // x,y coordinate of the mouse position

int leftMouseButton = 0; // 1 if pressed, 0 if not
int middleMouseButton = 0; // 1 if pressed, 0 if not
int rightMouseButton = 0; // 1 if pressed, 0 if not

typedef enum { ROTATE, TRANSLATE, SCALE } CONTROL_STATE;
CONTROL_STATE controlState = ROTATE;

// state of the world
float landRotate[3] = { 0.0f, 0.0f, 0.0f };
float landTranslate[3] = { 0.0f, 0.0f, 0.0f };
float landScale[3] = { 1.0f, 1.0f, 1.0f };

int windowWidth = 1280;
int windowHeight = 720;
char windowTitle[512] = "CSCI 420 homework II";

//ImageIO * heightmapImage;

//set up modelView and projection matrices
OpenGLMatrix * matrix = new OpenGLMatrix();
float m[16];
float p[16];
const float fov=45;
///pipeline

BasicPipelineProgram  * g_pipeline = new BasicPipelineProgram();
GLuint program = 0;

//SplinePipelineProgram  * g_splinePipeline= new SplinePipelineProgram();
//GLint splineProgram =0;


//translation varibale
const float TRANSLATION_MODIFIER = 40;

//animation variables

int MAXIMUM_SCREENSHOTS = 1001;
int frameNum;
int g_screenshotCounter = 0;
float tacc=0;
int cameraSteps =0;

//screen shot

bool takeScreenshots = false;
const float sValue=0.5;

// represents one control point along the spline
struct Point
{
    double x;
    double y;
    double z;
};
// spline struct
// contains how many control points the spline has, and an array of control points
struct Spline
{
    int numControlPoints;
    Point * points;
};

// the spline array
Spline * splines;
// total number of splines
int numSplines;

// data for spline postions and colors

std::vector<GLfloat> splinesColor;// color
std::vector<float> splineVertices;//position


// VBO and VAO
GLuint splineVBO;
GLuint splineVAO;
//data for ground
std::vector<float> terrainVertices;
std::vector<float> terrainUVs;
// VBO and VAO
GLuint terrianVBO;
GLuint terrianVAO;
GLuint skyVBO;
GLuint skyVAO;
std::vector<float> skyVertices;
std::vector<float> skyUVs;
///
GLuint trackVBO;
GLuint trackVAO;
std::vector<float> trackVertices;
std::vector<float> trackUVs;

GLuint texHandle;
GLuint texHandle1;
GLuint texHandle2;

Point normalCompute (const Point& unitTangent) {
    
    // Compute the normal between two vectors -- y axis to get one axis
     Point normal;
    normal.x = (unitTangent.y * 0 - unitTangent.z * 1);
    normal.y = (unitTangent.x * 0 - unitTangent.z * 0);
    normal.z = (unitTangent.x * 1 - unitTangent.y * 0);
    
    // Compute the binormal by crossing the normal with the tangent
    Point binormal;
    binormal.x = (normal.y * unitTangent.z - normal.z * unitTangent.y);
    binormal.y = -(normal.x * unitTangent.z - normal.z * unitTangent.x);
    binormal.z = (normal.x * unitTangent.y - normal.y * unitTangent.z);
    
    return binormal;
}

void CameraPosition (const Point& point0, const Point& point1, const Point& point2, const Point& point3) {
    
    // Compute the tangent
   
    const float u = 0.001f;
    Point tangent;
    
    tangent.x = sValue * (
                           (-point0.x + point2.x) +
                           (2 * point0.x - 5 * point1.x + 4 * point2.x - point3.x) * (2 * u) +
                           (-point0.x + 3 * point1.x - 3 * point2.x + point3.x) * (3 * u * u)
                           );
    
    tangent.y = sValue * (
                           (-point0.y + point2.y) +
                           (2 * point0.y - 5 * point1.y + 4 * point2.y - point3.y) * (2 * u) +
                           (-point0.y + 3 * point1.y - 3 * point2.y + point3.y) * (3 * u * u)
                           );
    
    tangent.z = sValue * (
                           (-point0.z + point2.z) +
                           (2 * point0.z - 5 * point1.z + 4 * point2.z - point3.z) * (2 * u) +
                           (-point0.z + 3 * point1.z - 3 * point2.z + point3.z) * (3 * u * u)
                           );
    
    // Compute the unit tangent
    float magnitude = std::sqrt (std::pow (tangent.x, 2) + std::pow (tangent.y, 2) + std::pow (tangent.z, 2));
    Point unitTangent;
    unitTangent.x = tangent.x / magnitude;
    unitTangent.y = tangent.y / magnitude;
    unitTangent.z = tangent.z / magnitude;
     
    // Compute the normal
    Point normal = normalCompute (unitTangent);
    matrix->LookAt(
                      point0.x, point0.y + 0.5f, point0.z,
                      point0.x + unitTangent.x, point0.y + unitTangent.y, point0.z + unitTangent.z,
                      normal.x, normal.y, normal.z
                      );
    
}

// Update the camera position based on the provided index

void cameraForRide (int index) {
    
    Point point1;
    point1.x =splineVertices[index];
    point1.y= splineVertices[index+1];
    point1.z= splineVertices[index+2];
    
    Point point2;
    point2.x =splineVertices[index+3];
    point2.y= splineVertices[index+4];
    point2.z= splineVertices[index+5];

    Point point3;
    point3.x =splineVertices[index+6];
    point3.y= splineVertices[index+7];
    point3.z= splineVertices[index+8];

    Point point4;
    point4.x =splineVertices[index+9];
    point4.y= splineVertices[index+10];
    point4.z= splineVertices[index+11];
 
    CameraPosition (point1, point2, point3, point4);
}


int loadSplines(char * argv)
{
    char * cName = (char *) malloc(128 * sizeof(char));
    FILE * fileList;
    FILE * fileSpline;
    int iType, i = 0, j, iLength;
    
    // load the track file
    fileList = fopen(argv, "r");
    if (fileList == NULL)
    {
        printf ("can't open file\n");
        exit(1);
    }
    // stores the number of splines in a global variable
    fscanf(fileList, "%d", &numSplines);
    
    splines = (Spline*) malloc(numSplines * sizeof(Spline));
    
    // reads through the spline files
    for (j = 0; j < numSplines; j++)
    {
        i = 0;
        fscanf(fileList, "%s", cName);
        fileSpline = fopen(cName, "r");
        
        if (fileSpline == NULL)
        {
            printf ("can't open file\n");
            exit(1);
        }
        // gets length for spline file
        fscanf(fileSpline, "%d %d", &iLength, &iType);
        
        // allocate memory for all the points
        splines[j].points = (Point *)malloc(iLength * sizeof(Point));
        splines[j].numControlPoints = iLength;
        
        // saves the data to the struct
        while (fscanf(fileSpline, "%lf %lf %lf",
                      &splines[j].points[i].x,
                      &splines[j].points[i].y,
                      &splines[j].points[i].z) != EOF)
        {
            i++;
        }
    }
    
    free(cName);
    return 0;
}

int initTexture(const char * imageFilename, GLuint textureHandle)
{
    // read the texture image
    ImageIO img;
    ImageIO::fileFormatType imgFormat;
    ImageIO::errorType err = img.load(imageFilename, &imgFormat);
    
    if (err != ImageIO::OK)
    {
        printf("Loading texture from %s failed.\n", imageFilename);
        return -1;
    }
    
    // check that the number of bytes is a multiple of 4
    if (img.getWidth() * img.getBytesPerPixel() % 4)
    {
        printf("Error (%s): The width*numChannels in the loaded image must be a multiple of 4.\n", imageFilename);
        return -1;
    }
    
    // allocate space for an array of pixels
    int width = img.getWidth();
    int height = img.getHeight();
    unsigned char * pixelsRGBA = new unsigned char[4 * width * height]; // we will use 4 bytes per pixel, i.e., RGBA
    
    // fill the pixelsRGBA array with the image pixels
    memset(pixelsRGBA, 0, 4 * width * height); // set all bytes to 0
    for (int h = 0; h < height; h++)
        for (int w = 0; w < width; w++)
        {
            // assign some default byte values (for the case where img.getBytesPerPixel() < 4)
            pixelsRGBA[4 * (h * width + w) + 0] = 0; // red
            pixelsRGBA[4 * (h * width + w) + 1] = 0; // green
            pixelsRGBA[4 * (h * width + w) + 2] = 0; // blue
            pixelsRGBA[4 * (h * width + w) + 3] = 255; // alpha channel; fully opaque
            
            // set the RGBA channels, based on the loaded image
            int numChannels = img.getBytesPerPixel();
            for (int c = 0; c < numChannels; c++) // only set as many channels as are available in the loaded image; the rest get the default value
                pixelsRGBA[4 * (h * width + w) + c] = img.getPixel(w, h, c);
        }
    
    // bind the texture
    glBindTexture(GL_TEXTURE_2D, textureHandle);
    
    // initialize the texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixelsRGBA);
    
    
    // generate the mipmaps for this texture
    glGenerateMipmap(GL_TEXTURE_2D);
    
    // set the texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // query support for anisotropic texture filtering
    GLfloat fLargest;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
    printf("Max available anisotropic samples: %f\n", fLargest);
    // set anisotropic texture filtering
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 0.5f * fLargest);
    
    // query for any errors
    GLenum errCode = glGetError();
    if (errCode != 0)
    {
        printf("Texture initialization error. Error code: %d.\n", errCode);
        return -1;
    }
    
    // de-allocate the pixel array -- it is no longer needed
    delete [] pixelsRGBA;
    
    return 0;
}
// initlize pipeline

void initPipeLine(){
    g_pipeline->Init("../openGLHelper-starterCode");
    g_pipeline->Bind();
    program = g_pipeline->GetProgramHandle();
}

//initlize buffers
void initBuffer(){
    // spline VA and VB
    glGenVertexArrays(1, &splineVAO);
    glBindVertexArray(splineVAO);
    
    glGenBuffers(1,&splineVBO);
    glBindBuffer(GL_ARRAY_BUFFER,splineVBO);
    glBufferData(GL_ARRAY_BUFFER, splineVertices.size() * sizeof(float) + splinesColor.size() * sizeof(float),NULL,GL_STATIC_DRAW);
    
    //UPLOAD POSITION DATA
    
    glBufferSubData(GL_ARRAY_BUFFER,0,splineVertices.size()*sizeof(float),splineVertices.data());
    
    //UPLOAD COLOR DATA
    
    glBufferSubData(GL_ARRAY_BUFFER ,splineVertices.size()*sizeof(float), splinesColor.size() *sizeof(float),splinesColor.data());
    // get location index of the “position” shader variable
    GLuint loc = glGetAttribLocation(program, "position");
    glEnableVertexAttribArray(loc);
    // enable the “position” attribute
    const void *offset = (const void*) 0;
    GLsizei stride = 0;
    GLboolean normalized = GL_FALSE;
    // set the layout of the “position” attribute data
    glVertexAttribPointer(loc, 3, GL_FLOAT, normalized, stride, offset);
    
    // get the location index of the “color” shader variable
    loc = glGetAttribLocation(program, "color");
    
    glEnableVertexAttribArray(loc);
    // enable the “color” attribute
    offset = (const void*) (splineVertices.size()*sizeof(float));
    
    // set the layout of the “color” attribute data f
    glVertexAttribPointer(loc, 4, GL_FLOAT, normalized, stride, offset);
    
    glBindVertexArray(0); // unbind the VAO
    // terrain VA and VB
    
    glGenVertexArrays(1, &terrianVAO);
    glBindVertexArray(terrianVAO);
    
    glGenBuffers(1,&terrianVBO);
    glBindBuffer(GL_ARRAY_BUFFER,terrianVBO);
    glBufferData(GL_ARRAY_BUFFER, terrainVertices.size() * sizeof(float) + terrainUVs.size() * sizeof(float),NULL,GL_STATIC_DRAW);
    
    //UPLOAD POSITION DATA
    
    glBufferSubData(GL_ARRAY_BUFFER,0,terrainVertices.size()*sizeof(float),terrainVertices.data());
    
    //UPLOAD UVS DATA
    
    glBufferSubData(GL_ARRAY_BUFFER ,terrainVertices.size()*sizeof(float), terrainUVs.size() *sizeof(float),terrainUVs.data());
    
    // get location index of the “position” shader variable
    GLuint loc1 = glGetAttribLocation(program, "position");
    glEnableVertexAttribArray(loc1);
    // enable the “position” attribute
    const void *offset1 = (const void*) 0;
    stride = 0;
    normalized = GL_FALSE;
    // set the layout of the “position” attribute data
    glVertexAttribPointer(loc1, 3, GL_FLOAT, normalized, stride, offset1);
    
    // get location index of the “texCoord” shader variable
    loc1 = glGetAttribLocation(program, "texCoord");
    glEnableVertexAttribArray(loc1); // enable “texCoord” attribute
    // set the layout of the “texCoord” attribute data
    offset1 = (const void*) (terrainVertices.size()*sizeof(float));
    stride = 0;
    
    glVertexAttribPointer(loc1, 2, GL_FLOAT, GL_FALSE, stride, offset1);
    
    glBindVertexArray(0); // unbind the VAO
// sky VA and VB
    
    glGenVertexArrays(1, &skyVAO);
    glBindVertexArray(skyVAO);
    
    glGenBuffers(1,&skyVBO);
    glBindBuffer(GL_ARRAY_BUFFER,skyVBO);
    glBufferData(GL_ARRAY_BUFFER, skyVertices.size() * sizeof(float) + skyUVs.size() * sizeof(float),NULL,GL_STATIC_DRAW);
    
    //UPLOAD POSITION DATA
    
    glBufferSubData(GL_ARRAY_BUFFER,0,skyVertices.size()*sizeof(float),skyVertices.data());
    
    //UPLOAD UVS DATA
    
    glBufferSubData(GL_ARRAY_BUFFER ,skyVertices.size()*sizeof(float), skyUVs.size() *sizeof(float),skyUVs.data());
    
    // get location index of the “position” shader variable
    GLuint loc2 = glGetAttribLocation(program, "position");
    glEnableVertexAttribArray(loc2);
    // enable the “position” attribute
    const void *offset2 = (const void*) 0;
    stride = 0;
    normalized = GL_FALSE;
    // set the layout of the “position” attribute data
    glVertexAttribPointer(loc2, 3, GL_FLOAT, normalized, stride, offset2);
    
    // get location index of the “texCoord” shader variable
    loc2 = glGetAttribLocation(program, "texCoord");
    glEnableVertexAttribArray(loc1); // enable “texCoord” attribute
    // set the layout of the “texCoord” attribute data
    offset1 = (const void*) (skyVertices.size()*sizeof(float));
    stride = 0;
    
    glVertexAttribPointer(loc2, 2, GL_FLOAT, GL_FALSE, stride, offset1);
    
    glBindVertexArray(0); // unbind the VAO
    
    // track VA and VB
    
    glGenVertexArrays(1, &trackVAO);
    glBindVertexArray(trackVAO);
    
    glGenBuffers(1,&trackVBO);
    glBindBuffer(GL_ARRAY_BUFFER,trackVBO);
    glBufferData(GL_ARRAY_BUFFER, trackVertices.size() * sizeof(float) + trackUVs.size() * sizeof(float),NULL,GL_STATIC_DRAW);
    
    //UPLOAD POSITION DATA
    
    glBufferSubData(GL_ARRAY_BUFFER,0,trackVertices.size()*sizeof(float),trackVertices.data());
    
    //UPLOAD UVS DATA
    
    glBufferSubData(GL_ARRAY_BUFFER ,trackVertices.size()*sizeof(float), trackUVs.size() *sizeof(float),trackUVs.data());
    
    // get location index of the “position” shader variable
    GLuint loc3 = glGetAttribLocation(program, "position");
    glEnableVertexAttribArray(loc3);
    // enable the “position” attribute
    const void *offset3 = (const void*) 0;
    stride = 0;
    normalized = GL_FALSE;
    // set the layout of the “position” attribute data
    glVertexAttribPointer(loc3, 3, GL_FLOAT, normalized, stride, offset3);
    
    // get location index of the “texCoord” shader variable
    loc1 = glGetAttribLocation(program, "texCoord");
    glEnableVertexAttribArray(loc1); // enable “texCoord” attribute
    // set the layout of the “texCoord” attribute data
    offset1 = (const void*) (trackVertices.size()*sizeof(float));
    stride = 0;
    glVertexAttribPointer(loc1, 2, GL_FLOAT, GL_FALSE, stride, offset1);
    glBindVertexArray(0); // unbind the VAO
   
}

//multitexturing

void setTextureUnit(GLint unit){
    
    glActiveTexture(unit); // select the active texture unit
    // get a handle to the “textureImage” shader variable
    GLint h_textureImage = glGetUniformLocation(program, "textureImage");
     // deem the shader variable “textureImage” to read from texture unit “unit”
    glUniform1i(h_textureImage, unit - GL_TEXTURE0);
    
}

// write a screenshot to the specified filename
void saveScreenshot(const char * filename)
{
    unsigned char * screenshotData = new unsigned char[windowWidth * windowHeight * 3];
    glReadPixels(0, 0, windowWidth, windowHeight, GL_RGB, GL_UNSIGNED_BYTE, screenshotData);
    
    ImageIO screenshotImg(windowWidth, windowHeight, 3, screenshotData);
    
    if (screenshotImg.save(filename, ImageIO::FORMAT_JPEG) == ImageIO::OK)
        cout << "File " << filename << " saved successfully." << endl;
    else cout << "Failed to save file " << filename << '.' << endl;
    
    delete [] screenshotData;
}

void displayFunc()
{
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT| GL_DEPTH_BUFFER_BIT);
    GLint h_modelViewMatrix =glGetUniformLocation(program, "modelViewMatrix");
    // compute modelview matrix
    matrix->SetMatrixMode(OpenGLMatrix::ModelView);
    matrix->LoadIdentity();
    //set the camera
     //matrix->LookAt(0,5,40,
             //      0,0,0,
                 //  0,1,0);
    // Move the camera along the spline track
    
    if (cameraSteps <splineVertices.size () - 3) {
        cameraForRide (cameraSteps);
        cameraSteps++;
       // cout << " spline verteces ......" <<cameraSteps << endl;
        
    }
    // start translation
    matrix->Translate(landTranslate[0]*TRANSLATION_MODIFIER,landTranslate[1]*TRANSLATION_MODIFIER,landTranslate[2]*TRANSLATION_MODIFIER);
    //strat rotation
    matrix->Rotate(landRotate[0], 1,0,0);
    matrix->Rotate(landRotate[1], 0,1,0);
    matrix->Rotate(landRotate[2], 0,0,1);
    //start scaling
    matrix->Scale(landScale[0],landScale[1],landScale[2]);

    matrix->GetMatrix(m);// fill "m" with the matrix entries
    // upload m to the GPU
    g_pipeline->Bind(); // must do (once) before glUniformMatrix4fv
    GLboolean isRowMajor=GL_FALSE;
    glUniformMatrix4fv(h_modelViewMatrix, 1, isRowMajor, m);
    matrix->SetMatrixMode(OpenGLMatrix::Projection);
    // get a handle to the projectionMatrix shader variable
    GLint h_projectionMatrix = glGetUniformLocation(program, "projectionMatrix");
    matrix->GetMatrix(p);
    // upload p to the GPU
    glUniformMatrix4fv(h_projectionMatrix, 1, isRowMajor, p);
    
    // Use the VAO
    g_pipeline->Bind(); // bind the pipeline program
    GLint first = 0;
    GLsizei count;
    // select the active texture unit
    setTextureUnit(GL_TEXTURE0); // it is safe to always use GL_TEXTURE0
    // select the texture to use (“texHandle”was generated by glGenTextures)
    glBindTexture(GL_TEXTURE_2D, texHandle);
    
    glBindVertexArray(splineVAO); // bind the VAO
    count = splineVertices.size() / 3;
    glDrawArrays(GL_LINE_STRIP, first, count);
    glBindVertexArray(0); // unbind the VAO
    
    setTextureUnit(GL_TEXTURE0); // it is safe to always use GL_TEXTURE0
    // select the texture to use (“texHandle”was generated by glGenTextures)
    glBindTexture(GL_TEXTURE_2D, texHandle1);
    
    glBindVertexArray(terrianVAO); // bind the VAO
    count = terrainVertices.size() / 3;
    glDrawArrays(GL_TRIANGLES, first, count);
    glBindVertexArray(0); // unbind the VAO
    
    setTextureUnit(GL_TEXTURE0); // it is safe to always use GL_TEXTURE0
    // select the texture to use (“texHandle”was generated by glGenTextures)
    glBindTexture(GL_TEXTURE_2D, texHandle);

    glBindVertexArray(skyVAO); // bind the VAO
    count = skyVertices.size() / 3;
    glDrawArrays(GL_TRIANGLES, first, count);
    glBindVertexArray(0); // unbind the VAO
    
    setTextureUnit(GL_TEXTURE0); // it is safe to always use GL_TEXTURE0
    // select the texture to use (“texHandle”was generated by glGenTextures)
    glBindTexture(GL_TEXTURE_2D, texHandle2);
    
    ///track
    
    glBindVertexArray(trackVAO); // bind the VAO
    count = trackVertices.size() / 3;
    glDrawArrays(GL_TRIANGLES, first, count);
    glBindVertexArray(0); // unbind the VAO
    
    setTextureUnit(GL_TEXTURE0); // it is safe to always use GL_TEXTURE0
    // select the texture to use (“texHandle”was generated by glGenTextures)
    glBindTexture(GL_TEXTURE_2D, texHandle2);
    
    //switch buffer
    glutSwapBuffers();
}

void idleFunc()
{
    // animation. save files
    if (g_screenshotCounter <  MAXIMUM_SCREENSHOTS && takeScreenshots) {
        auto start =chrono::steady_clock::now();
        std::string filename = std::to_string(frameNum) + ".jpg";
        if (frameNum < 100) {
            filename = "0" + filename;
        }
        if (frameNum < 10) {
            filename = "0" + filename;
        }
        filename = "Recording/" + filename;
        saveScreenshot(filename.c_str());
        ++frameNum;
        g_screenshotCounter++;
        
        auto end =chrono::steady_clock::now();
        // find elapse time
        float timeElaspe = chrono::duration_cast<chrono::microseconds>(end -start).count();
        tacc=  timeElaspe /1000000 + tacc;
        cout << "tacc........" << tacc << endl;
        // check the frame rate dump out if over
        if (tacc>0.0666){
            //dump out an image
            MAXIMUM_SCREENSHOTS =MAXIMUM_SCREENSHOTS+1;
            frameNum--;
            tacc=0;
        }
        
       /* // Move the camera along the spline track
        if (cameraSteps <splineVertices.size () - 3) {
            cameraForRide (cameraSteps);
            cameraSteps++;
            cout << " spline verteces ......" <<cameraSteps << endl;

        }*/

    }
    // make the screen update
    glutPostRedisplay();
}
//reshape function to set up perspective function
void reshapeFunc(int w, int h)
{   glViewport(0, 0, w, h);
    //set up  projection matrix
    GLfloat aspect =(GLfloat)1280/(GLfloat)720;
    matrix->SetMatrixMode(OpenGLMatrix::Projection);
    matrix->LoadIdentity();
    //set camera to perspective
    matrix->Perspective(fov, aspect, 0.01, 1000.0);
    matrix->GetMatrix (p);
    // Send matrix to the pipeline
    g_pipeline->SetProjectionMatrix (p);
    //set back to model view
    matrix->SetMatrixMode(OpenGLMatrix::ModelView);
}

void mouseMotionDragFunc(int x, int y)
{
    // mouse has moved and one of the mouse buttons is pressed (dragging)
    // the change in mouse position since the last invocation of this function
    int mousePosDelta[2] = { x - mousePos[0], y - mousePos[1] };
    
    switch (controlState){
            // translate the landscape
        case TRANSLATE:
            if (leftMouseButton)
            {
                // control x,y translation via the left mouse button
                landTranslate[0] += mousePosDelta[0] * 0.01f;
                landTranslate[1] -= mousePosDelta[1] * 0.01f;
            }
            if (middleMouseButton)
            {
                // control z translation via the middle mouse button
                landTranslate[2] += mousePosDelta[1] * 0.01f;
            }
            
            break;
            // rotate the landscape
        case ROTATE:
            if (leftMouseButton)
            {
                // control x,y rotation via the left mouse button
                landRotate[0] += mousePosDelta[1];
                landRotate[1] += mousePosDelta[0];
            }
            if (middleMouseButton)
            {
                // control z rotation via the middle mouse button
                landRotate[2] += mousePosDelta[1];
            }
            
            break;
            // scale the landscape
        case SCALE:
            if (leftMouseButton) {
                // control x,y scaling via the left mouse button
                landScale[0] *= 1.0f + mousePosDelta[0] * 0.01f;
                landScale[1] *= 1.0f - mousePosDelta[1] * 0.01f;
            }
            if (middleMouseButton)
            {
                // control z scaling via the middle mouse button
                landScale[2] *= 1.0f - mousePosDelta[1] * 0.01f;
            }
            break;
    }
    // store the new mouse position
    mousePos[0] = x;
    mousePos[1] = y;
}

void mouseMotionFunc(int x, int y)
{
    // mouse has moved
    // store the new mouse position
    mousePos[0] = x;
    mousePos[1] = y;
}

void mouseButtonFunc(int button, int state, int x, int y)
{
    // a mouse button has has been pressed or depressed
    
    // keep track of the mouse button state, in leftMouseButton, middleMouseButton, rightMouseButton variables
    switch (button)
    {
        case GLUT_LEFT_BUTTON:
            leftMouseButton = (state == GLUT_DOWN);
            break;
            
        case GLUT_MIDDLE_BUTTON:
            middleMouseButton = (state == GLUT_DOWN);
            break;
            
        case GLUT_RIGHT_BUTTON:
            rightMouseButton = (state == GLUT_DOWN);
            break;
    }
    
    // keep track of whether CTRL and SHIFT keys are pressed
    switch (glutGetModifiers())
    {
        case GLUT_ACTIVE_CTRL:
            controlState = TRANSLATE;
            break;
            
        case GLUT_ACTIVE_SHIFT:
            controlState = SCALE;
            break;
            // if CTRL and SHIFT are not pressed, we are in rotate mode
        default:
            controlState = ROTATE;
            break;
    }
    
    // store the new mouse position
    mousePos[0] = x;
    mousePos[1] = y;
}
void keyboardFunc(unsigned char key, int x, int y)
{
    switch (key)
    {
        case 27: // ESC key
            exit(0); // exit the program
            break;
        case ' ':
            cout << "You pressed the spacebar." << endl;
            break;
        case 'x':
            // take a screenshot
            saveScreenshot("screenshot.jpg");
            break;
        case 'q':// start animation
            takeScreenshots = true;
            std::cout << "animation!." << std::endl;
            break;
            //Transformation
        case 't':
            controlState = TRANSLATE;
            break;
    }
}

void generateTerrain(){
    
    ImageIO * imageIO= new ImageIO();
    if (imageIO->loadJPEG("textures/grnd.jpg") != ImageIO::OK)
    {
        cout << "Error" << endl;
        exit(EXIT_FAILURE); }
    
     // Generate corners -- Bottom left
    GLfloat bl[3] = { -128, -1, -128 };
    
    // Generate corners -- Top Left
    GLfloat tl[3] = { -128, -1, 128 };
    
    // Generate corners -- Top Right
    GLfloat tr[3] = { 128, -1, 128 };
    
    // Generate corners -- Bottom Right
    GLfloat br[3] = { 128, -1, -128 };

    // Push our coordinates into the vertex buffer as two triangles (clockwise)
    terrainVertices.insert(terrainVertices.end(), tl, tl + 3);
    terrainVertices.insert(terrainVertices.end(), tr, tr + 3);
    terrainVertices.insert(terrainVertices.end(), bl, bl + 3);
    terrainVertices.insert(terrainVertices.end(), bl, bl + 3);
    terrainVertices.insert(terrainVertices.end(), tr, tr + 3);
    terrainVertices.insert(terrainVertices.end(), br, br + 3);
    
    // Generate corners -- Bottom left
    GLfloat bl_uv[3] = { 0, 0 };
    
    // Generate corners -- Top Left
    GLfloat tl_uv[3] = { 0, 1 };
    
    // Generate corners -- Top Right
    GLfloat tr_uv[3] = { 1, 1 };
    
    // Generate corners -- Bottom Right
    GLfloat br_uv[3] = { 1, 0 };
    
    // Push our uv data to the buffer
    terrainUVs.insert(terrainUVs.end(), tl_uv, tl_uv + 2);
    terrainUVs.insert(terrainUVs.end(), tr_uv, tr_uv + 2);
    terrainUVs.insert(terrainUVs.end(), bl_uv, bl_uv + 2);
    terrainUVs.insert(terrainUVs.end(), bl_uv, bl_uv + 2);
    terrainUVs.insert(terrainUVs.end(), tr_uv, tr_uv + 2);
    terrainUVs.insert(terrainUVs.end(), br_uv, br_uv + 2);
}

void generateSky(){
    {
        ImageIO * imageIO= new ImageIO();
        if (imageIO->loadJPEG("textures/sky.jpg") != ImageIO::OK)
        {
            cout << "Error" << endl;
            exit(EXIT_FAILURE); }
        
        // Generate corners -- Bottom left
        GLfloat bl[3] = { -128, -128, -128 };
        // Generate corners -- Top Left
        GLfloat tl[3] = { -128, 128, -128 };

        // Generate corners -- Top Right
        GLfloat tr[3] = { -128, 128, 128 };

        // Generate corners -- Bottom Right
        GLfloat br[3] = { -128, -128, 128 };

        // Push our coordinates into the vertex buffer as two triangles (clockwise)
        skyVertices.insert(skyVertices.end(), tl, tl + 3);
        skyVertices.insert(skyVertices.end(), tr, tr + 3);
        skyVertices.insert(skyVertices.end(), bl, bl + 3);
        skyVertices.insert(skyVertices.end(), bl, bl + 3);
        skyVertices.insert(skyVertices.end(), tr, tr + 3);
        skyVertices.insert(skyVertices.end(), br, br + 3);
    }
    {
        // Generate corners -- Bottom left
        GLfloat bl[3] = { -128, -128, 128 };

        // Generate corners -- Top Left
        GLfloat tl[3] = { -128, 128, 128 };

        // Generate corners -- Top Right
        GLfloat tr[3] = { 128, 128, 128 };

        // Generate corners -- Bottom Right
        GLfloat br[3] = { 128, -128, 128 };

        // Push our coordinates into the vertex buffer as two triangles (clockwise)
        skyVertices.insert(skyVertices.end(), tl, tl + 3);
        skyVertices.insert(skyVertices.end(), tr, tr + 3);
        skyVertices.insert(skyVertices.end(), bl, bl + 3);
        skyVertices.insert(skyVertices.end(), bl, bl + 3);
        skyVertices.insert(skyVertices.end(), tr, tr + 3);
        skyVertices.insert(skyVertices.end(), br, br + 3);

    }

    {
        // Generate corners -- Bottom left
        GLfloat bl[3] = { 128, -128, 128 };

        // Generate corners -- Top Left
        GLfloat tl[3] = { 128, 128, 128 };

        // Generate corners -- Top Right
        GLfloat tr[3] = { 128, 128, -128 };

        // Generate corners -- Bottom Right
        GLfloat br[3] = { 128, -128, -128 };

        // Push our coordinates into the vertex buffer as two triangles (clockwise)
        skyVertices.insert(skyVertices.end(), tl, tl + 3);
        skyVertices.insert(skyVertices.end(), tr, tr + 3);
        skyVertices.insert(skyVertices.end(), bl, bl + 3);
        skyVertices.insert(skyVertices.end(), bl, bl + 3);
        skyVertices.insert(skyVertices.end(), tr, tr + 3);
        skyVertices.insert(skyVertices.end(), br, br + 3);
    }
    {
        // Generate corners -- Bottom left
        GLfloat bl[3] = { -128, -128, -128 };

        // Generate corners -- Top Left
        GLfloat tl[3] = { -128, 128, -128 };

        // Generate corners -- Top Right
        GLfloat tr[3] = { 128, 128, -128 };

        // Generate corners -- Bottom Right
        GLfloat br[3] = { 128, -128, -128 };

        // Push our coordinates into the vertex buffer as two triangles (clockwise)
        skyVertices.insert(skyVertices.end(), tl, tl + 3);
        skyVertices.insert(skyVertices.end(), tr, tr + 3);
        skyVertices.insert(skyVertices.end(), bl, bl + 3);
        skyVertices.insert(skyVertices.end(), bl, bl + 3);
        skyVertices.insert(skyVertices.end(), tr, tr + 3);
        skyVertices.insert(skyVertices.end(), br, br + 3);
    }

   {
        // Generate corners -- Bottom left
        GLfloat bl[3] = { -128, 128, -128 };

        // Generate corners -- Top Left
        GLfloat tl[3] = { 128, 128, -128 };

        // Generate corners -- Top Right
        GLfloat tr[3] = { 128, 128, 128 };

        // Generate corners -- Bottom Right
        GLfloat br[3] = { -128, 128, 128 };

        // Push our coordinates into the vertex buffer as two triangles (clockwise)
        skyVertices.insert(skyVertices.end(), tl, tl + 3);
        skyVertices.insert(skyVertices.end(), tr, tr + 3);
        skyVertices.insert(skyVertices.end(), bl, bl + 3);
        skyVertices.insert(skyVertices.end(), bl, bl + 3);
        skyVertices.insert(skyVertices.end(), tr, tr + 3);
        skyVertices.insert(skyVertices.end(), br, br + 3);
    }

   {
        // Generate corners -- Bottom left
        GLfloat bl[3] = { -128, -128, -128 };

        // Generate corners -- Top Left
        GLfloat tl[3] = { 128, -128, -128 };

        // Generate corners -- Top Right
        GLfloat tr[3] = { 128, -128, 128 };

        // Generate corners -- Bottom Right
        GLfloat br[3] = { -128, -128, 128 };

        // Push our coordinates into the vertex buffer as two triangles (clockwise)
        skyVertices.insert(skyVertices.end(), tl, tl + 3);
        skyVertices.insert(skyVertices.end(), tr, tr + 3);
        skyVertices.insert(skyVertices.end(), bl, bl + 3);
        skyVertices.insert(skyVertices.end(), bl, bl + 3);
        skyVertices.insert(skyVertices.end(), tr, tr + 3);
        skyVertices.insert(skyVertices.end(), br, br + 3);
    }

   for (int i = 0; i < 6; i++) {
       // Generate corners -- Bottom left
       GLfloat bl_uv[3] = { 0, 0 };
       
       // Generate corners -- Top Left
       GLfloat tl_uv[3] = { 0, 1 };
       
       // Generate corners -- Top Right
       GLfloat tr_uv[3] = { 1, 1 };
       
       // Generate corners -- Bottom Right
       GLfloat br_uv[3] = { 1, 0 };
       
       skyUVs.insert(skyUVs.end(), tl_uv, tl_uv + 2);
       skyUVs.insert(skyUVs.end(), tr_uv, tr_uv + 2);
       skyUVs.insert(skyUVs.end(), bl_uv, bl_uv + 2);
       skyUVs.insert(skyUVs.end(), bl_uv, bl_uv + 2);
       skyUVs.insert(skyUVs.end(), tr_uv, tr_uv + 2);
       skyUVs.insert(skyUVs.end(), br_uv, br_uv + 2);
    }
 }

void generateSplns(){
    
    GLfloat color = 1.0f;
    
    for (int i=0; i<numSplines;i++){
        for (int j=0; j<splines[i].numControlPoints-3;j++){
            
            Point point0= splines[0].points[j];
            Point point1= splines[0].points[j+1];
            Point point2= splines[0].points[j+2];
            Point point3= splines[0].points[j+3];
            
            // brute force
            
            for (float u=0; u < 1.000001f; u+=0.001f){
                
                Point UPoint;
                UPoint.x= (float) sValue * (
                                            (2 * point1.x) +
                                            (-point0.x + point2.x) * u +
                                            (2 * point0.x - 5 * point1.x + 4 * point2.x - point3.x) * (u * u) +
                                            (-point0.x + 3 * point1.x - 3 * point2.x + point3.x) * (u * u * u));
                
                UPoint.y= (float) sValue * (
                                            (2 * point1.y) +
                                            (-point0.y + point2.y) * u +
                                            (2 * point0.y - 5 * point1.y + 4 * point2.y - point3.y) * (u * u) +
                                            (-point0.y + 3 * point1.y - 3 * point2.y + point3.y) * (u * u * u)
                                            );
                
                UPoint.z=  (float) sValue * (
                                             (2 * point1.z) +
                                             (-point0.z + point2.z) * u +
                                             (2 * point0.z - 5 * point1.z + 4 * point2.z - point3.z) * (u * u) +
                                             (-point0.z + 3 * point1.z - 3 * point2.z + point3.z) * (u * u * u)
                                             );
                
                splineVertices.push_back((float)UPoint.x);
                splineVertices.push_back((float)UPoint.y);
                splineVertices.push_back((float)UPoint.z);
                GLfloat colorWF[4] = { 0.0f, 1.0f, 0.0f, 1.0f };
                splinesColor.insert (splinesColor.end(), colorWF, colorWF + 4);
                
            }
        }
    }
     ImageIO * imageIO2= new ImageIO();
    if (imageIO2->loadJPEG("textures/track.jpg") != ImageIO::OK)
    {
        cout << "Error" << endl;
        exit(EXIT_FAILURE); }
     for (int i = 0; i < splineVertices.size() - 3; i++) {
         
        Point point0;
        point0.x =splineVertices[i];
        point0.y= splineVertices[i+1];
        point0.z= splineVertices[i+2];
        
        Point point1;
        point1.x =splineVertices[i+3];
        point1.y= splineVertices[i+4];
        point1.z= splineVertices[i+5];
        
        Point point2;
        point2.x =splineVertices[i+6];
        point2.y= splineVertices[i+7];
        point2.z= splineVertices[i+8];
        
        Point point3;
        point3.x =splineVertices[i+9];
        point3.y= splineVertices[i+10];
        point3.z= splineVertices[i+11];
         
        const float u = 0.9f;
        Point tangent;
        tangent.x = sValue * (
                               (-point0.x + point2.x) +
                               (2 * point0.x - 5 * point1.x + 4 * point2.x - point3.x) * (2 * u) +
                               (-point0.x + 3 * point1.x - 3 * point2.x + point3.x) * (3 * u * u)
                               );
        
        tangent.y = sValue * (
                               (-point0.y + point2.y) +
                               (2 * point0.y - 5 * point1.y + 4 * point2.y - point3.y) * (2 * u) +
                               (-point0.y + 3 * point1.y - 3 * point2.y + point3.y) * (3 * u * u)
                               );
        
        tangent.z = sValue * (
                               (-point0.z + point2.z) +
                               (2 * point0.z - 5 * point1.z + 4 * point2.z - point3.z) * (2 * u) +
                               (-point0.z + 3 * point1.z - 3 * point2.z + point3.z) * (3 * u * u)
                               );
        
        // Compute the unit tangent!
        float magnitude = std::sqrt (std::pow (tangent.x, 2) + std::pow (tangent.y, 2) + std::pow (tangent.z, 2));
        Point unitTangent;
        unitTangent.x = tangent.x / magnitude;
        unitTangent.y = tangent.y / magnitude;
        unitTangent.z = tangent.z / magnitude;
        
        // Compute the normal between two vectors -- used the world y axis to get one axis
        //(a2b3−a3b2)i−(a1b3−a3b1)j+(a1b2−a2b1)k
        Point normal;
        normal.x = (unitTangent.y * 0 - unitTangent.z * 1);
        normal.y = (unitTangent.x * 0 - unitTangent.z * 0);
        normal.z = (unitTangent.x * 1 - unitTangent.y * 0);
        
        // Compute the binormal by crossing the normal with the tangent
        Point binormal;
        binormal.x = (normal.y * unitTangent.z - normal.z * unitTangent.y);
        binormal.y = (normal.x * unitTangent.z - normal.z * unitTangent.x);
        binormal.z = (normal.x * unitTangent.y - normal.y * unitTangent.z);
         
          
         const float scalar = 0.55f;
        GLfloat p[] = {
            // Front face
            // V2
            point0.x + scalar * (normal.x - binormal.x), point0.y + scalar * (normal.y - binormal.y), point0.z + scalar * (normal.z - binormal.z),
            // V1
            point0.x + scalar * (normal.x + binormal.x), point0.y + scalar * (normal.y + binormal.y), point0.z + scalar * (normal.z + binormal.z),
            // V3
            point0.x + scalar * (-normal.x - binormal.x), point0.y + scalar * (-normal.y - binormal.y), point0.z + scalar * (-normal.z - binormal.z),
            // V3
            point0.x + scalar * (-normal.x - binormal.x), point0.y + scalar * (-normal.y - binormal.y), point0.z + scalar * (-normal.z - binormal.z),
            // V1
            point0.x + scalar * (normal.x + binormal.x), point0.y + scalar * (normal.y + binormal.y), point0.z + scalar * (normal.z + binormal.z),
            // V0
            point0.x + scalar * (-normal.x + binormal.x), point0.y + scalar * (-normal.y + binormal.y), point0.z + scalar * (-normal.z + binormal.z),
            
            // Left face
            // V6
            point3.x + scalar * (normal.x - binormal.x), point3.y + scalar * (normal.y - binormal.y), point3.z + scalar * (normal.z - binormal.z),
            // V2
            point0.x + scalar * (normal.x - binormal.x), point0.y + scalar * (normal.y - binormal.y), point0.z + scalar * (normal.z - binormal.z),
            // V7
            point3.x + scalar * (-normal.x - binormal.x), point3.y + scalar * (-normal.y - binormal.y), point3.z + scalar * (-normal.z - binormal.z),
            // V7
            point3.x + scalar * (-normal.x - binormal.x), point3.y + scalar * (-normal.y - binormal.y), point3.z + scalar * (-normal.z - binormal.z),
            // V2
            point0.x + scalar * (normal.x - binormal.x), point0.y + scalar * (normal.y - binormal.y), point0.z + scalar * (normal.z - binormal.z),
            // V3
            point0.x + scalar * (-normal.x - binormal.x), point0.y + scalar * (-normal.y - binormal.y), point0.z + scalar * (-normal.z - binormal.z),
            
            // Right face
            // V1
            point0.x + scalar * (normal.x + binormal.x), point0.y + scalar * (normal.y + binormal.y), point0.z + scalar * (normal.z + binormal.z),
            // V5
            point3.x + scalar * (normal.x + binormal.x), point3.y + scalar * (normal.y + binormal.y), point3.z + scalar * (normal.z + binormal.z),
            // V0
            point0.x + scalar * (-normal.x + binormal.x), point0.y + scalar * (-normal.y + binormal.y), point0.z + scalar * (-normal.z + binormal.z),
            // V0
            point0.x + scalar * (-normal.x + binormal.x), point0.y + scalar * (-normal.y + binormal.y), point0.z + scalar * (-normal.z + binormal.z),
            // V5
            point3.x + scalar * (normal.x + binormal.x), point3.y + scalar * (normal.y + binormal.y), point3.z + scalar * (normal.z + binormal.z),
            // V4
            point3.x + scalar * (-normal.x + binormal.x), point3.y + scalar * (-normal.y + binormal.y), point3.z + scalar * (-normal.z + binormal.z),
            
            // Top face
            // V6
            point3.x + scalar * (normal.x - binormal.x), point3.y + scalar * (normal.y - binormal.y), point3.z + scalar * (normal.z - binormal.z),
            // V5
            point3.x + scalar * (normal.x + binormal.x), point3.y + scalar * (normal.y + binormal.y), point3.z + scalar * (normal.z + binormal.z),
            // V2
            point0.x + scalar * (normal.x - binormal.x), point0.y + scalar * (normal.y - binormal.y), point0.z + scalar * (normal.z - binormal.z),
            // V2
            point0.x + scalar * (normal.x - binormal.x), point0.y + scalar * (normal.y - binormal.y), point0.z + scalar * (normal.z - binormal.z),
            // V5
            point3.x + scalar * (normal.x + binormal.x), point3.y + scalar * (normal.y + binormal.y), point3.z + scalar * (normal.z + binormal.z),
            // V1
            point0.x + scalar * (normal.x + binormal.x), point0.y + scalar * (normal.y + binormal.y), point0.z + scalar * (normal.z + binormal.z),
            
            // Bottom face
            // V7
            point3.x + scalar * (-normal.x - binormal.x), point3.y + scalar * (-normal.y - binormal.y), point3.z + scalar * (-normal.z - binormal.z),
            // V4
            point3.x + scalar * (-normal.x + binormal.x), point3.y + scalar * (-normal.y + binormal.y), point3.z + scalar * (-normal.z + binormal.z),
            // V3
            point0.x + scalar * (-normal.x - binormal.x), point0.y + scalar * (-normal.y - binormal.y), point0.z + scalar * (-normal.z - binormal.z),
            // V3
            point0.x + scalar * (-normal.x - binormal.x), point0.y + scalar * (-normal.y - binormal.y), point0.z + scalar * (-normal.z - binormal.z),
            // V4
            point3.x + scalar * (-normal.x + binormal.x), point3.y + scalar * (-normal.y + binormal.y), point3.z + scalar * (-normal.z + binormal.z),
            // V0
            point0.x + scalar * (-normal.x + binormal.x), point0.y + scalar * (-normal.y + binormal.y), point0.z + scalar * (-normal.z + binormal.z),
            
            // Back face
            // V6
            point3.x + scalar * (normal.x - binormal.x), point3.y + scalar * (normal.y - binormal.y), point3.z + scalar * (normal.z - binormal.z),
            // V5
            point3.x + scalar * (normal.x + binormal.x), point3.y + scalar * (normal.y + binormal.y), point3.z + scalar * (normal.z + binormal.z),
            // V7
            point3.x + scalar * (-normal.x - binormal.x), point3.y + scalar * (-normal.y - binormal.y), point3.z + scalar * (-normal.z - binormal.z),
            // V7
            point3.x + scalar * (-normal.x - binormal.x), point3.y + scalar * (-normal.y - binormal.y), point3.z + scalar * (-normal.z - binormal.z),
            // V5
            point3.x + scalar * (normal.x + binormal.x), point3.y + scalar * (normal.y + binormal.y), point3.z + scalar * (normal.z + binormal.z),
            // V4
            point3.x + scalar * (-normal.x + binormal.x), point3.y + scalar * (-normal.y + binormal.y), point3.z + scalar * (-normal.z + binormal.z),
        };
         
        // CameraPosition(point0, point1,point2,point3);

        trackVertices.insert (trackVertices.end(), p, p + 108);
        
        for (int j = 0; j < 36; j++) {
            
            GLfloat bl_uv[3] = { 0, 0 };
            
            // Generate corners -- Top Left
            GLfloat tl_uv[3] = { 0, 1 };
            
            // Generate corners -- Top Right
            GLfloat tr_uv[3] = { 1, 1 };
            
            // Generate corners -- Bottom Right
            GLfloat br_uv[3] = { 1, 0 };
            
            trackUVs.insert(trackUVs.end(), tl_uv, tl_uv + 2);
            trackUVs.insert(trackUVs.end(), tr_uv, tr_uv + 2);
            trackUVs.insert(trackUVs.end(), bl_uv, bl_uv + 2);
            trackUVs.insert(trackUVs.end(), bl_uv, bl_uv + 2);
            trackUVs.insert(trackUVs.end(), tr_uv, tr_uv + 2);
            trackUVs.insert(trackUVs.end(), br_uv, br_uv + 2);
        }
 
    }
    
}

void initScene(int argc, char *argv[])
{    // load the splines from the provided filename
    loadSplines(argv[1]);
    printf("Loaded %d spline(s).\n", numSplines);
    for(int i=0; i<numSplines; i++)
        printf("Num control points in spline %d: %d.\n", i, splines[i].numControlPoints);
    
    glGenTextures(1, &texHandle1);
    
    int code= initTexture ("textures/grnd.jpg", texHandle1);
    if (code !=0)
    { printf ("Error loading the texture image .\n ");
        exit(EXIT_FAILURE);
    }
   
    glGenTextures(1, &texHandle);
     code= initTexture ("textures/sky.jpg", texHandle);
    if (code !=0)
    { printf ("Error loading the texture image .\n ");
        exit(EXIT_FAILURE);
    }
    
    glGenTextures(1, &texHandle2);
    code= initTexture ("textures/track.jpg", texHandle2);
    if (code !=0)
    { printf ("Error loading the texture image .\n ");
        exit(EXIT_FAILURE);
    }

    
     glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    //init Pipeline Program
     initPipeLine();
      generateTerrain();
     generateSky();
     generateSplns();
    //initilize buffer
    initBuffer();
    //enable depth buffer
    glEnable(GL_DEPTH_TEST);
    
}
int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf ("usage: %s <trackfile>\n", argv[0]);
        exit(0);
    }
    cout << "Initializing GLUT..." << endl;
    glutInit(&argc,argv);
    
    cout << "Initializing OpenGL..." << endl;
    
#ifdef __APPLE__
    glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
#else
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
#endif
    
    glutInitWindowSize(windowWidth, windowHeight);
    glutInitWindowPosition(0, 0);
    glutCreateWindow(windowTitle);
    cout << "OpenGL Version: " << glGetString(GL_VERSION) << endl;
    cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << endl;
    cout << "Shading Language Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
    // tells glut to use a particular display function to redraw
    glutDisplayFunc(displayFunc);
    // perform animation inside idleFunc
    glutIdleFunc(idleFunc);
    // callback for mouse drags
    glutMotionFunc(mouseMotionDragFunc);
    // callback for idle mouse movement
    glutPassiveMotionFunc(mouseMotionFunc);
    // callback for mouse button changes
    glutMouseFunc(mouseButtonFunc);
    // callback for resizing the window
    glutReshapeFunc(reshapeFunc);
    // callback for pressing the keys on the keyboard
    glutKeyboardFunc(keyboardFunc);
    // init glew
#ifdef __APPLE__
    // nothing is needed on Apple
#else
    // Windows, Linux
    GLint result = glewInit();
    if (result != GLEW_OK)
    {
        cout << "error: " << glewGetErrorString(result) << endl;
        exit(EXIT_FAILURE);
    }
#endif
    
    // do initialization
    initScene(argc, argv);
    // sink forever into the glut loop
    glutMainLoop();
 }
