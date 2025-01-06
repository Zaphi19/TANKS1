#include <GL/glew.h>
#include <glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <common/shader.hpp>
#include <map>


//Global Variables
const unsigned int WIDTH = 2560, HEIGHT = 1440;     //Screen Size
float platformSize = 120.0f;                        //Size of plattform 120x120
float maxHeight = 5.0f;
float cameraAngle = 0.0f;                           
float rise = 10.0f;                                 //Determines how far the walls will rise in the pregame method
bool isPlayerAlive = true;                          //used to end the game if player is hit
int level = 1;                                      //determines current level is as of now useless (only one level)
float cameraYaw = 0.0f;  
glm::vec3 cameraOffset(0.0f, 1.5f, 0.0f);           //used to position the camera relative to the player

//for the Shader
struct SpotLight {
    glm::vec3 position;
    glm::vec3 direction;
    float innerCone;
    float outerCone;
    glm::vec3 diffuse;
    glm::vec3 specular;
    glm::vec3 ambient;
};

//Used for players and enemies
struct cube {
    float x, z;
    float rotation;
    float speed;
    float lastShotTime = 0.0f;
    float shootCooldown = 5.0f;
};

//for the walls
struct wall {
    float x, z, h;
};

//for bullets
struct bullet {
    float x, z;
    float rotation;
    int bounce;                                     //all bullets can collide with one wall and bounce instead of being destroyed
    bool enemy;                                     //so the shooter of the bullet doesnt instantly collide with his own bullet
    std::vector<std::pair<float, float>> collidedWalls;     //to stop double collisions with the same wall
};

GLuint VAO[2], VBO[2], EBO[2];
GLuint shaderProgram;
GLuint rectVAO, rectVBO, rectEBO;
GLuint textVAO, textVBO;

//reads playerinput
void processInput(GLFWwindow* window, cube& player, std::vector<bullet>& bullets, float currentTime) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)       //rotates playercube to the left
        player.rotation -= 1.0f;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)       //rotates playercube to the right
        player.rotation += 1.0f;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {     //move forward
        player.x += player.speed * sin(glm::radians(player.rotation));//calculates how the player has to move forward
        player.z -= player.speed * cos(glm::radians(player.rotation));
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {     //move backwards
        player.x -= player.speed * sin(glm::radians(player.rotation));//calculates how the player has to move backwards
        player.z += player.speed * cos(glm::radians(player.rotation));
    }

    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {     //turns view to the left
        cameraYaw -= 1.0f;  
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {     //turns view to the right
        cameraYaw += 1.0f; 
    }

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && (currentTime - player.lastShotTime) > player.shootCooldown) {   //shoot bullet
        float shootAngle = player.rotation + cameraYaw;                         //calculates at which angle the bullet should be shot
        bullets.push_back({ player.x, player.z, shootAngle, 1, false });            
        player.lastShotTime = currentTime;
    }

}

GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "Shader Compilation Error:\n" << infoLog << std::endl;
    }

    return shader;
}

void setupPlatformAndCube() {

    float platform = platformSize / 2.0f;

    float platformVertices[] = {
        -platform, 0.0f, -platform,
         platform, 0.0f, -platform,
         platform, 0.0f,  platform,
        -platform, 0.0f,  platform,
    };
    unsigned int platformIndices[] = {
        0, 1, 2, 2, 3, 0
    };

    float cubeVertices[] = {
        -0.5f, -0.5f, -0.5f,  0.5f, -0.5f, -0.5f,  0.5f,  0.5f, -0.5f, -0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f,  0.5f,  0.5f, -0.5f,  0.5f,  0.5f
    };
    unsigned int cubeIndices[] = {
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4,
        0, 4, 7, 7, 3, 0,
        1, 5, 6, 6, 2, 1,
        3, 2, 6, 6, 7, 3,
        0, 1, 5, 5, 4, 0
    };

    glGenVertexArrays(2, VAO);
    glGenBuffers(2, VBO);
    glGenBuffers(2, EBO);

    glBindVertexArray(VAO[0]);
    glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(platformVertices), platformVertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO[0]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(platformIndices), platformIndices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(VAO[1]);
    glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO[1]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIndices), cubeIndices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
}

