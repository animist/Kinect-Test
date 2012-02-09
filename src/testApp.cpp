#include "testApp.h"

#define MAX_NUM 500
// 30 フレームで 1秒
#define FPS 30

ofPoint points[MAX_NUM];
ofPoint direction[MAX_NUM];
float rad[MAX_NUM];
ofColor colors[MAX_NUM];

ofImage img;

int winWidth = 640;
int winHeight = 480;
int camWidth, camHeight;
float x;
float y;

bool centering = false;
float mouseX = 0;
float mouseY = 0;

float prevDepth;
int stateCounter;

//--------------------------------------------------------------
void testApp::setup(){
    ofSetFrameRate(FPS);
    ofBackground(0,0,0);    
    ofEnableSmoothing();
    ofSetWindowShape(winWidth, winHeight);
    ofEnableAlphaBlending();
    
    camWidth = winWidth;
    camHeight = winHeight;
    
    for (int i = 0; i < MAX_NUM; ++i) {
        rad[i] = ofRandom(5.0, 20.0);
    }        
    for (int i = 0; i < MAX_NUM; ++i) {
        int x = ofRandom(winWidth + rad[i] + 1, 1);
        int y = ofRandom(winHeight - rad[i] - 1, 1);

        colors[i].r = ofRandom(255);
        colors[i].g = ofRandom(255);
        colors[i].b = ofRandom(255);
        colors[i].a = ofRandom(255);
        
        points[i].set(x, y);
    }
    for (int i = 0; i < MAX_NUM; ++i) {
        float x = ofRandom(-5, 5);
        float y = ofRandom(-5, 5);
        
        direction[i].set(x, y);
    }
    
    ofxOpenNICtx.setupUsingXMLFile();
    depthGenerator.setup(&ofxOpenNICtx);
    imageGenerator.setup(&ofxOpenNICtx);
    handGenerator.setup(&ofxOpenNICtx, 2);
    
    handGenerator.setSmoothing(0.1f);
    
    ofxOpenNICtx.toggleRegisterViewport();
    ofxOpenNICtx.toggleMirror();
}

//--------------------------------------------------------------
void testApp::update(){
    
    x += 1;
    if (x > winWidth) { x = 0; }
    y += 1;
    if (y > winHeight) { y = 0; }

    
    for (int i = 0; i < MAX_NUM; ++i) {
        if (i == 0) { printf("X : %f, Y : %f\n", direction[i].x, direction[i].y); }
        if (i == 0) { printf("mouseX : %d, mouseY : %d\n", mouseX, mouseY); }
        
        if (centering){
        
            float x = direction[i].x;
            float y = direction[i].y;
            
            if (abs(mouseX - points[i].x) > 30) {
                points[i].x += (mouseX > points[i].x) ? 30 : -30;
            }
            else if (abs(mouseX - points[i].x) > 10) {
                points[i].x += (mouseX > points[i].x) ? 10 : -10;
            }
            else {
                points[i].x += (mouseX > points[i].x) ? 1 : -1;
            }
            if (abs(mouseY - points[i].y) > 30) {
                points[i].y += (mouseY > points[i].y) ? 30 : -30;
            }
            else if (abs(mouseY - points[i].y) > 10) {
                points[i].y += (mouseY > points[i].y) ? 10 : -10;
            }
            else {
                points[i].y += (mouseY > points[i].y) ? 1 : -1;
            }
        }
        else {
            points[i].x += direction[i].x;
            points[i].y += direction[i].y;
            
            if (points[i].x + rad[i] > ofGetWidth() || points[i].x - rad[i] < 0) {
                direction[i].x *= -1;
            }
            if (points[i].y + rad[i] > ofGetHeight() || points[i].y - rad[i] < 0) {
                direction[i].y *= -1;
            }
        }
    }
    
    ofxOpenNICtx.update();
    depthGenerator.update();
    imageGenerator.update();
    
    int trackedHands = handGenerator.getNumTrackedHands();
    ofxTrackedHand *hand = handGenerator.getHand(0);
    if (trackedHands > 0) {
        printf("I Detected %d Hand(s)!!!\n", handGenerator.getNumTrackedHands());
        
        int throwThreshold = FPS / 3;
        if (stateCounter < throwThreshold) {
            centering = true;
            mouseX = hand->projectPos.x;
            mouseY = hand->projectPos.y;
        }
        else if (stateCounter > FPS * 6) {
            stateCounter = 0;
        }
        
        if (centering && hand->projectPos.z < prevDepth) {
            ++stateCounter;
            if (stateCounter > throwThreshold) {
                printf(" ================> throw Now! \n");
                centering = false;
            }
        } else if (!centering) {
            ++stateCounter;
        }
        else {
            stateCounter = 0;
        }
        printf("hand Depth ----------> %f\n", hand->projectPos.z);
        printf("stateCounter : %d\n", stateCounter);
        prevDepth = hand->projectPos.z;
    }
    else {
        centering = false;
        printf("I lost hand......\n");
    }
    printf("  ---  \n");
    
    // 直前の手の深度と現在の手の深度を比較
    // 近づいていればカウントアップ
    // 30越えたら1越えたものとしてフラグを立てる
    // 
    // 120越えたらフラグを戻す
}

