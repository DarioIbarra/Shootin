#include <SFML/Graphics.hpp>
#include <iostream>
#include <cmath>
#include <vector>
#include <list>

constexpr float M_P = 3.14159265f;

 constexpr float TURN_SPEED = 200.0f;
 constexpr float PLAYER_SPEED = 200.0f;
 constexpr float SHOOT_DELAY = 0.2f;
 constexpr float BULLET_SPEED = 400.0f;
 constexpr float BULLET_LIFE = 3.0f;

class Entity {
    public:
        Entity(sf::Vector2f position, float angle)
            : position(position), angle(angle){}
        virtual void update(float deltaTime) = 0;
        virtual void render(sf::RenderWindow& window) = 0;
        sf::Vector2f position;
        float angle;
};

std::vector<Entity*> entities{};
std::list<std::vector<Entity*>::iterator> toRemoveList{};
std::list<Entity*> toAddList{};

class Bullet : public Entity {
    public:
       Bullet(sf::Vector2f position, sf::Vector2f direction)
            : shape(1.0f), direction(direction), Entity(position, 0.0f), lifetime(BULLET_LIFE){}

        void update(float deltaTime) override {
            lifetime -= deltaTime;
            position += direction * BULLET_SPEED * deltaTime;
            if(lifetime <= 0.0f){
                toRemoveList.push_back(std::find(entities.begin(), entities.end(), this));
                delete this;
            }
        }
        void render(sf::RenderWindow& window) override {
            window.draw(shape, sf::Transform().translate(position));
        }
 
    private:
        sf::CircleShape shape;
        sf::Vector2f direction;
        float lifetime;
};

class Player : public Entity {
public:
    Player()
        : Entity(sf::Vector2f(500, 500),0), array(sf::Quads, 4), shootTimer() {
        // Coordenadas relativas al centro del jugador
        array[0].position = sf::Vector2f(20, 0);
        array[1].position = sf::Vector2f(-30, -20);
        array[2].position = sf::Vector2f(-15, 0);
        array[3].position = sf::Vector2f(-30, 20);

        // Asignar colores para cada vértice (opcional)
        for (size_t i = 0; i < array.getVertexCount(); i++) {
            array[i].color = sf::Color::White;
        }
    }
    void update(float deltaTime) override {
        shootTimer -= deltaTime;
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
            angle -= TURN_SPEED * deltaTime;
        }
        if(sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
            angle += TURN_SPEED * deltaTime;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)){
            float radians = angle * (M_P/180.0f);
            position.x += cos(radians) * PLAYER_SPEED * deltaTime;
            position.y += sin(radians) * PLAYER_SPEED * deltaTime;

        }

        if(sf::Keyboard::isKeyPressed(sf::Keyboard::Space) && shootTimer <= 0.0f) {
            shootTimer = SHOOT_DELAY;
            float radians = angle * (M_P/180.0f);
            toAddList.push_back(new Bullet(position, sf::Vector2f(cos(radians),sin(radians))));
        }
            
    }

    void render(sf::RenderWindow& window) override {
        // Dibujar el vértice con la transformación aplicada
        window.draw(array, sf::Transform().translate(position).rotate(angle));
    }

private:
    sf::VertexArray array;
    float shootTimer;
};

int main() {
    sf::RenderWindow window(sf::VideoMode(1200, 900), "Shooting Project",
        sf::Style::Close | sf::Style::Titlebar);
    sf::Clock clock;

    entities.push_back(new Player());

    while (window.isOpen()) {
        float deltaTime = clock.restart().asSeconds();
        sf::Event e{};
        while (window.pollEvent(e)) {
            if (e.type == sf::Event::Closed) {
                window.close();
            } else if (e.type = sf::Event::KeyPressed){
                if(e.key.code = sf::Keyboard::Q){
                    printf("%d\n", entities.size());
                }
            }
        }
        toAddList.clear();
        toRemoveList.clear();
        window.clear(sf::Color::Black); // Limpiar la ventana con color negro

        for(size_t i =0; i<entities.size(); i++){
            entities[i]->update(deltaTime);
            entities[i]->render(window);
        }
        for (const auto& it : toRemoveList){
            delete* it;
            entities.erase(it);
        }
        for (auto& ptr : toAddList){
            entities.push_back(ptr);
        }
        
       
        window.display(); // Mostrar todo lo dibujado
    }
}