//Renders game
void renderScene(cube player, const std::vector<wall>& walls, std::vector<bullet>& bullets, std::vector<cube> enemies) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(shaderProgram);

    glm::vec3 playerPosition(player.x, 0.5f, player.z);

    //player view
    glm::vec3 cameraPosition = playerPosition + cameraOffset;
    glm::vec3 direction(
        sin(glm::radians(player.rotation + cameraYaw)),  
        0.0f,  
        -cos(glm::radians(player.rotation + cameraYaw)) 
    );

    glm::mat4 view = glm::lookAt(
        cameraPosition,
        cameraPosition + direction,
        glm::vec3(0.0f, 1.0f, 0.0f)  
    );

    glm::mat4 projection = glm::perspective(glm::radians(70.0f), (float)WIDTH / (float)HEIGHT, 0.1f, 100.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));


    //spotlight
    glm::vec3 lightPos = glm::vec3(player.x, 5.0f, player.z);
    glm::vec3 viewPos(40.0f * sin(glm::radians(cameraAngle)), 40.0f, 40.0f * cos(glm::radians(cameraAngle)));
    glUniform3fv(glGetUniformLocation(shaderProgram, "lightPos"), 1, glm::value_ptr(lightPos));
    glUniform3fv(glGetUniformLocation(shaderProgram, "viewPos"), 1, glm::value_ptr(viewPos));
    glUniform1f(glGetUniformLocation(shaderProgram, "ambientStrength"), 0.2f);
    glUniform1f(glGetUniformLocation(shaderProgram, "specularStrength"), 0.5f);
    glUniform1f(glGetUniformLocation(shaderProgram, "shininess"), 10.0f);
    glUniform1f(glGetUniformLocation(shaderProgram, "lightIntensity"), 1.0f);

    SpotLight spotLight;
    spotLight.position = glm::vec3(player.x, 0.0f, player.z);  
    float rad = glm::radians(player.rotation + cameraYaw);    

    spotLight.direction = glm::normalize(glm::vec3(sin(-rad), 0.0f, cos(rad)));
    spotLight.innerCone = cos(glm::radians(12.5f));
    spotLight.outerCone = cos(glm::radians(30.5f));
    spotLight.diffuse = glm::vec3(1.0f, 1.0f, 1.0f);
    spotLight.specular = glm::vec3(1.0f, 1.0f, 1.0f);
    spotLight.ambient = glm::vec3(0.2f, 0.2f, 0.2f);

    glUniform3fv(glGetUniformLocation(shaderProgram, "u_spot_light.position"), 1, glm::value_ptr(spotLight.position));
    glUniform3fv(glGetUniformLocation(shaderProgram, "u_spot_light.direction"), 1, glm::value_ptr(spotLight.direction));
    glUniform1f(glGetUniformLocation(shaderProgram, "u_spot_light.innerCone"), spotLight.innerCone);
    glUniform1f(glGetUniformLocation(shaderProgram, "u_spot_light.outerCone"), spotLight.outerCone);
    glUniform3fv(glGetUniformLocation(shaderProgram, "u_spot_light.diffuse"), 1, glm::value_ptr(spotLight.diffuse));
    glUniform3fv(glGetUniformLocation(shaderProgram, "u_spot_light.specular"), 1, glm::value_ptr(spotLight.specular));
    glUniform3fv(glGetUniformLocation(shaderProgram, "u_spot_light.ambient"), 1, glm::value_ptr(spotLight.ambient));

    //platform
    glBindVertexArray(VAO[0]);
    glm::mat4 model = glm::mat4(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform4f(glGetUniformLocation(shaderProgram, "objectColor"), 0.24f, 0.39f, 0.18f, 1.0f);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    //player
    glBindVertexArray(VAO[1]);
    model = glm::translate(glm::mat4(1.0f), glm::vec3(player.x, 0.5f, player.z));
    model = glm::rotate(model, glm::radians(-player.rotation), glm::vec3(0.0f, 1.0f, 0.0f));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform4f(glGetUniformLocation(shaderProgram, "objectColor"), 0.1f, 0.4f, 0.8f, 1.0f);
    glUniform1f(glGetUniformLocation(shaderProgram, "playerDistance"), glm::length(playerPosition));
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

    //enemies
    if (!enemies.empty()) {
        auto enemyIt = enemies.begin();
        while (enemyIt != enemies.end()) {
            model = glm::translate(glm::mat4(1.0f), glm::vec3(enemyIt->x, 0.5f, enemyIt->z));
            model = glm::rotate(model, glm::radians(-enemyIt->rotation), glm::vec3(0.0f, 1.0f, 0.0f));
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
            glUniform4f(glGetUniformLocation(shaderProgram, "objectColor"), 0.412f, 0.412f, 0.412f, 1.0f);
            glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
            ++enemyIt;
        }
    }

    //walls
    for (const wall& w : walls) {
        glm::vec3 wallPosition(w.x, w.h / 2.0f, w.z);
        model = glm::translate(glm::mat4(1.0f), wallPosition);
        model = glm::scale(model, glm::vec3(1.0f, w.h, 1.0f));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniform4f(glGetUniformLocation(shaderProgram, "objectColor"), 0.54f, 0.27f, 0.07f, 1.0f);
        glUniform1f(glGetUniformLocation(shaderProgram, "playerDistance"), glm::length(playerPosition - wallPosition));
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    }

    //bullets
    for (const bullet& b : bullets) {
        glm::vec3 bulletPosition(b.x, 0.5f, b.z);
        model = glm::translate(glm::mat4(1.0f), bulletPosition);
        model = glm::scale(model, glm::vec3(0.2f));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniform4f(glGetUniformLocation(shaderProgram, "objectColor"), 1.0f, 0.2f, 0.2f, 1.0f); 
        glUniform1f(glGetUniformLocation(shaderProgram, "playerDistance"), glm::length(playerPosition - bulletPosition));
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    }
}

