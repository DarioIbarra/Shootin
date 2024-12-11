#include <SFML/Graphics.hpp>
#include <iostream>
#include <cmath>
#include <vector>
#include <list>
#include <random>
#include <algorithm>

// Constantes
constexpr float SCREEN_WIDTH = 1200.0f;
constexpr float SCREEN_HEIGHT = 900.0f;
constexpr float M_P = 3.14159265f;

constexpr float PLAYER_W = 50.0f;
constexpr float PLAYER_H = 40.0f;
constexpr float TURN_SPEED = 200.0f;
constexpr float PLAYER_SPEED = 200.0f;
constexpr float SHOOT_DELAY = 0.2f;
constexpr float BULLET_SPEED = 400.0f;
constexpr float BULLET_LIFE = 3.0f;

constexpr float ASTEROID_W = 90.0f;
constexpr float ASTEROID_H = 80.0f;
constexpr float ASTEROID_SPIN = 25.0f;
constexpr float ASTEROID_SPEED = 280.0f;
constexpr float ASTEROID_SPAWN_TIME = 3.0f;

bool checkCollision(const sf::Vector2f& pos1, float radius1, const sf::Vector2f& pos2, float radius2) {
    float dx = pos1.x - pos2.x;
    float dy = pos1.y - pos2.y;
    float distanceSquared = dx * dx + dy * dy;
    float radiusSum = radius1 + radius2;
    return distanceSquared <= (radiusSum * radiusSum);
}


// Clase base para entidades
class Entity {
public:
    Entity(sf::Vector2f position, float angle) : position(position), angle(angle) {}
    virtual void update(float deltaTime) = 0;
    virtual void render(sf::RenderWindow& window) = 0;
    virtual ~Entity() = default;
    sf::Vector2f position;
    float angle;
};

// Vectores globales para manejar entidades
std::vector<Entity*> entities{};
std::list<Entity*> toRemoveList{};
std::list<Entity*> toAddList{};

// Clase Bullet
class Bullet : public Entity {
public:
    Bullet(sf::Vector2f position, sf::Vector2f direction)
        : shape(3.0f), direction(direction), Entity(position, 0.0f), lifetime(BULLET_LIFE), radius(5.0f) {
        shape.setFillColor(sf::Color::White);
    }


    void update(float deltaTime) override {
        lifetime -= deltaTime;
        position += direction * BULLET_SPEED * deltaTime;

        if (lifetime <= 0.0f) {
            toRemoveList.push_back(this);
        }
    }

    void render(sf::RenderWindow& window) override {
        shape.setPosition(position);
        window.draw(shape);
    }

private:
    sf::CircleShape shape;
    sf::Vector2f direction;
    float lifetime;
    float radius;
};

// Clase Player
class Player : public Entity {
public:
    Player()
        : Entity(sf::Vector2f(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2), 0), array(sf::LinesStrip, 5), shootTimer(0) {
        array[0].position = sf::Vector2f(20, 0);
        array[1].position = sf::Vector2f(-30, -20);
        array[2].position = sf::Vector2f(-15, 0);
        array[3].position = sf::Vector2f(-30, 20);
        array[4].position = array[0].position;

        for (size_t i = 0; i < array.getVertexCount(); i++) {
            array[i].color = sf::Color::White;
        }
    }

    void update(float deltaTime) override {
        shootTimer -= deltaTime;

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
            angle -= TURN_SPEED * deltaTime;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
            angle += TURN_SPEED * deltaTime;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) {
            float radians = angle * (M_P / 180.0f);
            position.x += cos(radians) * PLAYER_SPEED * deltaTime;
            position.y += sin(radians) * PLAYER_SPEED * deltaTime;

            position.x = std::min(std::max(position.x, PLAYER_W / 2.0f), SCREEN_WIDTH - PLAYER_W / 2.0f);
            position.y = std::min(std::max(position.y, PLAYER_H / 2.0f), SCREEN_HEIGHT - PLAYER_H / 2.0f);
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space) && shootTimer <= 0.0f) {
            shootTimer = SHOOT_DELAY;
            float radians = angle * (M_P / 180.0f);
            toAddList.push_back(new Bullet(position, sf::Vector2f(cos(radians), sin(radians))));
        }
    }

    void render(sf::RenderWindow& window) override {
        window.draw(array, sf::Transform().translate(position).rotate(angle));
    }

private:
    sf::VertexArray array;
    float shootTimer;
};

