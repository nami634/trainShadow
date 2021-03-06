#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){
    // of & gl init
    ofSetVerticalSync(false);
    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
    ofEnableDepthTest();
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_CULL_FACE);
    GLint textureUnits = 0;
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &textureUnits);
//    cout << "texture units " + ofToString(  cntextureUnits) << endl;
    light_max = textureUnits;
    //
    
    // depth fbo
    float length = 4096;
    ofFbo::Settings settings;
    settings.width = length;
    settings.height = length;
    settings.useDepth = true;
    settings.useStencil = true;
    settings.depthStencilAsTexture = true;
    fbo.allocate(settings);
    //
    
    // shader
    shader.load("vert.vert", "frag.frag");
    d_shader.load("dvert.vert", "dfrag.frag");
    //
    
    // osc
    receiver.setup(12345);
    //
    
    // cam
    cam.setPosition(vec3(0,300,0));
    cam.setUpAxis(vec3(0,1,0));
    cam.setFarClip(1500.);
    //
    
    // light
//    light.setPosition(vec3(150, 100, -500));
//    light.setFov(50.0f);
//    light.lookAt(light.getGlobalPosition() + vec3(-100, 0,0));
//    light.rotateDeg(20.0f, vec3(0,0,1));
    
    light_count = 0;
    //
    
    // box
    box.setPosition(vec3(110, 100,-100));
    box.set(50);
    //
    
    // model
    loader.setScale(2.0,2.0,2.0);
    loader.loadModel("test.obj");
    
    ofLogNotice() << loader.getMeshCount();
    
    mesh = loader.getMesh(0);
    for (int i = 0; i < mesh.getNumVertices(); i++) mesh.addColor(ofColor(255,255,255,255));
    //
    
    // test
    tolerate = 0.;

    
    
    for (int i = 0; i < 30; ++i) {
        vbo.addVertex(vec3(ofRandom(-15, 15), ofRandom(15)+50, i*33.3 -500));
    }
    
    cout << (1L << 4L) << endl;
}

//--------------------------------------------------------------
void ofApp::update(){
    // matrix
    tmpm = cam.getModelViewProjectionMatrix();
    //
    
    // model
    loader.update();
    //
    
    // osc
    while (receiver.hasWaitingMessages()) {
        ofxOscMessage m;
        receiver.getNextMessage(m);
        if (m.getAddress() == "/light") {
            light.setPosition(m.getArgAsFloat(0), m.getArgAsFloat(1), m.getArgAsFloat(2));
        } else if (m.getAddress() == "/box") {
            boxes.clear();
            vbo.clear();
            for (int i = 0; i < 30; ++i) {
                vbo.addVertex(vec3(ofRandom(50) + 100, ofRandom(100), i*33.3 -500));
            }

        } else if (m.getAddress() == "/create_light") {
            ofCamera cam;
            lights.push_back(*new townLight(&fbo, &d_shader));
            light_count++;
            if (light_count >= light_max) light_count = 0;
        } else if (m.getAddress() == "/test") {
//            lights[0].position = vec3(m.getArgAsFloat(0), m.getArgAsFloat(1), m.getArgAsFloat(2));
        } else if (m.getAddress() == "/tolerate") {
//            tolerate = m.getArgAsFloat(0);
            lights[0].light.setFarClip(m.getArgAsFloat(0));
        }
    }
    //
    
    // lights update
    if (lights.size() > 0) {
        auto itr = lights.begin();
        while (itr != lights.end()) {
            if (lights[std::distance(lights.begin(),itr)].update()) {
                lights.erase(itr);
            } else {
                itr++;
            }
        }
    }
    //
    int g = int(ofGetElapsedTimef() * 20.) % 30;
    
    vbo.getVertices()[g] = vec3(ofRandom(-25, 50), ofRandom(75)+25, g*33.3 -500);
}