//---------------------------------------------------Collsision Methods---------------------------------------------------//
void updateBullets(std::vector<bullet>& bullets, const std::vector<wall>& walls, float platformSize, std::vector<cube>& enemies, cube player, float bulletSpeed) {
    float border = platformSize / 2.0f;
    float wallSize = 0.5f;
    float bulletSize = 0.2f;
    float cSize = 0.5f;

    // Used to track what should be deleted. Directly deleting enemies/bullets caused issues.
    std::vector<int> enemiesToRemove;
    std::vector<int> bulletsToRemove;

    for (int i = 0; i < bullets.size(); ++i) {
        auto& b = bullets[i];
        b.x += bulletSpeed * sin(glm::radians(b.rotation));     //calculates x and z speed
        b.z -= bulletSpeed * cos(glm::radians(b.rotation));

        bool bounced = false;

        //Checks collision with platform borders
        if (b.x <= -border || b.x >= border) {                 
            b.rotation = -b.rotation;
            bounced = true;
        }
        if (b.z <= -border || b.z >= border) {
            b.rotation = 180.0f - b.rotation;
            bounced = true;
        }

        //checks for collisions with walls
        for (const wall& w : walls) {
            bool collisionX = b.x > w.x - wallSize && b.x < w.x + wallSize;
            bool collisionZ = b.z > w.z - wallSize && b.z < w.z + wallSize;

            if (collisionX && collisionZ) {             //checks if the bullet collided with a wall and flips the speed accordingly
                float overlapX = std::abs(b.x - w.x);
                float overlapZ = std::abs(b.z - w.z);
                if (overlapX > overlapZ) {
                    b.rotation = -b.rotation;
                }
                else {
                    b.rotation = 180.0f - b.rotation;
                }
                bounced = true;
                break;
            }
        }

        //checks player bullet collision
        if (!b.enemy) {
            for (int j = 0; j < enemies.size(); ++j) {
                float distanceX = b.x - enemies[j].x;
                float distanceZ = b.z - enemies[j].z;
                float distance = sqrt(distanceX * distanceX + distanceZ * distanceZ);

                if (distance < (bulletSize + cSize)) {
                    enemiesToRemove.push_back(j);  
                    bulletsToRemove.push_back(i);  
                    break;
                }
            }
        }

        //checks enemy bullet collision
        if (b.enemy) {
            float distanceToPlayerX = b.x - player.x;
            float distanceToPlayerZ = b.z - player.z;
            float distanceToPlayer = sqrt(distanceToPlayerX * distanceToPlayerX + distanceToPlayerZ * distanceToPlayerZ);

            if (distanceToPlayer < (bulletSize + 0.5f)) {
                isPlayerAlive = false;
                bulletsToRemove.push_back(i);  
            }
        }

        if (bounced) {
            if (b.bounce > 0) {
                b.bounce--;
            }
            else {
                bulletsToRemove.push_back(i);  
            }
        }
    }

    //deletes the bullets and enemies
    for (int i = bulletsToRemove.size() - 1; i >= 0; --i) {
        bullets.erase(bullets.begin() + bulletsToRemove[i]);
    }


    for (int i = enemiesToRemove.size() - 1; i >= 0; --i) {
        enemies.erase(enemies.begin() + enemiesToRemove[i]);
    }
}