// Clase Asteroid
class Asteroid : public Entity {
public:
    Asteroid(sf::Vector2f direction = Asteroid::getRandomDirection(),
             sf::Vector2f position = Asteroid::getRandomPosition())
        : Entity(position, 0), direction(direction), array(sf::LinesStrip, 12) {
        array[0].position = sf::Vector2f(-40, 40);
        array[1].position = sf::Vector2f(-50, 10);
        array[2].position = sf::Vector2f(-10, -20);
        array[3].position = sf::Vector2f(-20, -40);
        array[4].position = sf::Vector2f(10, -40);
        array[5].position = sf::Vector2f(40, -20);
        array[6].position = sf::Vector2f(40, -10);
        array[7].position = sf::Vector2f(10, 0);
        array[8].position = sf::Vector2f(40, 20);
        array[9].position = sf::Vector2f(20, 40);
        array[10].position = sf::Vector2f(0, 30);
        array[11].position = array[0].position;

        for (size_t i = 0; i < array.getVertexCount(); i++) {
            array[i].color = sf::Color::White;
        }
    }

    void update(float deltaTime) override {
        position += ASTEROID_SPEED * direction * deltaTime;
        angle += ASTEROID_SPIN * deltaTime;

        if (position.x <= ASTEROID_W / 2.0f || position.x >= SCREEN_WIDTH - ASTEROID_W / 2.0f) {
            direction.x = -direction.x;
        }
        if (position.y <= ASTEROID_H / 2.0f || position.y >= SCREEN_HEIGHT - ASTEROID_H / 2.0f) {
            direction.y = -direction.y;
        }
    }

    void render(sf::RenderWindow& window) override {
        window.draw(array, sf::Transform().translate(position).rotate(angle));
    }

    static sf::Vector2f getRandomDirection() {
        static std::mt19937 gen(static_cast<unsigned int>(time(0)));
        std::uniform_real_distribution<float> dist(0.0f, 2.0f * M_P);
        float angle = dist(gen);
        return sf::Vector2f(cos(angle), sin(angle));
    }

    static sf::Vector2f getRandomPosition() {
        static std::mt19937 gen(static_cast<unsigned int>(time(0)));
        std::uniform_real_distribution<float> xAxis(ASTEROID_W / 2.0f, SCREEN_WIDTH - ASTEROID_W / 2.0f);
        std::uniform_real_distribution<float> yAxis(ASTEROID_W / 2.0f, SCREEN_HEIGHT - ASTEROID_H / 2.0f);
        return sf::Vector2f(xAxis(gen), yAxis(gen));
    }

private:
    float radius = ASTEROID_W / 2.0f;
    sf::VertexArray array;
    sf::Vector2f direction;
};

// Función principal
int main() {
    sf::RenderWindow window(sf::VideoMode(static_cast<int>(SCREEN_WIDTH), static_cast<int>(SCREEN_HEIGHT)), "Asteroids Game", sf::Style::Close | sf::Style::Titlebar);
    sf::Clock clock;

    entities.push_back(new Player());

    float asteroidSpawnTime = ASTEROID_SPAWN_TIME;

    while (window.isOpen()) {
        float deltaTime = clock.restart().asSeconds();
        sf::Event e{};
        while (window.pollEvent(e)) {
                        if (e.type == sf::Event::Closed) {
                window.close();
            }
        }

        // Lógica de generación de asteroides
        asteroidSpawnTime -= deltaTime;
        if (asteroidSpawnTime <= 0.0f) {
            asteroidSpawnTime = ASTEROID_SPAWN_TIME;
            toAddList.push_back(new Asteroid());
        }

        // Actualización de las entidades
        for (auto& entity : entities) {
            entity->update(deltaTime);
        }

        // Detectar colisiones entre balas y asteroides
        for (auto& entity1 : entities) {
            Bullet* bullet = dynamic_cast<Bullet*>(entity1);
                if (bullet) {
            for (auto& entity2 : entities) {
                Asteroid* asteroid = dynamic_cast<Asteroid*>(entity2);
                    if (asteroid && checkCollision(bullet->position, 5.0f, asteroid->position, ASTEROID_W / 2.0f)) {
                // Colisión detectada
                    toRemoveList.push_back(bullet);
                    toRemoveList.push_back(asteroid);
                }
            }
        }
    }


        // Eliminar entidades marcadas
        for (auto& entity : toRemoveList) {
            auto it = std::find(entities.begin(), entities.end(), entity);
            if (it != entities.end()) {
                delete *it;
                entities.erase(it);
            }
        }
        toRemoveList.clear();

        // Añadir nuevas entidades
        for (auto& entity : toAddList) {
            entities.push_back(entity);
        }
        toAddList.clear();

        // Renderizado
        window.clear();
        for (auto& entity : entities) {
            entity->render(window);
        }
        window.display();
    }

    // Liberar memoria al finalizar el programa
    for (auto& entity : entities) {
        delete entity;
    }

    return 0;
}


