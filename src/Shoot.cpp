#include <SFML/Graphics.hpp>
#include <iostream>
#include <cmath>
#include <vector>
#include <list>
#include <random>
#include <algorithm>
#include <SFML/Audio.hpp>
#include <functional>

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

// Función auxiliar para verificar colisión circular
bool checkCollision(const sf::Vector2f& pos1, float radius1, const sf::Vector2f& pos2, float radius2) {
    float dx = pos1.x - pos2.x;
    float dy = pos1.y - pos2.y;
    float distanceSquared = dx * dx + dy * dy;
    float radiusSum = radius1 + radius2;
    return distanceSquared <= (radiusSum * radiusSum);
}

// Clase para manejar métodos de detección de colisión
class CollisionDriver {
public:
    using CollisionMethod = std::function<bool(const std::vector<sf::Vector2f>&, const std::vector<sf::Vector2f>&)>;

    CollisionDriver() {
        // Registrar el método SAT por defecto
        methods["sat"] = polygonIntersectionSAT;
    }

    void addMethod(const std::string& name, CollisionMethod method) {
        methods[name] = method;
    }

    bool checkCollision(const std::string& method, const std::vector<sf::Vector2f>& poly1, const std::vector<sf::Vector2f>& poly2) {
        if (methods.find(method) == methods.end()) {
            throw std::runtime_error("Método no encontrado: " + method);
        }
        return methods[method](poly1, poly2);
    }

private:
    std::map<std::string, CollisionMethod> methods;

    // Implementación del algoritmo SAT
    static bool polygonIntersectionSAT(const std::vector<sf::Vector2f>& poly1, const std::vector<sf::Vector2f>& poly2) {
        auto getAxes = [](const std::vector<sf::Vector2f>& poly) {
            std::vector<sf::Vector2f> axes;
            for (size_t i = 0; i < poly.size(); ++i) {
                sf::Vector2f p1 = poly[i];
                sf::Vector2f p2 = poly[(i + 1) % poly.size()];
                sf::Vector2f edge = { p2.x - p1.x, p2.y - p1.y };
                sf::Vector2f normal = { -edge.y, edge.x };
                float length = std::sqrt(normal.x * normal.x + normal.y * normal.y);
                axes.push_back({ normal.x / length, normal.y / length });
            }
            return axes;
        };

        auto projectPolygon = [](const std::vector<sf::Vector2f>& poly, const sf::Vector2f& axis) {
            float min = poly[0].x * axis.x + poly[0].y * axis.y;
            float max = min;
            for (const auto& vertex : poly) {
                float projection = vertex.x * axis.x + vertex.y * axis.y;
                if (projection < min) min = projection;
                if (projection > max) max = projection;
            }
            return std::make_pair(min, max);
        };

        auto overlap = [](const std::pair<float, float>& proj1, const std::pair<float, float>& proj2) {
            return proj1.second >= proj2.first && proj2.second >= proj1.first;
        };

        auto axes1 = getAxes(poly1);
        auto axes2 = getAxes(poly2);

        for (const auto& axis : axes1) {
            if (!overlap(projectPolygon(poly1, axis), projectPolygon(poly2, axis))) {
                return false;  // Separación encontrada
            }
        }

        for (const auto& axis : axes2) {
            if (!overlap(projectPolygon(poly1, axis), projectPolygon(poly2, axis))) {
                return false;  // Separación encontrada
            }
        }

        return true;  // No se encontró separación, hay intersección
    }
};

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
        shootSoundBuffer.loadFromFile("assets/music/Shoot.wav");
        ShootSound.setBuffer(shootSoundBuffer);
        ShootSound.setVolume(100); // Volumen máximo

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
            ShootSound.play();
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
    sf::SoundBuffer shootSoundBuffer;
    sf::Sound ShootSound;
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

std::vector<Asteroid*> inicioAsteroids;