void checkPlayerCollision(cube& player, const std::vector<wall>& walls) {
    float border = platformSize / 2.0f;     //is halved because the border is from -60 to 60 and not from 0 to 120
    float playerSize = 0.5f;
    float wallSize = 0.5f;

    glm::mat4 model = glm::mat4(1.0f);

    if (player.x <= -border + 0.5f) player.x = -border + 0.5f;

    if (player.x >= border - 0.5f)  player.x = border - 0.5f;



    if (player.z <= -border + 0.5f) player.z = -border + 0.5f;

    if (player.z >= border - 0.5f)  player.z = border - 0.5f;

    //checks player wall collision
    for (const wall& w : walls) {
        bool collisionX = player.x + playerSize > w.x - wallSize && player.x - playerSize < w.x + wallSize;
        bool collisionZ = player.z + playerSize > w.z - wallSize && player.z - playerSize < w.z + wallSize;

        if (collisionX && collisionZ) {
            
            float overlapX = (playerSize + wallSize) - std::abs(player.x - w.x);
            float overlapZ = (playerSize + wallSize) - std::abs(player.z - w.z);

            if (overlapX < overlapZ) {
  
                if (player.x < w.x) {
                    player.x -= overlapX; 
                }
                else {
                    player.x += overlapX; 
                }
            }
            else {

                if (player.z < w.z) {
                    player.z -= overlapZ; 
                }
                else {
                    player.z += overlapZ; 
                }
            }
        }
    }
}

//---------------------------------------------------Methods to build structures---------------------------------------------------//
bool hasLineOfSight(cube enemy, cube player, const std::vector<wall>& walls) {
    glm::vec2 enemyPos(enemy.x, enemy.z);
    glm::vec2 playerPos(player.x, player.z);
    glm::vec2 direction = glm::normalize(playerPos - enemyPos);
    float distance = glm::length(playerPos - enemyPos);

    float stepSize = 0.5f;
    glm::vec2 currentPos = enemyPos;
    //steps from enemy to the player to check if there is a wall between them
    while (glm::length(currentPos - enemyPos) < distance) {
        currentPos += direction * stepSize;
        for (const wall& w : walls) {
            glm::vec2 wallPos(w.x, w.z);
            float wallRadius = 0.5f;

            if (glm::length(currentPos - wallPos) < wallRadius) {
                return false;
            }
        }
    }
    return true;
}

void enemyShootAtPlayer(std::vector<cube>& enemies, cube player, std::vector<bullet>& bullets, const std::vector<wall>& walls, float currentTime) {
    for (cube& enemy : enemies) {
#
        //rotates enemy towards player
        float deltaX = player.x - enemy.x;
        float deltaZ = player.z - enemy.z;

        float angle = atan2(deltaX, -deltaZ);
        enemy.rotation = glm::degrees(angle);

        if (hasLineOfSight(enemy, player, walls) && (currentTime - enemy.lastShotTime) > enemy.shootCooldown) {         //if the enemy has line of sight it shoots a bullet towards the players current location
            bullets.push_back({ enemy.x, enemy.z, enemy.rotation, 1, true });
            enemy.lastShotTime = currentTime;       //for checking if the last shot was at least 5 seconds ago (reload time)
        }
    }
}

