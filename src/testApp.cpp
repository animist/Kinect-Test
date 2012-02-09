#include "testApp.h"

#define MAX_NUM 500
// 30 フレームで 1秒
#define FPS 30

// パーティクルの情報を格納
ofPoint points[MAX_NUM];
ofPoint direction[MAX_NUM];
float rad[MAX_NUM];
ofColor colors[MAX_NUM];

// ウィンドウサイズやカメラ画像のサイズ
int winWidth = 640;
int winHeight = 480;
int camWidth, camHeight;

// 挙動を変化させるためのグローバル変数
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

    // パーティクルのサイズを適当に初期化
    for (int i = 0; i < MAX_NUM; ++i) {
        rad[i] = ofRandom(5.0, 20.0);
    }
    // パーティクルの初期位置と色を適当に初期化
    for (int i = 0; i < MAX_NUM; ++i) {
        int x = ofRandom(winWidth + rad[i] + 1, 1);
        int y = ofRandom(winHeight - rad[i] - 1, 1);

        colors[i].r = ofRandom(255);
        colors[i].g = ofRandom(255);
        colors[i].b = ofRandom(255);
        colors[i].a = ofRandom(255);

        points[i].set(x, y);
    }
    // パーティクルの移動速度を適当に初期化
    for (int i = 0; i < MAX_NUM; ++i) {
        float x = ofRandom(-5, 5);
        float y = ofRandom(-5, 5);

        direction[i].set(x, y);
    }

    // Kinect ハードウェア回りを初期化
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
    // 各パーティクルを動かす
    for (int i = 0; i < MAX_NUM; ++i) {
        if (i == 0) { printf("X : %f, Y : %f\n", direction[i].x, direction[i].y); }
        if (i == 0) { printf("mouseX : %d, mouseY : %d\n", mouseX, mouseY); }

        // もし手を見つけていたら、そこに集まるように
        if (centering){
            float x = direction[i].x;
            float y = direction[i].y;

            // 手とパーティクル自身の位置で挙動を変化させる(X軸編)
            if (abs(mouseX - points[i].x) > 30) {
                // 遠ければ素早く
                points[i].x += (mouseX > points[i].x) ? 30 : -30;
            }
            else if (abs(mouseX - points[i].x) > 10) {
                // 近づいたらだんだんゆっくり
                points[i].x += (mouseX > points[i].x) ? 10 : -10;
            }
            else {
                // 微調整
                points[i].x += (mouseX > points[i].x) ? 1 : -1;
            }

            // Y軸でも同じように近づける
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
        // 手を検出していなかったらランダム方向に動かす
        else {
            // 加速
            points[i].x += direction[i].x;
            points[i].y += direction[i].y;

            // 端っこまで来たら反転させる。
            // ギリギリまで行き過ぎるとまれに通過するので半径分余裕を見る
            if (points[i].x + rad[i] > ofGetWidth() || points[i].x - rad[i] < 0) {
                direction[i].x *= -1;
            }
            if (points[i].y + rad[i] > ofGetHeight() || points[i].y - rad[i] < 0) {
                direction[i].y *= -1;
            }
        }
    }

    // Kinect から来てる情報を更新
    ofxOpenNICtx.update();
    depthGenerator.update();
    imageGenerator.update();

    // 認識してる手の数を数える
    int trackedHands = handGenerator.getNumTrackedHands();
    // 一個目の手の詳細を得る
    ofxTrackedHand *hand = handGenerator.getHand(0);

    // 手が見つかってたら特別な処理をする
    if (trackedHands > 0) {
        printf("I Detected %d Hand(s)!!!\n", handGenerator.getNumTrackedHands());

        // 「投げる」コマンド実行のための時間を決める閾値
        // どのくらいの時間、手が前に出続けたら投げたと判断するか
        int throwThreshold = FPS / 3;

        // 投げたと判断した時の処理
        if (stateCounter < throwThreshold) {
            // フラグを立てる
            centering = true;
            // 手の X 座標と Y 座標を保存
            mouseX = hand->projectPos.x;
            mouseY = hand->projectPos.y;
        }
        // 投げ終えた後の処理
        // 適当な余裕時間を見るようにしている
        else if (stateCounter > FPS * 6) {
            stateCounter = 0;
        }

        // 「投げる」コマンドの判断を行う
        // フラグが立っている最中に、手が直前のフレームの計測時より前に出ているか
        if (centering && hand->projectPos.z < prevDepth) {
            // 条件に会えばカウンターを加算
            ++stateCounter;
            // カウンターが、閾値を越えたら投げたものと判断する
            if (stateCounter > throwThreshold) {
                printf(" ================> throw Now! \n");
                // 投げ終えたのでフラグをへし折る
                centering = false;
            }
        // 手は見つかっているがフラグは立っていない状態の処理
        // 投げ終えて、パーティクルが飛び散っている間の時間
        } else if (!centering) {
            // カウンターを加算
            ++stateCounter;
        }
        else {
            // 条件に見合わなければカウンターは初期化
            stateCounter = 0;
        }
        printf("hand Depth ----------> %f\n", hand->projectPos.z);
        printf("stateCounter : %d\n", stateCounter);
        prevDepth = hand->projectPos.z;
    }
    // 手が無いときはフラグをへし折る
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

    // Kinect のカメラの画像を描画
    imageGenerator.draw(0,0,winWidth,winHeight);

    // 各パーティクルを描画
    for (int i = 0; i < MAX_NUM; ++i) {
        ofSetColor(colors[i]);
        ofCircle(points[i].x, points[i].y, rad[i]);
    }
}

//--------------------------------------------------------------
void testApp::keyPressed(int key){
    // s のキーが押されたら、色を再度割り振りなおす
    if (key == 's') {
        for (int i = 0; i < MAX_NUM; ++i) {
            colors[i].r = ofRandom(255);
            colors[i].g = ofRandom(255);
            colors[i].b = ofRandom(255);
            colors[i].a = ofRandom(255);
        }
    }
    // h のキーが押されている最中は x 軸方向の加速度を減算
    if (key == 'h'){
        for (int i=0; i < MAX_NUM; ++i) {
            direction[i].set(direction[i].x-1,0);
        }
    }
    // j のキーが押されている最中は y 軸方向の加速度を加算
    if (key == 'j'){
        for (int i=0; i < MAX_NUM; ++i) {
            direction[i].set(0,direction[i].y+1);
        }
    }
    // k のキーが押されている最中は y 軸方向の加速度を減算
    if (key == 'k'){
        for (int i=0; i < MAX_NUM; ++i) {
            direction[i].set(0,direction[i].y-1);
        }
    }
    // l のキーが押されている最中は x 軸方向の加速度を加算
    if (key == 'l'){
        for (int i=0; i < MAX_NUM; ++i) {
            direction[i].set(direction[i].x+1,0);
        }
    }
}

//--------------------------------------------------------------
void testApp::keyReleased(int key){
    // 移動をつかさどる各キーが離されたら初期化
    if (key == 'j' || key == 'h' || key == 'k' || key == 'l') {
        for (int i=0; i < MAX_NUM; ++i) {
            // 各パーティクルの加速度をランダムに設定しなおす
            direction[i].set(ofRandom(-5, 5),ofRandom(-5, 5));

            // キーを押し続けると、壁にメリ込んじゃう子が居るので救出
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
    // マウスがクリックされたらフラグを立てて、カーソル位置を記録
    centering = true;
    mouseX = x;
    mouseY = y;
}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button){
    // マウスが離されたらフラグをへし折る
    centering = false;
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
    // 終了時のお約束
    ofxOpenNICtx.shutdown();
}
