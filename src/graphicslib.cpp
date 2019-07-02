#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include <graphicslib.hpp>
#include <utils.hpp>
#include <matrixlib.hpp>
#include <shader.hpp>
#include <model.hpp>
#include <camera.hpp>

#include <glm/gtc/type_ptr.hpp>


#define FILE "scene.txt"


namespace graphicslib {

    // camera
    Camera camera;
    float lastX;
    float lastY;
    float firstMouse = true;

    // lighting
    glm::vec3 lightPos(1.2f, 1.0f, 2.0f);

    //initialize glfw stuff
    Window::Window(int windowWidth, int windowHeight){
        //listen for errors generated by glfw
        glfwSetErrorCallback(glfwErrorCallback);

        //initialize glfw
        glfwInit();

        //set some window options
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_RESIZABLE, GL_TRUE); //make window resizable
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); //compatibility to mac os users

        mWindowWidth = windowWidth;
        mWindowHeight = windowHeight;
        mWindow = NULL;
        mCoreProgram = 0;


        lastX = windowWidth/2.0f;
        lastY = windowHeight/2.0f;

        mPhong = true;
        mPReleased = true;

        // timing
        mDeltaTime = 0.0f;
        mLastFrame = 0.0f;
    }

    //destroy everything
    Window::~Window(){
        //destroy window
        if(mWindow){
            glfwDestroyWindow(mWindow);
        }
        //delete program
        if(mCoreProgram){
            glDeleteProgram(mCoreProgram);
        }
        glfwTerminate();
    }


    //create the window, load glad, load shaders
    void Window::createWindow() {
        //create window
        mWindow = glfwCreateWindow(mWindowWidth, mWindowHeight, "Camera View", NULL, NULL);

        if(mWindow == NULL) {
            std::cerr << "Failed to create GLFW window" << std::endl;
            this->~Window();
            return;
        }

        //make context current
        glfwMakeContextCurrent(mWindow); //IMPORTANT!!

        //set callback function to call when resize
        glfwSetFramebufferSizeCallback(mWindow, framebufferResizeCallback);
        glfwSetCursorPosCallback(mWindow, mouseCallback);

        // tell GLFW to capture our mouse
        glfwSetInputMode(mWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        //enable vsync
        glfwSwapInterval(1);

        //glad: load all OpenGL function pointers
        if(!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)){
            std::cerr << "Failed to initialize GLAD" << std::endl;
            this->~Window();
            return;
        }

        //opengl options
        //make possible using 3d
        glEnable(GL_DEPTH_TEST);

        //allow us to change the size of a point
        glEnable(GL_PROGRAM_POINT_SIZE);

    }


    void Window::run(){

        //------------------//
        //READ THE MODEL.TXT//
        //------------------//

        std::ifstream modelFile("model.txt");
        std::string modelPath;
        int i = 0;
        // build and compile shaders

        //delta is the distance between the origin of the coordinate system and the center of the i model
        std::vector<float> delta;
        delta.push_back(0);

        std::vector<Model> models;
        //previousModelSize is the half the x-axis size of the model on the left of the current model
        //it is used to obtain the delta value for the current model
        float previousModelSize = 0;
        // load models
        while(std::getline(modelFile, modelPath)){

            Model model(modelPath);

            // calculate the bounding box of the model
            model.calcBoundingBox();

            // size of the biggest dimension of the model
            float size = model.biggestDimensionSize();

            // initial rotation
            modelCoord.rotation[0] = 0.f;
            modelCoord.rotation[1] = 0.f;
            modelCoord.rotation[2] = 0.f;

            // initial scale
            modelCoord.scale[0] = 2.f/size;
            modelCoord.scale[1] = 2.f/size;
            modelCoord.scale[2] = 2.f/size;

            modelCoord.position[0] = -(model.boundingBox.x.center);
            modelCoord.position[1] = -(model.boundingBox.y.center);
            modelCoord.position[2] = -(model.boundingBox.z.center);

            //to set the delta distance, we need the delta of the previous model, half of its size, half of the
            //size of the current model and a constant value (so the models won't be rendered too close to each other)
            if(i != 0) {
                delta.push_back(delta[i - 1] + previousModelSize + modelCoord.scale[0]*model.boundingBox.x.size/2 + 0.25);
            }
            previousModelSize = modelCoord.scale[0]*model.boundingBox.x.size/2;

            i++;
            modelCoordVector.push_back(modelCoord);
            models.push_back(model);
        }

        modelFile.close();




        //------------------//
        //READ THE SCENE.TXT//
        //------------------//

        std::ifstream sceneFile(FILE);
        //check if an error has ocurred while opening the file
        if(!sceneFile){
            std::cerr << "*** Error while opening file " << FILE << " ***" << std::endl;
            exit(EXIT_FAILURE);
        }
        std::string line;

        //initialize the number of lights
        lightingInformation.numberOfPointLights = 0;
        //read a line of the file
        while(std::getline(sceneFile, line)){
            //treat the case of having \n in the end of the file
            if(line.compare("") == 0){
                continue;
            }

            // first word of the line
            std::string firstWord;

            //read the first word of the line
            std::istringstream lineStream(line);
            lineStream >> firstWord;

            //if it's adding a light in the scene
            if(firstWord == "light"){
                //get the point light to alter
                int index = lightingInformation.numberOfPointLights;
                PointLight* currentPointLight = &lightingInformation.pointLights[index];

                char auxString[20];
                //read and store the information in the point light
                sscanf(lineStream.str().c_str(), "%s  %f %f %f  %f %f %f  %f %f %f",
                       auxString,
                       &(currentPointLight->position[0]),
                       &(currentPointLight->position[1]),
                       &(currentPointLight->position[2]),
                       &(currentPointLight->ambient[0]),
                       &(currentPointLight->ambient[1]),
                       &(currentPointLight->ambient[2]),
                       &(currentPointLight->linear),
                       &(currentPointLight->constant),
                       &(currentPointLight->quadratic));
                currentPointLight->diffuse = currentPointLight->ambient;
                currentPointLight->specular = currentPointLight->ambient;


                //also alter the point light for buffer
                PointLightForBuffer* currentPointLightForBuffer = &lightingInformation.bufferOfPointLights[index];
                currentPointLightForBuffer->position = currentPointLight->position;
                currentPointLightForBuffer->color = currentPointLight->ambient;

                //increment the number of point lights
                lightingInformation.numberOfPointLights++;
            }

            // if it's defining a camera
            else if(firstWord == "camera"){
                glm::vec3 position, up, lookAt;
                // read information about the camera (position, lookAt point and view up vector)
                lineStream >> position.x >> position.y >> position.z
                           >> lookAt.x >> lookAt.y >> lookAt.z
                           >> up.x >> up.y >> up.z;
                camera = Camera(position, up, lookAt);
            }

        }
        sceneFile.close();




        //-------------//
        //SETUP SHADERS//
        //-------------//

        //load the point lights VAO (buffer already filled)
        unsigned int pointLightsVAO = loadPointLightsVAO();

        //load the cube
        unsigned int cubeVAO = loadCubeVAO();

        float currentFrame;
        ml::matrix<float> projection(4, 4);
        ml::matrix<float> view(4, 4);
        ml::matrix<float> modelMatrix(4, 4, true);


        Shader phongShader("src/multipleLightsPhong.vs", "src/multipleLightsPhong.fs");
        //set phong shader material properties
        phongShader.use();
        phongShader.setInt("material.diffuse", 0);
        phongShader.setInt("material.specular", 1);
        phongShader.setFloat("material.shininess", 32.0f);

        //send the number of point lights
        phongShader.setInt("numberOfPointLights", lightingInformation.numberOfPointLights);


        Shader gouraudShader("src/multipleLightsGouraud.vs", "src/multipleLightsGouraud.fs");
        //set gouraud shader material properties
        gouraudShader.use();
        gouraudShader.setInt("material.diffuse", 0);
        gouraudShader.setInt("material.specular", 1);
        gouraudShader.setFloat("material.shininess", 32.0f);

        //send the number of point lights
        gouraudShader.setInt("numberOfPointLights", lightingInformation.numberOfPointLights);
        //send the color of the cube
        gouraudShader.setVec3("objectColor", glm::vec3(1.0, 0.5, 0.31));


        Shader phongTexShader("src/multipleLightsPhongTex.vs", "src/multipleLightsPhongTex.fs");
        //setup the multiple light model using textures
        phongTexShader.use();
        phongTexShader.setFloat("shininess", 32.0f);
        //send the number of point lights
        phongTexShader.setInt("numberOfPointLights", lightingInformation.numberOfPointLights);


        Shader gouraudTexShader("src/multipleLightsGouraudTex.vs", "src/multipleLightsGouraudTex.fs");
        //setup the multiple light model using textures
        gouraudTexShader.use();
        gouraudTexShader.setFloat("shininess", 32.0f);
        //send the number of point lights
        gouraudTexShader.setInt("numberOfPointLights", lightingInformation.numberOfPointLights);

        Shader lampShader("src/lamp.vs", "src/lamp.fs");




        //-----------//
        //RENDER LOOP//
        //-----------//

        // render loop
        while(!glfwWindowShouldClose(mWindow)){
            // per-frame time logic
            currentFrame = glfwGetTime();
            mDeltaTime = currentFrame - mLastFrame;
            mLastFrame = currentFrame;

            // input
            updateInput(mWindow);

            // render
            glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            //use the phong shader
            if(mPhong){


                //----------//
                //DRAW MODEL//
                //----------//

                //enable shader
                phongTexShader.use();
                //use perspective projection
                projection = utils::perspectiveMatrix(0.f, 1.f, 0.f, 1.f, 5.f, -5.f);
                // view/projection transformations
                phongTexShader.setMat4("projection", projection.getMatrix());
                view = camera.GetViewMatrix();
                phongTexShader.setMat4("view", view.getMatrix());

                i = 0;
                // render the loaded models
                for(auto model : models){

                    phongTexShader.setVec3("viewPos", camera.Position);

                    //send the point lights information to the shader for each point light
                    for(int i = 0; i < lightingInformation.numberOfPointLights; i++){
                        //get the point light
                        PointLight* currentPointLight = &lightingInformation.pointLights[i];
                        //set the parameters of the shader
                        phongTexShader.setVec3(std::string("pointLights[") + std::to_string(i) +
                            std::string("].position"), currentPointLight->position);
                        phongTexShader.setFloat(std::string("pointLights[") + std::to_string(i) +
                            std::string("].constant"), currentPointLight->constant);
                        phongTexShader.setFloat(std::string("pointLights[") + std::to_string(i) +
                            std::string("].linear"), currentPointLight->linear);
                        phongTexShader.setFloat(std::string("pointLights[") + std::to_string(i) +
                            std::string("].quadratic"), currentPointLight->quadratic);
                        phongTexShader.setVec3(std::string("pointLights[") + std::to_string(i) +
                            std::string("].ambient"), currentPointLight->ambient);
                        phongTexShader.setVec3(std::string("pointLights[") + std::to_string(i) +
                            std::string("].diffuse"), currentPointLight->diffuse);
                        phongTexShader.setVec3(std::string("pointLights[") + std::to_string(i) +
                            std::string("].specular"), currentPointLight->specular);
                    }

                    //translate the current model to the side of the previous model
                    ml::matrix<float> modelMatrix(4, 4, true);
                    //tmp is the array used to translate the model using its assigned delta value
                    float tmp[3] = {delta[i], 0, 0};
                    modelMatrix = utils::translate(modelMatrix, tmp);

                    // apply rotation
                    modelMatrix = utils::rotateX(modelMatrix, modelCoordVector[i].rotation[0]);
                    modelMatrix = utils::rotateY(modelMatrix, modelCoordVector[i].rotation[1]);
                    modelMatrix = utils::rotateZ(modelMatrix, modelCoordVector[i].rotation[2]);

                    // apply scale
                    modelMatrix = utils::scale(modelMatrix, modelCoordVector[i].scale);

                    // apply translation to the origin
                    modelMatrix = utils::translate(modelMatrix, modelCoordVector[i].position);


                    //transpose the matrix
                    modelMatrix = modelMatrix.transpose();

                    //pass the model matrix to the shader
                    phongTexShader.setMat4("model", modelMatrix.getMatrix());
                    model.Draw(phongTexShader);

                    i++;
                }




                //---------//
                //DRAW CUBE//
                //---------//

                // be sure to activate shader when setting uniforms/drawing objects
                phongShader.use();
                phongShader.setVec3("viewPos", camera.Position);

                //send the point lights information to the shader for each point light
                for(int i = 0; i < lightingInformation.numberOfPointLights; i++){
                    //get the point light
                    PointLight* currentPointLight = &lightingInformation.pointLights[i];
                    //set the parameters of the shader
                    phongShader.setVec3(std::string("pointLights[") + std::to_string(i) +
                                        std::string("].position"), currentPointLight->position);
                    phongShader.setFloat(std::string("pointLights[") + std::to_string(i) +
                                        std::string("].constant"), currentPointLight->constant);
                    phongShader.setFloat(std::string("pointLights[") + std::to_string(i) +
                                        std::string("].linear"), currentPointLight->linear);
                    phongShader.setFloat(std::string("pointLights[") + std::to_string(i) +
                                        std::string("].quadratic"), currentPointLight->quadratic);
                    phongShader.setVec3(std::string("pointLights[") + std::to_string(i) +
                                        std::string("].ambient"), currentPointLight->ambient);
                    phongShader.setVec3(std::string("pointLights[") + std::to_string(i) +
                                        std::string("].diffuse"), currentPointLight->diffuse);
                    phongShader.setVec3(std::string("pointLights[") + std::to_string(i) +
                                        std::string("].specular"), currentPointLight->specular);
                }

                //set tranformation matrices
                modelMatrix = modelMatrix.transpose();
                phongShader.setMat4("model", modelMatrix.getMatrix());
                view = camera.GetViewMatrix();
                phongShader.setMat4("view", view.getMatrix());
                projection = utils::perspectiveMatrix(0.f, 1.f, 0.f, 1.f, 5.f, -5.f);
                phongShader.setMat4("projection", projection.getMatrix());

                // render the cube
                glBindVertexArray(cubeVAO);
                glDrawArrays(GL_TRIANGLES, 0, 36);


            //use the gouraud shader
            }else{


                //----------//
                //DRAW MODEL//
                //----------//

                //enable shader
                gouraudTexShader.use();
                //use perspective projection
                projection = utils::perspectiveMatrix(0.f, 1.f, 0.f, 1.f, 5.f, -5.f);
                // view/projection transformations
                gouraudTexShader.setMat4("projection", projection.getMatrix());
                view = camera.GetViewMatrix();
                gouraudTexShader.setMat4("view", view.getMatrix());

                i = 0;
                // render the loaded models
                for(auto model : models){

                    gouraudTexShader.setVec3("viewPos", camera.Position);

                    //send the point lights information to the shader for each point light
                    for(int i = 0; i < lightingInformation.numberOfPointLights; i++){
                        //get the point light
                        PointLight* currentPointLight = &lightingInformation.pointLights[i];
                        //set the parameters of the shader
                        gouraudTexShader.setVec3(std::string("pointLights[") + std::to_string(i) +
                            std::string("].position"), currentPointLight->position);
                        gouraudTexShader.setFloat(std::string("pointLights[") + std::to_string(i) +
                            std::string("].constant"), currentPointLight->constant);
                        gouraudTexShader.setFloat(std::string("pointLights[") + std::to_string(i) +
                            std::string("].linear"), currentPointLight->linear);
                        gouraudTexShader.setFloat(std::string("pointLights[") + std::to_string(i) +
                            std::string("].quadratic"), currentPointLight->quadratic);
                        gouraudTexShader.setVec3(std::string("pointLights[") + std::to_string(i) +
                            std::string("].ambient"), currentPointLight->ambient);
                        gouraudTexShader.setVec3(std::string("pointLights[") + std::to_string(i) +
                            std::string("].diffuse"), currentPointLight->diffuse);
                        gouraudTexShader.setVec3(std::string("pointLights[") + std::to_string(i) +
                            std::string("].specular"), currentPointLight->specular);
                    }

                    //translate the current model to the side of the previous model
                    ml::matrix<float> modelMatrix(4, 4, true);
                    //tmp is the array used to translate the model using its assigned delta value
                    float tmp[3] = {delta[i], 0, 0};
                    modelMatrix = utils::translate(modelMatrix, tmp);

                    // apply rotation
                    modelMatrix = utils::rotateX(modelMatrix, modelCoordVector[i].rotation[0]);
                    modelMatrix = utils::rotateY(modelMatrix, modelCoordVector[i].rotation[1]);
                    modelMatrix = utils::rotateZ(modelMatrix, modelCoordVector[i].rotation[2]);

                    // apply scale
                    modelMatrix = utils::scale(modelMatrix, modelCoordVector[i].scale);

                    // apply translation to the origin
                    modelMatrix = utils::translate(modelMatrix, modelCoordVector[i].position);


                    //transpose the matrix
                    modelMatrix = modelMatrix.transpose();

                    //pass the model matrix to the shader
                    gouraudTexShader.setMat4("model", modelMatrix.getMatrix());
                    model.Draw(gouraudTexShader);

                    i++;
                }




                //---------//
                //DRAW CUBE//
                //---------//

                // be sure to activate shader when setting uniforms/drawing objects
                gouraudShader.use();
                gouraudShader.setVec3("viewPos", camera.Position);

                //send the point lights information to the shader for each point light
                for(int i = 0; i < lightingInformation.numberOfPointLights; i++){
                    //get the point light
                    PointLight* currentPointLight = &lightingInformation.pointLights[i];
                    //set the parameters of the shader
                    gouraudShader.setVec3(std::string("pointLights[") + std::to_string(i) +
                                        std::string("].position"), currentPointLight->position);
                    gouraudShader.setFloat(std::string("pointLights[") + std::to_string(i) +
                                        std::string("].constant"), currentPointLight->constant);
                    gouraudShader.setFloat(std::string("pointLights[") + std::to_string(i) +
                                        std::string("].linear"), currentPointLight->linear);
                    gouraudShader.setFloat(std::string("pointLights[") + std::to_string(i) +
                                        std::string("].quadratic"), currentPointLight->quadratic);
                    gouraudShader.setVec3(std::string("pointLights[") + std::to_string(i) +
                                        std::string("].ambient"), currentPointLight->ambient);
                    gouraudShader.setVec3(std::string("pointLights[") + std::to_string(i) +
                                        std::string("].diffuse"), currentPointLight->diffuse);
                    gouraudShader.setVec3(std::string("pointLights[") + std::to_string(i) +
                                        std::string("].specular"), currentPointLight->specular);
                }

                // be sure to activate shader when setting uniforms/drawing objects
                gouraudShader.use();
                gouraudShader.setVec3("objectColor", 1.0f, 0.5f, 0.31f);

                // view/projection transformations
                modelMatrix = modelMatrix.transpose();
                gouraudShader.setMat4("model", modelMatrix.getMatrix());
                view = camera.GetViewMatrix();
                gouraudShader.setMat4("view", view.getMatrix());
                projection = utils::perspectiveMatrix(0.f, 1.f, 0.f, 1.f, 5.f, -5.f);
                gouraudShader.setMat4("projection", projection.getMatrix());

                // render the cube
                glBindVertexArray(cubeVAO);
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }




            //-----------------//
            //DRAW POINT LIGHTS//
            //-----------------//


            lampShader.use();
            // view/projection transformations
            lampShader.setMat4("projection", projection.getMatrix());
            lampShader.setMat4("view", view.getMatrix());
            lampShader.setMat4("model", modelMatrix.getMatrix());

            //draw the pointLight
            glBindVertexArray(pointLightsVAO);
            glDrawArrays(GL_POINTS, 0, lightingInformation.numberOfPointLights);




            // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
            glfwSwapBuffers(mWindow);
            glfwPollEvents();
        }

    }

    // set up vertex data (and buffer(s)) and configure vertex attributes from the cube
    unsigned int Window::loadCubeVAO(){

        // ------------------------------------------------------------------
        // configure the cube's VAO (and VBO)
        unsigned int VBO, cubeVAO;
        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &VBO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);

        glBindVertexArray(cubeVAO);

        // position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        // normal attribute
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        return cubeVAO;
    }

    //gen and setup a VAO to the point lights and fill it's buffer
    unsigned int Window::loadPointLightsVAO(){

        //generate the VAO
        unsigned int pointLightsVBO, pointLightsVAO;
        glGenVertexArrays(1, &pointLightsVAO);

        //fill the buffer with the light points
        glGenBuffers(1, &pointLightsVBO);
        glBindBuffer(GL_ARRAY_BUFFER, pointLightsVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(PointLightForBuffer) * lightingInformation.numberOfPointLights,
                     lightingInformation.bufferOfPointLights, GL_STATIC_DRAW);

        //setup the VAO attributes
        glBindVertexArray(pointLightsVAO);
        // position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(PointLightForBuffer), (void*)0);
        glEnableVertexAttribArray(0);
        // color attribute
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(PointLightForBuffer), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        return pointLightsVAO;
    }

    //callback function to execute when the window is resized
    void Window::framebufferResizeCallback(GLFWwindow* window, int fbWidth, int fbHeight){
        glViewport(0, 0, fbWidth, fbHeight);
    }


    //update the user input
    void Window::updateInput(GLFWwindow *window){
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS){
            glfwSetWindowShouldClose(window, true);
        }

        bool shiftIsPressed = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;

        if(shiftIsPressed){
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS){
                camera.ProcessKeyboard(UP, mDeltaTime);
            }
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS){
                camera.ProcessKeyboard(DOWN, mDeltaTime);
            }
        }else{
            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS){
                camera.ProcessKeyboard(FORWARD, mDeltaTime);
            }
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS){
                camera.ProcessKeyboard(BACKWARD, mDeltaTime);
            }
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS){
            camera.ProcessKeyboard(LEFT, mDeltaTime);
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS){
            camera.ProcessKeyboard(RIGHT, mDeltaTime);
        }

        if(mPReleased){
            if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS){
                mPhong = !mPhong;
            }
            mPReleased = false;
        }
        if (glfwGetKey(window, GLFW_KEY_L) == GLFW_RELEASE){
            mPReleased = true;
        }

    }

    // glfw: whenever the mouse moves, this callback is called
    void mouseCallback(GLFWwindow* window, double xpos, double ypos)
    {
        if (firstMouse)
        {
            lastX = xpos;
            lastY = ypos;
            firstMouse = false;
        }

        float xoffset = xpos - lastX;
        // yoffset reversed since y-coordinates go from bottom to top
        float yoffset = lastY - ypos; 

        lastX = xpos;
        lastY = ypos;

        camera.ProcessMouseMovement(xoffset, yoffset);
    }

    void Window::glfwErrorCallback(int error, const char* description) {
        std::cerr << "GLFW error code " << error << ". Description: " << description << std::endl;
    }

}