//---------------------------------------------------Methods to build structures---------------------------------------------------//
void createCircularRoom(std::vector<wall>& levelPreset, int centerX, int centerZ, int radius, float maxHeight) {
    //creates a circular room with holes on 4 sides to enter
    for (int x = -radius; x <= radius; x++) {
        for (int z = -radius; z <= radius; z++) {
            if ((x * x + z * z <= radius * radius) &&
                (x * x + z * z > (radius - 1) * (radius - 1)) &&  
                !((z == radius || z == -radius) && x >= -1 && x <= 1) &&  
                !((x == radius || x == -radius) && z >= -1 && z <= 1)) {  
                wall w = { centerX + x, centerZ + z, maxHeight };
                w.h = (std::rand() % 4 + 4);
                levelPreset.push_back(w);
            }
        }
    }
}

void createSquareRoom(std::vector<wall>& levelPreset, int centerX, int centerZ, int roomSize, float maxHeight) {
    //creates a square room with 4 entrances
    for (int x = -roomSize / 2; x <= roomSize / 2; x++) {
        for (int z = -roomSize / 2; z <= roomSize / 2; z++) {
            if ((x == -roomSize / 2 || x == roomSize / 2 || z == -roomSize / 2 || z == roomSize / 2) &&
                !((x >= -1 && x <= 1 && (z == -roomSize / 2 || z == roomSize / 2)) ||
                    (z >= -1 && z <= 1 && (x == -roomSize / 2 || x == roomSize / 2)))) {
                wall w = { centerX + x, centerZ + z, maxHeight };
                w.h = (std::rand() % 4 + 4);
                levelPreset.push_back(w);
            }
        }
    }
}

void createCross(std::vector<wall>& levelPreset, int centerX, int centerZ, int armLength, float maxHeight) {
    //creates a cross with a thickness of on and a designated arm length
    for (int x = 0; x <= 0; x++) {
        for (int z = -armLength; z <= armLength; z++) {
            wall w = { centerX + x, centerZ + z, maxHeight };
            w.h = (std::rand() % 4 + 4);
            levelPreset.push_back(w);
        }
    }

    for (int z = 0; z <= 0; z++) {
        for (int x = -armLength; x <= armLength; x++) {
            if (true) {
                wall w = { centerX + x, centerZ + z, maxHeight };
                w.h = (std::rand() % 4 + 4);
                levelPreset.push_back(w);
            }
        }
    }
}

//---------------------------------------------------Methods that use the build methods to place structures and enemies---------------------------------------------------//
std::vector<wall> generatelevel(float maxHeight, int level) {
    std::vector<wall> levelPreset;
    levelPreset.clear();

    //the first level 
    if (level == 1) {
        //vectors that contain the x,z and size of the rooms {x, z, size}
        std::vector<std::tuple<int, int, int>> squareRooms = {      
            {-50, -50, 20}, {50, -50, 20}, {-50, 50, 20}, {50, 50, 20}, { 35, -15, 15}, { -20, 25, 12 }, { -40, -10, 18 },  { 30, 33, 8 }
        };
        std::vector<std::tuple<int, int, int>> roundRooms = {
            {0, 0, 20}
        };
        std::vector<std::tuple<int, int, int>> cross = {
            {0, 0, 10}
        };
        int roomSize = 20;
        int crossLength = 10;

        //creates the rooms
        for (auto& pos : squareRooms) {
            createSquareRoom(levelPreset, std::get<0>(pos), std::get<1>(pos), std::get<2>(pos), maxHeight);
        }

        for (auto& pos : roundRooms) {
            createCircularRoom(levelPreset, std::get<0>(pos), std::get<1>(pos), std::get<2>(pos), maxHeight);
        }
        for (auto& pos : cross) {
            createCross(levelPreset, std::get<0>(pos), std::get<1>(pos), std::get<2>(pos), maxHeight);
        }

        int smallRoomSize = 20;
        int roomCenterX = 0;
        int roomCenterZ = -50;
        //player spawn room
        for (int x = -smallRoomSize / 2; x <= smallRoomSize / 2; x++) {
            for (int z = -smallRoomSize / 2; z <= smallRoomSize / 2; z++) {
                if ((x == -smallRoomSize / 2 || x == smallRoomSize / 2 || z == -smallRoomSize / 2 || z == smallRoomSize / 2) &&
                    !(x >= -1 && x <= 1 && z == smallRoomSize / 2)) { 
                    wall w = { roomCenterX + x, roomCenterZ + z, maxHeight };
                    levelPreset.push_back(w);
                }
            }
        }
    }
    //future levels would be added here
    return levelPreset;
}

