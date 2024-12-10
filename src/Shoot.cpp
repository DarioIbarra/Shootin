#include <SFML/Graphics.hpp>
#include <iostream>
#include <cmath>
#include <vector>
#include <list>

// Constantes
constexpr float M_P = 3.14159265f;
constexpr float TURN_SPEED = 200.0f;
constexpr float PLAYER_SPEED = 200.0f;
constexpr float SHOOT_DELAY = 0.2f;
constexpr float BULLET_SPEED = 400.0f;
constexpr float BULLET_LIFE = 3.0f;

// Clase base para entidades
class Entity {
public:
    Entity(sf::Vector2f position, float angle) : position(position), angle(angle) {}
    virtual void update(float deltaTime) = 0;
    virtual void render(sf::RenderWindow& window) = 0;
    sf::Vector2f position;
    float angle;
};

// Vectores globales para manejar entidades
std::vector<Entity*> entities{};
std::list<std::vector<Entity*>::iterator> toRemoveList{};
std::list<Entity*> toAddList{};

// Clase Bullet
class Bullet : public Entity {
public:
    Bullet(sf::Vector2f position, sf::Vector2f direction)
        : shape(1.0f), direction(direction), Entity(position, 0.0f), lifetime(BULLET_LIFE) {}

    void update(float deltaTime) override {
        lifetime -= deltaTime;
        position += direction * BULLET_SPEED * deltaTime;

        if (lifetime <= 0.0f) {
            toRemoveList.push_back(std::find(entities.begin(), entities.end(), this));
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
};

// Clase Player
class Player : public Entity {
public:
    Player()
        : Entity(sf::Vector2f(500, 500), 0), array(sf::LinesStrip, 5), shootTimer(0) {
        // Coordenadas relativas al centro del jugador
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
    Asteroid()
        : Entity(sf::Vector2f(900, 300), 0), array(sf::LinesStrip, 12) {
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

    void update(float deltaTime) override {}

    void render(sf::RenderWindow& window) override {
        window.draw(array, sf::Transform().translate(position).rotate(angle));
    }

private:
    sf::VertexArray array;
};

// Función principal
int main() {
    sf::RenderWindow window(sf::VideoMode(1200, 900), "Shooting Project",
                            sf::Style::Close | sf::Style::Titlebar);
    sf::Clock clock;

    entities.push_back(new Player());
    entities.push_back(new Asteroid());

    while (window.isOpen()) {
        float deltaTime = clock.restart().asSeconds();
        sf::Event e{};
        while (window.pollEvent(e)) {
            if (e.type == sf::Event::Closed) {
                window.close();
            } else if (e.type == sf::Event::KeyPressed) {
                if (e.key.code == sf::Keyboard::Q) {
                    std::cout << "Entities count: " << entities.size() << std::endl;
                }
            }
        }

        // Limpiar listas de añadir/eliminar
        toAddList.clear();
        toRemoveList.clear();
        window.clear(sf::Color::Black);

        // Actualizar y renderizar entidades
        for (auto entity : entities) {
            entity->update(deltaTime);
            entity->render(window);
        }

        // Eliminar entidades marcadas
        for (const auto& it : toRemoveList) {
            delete *it;
            entities.erase(it);
        }

        // Añadir nuevas entidades
        for (auto& ptr : toAddList) {
            entities.push_back(ptr);
        }

        window.display();
    }

    // Liberar memoria al cerrar
    for (auto entity : entities) {
        delete entity;
    }
    entities.clear();

    return 0;
}

