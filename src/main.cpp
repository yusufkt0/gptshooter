#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <iostream>
#include <algorithm>
#include <vector>

struct Entity {
    SDL_Rect rect;
    double xSpeed;
    double ySpeed;
    double enYspeed = -0.4;
};

struct Projectile : Entity {
    bool active;
};

struct Enemy : Entity {
    bool active;
};

bool checkCollision(const Entity& entity, const Entity& other);

void handleInput(Entity& entity, std::vector<Projectile>& projectiles, Uint32& lastProjectileTime) {
    SDL_Event e;
    while (SDL_PollEvent(&e) != 0) {
        if (e.type == SDL_QUIT) {
            SDL_Quit();
            exit(0);
        } else if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
            switch (e.key.keysym.sym) {
                case SDLK_UP:
                    entity.ySpeed = (e.type == SDL_KEYDOWN) ? -0.2 : 0.0;
                    break;
                case SDLK_DOWN:
                    entity.ySpeed = (e.type == SDL_KEYDOWN) ? 0.2 : 0.0;
                    break;
                case SDLK_LEFT:
                    entity.xSpeed = (e.type == SDL_KEYDOWN) ? -0.2 : 0.0;
                    break;
                case SDLK_RIGHT:
                    entity.xSpeed = (e.type == SDL_KEYDOWN) ? 0.2 : 0.0;
                    break;
                case SDLK_SPACE:
                    Uint32 currentTime = SDL_GetTicks();
                    if (currentTime - lastProjectileTime > 1000) {
                        Projectile projectile;
                        projectile.rect.x = entity.rect.x + entity.rect.w / 2 - 5;
                        projectile.rect.y = entity.rect.y;
                        projectile.rect.w = 10;
                        projectile.rect.h = 10;
                        projectile.xSpeed = 0;
                        projectile.ySpeed = -0.2;
                        projectile.active = true;

                        projectiles.push_back(projectile);
                        lastProjectileTime = currentTime;
                    }
                    break;
            }
        }
    }
}

void updateEntity(Entity& entity, Uint32 deltaTime) {
    entity.rect.x += static_cast<int>(entity.xSpeed * deltaTime);
    entity.rect.y += static_cast<int>(entity.ySpeed * deltaTime);
    // Ensure the entity stays within the window boundaries
    entity.rect.x = std::max(0, std::min(800 - entity.rect.w, entity.rect.x));
    entity.rect.y = std::max(0, std::min(600 - entity.rect.h, entity.rect.y));
}

// ...

void updateProjectiles(std::vector<Projectile>& projectiles, Uint32 deltaTime, std::vector<Enemy>& enemies, int& score) {
    projectiles.erase(std::remove_if(projectiles.begin(), projectiles.end(),
                                     [](const Projectile& p) { return !p.active; }),
                      projectiles.end());

    for (auto& projectile : projectiles) {
        projectile.rect.x += static_cast<int>(projectile.xSpeed * deltaTime);
        projectile.rect.y += static_cast<int>(projectile.ySpeed * deltaTime);
        // Deactivate the projectile if it goes out of the window
        if (projectile.rect.y + projectile.rect.h < 0 || projectile.rect.y > 600) {
            projectile.active = false;
        }

        // Check for collisions with enemies
        for (auto& enemy : enemies) {
            if (enemy.active && checkCollision(projectile, enemy)) {
                projectile.active = false;
                enemy.active = false;
                score += 10; // Increase the score when a collision occurs
            }
        }
    }
}

void updateEnemies(std::vector<Enemy>& enemies, Uint32 deltaTime) {
    for (auto& enemy : enemies) {
        enemy.rect.y -= static_cast<int>(enemy.enYspeed * deltaTime);
        // Deactivate the enemy if it goes out of the window
        if (enemy.rect.y > 600) {
            enemy.active = false;
        }
    }
}

// ...


bool checkCollision(const Entity& entity, const Entity& other) {
    return (entity.rect.x < other.rect.x + other.rect.w &&
            entity.rect.x + entity.rect.w > other.rect.x &&
            entity.rect.y < other.rect.y + other.rect.h &&
            entity.rect.y + entity.rect.h > other.rect.y);
}