std::vector<cube> distributeEnemies(int level) {
    std::vector<cube> enemies;
    enemies.clear();
    if (level == 1) {
        std::vector<std::pair<float, float>> roomCenters = {        //contains the room coordinates
            {-50, -50}, {50, -50}, {-50, 50}, {50, 50}, { 35, -15}, { -20, 25}, { -40, -10},  { 30, 33}  
        };
        //randomly places one or two enemies in each room
        for (auto& center : roomCenters) {
            int enemyCount = (std::rand() % 2) + 1;  
            for (int i = 0; i < enemyCount; i++) {
                float x = center.first + static_cast<float>((std::rand() % 8) - 4);
                float z = center.second + static_cast<float>((std::rand() % 8) - 4);
                cube enemy = { x, z, 0.0f, 0.05f, 0.0f };
                enemies.push_back(enemy);
            }
        }

        //enemies in the circular room are placed here
        cube enemy = { 14, 12, 0.0f, 0.05f, 0.0f };
        enemies.push_back(enemy);
        enemy = { -10,-8, 0.0f, 0.05f, 0.0f };
        enemies.push_back(enemy);
        enemy = { -4,3, 0.0f, 0.05f, 0.0f };
        enemies.push_back(enemy);
    }
    return enemies;
}

std::vector<wall> setupGame(float platformSize, float maxHeight, int level) {
    std::vector<wall> walls;

    walls = generatelevel(maxHeight, 1);

    return walls;
}

//---------------------------------------------------Loops for the pregame render---------------------------------------------------//
void renderPreGameScene(cube player, const std::vector<wall>& walls, std::vector<bullet>& bullets) {
    //almost the same as render scene only difference is no spotlight and no enemies are shown yet
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glUseProgram(shaderProgram);
    glm::vec3 playerPosition(player.x, 0.0f, player.z);
    glm::mat4 view = glm::lookAt(
        glm::vec3(40.0f * sin(glm::radians(cameraAngle)), 40.0f, 40.0f * cos(glm::radians(cameraAngle))),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)WIDTH / (float)HEIGHT, 0.1f, 100.0f);

    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    glUniform1i(glGetUniformLocation(shaderProgram, "isPreGame"), GL_TRUE);

    glm::vec3 ambientLightColor(1.0f, 1.0f, 1.0f);
    float ambientIntensity = 0.3f;

    glUniform3fv(glGetUniformLocation(shaderProgram, "lightPos"), 1, glm::value_ptr(glm::vec3(0.0f, 10.0f, 0.0f)));
    glUniform3fv(glGetUniformLocation(shaderProgram, "viewPos"), 1, glm::value_ptr(glm::vec3(0.0f, 10.0f, 0.0f)));
    glUniform1f(glGetUniformLocation(shaderProgram, "ambientStrength"), ambientIntensity);
    glUniform1f(glGetUniformLocation(shaderProgram, "lightIntensity"), 0.6f);

    //platform
    glBindVertexArray(VAO[0]);
    glm::mat4 model = glm::mat4(1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform4f(glGetUniformLocation(shaderProgram, "objectColor"), 0.24f, 0.39f, 0.18f, 1.0f);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    //player
    glBindVertexArray(VAO[1]);
    model = glm::translate(glm::mat4(1.0f), glm::vec3(player.x, 0.5f, player.z));
    model = glm::rotate(model, glm::radians(-player.rotation), glm::vec3(0.0f, 1.0f, 0.0f));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform4f(glGetUniformLocation(shaderProgram, "objectColor"), 0.1f, 0.4f, 0.8f, 1.0f);
    glUniform1f(glGetUniformLocation(shaderProgram, "playerDistance"), glm::length(playerPosition));
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

    //walls
    for (const wall& w : walls) {
        glm::vec3 wallPosition(w.x, (w.h / 2.0f) - rise, w.z);
        model = glm::translate(glm::mat4(1.0f), wallPosition);
        model = glm::scale(model, glm::vec3(1.0f, w.h, 1.0f));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniform4f(glGetUniformLocation(shaderProgram, "objectColor"), 0.54f, 0.27f, 0.07f, 1.0f);
        glUniform1f(glGetUniformLocation(shaderProgram, "playerDistance"), glm::length(playerPosition - wallPosition));
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    }

    glUniform1i(glGetUniformLocation(shaderProgram, "isPreGame"), GL_FALSE);

}