// Función principal
int main() {
    sf::RenderWindow window(sf::VideoMode(static_cast<int>(SCREEN_WIDTH), static_cast<int>(SCREEN_HEIGHT)), "Asteroids Game", sf::Style::Close | sf::Style::Titlebar);
    sf::Clock clock;

    // Música de fondo
    sf::Music backgroundMusic;
    if (!backgroundMusic.openFromFile("assets/music/message-of-the-sun-72756.mp3")) {
        std::cerr << "Error al cargar el archivo de música de fondo." << std::endl;
        return -1;
    }
    backgroundMusic.setLoop(true);
    backgroundMusic.setVolume(70);
    backgroundMusic.play();

    // Entidades del juego
    std::vector<Entity*> entities;
    entities.push_back(new Player());

    int score = 0; // Puntaje inicial

    // Cargar la fuente
    sf::Font font;
    if (!font.loadFromFile("assets/fonts/Minecraft.ttf")) {
        std::cerr << "Cannot load the font." << std::endl;
        return -1;
    }

    // Crear el texto para el puntaje
    sf::Text scoreText;
    scoreText.setFont(font);
    scoreText.setCharacterSize(40); // Tamaño de la fuente
    scoreText.setFillColor(sf::Color::White); // Color blanco
    scoreText.setPosition(50.0f, 50.0f); // Posición del puntaje

    float asteroidSpawnTime = ASTEROID_SPAWN_TIME;
    sf::SoundBuffer explosionBuffer;
    sf::Sound explosionSound;
    if (!explosionBuffer.loadFromFile("assets/music/pop.mp3")) {
        std::cerr << "Error al cargar el archivo de sonido de explosión." << std::endl;
        return -1;
    }
    explosionSound.setBuffer(explosionBuffer);
    explosionSound.setVolume(100);

    bool gameOver = false;  // Variable que indica si el juego ha terminado
    bool gameStarted = false; // Variable que indica si el juego ha iniciado

    // Asteroides en la pantalla de inicio
    for (int i = 0; i < 10; i++) {
        inicioAsteroids.push_back(new Asteroid());
        inicioAsteroids[i]->position = sf::Vector2f(fmod(rand(), SCREEN_WIDTH), fmod(rand(), SCREEN_HEIGHT));
    }

    while (window.isOpen()) {
        float deltaTime = clock.restart().asSeconds();
        sf::Event e{};
        while (window.pollEvent(e)) {
            if (e.type == sf::Event::Closed) {
                window.close();
            }

            // Detectar si el jugador presiona Enter para iniciar el juego
            if (!gameStarted && e.type == sf::Event::KeyPressed && e.key.code == sf::Keyboard::Return) {
                gameStarted = true;
            }

            // Detectar si el jugador presiona Space para reiniciar el juego
            if (gameOver && e.type == sf::Event::KeyPressed && e.key.code == sf::Keyboard::P) {
                // Reiniciar el juego
                gameOver = false;
                score = 0;

                // Limpiar las entidades y reiniciar
                for (auto& entity : entities) {
                    delete entity;
                }
                entities.clear();
                entities.push_back(new Player());  // Crear el jugador nuevamente
            }

            // Detectar si el jugador presiona E para salir en el Game Over
            if (gameOver && e.type == sf::Event::KeyPressed && e.key.code == sf::Keyboard::E) {
                window.close();
            }
        }

        // Lógica de generación de asteroides
        if (gameStarted) {
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
                            score += 20; // Incrementar puntaje al destruir un asteroide
                        }
                    }
                }
            }

            // Detectar colisión entre el jugador y los asteroides
            for (auto& entity : entities) {
                Player* player = dynamic_cast<Player*>(entity);
                if (player) {
                    for (auto& entity2 : entities) {
                        Asteroid* asteroid = dynamic_cast<Asteroid*>(entity2);
                        if (asteroid && checkCollision(player->position, PLAYER_W / 2.0f, asteroid->position, ASTEROID_W / 2.0f)) {
                            // Colisión detectada entre el jugador y un asteroide
                            gameOver = true;  // El juego ha terminado
                        }
                    }
                }
            }

            // Eliminar entidades marcadas para ser eliminadas
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

            // Actualizar el texto del puntaje
            scoreText.setString("Score: " + std::to_string(score));
        }

        // Renderizado
        if (gameOver) {
            // Limpiar la pantalla con fondo negro cuando el juego haya terminado
            window.clear(sf::Color::Black);

            // Mostrar el mensaje de "Game Over"
            sf::Text gameOverText;
            gameOverText.setFont(font);
            gameOverText.setCharacterSize(80);
            gameOverText.setFillColor(sf::Color::White);
            gameOverText.setString("GAME OVER");
            gameOverText.setPosition(SCREEN_WIDTH / 2 - gameOverText.getLocalBounds().width / 2, 
                                     SCREEN_HEIGHT / 2 - gameOverText.getLocalBounds().height / 2);
            window.draw(gameOverText);

            // Mostrar la opción de reiniciar
            sf::Text restartText;
            restartText.setFont(font);
            restartText.setCharacterSize(40);
            restartText.setFillColor(sf::Color::White);
            restartText.setString("Press P to Restart");
            restartText.setPosition(SCREEN_WIDTH / 2 - restartText.getLocalBounds().width / 2, 
                                    SCREEN_HEIGHT / 2 + 100);
            window.draw(restartText);

            // Mostrar la opción de salir
            sf::Text exitText;
            exitText.setFont(font);
            exitText.setCharacterSize(40);
            exitText.setFillColor(sf::Color::White);
            exitText.setString("Press E to Exit");
            exitText.setPosition(SCREEN_WIDTH / 2 - exitText.getLocalBounds().width / 2, 
                                SCREEN_HEIGHT / 2 + 150);
            window.draw(exitText);
        } else if (!gameStarted) {
            // Limpiar la pantalla con fondo negro cuando el juego no ha iniciado
            window.clear(sf::Color::Black);

            // Mostrar el título del juego
            sf::Text titleText;
            titleText.setFont(font);
            titleText.setCharacterSize(80);
            titleText.setFillColor(sf::Color::White);
            titleText.setString("DART PROYECT");
            titleText.setPosition(SCREEN_WIDTH / 2 - titleText.getLocalBounds().width / 2, 
                                  SCREEN_HEIGHT / 2 - titleText.getLocalBounds().height / 2);
            window.draw(titleText);

            // Mostrar la opción de iniciar el juego
            sf::Text startText;
            startText.setFont(font);
            startText.setCharacterSize(40);
            startText.setFillColor(sf::Color::White);
            startText.setString("Press Enter to Start");
            startText.setPosition(SCREEN_WIDTH / 2 - startText.getLocalBounds().width / 2, 
                                  SCREEN_HEIGHT / 2 + 100);
            window.draw(startText);

            // Mostrar los asteroides en la pantalla de inicio
            for (auto& asteroid : inicioAsteroids) {
                asteroid->render(window);
            }
        } else {
            // Limpiar la pantalla y mostrar el juego normal si no está en "Game Over"
            window.clear();
            for (auto& entity : entities) {
                entity->render(window);
            }
            window.draw(scoreText); // Dibujar el puntaje
        }

        window.display();
    }

    // Liberar memoria
    for (auto& entity : entities) {
        delete entity;
    }

    for (auto& asteroid : inicioAsteroids) {
        delete asteroid;
    }

    return 0;
}