//--------------------------------------------------------------
void ofApp::draw(){
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    
    mat4 bmm = box.getGlobalTransformMatrix();
    
    mmm = mat4(loader.getModelMatrix());
    mmm = rotate(mmm, radians(180.0f), vec3(0.0f,0.0f,1.0f));
    
    
    for (int i = 0; i < lights.size(); i++) {
        lights[i].begin();
        
//        d_shader.begin();
        d_shader.setUniform1f("clipD", lights[i].light.getFarClip()- lights[i].light.getNearClip());
//        d_shader.setUniformMatrix4f("lgtMatrix", lights[i].light.getModelViewProjectionMatrix() * bmm);
//        box.draw();
        
//        for (auto b : boxes) {
//            d_shader.setUniformMatrix4f("lgtMatrix", lights[i].light.getModelViewProjectionMatrix() * b.getGlobalTransformMatrix());
//            b.draw();
//        }
        d_shader.setUniformMatrix4f("lgtMatrix", lights[i].light.getModelViewProjectionMatrix());
        vbo.setMode(OF_PRIMITIVE_TRIANGLE_STRIP);
        glDisable(GL_CULL_FACE);
        vbo.draw();
        
        
        d_shader.setUniformMatrix4f("lgtMatrix", lights[i].light.getModelViewProjectionMatrix() * mmm);
        mesh.draw();
//        d_shader.end();
        lights[i].end();
    }

    
    
    // render fbo init
    ofClear(180, 180, 180, 255);
    glClearDepth(1.0);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    glViewport(0, 0, ofGetWidth(), ofGetHeight());
    //
    
    ofPushMatrix();
    shader.begin();
    cam.begin();
    glEnable(GL_CULL_FACE);
    ofEnableDepthTest();
    
    array<mat4, 5> tMatrixes;
    array<mat4, 5> lgtMatrixes;
    array<vec3, 5> lightPositions;
    vector<ofTexture> d_textures;
    
    GLfloat clipDs[5];
    GLint active_lights[5];

    
    for (int i = 0; i < 5; i++) {
        if (i >= lights.size()) {
            active_lights[i] = false;
        } else {
            d_textures.push_back(lights[i].getTexture());
            
            active_lights[i] = true;
            clipDs[i] = lights[i].light.getFarClip() - lights[i].light.getNearClip();
            tMatrixes[i] = lights[i].tm;
            lgtMatrixes[i] = lights[i].light.getModelViewProjectionMatrix();
            lightPositions[i] = lights[i].light.getGlobalPosition();
        }
    }
    
    shader.setUniform1iv("active_light", &active_lights[0], 5);
    shader.setUniform1fv("clipD", &clipDs[0], 5);
    shader.setUniform3fv("lightPosition", &lightPositions[0].x, lightPositions.size());
    shader.setUniformMatrix4f("tMatrix", tMatrixes[0], tMatrixes.size());
    shader.setUniformMatrix4f("lgtMatrix", lgtMatrixes[0], lgtMatrixes.size());
    shader.setUniformArrayTexture("d_texture", d_textures);

    
    shader.setUniformMatrix4f("mMatrix", mat4());
    shader.setUniformMatrix4f("mvpMatrix", tmpm);
    shader.setUniformMatrix4f("invMatrix", inverse(mat4()));
    vbo.setMode(OF_PRIMITIVE_TRIANGLE_STRIP);
    glDisable(GL_CULL_FACE);
    vbo.draw();
    glEnable(GL_CULL_FACE);
    
    
    ofSetColor(255, 255, 255);
//    box.draw();
    
    shader.setUniformMatrix4f("mMatrix", mmm);
    shader.setUniformMatrix4f("mvpMatrix", tmpm * mmm);
    shader.setUniformMatrix4f("invMatrix", inverse(mmm));
    shader.setUniform4f("ambientColor", vec4(0.1,0.1,0.1,1.0));
    
    mesh.draw();
    
    cam.end();
    shader.end();
    
    // debug
    cam.begin();
//    light.draw();
//    ofDrawSphere(light.getGlobalPosition() + light.getLookAtDir() * 10., 5);
    for(auto l : lights) {
//        l.light.draw();
    }
    cam.end();
    //
    ofPopMatrix();
    
    glDisable(GL_CULL_FACE);
    ofSetColor(0,0,255,255);
    ofDrawBitmapString(ofToString(ofGetFrameRate()), 5, 10);
    
    for (int i = 0; i < lights.size(); i++) {
        ofSetColor(255,255,255,255);
//        ofPushMatrix();
//        lights[i].getTexture().draw(i * 100, 0, 100, 100);
//        ofPopMatrix();
    }
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