void gameStart(GLFWwindow* window, cube player, std::vector<bullet>& bullets, std::vector<cube> enemies, const std::vector<wall>& walls) {
    //method that runs during the pre game map showcase (the spinning map overview in the beginning)
    while (!glfwWindowShouldClose(window)) {
        if (cameraAngle < 360.1f) { cameraAngle = cameraAngle + 0.1f; }
        if (rise > 0) { rise = rise - 0.005f; }
        renderPreGameScene(player, walls, bullets);
        glfwSwapBuffers(window);
        glfwPollEvents();
        if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS) {
            break;
        }
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS){ 
            glfwSetWindowShouldClose(window, true); 
        }

    }
}

//---------------------------------------------------Main, Game loop---------------------------------------------------//
void gameloop(GLFWwindow* window, cube player, std::vector<bullet>& bullets, std::vector<cube> enemies, const std::vector<wall>& walls) {
    while (!glfwWindowShouldClose(window) && !enemies.empty()) {
        float currentTime = glfwGetTime();
        processInput(window, player, bullets, currentTime);
        updateBullets(bullets, walls, platformSize, enemies, player, 0.2f);
        renderScene(player, walls, bullets, enemies);
        checkPlayerCollision(player, walls);
        enemyShootAtPlayer(enemies, player, bullets, walls, currentTime);
        glfwSwapBuffers(window);
        glfwPollEvents();
        if (!isPlayerAlive) {
            std::cout << "Player has been defeated!" << std::endl;
            break;
        }
    }
}

int main() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "OpenGL Project", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        return -1;
    }

    shaderProgram = LoadShaders("SimpleVertexShader.vertexshader", "SimpleFragmentShader.fragmentshader");
    glEnable(GL_DEPTH_TEST);

    setupPlatformAndCube();
    cube player = { 0.0f, -50.0f, 180.0f, 0.1f, 0.0f };
    //these methods should be placed in the while loop if there were more levels
    std::vector<cube> enemies = distributeEnemies(1);
    std::vector<wall> walls = setupGame(platformSize, maxHeight, 1);
    std::vector<bullet> bullets;

    while (level == 1 && !glfwWindowShouldClose(window)) {
        isPlayerAlive = true;
        glUniform1i(glGetUniformLocation(shaderProgram, "isPreGame"), GL_TRUE);
        gameStart(window, player, bullets, enemies, walls);

        cameraAngle = 0.0f;
        glUniform1i(glGetUniformLocation(shaderProgram, "isPreGame"), GL_FALSE);
        gameloop(window, player, bullets, enemies, walls);
    }


    glDeleteVertexArrays(2, VAO);
    glDeleteBuffers(2, VBO);
    glDeleteBuffers(2, EBO);
    glfwTerminate();
    return 0;
}