//--------------------------------------------------------------
void testApp::draw(){
    ofSetColor(255,255,255);
    ofFill();
    glEnable(GL_BLEND);
    imageGenerator.draw(0,0,winWidth,winHeight);

    for (int i = 0; i < MAX_NUM; ++i) {
        ofSetColor(colors[i]);
        ofCircle(points[i].x, points[i].y, rad[i]);
    }
}

//--------------------------------------------------------------
void testApp::keyPressed(int key){
    if (key == 's' || key == 'S') {
        for (int i = 0; i < MAX_NUM; ++i) {
            colors[i].r = ofRandom(255);
            colors[i].g = ofRandom(255);
            colors[i].b = ofRandom(255);
            colors[i].a = ofRandom(255);
        }
    }
    if (key == 'j'){
        for (int i=0; i < MAX_NUM; ++i) {
            direction[i].set(0,direction[i].y+1);
        }
    }
    if (key == 'k'){
        for (int i=0; i < MAX_NUM; ++i) {
            direction[i].set(0,direction[i].y-1);
        }
    }
    if (key == 'l'){
        for (int i=0; i < MAX_NUM; ++i) {
            direction[i].set(direction[i].x+1,0);
        }
    }
    if (key == 'h'){
        for (int i=0; i < MAX_NUM; ++i) {
            direction[i].set(direction[i].x-1,0);
        }
    }
}

//--------------------------------------------------------------
void testApp::keyReleased(int key){
    if (key == 'j' || key == 'h' || key == 'k' || key == 'l') {
        for (int i=0; i < MAX_NUM; ++i) {
            direction[i].set(ofRandom(-5, 5),ofRandom(-5, 5));
            if (points[i].x - rad[i] * 2 < 0) {
                points[i].x = rad[i] + 1;   
            } else if (points[i].x > winWidth - rad[i]) {
                points[i].x = winWidth - rad[i] * 2;
            }
            if (points[i].y - rad[i] * 2 < 0) {
                points[i].y = rad[i] + 1;   
            } else if (points[i].y > winHeight - rad[i]) {
                points[i].y = winHeight - rad[i] * 2;
            }            
        }
    }
}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y ){
    
}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button){
    centering = true;
    mouseX = x;
    mouseY = y;
}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button){
    centering = false;
    for (int i=0; i < MAX_NUM; ++i) {
        points[i].set(x, y);
    }
}

//--------------------------------------------------------------
void testApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void testApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void testApp::dragEvent(ofDragInfo dragInfo){ 

}

void testApp::exit() {
    ofxOpenNICtx.shutdown();
}