bool checkGameOver(const Entity& entity, const std::vector<Enemy>& enemies) {
    for (const auto& enemy : enemies) {
        if (enemy.active && checkCollision(entity, enemy)) {
            return true; // Game over if there is a collision with an active enemy
        }
    }
    return false;
}

void render(SDL_Renderer* renderer, const Entity& entity, const std::vector<Projectile>& projectiles, const std::vector<Enemy>& enemies, int score, TTF_Font* font) {
    // Set the background color to dark blue
    SDL_SetRenderDrawColor(renderer, 0, 0, 128, 255); // Dark blue color for the background
    SDL_RenderClear(renderer);

    // Draw the entity (cube)
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red color for the cube
    SDL_RenderFillRect(renderer, &entity.rect);

    // Draw the projectiles
    SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255); // Blue color for the projectiles
    for (const auto& projectile : projectiles) {
        SDL_RenderFillRect(renderer, &projectile.rect);
    }

    // Draw the enemies
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255); // Green color for the enemies
    for (const auto& enemy : enemies) {
        SDL_RenderFillRect(renderer, &enemy.rect);
    }

    // Render the score
    SDL_Color textColor = {255, 255, 255, 255}; // White color for the text
    SDL_Rect scoreRect = {10, 10, 100, 30};
    char scoreText[50];
    snprintf(scoreText, sizeof(scoreText), "Score: %d", score);
    SDL_Surface* surface = TTF_RenderText_Solid(font, scoreText, textColor);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_RenderCopy(renderer, texture, NULL, &scoreRect);
    SDL_FreeSurface(surface);
    SDL_DestroyTexture(texture);

    // Present the renderer
    SDL_RenderPresent(renderer);
}


int main(int argc, char* argv[]) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Create a window
    SDL_Window* window = SDL_CreateWindow("SDL Window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_SHOWN);
    if (window == nullptr) {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Create a renderer
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == nullptr) {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Initialize SDL_ttf
    if (TTF_Init() == -1) {
        std::cerr << "SDL_ttf could not initialize! TTF_Error: " << TTF_GetError() << std::endl;
        return 1;
    }

    // Load a font
    TTF_Font* font = TTF_OpenFont("/home/yasfur/Documents/wsad/proj/Arial.ttf", 24);
    if (font == nullptr) {
        std::cerr << "Failed to load font! TTF_Error: " << TTF_GetError() << std::endl;
        return 1;
    }

    // Entity (Cube) properties
    Entity entity = {{375, 500, 50, 50}, 0, 0};

    // Projectiles
    std::vector<Projectile> projectiles;
    Uint32 lastProjectileTime = 0;

    // Enemies
    std::vector<Enemy> enemies;

    // Score
    int score = 0;

    // Main loop
    bool quit = false;
    Uint32 lastTime = SDL_GetTicks();
    while (!quit) {
        handleInput(entity, projectiles, lastProjectileTime);

        Uint32 currentTime = SDL_GetTicks();
        Uint32 deltaTime = currentTime - lastTime;

        updateEntity(entity, deltaTime);
        updateProjectiles(projectiles, deltaTime, enemies, score);

        // Spawn enemies at the top of the screen
        if (rand() % 100 < 2) {
            Enemy enemy;
            enemy.rect.x = rand() % 750; // Random X position
            enemy.rect.y = 0;
            enemy.rect.w = 50;
            enemy.rect.h = 50;
            enemy.ySpeed = 0.05; // Adjust the speed of the enemies
            enemy.active = true;

            enemies.push_back(enemy);
        }
        

        updateEnemies(enemies, deltaTime);

        // Check for game over
        if (checkGameOver(entity, enemies)) {
            std::cout << "Game Over! Score: " << score << std::endl;
            quit = true;
        }

        render(renderer, entity, projectiles, enemies, score, font);

        lastTime = currentTime;

        // Adjust the delay to control the frame rate
        SDL_Delay(16); // Aim for approximately 60 frames per second
    }
    

    // Cleanup and exit
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_CloseFont(font);
    TTF_Quit();
    SDL_Quit();

    return 0;
}
